import { readFile } from "node:fs/promises";
import { test, expect } from "./fixtures";
import { navigate, seedEvents } from "./support";

test.describe("observer alert workflow", () => {
  test.use({ backendVariant: "empty", observerArgs: ["--max-events", "2"] });
  test("validated rules react to expired error events, preserve evidence through clear and download configuration only", async ({ page, request }) => {
    await page.goto("/#alerts");
    await page.getByLabel("Rule name", { exact: true }).fill("Error burst");
    await page.getByLabel("Alert threshold", { exact: true }).fill("2");
    await page.getByRole("button", { name: "Create alert rule", exact: true }).click();
    const rules = page.getByRole("article", { name: "Current alert rules" });
    await expect(rules).toContainText("Error burst");
    await seedEvents(request, [{ event_type: "runtime.error", details: { private_marker: "not-in-configuration" } }, { event_type: "runtime.error" },
      { event_type: "callback.invoked" }, { event_type: "callback.invoked" }]);
    await expect(rules).toContainText("firing");
    await expect(rules).toContainText("2 observations");
    await request.delete("/api/events");
    await expect(rules).toContainText("firing");
    await page.reload();
    await expect(rules).toContainText("Error burst");
    const history = page.getByRole("article", { name: "Alert evidence history" });
    await history.getByRole("button", { name: /Acknowledge evidence/ }).first().click();
    await expect(history).toContainText("Acknowledged");
    await expect(rules).toContainText("firing");
    const download = page.waitForEvent("download");
    await page.getByRole("button", { name: "Download alert configuration", exact: true }).click();
    const document = await (await download).path();
    const exported = await readFile(document!, "utf8");
    expect(exported).toContain("Error burst");
    expect(exported).not.toMatch(/not-in-configuration|history|instance_id|api_paths|captured_at/);
    await page.getByRole("button", { name: "Pause view", exact: true }).click();
    await expect(rules.getByRole("button", { name: "Disable Error burst", exact: true })).toBeDisabled();
    await page.getByRole("button", { name: "Resume live view", exact: true }).click();
    page.on("dialog", (dialog) => void dialog.accept());
    await rules.getByRole("button", { name: "Reset baseline Error burst", exact: true }).click();
    await expect(rules).toContainText("clear");
    await expect(history).toContainText("errors-threshold");
  });

  test("stale observations mean idle until an exact explicit disconnect; unrelated RTIs do not close the rule", async ({ page, request }) => {
    await page.goto("/#alerts");
    await page.getByLabel("Rule name", { exact: true }).fill("Alice freshness");
    await page.getByLabel("Alert condition", { exact: true }).selectOption("stale-observations");
    await page.getByLabel("Alert threshold", { exact: true }).fill("0.1");
    await page.getByLabel("Alert source", { exact: true }).fill("provider");
    await page.getByLabel("Alert federate", { exact: true }).fill("Alice");
    await page.getByLabel("Alert session ID", { exact: true }).fill("join");
    await page.getByRole("button", { name: "Create alert rule", exact: true }).click();
    const rules = page.getByRole("article", { name: "Current alert rules" });
    await expect(rules).toContainText("not-observed");
    await seedEvents(request, [{ event_type: "object.updated", source: "provider", federate: "Alice", extensions: { session_id: "join" }, object_key: "one" }]);
    await expect(rules).toContainText("observations-idle");
    await seedEvents(request, [{ event_type: "session.disconnected", source: "other", extensions: { session_id: "join" } }]);
    await expect(rules).not.toContainText("explicit-disconnect");
    await seedEvents(request, [{ event_type: "session.disconnected", source: "provider", extensions: { session_id: "join" } }]);
    await expect(rules).toContainText("explicit-disconnect");
    await expect(rules).toContainText("ended");
    await seedEvents(request, [{ event_type: "object.updated", source: "provider", federate: "Alice", extensions: { session_id: "join" }, object_key: "one" }]);
    await expect(rules).toContainText("ended");
  });

  test("imports reject unknown fields without following URLs and pending rules remain unknown without commands", async ({ page }) => {
    await page.goto("/#alerts");
    await page.getByLabel("Rule name", { exact: true }).fill("Pending work");
    await page.getByLabel("Alert condition", { exact: true }).selectOption("pending-commands");
    await page.getByRole("button", { name: "Create alert rule", exact: true }).click();
    const rules = page.getByRole("article", { name: "Current alert rules" });
    await expect(rules).toContainText("commands-unavailable");
    let external = false;
    await page.route("https://must-not-follow.invalid/**", (route) => { external = true; return route.abort(); });
    page.on("dialog", (dialog) => void dialog.accept());
    await page.getByLabel("Import alert configuration", { exact: false }).setInputFiles({ name: "unsafe.json", mimeType: "application/json",
      buffer: Buffer.from(JSON.stringify({ configuration_version: "umbra-observer-alerts-v1", rules: [], api_paths: { alerts: "https://must-not-follow.invalid/read" } })) });
    await expect(page.getByRole("alert")).toContainText("rejected");
    await expect(rules).toContainText("Pending work");
    expect(external).toBeFalsy();
    await navigate(page, "Events");
    await expect(page.getByRole("region", { name: "Event explorer" })).toBeVisible();
  });

  test("a remote configuration change makes an existing edit stale without overwriting it", async ({ page, request }) => {
    await page.goto("/#alerts");
    await page.getByLabel("Rule name", { exact: true }).fill("Original");
    await page.getByRole("button", { name: "Create alert rule", exact: true }).click();
    await page.getByRole("button", { name: "Edit Original", exact: true }).click();
    await page.getByLabel("Rule name", { exact: true }).fill("Local draft");
    const current = await (await request.get("/api/alerts")).json();
    const changed = await request.put("/api/alerts/configuration", { data: { expected_instance_id: current.instance_id, expected_revision: current.revision,
      configuration: { configuration_version: "umbra-observer-alerts-v1", rules: current.rules.map((row: object) => ({ ...row, name: "Remote update" })) } } });
    expect(changed.ok()).toBeTruthy();
    await expect(page.getByText(/Configuration changed after editing began/)).toBeVisible();
    await expect(page.getByRole("button", { name: "Save alert changes", exact: true })).toBeDisabled();
    await page.getByRole("button", { name: "Cancel editing", exact: true }).click();
    await expect(page.getByRole("article", { name: "Current alert rules" })).toContainText("Remote update");
  });
});

test.describe("Java read-only alert contracts", () => {
  test.use({ backendVariant: "java-retention-loss" });
  test("the same UI reads Java alert evidence without offering mutation or disabling unrelated views", async ({ page }) => {
    await page.goto("/#alerts");
    await expect(page.getByText(/Read-only alert evidence:/)).toBeVisible();
    const rules = page.getByRole("article", { name: "Current alert rules" });
    await expect(rules).toContainText("Fixture history loss");
    await expect(rules).toContainText("history-loss-threshold");
    await expect(rules).toContainText("commands-unavailable");
    await expect(page.getByRole("button", { name: "Create alert rule", exact: true })).toBeDisabled();
    await navigate(page, "Logical time");
    await expect(page.getByRole("heading", { name: "Logical-time observations", exact: true })).toBeVisible();
  });
  for (const mode of ["older", "disabled", "unsupported"] as const) test(`${mode} alert support leaves observation views usable`, async ({ page }) => {
    await page.route("**/api/state?**", async (route) => {
      const response = await route.fetch(); const body = await response.json();
      if (mode === "older") delete body.capabilities.alerts; else body.capabilities.alerts.state = mode;
      await route.fulfill({ json: body });
    });
    await page.goto("/#alerts");
    await expect(page.getByRole("heading", { name: "Observer alerts unavailable", exact: true })).toBeVisible();
    await navigate(page, "Events");
    await expect(page.getByRole("region", { name: "Event explorer" })).toBeVisible();
    await page.unrouteAll({ behavior: "wait" });
  });
});
