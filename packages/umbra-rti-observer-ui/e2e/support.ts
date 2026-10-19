import type { APIRequestContext, Page } from "@playwright/test";
import type { components } from "../src/api/generated";
import { expect } from "./fixtures";

export async function navigate(page: Page, name: string): Promise<void> {
  const button = page.getByRole("navigation", { name: "Observer views" })
    .getByRole("button", { name, exact: true });
  await button.click();
  await expect(button).toHaveAttribute("aria-current", "page");
}

export async function seedEvents(request: APIRequestContext,
  events: components["schemas"]["EventBatchRequest"]["events"]): Promise<void> {
  const response = await request.post("/api/events", { data: { events } });
  expect(response.ok(), await response.text()).toBeTruthy();
}

export async function eventCount(request: APIRequestContext): Promise<number> {
  const response = await request.get("/api/state?include_events=false");
  expect(response.ok()).toBeTruthy();
  const state = await response.json() as components["schemas"]["StateResponse-Output"];
  return state.live_metrics.event_count;
}
