import type { components } from "../src/api/generated";
import type { StateResponse } from "../src/api/types";
import { test, expect } from "./fixtures";
import { navigate, seedEvents } from "./support";

type Observation = components["schemas"]["SynchronizationObservation"] | components["schemas"]["PersistenceObservation"];
const base = { operation_id: "op-1", session_id: "s1", identity_namespace: "provider", federation_id: "fed", label: "Checkpoint", scope: "federation" as const };
function report(observation: Observation, source = "e2e"): components["schemas"]["EventInput"] {
  return { event_type: "coordination.observed", source, coordination: observation };
}

test.describe("explicit coordination inspection", () => {
  test.use({ backendVariant: "empty" });

  test("local save completion does not imply collective success and timeline keeps all scopes", async ({ page, request }) => {
    await seedEvents(request, [
      report({ ...base, kind: "save", phase: "requested" }),
      report({ ...base, kind: "save", phase: "begun", scope: "local", subject: { participant_id: "a", membership_id: "j1", display_name: "Alice" } }),
      report({ ...base, kind: "save", phase: "completed", scope: "local", subject: { participant_id: "a", membership_id: "j1", display_name: "Alice" } }),
      report({ ...base, kind: "save", phase: "failed", reason: "Peer storage failed", participants: [{ identity: { participant_id: "b", membership_id: "j2", display_name: "Bob" }, status: "failed", reason: "Disk unavailable" }] }),
    ]);
    await page.goto("/#persistence");
    const table = page.getByRole("article", { name: "Latest coordination reports" });
    const sequence = page.getByRole("article", { name: "Coordination phase sequence" });
    await expect(table.locator("tbody tr")).toHaveCount(2);
    await expect(table).toContainText("completed");
    await expect(table).toContainText("failed");
    await table.getByText("Reported participants (1)").click();
    await expect(table).toContainText("Bob");
    await expect(table).toContainText("Disk unavailable");
    await page.getByLabel("Coordination phase", { exact: true }).selectOption("completed");
    await expect(table.locator("tbody tr")).toHaveCount(1);
    await expect(table).toContainText("local");
    await table.getByRole("button", { name: "Follow operation" }).click();
    const progress = page.getByRole("article", { name: "Selected coordination operation" });
    await expect(progress).toContainText("Alice / j1");
    await expect(progress).toContainText("Federation report");
    await expect(progress).toContainText("Peer storage failed");
    await expect(sequence.locator(".coordination-timeline > li")).toHaveCount(4);
    await expect(sequence).toContainText("Peer storage failed");
    await page.getByLabel("Coordination scope").selectOption("federation");
    await expect(table.locator("tbody tr")).toHaveCount(0);
    await expect(sequence.locator(".coordination-timeline > li")).toHaveCount(4);
  });

  test("synchronization registration failures, reused labels and joins remain distinct", async ({ page, request }) => {
    await seedEvents(request, [
      report({ ...base, kind: "synchronization", phase: "registration-failed", reason: "Label already registered" }),
      report({ ...base, kind: "synchronization", phase: "announced", operation_id: "op-2" }),
      report({ ...base, kind: "synchronization", phase: "achieved", operation_id: "op-2", scope: "local", subject: { participant_id: "a", membership_id: "j1", display_name: "Same" } }),
      report({ ...base, kind: "synchronization", phase: "achieved", operation_id: "op-2", scope: "local", subject: { participant_id: "a", membership_id: "j2", display_name: "Same" } }),
      report({ ...base, kind: "synchronization", phase: "registered" }, "second-reporter"),
    ]);
    await page.goto("/#synchronization");
    const table = page.getByRole("article", { name: "Latest coordination reports" });
    await expect(table.locator("tbody tr")).toHaveCount(5);
    await expect(table).not.toContainText("completed");
    await page.getByLabel("Search coordination reports").fill("already registered");
    await expect(table.locator("tbody tr")).toHaveCount(1);
    await page.getByLabel("Search coordination reports").clear();
    await page.getByLabel("Coordination operation").selectOption(JSON.stringify(["e2e", "s1", "provider", "fed", "synchronization", "op-2"]));
    await expect(table.locator("tbody tr")).toHaveCount(3);
    await expect(table).toContainText("a / j1");
    await expect(table).toContainText("a / j2");
  });

  test("pause, complete replacement and clear preserve operation selection", async ({ page, request }) => {
    await seedEvents(request, [report({ ...base, kind: "restore", phase: "begun", participants: [] })]);
    await page.goto("/#persistence");
    const table = page.getByRole("article", { name: "Latest coordination reports" });
    await expect(table.locator("tbody tr")).toHaveCount(1);
    await table.getByText("Reported participants (0)").click();
    await expect(table).toContainText("Explicit empty participant report");
    await table.getByRole("button", { name: "Follow operation" }).click();
    await page.getByRole("button", { name: "Pause view" }).click();
    await seedEvents(request, [report({ ...base, kind: "restore", phase: "completed" })]);
    await expect(table.getByText("begun", { exact: true })).toBeVisible();
    await page.getByRole("button", { name: "Resume live view" }).click();
    await expect(table.getByText("completed", { exact: true })).toBeVisible();
    await expect(table).toContainText("Participant progress unknown");
    await request.delete("/api/events");
    await page.getByRole("button", { name: "Sync now" }).click();
    await expect(page.getByLabel("Coordination operation").locator("option:checked")).toHaveText("Selected operation no longer reported");
    await expect(table.locator("tbody tr")).toHaveCount(0);
    await expect(page.locator(".coordination-timeline > li")).toHaveCount(0);
  });

  test("bounded latest reports and independent phase history disclose both losses", async ({ page, request }) => {
    await seedEvents(request, Array.from({ length: 2001 }, (_, index) => report({ ...base, kind: "save", phase: "requested", operation_id: `op-${index}` })));
    await page.goto("/#persistence");
    await expect(page.getByText(/Coordination coverage is incomplete: 1001 scoped snapshots and 1 transitions expired/)).toBeVisible();
    const rows = page.getByRole("article", { name: "Latest coordination reports" }).locator("tbody tr");
    await expect(rows).toHaveCount(100);
    await expect(page.locator(".coordination-timeline > li")).toHaveCount(100);
    await page.getByRole("button", { name: "Show 100 more coordination reports" }).click();
    await page.getByRole("button", { name: "Show 100 earlier phase reports" }).click();
    await expect(rows).toHaveCount(200);
    await expect(page.locator(".coordination-timeline > li")).toHaveCount(200);
  });

  test("operation drill-down pages scopes and participant reports independently", async ({ page, request }) => {
    const participants = Array.from({ length: 40 }, (_, index) => ({ identity: { participant_id: `peer-${index}`, membership_id: `join-${index}` }, status: "unknown" as const }));
    await seedEvents(request, Array.from({ length: 25 }, (_, index) => report({ ...base, kind: "save", phase: "begun",
      scope: "local", subject: { participant_id: `local-${index}`, membership_id: `scope-${index}` }, participants })));
    await page.goto("/#persistence");
    await page.getByRole("article", { name: "Latest coordination reports" }).getByRole("button", { name: "Follow operation" }).first().click();
    const progress = page.getByRole("article", { name: "Selected coordination operation" });
    await expect(progress.locator(".coordination-scope-card")).toHaveCount(20);
    await expect(progress.locator(".coordination-participant-grid li")).toHaveCount(500);
    await progress.getByRole("button", { name: "Show 25 more participant reports" }).first().click();
    await expect(progress.locator(".coordination-participant-grid li")).toHaveCount(515);
    await progress.getByRole("button", { name: "Show 20 more scopes" }).click();
    await expect(progress.locator(".coordination-scope-card")).toHaveCount(25);
    await expect(progress.locator(".coordination-participant-grid li")).toHaveCount(640);
  });
});

test.describe("independent Java coordination models", () => {
  test.use({ backendVariant: "java-full" });
  test("Java local completion and federation failure render without MOM dependence", async ({ page }, testInfo) => {
    await page.goto("/#persistence");
    const table = page.getByRole("article", { name: "Latest coordination reports" });
    await expect(table.locator("tbody tr")).toHaveCount(2);
    await expect(table).toContainText("completed");
    await expect(table).toContainText("failed");
    await expect(page.locator(".coordination-timeline > li")).toHaveCount(3);
    await page.screenshot({ path: testInfo.outputPath("coordination.png"), fullPage: true });
    await navigate(page, "Synchronization");
    await expect(table.locator("tbody tr")).toHaveCount(2);
    await expect(table).toContainText("registered");
    await expect(table).toContainText("completed");
  });
  for (const mode of ["older", "disabled", "unsupported", "partial"] as const) test(`${mode} coordination capabilities do not disable independent views`, async ({ page }) => {
    await page.route("**/api/state?**", async (route) => {
      const response = await route.fetch();
      const state = await response.json() as StateResponse;
      // Keep legacy provider-level persistence support. It never promised the
      // additive typed inspection model used by this new workspace.
      if (mode === "older") { delete state.capabilities.synchronization; delete state.capabilities.persistence_inspection; delete state.inspectors.coordination; }
      else for (const feature of ["synchronization", "persistence_inspection"]) state.capabilities[feature] = { ...state.capabilities[feature]!, state: mode, reason: "Fixture coordination coverage" };
      await route.fulfill({ json: state });
    });
    await page.goto("/#synchronization");
    if (mode === "partial") await expect(page.getByText("Partial synchronization coverage: Fixture coordination coverage")).toBeVisible();
    else await expect(page.getByRole("heading", { name: "Synchronization unavailable" })).toBeVisible();
    await navigate(page, "Save / restore");
    if (mode === "partial") await expect(page.getByText("Partial persistence coverage: Fixture coordination coverage")).toBeVisible();
    else await expect(page.getByRole("heading", { name: "Save and restore unavailable" })).toBeVisible();
    await navigate(page, "Events");
    await expect(page.getByRole("region", { name: "Event explorer" }).locator("tbody tr").first()).toBeVisible();
  });
});

test.describe("limited provider coordination", () => {
  test.use({ backendVariant: "limited" });
  test("unsupported synchronization and disabled persistence keep events and time useful", async ({ page }) => {
    await page.goto("/#synchronization");
    await expect(page.getByRole("heading", { name: "Synchronization unavailable" })).toBeVisible();
    await navigate(page, "Save / restore");
    await expect(page.getByRole("heading", { name: "Save and restore unavailable" })).toBeVisible();
    await navigate(page, "Events");
    await expect(page.getByRole("region", { name: "Event explorer" }).locator("tbody tr").first()).toBeVisible();
  });
});
