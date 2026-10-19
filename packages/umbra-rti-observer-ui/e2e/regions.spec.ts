import type { components } from "../src/api/generated";
import type { StateResponse } from "../src/api/types";
import { test, expect } from "./fixtures";
import { navigate, seedEvents } from "./support";

const range = { dimension_id: "D", lower_bound: "9007199254740993", upper_bound: "9007199254741093", dimension_upper_bound: "18446744073709551615" };
const base = { session_id: "s1", identity_namespace: "provider", federation_id: "fed", evidence: "declared" as const, status: "active" as const };
type Observation = components["schemas"]["EventInput"]["region_observation"];
function report(region_observation: Observation, source = "e2e"): components["schemas"]["EventInput"] {
  return { event_type: "region.observed", source, region_observation };
}
function region(overrides: Partial<components["schemas"]["RegionStateObservation"]> = {}): components["schemas"]["RegionStateObservation"] {
  return { ...base, kind: "region", region_id: "R", extents: [{ extent_id: "E", dimensions: [range] }], ...overrides };
}

test.describe("exact region and DDM evidence", () => {
  test.use({ backendVariant: "empty" });
  test("large bounds remain exact, declared associations do not invent routing, and references resolve only in scope", async ({ page, request }) => {
    await seedEvents(request, [report(region()), report(region({ session_id: "s2", status: "deleted", extents: null })), report({ ...base, kind: "association", association_id: "A", target_kind: "object", target_id: "instance-1", direction: "update", region_ids: ["R", "unreported"], attribute_keys: ["load"] })]);
    await page.goto("/#regions");
    const table = page.getByRole("article", { name: "Reported region evidence" });
    const selected = page.getByRole("article", { name: "Selected region report" });
    await expect(table.locator("tbody tr")).toHaveCount(3);
    await expect(selected.getByText("9007199254740993 → 9007199254741093")).toBeVisible();
    await expect(selected.getByRole("img")).toHaveAttribute("aria-label", /9007199254740993/);
    await page.getByLabel("Region record kind").selectOption("routing");
    await expect(table.locator("tbody tr")).toHaveCount(0);
    await page.getByLabel("Region record kind").selectOption("association");
    await table.getByRole("button", { name: "Inspect report" }).click();
    await expect(selected.getByText("unreported · not reported in this exact scope")).toBeVisible();
    await selected.getByRole("button", { name: "Inspect region R · active", exact: true }).click();
    await expect(selected.getByText("9007199254740993 → 9007199254741093")).toBeVisible();
    await expect(selected.getByText("Extents unknown", { exact: false })).toHaveCount(0);
  });

  test("unknown dimension limits never become zero bounds or inferred plots", async ({ page, request }) => {
    await seedEvents(request, [report(region({ extents: [{ extent_id: "E", dimensions: [{ dimension_id: "Unbounded", lower_bound: "0", upper_bound: "10" }, { dimension_id: "Zero", lower_bound: "0", upper_bound: "0", dimension_upper_bound: "0" }] }] }))]);
    await page.goto("/#regions");
    const selected = page.getByRole("article", { name: "Selected region report" });
    await expect(selected.getByText("0 → 10")).toBeVisible();
    await expect(selected.getByText("Dimension limit unknown or invalid — no proportional plot")).toBeVisible();
    await expect(selected.getByText("Zero dimension limit — no proportional plot")).toBeVisible();
    await expect(selected.getByRole("img")).toHaveCount(0);
  });

  test("pause, full replacement, deletion and clear do not retain stale extents or switch scope", async ({ page, request }) => {
    await seedEvents(request, [report(region())]);
    await page.goto("/#regions");
    const selected = page.getByRole("article", { name: "Selected region report" });
    await expect(selected.getByText("9007199254740993 → 9007199254741093")).toBeVisible();
    await page.getByRole("button", { name: "Pause view" }).click();
    await seedEvents(request, [report(region({ status: "deleted", extents: null }))]);
    await expect(selected.getByRole("img")).toHaveCount(1);
    await page.getByRole("button", { name: "Resume live view" }).click();
    await expect(selected.getByText(/Extents unknown — not reported/)).toBeVisible();
    await expect(selected.getByRole("img")).toHaveCount(0);
    await request.delete("/api/events");
    await page.getByRole("button", { name: "Sync now" }).click();
    await expect(page.getByLabel("Selected DDM report").locator("option:checked")).toHaveText("Selected report no longer available");
  });

  test("routing evidence stays explicit and unknown endpoint identity is not a receive or topology edge", async ({ page, request }) => {
    await seedEvents(request, [report({ ...base, kind: "routing", routing_id: "route", evidence: "observed", region_ids: [], recipient: { participant_id: "b", membership_id: "b1", display_name: "Bob" } })]);
    await page.goto("/#regions");
    const selected = page.getByRole("article", { name: "Selected region report" });
    await expect(selected.getByText("Explicit empty region reference set")).toBeVisible();
    await expect(selected.getByText("Producer: unknown")).toBeVisible();
    await expect(selected.getByText("Recipient: Bob · b / b1")).toBeVisible();
    await page.getByLabel("Region evidence kind").selectOption("declared");
    await expect(page.getByRole("article", { name: "Reported region evidence" }).locator("tbody tr")).toHaveCount(0);
    await navigate(page, "Topology");
    await expect(page.getByTestId("topology-edge")).toHaveCount(0);
  });

  test("bounded indexes and rendering disclose omitted evidence", async ({ page, request }) => {
    await seedEvents(request, Array.from({ length: 1001 }, (_, index) => report(region({ region_id: `R-${index}`, extents: [] }))));
    await page.goto("/#regions");
    await expect(page.getByText(/Region evidence is incomplete: 501 snapshots and 1 transitions expired/)).toBeVisible();
    const rows = page.getByRole("article", { name: "Reported region evidence" }).locator("tbody tr");
    await expect(rows).toHaveCount(100);
    await page.getByRole("button", { name: "Show 100 more region reports" }).click();
    await expect(rows).toHaveCount(200);
    await page.getByLabel("Search region evidence").fill("R-0");
    await expect(rows).toHaveCount(0);
  });

  test("persistent extents survive unrelated event eviction", async ({ page, request }) => {
    await seedEvents(request, [report(region()), ...Array.from({ length: 2001 }, () => ({ event_type: "callback.invoked" }))]);
    await page.goto("/#regions");
    await expect(page.getByRole("article", { name: "Selected region report" }).getByText("9007199254740993 → 9007199254741093")).toBeVisible();
  });

  test("a point at the maximum bound still has a visible in-domain marker", async ({ page, request }) => {
    await seedEvents(request, [report(region({ extents: [{ extent_id: "E", dimensions: [{ dimension_id: "Max", lower_bound: "10", upper_bound: "10", dimension_upper_bound: "10" }] }] }))]);
    await page.goto("/#regions");
    const track = page.getByRole("img", { name: "Max exact range 10 to 10, dimension limit 10" });
    const marker = track.locator("span");
    await expect(marker).toBeVisible();
    const outer = await track.boundingBox(), point = await marker.boundingBox();
    expect(point!.width).toBeGreaterThanOrEqual(1);
    expect(point!.x + point!.width).toBeLessThanOrEqual(outer!.x + outer!.width + 1);
  });
});

test.describe("independent Java DDM contract", () => {
  test.use({ backendVariant: "java-full" });
  test("Java supplies exact ranges, declared associations and observed routing unchanged", async ({ page }, testInfo) => {
    await page.goto("/#regions");
    await expect(page.getByRole("article", { name: "Reported region evidence" }).locator("tbody tr")).toHaveCount(3);
    await expect(page.getByRole("article", { name: "Selected region report" }).getByText("9007199254740993 → 9007199254741093")).toBeVisible();
    await page.screenshot({ path: testInfo.outputPath("regions.png"), fullPage: true });
  });
  for (const mode of ["older", "disabled", "unsupported", "partial"] as const) test(`${mode} region inspection preserves independent views`, async ({ page }) => {
    await page.route("**/api/state?**", async (route) => {
      const response = await route.fetch();
      const state = await response.json() as StateResponse;
      if (mode === "older") { delete state.capabilities.regions_inspection; delete state.inspectors.regions; }
      else state.capabilities.regions_inspection = { ...state.capabilities.regions_inspection!, state: mode, reason: "Fixture region coverage" };
      await route.fulfill({ json: state });
    });
    await page.goto("/#regions");
    if (mode === "partial") await expect(page.getByText("Partial region coverage: Fixture region coverage")).toBeVisible();
    else await expect(page.getByRole("heading", { name: "Region inspection unavailable" })).toBeVisible();
    await navigate(page, "Events");
    await expect(page.getByRole("region", { name: "Event explorer" }).locator("tbody tr").first()).toBeVisible();
  });
});
