import type { ContractResponse, StateResponse } from "../src/api/types";
import { test, expect } from "./fixtures";
import { navigate, seedEvents } from "./support";

test.use({ backendVariant: "empty" });

async function contractMode(page: import("@playwright/test").Page, mode: () => "both" | "sse" | "http") {
  await page.route("**/api/contract", async (route) => {
    const response = await route.fetch();
    const contract = await response.json() as ContractResponse;
    const current = mode();
    await route.fulfill({ json: { ...contract, capabilities: { ...contract.capabilities,
      websocket: { ...contract.capabilities.websocket, state: current === "both" ? "available" : "disabled" },
      sse: { ...contract.capabilities.sse, state: current === "http" ? "disabled" : "available" } } } });
  });
}

test("SSE-only discovery supplies live observations and an observer-lifetime cursor", async ({ page, request }) => {
  await contractMode(page, () => "sse");
  const sources: string[] = [], sockets: string[] = [];
  page.on("request", (request) => { if (request.resourceType() === "eventsource") sources.push(request.url()); });
  page.on("websocket", (socket) => sockets.push(socket.url()));
  await page.goto("/#diagnostics");
  const delivery = page.getByRole("article", { name: "Live transport" });
  await expect(delivery).toContainText("Transport: sse");
  expect(sockets).toEqual([]);
  expect(sources.length).toBe(1);
  const source = new URL(sources[0]!);
  expect(source.searchParams.get("instance_id")).toBeTruthy();
  expect(source.searchParams.get("after")).toBe("0");
  await seedEvents(request, [{ event_type: "callback.invoked", source: "stream-e2e", details: { note: "sse-marker" } }]);
  await navigate(page, "Events");
  await expect(page.getByRole("region", { name: "Event explorer" }).locator("tbody tr")).toHaveCount(1);
  await page.getByLabel("Search events").fill("sse-marker");
  await expect(page.getByRole("region", { name: "Event explorer" }).locator("tbody tr")).toHaveCount(1);
  await request.delete("/api/events");
  await expect(page.getByRole("region", { name: "Event explorer" }).locator("tbody tr")).toHaveCount(0);
});

test("a refused WebSocket falls through to SSE, then recovers an interrupted SSE without duplicate events", async ({ page, request }) => {
  await page.routeWebSocket("**/ws/events**", (route) => void route.close({ code: 1013, reason: "WS blocked for fallback check" }));
  const sources: string[] = [];
  page.on("request", (request) => { if (request.resourceType() === "eventsource") sources.push(request.url()); });
  await page.goto("/#diagnostics");
  await expect(page.getByRole("article", { name: "Live transport" })).toContainText("Transport: sse");
  await seedEvents(request, [{ event_type: "callback.invoked", source: "stream-e2e", operation: "first" }]);
  await navigate(page, "Events");
  await expect(page.getByRole("region", { name: "Event explorer" }).locator("tbody tr")).toHaveCount(1);
  await page.context().setOffline(true);
  await expect(page.locator(".connection-badge")).not.toHaveText("live");
  await page.context().setOffline(false);
  await seedEvents(request, [{ event_type: "callback.invoked", source: "stream-e2e", operation: "second" }]);
  await page.getByRole("button", { name: "Sync now" }).click();
  await expect(page.locator(".connection-badge")).toHaveText("live");
  await expect.poll(() => sources.length).toBeGreaterThan(1);
  const newest = new URL(sources.at(-1)!);
  expect(Number(newest.searchParams.get("after"))).toBeGreaterThanOrEqual(1);
  await expect(page.getByRole("region", { name: "Event explorer" }).locator("tbody tr")).toHaveCount(2);
});

test("refreshed discovery moves between WebSocket, HTTP and a relocated SSE resource without reload", async ({ page, request }) => {
  let mode: "both" | "http" | "sse" = "both";
  const discoveredStates: string[] = [];
  page.on("request", (request) => {
    const path = new URL(request.url()).pathname;
    if (path === "/api/state" || path === "/observer/relocated-state") discoveredStates.push(path);
  });
  await page.route("**/api/contract", async (route) => {
    const response = await route.fetch();
    const contract = await response.json() as ContractResponse;
    await route.fulfill({ json: { ...contract, api_paths: { ...contract.api_paths,
      state: mode === "both" ? "/api/state" : "/observer/relocated-state", event_stream: "/observer/relocated-sse" },
      capabilities: { ...contract.capabilities,
        websocket: { ...contract.capabilities.websocket, state: mode === "both" ? "available" : "disabled" },
        sse: { ...contract.capabilities.sse, state: mode === "http" ? "disabled" : "available" } } } });
  });
  await page.route("**/observer/relocated-state?*", async (route) => {
    const url = new URL(route.request().url()); url.pathname = "/api/state";
    const response = await route.fetch({ url: url.toString() });
    await route.fulfill({ response });
  });
  // Streaming requests cannot be buffered through route.fetch(). Redirect to
  // the actual same-origin stream while preserving its negotiated cursor.
  await page.route("**/observer/relocated-sse**", (route) => {
    const url = new URL(route.request().url()); url.pathname = "/api/events/stream";
    return route.fulfill({ status: 307, headers: { Location: url.toString() } });
  });
  await page.goto("/#diagnostics");
  const delivery = page.getByRole("article", { name: "Live transport" });
  await expect(delivery).toContainText("Transport: websocket");
  discoveredStates.length = 0;
  mode = "http";
  await page.getByRole("button", { name: "Sync now" }).click();
  await expect(delivery).toContainText("Transport: http");
  await expect(page.locator(".connection-badge")).toHaveText("polling");
  expect(discoveredStates).toEqual(["/observer/relocated-state"]);
  await seedEvents(request, [{ event_type: "callback.invoked", source: "stream-e2e" }]);
  mode = "sse";
  await page.getByRole("button", { name: "Sync now" }).click();
  await expect(delivery).toContainText("Transport: sse");
  await navigate(page, "Events");
  await expect(page.getByRole("region", { name: "Event explorer" }).locator("tbody tr")).toHaveCount(1);
});

test("older HTTP-only services omit observer lifetime and resumption without losing useful observations", async ({ page, request }) => {
  await page.route("**/api/contract", async (route) => {
    const response = await route.fetch();
    const contract = await response.json() as ContractResponse;
    const legacyStream: Partial<ContractResponse["stream"]> = { ...contract.stream };
    delete legacyStream.resumption;
    for (const name of ["websocket", "sse"]) contract.capabilities[name] = { ...contract.capabilities[name]!, state: "unsupported" };
    delete contract.api_paths.websocket;
    delete contract.api_paths.event_stream;
    await route.fulfill({ json: { ...contract, stream: legacyStream } });
  });
  await page.route("**/api/state?*", async (route) => {
    const response = await route.fetch();
    const state = await response.json() as StateResponse;
    delete state.observer_lifetime;
    await route.fulfill({ json: state });
  });
  await seedEvents(request, [{ event_type: "callback.invoked", source: "legacy-http" }]);
  await page.goto("/#diagnostics");
  await expect(page.getByRole("article", { name: "Live transport" })).toContainText("Observer lifetime is not reported");
  await expect(page.locator(".connection-badge")).toHaveText("polling");
  await navigate(page, "Events");
  const rows = page.getByRole("region", { name: "Event explorer" }).locator("tbody tr");
  await expect(rows).toHaveCount(1);
  await request.delete("/api/events");
  await page.getByRole("button", { name: "Sync now" }).click();
  await expect(rows).toHaveCount(0);
});

test("a delayed HTTP cut cannot resurrect rows after a newer clear arrives on the stream", async ({ page, request }) => {
  await seedEvents(request, [{ event_type: "callback.invoked", source: "stream-e2e", operation: "must-not-return" }]);
  let delay = false;
  let release: (() => void) | undefined;
  let captured = false;
  await page.route("**/api/state?*", async (route) => {
    const response = await route.fetch();
    const json = await response.json();
    if (delay) {
      delay = false; captured = true;
      await new Promise<void>((resolve) => { release = resolve; });
    }
    await route.fulfill({ json });
  });
  await page.goto("/#events");
  await expect(page.locator(".connection-badge")).toHaveText("live");
  const rows = page.getByRole("region", { name: "Event explorer" }).locator("tbody tr");
  await expect(rows).toHaveCount(1);
  delay = true;
  await page.getByRole("button", { name: "Sync now" }).click();
  await expect.poll(() => captured).toBeTruthy();
  await request.delete("/api/events");
  await expect(rows).toHaveCount(0);
  release?.();
  await expect(rows).toHaveCount(0);
  await page.getByRole("button", { name: "Sync now" }).click();
  await expect(rows).toHaveCount(0);
});

test("a changed observer lifetime replaces reused sequence numbers and detached inspectors", async ({ page, request }) => {
  await contractMode(page, () => "http");
  await seedEvents(request, [{ event_type: "object.updated", source: "stream-e2e", federate: "Old", object_key: "old-object", attributes: { note: "old-instance" } }]);
  let replacement: StateResponse | undefined;
  await page.route("**/api/state?*", async (route) => {
    if (replacement) return route.fulfill({ json: replacement });
    return route.continue();
  });
  await page.goto("/#events");
  await expect(page.getByRole("region", { name: "Event explorer" }).locator("tbody tr")).toHaveCount(1);
  const original = await (await request.get("/api/state?include_events=true")).json() as StateResponse;
  await request.delete("/api/events");
  replacement = await (await request.get("/api/state?include_events=true")).json() as StateResponse;
  replacement.observer_lifetime = { instance_id: "replacement-observer", revision: 0 };
  replacement.retention.next_sequence = 1;
  replacement.retention.cleared_events = 0;
  expect(original.observer_lifetime?.instance_id).not.toBe(replacement.observer_lifetime.instance_id);
  await page.getByRole("button", { name: "Sync now" }).click();
  await expect(page.getByText(/Observer backend changed/)).toBeVisible();
  await expect(page.getByRole("region", { name: "Event explorer" }).locator("tbody tr")).toHaveCount(0);
  await navigate(page, "Inspectors");
  await expect(page.getByText("No objects observed.")).toBeVisible();
});
