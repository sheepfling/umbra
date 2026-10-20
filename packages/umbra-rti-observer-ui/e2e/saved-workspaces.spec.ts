import { readFile } from "node:fs/promises";
import { test, expect } from "./fixtures";
import { legacyPerspective, navigate, seedEvents } from "./support";

const storageKey = "umbra.observer.workspace-library.v1";
const document = (name: string, settings: object = {}) => ({ workspace_version: "umbra-observer-workspace-v1", name, settings });

test.describe("saved view configurations", () => {
  test.use({ backendVariant: "empty" });

  test("save/reload/apply restores view filters and window without exporting observations or sending actions", async ({ page, request }) => {
    await seedEvents(request, [{ event_type: "object.updated", source: "test-source", federate: "Alice", object_key: "one", attributes: { marker: "private-payload-value" } }]);
    const writes: string[] = [];
    page.on("request", (request) => { if (request.method() !== "GET") writes.push(request.url()); });
    await page.goto("/#events");
    await page.getByLabel("Chart time window", { exact: true }).selectOption("900");
    await page.getByLabel("Search events", { exact: true }).fill("one");
    await page.getByLabel("Filter by source", { exact: true }).selectOption("test-source");
    await page.getByRole("button", { name: "Saved workspaces", exact: true }).click();
    await page.getByLabel("Workspace name", { exact: true }).fill("Miguel events");
    await page.getByRole("button", { name: "Save current workspace", exact: true }).click();
    await expect(page.getByLabel("Saved workspace", { exact: true })).toHaveValue("Miguel events");
    await page.reload();
    await expect(page.getByLabel("Search events", { exact: true })).toHaveValue("one");
    await expect(page.getByLabel("Filter by source", { exact: true })).toHaveValue("test-source");
    await expect(page.getByLabel("Chart time window", { exact: true })).toHaveValue("900");
    await page.getByLabel("Search events", { exact: true }).fill("different");
    await navigate(page, "Federation");
    await page.getByRole("button", { name: "Saved workspaces", exact: true }).click();
    await page.getByRole("button", { name: "Apply saved workspace", exact: true }).click();
    await expect(page.getByLabel("Search events", { exact: true })).toHaveValue("one");
    const download = page.waitForEvent("download");
    await page.getByRole("button", { name: "Download saved workspace", exact: true }).click();
    const contents = await readFile((await (await download).path())!, "utf8");
    expect(contents).toContain("Miguel events");
    expect(contents).not.toMatch(/private-payload-value|normalized_events|api_paths|credential|confirmed|recording_id|instance_id/);
    expect(writes).toEqual([]);
  });

  test("numeric attribute choices persist for their exact object perspective", async ({ page, request }) => {
    await seedEvents(request, [{ event_type: "object.updated", source: "test-source", federate: "Alice", object_key: "one", attributes: { a: 1, b: 2 } }]);
    await page.goto("/#objects");
    await page.getByLabel("Numeric attribute", { exact: true }).selectOption("b");
    await page.getByLabel("Object perspective", { exact: true }).selectOption(`f:${legacyPerspective("Alice", "test-source")}`);
    await expect(page.getByLabel("Numeric attribute", { exact: true })).toHaveValue("a");
    await page.getByLabel("Object perspective", { exact: true }).selectOption("global");
    await expect(page.getByLabel("Numeric attribute", { exact: true })).toHaveValue("b");
    await page.reload();
    await expect(page.getByLabel("Numeric attribute", { exact: true })).toHaveValue("b");
  });

  test("Legacy reporting sessions retain independent selections and filters across switches and reload without altering global views", async ({ page, request }) => {
    await seedEvents(request, [
      { event_type: "time.observed", source: "e2e", federate: "Alice", time_observation: { kind: "grant", session_id: "a1", logical_time: 1 } },
      { event_type: "time.observed", source: "e2e", federate: "Alice", time_observation: { kind: "grant", session_id: "a2", logical_time: 2 } },
      { event_type: "time.observed", source: "e2e", federate: "Bob", time_observation: { kind: "grant", session_id: "b1", logical_time: 3 } },
      { event_type: "time.observed", source: "e2e", federate: "Bob", time_observation: { kind: "grant", session_id: "b2", logical_time: 4 } },
    ]);
    await page.goto("/#time");
    const globalTime = JSON.stringify(["e2e", "b2", null, "Bob"]);
    await page.getByLabel("Time perspective", { exact: true }).selectOption(globalTime);
    await navigate(page, "Federates");
    const people = page.getByRole("complementary", { name: "Observed federates" });
    await people.getByRole("button", { name: /^Alice .*a1/ }).click();
    await page.getByLabel("Time perspective", { exact: true }).selectOption({ index: 0 });
    const alice = await page.getByLabel("Time perspective", { exact: true }).inputValue();
    await page.getByLabel("Search events", { exact: true }).fill("alice-only");
    await page.getByLabel("Search declarations", { exact: true }).fill("alice-classes");
    await people.getByRole("button", { name: /^Bob .*b1/ }).click();
    await expect(page.getByLabel("Search events", { exact: true })).toHaveValue("");
    await expect(page.getByLabel("Search declarations", { exact: true })).toHaveValue("");
    await page.getByLabel("Time perspective", { exact: true }).selectOption({ index: 0 });
    const bob = await page.getByLabel("Time perspective", { exact: true }).inputValue();
    expect(bob).not.toBe(alice);
    await page.getByLabel("Search events", { exact: true }).fill("bob-only");
    await page.reload();
    await expect(page.getByLabel("Search events", { exact: true })).toHaveValue("bob-only");
    await expect(page.getByLabel("Time perspective", { exact: true })).toHaveValue(bob);
    await people.getByRole("button", { name: /^Alice .*a1/ }).click();
    await expect(page.getByLabel("Search events", { exact: true })).toHaveValue("alice-only");
    await expect(page.getByLabel("Search declarations", { exact: true })).toHaveValue("alice-classes");
    await expect(page.getByLabel("Time perspective", { exact: true })).toHaveValue(alice);
    await navigate(page, "Events");
    await expect(page.getByLabel("Search events", { exact: true })).toHaveValue("");
    await navigate(page, "Declarations");
    await expect(page.getByLabel("Search declarations", { exact: true })).toHaveValue("");
    await navigate(page, "Logical time");
    await expect(page.getByLabel("Time perspective", { exact: true })).toHaveValue(globalTime);
  });

  test("unavailable saved participants and filters stay selected instead of borrowing another participant", async ({ page, request }) => {
    await seedEvents(request, [{ event_type: "object.updated", federate: "Bob", object_key: "one" }]);
    await page.goto("/");
    await page.getByRole("button", { name: "Saved workspaces", exact: true }).click();
    await page.getByLabel("Import workspace configuration", { exact: false }).setInputFiles({ name: "alice.json", mimeType: "application/json",
      buffer: Buffer.from(JSON.stringify(document("Alice investigation", { view: "federates", selected_federate: "Alice", events: { source: "absent-source" } }))) });
    await expect(page.getByLabel("Saved workspace", { exact: true })).toHaveValue("Alice investigation");
    await page.getByRole("button", { name: "Apply saved workspace", exact: true }).click();
    await expect(page.getByRole("heading", { name: "Alice", exact: true })).toBeVisible();
    await expect(page.getByText(/Roster metadata is not retained/)).toBeVisible();
    await navigate(page, "Events");
    await expect(page.getByLabel("Filter by source", { exact: true })).toHaveValue("absent-source");
    await expect(page.getByLabel("Filter by source", { exact: true })).toContainText("Saved selection");
  });

  test("imports reject captured data and URLs, never apply automatically and remain disabled for application during pause", async ({ page }) => {
    await page.goto("/#events");
    await page.getByRole("button", { name: "Saved workspaces", exact: true }).click();
    const upload = page.getByLabel("Import workspace configuration", { exact: false });
    await upload.setInputFiles({ name: "unsafe.json", mimeType: "application/json", buffer: Buffer.from(JSON.stringify({ ...document("Unsafe"), api_paths: { state: "https://never-follow.invalid" } })) });
    await expect(page.getByRole("alert")).toContainText("Invalid workspace");
    await upload.setInputFiles({ name: "safe.json", mimeType: "application/json", buffer: Buffer.from(JSON.stringify(document("Saved time", { view: "time", window_seconds: 30 }))) });
    await expect(page.getByLabel("Saved workspace", { exact: true })).toHaveValue("Saved time");
    await expect(page.getByRole("navigation", { name: "Observer views" }).getByRole("button", { name: "Events", exact: true })).toHaveAttribute("aria-current", "page");
    await page.getByRole("button", { name: "Pause view", exact: true }).click();
    await expect(page.getByRole("button", { name: "Apply saved workspace", exact: true })).toBeDisabled();
    await page.getByRole("button", { name: "Resume live view", exact: true }).click();
    await page.getByRole("button", { name: "Apply saved workspace", exact: true }).click();
    await expect(page.getByLabel("Chart time window", { exact: true })).toHaveValue("30");
  });

  test("corrupt browser configuration is preserved and blocked storage does not break observation views", async ({ page }) => {
    await page.addInitScript((key) => localStorage.setItem(key, '{"library_version":"future","private":"keep-original"}'), storageKey);
    await page.goto("/#events");
    await expect(page.getByText(/The original browser copy was kept unchanged/)).toBeVisible();
    await expect(page.getByRole("region", { name: "Event explorer" })).toBeVisible();
    expect(await page.evaluate((key) => localStorage.getItem(key), storageKey)).toContain("keep-original");
    await page.getByRole("button", { name: "Saved workspaces", exact: true }).click();
    await expect(page.getByRole("button", { name: "Save current workspace", exact: true })).toBeDisabled();
  });

  test("browser storage failures are explicit and do not disable the active view", async ({ page }) => {
    await page.addInitScript(() => { Storage.prototype.setItem = () => { throw new DOMException("Full", "QuotaExceededError"); }; });
    await page.goto("/#events");
    await expect(page.getByText(/Current settings could not be saved/)).toBeVisible();
    await expect(page.getByRole("region", { name: "Event explorer" })).toBeVisible();
  });

  test("another tab never silently overwrites the active investigation; explicit reload reconciles the browser copy", async ({ page }) => {
    await page.goto("/#events");
    await page.getByLabel("Search events", { exact: true }).fill("first-tab");
    await expect.poll(() => page.evaluate((key) => JSON.parse(localStorage.getItem(key) ?? "null")?.current?.events.search, storageKey)).toBe("first-tab");
    const other = await page.context().newPage();
    try {
      await other.goto(page.url());
      await expect(other.getByLabel("Search events", { exact: true })).toHaveValue("first-tab");
      await other.getByLabel("Search events", { exact: true }).fill("second-tab");
      await expect(page.getByText(/Saved workspaces changed in another tab/)).toBeVisible();
      await expect(page.getByLabel("Search events", { exact: true })).toHaveValue("first-tab");
      await page.getByRole("button", { name: "Saved workspaces", exact: true }).click();
      await expect(page.getByRole("button", { name: "Save current workspace", exact: true })).toBeDisabled();
      await page.getByRole("button", { name: "Reload browser copy", exact: true }).click();
      await expect(page.getByLabel("Search events", { exact: true })).toHaveValue("second-tab");
      await expect(page.getByRole("button", { name: "Save current workspace", exact: true })).toBeEnabled();
    } finally { await other.close(); }
  });

  test("reset removes only this application's settings and does not clear unrelated browser data or observations", async ({ page, request }) => {
    await seedEvents(request, [{ event_type: "runtime.error", details: { message: "keep-this-observation" } }]);
    await page.goto("/#events");
    await page.evaluate(() => localStorage.setItem("unrelated-application", "keep-this-copy"));
    await page.getByRole("button", { name: "Saved workspaces", exact: true }).click();
    await page.getByRole("button", { name: "Save current workspace", exact: true }).click();
    page.once("dialog", (dialog) => dialog.accept());
    await page.getByRole("button", { name: "Reset browser workspaces", exact: true }).click();
    await expect(page.getByLabel("Saved workspace", { exact: true })).toHaveValue("");
    expect(await page.evaluate(() => localStorage.getItem("unrelated-application"))).toBe("keep-this-copy");
    const state = await request.get("/api/state?include_events=true");
    expect((await state.json()).normalized_events).toHaveLength(1);
  });

  test("named capacity failures and declined replacement keep all previously saved choices", async ({ page }) => {
    const saved = Array.from({ length: 12 }, (_, index) => document(`Workspace ${index}`, { view: "events", events: { search: `saved-${index}` } }));
    await page.addInitScript(({ key, saved }) => localStorage.setItem(key, JSON.stringify({ library_version: "umbra-observer-workspace-library-v1", saved })), { key: storageKey, saved });
    await page.goto("/#events");
    await page.getByRole("button", { name: "Saved workspaces", exact: true }).click();
    await page.getByRole("button", { name: "Save current workspace", exact: true }).click();
    await expect(page.getByRole("alert")).toContainText("At most 12 named workspaces");
    await page.getByLabel("Workspace name", { exact: true }).fill("Workspace 0");
    page.once("dialog", (dialog) => dialog.dismiss());
    await page.getByRole("button", { name: "Save current workspace", exact: true }).click();
    await page.getByRole("button", { name: "Apply saved workspace", exact: true }).click();
    await expect(page.getByLabel("Search events", { exact: true })).toHaveValue("saved-0");
    expect(await page.evaluate((key) => JSON.parse(localStorage.getItem(key)!).saved.length, storageKey)).toBe(12);
  });

  test("a delayed import cannot replace a newer same-name profile without fresh confirmation", async ({ page }) => {
    await page.addInitScript(() => {
      const original = File.prototype.text;
      File.prototype.text = async function () {
        const raw = await original.call(this);
        if (this.name === "slow.json") await new Promise<void>((done) => { (window as Window & { releaseWorkspaceImport?: () => void }).releaseWorkspaceImport = done; });
        return raw;
      };
    });
    await page.goto("/#events");
    await page.getByRole("button", { name: "Saved workspaces", exact: true }).click();
    const upload = page.getByLabel("Import workspace configuration", { exact: false });
    await upload.setInputFiles({ name: "slow.json", mimeType: "application/json", buffer: Buffer.from(JSON.stringify(document("Same name", { view: "events", events: { search: "stale-import" } }))) });
    await expect.poll(() => page.evaluate(() => typeof (window as Window & { releaseWorkspaceImport?: () => void }).releaseWorkspaceImport)).toBe("function");
    await upload.setInputFiles({ name: "fast.json", mimeType: "application/json", buffer: Buffer.from(JSON.stringify(document("Same name", { view: "events", events: { search: "newer-profile" } }))) });
    await expect(page.getByLabel("Saved workspace", { exact: true })).toHaveValue("Same name");
    await page.evaluate(() => (window as Window & { releaseWorkspaceImport?: () => void }).releaseWorkspaceImport?.());
    await expect(page.getByRole("alert")).toContainText("changed before replacement was confirmed");
    await page.getByRole("button", { name: "Apply saved workspace", exact: true }).click();
    await expect(page.getByLabel("Search events", { exact: true })).toHaveValue("newer-profile");
  });

  test("replay cannot apply reload or reset live workspace settings", async ({ page, request }) => {
    const started = await request.post("/api/recordings/start", { data: { label: "Workspace isolation" } });
    const { recording_id } = await started.json();
    expect((await request.post("/api/recordings/stop", { data: { recording_id } })).ok()).toBeTruthy();
    const exported = await request.get(`/api/recordings/export?recording_id=${recording_id}`);
    await page.goto("/#recordings");
    await page.getByLabel("Recording file", { exact: true }).setInputFiles({ name: "capture.json", mimeType: "application/json", buffer: Buffer.from(await exported.text()) });
    await page.getByRole("button", { name: "Open recording", exact: true }).click();
    await expect(page.getByRole("region", { name: "Replay controls" })).toBeVisible();
    await page.getByRole("button", { name: "Saved workspaces", exact: true }).click();
    await page.getByLabel("Import workspace configuration", { exact: false }).setInputFiles({ name: "safe.json", mimeType: "application/json", buffer: Buffer.from(JSON.stringify(document("Live view"))) });
    await expect(page.getByLabel("Saved workspace", { exact: true })).toHaveValue("Live view");
    for (const name of ["Apply saved workspace", "Reload browser copy", "Reset browser workspaces"]) {
      await expect(page.getByRole("button", { name, exact: true })).toBeDisabled();
    }
    await page.getByRole("button", { name: "Return to live", exact: true }).click();
    await expect(page.getByRole("button", { name: "Apply saved workspace", exact: true })).toBeEnabled();
  });
});

test.describe("saved settings across investigation screens", () => {
  test.use({ backendVariant: "rich" });
  test("restored filters and exact unavailable selections survive every independent workspace", async ({ page }) => {
    const settings = { view: "events", window_seconds: 900, selected_federate: "absent-participant",
      events: { search: "global-event-filter" }, federate_events: { search: "perspective-event-filter" },
      federation: { distribution: "families" }, objects: { status: "future-object-status", selected: "absent-object" },
      declarations: { search: "global-declarations", kind: "future-kind" }, federate_declarations: { search: "perspective-declarations" },
      topology: { source: "absent-source", focused: "absent-join" }, interactions: { order: "future-order" },
      ownership: { owner_state: "future-owner-state" }, regions: { selected: "absent-region", status: "future-region-status" },
      mom: { counter: "absent-series", service_status: "future-service-status" },
      fom: { selected: "absent-element", model: "absent-model" }, time: { selected: "absent-time-scope" },
      synchronization: { selected: "absent-sync", search: "sync-filter" }, persistence: { selected: "absent-save", search: "save-filter" },
      alerts: { search: "alert-filter" } };
    await page.goto("/#events");
    await page.getByRole("button", { name: "Saved workspaces", exact: true }).click();
    await page.getByLabel("Import workspace configuration", { exact: false }).setInputFiles({ name: "all.json", mimeType: "application/json", buffer: Buffer.from(JSON.stringify(document("All views", settings))) });
    await page.getByRole("button", { name: "Apply saved workspace", exact: true }).click();
    await page.reload();
    await expect(page.getByLabel("Chart time window", { exact: true })).toHaveValue("900");
    const screens: [string, Record<string, string>][] = [
      ["Events", { "Search events": "global-event-filter" }],
      ["Objects", { "Object status": "future-object-status" }],
      ["Declarations", { "Search declarations": "global-declarations", "Declaration kind": "future-kind" }],
      ["Topology", { "Relationship source": "absent-source", "Topology focus": "absent-join" }],
      ["Interaction flow", { "Interaction reported order": "future-order" }],
      ["Ownership", { "Ownership state filter": "future-owner-state" }],
      ["Regions / DDM", { "Selected DDM report": "absent-region", "Region reported status": "future-region-status" }],
      ["MOM reports", { "Management counter series": "absent-series", "Management service status": "future-service-status" }],
      ["FOM browser", { "Selected FOM element": "absent-element", "FOM model revision": "absent-model" }],
      ["Logical time", { "Time perspective": "absent-time-scope" }],
      ["Synchronization", { "Coordination operation": "absent-sync", "Search coordination reports": "sync-filter" }],
      ["Save / restore", { "Coordination operation": "absent-save", "Search coordination reports": "save-filter" }],
      ["Alerts", { "Search alert evidence": "alert-filter" }],
    ];
    for (const [screen, controls] of screens) {
      await navigate(page, screen);
      for (const [label, value] of Object.entries(controls)) await expect(page.getByLabel(label, { exact: true })).toHaveValue(value);
    }
    await navigate(page, "Federates");
    await expect(page.getByRole("heading", { name: "absent-participant", exact: true })).toBeVisible();
    await expect(page.getByLabel("Search events", { exact: true })).toHaveValue("perspective-event-filter");
    await expect(page.getByLabel("Search declarations", { exact: true })).toHaveValue("perspective-declarations");
  });
});

test.describe("workspace portability", () => {
  test.use({ backendVariant: "java-limited" });
  test("local workspace validation works without a Python endpoint or runtime code generation", async ({ page }) => {
    await page.route("**/", async (route) => {
      const response = await route.fetch();
      await route.fulfill({ response, headers: { ...response.headers(), "Content-Security-Policy": "default-src 'self'; script-src 'self'; style-src 'self' 'unsafe-inline'; connect-src 'self' ws:; worker-src 'self'" } });
    });
    await page.goto("/#events");
    await page.getByRole("button", { name: "Saved workspaces", exact: true }).click();
    await page.getByLabel("Import workspace configuration", { exact: false }).setInputFiles({ name: "limited.json", mimeType: "application/json", buffer: Buffer.from(JSON.stringify(document("Limited RTI", { view: "mom", mom: { search: "keep-this-filter" } }))) });
    await expect(page.getByLabel("Saved workspace", { exact: true })).toHaveValue("Limited RTI");
    await page.getByRole("button", { name: "Apply saved workspace", exact: true }).click();
    await expect(page.getByRole("heading", { name: "Management inspection unavailable", exact: true })).toBeVisible();
    await navigate(page, "Events");
    await expect(page.getByRole("region", { name: "Event explorer" })).toBeVisible();
    await page.unrouteAll({ behavior: "wait" });
  });
});
