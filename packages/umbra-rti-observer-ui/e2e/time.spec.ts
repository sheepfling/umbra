import type { StateResponse } from "../src/api/types";
import { test, expect } from "./fixtures";
import { navigate, seedEvents } from "./support";

test.describe("persistent time investigation", () => {
  test.use({ backendVariant: "empty" });

  test("state, pending ages and matched grants use explicit scope rather than payload timestamps", async ({ page, request }) => {
    await seedEvents(request, [
      { event_type: "time.observed", source: "e2e", federate: "Alice", time_observation: { kind: "state", session_id: "s1", lookahead: 0, regulating: false, constrained: true, galt: 5 } },
      { event_type: "time.observed", source: "e2e", federate: "Alice", time_observation: { kind: "grant", session_id: "s1", logical_time: 0 } },
      { event_type: "time.observed", source: "e2e", federate: "Alice", time_observation: { kind: "request", session_id: "s1", request_id: "advance-1", logical_time: 10 } },
      { event_type: "object.updated", source: "e2e", federate: "Alice", object_key: "future", logical_time: 999 },
      { event_type: "time.observed", source: "e2e", federate: "Bob", time_observation: { kind: "grant", session_id: "s1", request_id: "advance-1", logical_time: 10 } },
    ]);
    await page.goto("/#time");
    const state = page.getByRole("article", { name: "Reported time state" });
    const ledger = page.getByRole("article", { name: "Time request ledger" });
    await expect(state.locator(".context-item").filter({ has: page.getByText("Last granted time", { exact: true }) }).locator("strong")).toHaveText("0");
    await expect(state.locator(".context-item").filter({ hasText: "Lookahead" }).locator("span")).toHaveText("0");
    await expect(state.locator(".context-item").filter({ hasText: "Regulating" }).locator("span")).toHaveText("Disabled");
    await expect(state.locator(".context-item").filter({ hasText: "LBTS" }).locator("span")).toHaveText("Unknown");
    await expect(ledger.locator("tbody tr")).toHaveCount(1);
    await expect(ledger.locator("tbody tr")).toContainText("pending");
    await expect(page.getByRole("heading", { name: "No correlated grant delays" })).toBeVisible();
    await page.getByRole("button", { name: "Pause view" }).click();
    await seedEvents(request, [{ event_type: "time.observed", source: "e2e", federate: "Alice", time_observation: { kind: "grant", session_id: "s1", request_id: "advance-1", logical_time: 8 } }]);
    await expect(ledger.locator("tbody tr")).toContainText("pending");
    await page.getByRole("button", { name: "Resume live view" }).click();
    await expect(ledger.locator("tbody tr")).toContainText("granted");
    await expect(page.getByRole("heading", { name: "Observed request-to-grant delay" })).toBeVisible();
    await expect(state.locator(".context-item").filter({ has: page.getByText("Last granted time", { exact: true }) }).locator("strong")).toHaveText("8");
    await navigate(page, "Federates");
    await page.getByRole("complementary", { name: "Observed federates" }).getByRole("button", { name: /^Bob / }).click();
    await expect(page.getByRole("region", { name: "Persistent time inspection" })).toHaveCount(1);
    await expect(page.getByRole("article", { name: "Reported time state" })).not.toContainText("Alice");
    await expect(page.getByRole("article", { name: "Time request ledger" }).locator("tbody tr")).toHaveCount(0);
  });

  test("request and grant snapshots survive eviction then clear without changing perspective", async ({ page, request }) => {
    await seedEvents(request, [
      { event_type: "time.observed", source: "e2e", federate: "Alice", time_observation: { kind: "grant", session_id: "old", logical_time: 3 } },
      { event_type: "time.observed", source: "e2e", federate: "Alice", time_observation: { kind: "request", session_id: "old", request_id: "pending-after-eviction", logical_time: 6 } },
      ...Array.from({ length: 2001 }, () => ({ event_type: "callback.invoked", source: "e2e" })),
    ]);
    await page.goto("/#time");
    const ledger = page.getByRole("article", { name: "Time request ledger" });
    await expect(ledger).toContainText("pending-after-eviction");
    await expect(ledger.locator("tbody tr")).toContainText("pending");
    const selected = await page.getByLabel("Time perspective").inputValue();
    await request.delete("/api/events");
    await page.getByRole("button", { name: "Sync now" }).click();
    await expect(page.getByRole("heading", { name: "No persistent time observations" })).toBeVisible();
    await expect(page.getByLabel("Time perspective")).toHaveValue(selected);
    await expect(ledger).toHaveCount(0);
  });

  test("duplicate IDs, outcomes and reconnecting sessions do not manufacture grants", async ({ page, request }) => {
    await seedEvents(request, [
      { event_type: "time.observed", source: "e2e", federate: "Alice", time_observation: { kind: "request", session_id: "s1", request_id: "duplicate", logical_time: 1 } },
      { event_type: "time.observed", source: "e2e", federate: "Alice", time_observation: { kind: "request", session_id: "s1", request_id: "duplicate", logical_time: 1 } },
      { event_type: "time.observed", source: "e2e", federate: "Alice", time_observation: { kind: "grant", session_id: "s1", request_id: "duplicate", logical_time: 1 } },
      { event_type: "time.observed", source: "e2e", federate: "Alice", time_observation: { kind: "request", session_id: "s1", request_id: "failed", logical_time: 2 } },
      { event_type: "time.observed", source: "e2e", federate: "Alice", time_observation: { kind: "failed", session_id: "s1", request_id: "failed", reason: "Explicit failure" } },
      { event_type: "time.observed", source: "e2e", federate: "Alice", time_observation: { kind: "request", session_id: "s2", request_id: "duplicate", logical_time: 4 } },
    ]);
    await page.goto("/#time");
    const ledger = page.getByRole("article", { name: "Time request ledger" });
    await expect(ledger).toContainText("ambiguous");
    await expect(ledger).toContainText("Explicit failure");
    const second = await page.getByLabel("Time perspective").locator("option").allTextContents();
    await page.getByLabel("Time perspective").selectOption({ label: second.find((text) => text.includes("s2"))! });
    await expect(ledger.locator("tbody tr")).toHaveCount(1);
    await expect(ledger).toContainText("pending");
    await expect(page.getByRole("heading", { name: "No correlated grant delays" })).toBeVisible();
  });
});

test.describe("no analytics or MOM", () => {
  test.use({ backendVariant: "java-no-analytics" });
  test("grant-only Java inspection remains useful without analytics or request IDs", async ({ page }) => {
    await page.goto("/#time");
    await expect(page.getByRole("article", { name: "Reported time state" })).toContainText("4");
    await expect(page.getByText(/Uncorrelated observations: 0 requests · 1 grants/)).toBeVisible();
    await expect(page.getByRole("heading", { name: "No correlated grant delays" })).toBeVisible();
    await expect(page.getByRole("heading", { name: "Time analytics unavailable" })).toBeVisible();
  });
});

test.describe("independent Java correlated time", () => {
  test.use({ backendVariant: "java-full" });
  test("named models carry pending and matched requests without a Python process", async ({ page }) => {
    await page.goto("/#time");
    const ledger = page.getByRole("article", { name: "Time request ledger" });
    await expect(ledger.locator("tbody tr")).toHaveCount(2);
    await expect(ledger).toContainText("advance-2");
    await expect(ledger).toContainText("pending");
    await expect(ledger).toContainText("granted");
    await expect(page.getByRole("heading", { name: "Observed request-to-grant delay" })).toBeVisible();
    await expect(page.locator(".connection-badge")).toHaveText("polling");
  });
});

test.describe("bounded Java ledger coverage", () => {
  test.use({ backendVariant: "java-retention-loss" });
  test("request eviction coverage is explicit and does not remove the reported grant", async ({ page }) => {
    await page.route("**/api/state?**", async (route) => {
      const response = await route.fetch();
      const state = await response.json() as StateResponse;
      state.inspectors.time!.dropped_requests = 2;
      await route.fulfill({ json: state });
    });
    await page.goto("/#time");
    await expect(page.getByText(/Request history is incomplete: 2 records expired/)).toBeVisible();
    await expect(page.getByRole("article", { name: "Time request ledger" })).toContainText("retained-pending");
  });
});

test.describe("time feature degradation", () => {
  test.use({ backendVariant: "java-partial" });
  for (const mode of ["older", "disabled", "partial"] as const) test(`${mode} time inspection preserves other useful views`, async ({ page }) => {
    await page.route("**/api/state?**", async (route) => {
      const response = await route.fetch();
      const state = await response.json() as StateResponse;
      if (mode === "older") {
        delete state.inspectors.time;
        delete state.capabilities.time_inspection;
      } else state.capabilities.time_inspection = { ...state.capabilities.time_inspection!, state: mode, reason: "Fixture time coverage" };
      await route.fulfill({ json: state });
    });
    await page.goto("/#time");
    if (mode === "partial") await expect(page.getByText("Partial time inspection: Fixture time coverage")).toBeVisible();
    else await expect(page.getByRole("heading", { name: "Persistent time inspection unavailable" })).toBeVisible();
    await expect(page.getByRole("heading", { name: "Reported logical timestamps" })).toBeVisible();
    await navigate(page, "Events");
    await expect(page.getByRole("region", { name: "Event explorer" }).locator("tbody tr")).toHaveCount(5);
  });
});
