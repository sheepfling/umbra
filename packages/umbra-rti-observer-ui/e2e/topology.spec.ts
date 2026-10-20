import type { components } from "../src/api/generated";
import type { StateResponse } from "../src/api/types";
import { test, expect } from "./fixtures";
import { navigate, seedEvents } from "./support";

type RelationshipInput = components["schemas"]["RelationshipObservationInput"];
function report(overrides: Partial<RelationshipInput> = {}, source = "e2e"): components["schemas"]["EventInput"] {
  return { event_type: "relationship.observed", source, relationship: {
    relationship_id: "route-1", session_id: "s1", identity_namespace: "provider", federation_id: "fed-1",
    kind: "object", evidence: "declared", status: "active", viewpoint: "observer",
    producer: { participant_id: "a", membership_id: "a1", display_name: "Same name", handle: "1" },
    recipient: { participant_id: "b", membership_id: "b1", display_name: "Same name", handle: "1" },
    class_key: "Sensor", ...overrides,
  } };
}

test.describe("explicit topology", () => {
  test.use({ backendVariant: "empty" });

  test("same names and handles keep distinct joins, endpoints and reporter evidence", async ({ page, request }) => {
    await seedEvents(request, [report(), report({ relationship_id: "route-2", producer: { participant_id: "a", membership_id: "a2", display_name: "Same name" }, kind: "interaction", evidence: "observed", status: "unknown" }), report({}, "second-reporter")]);
    await page.goto("/#topology");
    const graph = page.getByRole("article", { name: "Explicit relationship graph" });
    const table = page.getByRole("article", { name: "Reported relationships" });
    await expect(table.locator("tbody tr")).toHaveCount(3);
    await expect(graph.getByTestId("topology-edge")).toHaveCount(3);
    await expect(graph.getByRole("button", { name: /^Focus / })).toHaveCount(3);
    await graph.getByRole("button", { name: "Focus Same name · a / a2", exact: true }).press("Enter");
    await expect(table.locator("tbody tr")).toHaveCount(1);
    await expect(table).toContainText("route-2");
    await page.getByLabel("Topology focus").selectOption("all");
    await page.getByLabel("Relationship source").selectOption("second-reporter");
    await expect(table.locator("tbody tr")).toHaveCount(1);
    await expect(table).toContainText("route-1");
    await page.getByLabel("Relationship source").selectOption("all");
    await page.getByLabel("Relationship evidence").selectOption("observed");
    await expect(table.locator("tbody tr")).toHaveCount(1);
    await expect(table).toContainText("unknown");
    await page.getByLabel("Relationship evidence").selectOption("all");
    await page.getByLabel("Search relationships").fill("SENSOR");
    await expect(table.locator("tbody tr")).toHaveCount(3);
    await page.getByLabel("Relationship kind").selectOption("object");
    await expect(table.locator("tbody tr")).toHaveCount(2);
  });

  test("no relationships inferred from matching declarations or delivery", async ({ page, request }) => {
    await seedEvents(request, [
      { event_type: "declaration.observed", federate: "A", declaration: { kind: "interaction", class_key: "Signal", direction: "publication", status: "declared" } },
      { event_type: "declaration.observed", federate: "B", declaration: { kind: "interaction", class_key: "Signal", direction: "subscription", status: "declared" } },
      { event_type: "interaction.received", federate: "B", interaction_class: "Signal" },
    ]);
    await page.goto("/#topology");
    await expect(page.getByRole("heading", { name: "Federation topology", exact: true })).toBeVisible();
    await expect(page.getByTestId("topology-edge")).toHaveCount(0);
    await expect(page.getByRole("article", { name: "Explicit relationship graph" })).toContainText("never paired into guessed edges");
  });

  test("replacement, lifecycle, pause and clear keep the selected join honest", async ({ page, request }) => {
    await seedEvents(request, [report()]);
    await page.goto("/#topology");
    const table = page.getByRole("article", { name: "Reported relationships" });
    await expect(table.locator("tbody tr")).toHaveCount(1);
    await page.getByLabel("Topology focus").selectOption(JSON.stringify(["provider", "fed-1", "a", "a1"]));
    await page.getByRole("button", { name: "Pause view" }).click();
    await seedEvents(request, [report({ status: "ended", class_key: null })]);
    await expect(table.getByText("active", { exact: true })).toBeVisible();
    await page.getByRole("button", { name: "Resume live view" }).click();
    await expect(table.getByText("ended", { exact: true })).toBeVisible();
    await expect(table).toContainText("Class unreported");
    await expect(page.getByTestId("topology-edge")).toHaveClass(/ended/);
    await page.getByLabel("Relationship status").selectOption("active");
    await expect(table.locator("tbody tr")).toHaveCount(0);
    await request.delete("/api/events");
    await page.getByRole("button", { name: "Sync now" }).click();
    await expect(page.getByLabel("Topology focus").locator("option:checked")).toHaveText("Selected join no longer reported");
    await expect(page.getByTestId("topology-edge")).toHaveCount(0);
  });

  test("graph and table limits disclose omitted edges and index loss", async ({ page, request }) => {
    await seedEvents(request, Array.from({ length: 1001 }, (_, index) => report({ relationship_id: `route-${String(index).padStart(4, "0")}` })));
    await page.goto("/#topology");
    const table = page.getByRole("article", { name: "Reported relationships" });
    await expect(page.getByText(/Relationship coverage is incomplete: 1 records expired/)).toBeVisible();
    await expect(page.getByText(/Drawing limited to the newest 40/)).toBeVisible();
    await expect(page.getByTestId("topology-edge")).toHaveCount(40);
    await expect(table.locator("tbody tr")).toHaveCount(100);
    await page.getByRole("button", { name: "Show 100 more relationships" }).click();
    await expect(table.locator("tbody tr")).toHaveCount(200);
    await page.getByLabel("Search relationships").fill("route-0000");
    await expect(table.locator("tbody tr")).toHaveCount(0);
  });
});

test.describe("independent Java topology contract", () => {
  test.use({ backendVariant: "java-full" });
  test("same UI renders explicit Java endpoints and provenance", async ({ page }, testInfo) => {
    await page.goto("/#topology");
    await expect(page.getByTestId("topology-edge")).toHaveCount(1);
    const table = page.getByRole("article", { name: "Reported relationships" });
    await expect(table).toContainText("FixtureRecipient");
    await expect(table).toContainText("fixture-provider");
    await expect(page.locator(".connection-badge")).toHaveText("polling");
    await page.screenshot({ path: testInfo.outputPath("topology.png"), fullPage: true });
  });
  for (const mode of ["older", "disabled", "unsupported", "partial"] as const) test(`${mode} relationship coverage leaves other tabs working`, async ({ page }) => {
    await page.route("**/api/state?**", async (route) => {
      const response = await route.fetch();
      const state = await response.json() as StateResponse;
      if (mode === "older") { delete state.capabilities.relationships; delete state.inspectors.relationships; }
      else state.capabilities.relationships = { ...state.capabilities.relationships!, state: mode, reason: "Fixture relationship coverage" };
      await route.fulfill({ json: state });
    });
    await page.goto("/#topology");
    if (mode === "partial") {
      await expect(page.getByText("Partial relationship coverage: Fixture relationship coverage")).toBeVisible();
      await expect(page.getByTestId("topology-edge")).toHaveCount(1);
    } else await expect(page.getByRole("heading", { name: "Topology unavailable" })).toBeVisible();
    await navigate(page, "Events");
    await expect(page.getByRole("region", { name: "Event explorer" }).locator("tbody tr").first()).toBeVisible();
  });
});
