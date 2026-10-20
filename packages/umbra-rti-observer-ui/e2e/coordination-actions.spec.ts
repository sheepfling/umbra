import type { APIRequestContext, Locator, Page } from "@playwright/test";
import type { CommandCatalog, CommandHistory, CommandRecord, StateResponse } from "../src/api/types";
import { test, expect } from "./fixtures";
import { navigate } from "./support";

async function invoke(page: Page, request: APIRequestContext, panel: Locator, title: string, fields: Record<string, string> = {}) {
  await panel.getByRole("button", { name: new RegExp(`^${title} available$`) }).click();
  for (const [label, value] of Object.entries(fields)) {
    const field = panel.getByLabel(label, { exact: true });
    if (await field.evaluate((element) => element.tagName === "SELECT")) await field.selectOption(value);
    else await field.fill(value);
  }
  const confirmation = panel.getByRole("checkbox");
  if (await confirmation.count()) await confirmation.check();
  const submitted = page.waitForResponse((response) => response.url().endsWith("/api/commands") && response.request().method() === "POST");
  await panel.getByRole("button", { name: title, exact: true }).click();
  const initial = await (await submitted).json() as CommandRecord;
  await expect.poll(async () => {
    const history = await (await request.get("/api/commands")).json() as CommandHistory;
    return history.records.find((row) => row.command_id === initial.command_id)?.status;
  }).toBe("succeeded");
  const catalog = await (await request.get("/api/commands/catalog")).json() as CommandCatalog;
  await expect(page.locator(".owned-target-version")).toContainText(`revision ${catalog.targets[0]!.version!.revision}.`);
}

async function reported(request: APIRequestContext, kind: string, phase: string, label: string) {
  const state = await (await request.get("/api/state")).json() as StateResponse;
  return state.inspectors.coordination?.snapshots.some((row) => row.source === "showcase-session"
    && row.kind === kind && row.phase === phase && row.scope === "federation" && row.label === label) ?? false;
}

test("owned sync, save and failed restore produce explicit reports and a scoped drill-down", async ({ page, request }) => {
  await page.goto("/#actions");
  const lifecycle = page.getByRole("article", { name: "Session lifecycle actions" });
  const coordination = page.getByRole("article", { name: "Coordination actions" });
  await expect(coordination.getByRole("button", { name: "Register synchronization", exact: true })).toBeDisabled();
  await invoke(page, request, lifecycle, "Connect");
  await invoke(page, request, lifecycle, "Join federation", { "Federate name": "TourObserver" });
  await invoke(page, request, coordination, "Register synchronization", { "Operation label": "Tour barrier", "Tag text": "junior walkthrough" });
  await expect.poll(() => reported(request, "synchronization", "announced", "Tour barrier")).toBe(true);
  await invoke(page, request, coordination, "Report synchronization achievement", { "Operation label": "Tour barrier", "Local result": "achieved" });
  await expect.poll(() => reported(request, "synchronization", "completed", "Tour barrier")).toBe(true);
  await invoke(page, request, coordination, "Request save", { "Operation label": "Tour checkpoint" });
  await expect.poll(() => reported(request, "save", "requested", "Tour checkpoint")).toBe(true);
  await invoke(page, request, coordination, "Report save begun");
  await invoke(page, request, coordination, "Report save result", { "Local result": "completed" });
  await expect.poll(() => reported(request, "save", "completed", "Tour checkpoint")).toBe(true);
  await invoke(page, request, coordination, "Request restore", { "Operation label": "Tour checkpoint" });
  await expect.poll(() => reported(request, "restore", "begun", "Tour checkpoint")).toBe(true);
  await invoke(page, request, coordination, "Report restore result", { "Local result": "failed" });
  await expect.poll(() => reported(request, "restore", "failed", "Tour checkpoint")).toBe(true);
  await page.getByRole("button", { name: "Pause view" }).click();
  await expect(coordination.getByRole("button", { name: "Report restore result", exact: true })).toBeDisabled();
  await page.getByRole("button", { name: "Resume live view" }).click();
  await navigate(page, "Save / restore");
  await page.getByLabel("Search coordination reports").fill("Tour checkpoint");
  await page.getByLabel("Coordination kind").selectOption("restore");
  await page.getByRole("article", { name: "Latest coordination reports" }).getByRole("button", { name: "Follow operation" }).first().click();
  const progress = page.getByRole("article", { name: "Selected coordination operation" });
  await expect(progress).toContainText("TourObserver");
  await expect(progress).toContainText("Federation report");
  await expect(progress).toContainText("failed");
});

test.describe("limited coordination controls", () => {
  test.use({ backendVariant: "limited" });
  test("omitted coordination actions preserve lifecycle and observation", async ({ page, request }) => {
    await page.goto("/#actions");
    const lifecycle = page.getByRole("article", { name: "Session lifecycle actions" });
    await expect(page.getByRole("article", { name: "Coordination actions" })).toContainText("has not enabled coordination controls");
    await invoke(page, request, lifecycle, "Connect");
    await invoke(page, request, lifecycle, "Join federation");
    await navigate(page, "Logical time");
    await expect(page.getByRole("heading", { name: "Logical-time observations", exact: true })).toBeVisible();
    await navigate(page, "Synchronization");
    await expect(page.getByRole("heading", { name: "Synchronization unavailable" })).toBeVisible();
  });
});

test("rich demo exposes complex graph layouts and every workspace in its walkthrough", async ({ page }) => {
  await page.goto("/#topology");
  await expect(page.getByTestId("topology-edge")).toHaveCount(15);
  const graph = page.getByRole("article", { name: "Explicit relationship graph" });
  for (const layout of ["circle", "components", "layered"]) {
    await page.getByLabel("Topology layout").selectOption(layout);
    await expect(graph.getByRole("button", { name: /^Focus / })).toHaveCount(10);
    await expect(page.getByTestId("topology-edge")).toHaveCount(15);
  }
  await graph.getByRole("button", { name: "Inspect relationship mesh-9 from showcase-umbra", exact: true }).press("Enter");
  await expect(page.getByRole("region", { name: "Selected relationship" })).toContainText("Monitor");
  await page.getByRole("button", { name: "Demo walkthrough", exact: true }).click();
  const tour = page.getByRole("region", { name: "Demo walkthrough", exact: true });
  await expect(tour.getByRole("button")).toHaveCount(19);
  await tour.getByRole("button", { name: "Recordings", exact: true }).click();
  await expect(page.getByRole("heading", { name: "Recordings and replay", exact: true })).toBeVisible();
});

test("demo connects workspace configuration, pause, recording/replay and alert modalities", async ({ page }) => {
  await page.goto("/#events");
  await page.getByRole("button", { name: "Saved workspaces", exact: true }).click();
  await page.getByLabel("Workspace name").fill("Demo tour configuration");
  await page.getByRole("button", { name: "Save current workspace" }).click();
  await expect(page.getByLabel("Saved workspace", { exact: true })).toHaveValue("Demo tour configuration");
  await page.getByRole("button", { name: "Saved workspaces", exact: true }).click();
  await page.getByRole("button", { name: "Pause view" }).click();
  await expect(page.getByText(/View paused at/)).toBeVisible();
  await page.getByRole("button", { name: "Resume live view" }).click();
  await navigate(page, "Recordings");
  const capture = page.getByRole("article", { name: "Backend recording" });
  await capture.getByRole("button", { name: "Start recording", exact: true }).click();
  await navigate(page, "Federation");
  await expect(page.getByRole("heading", { name: "Traffic over time", exact: true })).toBeVisible();
  await navigate(page, "Recordings");
  await capture.getByRole("button", { name: "Stop recording" }).click();
  await capture.getByRole("button", { name: "Replay retained recording" }).click();
  await expect(page.getByRole("region", { name: "Replay controls" })).toBeVisible();
  await navigate(page, "Actions");
  await expect(page.getByText(/Replay is read-only. Captured capabilities do not grant command authority/)).toBeVisible();
  await page.getByRole("button", { name: "Return to live" }).click();
  await navigate(page, "Alerts");
  await page.getByLabel("Rule name").fill("Tour error rule");
  await page.getByRole("button", { name: "Create alert rule" }).click();
  await expect(page.getByRole("article", { name: "Current alert rules" })).toContainText("Tour error rule");
});
