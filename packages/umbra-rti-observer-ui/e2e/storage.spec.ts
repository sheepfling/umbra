import { test, expect } from "./fixtures";
import { navigate, seedEvents } from "./support";

test.describe("whole-record storage coverage", () => {
  test.use({ backendVariant: "empty", observerArgs: ["--max-federates", "2", "--max-time-snapshots", "1", "--max-evidence-bytes", "262144"] });
  test("expired roster metadata does not erase retained attribution or silently switch a selection", async ({ page, request }) => {
    await seedEvents(request, ["A", "B", "C"].map((federate) => ({ event_type: "object.updated", source: "e2e", federate, object_key: federate, attributes: { value: 1 } })));
    await page.goto("/#federates");
    await page.getByRole("complementary", { name: "Observed federates" }).getByRole("button", { name: /A/ }).click();
    await expect(page.getByRole("heading", { name: "A", exact: true })).toBeVisible();
    await expect(page.getByText(/Roster metadata is not retained/)).toBeVisible();
    await expect(page.getByRole("article", { name: "Attributed events", exact: true }).locator("strong")).toHaveText("1");
    await request.delete("/api/events");
    await expect(page.getByRole("heading", { name: "A", exact: true })).toBeVisible();
    await navigate(page, "Diagnostics");
    await expect(page.getByRole("region", { name: "Storage coverage" })).toBeVisible();
  });
  test("pending request remains inspectable and can match its exact grant after latest snapshot expiry", async ({ page, request }) => {
    for (const federate of ["A", "B"]) await seedEvents(request, [{ event_type: "time.advance_requested", source: "e2e", federate, federation: "F",
      time_observation: { kind: "request", session_id: `${federate}-join`, request_id: "request", logical_time: 10 } }]);
    await page.goto("/#time");
    const select = page.getByLabel("Time perspective", { exact: true });
    const choice = select.locator("option").filter({ hasText: /^A ·/ });
    await expect(choice).toContainText("snapshot expired");
    await select.selectOption((await choice.getAttribute("value"))!);
    await expect(page.getByRole("heading", { name: "No persistent time observations", exact: true })).toBeVisible();
    await expect(page.getByRole("article", { name: "Time request ledger" })).toContainText("pending");
    await seedEvents(request, [{ event_type: "time.advance_granted", source: "e2e", federate: "A", federation: "F",
      time_observation: { kind: "grant", session_id: "A-join", request_id: "request", logical_time: 10 } }]);
    await expect(page.getByRole("article", { name: "Time request ledger" })).toContainText("granted");
    await expect(page.getByRole("article", { name: "Reported time state" })).toContainText("Unknown");
  });
  test("oversized evidence reports whole-record loss, advances the empty-window cursor and recovers", async ({ page, request }, testInfo) => {
    await seedEvents(request, [{ event_type: "callback.invoked", details: { too_big: "x".repeat(300000) } }]);
    const gap = await (await request.get("/api/events?after=0&limit=2")).json();
    expect(gap.gap).toBeTruthy(); expect(gap.next_after).toBe(1);
    await page.goto("/#diagnostics");
    await expect(page.getByRole("region", { name: "Storage coverage" })).toContainText("events");
    await expect(page.getByText(/Storage limits have expired/)).toBeVisible();
    await seedEvents(request, [{ event_type: "callback.invoked", details: { marker: "after-expiry" } }]);
    await navigate(page, "Events");
    await page.getByLabel("Search events", { exact: true }).fill("after-expiry");
    await expect(page.getByRole("region", { name: "Event explorer" }).locator("tbody tr")).toHaveCount(1);
    await page.screenshot({ path: testInfo.outputPath("storage-recovery.png"), fullPage: true });
  });
});

test.describe("older backend storage coverage", () => {
  test.use({ backendVariant: "java-full" });
  for (const mode of ["disabled", "unsupported"] as const) test(`${mode} storage inspection keeps observation views independent`, async ({ page }) => {
    await page.route("**/api/state?**", async (route) => {
      const response = await route.fetch(); const body = await response.json();
      body.capabilities.storage_inspection.state = mode;
      await route.fulfill({ json: body });
    });
    await page.goto("/#diagnostics");
    await expect(page.getByText(`Storage coverage ${mode}:`, { exact: false })).toBeVisible();
    await expect(page.getByRole("region", { name: "Storage coverage" })).toHaveCount(0);
    await navigate(page, "Events");
    await expect(page.getByRole("region", { name: "Event explorer" })).toBeVisible();
    await page.unrouteAll({ behavior: "wait" });
  });
  test("missing storage models stay unknown and do not disable independent screens", async ({ page }) => {
    await page.route("**/api/state?**", async (route) => {
      const response = await route.fetch(); const body = await response.json();
      delete body.storage; delete body.capabilities.storage_inspection;
      await route.fulfill({ json: body });
    });
    await page.goto("/#diagnostics");
    await expect(page.getByText(/Storage budgets and expiry reasons are not reported/)).toBeVisible();
    await navigate(page, "Objects");
    await expect(page.getByRole("heading", { name: "Object workspace", exact: true })).toBeVisible();
    await page.unrouteAll({ behavior: "wait" });
  });
});
