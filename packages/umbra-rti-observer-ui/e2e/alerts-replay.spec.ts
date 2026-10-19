import { test, expect } from "./fixtures";
import { navigate } from "./support";

test.use({ backendVariant: "empty" });

test("replay never fetches current alert evidence or offers configuration mutations", async ({ page, request }) => {
  const started = await request.post("/api/recordings/start", { data: { label: "Alert isolation" } });
  expect(started.ok()).toBeTruthy();
  const { recording_id } = await started.json();
  expect((await request.post("/api/recordings/stop", { data: { recording_id } })).ok()).toBeTruthy();
  const exported = await request.get(`/api/recordings/export?recording_id=${recording_id}`);
  expect(exported.ok()).toBeTruthy();
  await page.goto("/#recordings");
  await page.getByLabel("Recording file", { exact: true }).setInputFiles({ name: "capture.json", mimeType: "application/json", buffer: Buffer.from(await exported.text()) });
  await page.getByRole("button", { name: "Open recording", exact: true }).click();
  await expect(page.getByRole("region", { name: "Replay controls" })).toBeVisible();
  const calls: string[] = [];
  await page.route("**/api/alerts**", async (route) => { calls.push(route.request().method()); await route.continue(); });
  await navigate(page, "Alerts");
  await expect(page.getByRole("heading", { name: "Live alerts are not part of this recording", exact: true })).toBeVisible();
  await expect(page.getByRole("button", { name: "Create alert rule", exact: true })).toHaveCount(0);
  expect(calls).toEqual([]);
  await page.getByRole("button", { name: "Return to live", exact: true }).click();
  await expect(page.getByRole("article", { name: "Current alert rules" })).toContainText("No alert rules configured");
  await expect.poll(() => calls.filter((method) => method === "GET").length).toBeGreaterThan(0);
  expect(calls.filter((method) => method !== "GET")).toEqual([]);
  await page.unrouteAll({ behavior: "wait" });
});

test("opening alerts for the first time in a paused view never substitutes a live report", async ({ page }) => {
  await page.goto("/");
  await expect(page.getByRole("button", { name: "Pause view", exact: true })).toBeEnabled();
  await page.getByRole("button", { name: "Pause view", exact: true }).click();
  let reads = 0;
  await page.route("**/api/alerts**", async (route) => { reads += 1; await route.continue(); });
  await navigate(page, "Alerts");
  await expect(page.getByRole("heading", { name: "Resume live to inspect alerts", exact: true })).toBeVisible();
  expect(reads).toBe(0);
  await page.getByRole("button", { name: "Resume live view", exact: true }).click();
  await expect(page.getByRole("article", { name: "Current alert rules" })).toContainText("No alert rules configured");
  await expect.poll(() => reads).toBeGreaterThan(0);
  await page.unrouteAll({ behavior: "wait" });
});
