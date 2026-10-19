import type { components } from "../src/api/generated";
import type { StateResponse } from "../src/api/types";
import { test, expect } from "./fixtures";
import { navigate, seedEvents } from "./support";

const actor = { participant_id: "a", membership_id: "a1", display_name: "Same" };
const base = { session_id: "s1", identity_namespace: "provider", federation_id: "fed", object_id: "instance-1", attribute_key: "value" };
type Observation = components["schemas"]["EventInput"]["ownership"];
function report(ownership: Observation, source = "e2e"): components["schemas"]["EventInput"] {
  return { event_type: "ownership.observed", source, ownership };
}

test.describe("explicit attribute ownership", () => {
  test.use({ backendVariant: "empty" });
  test("owner state does not follow publishers, updates or completed service tickets", async ({ page, request }) => {
    await seedEvents(request, [{ event_type: "object.updated", federate: "Same", object_key: "instance-1", attributes: { value: 1 } }]);
    await page.goto("/#ownership");
    const states = page.getByRole("article", { name: "Latest ownership states" });
    const requests = page.getByRole("article", { name: "Ownership request evidence" });
    await expect(states.locator("tbody tr")).toHaveCount(0);
    await seedEvents(request, [report({ ...base, kind: "state", owner_state: "known", owner: actor, transition: "reported" }), report({ ...base, kind: "request", request_id: "one", action: "divest", requester: actor }), report({ ...base, kind: "outcome", request_id: "one", action: "divest", requester: actor, outcome: "completed" })]);
    await expect(states).toContainText("a / a1");
    await expect(requests.getByText("completed", { exact: true })).toBeVisible();
    await expect(states.getByText("known", { exact: true })).toBeVisible();
    await seedEvents(request, [report({ ...base, kind: "state", owner_state: "unowned", transition: "divested" })]);
    await expect(states.getByText("unowned", { exact: true })).toBeVisible();
    await expect(states).not.toContainText("a / a1");
  });

  test("reporter/session scopes and requester joins do not merge, and duplicate tickets are ambiguous", async ({ page, request }) => {
    await seedEvents(request, [report({ ...base, kind: "state", owner_state: "known", owner: actor, transition: "reported" }), report({ ...base, session_id: "s2", kind: "state", owner_state: "unknown", transition: "reported" }), report({ ...base, kind: "request", request_id: "one", action: "acquire", requester: actor }), report({ ...base, kind: "outcome", request_id: "one", action: "acquire", requester: { ...actor, membership_id: "rejoined" }, outcome: "completed" })]);
    await page.goto("/#ownership");
    const states = page.getByRole("article", { name: "Latest ownership states" });
    const requests = page.getByRole("article", { name: "Ownership request evidence" });
    await expect(states.locator("tbody tr")).toHaveCount(2);
    await expect(requests.getByText("pending", { exact: true })).toBeVisible();
    await expect(page.getByText(/1 outcome reports had no retained pending ticket/)).toBeVisible();
    await seedEvents(request, [report({ ...base, kind: "request", request_id: "one", action: "acquire", requester: actor })]);
    await expect(requests.getByText("ambiguous", { exact: true })).toBeVisible();
    await page.getByLabel("Ownership state filter").selectOption("unknown");
    await expect(states.locator("tbody tr")).toHaveCount(1);
    await expect(states).toContainText("s2");
    await page.getByLabel("Search ownership evidence").fill("s1");
    await expect(states.locator("tbody tr")).toHaveCount(0);
  });

  test("pause and clear preserve explicit unknown state and missing scope selection", async ({ page, request }) => {
    await seedEvents(request, [report({ ...base, kind: "state", owner_state: "rti", transition: "reported" })]);
    await page.goto("/#ownership");
    const states = page.getByRole("article", { name: "Latest ownership states" });
    await states.getByRole("button", { name: "Follow attribute scope" }).click();
    await page.getByRole("button", { name: "Pause view" }).click();
    await seedEvents(request, [report({ ...base, kind: "state", owner_state: "unknown", transition: "reported" })]);
    await expect(states.getByText("rti", { exact: true })).toBeVisible();
    await page.getByRole("button", { name: "Resume live view" }).click();
    await expect(states.getByText("unknown", { exact: true })).toBeVisible();
    await request.delete("/api/events");
    await page.getByRole("button", { name: "Sync now" }).click();
    await expect(states.locator("tbody tr")).toHaveCount(0);
    await expect(page.getByLabel("Ownership scope").locator("option:checked")).toHaveText("Selected scope no longer reported");
  });

  test("separate bounded state/request/history indexes disclose loss and row limits", async ({ page, request }) => {
    await seedEvents(request, [...Array.from({ length: 501 }, (_, index) => report({ ...base, attribute_key: `attribute-${index}`, kind: "state", owner_state: "unknown", transition: "reported" })), ...Array.from({ length: 501 }, (_, index) => report({ ...base, kind: "request", request_id: `request-${index}`, action: "acquire", requester: actor }))]);
    await page.goto("/#ownership");
    await expect(page.getByText(/Ownership evidence is incomplete: 1 states, 1 requests and 2 transitions expired/)).toBeVisible();
    const states = page.getByRole("article", { name: "Latest ownership states" }).locator("tbody tr");
    await expect(states).toHaveCount(100);
    await page.getByRole("button", { name: "Show 100 more ownership states" }).click();
    await expect(states).toHaveCount(200);
    await expect(page.getByRole("article", { name: "Ownership request evidence" }).locator("tbody tr")).toHaveCount(100);
    await page.getByRole("button", { name: "Show 100 more ownership requests" }).click();
    await expect(page.getByRole("article", { name: "Ownership request evidence" }).locator("tbody tr")).toHaveCount(200);
  });

  test("explicit owner and pending tickets outlive event history expiry", async ({ page, request }) => {
    await seedEvents(request, [report({ ...base, kind: "state", owner_state: "known", owner: actor, transition: "acquired" }), report({ ...base, kind: "request", request_id: "one", action: "divest", requester: actor }), ...Array.from({ length: 2001 }, () => ({ event_type: "callback.invoked" }))]);
    await page.goto("/#ownership");
    await expect(page.getByRole("article", { name: "Latest ownership states" })).toContainText("a / a1");
    await expect(page.getByRole("article", { name: "Ownership request evidence" }).getByText("pending", { exact: true })).toBeVisible();
  });

  test("handle-only RTIs retain unresolved metadata without an invented participant identity", async ({ page, request }) => {
    await seedEvents(request, [report({ ...base, kind: "state", owner_state: "unknown", transition: "reported", reported_owner_name: "Unresolved participant", reported_owner_handle: "0x45" })]);
    await page.goto("/#ownership");
    const states = page.getByRole("article", { name: "Latest ownership states" });
    await expect(states.getByText("unknown", { exact: true })).toBeVisible();
    await expect(states).toContainText("Unresolved participant");
    await expect(states).toContainText("0x45");
    await page.getByLabel("Search ownership evidence").fill("0x45");
    await expect(states.locator("tbody tr")).toHaveCount(1);
    await expect(states).not.toContainText("a / a1");
  });
});

test.describe("independent Java ownership contract", () => {
  test.use({ backendVariant: "java-full" });
  test("unchanged UI renders owner identity, completed and pending tickets from Java", async ({ page }, testInfo) => {
    await page.goto("/#ownership");
    await expect(page.getByRole("article", { name: "Latest ownership states" })).toContainText("recipient / join-2");
    await expect(page.getByRole("article", { name: "Latest ownership states" })).toContainText("Unresolved owner / 0x45");
    const requests = page.getByRole("article", { name: "Ownership request evidence" });
    await expect(requests.getByText("pending", { exact: true })).toBeVisible();
    await expect(requests.getByText("completed", { exact: true })).toBeVisible();
    await page.screenshot({ path: testInfo.outputPath("ownership.png"), fullPage: true });
  });
  for (const mode of ["older", "disabled", "unsupported", "partial"] as const) test(`${mode} ownership inspection leaves attributes and events usable`, async ({ page }) => {
    await page.route("**/api/state?**", async (route) => {
      const response = await route.fetch();
      const state = await response.json() as StateResponse;
      if (mode === "older") { delete state.capabilities.ownership_inspection; delete state.inspectors.ownership; }
      else state.capabilities.ownership_inspection = { ...state.capabilities.ownership_inspection!, state: mode, reason: "Fixture ownership coverage" };
      await route.fulfill({ json: state });
    });
    await page.goto("/#ownership");
    if (mode === "partial") await expect(page.getByText("Partial ownership coverage: Fixture ownership coverage")).toBeVisible();
    else await expect(page.getByRole("heading", { name: "Ownership inspection unavailable" })).toBeVisible();
    await navigate(page, "Events");
    await expect(page.getByRole("region", { name: "Event explorer" }).locator("tbody tr").first()).toBeVisible();
  });
});
