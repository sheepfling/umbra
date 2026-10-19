import type { StateResponse } from "../src/api/types";
import { test, expect } from "./fixtures";
import { navigate, seedEvents } from "./support";

test.describe("bounded object-perspective rendering", () => {
  test.use({ backendVariant: "empty" });
  test("object lists page without losing search or attributed values", async ({ page, request }) => {
    await seedEvents(request, Array.from({ length: 201 }, (_, index) => ({ event_type: "object.updated", source: "e2e", federate: "A", object_key: `object-${index}`, attributes: { value: index } })));
    await page.goto("/#objects");
    const list = page.getByRole("complementary", { name: "Matching objects" });
    await expect(list.locator(".participant-links button")).toHaveCount(100);
    await list.getByRole("button", { name: "Show 100 more objects", exact: true }).click();
    await expect(list.locator(".participant-links button")).toHaveCount(200);
    await page.getByLabel("Search objects", { exact: true }).fill("object-200");
    await expect(list.locator(".participant-links button")).toHaveCount(1);
    await page.getByLabel("Search objects", { exact: true }).clear();
    await expect(list.locator(".participant-links button")).toHaveCount(100);
    await navigate(page, "Federates");
    await expect(page.locator(".object-cards details")).toHaveCount(100);
    await page.getByRole("button", { name: "Show 100 more attributed objects", exact: true }).click();
    await expect(page.locator(".object-cards details")).toHaveCount(200);
  });
  test("many-to-many perspectives disclose eviction rather than implying removal or complete coverage", async ({ page, request }, testInfo) => {
    for (let start = 0; start < 20000; start += 5000) await seedEvents(request, Array.from({ length: 5000 }, (_, offset) => {
      const index = start + offset;
      return { event_type: "object.updated", source: "e2e", federate: `Participant-${(index + Math.floor(index / 200)) % 100}`, object_key: `object-${index % 200}`, attributes: { value: index } };
    }));
    const state = await (await request.get("/api/state?include_events=true")).json() as StateResponse;
    expect(state.inspectors.object_observations?.snapshots).toHaveLength(5000);
    expect(state.inspectors.object_observations?.dropped_snapshots).toBeGreaterThan(0);
    expect(state.normalized_events).toHaveLength(2000);
    await page.goto("/#objects");
    const coverage = page.getByRole("status", { name: "Object snapshot coverage" });
    await expect(coverage).toContainText("Object perspective coverage is incomplete");
    await expect(coverage).toContainText("Expiry is not object removal");
    await expect(page.getByRole("complementary", { name: "Matching objects" }).locator(".participant-links button")).toHaveCount(100);
    await page.screenshot({ path: testInfo.outputPath("object-budget.png"), fullPage: true });
    await navigate(page, "Diagnostics");
    await expect(page.getByRole("status", { name: "Object snapshot coverage" })).toBeVisible();
  });
});

test.describe("older object budget metadata", () => {
  test.use({ backendVariant: "java-full" });
  test("unreported budgets are not fabricated as unlimited or zero-loss", async ({ page }) => {
    await page.route("**/api/state?**", async (route) => {
      const response = await route.fetch(); const state = await response.json() as StateResponse;
      const data = state.inspectors.object_observations!;
      delete data.snapshot_capacity; delete data.dropped_snapshots; delete data.omitted_changes;
      await route.fulfill({ json: state });
    });
    await page.goto("/#objects");
    await expect(page.getByRole("heading", { name: "Object workspace", exact: true })).toBeVisible();
    await expect(page.getByLabel("Object snapshot coverage", { exact: true })).toHaveCount(0);
    await expect(page.getByRole("complementary", { name: "Matching objects" }).locator(".participant-links button")).toHaveCount(1);
  });
});
