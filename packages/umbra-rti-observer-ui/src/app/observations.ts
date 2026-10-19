import type { EventEnvelope, JsonValue } from "../api/types";

export function eventFederate(event: EventEnvelope): string | undefined {
  const value = event.federate ?? event.participant;
  if (typeof value === "string" && value.trim()) return value.trim();
  if (value && typeof value === "object" && !Array.isArray(value)) {
    for (const field of ["name", "id", "handle"]) {
      const item: JsonValue | undefined = value[field] as JsonValue | undefined;
      if ((typeof item === "string" || typeof item === "number") && String(item).trim()) return String(item).trim();
    }
  }
  return event.federate_name ?? event.participant_name ?? undefined;
}

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
