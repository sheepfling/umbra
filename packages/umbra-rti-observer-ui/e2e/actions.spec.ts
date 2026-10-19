import type { CommandCatalog } from "../src/api/types";
import { test, expect } from "./fixtures";
import { eventCount } from "./support";

for (const profile of ["rich", "limited"] as const) {
  test.describe(`${profile} controlled session`, () => {
    test.use({ backendVariant: profile });

    test("connect, join, confirmed resign/disconnect and refresh preserve results", async ({ page }) => {
      await page.goto("/#actions");
      const lifecycle = page.getByRole("article", { name: "Session lifecycle actions" });
      const state = page.getByRole("article", { name: "Controlled session state" });
      const history = page.getByRole("article", { name: "Command results" });
      await expect(state.getByText("disconnected", { exact: true })).toBeVisible();
      await expect(page.getByText("Synthetic showcase: these controls affect a separate demo session.")).toBeVisible();

      await lifecycle.getByRole("button", { name: /^Join federation disabled$/ }).click();
      await expect(lifecycle.getByRole("button", { name: "Join federation", exact: true })).toBeDisabled();
      await lifecycle.getByRole("button", { name: /^Connect available$/ }).click();
      await lifecycle.getByRole("button", { name: "Connect", exact: true }).click();
      await expect(state.getByText("connected", { exact: true })).toBeVisible();

      await lifecycle.getByRole("button", { name: /^Join federation available$/ }).click();
      await lifecycle.getByLabel("Federate name", { exact: true }).fill("MiguelObserver");
      await lifecycle.getByRole("button", { name: "Join federation", exact: true }).click();
      await expect(state.getByText("joined", { exact: true })).toBeVisible();
      await expect(state.getByText("MiguelObserver", { exact: true })).toBeVisible();

      await page.reload();
      await expect(state.getByText("joined", { exact: true })).toBeVisible();
      await expect(history.locator("tbody tr")).toHaveCount(2);
      await expect(history.getByText("succeeded", { exact: true })).toHaveCount(2);

      await lifecycle.getByRole("button", { name: /^Disconnect disabled$/ }).click();
      await expect(lifecycle.getByRole("button", { name: "Disconnect", exact: true })).toBeDisabled();
      await lifecycle.getByRole("button", { name: /^Resign available$/ }).click();
      await expect(lifecycle.getByRole("button", { name: "Resign", exact: true })).toBeDisabled();
      await lifecycle.getByRole("checkbox", { name: /I confirm resign/ }).check();
      await lifecycle.getByRole("button", { name: "Resign", exact: true }).click();
      await expect(state.getByText("connected", { exact: true })).toBeVisible();
      await lifecycle.getByRole("button", { name: /^Disconnect available$/ }).click();
      await expect(lifecycle.getByRole("button", { name: "Disconnect", exact: true })).toBeDisabled();
      await lifecycle.getByRole("checkbox", { name: /I confirm disconnect/ }).check();
      await lifecycle.getByRole("button", { name: "Disconnect", exact: true }).click();
      await expect(state.getByText("disconnected", { exact: true })).toBeVisible();
      await expect(history.locator("tbody tr")).toHaveCount(4);
      await expect(history.getByText("succeeded", { exact: true })).toHaveCount(4);
      await history.getByText("Request and result details", { exact: true }).first().click();
      await expect(history.locator("details[open] pre")).toContainText('"request_id"');
      await page.reload();
      await expect(history.getByText("succeeded", { exact: true })).toHaveCount(4);
    });
  });
}

test("pause freezes the view and blocks commands without stopping collection", async ({ page, request }) => {
  await page.goto("/#actions");
  const submit = page.getByRole("button", { name: "Connect", exact: true });
  const metric = page.getByRole("article", { name: "Window events", exact: true }).locator("strong");
  await expect(submit).toBeEnabled();
  await expect(metric).toBeVisible();
  await page.getByRole("button", { name: "Pause view" }).click();
  const frozen = await metric.innerText();
  await expect(submit).toBeDisabled();
  await expect(page.getByLabel("Chart time window")).toBeDisabled();
  await expect(page.getByRole("button", { name: "Sync now" })).toBeDisabled();
  const before = await eventCount(request);
  await expect.poll(() => eventCount(request)).toBeGreaterThan(before);
  await expect(metric).toHaveText(frozen);
  await page.getByRole("button", { name: "Resume live view" }).click();
  await expect(submit).toBeEnabled();
  await expect.poll(async () => Number(await metric.innerText())).toBeGreaterThan(Number(frozen));
});

test("a stale command catalog disables submission while observation stays usable", async ({ page, request }) => {
  let stale = false;
  let posts = 0;
  page.on("request", (request) => { if (request.method() === "POST" && request.url().endsWith("/api/commands")) posts++; });
  await page.route("**/api/commands/catalog", async (route) => {
    if (stale) await route.fulfill({ status: 503, json: { detail: "Test catalog unavailable" } });
    else await route.continue();
  });
  await page.goto("/#actions");
  const submit = page.getByRole("button", { name: "Connect", exact: true });
  await expect(submit).toBeEnabled();
  stale = true;
  await expect(page.getByRole("alert")).toContainText("Session controls could not refresh");
  await expect(submit).toBeDisabled();
  const before = await eventCount(request);
  await expect.poll(() => eventCount(request)).toBeGreaterThan(before);
  expect(posts).toBe(0);
  stale = false;
  await expect(submit).toBeEnabled();
  await expect(page.getByRole("alert")).toHaveCount(0);
});

test("unsupported actions show the provider reason and never submit", async ({ page, request }) => {
  const response = await request.get("/api/commands/catalog");
  const catalog = await response.json() as CommandCatalog;
  const unsupported: CommandCatalog = { ...catalog, targets: catalog.targets.map((target) => ({
    ...target, commands: target.commands.map((command) => command.action === "session.connect"
      ? { ...command, state: "unsupported", reason: "This provider does not expose connection control." }
      : command),
  })) };
  let posts = 0;
  page.on("request", (request) => { if (request.method() === "POST" && request.url().endsWith("/api/commands")) posts++; });
  await page.route("**/api/commands/catalog", (route) => route.fulfill({ json: unsupported }));
  await page.goto("/#actions");
  await expect(page.getByRole("button", { name: /^Connect unsupported$/ })).toBeVisible();
  await expect(page.getByRole("button", { name: "Connect", exact: true })).toBeDisabled();
  await expect(page.getByText("This provider does not expose connection control.")).toBeVisible();
  expect(posts).toBe(0);
});
