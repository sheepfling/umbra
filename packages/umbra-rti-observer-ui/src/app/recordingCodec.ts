import type { JsonObject, JsonValue, PortableRecording, RecordingEdit, RecordingFrame, StateResponse } from "../api/types";
import type { MetricSample } from "./useObserver";

const MAX_STATE_BYTES = 8 * 1024 * 1024;
const MAX_WORK_BYTES = 1024 * 1024 * 1024;
const MAX_EDITS = 100000;
const object = (value: JsonValue): value is JsonObject => value !== null && typeof value === "object" && !Array.isArray(value);
const own = (target: object, key: PropertyKey) => Object.prototype.hasOwnProperty.call(target, key);

function child(target: JsonValue, key: string | number): JsonValue {
  if (object(target) && typeof key === "string" && own(target, key)) return target[key]!;
  if (Array.isArray(target) && Number.isSafeInteger(key) && typeof key === "number" && key >= 0 && key < target.length) return target[key]!;
  throw new Error("Edit path does not address an existing own key or bounded index.");
}

export function applyRecordingEdits(state: JsonObject, edits: RecordingEdit[]): JsonObject {
  if (edits.length > 20000) throw new Error("Frame edit limit exceeded.");
  let root = state;
  for (const edit of edits) {
    if (edit.path.length > 96 || edit.path.some((key) => typeof key !== "string" && !(Number.isSafeInteger(key) && key >= 0))) throw new Error("Invalid edit path.");
    let target: JsonValue = root;
    if (edit.op === "splice") {
      for (const key of edit.path) target = child(target, key);
      if (!Array.isArray(target) || !Number.isSafeInteger(edit.index) || !Number.isSafeInteger(edit.delete_count) || edit.index < 0 || edit.index > target.length || edit.delete_count < 0 || edit.delete_count > target.length - edit.index) throw new Error("Splice exceeds the addressed array.");
      // Avoid spreading a potentially large literal into a function call stack.
      const next = [...target.slice(0, edit.index), ...structuredClone(edit.items), ...target.slice(edit.index + edit.delete_count)];
      target.length = 0;
      for (const item of next) target.push(item);
      continue;
    }
    if (!edit.path.length) {
      if (edit.op !== "set" || !object(edit.value)) throw new Error("State root must remain an object.");
      root = structuredClone(edit.value); continue;
    }
    for (const key of edit.path.slice(0, -1)) target = child(target, key);
    const key = edit.path.at(-1)!;
    if (object(target) && typeof key === "string") {
      if (edit.op === "delete") {
        if (!own(target, key)) throw new Error("Delete requires an existing own key.");
        delete target[key];
      } else Object.defineProperty(target, key, { value: structuredClone(edit.value), writable: true, enumerable: true, configurable: true });
    } else if (edit.op === "set" && Array.isArray(target) && typeof key === "number" && Number.isSafeInteger(key) && key >= 0 && key < target.length) target[key] = structuredClone(edit.value);
    else throw new Error("Edit container type does not match its path.");
  }
  return root;
}

function boundedState(state: JsonObject): number {
  const pending: [JsonValue, number][] = [[state, 0]];
  let nodes = 0;
  while (pending.length) {
    const [value, depth] = pending.pop()!;
    if (++nodes > 750000 || depth > 96 || (typeof value === "number" && !Number.isFinite(value))) throw new Error("State depth/node/number limit exceeded.");
    if (value !== null && typeof value === "object") for (const item of Object.values(value)) pending.push([item, depth + 1]);
  }
  const size = new TextEncoder().encode(JSON.stringify(state)).length;
  if (size > MAX_STATE_BYTES) throw new Error("Reconstructed state exceeds 8 MiB.");
  return size;
}

export function metricSample(frame: RecordingFrame): MetricSample {
  return { at: frame.captured_at * 1000, eventCount: frame.state.live_metrics.event_count,
    eventRate: frame.state.live_metrics.event_rate_per_second, errorCount: frame.state.live_metrics.error_count };
}

/** Owned by a worker. Only its current state and last 60 small metric samples
 * are expanded. PostMessage clones results before the next mutation. */
export class RecordingDecoder {
  readonly document: PortableRecording;
  index = -1;
  frame: RecordingFrame | undefined;
  samples: MetricSample[] = [];
  private working: JsonObject | undefined;
  private work = 0;
  constructor(document: PortableRecording) {
    this.document = document;
    if (document.frames.length < 1 || document.frames.length > 2000 || document.captured_bytes > 16 * 1024 * 1024) throw new Error("Invalid recording bounds.");
    if (document.recording_version === "umbra-observer-recording-v2") {
      if (document.codec !== "observer-json-edits-v1" || document.max_state_bytes !== MAX_STATE_BYTES || document.max_decoded_state_bytes !== MAX_WORK_BYTES || document.decoded_state_bytes > MAX_WORK_BYTES || document.edit_count > MAX_EDITS) throw new Error("Unsupported compact recording limits/codec.");
      if (document.frames.reduce((sum, frame) => sum + frame.state_bytes, 0) !== document.decoded_state_bytes || document.frames.reduce((sum, frame) => sum + frame.edits.length, 0) !== document.edit_count) throw new Error("Invalid compact work accounting.");
    }
  }
  reset() { this.index = -1; this.frame = undefined; this.working = undefined; this.work = 0; this.samples = []; }
  advance() {
    const document = this.document, next = this.index + 1;
    if (next >= document.frames.length) return;
    if (document.recording_version === "umbra-observer-recording-v1") this.frame = document.frames[next]!;
    else {
      const row = document.frames[next]!;
      if (row.index !== next || (next === 0 && (row.edits.length || row.arrivals.length || row.change !== "start"))) throw new Error("Invalid compact frame ordering/baseline.");
      this.working ??= structuredClone(document.baseline) as unknown as JsonObject;
      this.working = applyRecordingEdits(this.working, row.edits);
      const actual = boundedState(this.working);
      this.work += Math.max(actual, row.state_bytes);
      if (this.work > MAX_WORK_BYTES) throw new Error("Reconstruction work limit exceeded.");
      this.frame = { index: row.index, offset_seconds: row.offset_seconds, captured_at: row.captured_at,
        change: row.change, arrivals: row.arrivals, state: this.working as unknown as StateResponse };
    }
    this.index = next;
    this.samples = [...this.samples.slice(-59), metricSample(this.frame)];
  }
}

export type ReplayWorkerRequest = { type: "load"; document: PortableRecording } | { type: "seek"; index: number };
export type ReplayWorkerResponse = { type: "frame"; index: number; frame: RecordingFrame; samples: MetricSample[] } | { type: "error" };
