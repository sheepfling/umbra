import type { CompactRecordingDocument } from "../src/api/types";
import { test, expect } from "./fixtures";
import { navigate, seedEvents } from "./support";

test.describe("lossless compact replay", () => {
  test.use({ backendVariant: "empty" });
  test("v2 import seeks across edits, earlier values and a clear without captured URLs or live substitution", async ({ page, request }, testInfo) => {
    const started = await request.post("/api/recordings/start", { data: { label: "Compact Miguel investigation", storage_format: "compact-v2" } });
    expect(started.ok()).toBeTruthy();
    const { recording_id } = await started.json() as { recording_id: string };
    for (let index = 0; index < 12; index++) await seedEvents(request, [{ event_type: "object.updated", source: "e2e", object_key: "compact-object", federate: "Alice", attributes: { value: index } }]);
    await request.delete("/api/events");
    await seedEvents(request, [{ event_type: "callback.invoked", source: "e2e", details: { note: "compact-after-clear" } }]);
    expect((await request.post("/api/recordings/stop", { data: { recording_id } })).ok()).toBeTruthy();
    const exported = await request.get(`/api/recordings/export?recording_id=${recording_id}&storage_format=compact-v2`);
    expect(exported.ok(), await exported.text()).toBeTruthy();
    const doc = await exported.json() as CompactRecordingDocument;
    expect(doc.frames.length).toBeGreaterThanOrEqual(16);
    expect(doc.edit_count).toBeGreaterThan(0);
    doc.frames.forEach((frame, index) => { frame.offset_seconds = index * 2; });
    doc.duration_seconds = doc.frames.at(-1)!.offset_seconds;
    doc.contract.api_paths.websocket = "/captured-do-not-open";
    let followed = 0;
    page.on("request", (request) => { if (request.url().includes("captured-do-not-open")) followed++; });
    await page.goto("/#recordings");
    await page.getByLabel("Recording file", { exact: true }).setInputFiles({ name: "compact.json", mimeType: "application/json", buffer: Buffer.from(JSON.stringify(doc)) });
    await page.getByRole("button", { name: "Open recording", exact: true }).click();
    await expect(page.getByRole("heading", { name: "Replay: Compact Miguel investigation", exact: true })).toBeVisible();
    await navigate(page, "Objects");
    const slider = page.getByLabel("Replay frame", { exact: true });
    const first = doc.frames.findIndex((frame) => frame.change === "events");
    await slider.focus(); await page.keyboard.press("Home");
    for (let index = 0; index < first; index++) await page.keyboard.press("ArrowRight");
    await expect(page.getByRole("region", { name: "Object workspace" })).toContainText("compact-object");
    await slider.focus(); await page.keyboard.press("End");
    await expect(page.getByRole("heading", { name: "No object selected", exact: true })).toBeVisible();
    await navigate(page, "Events");
    await expect(page.getByRole("region", { name: "Event explorer" }).locator("tbody tr")).toHaveCount(1);
    await slider.focus(); await page.keyboard.press("Home");
    await expect(page.getByRole("region", { name: "Event explorer" }).locator("tbody tr")).toHaveCount(0);
    await expect(page.getByText(/Seeking recorded state/)).toHaveCount(0);
    expect(followed).toBe(0);
    await page.screenshot({ path: testInfo.outputPath("compact-replay.png"), fullPage: true });
  });
  test("older recording capability negotiates the unchanged v1 start/export shape", async ({ page }) => {
    await page.route("**/api/contract", async (route) => {
      const response = await route.fetch(); const body = await response.json();
      delete body.capabilities.recording_compact;
      await route.fulfill({ response, json: body });
    });
    let startShape: Record<string, unknown> | undefined;
    page.on("request", (request) => { if (request.url().endsWith("/api/recordings/start")) startShape = request.postDataJSON() as Record<string, unknown>; });
    await page.goto("/#recordings");
    await page.getByRole("button", { name: "Start recording", exact: true }).click();
    await expect(page.getByRole("button", { name: "Stop recording", exact: true })).toBeEnabled();
    expect(startShape).not.toHaveProperty("storage_format");
    await page.getByRole("button", { name: "Stop recording", exact: true }).click();
    await page.getByRole("button", { name: "Replay retained recording", exact: true }).click();
    await expect(page.getByRole("region", { name: "Replay controls" })).toBeVisible();
    await page.unrouteAll({ behavior: "wait" });
  });
  test("unavailable replay workers fail closed without substituting live data or trapping navigation", async ({ page }) => {
    await page.addInitScript(() => {
      Object.defineProperty(window, "Worker", { value: class { constructor() { throw new Error("Synthetic worker restriction"); } } });
    });
    await page.goto("/#recordings");
    await page.getByRole("button", { name: "Start recording", exact: true }).click();
    await page.getByRole("button", { name: "Stop recording", exact: true }).click();
    await page.getByRole("button", { name: "Replay retained recording", exact: true }).click();
    await expect(page.getByRole("alert")).toContainText("Recording reconstruction failed");
    await expect(page.getByRole("region", { name: "Runtime metrics" })).toHaveCount(0);
    await expect(page.getByRole("button", { name: "Sync now", exact: true })).toBeDisabled();
    await page.getByRole("button", { name: "Return to live", exact: true }).click();
    await expect(page.getByRole("region", { name: "Runtime metrics" })).toBeVisible();
  });
});
