import type { APIRequestContext } from "@playwright/test";
import type { StateResponse } from "../src/api/types";
import { test, expect } from "./fixtures";
import { navigate, seedEvents } from "./support";

function actor(namespace = "namespace-a", membership = "join-1", source = "reporter", name = "Twin") {
  return { source, federate: name, federation: "Shared federation", participant_attribution: {
    session_id: "session", identity_namespace: namespace, federation_id: "federation",
    participant: { participant_id: "participant", membership_id: membership, display_name: name, handle: "7" },
  } };
}
function key(namespace = "namespace-a", membership = "join-1", source = "reporter") {
  return JSON.stringify(["explicit-join", source, "session", namespace, "federation", "participant", membership]);
}
async function seed(request: APIRequestContext) {
  const first = actor(), second = actor("namespace-a", "join-2"), other = actor("namespace-b");
  const context = (namespace: string) => ({ identity_namespace: namespace, federation_id: "federation" });
  await seedEvents(request, [
    { ...first, event_type: "object.discovered", object_key: "shared", object_name: "Shared sensor", object_context: context("namespace-a"), attributes: { value: 10, private: "first-join-only" } },
    { ...second, event_type: "object.updated", object_key: "shared", object_context: context("namespace-a"), attributes: { value: 100, private: "second-join-only" } },
    { ...other, event_type: "object.updated", object_key: "shared", object_context: context("namespace-b"), attributes: { value: 200, private: "other-namespace-only" } },
    { ...first, event_type: "time.observed", time_observation: { kind: "request", request_id: "same-id", logical_time: 5 } },
    { ...second, event_type: "time.observed", time_observation: { kind: "grant", request_id: "same-id", logical_time: 999 } },
    { ...other, event_type: "time.observed", time_observation: { kind: "request", request_id: "same-id", logical_time: 6 } },
    { ...other, event_type: "time.observed", time_observation: { kind: "grant", request_id: "same-id", logical_time: 6 } },
    { ...first, event_type: "declaration.observed", declaration: { kind: "object", direction: "publication", status: "declared", class_key: "Class-A", attribute_scope: "selected", attributes: ["value"] } },
    { ...second, event_type: "declaration.observed", declaration: { kind: "object", direction: "publication", status: "declared", class_key: "Class-B", attribute_scope: "selected", attributes: [] } },
    { ...actor("namespace-a", "join-1", "reporter", "Renamed Twin"), event_type: "object.updated", object_key: "shared", object_context: context("namespace-a"), attributes: { value: 11 } },
    { ...actor("namespace-a", "join-1", "reporter", "Renamed Twin"), event_type: "time.observed", time_observation: { kind: "state", logical_time: 25 } },
  ]);
}

test.describe("exact reporting perspectives", () => {
  test.use({ backendVariant: "empty" });

  test("aliases preserve selection while duplicate names, object contexts and reused request IDs remain isolated", async ({ page, request }) => {
    await seed(request);
    await page.goto("/");
    await page.getByRole("button", { name: /^Renamed Twin / }).click();
    const people = page.getByRole("complementary", { name: "Observed federates" });
    await expect(people.locator(".participant-links button")).toHaveCount(3);
    await expect(page.getByRole("heading", { name: "Renamed Twin", exact: true })).toBeVisible();
    await expect(page.locator(".object-cards pre")).toContainText('"value": 11');
    await expect(page.locator(".object-cards pre")).not.toContainText("second-join-only");
    const declarations = page.getByRole("article", { name: "Reported declarations" });
    await expect(declarations).toContainText("Class-A");
    await expect(declarations).not.toContainText("Class-B");
    const ledger = page.getByRole("article", { name: "Time request ledger" });
    await expect(ledger.locator("tbody tr")).toHaveCount(1);
    await expect(ledger).toContainText("pending");
    await expect(ledger).not.toContainText("999");
    await expect(page.getByLabel("Time perspective", { exact: true })).toHaveValue(key());
    await page.getByLabel("Search events", { exact: true }).fill("first-join-only");
    await page.reload();
    await expect(page.getByLabel("Search events", { exact: true })).toHaveValue("first-join-only");
    await seedEvents(request, [{ ...actor("namespace-a", "join-1", "reporter", "Alias again"), event_type: "object.updated", object_key: "shared",
      object_context: { identity_namespace: "namespace-a", federation_id: "federation" }, attributes: { value: 12 } }]);
    await expect(page.getByRole("heading", { name: "Alias again", exact: true })).toBeVisible();
    await expect(people.locator(".participant-links button")).toHaveCount(3);
    await expect(page.getByLabel("Search events", { exact: true })).toHaveValue("first-join-only");
    await people.getByRole("button", { name: /^Twin .*namespace-a.*join-2/ }).click();
    await expect(page.getByLabel("Search events", { exact: true })).toHaveValue("");
    await expect(page.locator(".object-cards pre")).toContainText('"value": 100');
    await expect(declarations).toContainText("Class-B");
    await expect(declarations).not.toContainText("Class-A");
    await expect(ledger.locator("tbody tr")).toHaveCount(0);
    await people.getByRole("button", { name: /^Twin .*namespace-b/ }).click();
    await expect(page.locator(".object-cards pre")).toContainText('"value": 200');
    await expect(ledger).toContainText("granted");
  });

  test("a saved selection stays dormant after clear and rejoin rather than binding to the reused name", async ({ page, request }) => {
    await seed(request);
    await page.goto("/");
    await page.getByRole("button", { name: /^Renamed Twin / }).click();
    await request.delete("/api/events");
    await seedEvents(request, [{ ...actor("namespace-a", "new-join", "reporter", "Renamed Twin"), event_type: "object.updated", object_key: "new-only", attributes: { marker: "must-not-substitute" } }]);
    await page.getByRole("button", { name: "Sync now", exact: true }).click();
    await expect(page.getByText(/No participant with a matching name was substituted/)).toBeVisible();
    await expect(page.locator(".object-cards details")).toHaveCount(0);
    await expect(page.getByRole("region", { name: "Federate event timeline" }).locator("tbody tr")).toHaveCount(0);
    const selected = await page.evaluate(() => JSON.parse(localStorage.getItem("umbra.observer.workspace-library.v1")!).current.selected_federate);
    expect(selected).toBe(key());
    await page.reload();
    await expect(page.getByText(/No participant with a matching name was substituted/)).toBeVisible();
  });

  test("exact alert filters do not count same-name errors from another join", async ({ page, request }) => {
    await seed(request);
    await page.goto("/#alerts");
    await page.getByLabel("Rule name", { exact: true }).fill("First join only");
    await page.getByLabel("Alert threshold", { exact: true }).fill("2");
    await page.getByLabel("Alert participant perspective", { exact: true }).selectOption(key());
    await expect(page.getByLabel("Alert source", { exact: true })).toBeDisabled();
    await page.getByRole("button", { name: "Create alert rule", exact: true }).click();
    const rules = page.getByRole("article", { name: "Current alert rules" });
    await expect(rules).toContainText("First join only");
    await seedEvents(request, [{ ...actor("namespace-a", "join-2"), event_type: "runtime.error" }]);
    await expect(rules).toContainText("0 observations");
    await seedEvents(request, [{ ...actor(), event_type: "runtime.error" }, { ...actor("namespace-a", "join-1", "reporter", "New alias"), event_type: "runtime.error" }]);
    await expect(rules).toContainText("firing");
    await expect(rules).toContainText("2 observations");
    const report = await (await request.get("/api/alerts")).json();
    expect(report.rules[0].scope.perspective.key).toBe(key());
  });

  test("opaque legacy handles cannot collide with scoped object keys or inherit their chart choices", async ({ page, request }) => {
    const scopedKey = JSON.stringify(["federation-object", "namespace-a", "federation", "raw"]);
    await seedEvents(request, [
      { ...actor(), event_type: "object.updated", object_key: "raw", object_name: "Scoped object", object_context: { identity_namespace: "namespace-a", federation_id: "federation" }, attributes: { first: 1, second: 10 } },
      { ...actor(), event_type: "object.updated", object_key: scopedKey, object_name: "Legacy object", attributes: { first: 2, second: 20 } },
    ]);
    await page.goto("/#objects");
    const list = page.getByRole("complementary", { name: "Matching objects" });
    await expect(list.locator(".participant-links button")).toHaveCount(2);
    await list.getByRole("button", { name: /^Scoped object / }).click();
    await page.getByLabel("Numeric attribute", { exact: true }).selectOption("second");
    await list.getByRole("button", { name: /^Legacy object / }).click();
    await expect(page.getByRole("article", { name: "Selected object" })).toContainText("Legacy object");
    await expect(page.getByLabel("Numeric attribute", { exact: true })).toHaveValue("first");
    await list.getByRole("button", { name: /^Scoped object / }).click();
    await expect(page.getByLabel("Numeric attribute", { exact: true })).toHaveValue("second");
    await expect(page.getByRole("article", { name: "Attribute comparisons" }).getByText("+19", { exact: true })).toHaveCount(0);
  });

  test("large same-name rosters are searchable and paged without changing the selected investigation", async ({ page, request }) => {
    await seedEvents(request, Array.from({ length: 201 }, (_, index) => ({ ...actor("namespace-a", `join-${index}`), event_type: "federate.joined" })));
    await page.goto("/#federates");
    const people = page.getByRole("complementary", { name: "Observed federates" });
    await expect(people.locator(".participant-links button")).toHaveCount(100);
    await people.getByRole("button", { name: "Show 100 more perspectives", exact: true }).click();
    await expect(people.locator(".participant-links button")).toHaveCount(200);
    await page.getByLabel("Search federate perspectives", { exact: true }).fill("join-200");
    await expect(people.locator(".participant-links button")).toHaveCount(1);
    await people.getByRole("button", { name: /^Twin / }).click();
    await page.getByLabel("Participant identity evidence", { exact: true }).selectOption("legacy-provenance");
    await expect(people.locator(".participant-links button")).toHaveCount(0);
    await expect(page.getByRole("heading", { name: "Twin", exact: true })).toBeVisible();
    await expect(page.getByText(/selected investigation was not replaced/)).toBeVisible();
  });

  test("replay keeps captured join-scoped values and requests separate from later live aliases", async ({ page, request }) => {
    await seed(request);
    const started = await request.post("/api/recordings/start", { data: { label: "Identity investigation" } });
    expect(started.ok()).toBeTruthy();
    const { recording_id } = await started.json();
    await seedEvents(request, [{ ...actor("namespace-a", "join-1", "reporter", "Recorded alias"), event_type: "object.updated", object_key: "shared",
      object_context: { identity_namespace: "namespace-a", federation_id: "federation" }, attributes: { value: 21 } }]);
    expect((await request.post("/api/recordings/stop", { data: { recording_id } })).ok()).toBeTruthy();
    const exported = await request.get(`/api/recordings/export?recording_id=${recording_id}`);
    expect(exported.ok()).toBeTruthy();
    const document = await exported.json();
    await page.goto("/#recordings");
    await page.getByLabel("Recording file", { exact: true }).setInputFiles({ name: "identity.json", mimeType: "application/json", buffer: Buffer.from(JSON.stringify(document)) });
    await page.getByRole("button", { name: "Open recording", exact: true }).click();
    await expect(page.getByRole("region", { name: "Replay controls" })).toBeVisible();
    await page.getByLabel("Replay frame", { exact: true }).focus(); await page.keyboard.press("End");
    await navigate(page, "Federates");
    const people = page.getByRole("complementary", { name: "Observed federates" });
    await people.getByRole("button", { name: /^Recorded alias / }).click();
    await seedEvents(request, [{ ...actor("namespace-a", "join-1", "reporter", "Live changed"), event_type: "object.updated", object_key: "shared",
      object_context: { identity_namespace: "namespace-a", federation_id: "federation" }, attributes: { value: 900 } }]);
    await expect(page.getByRole("heading", { name: "Recorded alias", exact: true })).toBeVisible();
    await expect(page.locator(".object-cards pre")).toContainText('"value": 21');
    await expect(page.getByRole("article", { name: "Time request ledger" })).toContainText("pending");
    await expect(people.locator(".participant-links button")).toHaveCount(3);
    await page.getByRole("button", { name: "Return to live", exact: true }).click();
    await expect(page.getByRole("heading", { name: "Live changed", exact: true })).toBeVisible();
    await expect(page.locator(".object-cards pre")).toContainText('"value": 900');
  });
});

test.describe("independent Java identity contract", () => {
  test.use({ backendVariant: "java-identity" });
  test("same names and handles stay separate with partial time, no MOM evidence, and HTTP polling", async ({ page }) => {
    await page.goto("/#federates");
    const people = page.getByRole("complementary", { name: "Observed federates" });
    await expect(people.locator(".participant-links button")).toHaveCount(6);
    await page.getByLabel("Participant identity evidence", { exact: true }).selectOption("explicit-join");
    await expect(people.locator(".participant-links button")).toHaveCount(5);
    await page.getByLabel("Search federate perspectives", { exact: true }).fill("namespace-b");
    await expect(people.locator(".participant-links button")).toHaveCount(1);
    await people.getByRole("button", { name: /^Twin / }).click();
    await expect(page.locator(".object-cards pre")).toContainText('"value": 220');
    await expect(page.getByRole("article", { name: "Time request ledger" })).toContainText("granted");
    await expect(page.getByRole("article", { name: "Reported declarations" })).toContainText("Class-2");
    await expect(page.getByText(/Explicit synthetic requests\/grants are reported/)).toBeVisible();
    await expect(page.getByText(/Only selected fixture publications are reported/)).toBeVisible();
    await expect(page.locator(".connection-badge")).toHaveText("polling");
    expect(await page.evaluate(() => document.documentElement.scrollWidth)).toBeLessThanOrEqual(page.viewportSize()!.width + 1);
    await page.screenshot({ path: test.info().outputPath("identity-java.png"), fullPage: true });
  });
});

for (const profile of ["rich", "limited"] as const) test.describe(`${profile} showcase reporting session`, () => {
  test.use({ backendVariant: profile });
  test("a sample federate keeps one reporting view for its objects and time without invented join IDs", async ({ page }) => {
    await page.goto("/#federates");
    const people = page.getByRole("complementary", { name: "Observed federates" });
    const server = people.getByRole("button", { name: /^ShowcaseServer / });
    await expect(server).toHaveCount(1);
    await server.click();
    await expect(page.locator(".object-cards details").first()).toBeVisible();
    await expect(page.getByRole("article", { name: "Reported time state" })).toBeVisible();
    await expect(page.getByText(/legacy metadata; join IDs unreported/).first()).toBeVisible();
    await expect(server).toHaveCount(1);
    await people.getByRole("button", { name: /^ShowcaseObserver / }).click();
    await expect(page.getByRole("article", { name: "Reported declarations" }).locator("tbody tr").first()).toBeVisible();
  });
});

test.describe("older name-only backends", () => {
  test.use({ backendVariant: "java-limited" });
  test("missing attribution codec keeps an explicitly labelled legacy view instead of invented join IDs", async ({ page }) => {
    await page.route("**/api/state?**", async (route) => {
      const response = await route.fetch(), state = await response.json() as StateResponse;
      delete state.capabilities.participant_provenance;
      delete state.capabilities.analytics;
      state.federate_roster.forEach((row) => { delete row.perspective; });
      state.inspectors.object_observations?.snapshots.forEach((row) => { delete row.perspective; });
      state.inspectors.object_observations?.changes.forEach((row) => { delete row.perspective; });
      state.inspectors.interactions.forEach((row) => { delete row.perspective; });
      state.inspectors.time?.snapshots.forEach((row) => { delete row.scope.perspective; });
      state.inspectors.time?.requests.forEach((row) => { delete row.scope.perspective; });
      state.inspectors.declarations?.snapshots.forEach((row) => { delete row.scope.perspective; });
      await route.fulfill({ json: state });
    });
    await page.goto("/#federates");
    await expect(page.getByRole("heading", { name: "FixtureFederate", exact: true })).toBeVisible();
    await expect(page.getByText(/Legacy name-only view/).first()).toBeVisible();
    await expect(page.locator(".object-cards pre")).toContainText('"value": 3');
    await expect(page.getByText(/must not be treated as one verified participant/)).toBeVisible();
  });
});
