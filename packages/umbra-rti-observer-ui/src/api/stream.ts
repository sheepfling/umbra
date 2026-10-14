import type { EventEnvelope, StateResponse } from "./types";

export interface SnapshotMessage {
  type: "snapshot";
  state: StateResponse;
}

export interface EventsMessage {
  type: "events";
  events: EventEnvelope[];
  state: StateResponse;
}

export interface HeartbeatMessage {
  type: "heartbeat";
}

export interface GapMessage {
  type: "gap";
  retention: {
    first_retained_sequence: number;
    last_sequence: number;
    retained_events: number;
    dropped_events: number;
    cleared_events: number;
  };
}

export interface ErrorMessage {
  type: "error";
  message: string;
}

export type StreamMessage =
  | SnapshotMessage
  | EventsMessage
  | HeartbeatMessage
  | GapMessage
  | ErrorMessage;

export function parseStreamMessage(value: unknown): StreamMessage | undefined {
  if (!isRecord(value) || typeof value.type !== "string") return undefined;
  if (value.type === "snapshot" && isRecord(value.state)) return value as unknown as SnapshotMessage;
  if (value.type === "events" && isRecord(value.state) && Array.isArray(value.events)) return value as unknown as EventsMessage;
  if (value.type === "heartbeat") return { type: "heartbeat" };
  if (value.type === "gap" && isRecord(value.retention)) return value as unknown as GapMessage;
  if (value.type === "error" && typeof value.message === "string") return { type: "error", message: value.message };
  return undefined;
}

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === "object" && value !== null;
}
