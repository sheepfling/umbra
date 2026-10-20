import type { APIRequestContext, Page } from "@playwright/test";
import type { components } from "../src/api/generated";
import { expect } from "./fixtures";

export function legacyPerspective(name: string, source = "e2e", session: string | null = null, federation: string | null = null): string {
  return JSON.stringify(["legacy-provenance", source, session, federation, name]);
}

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
