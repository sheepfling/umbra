import type { CommandCatalog, CommandHistory, CommandRequest } from "../src/api/types";
import { test, expect } from "./fixtures";
import { eventCount, navigate } from "./support";

test.describe("owned connection recovery", () => {
  test.use({ observerArgs: ["--showcase-control-fault-once", "connect"] });

  test("inspection does not restore authority; explicit reconciliation preserves the failed operation and enables justified actions", async ({ page, request }) => {
    await page.goto("/#actions");
    const state = page.getByRole("article", { name: "Controlled session state" });
    const recovery = page.getByRole("article", { name: "Session recovery" });
    const lifecycle = page.getByRole("article", { name: "Session lifecycle actions" });
    await lifecycle.getByRole("button", { name: "Connect", exact: true }).click();
    await expect(state.getByText("unknown", { exact: true })).toBeVisible();
    const history = page.getByRole("article", { name: "Command results" });
    await expect(history.getByText("failed", { exact: true })).toHaveCount(1);
    const original = await (await request.get("/api/commands")).json() as CommandHistory;
    await recovery.getByRole("button", { name: "Inspect session", exact: true }).click();
    await expect(recovery.getByRole("region", { name: "Latest owned session inspection" })).toContainText("connection: connected · membership: not-joined");
    await expect(state.getByText("unknown", { exact: true })).toBeVisible();
    await expect(lifecycle.getByRole("button", { name: "Connect", exact: true })).toBeDisabled();
    const reconcile = recovery.getByRole("button", { name: "Reconcile session", exact: true });
    await expect(reconcile).toBeDisabled();
    await recovery.getByRole("checkbox", { name: /I confirm reconcile session/ }).check();
    await reconcile.click();
    await expect(state.getByText("connected", { exact: true })).toBeVisible();
    await expect(recovery.getByRole("heading", { name: "Control state reconciled" })).toBeVisible();
    const after = await (await request.get("/api/commands")).json() as CommandHistory;
    expect(after.records.find((record) => record.command_id === original.records[0]?.command_id)).toEqual(original.records[0]);
    expect(after.records.filter((record) => record.request.action === "session.connect")).toHaveLength(1);
    await lifecycle.getByRole("button", { name: /^Join federation available$/ }).click();
    await lifecycle.getByLabel("Federate name", { exact: true }).fill("RecoveredOwner");
    await lifecycle.getByRole("button", { name: "Join federation", exact: true }).click();
    await expect(state.getByText("joined", { exact: true })).toBeVisible();
    await page.reload();
    await expect(history.getByText("failed", { exact: true })).toHaveCount(1);
  });

  test("a changed target revision invalidates confirmation and stale recovery has no provider effect", async ({ page, request }) => {
    await page.goto("/#actions");
    await page.getByRole("button", { name: "Connect", exact: true }).click();
    const state = page.getByRole("article", { name: "Controlled session state" });
    await expect(state.getByText("unknown", { exact: true })).toBeVisible();
    const recovery = page.getByRole("article", { name: "Session recovery" });
    const confirmation = recovery.getByRole("checkbox", { name: /I confirm reconcile session/ });
    await confirmation.check();
    const before = await (await request.get("/api/commands/catalog")).json() as CommandCatalog;
    const target = before.targets[0]!;
    const response = await request.post("/api/commands", { data: { action: "session.inspect", target_id: target.target_id,
      request_id: "other-client-preview", expected_version: target.version, parameters: {} } });
    expect(response.status()).toBe(202);
    await expect(confirmation).not.toBeChecked();
    await expect(recovery.getByRole("button", { name: "Reconcile session", exact: true })).toBeDisabled();
    await page.route("**/api/commands", async (route) => {
      if (route.request().method() !== "POST") return route.continue();
      const payload = route.request().postDataJSON() as CommandRequest;
      const result = await route.fetch({ postData: JSON.stringify({ ...payload, expected_version: target.version }) });
      await route.fulfill({ response: result });
    });
    await confirmation.check();
    await recovery.getByRole("button", { name: "Reconcile session", exact: true }).click();
    await expect(page.getByRole("article", { name: "Command results" })).toContainText("Owned target incarnation/revision changed");
    await expect(state.getByText("unknown", { exact: true })).toBeVisible();
  });

  test("paused recovery stays read-only while ordinary observations continue", async ({ page, request }) => {
    let posts = 0;
    page.on("request", (request) => { if (request.method() === "POST" && request.url().endsWith("/api/commands")) posts++; });
    await page.goto("/#actions");
    await page.getByRole("button", { name: "Connect", exact: true }).click();
    const recovery = page.getByRole("article", { name: "Session recovery" });
    await expect(recovery.getByRole("button", { name: "Inspect session", exact: true })).toBeEnabled();
    await page.getByRole("button", { name: "Pause view" }).click();
    await expect(recovery.getByRole("button", { name: "Inspect session", exact: true })).toBeDisabled();
    await expect(recovery.getByRole("checkbox", { name: /I confirm reconcile/ })).toBeDisabled();
    const before = await eventCount(request);
    await expect.poll(() => eventCount(request)).toBeGreaterThan(before);
    expect(posts).toBe(1);
    await page.getByRole("button", { name: "Resume live view" }).click();
    await expect(recovery.getByRole("button", { name: "Inspect session", exact: true })).toBeEnabled();
  });
});

test.describe("limited observations and owned join recovery", () => {
  test.use({ backendVariant: "limited", observerArgs: ["--showcase-control-fault-once", "join"] });
  test("a join that took effect is reconciled without a second join or MOM", async ({ page, request }) => {
    await page.goto("/#actions");
    const lifecycle = page.getByRole("article", { name: "Session lifecycle actions" });
    const state = page.getByRole("article", { name: "Controlled session state" });
    await lifecycle.getByRole("button", { name: "Connect", exact: true }).click();
    await expect(state.getByText("connected", { exact: true })).toBeVisible();
    await lifecycle.getByRole("button", { name: /^Join federation available$/ }).click();
    await lifecycle.getByLabel("Federate name", { exact: true }).fill("TookEffect");
    await lifecycle.getByRole("button", { name: "Join federation", exact: true }).click();
    await expect(state.getByText("unknown", { exact: true })).toBeVisible();
    const recovery = page.getByRole("article", { name: "Session recovery" });
    await recovery.getByRole("checkbox", { name: /I confirm reconcile session/ }).check();
    await recovery.getByRole("button", { name: "Reconcile session", exact: true }).click();
    await expect(state.getByText("joined", { exact: true })).toBeVisible();
    await expect(state.getByText("TookEffect", { exact: true })).toBeVisible();
    const history = await (await request.get("/api/commands")).json() as CommandHistory;
    expect(history.records.filter((record) => record.request.action === "federate.join")).toHaveLength(1);
    await lifecycle.getByRole("button", { name: /^Resign available$/ }).click();
    await lifecycle.getByRole("checkbox", { name: /I confirm resign/ }).check();
    await lifecycle.getByRole("button", { name: "Resign", exact: true }).click();
    await expect(state.getByText("connected", { exact: true })).toBeVisible();
  });
});

test.describe("partial owned inspection", () => {
  test.use({ observerArgs: ["--showcase-control-fault-once", "connect", "--showcase-control-inspection", "partial"] });
  test("incomplete membership remains unknown after a successful query and does not disable observation views", async ({ page }) => {
    await page.goto("/#actions");
    await page.getByRole("button", { name: "Connect", exact: true }).click();
    const recovery = page.getByRole("article", { name: "Session recovery" });
    await expect(recovery.getByRole("button", { name: "Inspect session", exact: true })).toBeEnabled();
    await recovery.getByRole("checkbox", { name: /I confirm reconcile session/ }).check();
    await recovery.getByRole("button", { name: "Reconcile session", exact: true }).click();
    await expect(recovery.getByRole("region", { name: "Latest owned session inspection" })).toContainText("membership: unknown");
    await expect(page.getByRole("article", { name: "Controlled session state" }).getByText("unknown", { exact: true })).toBeVisible();
    await expect(page.getByRole("button", { name: "Connect", exact: true })).toBeDisabled();
    await navigate(page, "Events");
    await expect(page.getByRole("region", { name: "Event explorer" }).locator("tbody tr").first()).toBeVisible();
  });
});

test.describe("no owned inspection", () => {
  test.use({ observerArgs: ["--showcase-control-inspection", "disabled"] });
  test("unsupported recovery leaves ordinary lifecycle controls and independent views usable", async ({ page }) => {
    await page.goto("/#actions");
    await expect(page.getByRole("article", { name: "Session recovery" })).toContainText("This adapter does not offer");
    await page.getByRole("button", { name: "Connect", exact: true }).click();
    await expect(page.getByRole("article", { name: "Controlled session state" }).getByText("connected", { exact: true })).toBeVisible();
    await navigate(page, "Events");
    await expect(page.getByRole("region", { name: "Event explorer" }).locator("tbody tr").first()).toBeVisible();
  });
});
