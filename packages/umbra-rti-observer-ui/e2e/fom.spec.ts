import type { components } from "../src/api/generated";
import type { StateResponse } from "../src/api/types";
import { test, expect } from "./fixtures";
import { navigate, seedEvents } from "./support";

const base = { session_id: "s1", identity_namespace: "provider", federation_id: "fed", model_id: "model", revision: "1", status: "present" as const };
type Observation = NonNullable<components["schemas"]["EventInput"]["fom_observation"]>;
function metadata(fom_observation: Observation, source = "e2e"): components["schemas"]["EventInput"] { return { event_type: "fom.observed", source, fom_observation }; }
function selectKey(kind: Observation["kind"], id: string, revision = "1", source = "e2e"): string {
  return JSON.stringify([JSON.stringify([source, "s1", "provider", "fed", "model", revision]), kind, id]);
}

test.describe("supplied metadata boundaries", () => {
  test.use({ backendVariant: "empty" });
  test("class hierarchy and datatype links use supplied metadata without decoding runtime fields", async ({ page, request }) => {
    await seedEvents(request, [metadata({ ...base, kind: "object-class", element_id: "root", name: "Base", parent_state: "root", field_scope: "declared", attributes: [] }),
      metadata({ ...base, kind: "object-class", element_id: "child", name: "Child", parent_state: "parent", parent_id: "root", field_scope: "declared", attributes: [{ field_id: "value", name: "Value", datatype_id: "number" }] }),
      metadata({ ...base, kind: "datatype", element_id: "number", name: "Number", encoding: "primitive", representation: "adapter numeric type" })]);
    await page.goto("/#fom");
    const detail = page.getByRole("article", { name: "Selected FOM metadata" });
    await expect(detail.getByRole("button", { name: "Inspect class Base · present", exact: true })).toBeVisible();
    await expect(detail).toContainText("Field scope: declared");
    await detail.getByRole("button", { name: "Inspect datatype Number · present", exact: true }).click();
    await expect(detail).toContainText("adapter numeric type");
    await detail.getByLabel("Selected FOM element").selectOption(selectKey("object-class", "root"));
    await expect(detail).toContainText("Explicit empty field list");
    await detail.getByRole("button", { name: "Inspect child Child · present", exact: true }).click();
    await expect(detail.getByRole("heading", { name: "Child · object-class", exact: true })).toBeVisible();
  });
  test("same IDs across model revisions and reporters never repair missing references", async ({ page, request }) => {
    await seedEvents(request, [metadata({ ...base, kind: "object-class", element_id: "child", parent_state: "parent", parent_id: "root", field_scope: "unknown", attributes: [{ field_id: "value", datatype_id: "number" }] }),
      metadata({ ...base, revision: "2", kind: "object-class", element_id: "root", name: "Wrong revision parent", parent_state: "root", field_scope: "effective", attributes: [] }),
      metadata({ ...base, kind: "datatype", element_id: "number", name: "Wrong reporter type", encoding: "primitive" }, "other")]);
    await page.goto("/#fom");
    const detail = page.getByRole("article", { name: "Selected FOM metadata" });
    await expect(detail).toContainText("Parent root is not reported in this exact model revision");
    await expect(detail).toContainText("number · datatype not reported in this model revision");
    await expect(detail).toContainText("Name unreported");
    await expect(detail.getByRole("button", { name: /^Inspect datatype/ })).toHaveCount(0);
    await page.getByLabel("FOM model revision").selectOption(JSON.stringify(["e2e", "s1", "provider", "fed", "model", "1"]));
    await expect(page.getByRole("article", { name: "Reported FOM elements" }).locator("tbody tr")).toHaveCount(1);
  });
  test("cyclic references are bounded and unknown ancestry is not shown as a root", async ({ page, request }) => {
    await seedEvents(request, [metadata({ ...base, kind: "object-class", element_id: "a", parent_state: "parent", parent_id: "b", field_scope: "unknown" }),
      metadata({ ...base, kind: "object-class", element_id: "b", parent_state: "parent", parent_id: "a", field_scope: "unknown" })]);
    await page.goto("/#fom");
    const detail = page.getByRole("article", { name: "Selected FOM metadata" });
    await expect(detail).toContainText("Cyclic parent references reported");
    await expect(detail.locator(".fom-breadcrumbs li")).toHaveCount(2);
    await seedEvents(request, [metadata({ ...base, kind: "object-class", element_id: "a", parent_state: "unknown", field_scope: "unknown" })]);
    await expect(detail).toContainText("Ancestry unknown — no root is inferred");
    await expect(detail).toContainText("Field list unknown — not supplied");
  });
  test("arrays, enum aliases, record fields and exact dimension bounds remain distinct", async ({ page, request }) => {
    await seedEvents(request, [metadata({ ...base, kind: "datatype", element_id: "array", encoding: "array", cardinality: "0", element_type_id: "missing" }),
      metadata({ ...base, kind: "datatype", element_id: "enum", encoding: "enumeration", enumerators: [{ name: "Off", value: "0" }, { name: "Alias", value: "0" }] }),
      metadata({ ...base, kind: "datatype", element_id: "record", encoding: "record", fields: [{ field_id: "one" }] }),
      metadata({ ...base, kind: "dimension", element_id: "dimension", upper_bound: "18446744073709551615" })]);
    await page.goto("/#fom");
    const detail = page.getByRole("article", { name: "Selected FOM metadata" });
    await expect(detail).toContainText("Fixed cardinality: 0");
    await detail.getByLabel("Selected FOM element").selectOption(selectKey("datatype", "enum"));
    await expect(detail.locator("tbody tr")).toHaveCount(2);
    await expect(detail).toContainText("Alias");
    await detail.getByLabel("Selected FOM element").selectOption(selectKey("datatype", "record"));
    await expect(detail).toContainText("Datatype unreported");
    await detail.getByLabel("Selected FOM element").selectOption(selectKey("dimension", "dimension"));
    await expect(detail).toContainText("Exact upper bound: 18446744073709551615");
  });
  test("pause, replacement and clear preserve the selected revision without stale fields", async ({ page, request }) => {
    await seedEvents(request, [metadata({ ...base, kind: "interaction-class", element_id: "I", parent_state: "root", field_scope: "effective", parameters: [{ field_id: "old", name: "Old parameter" }] })]);
    await page.goto("/#fom");
    const detail = page.getByRole("article", { name: "Selected FOM metadata" });
    await expect(detail).toContainText("Old parameter");
    await page.getByRole("button", { name: "Pause view" }).click();
    await seedEvents(request, [metadata({ ...base, kind: "interaction-class", element_id: "I", status: "removed", parent_state: "unknown", field_scope: "unknown", parameters: [] })]);
    await expect(detail).toContainText("Old parameter");
    await page.getByRole("button", { name: "Resume live view" }).click();
    await expect(detail).not.toContainText("Old parameter");
    await expect(detail).toContainText("Explicit empty field list");
    await request.delete("/api/events");
    await page.getByRole("button", { name: "Sync now" }).click();
    await expect(detail.getByLabel("Selected FOM element").locator("option:checked")).toHaveText("Selected element no longer reported");
    await expect(detail).toContainText("No latest metadata for this selection");
  });
  test("independent retention and paged fields/elements keep rendering bounded", async ({ page, request }) => {
    await seedEvents(request, Array.from({ length: 2001 }, (_, index) => metadata({ ...base, kind: "object-class", element_id: `class-${index}`, parent_state: "root", field_scope: "declared", attributes: index === 2000 ? Array.from({ length: 101 }, (_, field) => ({ field_id: `field-${field}` })) : [] })));
    await page.goto("/#fom");
    await expect(page.getByText(/FOM evidence is incomplete: 1001 metadata elements and 1 arrivals expired/)).toBeVisible();
    const table = page.getByRole("article", { name: "Reported FOM elements" });
    await expect(table.locator("tbody tr")).toHaveCount(100);
    await table.getByRole("button", { name: "Show 100 more FOM elements", exact: true }).click();
    await expect(table.locator("tbody tr")).toHaveCount(200);
    const detail = page.getByRole("article", { name: "Selected FOM metadata" });
    await expect(detail.locator("tbody tr")).toHaveCount(100);
    await detail.getByRole("button", { name: "Show 100 more metadata fields", exact: true }).click();
    await expect(detail.locator("tbody tr")).toHaveCount(101);
  });
});

test.describe("partial FOM without MOM", () => {
  test.use({ backendVariant: "limited" });
  test("limited provider metadata works without management services", async ({ page }, testInfo) => {
    await page.goto("/#fom");
    await expect(page.getByText(/Partial FOM coverage:/)).toBeVisible();
    await expect(page.getByRole("article", { name: "Selected FOM metadata" })).toContainText("datatype not reported in this model revision");
    await page.screenshot({ path: testInfo.outputPath("fom-limited.png"), fullPage: true });
    await navigate(page, "MOM reports");
    await expect(page.getByRole("heading", { name: "Management inspection unavailable" })).toBeVisible();
    await navigate(page, "Logical time");
    await expect(page.getByRole("heading", { name: "Logical-time observations", exact: true })).toBeVisible();
  });
});

test.describe("independent Java metadata models", () => {
  test.use({ backendVariant: "java-full" });
  test("the unchanged UI navigates Java class/type metadata and exact dimensions", async ({ page }, testInfo) => {
    await page.goto("/#fom");
    const detail = page.getByRole("article", { name: "Selected FOM metadata" });
    await expect(detail.getByRole("button", { name: "Inspect class FixtureBase · present", exact: true })).toBeVisible();
    await expect(detail.getByRole("button", { name: "Inspect datatype FixtureNumber · present", exact: true })).toBeVisible();
    await page.screenshot({ path: testInfo.outputPath("fom.png"), fullPage: true });
    await page.getByLabel("FOM element kind").selectOption("dimension");
    const table = page.getByRole("article", { name: "Reported FOM elements" });
    await expect(table.locator("tbody tr")).toHaveCount(1);
    await table.getByRole("button", { name: "Inspect metadata", exact: true }).click();
    await expect(detail).toContainText("Exact upper bound: 18446744073709551615");
  });
  for (const mode of ["older", "disabled", "unsupported", "partial"] as const) test(`${mode} metadata inspection preserves independent views`, async ({ page }) => {
    await page.route("**/api/state?**", async (route) => {
      const response = await route.fetch(); const state = await response.json() as StateResponse;
      if (mode === "older") { delete state.capabilities.fom_inspection; delete state.inspectors.fom; }
      else state.capabilities.fom_inspection = { ...state.capabilities.fom_inspection!, state: mode, reason: "Fixture metadata coverage" };
      await route.fulfill({ json: state });
    });
    await page.goto("/#fom");
    if (mode === "partial") await expect(page.getByText("Partial FOM coverage: Fixture metadata coverage")).toBeVisible();
    else await expect(page.getByRole("heading", { name: "FOM inspection unavailable" })).toBeVisible();
    await navigate(page, "Events");
    await expect(page.getByRole("region", { name: "Event explorer" }).locator("tbody tr").first()).toBeVisible();
  });
});
