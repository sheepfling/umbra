import type { StateResponse } from "../src/api/types";
import { test, expect } from "./fixtures";
import { navigate, seedEvents } from "./support";

test.describe("persistent object perspectives", () => {
  test.use({ backendVariant: "empty" });

  test("one shared object has independent global, Alice and Bob values", async ({ page, request }) => {
    await seedEvents(request, [
      { event_type: "object.discovered", source: "e2e", federate: "Alice", object_key: "shared", object_name: "Shared sensor", class_name: "Sensor", attributes: { reading: 100, private: "alice-only" } },
      { event_type: "object.updated", source: "e2e", federate: "Bob", object_key: "shared", attributes: { reading: 200, private: "bob-only" } },
      { event_type: "object.updated", source: "e2e", federate: "Alice", object_key: "shared", attributes: { reading: 120 } },
      { event_type: "object.updated", source: "e2e", object_key: "shared", attributes: { reading: 900 } },
    ]);
    await page.goto("/#objects");
    const selected = page.getByRole("article", { name: "Selected object" });
    const reading = selected.locator("tbody tr").filter({ has: page.getByText("reading", { exact: true }) });
    await expect(reading.locator("td").nth(1)).toHaveText("900");
    await page.getByLabel("Object perspective").selectOption("f:Alice");
    await expect(reading.locator("td").nth(1)).toHaveText("120");
    await expect(reading.locator("td").nth(3)).toHaveText("100");
    await expect(selected).toContainText("alice-only");
    await expect(selected).not.toContainText("bob-only");
    await expect(page.getByRole("heading", { name: "Numeric attribute observations" })).toBeVisible();
    await page.getByLabel("Object perspective").selectOption("f:Bob");
    await expect(reading.locator("td").nth(1)).toHaveText("200");
    await expect(selected).toContainText("bob-only");
    await expect(selected).not.toContainText("alice-only");
    await navigate(page, "Federates");
    await page.getByRole("complementary", { name: "Observed federates" }).getByRole("button", { name: /^Alice / }).click();
    await page.locator(".object-cards summary").click();
    await expect(page.locator(".object-cards pre")).toContainText('"reading": 120');
    await expect(page.locator(".object-cards pre")).not.toContainText("bob-only");
  });

  test("expired history retains snapshots but never fabricates numeric samples", async ({ page, request }) => {
    await seedEvents(request, [
      { event_type: "object.updated", source: "e2e", federate: "Alice", object_key: "old", attributes: { reading: 7 } },
      ...Array.from({ length: 2001 }, () => ({ event_type: "callback.invoked", source: "e2e" })),
    ]);
    await page.goto("/#objects");
    await page.getByLabel("Object perspective").selectOption("f:Alice");
    await expect(page.getByRole("article", { name: "Selected object" })).toContainText("7");
    await expect(page.getByText(/History is incomplete:/)).toBeVisible();
    await expect(page.getByText("No retained comparison", { exact: true })).toBeVisible();
    await expect(page.getByRole("heading", { name: "No retained numeric samples" })).toBeVisible();
    await expect(page.getByRole("article", { name: "Attribute comparisons" }).locator("tbody tr")).toHaveCount(0);
    await navigate(page, "Federates");
    await page.locator(".object-cards summary").click();
    await expect(page.locator(".object-cards pre")).toContainText('"reading": 7');
    await navigate(page, "Objects");
    await page.getByLabel("Object perspective").selectOption("f:Alice");
    await request.delete("/api/events");
    await page.getByRole("button", { name: "Sync now" }).click();
    await expect(page.getByLabel("Object perspective")).toHaveValue("f:Alice");
    await expect(page.getByRole("heading", { name: "No object selected" })).toBeVisible();
  });

  test("attribute representations, comparisons and numeric choices are honest", async ({ page, request }) => {
    await seedEvents(request, [
      { event_type: "object.updated", source: "e2e", federate: "Alice", object_key: "values", attributes: { reading: 80, flag: true, text: "123", raw: { encoding: "hex", value: "00ff" }, nullable: null } },
      { event_type: "object.updated", source: "e2e", federate: "Alice", object_key: "values", attributes: { reading: 83, nullable: 1 } },
    ]);
    await page.goto("/#objects");
    await page.getByLabel("Object perspective").selectOption("f:Alice");
    await expect(page.getByText("Undecoded bytes · 2 B · hex")).toBeVisible();
    const options = page.getByLabel("Numeric attribute").locator("option");
    await expect(options).toHaveText(["nullable", "reading"]);
    await page.getByLabel("Numeric attribute").selectOption("reading");
    const comparisons = page.getByRole("article", { name: "Attribute comparisons" });
    await expect(comparisons.getByText("+3", { exact: true })).toBeVisible();
    const nullable = page.getByRole("article", { name: "Selected object" }).locator("tbody tr").filter({ has: page.getByText("nullable", { exact: true }) });
    await expect(nullable.locator("td").nth(3)).toHaveText("null");
    await page.getByRole("button", { name: "Pause view" }).click();
    await seedEvents(request, [{ event_type: "object.updated", source: "e2e", federate: "Alice", object_key: "values", attributes: { reading: 99 } }]);
    await expect(page.getByText(/View paused at/)).toBeVisible();
    const reading = page.getByRole("article", { name: "Selected object" }).locator("tbody tr").filter({ has: page.getByText("reading", { exact: true }) });
    await expect(reading.locator("td").nth(1)).toHaveText("83");
    await page.getByRole("button", { name: "Resume live view" }).click();
    await expect(reading.locator("td").nth(1)).toHaveText("99");
  });

  test("object search, lifecycle filters and empty states remain usable", async ({ page, request }) => {
    await page.goto("/#objects");
    await expect(page.getByRole("heading", { name: "No object selected" })).toBeVisible();
    await seedEvents(request, [
      { event_type: "object.discovered", source: "e2e", object_key: "sensor", object_name: "Weather station", class_name: "TemperatureSensor", attributes: { reading: 1 } },
      { event_type: "object.discovered", source: "e2e", object_key: "device", class_name: "Device" },
      { event_type: "object.removed", source: "e2e", object_key: "sensor" },
    ]);
    await page.getByLabel("Search objects").fill("TEMPERATURE");
    await expect(page.getByRole("complementary", { name: "Matching objects" }).getByRole("button")).toHaveCount(1);
    await page.getByLabel("Object status").selectOption("active");
    await expect(page.getByRole("heading", { name: "No object selected" })).toBeVisible();
    await page.getByLabel("Object status").selectOption("removed");
    await expect(page.getByRole("article", { name: "Selected object" })).toContainText("Weather station");
    await request.delete("/api/events");
    await page.getByRole("button", { name: "Sync now" }).click();
    await expect(page.getByRole("heading", { name: "No object selected" })).toBeVisible();
  });
});

test.describe("portable object contract", () => {
  test.use({ backendVariant: "java-limited" });
  test("Java supplies the same object models without MOM or a stream", async ({ page }) => {
    await page.goto("/#objects");
    await page.getByLabel("Object perspective").selectOption("f:FixtureFederate");
    await expect(page.getByRole("article", { name: "Selected object" })).toContainText("FixtureObject");
    await expect(page.getByRole("heading", { name: "Numeric attribute observations" })).toBeVisible();
    await expect(page.locator(".connection-badge")).toHaveText("polling");
  });

  for (const disabled of [false, true]) test(`${disabled ? "disabled" : "older"} object capabilities do not substitute global values`, async ({ page }) => {
    await page.route("**/api/state?**", async (route) => {
      const response = await route.fetch();
      const state = await response.json() as StateResponse;
      if (disabled) state.capabilities.object_observations = { ...state.capabilities.object_observations!, state: "disabled", reason: "Disabled for this observer" };
      else {
        delete state.inspectors.object_observations;
        delete state.capabilities.object_observations;
      }
      await route.fulfill({ json: state });
    });
    await page.goto("/#objects");
    await expect(page.getByRole("heading", { name: "Object perspectives unavailable" })).toBeVisible();
    await navigate(page, "Federates");
    await expect(page.getByText(/Persistent attributed values unavailable/)).toBeVisible();
    await navigate(page, "Inspectors");
    await expect(page.getByRole("article", { name: "Objects", exact: true })).toContainText("FixtureObject");
  });
});
