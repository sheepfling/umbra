import type { APIRequestContext, Page } from "@playwright/test";
import { readFile } from "node:fs/promises";
import type { ContractResponse, PortableRecording, RecordingDocument, StateResponse } from "../src/api/types";
import { test, expect } from "./fixtures";
import { navigate, seedEvents } from "./support";

async function fixture(request: APIRequestContext): Promise<RecordingDocument> {
  const response = await request.post("/api/recordings/start", { data: { label: "Portable investigation" } });
  expect(response.ok()).toBeTruthy();
  const { recording_id } = await response.json() as { recording_id: string };
  await seedEvents(request, [{ event_type: "object.updated", source: "e2e", federate: "Alice", object_key: "one", attributes: { value: 1 } }]);
  await seedEvents(request, [{ event_type: "object.updated", source: "e2e", federate: "Alice", object_key: "one", attributes: { value: 2 } }]);
  await request.delete("/api/events");
  await seedEvents(request, [{ event_type: "callback.invoked", source: "e2e", federate: "Bob", details: { note: "after-clear-marker" } }]);
  expect((await request.post("/api/recordings/stop", { data: { recording_id } })).ok()).toBeTruthy();
  const exported = await request.get(`/api/recordings/export?recording_id=${recording_id}`);
  expect(exported.ok(), await exported.text()).toBeTruthy();
  const document = await exported.json() as RecordingDocument;
  // Explicit monotonic test offsets remove network timing from playback checks.
  document.frames.forEach((frame, index) => { frame.offset_seconds = index * 2; });
  document.duration_seconds = document.frames.at(-1)!.offset_seconds;
  return document;
}

async function open(page: Page, document: RecordingDocument) {
  await page.getByLabel("Recording file", { exact: true }).setInputFiles({ name: "capture.json", mimeType: "application/json", buffer: Buffer.from(JSON.stringify(document)) });
  await page.getByRole("button", { name: "Open recording", exact: true }).click();
  await expect(page.getByRole("region", { name: "Replay controls" })).toBeVisible();
}

test.describe("portable state recording and replay", () => {
  test.use({ backendVariant: "empty" });
  test("capture survives navigation/refresh, downloads and replays without replacing live collection", async ({ page, request }) => {
    await page.goto("/#recordings");
    await page.getByLabel("Recording label", { exact: true }).fill("Miguel walkthrough");
    await page.getByRole("button", { name: "Start recording", exact: true }).click();
    await expect(page.getByRole("button", { name: "Stop recording", exact: true })).toBeEnabled();
    await seedEvents(request, [{ event_type: "callback.invoked", source: "e2e", details: { note: "captured-marker" } }]);
    await navigate(page, "Events");
    await page.reload();
    await navigate(page, "Recordings");
    await expect(page.getByRole("article", { name: "Backend recording" })).toContainText("Recording");
    await page.getByRole("button", { name: "Stop recording", exact: true }).click();
    const downloaded = page.waitForEvent("download");
    await page.getByRole("button", { name: "Download recording", exact: true }).click();
    const file = await downloaded;
    const document = JSON.parse(await readFile((await file.path())!, "utf8")) as PortableRecording;
    expect(document.recording_version).toBe("umbra-observer-recording-v2");
    expect(document.frames.some((frame) => frame.arrivals.length > 0)).toBeTruthy();
    await page.getByRole("button", { name: "Replay retained recording", exact: true }).click();
    await expect(page.getByRole("heading", { name: "Replay: Miguel walkthrough", exact: true })).toBeVisible();
    await expect(page.getByRole("button", { name: "Sync now", exact: true })).toBeDisabled();
    await expect(page.getByRole("button", { name: "Pause view", exact: true })).toBeDisabled();
    await seedEvents(request, [{ event_type: "callback.invoked", source: "e2e", details: { note: "new-live-marker" } }]);
    await navigate(page, "Events");
    await page.getByLabel("Replay frame", { exact: true }).focus(); await page.keyboard.press("End");
    await page.getByLabel("Search events", { exact: true }).fill("new-live-marker");
    await expect(page.getByRole("region", { name: "Event explorer" }).locator("tbody tr")).toHaveCount(0);
    await page.getByRole("button", { name: "Return to live", exact: true }).click();
    await expect(page.getByRole("region", { name: "Event explorer" }).locator("tbody tr")).toHaveCount(1);
  });
  test("exact seek restores earlier objects and clear boundaries without future analytics or resurrected events", async ({ page, request }) => {
    const document = await fixture(request);
    await page.goto("/#recordings"); await open(page, document);
    await navigate(page, "Objects");
    await expect(page.getByRole("heading", { name: "No object selected", exact: true })).toBeVisible();
    const index = document.frames.findIndex((frame) => frame.change === "events");
    const slider = page.getByLabel("Replay frame", { exact: true });
    await slider.focus(); await page.keyboard.press("Home");
    for (let step = 0; step < index; step++) await page.keyboard.press("ArrowRight");
    await expect(page.getByRole("region", { name: "Object workspace" })).toContainText("one");
    await slider.focus(); await page.keyboard.press("End");
    await expect(page.getByRole("heading", { name: "No object selected", exact: true })).toBeVisible();
    await navigate(page, "Events");
    const explorer = page.getByRole("region", { name: "Event explorer" });
    await expect(explorer.locator("tbody tr")).toHaveCount(1);
    await slider.focus(); await page.keyboard.press("Home");
    await expect(explorer.locator("tbody tr")).toHaveCount(0);
    await expect(page.getByRole("button", { name: "Play replay", exact: true })).toBeVisible();
  });
  test("play, speed, pause and exact stepping use the recorded clock", async ({ page, request }, testInfo) => {
    const document = await fixture(request);
    await page.goto("/#recordings"); await open(page, document);
    await page.getByLabel("Replay speed", { exact: true }).selectOption("8");
    await page.getByRole("button", { name: "Play replay", exact: true }).click();
    await expect(page.getByLabel("Replay frame", { exact: true })).not.toHaveValue("0");
    await page.getByRole("button", { name: "Pause replay", exact: true }).click();
    const before = await page.getByLabel("Replay clock", { exact: true }).inputValue();
    await navigate(page, "Events");
    expect(await page.getByLabel("Replay clock", { exact: true }).inputValue()).toBe(before);
    await page.getByRole("button", { name: "Previous recorded state", exact: true }).click();
    await page.getByRole("button", { name: "Next recorded state", exact: true }).click();
    await page.screenshot({ path: testInfo.outputPath("replay.png"), fullPage: true });
  });
  test("captured control capabilities and URLs never become live command or stream requests", async ({ page, request }) => {
    const document = await fixture(request);
    document.contract.capabilities.commands!.state = "available";
    document.frames.forEach((frame) => { frame.state.capabilities.commands!.state = "available"; });
    document.contract.api_paths.commands = "/captured-command-must-not-run";
    document.contract.api_paths.websocket = "/captured-socket-must-not-open";
    let submitted = 0, followed = 0;
    page.on("request", (request) => { if (request.url().includes("captured-")) followed++; if (request.method() === "POST" && request.url().endsWith("/api/commands")) submitted++; });
    await page.goto("/#recordings"); await open(page, document);
    await navigate(page, "Actions");
    await expect(page.getByText(/Replay is read-only/)).toBeVisible();
    await expect(page.getByRole("button", { name: "Submit command", exact: true })).toHaveCount(0);
    expect(submitted).toBe(0); expect(followed).toBe(0);
  });
  test("invalid files leave live observations intact and report rejection clearly", async ({ page, request }) => {
    const document = await fixture(request);
    const invalid = { ...document, recording_version: "incompatible-v9" };
    await page.goto("/#recordings");
    await page.getByLabel("Recording file", { exact: true }).setInputFiles({ name: "invalid.json", mimeType: "application/json", buffer: Buffer.from(JSON.stringify(invalid)) });
    await page.getByRole("button", { name: "Open recording", exact: true }).click();
    await expect(page.getByRole("alert")).toContainText("Recording rejected by the validated contract");
    await expect(page.getByRole("region", { name: "Replay controls" })).toHaveCount(0);
    await navigate(page, "Events");
    await expect(page.getByRole("region", { name: "Event explorer" }).locator("tbody tr")).toHaveCount(1);
  });
  test("retention limits stop recording, keep the file usable and require explicit replacement", async ({ page, request }) => {
    const response = await request.post("/api/recordings/start", { data: { label: "Capacity example", max_frames: 1 } });
    expect(response.ok()).toBeTruthy();
    await seedEvents(request, [{ event_type: "callback.invoked" }]);
    await page.goto("/#recordings");
    await expect(page.getByRole("article", { name: "Backend recording" })).toContainText("Stopped: frame-limit");
    await expect(page.getByRole("button", { name: "Replay retained recording", exact: true })).toBeEnabled();
    page.once("dialog", (dialog) => dialog.dismiss());
    await page.getByRole("button", { name: "Start new recording", exact: true }).click();
    await expect(page.getByRole("article", { name: "Backend recording" })).toContainText("Stopped: frame-limit");
    await page.getByRole("button", { name: "Replay retained recording", exact: true }).click();
    await expect(page.getByRole("heading", { name: "Replay: Capacity example", exact: true })).toBeVisible();
    await expect(page.getByRole("button", { name: "Play replay", exact: true })).toBeDisabled();
  });
  test("the portable full-model corpus preserves topology, MOM and FOM inspectors without reconstruction", async ({ page, request }) => {
    const document = await fixture(request);
    const root = new URL("../../umbra-rti-observer/conformance/fixtures/full/", import.meta.url);
    const routes = (await readFile(new URL("routes.tsv", root), "utf8")).trim().split(/\r?\n/).map((line) => line.split("\t"));
    const read = async (path: string) => JSON.parse(await readFile(new URL(routes.find((route) => route[0] === path)![2]!, root), "utf8"));
    const state = await read("/api/state?include_events=true") as StateResponse;
    const contract = await read("/api/contract") as ContractResponse;
    contract.capabilities = structuredClone(state.capabilities);
    document.contract = contract; document.label = "Portable full-model corpus";
    document.started_at = 1760000000; document.stopped_at = 1760000002; document.duration_seconds = 2;
    document.frames = [{ index: 0, offset_seconds: 0, captured_at: 1760000000, change: "start", arrivals: [], state },
      { index: 1, offset_seconds: 2, captured_at: 1760000002, change: "stop", arrivals: [], state: structuredClone(state) }];
    document.captured_bytes = Buffer.byteLength(JSON.stringify(document)) + 4096;
    await page.goto("/#recordings"); await open(page, document);
    await navigate(page, "Topology");
    await expect(page.getByRole("heading", { name: "Federation topology", exact: true })).toBeVisible();
    await expect(page.getByRole("group", { name: "Producer to recipient relationships", exact: true })).toBeVisible();
    await navigate(page, "MOM reports");
    await expect(page.getByRole("article", { name: "Management counter evidence" })).toContainText("18446744073709551615");
    await navigate(page, "FOM browser");
    await expect(page.getByRole("article", { name: "Selected FOM metadata" })).toContainText("FixtureClass");
    await page.getByRole("button", { name: "Next recorded state", exact: true }).click();
    await expect(page.getByRole("article", { name: "Selected FOM metadata" })).toContainText("FixtureClass");
  });
  test("unknown capture status cannot start or replace a recording and recovers explicitly", async ({ page }) => {
    let recovered = false;
    await page.route("**/api/recordings", async (route) => {
      if (!recovered) await route.fulfill({ status: 503, json: { detail: "Synthetic status outage" } });
      else await route.continue();
    });
    await page.goto("/#recordings");
    await expect(page.getByRole("article", { name: "Backend recording" })).toContainText("Recording status is unknown");
    await expect(page.getByRole("button", { name: "Start recording", exact: true })).toBeDisabled();
    recovered = true;
    await expect(page.getByRole("button", { name: "Start recording", exact: true })).toBeEnabled();
    await expect(page.getByRole("alert")).toHaveCount(0);
  });
  for (const mode of ["projected", "legacy"] as const) test(`${mode} recorded analytics uses the advertised request shape`, async ({ page, request }) => {
    const document = await fixture(request);
    if (mode === "legacy") await page.route("**/api/contract", async (route) => {
      const response = await route.fetch(); const contract = await response.json() as ContractResponse;
      delete contract.capabilities.recording_analytics_window;
      await route.fulfill({ json: contract });
    });
    let shape: "projected" | "legacy" | undefined;
    await page.route("**/api/recordings/analyze", async (route) => {
      const body = route.request().postDataJSON() as { window?: unknown; frame?: unknown };
      shape = "window" in body ? "projected" : "legacy";
      expect(JSON.stringify(body).includes('"inspectors"')).toBe(mode === "legacy");
      await route.continue();
    });
    await page.goto("/#recordings"); await open(page, document);
    await expect.poll(() => shape).toBe(mode);
    await expect(page.getByRole("article", { name: "Window events", exact: true }).locator("strong")).toHaveText("0");
  });
});

test.describe("recordings without analytics", () => {
  test.use({ backendVariant: "no-analytics" });
  test("a recorded model remains inspectable without windowed analytics", async ({ page, request }) => {
    const document = await fixture(request);
    await page.goto("/#recordings"); await open(page, document);
    await navigate(page, "Federation");
    await expect(page.getByRole("heading", { name: "Windowed analytics unavailable", exact: true })).toBeVisible();
    await navigate(page, "Objects");
    await expect(page.getByRole("heading", { name: "Object workspace", exact: true })).toBeVisible();
    await navigate(page, "Logical time");
    await expect(page.getByRole("heading", { name: "Time analytics unavailable", exact: true })).toBeVisible();
    await expect(page.getByRole("region", { name: "Persistent time inspection" })).toBeVisible();
  });
});

test.describe("limited-provider captures", () => {
  test.use({ backendVariant: "limited" });
  test("partial schema and grant-only time remain useful without MOM", async ({ page, request }) => {
    const document = await fixture(request);
    await page.goto("/#recordings"); await open(page, document);
    await navigate(page, "MOM reports");
    await expect(page.getByRole("heading", { name: "Management inspection unavailable", exact: true })).toBeVisible();
    await navigate(page, "FOM browser");
    await expect(page.getByText(/Partial FOM coverage:/)).toBeVisible();
    await navigate(page, "Logical time");
    await expect(page.getByRole("heading", { name: "Logical-time observations", exact: true })).toBeVisible();
  });
});

test.describe("recording capability degradation", () => {
  test.use({ backendVariant: "java-limited" });
  test("an older HTTP-only backend keeps independent views without recording resources", async ({ page }) => {
    await page.goto("/#recordings");
    await expect(page.getByText(/Recording unavailable:/)).toBeVisible();
    await expect(page.getByRole("button", { name: "Start recording", exact: true })).toBeDisabled();
    await navigate(page, "Events");
    await expect(page.getByRole("region", { name: "Event explorer" }).locator("tbody tr")).toHaveCount(6);
  });
});
