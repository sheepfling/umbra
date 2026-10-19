import { readFile } from "node:fs/promises";
import { test, expect } from "./fixtures";
import { navigate, seedEvents } from "./support";

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
    await page.getByLabel("Object perspective", { exact: true }).selectOption("f:Alice");
    await expect(page.getByLabel("Numeric attribute", { exact: true })).toHaveValue("a");
    await page.getByLabel("Object perspective", { exact: true }).selectOption("global");
    await expect(page.getByLabel("Numeric attribute", { exact: true })).toHaveValue("b");
    await page.reload();
    await expect(page.getByLabel("Numeric attribute", { exact: true })).toHaveValue("b");
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
