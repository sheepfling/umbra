import type { components } from "../src/api/generated";
import type { StateResponse } from "../src/api/types";
import { test, expect } from "./fixtures";
import { navigate, seedEvents } from "./support";

type Observation = components["schemas"]["InteractionFlowObservationInput"];
function report(kind: "send" | "receive", overrides: Partial<Observation> = {}, source = "e2e"): components["schemas"]["EventInput"] {
  return { event_type: kind === "send" ? "interaction.sent" : "interaction.received", source, interaction_flow: {
    kind, order: "receive-order", message_id: "message-1", session_id: "s1", identity_namespace: "provider", federation_id: "fed",
    class_key: "Signal", producer: { participant_id: "a", membership_id: "a1", display_name: "Same" },
    recipient: { participant_id: "b", membership_id: "b1", display_name: "Same" }, ...overrides,
  } };
}

test.describe("explicit interaction evidence", () => {
  test.use({ backendVariant: "empty" });
  test("matching IDs pair independent reporters while missing IDs remain uncorrelated", async ({ page, request }) => {
    await seedEvents(request, [report("send", { order: "timestamped", logical_time: 5 }), report("receive", { order: "timestamped", logical_time: 5, session_id: "receiver-session" }, "second-reporter"), report("receive", { message_id: null }), { event_type: "interaction.received", interaction_class: "Signal" }]);
    await page.goto("/#interactions");
    const table = page.getByRole("article", { name: "Interaction transmission evidence" });
    await expect(table.locator("tbody tr")).toHaveCount(3);
    await expect(table.getByText("paired", { exact: true })).toBeVisible();
    await page.getByLabel("Interaction pairing status").selectOption("paired");
    await expect(table.locator("tbody tr")).toHaveCount(1);
    await expect(table).toContainText("second-reporter");
    await expect(table).toContainText("receiver-session");
    await expect(table).toContainText("a / a1");
    await expect(table).toContainText("b / b1");
    await expect(page.getByRole("application")).toBeVisible();
    await page.getByLabel("Interaction reported order").selectOption("receive-order");
    await expect(table.locator("tbody tr")).toHaveCount(0);
    await page.getByLabel("Interaction reported order").selectOption("all");
    await page.getByLabel("Search interaction evidence").fill("SECOND-REPORTER");
    await expect(table.locator("tbody tr")).toHaveCount(1);
    await page.getByLabel("Interaction pairing status").selectOption("uncorrelated");
    await page.getByLabel("Search interaction evidence").clear();
    await expect(table.locator("tbody tr")).toHaveCount(2);
    await expect(table).not.toContainText("s observer interval");
  });

  test("duplicate identity invalidates pairing and pause preserves original evidence", async ({ page, request }) => {
    await seedEvents(request, [report("send"), report("receive")]);
    await page.goto("/#interactions");
    const table = page.getByRole("article", { name: "Interaction transmission evidence" });
    await expect(table.getByText("paired", { exact: true })).toBeVisible();
    await page.getByRole("button", { name: "Pause view" }).click();
    await seedEvents(request, [report("send")]);
    await expect(table.getByText("paired", { exact: true })).toBeVisible();
    await page.getByRole("button", { name: "Resume live view" }).click();
    await expect(table.getByText("ambiguous", { exact: true })).toBeVisible();
    await expect(table).toContainText("2 observations — original shown");
    await expect(table).toContainText("Arrival interval unknown");
    await expect(table).toContainText("#1");
    await expect(page.getByText("No measured unambiguous send-before-receive intervals.", { exact: false })).toBeVisible();
  });

  test("same names and classes never pair unrelated joins and receiving first has unknown interval", async ({ page, request }) => {
    await seedEvents(request, [report("send"), report("receive", { recipient: { participant_id: "b", membership_id: "rejoined", display_name: "Same" } }), report("receive", { message_id: "out-of-order" }), report("send", { message_id: "out-of-order" })]);
    await page.goto("/#interactions");
    const table = page.getByRole("article", { name: "Interaction transmission evidence" });
    await expect(table.locator("tbody tr")).toHaveCount(3);
    await page.getByLabel("Interaction pairing status").selectOption("paired");
    await expect(table.locator("tbody tr")).toHaveCount(1);
    await expect(table).toContainText("out-of-order");
    await expect(table).toContainText("no usable send-before-receive interval");
    await expect(table).toContainText("Arrival interval unknown");
  });

  test("pair evidence survives event eviction and clear never resurrects it", async ({ page, request }) => {
    await seedEvents(request, [report("send"), report("receive"), ...Array.from({ length: 2001 }, () => ({ event_type: "callback.invoked" }))]);
    await page.goto("/#interactions");
    const table = page.getByRole("article", { name: "Interaction transmission evidence" });
    await expect(table.locator("tbody tr")).toHaveCount(1);
    await expect(table.getByText("paired", { exact: true })).toBeVisible();
    await request.delete("/api/events");
    await page.getByRole("button", { name: "Sync now" }).click();
    await expect(table.locator("tbody tr")).toHaveCount(0);
    await seedEvents(request, [report("receive")]);
    await expect(table.getByText("receive-only", { exact: true })).toBeVisible();
    await expect(table.getByText("paired", { exact: true })).toHaveCount(0);
  });

  test("bounded index loss and rendered row limits stay explicit", async ({ page, request }) => {
    await seedEvents(request, Array.from({ length: 501 }, (_, index) => report("send", { message_id: `message-${index}` })));
    await page.goto("/#interactions");
    await expect(page.getByText(/Interaction evidence is incomplete: 1 records expired/)).toBeVisible();
    const rows = page.getByRole("article", { name: "Interaction transmission evidence" }).locator("tbody tr");
    await expect(rows).toHaveCount(100);
    await page.getByRole("button", { name: "Show 100 more interaction records" }).click();
    await expect(rows).toHaveCount(200);
    await page.getByLabel("Search interaction evidence").fill("message-0");
    await expect(rows).toHaveCount(0);
  });
});

test.describe("Java explicit flow models", () => {
  test.use({ backendVariant: "java-full" });
  test("unchanged UI shows Java transmission evidence and a genuine zero observer interval", async ({ page }, testInfo) => {
    await page.goto("/#interactions");
    const table = page.getByRole("article", { name: "Interaction transmission evidence" });
    await expect(table.locator("tbody tr")).toHaveCount(1);
    await expect(table).toContainText("fixture-message");
    await expect(table).toContainText("0.000 s observer interval");
    await expect(page.getByRole("application")).toBeVisible();
    await page.screenshot({ path: testInfo.outputPath("interaction-flow.png"), fullPage: true });
  });
  for (const mode of ["older", "disabled", "unsupported", "partial"] as const) test(`${mode} flow coverage leaves other views usable`, async ({ page }) => {
    await page.route("**/api/state?**", async (route) => {
      const response = await route.fetch();
      const state = await response.json() as StateResponse;
      if (mode === "older") { delete state.capabilities.interaction_flow; delete state.inspectors.interaction_flow; }
      else state.capabilities.interaction_flow = { ...state.capabilities.interaction_flow!, state: mode, reason: "Fixture flow coverage" };
      await route.fulfill({ json: state });
    });
    await page.goto("/#interactions");
    if (mode === "partial") await expect(page.getByText("Partial interaction coverage: Fixture flow coverage")).toBeVisible();
    else await expect(page.getByRole("heading", { name: "Interaction flow unavailable" })).toBeVisible();
    await navigate(page, "Events");
    await expect(page.getByRole("region", { name: "Event explorer" }).locator("tbody tr").first()).toBeVisible();
  });
});

test.describe("Java without analytics or message identities", () => {
  test.use({ backendVariant: "java-no-analytics" });
  test("receive-only observations remain counted but never become paired deliveries", async ({ page }) => {
    await page.goto("/#interactions");
    const table = page.getByRole("article", { name: "Interaction transmission evidence" });
    await expect(page.getByText(/Partial interaction coverage: Receive observations only/)).toBeVisible();
    await expect(table.locator("tbody tr")).toHaveCount(1);
    await expect(table).toContainText("uncorrelated");
    await expect(table).toContainText("Transmission ID unknown");
    await expect(table).toContainText("Unknown — not attributed");
    await expect(table).toContainText("Arrival interval unknown");
  });
});
