import { test, expect } from "./fixtures";
import { navigate } from "./support";

test.describe("limited provider", () => {
  test.use({ backendVariant: "limited" });
  test("missing MOM, ownership and regions do not blank useful views", async ({ page }) => {
    await page.goto("/#diagnostics");
    for (const name of ["Management object model", "Attribute ownership", "Regions and DDM", "Attribute ownership inspection", "Region and DDM inspection", "Management report inspection"]) {
      const capability = page.locator("article.capability").filter({ has: page.locator("strong", { hasText: new RegExp(`^${name}$`, "i") }) });
      await expect(capability.getByText("unsupported", { exact: true })).toBeVisible();
    }
    await navigate(page, "Logical time");
    await expect(page.getByRole("heading", { name: "Reported logical timestamps" })).toBeVisible();
    await navigate(page, "Inspectors");
    await expect(page.getByRole("article", { name: "Objects", exact: true })).toContainText("ExampleObject");
    await navigate(page, "Federates");
    await expect(page.getByRole("complementary", { name: "Observed federates" })).toContainText("ShowcaseServer");
    await navigate(page, "Actions");
    await expect(page.getByRole("button", { name: "Connect", exact: true })).toBeEnabled();
  });
});

test.describe("read-only backend", () => {
  test.use({ backendVariant: "read-only" });
  test("omitted command endpoints leave the observations working", async ({ page }) => {
    const controlRequests: string[] = [];
    page.on("request", (request) => { if (request.url().includes("/api/commands")) controlRequests.push(request.url()); });
    await page.goto("/#actions");
    await expect(page.getByRole("heading", { name: "Session controls are not enabled" })).toBeVisible();
    await navigate(page, "Events");
    await expect(page.getByRole("region", { name: "Event explorer" }).locator("tbody tr").first()).toBeVisible();
    await navigate(page, "Federation");
    await expect(page.getByRole("heading", { name: "Traffic over time" })).toBeVisible();
    expect(controlRequests).toEqual([]);
  });
});

test.describe("analytics disabled", () => {
  test.use({ backendVariant: "no-analytics" });
  test("metric/event fallback and controls remain usable without analytics", async ({ page }) => {
    const analyticsRequests: string[] = [];
    page.on("request", (request) => { if (request.url().includes("/api/analytics")) analyticsRequests.push(request.url()); });
    await page.goto("/");
    await expect(page.getByRole("heading", { name: "Windowed analytics unavailable" })).toBeVisible();
    await expect(page.getByRole("article", { name: "Retained events", exact: true })).toBeVisible();
    await navigate(page, "Logical time");
    await expect(page.getByRole("heading", { name: "Time analytics unavailable" })).toBeVisible();
    await navigate(page, "Declarations");
    await expect(page.getByRole("article", { name: "Reported declarations" }).locator("tbody tr").first()).toBeVisible();
    await navigate(page, "Events");
    await expect(page.getByRole("region", { name: "Event explorer" }).locator("tbody tr").first()).toBeVisible();
    await navigate(page, "Actions");
    await expect(page.getByRole("button", { name: "Connect", exact: true })).toBeEnabled();
    expect(analyticsRequests).toEqual([]);
  });
});

test.describe("empty backend", () => {
  test.use({ backendVariant: "empty" });
  test("empty and unknown features are explicit instead of a broken screen", async ({ page }) => {
    await page.goto("/#federates");
    await expect(page.getByRole("heading", { name: "No federates observed" })).toBeVisible();
    await navigate(page, "Events");
    await expect(page.getByText("No events match the current filters.")).toBeVisible();
    await expect(page.getByRole("button", { name: "Export JSON" })).toBeDisabled();
    await navigate(page, "Inspectors");
    await expect(page.getByText("No objects observed.")).toBeVisible();
    await expect(page.getByText("No interactions observed.")).toBeVisible();
    await navigate(page, "Logical time");
    await expect(page.getByRole("heading", { name: "Logical time is not available" })).toBeVisible();
    await navigate(page, "Diagnostics");
    await expect(page.getByRole("heading", { name: "Capabilities" })).toBeVisible();
    await expect(page.locator("article.capability").filter({ hasText: "unknown" }).first()).toBeVisible();
  });
});
