import type { EventEnvelope } from "../api/types";
export { eventFederate } from "./participants";

export function formatTime(at: number | null | undefined): string {
  return at == null ? "Not observed" : new Date(at * 1_000).toLocaleTimeString();
}

export function exportEvents(events: EventEnvelope[], label = "observations"): void {
  const blob = new Blob([JSON.stringify({ exported_at: new Date().toISOString(), events }, null, 2)], { type: "application/json" });
  const url = URL.createObjectURL(blob);
  const link = document.createElement("a");
  link.href = url;
  link.download = `umbra-${label}-${Date.now()}.json`;
  link.click();
  window.setTimeout(() => URL.revokeObjectURL(url), 1_000);
}
