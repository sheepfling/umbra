import { readFile } from "node:fs/promises";
import type { WebSocketRoute } from "@playwright/test";
import type { AnalyticsResponse, EventEnvelope } from "../src/api/types";
import { test, expect } from "./fixtures";
import { navigate, seedEvents } from "./support";

test("all main screens render without a page-wide mobile overflow", async ({ page }) => {
  await page.goto("/");
  const views = [
    ["Federation", "Traffic over time"], ["Federates", "Federates"],
    ["Objects", "Object workspace"],
    ["Logical time", "Logical-time observations"], ["Events", "Event explorer"],
    ["Inspectors", "Objects"], ["Actions", "Controlled sessions"],
    ["Diagnostics", "Observation health"],
  ] as const;
  for (const [view, heading] of views) {
    await navigate(page, view);
    await expect(page.getByRole("heading", { name: heading, exact: true })).toBeVisible();
    await expect(page.getByRole("heading", { name: "Federation workspace" })).toBeVisible();
    const overflow = await page.evaluate(() => document.documentElement.scrollWidth - document.documentElement.clientWidth);
    expect(overflow, `${view} must scroll its tables/nav locally, not the whole page`).toBeLessThanOrEqual(1);
  }
});

test("chart windows and distribution controls update live charts", async ({ page }) => {
  await page.goto("/");
  await expect(page.getByRole("heading", { name: "Traffic over time" })).toBeVisible();
  const changed = page.waitForResponse((response) => response.url().includes("/api/analytics?")
    && new URL(response.url()).searchParams.get("window_seconds") === "30" && response.ok());
  await page.getByLabel("Chart time window").selectOption("30");
  const response = await changed;
  expect((await response.json() as AnalyticsResponse).window_seconds).toBe(30);
  await page.getByLabel("Distribution grouping").selectOption("sources");
  await expect(page.getByLabel("Distribution grouping")).toHaveValue("sources");
  const chart = page.locator("article").filter({ has: page.getByRole("heading", { name: "Traffic over time" }) })
    .getByRole("application");
  await expect(chart).toBeVisible();
  const box = await chart.boundingBox();
  expect(box?.width).toBeGreaterThan(100);
  expect(box?.height).toBeGreaterThan(100);
  await navigate(page, "Logical time");
  await page.getByLabel("Time advance grants only").check();
  await expect(page.getByRole("heading", { name: "Time advance grants", exact: true })).toBeVisible();
});

test("keyboard navigation and browser history keep the selected screen clear", async ({ page }) => {
  await page.goto("/#federation");
  const nav = page.getByRole("navigation", { name: "Observer views" });
  await nav.getByRole("button", { name: "Federation", exact: true }).focus();
  await page.keyboard.press("Tab");
  await expect(nav.getByRole("button", { name: "Federates", exact: true })).toBeFocused();
  await page.keyboard.press("Enter");
  await expect(page).toHaveURL(/#federates$/);
  await expect(page.getByRole("complementary", { name: "Observed federates" })).toBeVisible();
  await navigate(page, "Events");
  await page.goBack();
  await expect(nav.getByRole("button", { name: "Federates", exact: true })).toHaveAttribute("aria-current", "page");
  await page.goForward();
  await expect(nav.getByRole("button", { name: "Events", exact: true })).toHaveAttribute("aria-current", "page");
});

test("a dropped stream reconnects and resumes observations without duplicate rows", async ({ page, request }) => {
  let stream: WebSocketRoute | undefined;
  let connections = 0;
  await page.routeWebSocket("**/ws/events", (route) => {
    stream = route;
    connections++;
    route.connectToServer();
  });
  await page.goto("/#events");
  const connection = page.locator(".connection-badge");
  await expect(connection).toHaveText("live");
  const before = connections;
  expect(stream).toBeDefined();
  await stream!.close({ code: 1012, reason: "Synthetic reconnect check" });
  await expect.poll(() => connections).toBeGreaterThan(before);
  await expect(connection).toHaveText("live");
  await seedEvents(request, [{ event_type: "callback.invoked", source: "e2e", federate: "ReconnectMarker",
    details: { note: "reconnected-marker" } }]);
  const explorer = page.getByRole("region", { name: "Event explorer" });
  await page.getByLabel("Search events").fill("reconnected-marker");
  await expect(explorer.locator("tbody tr")).toHaveCount(1);
  await page.getByLabel("Search events").clear();
  const sequences = await explorer.locator("tbody tr td:first-child code").allTextContents();
  expect(sequences.length).toBeGreaterThan(1);
  expect(new Set(sequences).size).toBe(sequences.length);
});

test.describe("deterministic observations", () => {
  test.use({ backendVariant: "empty" });

  test("federate perspectives do not mix another participant's events", async ({ page, request }) => {
    await seedEvents(request, [
      { event_type: "object.updated", source: "e2e", federate: "Alice", object_key: "alice-object", attributes: { note: "alice-only" } },
      { event_type: "object.updated", source: "e2e", federate: "Bob", object_key: "bob-object", attributes: { note: "bob-only" } },
    ]);
    await page.goto("/#federates");
    const roster = page.getByRole("complementary", { name: "Observed federates" });
    const timeline = page.getByRole("region", { name: "Federate event timeline" });
    await roster.getByRole("button", { name: /^Alice / }).click();
    await expect(timeline.locator("tbody tr")).toHaveCount(1);
    await timeline.getByText("View payload", { exact: true }).click();
    await expect(timeline.locator("pre")).toContainText("alice-only");
    await expect(timeline.locator("pre")).not.toContainText("bob-only");
    await roster.getByRole("button", { name: /^Bob / }).click();
    await expect(timeline.locator("tbody tr")).toHaveCount(1);
    await timeline.getByText("View payload", { exact: true }).click();
    await expect(timeline.locator("pre")).toContainText("bob-only");
    await expect(timeline.locator("pre")).not.toContainText("alice-only");
  });

  test("filters, payload expansion and JSON export agree on the same rows", async ({ page, request }) => {
    await seedEvents(request, [
      { event_type: "object.updated", family: "object", source: "source-A", federate: "Alice", object_key: "one", attributes: { note: "needle-alpha" } },
      { event_type: "object.updated", family: "object", source: "source-B", federate: "Bob", object_key: "two", attributes: { note: "needle-beta" } },
      { event_type: "interaction.received", family: "interaction", source: "source-A", federate: "Alice", parameters: { note: "needle-gamma" } },
    ]);
    await page.goto("/#events");
    const explorer = page.getByRole("region", { name: "Event explorer" });
    await expect(explorer.locator("tbody tr")).toHaveCount(3);
    await page.getByLabel("Filter by family").selectOption("object");
    await page.getByLabel("Filter by type").selectOption("object.updated");
    await page.getByLabel("Filter by source").selectOption("source-A");
    await page.getByLabel("Filter by federate").selectOption("Alice");
    await page.getByLabel("Search events").fill("needle-alpha");
    await expect(explorer.locator("tbody tr")).toHaveCount(1);
    await explorer.getByText("View payload", { exact: true }).click();
    await expect(explorer.locator("pre")).toContainText("needle-alpha");
    const downloaded = page.waitForEvent("download");
    await explorer.getByRole("button", { name: "Export JSON" }).click();
    const download = await downloaded;
    const path = await download.path();
    expect(path).not.toBeNull();
    const exported = JSON.parse(await readFile(path!, "utf8")) as { exported_at: string; events: EventEnvelope[] };
    expect(Number.isFinite(Date.parse(exported.exported_at))).toBeTruthy();
    expect(exported.events).toHaveLength(1);
    expect(exported.events[0]).toMatchObject({ event_type: "object.updated", federate: "Alice", source: "source-A", sequence: 1 });
    await page.getByLabel("Search events").fill("no-such-value");
    await expect(explorer.getByText("No events match the current filters.")).toBeVisible();
    await expect(explorer.getByRole("button", { name: "Export JSON" })).toBeDisabled();
  });

  test("large histories render bounded rows and cleared events do not reappear", async ({ page, request }) => {
    await seedEvents(request, Array.from({ length: 125 }, (_, index) => ({
      event_type: "object.updated", family: "object", source: "e2e", federate: "Alice",
      object_key: "sample", attributes: { value: index },
    })));
    await page.goto("/#events");
    const rows = page.getByRole("region", { name: "Event explorer" }).locator("tbody tr");
    await expect(rows).toHaveCount(100);
    await page.getByRole("button", { name: "Show 100 more" }).click();
    await expect(rows).toHaveCount(125);
    await expect(page.getByRole("button", { name: "Show 100 more" })).toHaveCount(0);
    expect((await request.delete("/api/events")).ok()).toBeTruthy();
    await expect(rows).toHaveCount(0);
    await page.getByRole("button", { name: "Sync now" }).click();
    await expect(rows).toHaveCount(0);
    await expect(page.getByRole("button", { name: "Export JSON" })).toBeDisabled();
    await navigate(page, "Federates");
    await expect(page.getByRole("heading", { name: "No federates observed" })).toBeVisible();
  });
});
