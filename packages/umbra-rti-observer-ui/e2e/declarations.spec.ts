import type { StateResponse } from "../src/api/types";
import { test, expect } from "./fixtures";
import { navigate, seedEvents } from "./support";

test.describe("explicit declaration snapshots", () => {
  test.use({ backendVariant: "empty" });

  test("search and filters keep participant/session declarations separate without inferred routes", async ({ page, request }) => {
    await seedEvents(request, [
      { event_type: "declaration.observed", source: "e2e", federate: "Alice", federation: "Demo", declaration: { kind: "object", direction: "publication", status: "declared", class_key: "Sensor", session_id: "s1", attribute_scope: "selected", attributes: ["load", "state"] } },
      { event_type: "declaration.observed", source: "e2e", federate: "Bob", federation: "Demo", declaration: { kind: "object", direction: "subscription", status: "declared", class_key: "Sensor", session_id: "s1", attribute_scope: "unknown" } },
      { event_type: "declaration.observed", source: "e2e", federate: "Alice", federation: "Demo", declaration: { kind: "object", direction: "publication", status: "declared", class_key: "Sensor", session_id: "s2", attribute_scope: "selected", attributes: [] } },
      { event_type: "declaration.observed", source: "e2e", federate: "Alice", federation: "Demo", declaration: { kind: "interaction", direction: "subscription", status: "declared", class_key: "Signal", session_id: "s1" } },
    ]);
    await page.goto("/#declarations");
    const table = page.getByRole("article", { name: "Reported declarations" });
    await expect(table.locator("tbody tr")).toHaveCount(4);
    await expect(page.getByText(/No routing edges are inferred/)).toBeVisible();
    await page.getByLabel("Search declarations").fill("LOAD");
    await expect(table.locator("tbody tr")).toHaveCount(1);
    await expect(table).toContainText("Alice");
    await page.getByLabel("Search declarations").clear();
    await page.getByLabel("Declaration perspective").selectOption({ label: "Alice · e2e · s1 · Demo" });
    await expect(table.locator("tbody tr")).toHaveCount(2);
    await page.getByLabel("Declaration direction").selectOption("publication");
    await expect(table.locator("tbody tr")).toHaveCount(1);
    await table.getByText("Selected attributes (2)").click();
    await expect(table.locator("li")).toHaveText(["load", "state"]);
    await navigate(page, "Federates");
    await page.getByRole("complementary", { name: "Observed federates" }).getByRole("button", { name: /^Bob / }).click();
    await expect(page.getByRole("article", { name: "Reported declarations" }).locator("tbody tr")).toHaveCount(1);
    await expect(page.getByRole("article", { name: "Reported declarations" })).not.toContainText("Alice");
  });

  test("replacement, withdrawal, pause and clear preserve honest attribute scope", async ({ page, request }) => {
    await seedEvents(request, [{ event_type: "declaration.observed", source: "e2e", federate: "Alice", declaration: { kind: "object", direction: "publication", status: "declared", class_key: "Sensor", attribute_scope: "selected", attributes: ["A", "B"] } }]);
    await page.goto("/#declarations");
    const table = page.getByRole("article", { name: "Reported declarations" });
    await expect(table.getByText("Selected attributes (2)")).toBeVisible();
    await page.getByRole("button", { name: "Pause view" }).click();
    await seedEvents(request, [{ event_type: "declaration.observed", source: "e2e", federate: "Alice", declaration: { kind: "object", direction: "publication", status: "declared", class_key: "Sensor", attribute_scope: "selected", attributes: ["B"] } }]);
    await expect(table.getByText("Selected attributes (2)")).toBeVisible();
    await page.getByRole("button", { name: "Resume live view" }).click();
    await table.getByText("Selected attributes (1)").click();
    await expect(table.locator("li")).toHaveText(["B"]);
    await seedEvents(request, [{ event_type: "declaration.observed", source: "e2e", federate: "Alice", declaration: { kind: "object", direction: "publication", status: "withdrawn", class_key: "Sensor", attribute_scope: "unknown" } }]);
    await page.getByLabel("Declaration status").selectOption("declared");
    await expect(table.getByText("No declarations match these filters.")).toBeVisible();
    await page.getByLabel("Declaration status").selectOption("withdrawn");
    await expect(table).toContainText("Unknown — selected set not reported");
    await request.delete("/api/events");
    await page.getByRole("button", { name: "Sync now" }).click();
    await expect(table.locator("tbody tr")).toHaveCount(0);
  });

  test("snapshots outlive event eviction without treating updates as declarations", async ({ page, request }) => {
    await seedEvents(request, [
      { event_type: "declaration.observed", source: "e2e", federate: "Alice", declaration: { kind: "object", direction: "publication", status: "unknown", class_key: "RetainedClass", attribute_scope: "unknown" } },
      ...Array.from({ length: 2001 }, () => ({ event_type: "object.updated", source: "e2e", object_key: "observed-only", class_name: "NotADeclaration" })),
    ]);
    await page.goto("/#declarations");
    const table = page.getByRole("article", { name: "Reported declarations" });
    await expect(table.locator("tbody tr")).toHaveCount(1);
    await expect(table).toContainText("RetainedClass");
    await expect(table).not.toContainText("NotADeclaration");
  });

  test("bounded snapshots report eviction and render explicit batches of rows", async ({ page, request }) => {
    await seedEvents(request, Array.from({ length: 1001 }, (_, index) => ({ event_type: "declaration.observed", source: "e2e", federate: "Alice", declaration: {
      kind: "object" as const, direction: "publication" as const, status: "declared" as const, class_key: `Class-${String(index).padStart(4, "0")}`, attribute_scope: "unknown" as const,
    } })));
    await page.goto("/#declarations");
    await expect(page.getByText(/Declaration coverage is incomplete: 1 snapshots expired/)).toBeVisible();
    const rows = page.getByRole("article", { name: "Reported declarations" }).locator("tbody tr");
    await expect(rows).toHaveCount(100);
    await page.getByRole("button", { name: "Show 100 more declarations" }).click();
    await expect(rows).toHaveCount(200);
    await page.getByLabel("Search declarations").fill("Class-0000");
    await expect(rows).toHaveCount(0);
  });
});

test.describe("independent Java declaration models", () => {
  test.use({ backendVariant: "java-full" });
  test("unknown object sets and interaction scope stay distinct in the shared contract", async ({ page }) => {
    await page.goto("/#declarations");
    const table = page.getByRole("article", { name: "Reported declarations" });
    await expect(table.locator("tbody tr")).toHaveCount(3);
    await expect(table.getByText("Unknown — selected set not reported")).toBeVisible();
    await expect(table.getByText("Not applicable to interactions")).toBeVisible();
    await expect(page.locator(".connection-badge")).toHaveText("polling");
  });
  for (const mode of ["older", "disabled", "unsupported", "partial"] as const) test(`${mode} declaration support leaves independent views usable`, async ({ page }) => {
    await page.route("**/api/state?**", async (route) => {
      const response = await route.fetch();
      const state = await response.json() as StateResponse;
      if (mode === "older") { delete state.capabilities.declarations; delete state.inspectors.declarations; }
      else state.capabilities.declarations = { ...state.capabilities.declarations!, state: mode, reason: "Fixture declaration coverage" };
      await route.fulfill({ json: state });
    });
    await page.goto("/#declarations");
    if (mode === "partial") {
      await expect(page.getByText("Partial declaration coverage: Fixture declaration coverage")).toBeVisible();
      await expect(page.getByRole("article", { name: "Reported declarations" }).locator("tbody tr")).toHaveCount(3);
    } else await expect(page.getByRole("heading", { name: "Declaration inspection unavailable" })).toBeVisible();
    await navigate(page, "Events");
    await expect(page.getByRole("region", { name: "Event explorer" }).locator("tbody tr").first()).toBeVisible();
  });
});
