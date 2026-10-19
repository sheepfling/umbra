import type { components } from "../src/api/generated";
import type { StateResponse } from "../src/api/types";
import { test, expect } from "./fixtures";
import { navigate, seedEvents } from "./support";

const base = { session_id: "s1", identity_namespace: "provider", federation_id: "fed", report_id: "report" };
type Observation = components["schemas"]["EventInput"]["mom_observation"];
function report(mom_observation: Observation, source = "e2e"): components["schemas"]["EventInput"] { return { event_type: "mom.observed", source, mom_observation }; }

test.describe("explicit management reports", () => {
  test.use({ backendVariant: "empty" });
  test("partial, empty and unknown member reports retain provenance without creating ordinary members", async ({ page, request }) => {
    await seedEvents(request, [report({ ...base, kind: "members", coverage: "partial", members: [{ participant: { display_name: "Same", handle: "0x45" }, status: "unknown" }, { participant: { identity: { participant_id: "a", membership_id: "j1", display_name: "Same" } }, status: "joined" }] })]);
    await page.goto("/#mom");
    const table = page.getByRole("article", { name: "Management member evidence" });
    await expect(table.locator("tbody tr")).toHaveCount(2);
    await expect(table).toContainText("0x45");
    await expect(table).toContainText("join identity unresolved");
    await expect(table).toContainText("provider-reported partial coverage");
    await navigate(page, "Federates");
    await expect(page.getByRole("heading", { name: "No federates observed" })).toBeVisible();
    await expect(page.getByRole("complementary", { name: "Observed federates" })).toHaveCount(0);
    await navigate(page, "MOM reports");
    await seedEvents(request, [report({ ...base, kind: "members", coverage: "complete", members: [] })]);
    await expect(table).toContainText("explicit empty member report");
    await seedEvents(request, [report({ ...base, kind: "members", coverage: "unknown" })]);
    await expect(table).toContainText("member set unknown");
  });
  test("numeric zero, opaque text and counter series keep representations and units separate", async ({ page, request }) => {
    await seedEvents(request, [report({ ...base, kind: "counter", counter_key: "Count", semantics: "cumulative", value: 0, unit: "events" }), report({ ...base, kind: "counter", counter_key: "Count", semantics: "cumulative", value: 7, unit: "events" }), report({ ...base, report_id: "opaque", kind: "counter", counter_key: "Opaque", semantics: "unknown", value_text: "18446744073709551615" })]);
    await page.goto("/#mom");
    const table = page.getByRole("article", { name: "Management counter evidence" });
    await expect(table.locator("tbody tr")).toHaveCount(2);
    await expect(page.getByRole("application")).toBeVisible();
    await expect(table).toContainText("18446744073709551615");
    await table.locator("tbody tr").filter({ hasText: "Opaque" }).getByRole("button", { name: "Chart this counter series" }).click();
    await expect(page.getByText(/No retained numeric samples in this exact counter series/)).toBeVisible();
    await seedEvents(request, [report({ ...base, kind: "counter", counter_key: "Count", semantics: "gauge", value: 1, unit: "bytes" })]);
    await expect(page.getByLabel("Management counter series").locator("option")).toHaveCount(4);
  });
  test("service failure evidence, pause and clear do not invent command results or retain stale errors", async ({ page, request }) => {
    await seedEvents(request, [report({ ...base, kind: "service", service_name: "ExampleService", status: "failed", error_name: "ReportedError" })]);
    await page.goto("/#mom");
    const table = page.getByRole("article", { name: "Management service evidence" });
    await expect(table).toContainText("ReportedError");
    await page.getByRole("button", { name: "Pause view" }).click();
    await seedEvents(request, [report({ ...base, kind: "service", service_name: "ExampleService", status: "completed" })]);
    await expect(table).toContainText("ReportedError");
    await page.getByRole("button", { name: "Resume live view" }).click();
    await expect(table).not.toContainText("ReportedError");
    await expect(table.getByText("completed", { exact: true })).toBeVisible();
    await page.getByLabel("Management report scope").selectOption(JSON.stringify(["e2e", "s1", "provider", "fed"]));
    await request.delete("/api/events");
    await page.getByRole("button", { name: "Sync now" }).click();
    await expect(page.getByLabel("Management report scope").locator("option:checked")).toHaveText("Selected scope no longer reported");
  });
  test("bounded latest/history evidence and row controls stay explicit", async ({ page, request }) => {
    await seedEvents(request, Array.from({ length: 2001 }, (_, index) => report({ ...base, report_id: `report-${index}`, kind: "counter", counter_key: "Count", semantics: "unknown", value: index })));
    await page.goto("/#mom");
    await expect(page.getByText(/Management evidence is incomplete: 1501 latest reports and 1 arrivals expired/)).toBeVisible();
    const rows = page.getByRole("article", { name: "Management counter evidence" }).locator("tbody tr");
    await expect(rows).toHaveCount(100);
    await page.getByRole("button", { name: "Show 100 more management counters" }).click();
    await expect(rows).toHaveCount(200);
  });
});

test.describe("independent Java MOM models", () => {
  test.use({ backendVariant: "java-full" });
  test("the unchanged UI uses typed Java members, service errors and counter history", async ({ page }, testInfo) => {
    await page.goto("/#mom");
    await expect(page.getByRole("article", { name: "Management member evidence" })).toContainText("Unresolved member");
    await expect(page.getByRole("article", { name: "Management service evidence" })).toContainText("FixtureError");
    await expect(page.getByRole("article", { name: "Management counter evidence" })).toContainText("18446744073709551615");
    await expect(page.getByRole("application")).toBeVisible();
    await page.screenshot({ path: testInfo.outputPath("mom.png"), fullPage: true });
  });
  for (const mode of ["older", "disabled", "unsupported", "partial"] as const) test(`${mode} management inspection preserves independent views`, async ({ page }) => {
    await page.route("**/api/state?**", async (route) => {
      const response = await route.fetch(); const state = await response.json() as StateResponse;
      if (mode === "older") { delete state.capabilities.mom_inspection; delete state.inspectors.mom; }
      else state.capabilities.mom_inspection = { ...state.capabilities.mom_inspection!, state: mode, reason: "Fixture management coverage" };
      await route.fulfill({ json: state });
    });
    await page.goto("/#mom");
    if (mode === "partial") await expect(page.getByText("Partial management coverage: Fixture management coverage")).toBeVisible();
    else await expect(page.getByRole("heading", { name: "Management inspection unavailable" })).toBeVisible();
    await navigate(page, "Events");
    await expect(page.getByRole("region", { name: "Event explorer" }).locator("tbody tr").first()).toBeVisible();
  });
});
