import type { ContractResponse } from "../src/api/types";
import { test, expect } from "./fixtures";
import { navigate } from "./support";

test.describe("independent Java HTTP-only backend", () => {
  test.use({ backendVariant: "java-limited" });
  test("the unchanged dashboard works without MOM, commands, or WebSockets", async ({ page }) => {
    const sockets: string[] = [];
    page.on("websocket", (socket) => sockets.push(socket.url()));
    await page.goto("/");
    await expect(page.locator(".connection-badge")).toHaveText("polling");
    const views = [["Federation", "Traffic over time"], ["Federates", "Federates"],
      ["Objects", "Object workspace"],
      ["Declarations", "Declaration explorer"],
      ["Logical time", "Logical-time observations"], ["Events", "Event explorer"],
      ["Inspectors", "Objects"], ["Actions", "Session controls are not enabled"],
      ["Diagnostics", "Observation health"]] as const;
    for (const [view, heading] of views) {
      await navigate(page, view);
      await expect(page.getByRole("heading", { name: heading, exact: true })).toBeVisible();
    }
    const mom = page.locator("article.capability").filter({ hasText: "Management object model" });
    await expect(mom.getByText("unsupported", { exact: true })).toBeVisible();
    expect(sockets).toEqual([]);
  });
});

test.describe("Java partial observations", () => {
  test.use({ backendVariant: "java-partial" });
  test("partial time is labelled while observations remain useful", async ({ page }) => {
    await page.goto("/#time");
    await expect(page.getByText("Partial time observations: Only grants are reported.")).toBeVisible();
    await expect(page.getByRole("heading", { name: "Reported logical timestamps" })).toBeVisible();
    await navigate(page, "Inspectors");
    await expect(page.getByRole("article", { name: "Objects", exact: true })).toContainText("FixtureObject");
  });
});

test.describe("Java empty backend", () => {
  test.use({ backendVariant: "java-empty" });
  test("empty observations are not confused with an incompatible backend", async ({ page }) => {
    await page.goto("/#federates");
    await expect(page.getByRole("heading", { name: "No federates observed" })).toBeVisible();
    await navigate(page, "Events");
    await expect(page.getByText("No events match the current filters.")).toBeVisible();
    await expect(page.getByRole("alert")).toHaveCount(0);
  });
});

test.describe("Java without analytics", () => {
  test.use({ backendVariant: "java-no-analytics" });
  test("omitted analytics does not disable event inspection", async ({ page }) => {
    await page.goto("/");
    await expect(page.getByRole("heading", { name: "Windowed analytics unavailable" })).toBeVisible();
    await navigate(page, "Events");
    await expect(page.getByRole("region", { name: "Event explorer" }).locator("tbody tr")).toHaveCount(5);
    await expect(page.getByRole("alert")).toHaveCount(0);
  });
});

test("REST resources are discovered rather than hard-coded to the Python layout", async ({ page }) => {
  await page.route("**/api/contract", async (route) => {
    const response = await route.fetch();
    const contract = await response.json() as ContractResponse;
    await route.fulfill({ json: { ...contract, snapshot: { ...contract.snapshot, path: "/observer/snapshot" }, api_paths: { ...contract.api_paths,
      state: "/observer/snapshot", analytics: "/observer/activity" } } });
  });
  const aliases = new Map([["/observer/snapshot", "/api/state"], ["/observer/activity", "/api/analytics"]]);
  const discovered: string[] = [];
  await page.route("**/observer/**", async (route) => {
    const url = new URL(route.request().url());
    discovered.push(url.pathname);
    const path = aliases.get(url.pathname);
    if (!path) throw new Error("Unexpected discovered path");
    url.pathname = path;
    await route.fulfill({ response: await route.fetch({ url: url.toString() }) });
  });
  await page.goto("/");
  await expect(page.getByRole("heading", { name: "Traffic over time" })).toBeVisible();
  await page.getByRole("button", { name: "Sync now" }).click();
  await expect.poll(() => discovered.includes("/observer/snapshot")).toBeTruthy();
  expect(discovered).toContain("/observer/activity");
});

test("an incompatible contract is rejected clearly before opening a stream", async ({ page }) => {
  const sockets: string[] = [];
  page.on("websocket", (socket) => sockets.push(socket.url()));
  await page.route("**/api/contract", async (route) => {
    const response = await route.fetch();
    const contract = await response.json() as ContractResponse;
    await route.fulfill({ json: { ...contract, contract_version: "incompatible-v2" } });
  });
  await page.goto("/");
  await expect(page.getByRole("alert")).toContainText("Incompatible observer contract");
  expect(sockets).toEqual([]);
});
