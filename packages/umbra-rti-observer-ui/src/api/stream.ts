import type { ContractResponse, EventEnvelope, StateResponse } from "./types";
import type { components } from "./generated";

type Generated = components["schemas"];
export type StreamTransport = "websocket" | "sse";
export type StreamCursor = Generated["StreamCursorResponse"];
export type SnapshotMessage = Omit<Generated["SnapshotMessage"], "state"> & { state: StateResponse };
export type EventsMessage = Omit<Generated["EventsMessage"], "state" | "events"> & { state: StateResponse; events: EventEnvelope[] };
export type HeartbeatMessage = Generated["HeartbeatMessage"];
export type GapMessage = Generated["GapMessage"];
export type ErrorMessage = Generated["ErrorMessage"];

export type StreamMessage =
  | SnapshotMessage
  | EventsMessage
  | HeartbeatMessage
  | GapMessage
  | ErrorMessage;

export function parseStreamMessage(value: unknown): StreamMessage | undefined {
  if (!isRecord(value) || typeof value.type !== "string") return undefined;
  if (value.cursor != null && !validCursor(value.cursor)) return undefined;
  if (value.type === "snapshot" || value.type === "events") {
    if (!isRecord(value.state) || !validRetention(value.state.retention)
      || !isRecord(value.state.inspectors) || !isRecord(value.state.live_metrics)
      || !Array.isArray(value.state.federate_roster) || !isRecord(value.state.capabilities)) return undefined;
    if (value.state.observer_lifetime != null && !validLifetime(value.state.observer_lifetime)) return undefined;
    if (value.cursor != null) {
      const lifetime = value.state.observer_lifetime;
      if (!isRecord(lifetime) || lifetime.instance_id !== value.cursor.instance_id || lifetime.revision !== value.cursor.revision
        || !isRecord(value.state.retention) || value.cursor.after >= Number(value.state.retention.next_sequence)) return undefined;
    }
    if (value.type === "snapshot") {
      if (value.resume != null && !["fresh", "resumed", "reset", "gap"].includes(String(value.resume))) return undefined;
      return value as unknown as SnapshotMessage;
    }
    if (!Array.isArray(value.events) || !value.events.every(validEvent)) return undefined;
    return value as unknown as EventsMessage;
  }
  if (value.type === "heartbeat") return value as unknown as HeartbeatMessage;
  if (value.type === "gap" && validRetention(value.retention)) return value as unknown as GapMessage;
  if (value.type === "error" && typeof value.message === "string") return { type: "error", message: value.message };
  return undefined;
}

export function streamTransports(contract: ContractResponse): StreamTransport[] {
  return (["websocket", "sse"] as const).filter((name) => {
    const state = contract.capabilities[name]?.state;
    return (state === "available" || state === "partial")
      && Boolean(contract.api_paths[name === "websocket" ? "websocket" : "event_stream"]);
  });
}

function count(value: unknown): value is number { return typeof value === "number" && Number.isSafeInteger(value) && value >= 0; }
function validLifetime(value: unknown): value is NonNullable<StateResponse["observer_lifetime"]> {
  return isRecord(value) && typeof value.instance_id === "string" && value.instance_id.length > 0
    && value.instance_id.length <= 128 && count(value.revision);
}
function validCursor(value: unknown): value is StreamCursor {
  return isRecord(value) && count(value.after) && validLifetime(value);
}
function validRetention(value: unknown): boolean {
  return isRecord(value) && ["first_retained_sequence", "last_sequence", "retained_events", "dropped_events", "cleared_events"]
    .every((key) => count(value[key]));
}
function validEvent(value: unknown): boolean {
  return isRecord(value) && count(value.sequence) && value.sequence > 0
    && ["event_type", "family", "source"].every((key) => typeof value[key] === "string")
    && typeof value.observed_at === "number" && Number.isFinite(value.observed_at);
}

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === "object" && value !== null;
}
