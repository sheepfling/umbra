import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import test from "node:test";
import { reconcileLive } from "../src/app/liveState.ts";
import { parseStreamMessage, streamTransports } from "../src/api/stream.ts";

const base = JSON.parse(readFileSync(new URL("../../umbra-rti-observer/conformance/fixtures/limited/response-02.json", import.meta.url), "utf8"));
function state(id, revision, sequences) {
  return { ...base, observer_lifetime: id ? { instance_id: id, revision } : undefined,
    normalized_events: sequences.map((sequence) => ({ ...base.normalized_events[0], sequence })),
    retention: { ...base.retention, first_retained_sequence: sequences[0] ?? 0, last_sequence: sequences.at(-1) ?? 0,
      retained_events: sequences.length, next_sequence: (sequences.at(-1) ?? 3) + 1 } };
}
const empty = () => ({ events: new Map() });

test("duplicate backfill is deduplicated and cursor never moves backwards", () => {
  const first = state("a", 1, [1, 2, 3]);
  const before = reconcileLive(empty(), first);
  const summary = { ...first, normalized_events: null };
  const after = reconcileLive(before, summary, first.normalized_events.slice(1), { instance_id: "a", revision: 1, after: 2 });
  assert.deepEqual([...after.events.keys()], [1, 2, 3]);
  assert.equal(after.cursor.after, 3);
});

test("clear and retention cuts reject stale full snapshots without resurrecting rows", () => {
  const before = state("a", 1, [1, 2, 3]);
  const clear = state("a", 2, []);
  let view = reconcileLive(reconcileLive(empty(), before), clear);
  assert.equal(view.events.size, 0);
  assert.strictEqual(reconcileLive(view, before), view);
  const next = state("a", 3, [4, 5]);
  view = reconcileLive(view, next);
  assert.deepEqual([...view.events.keys()], [4, 5]);
  assert.strictEqual(reconcileLive(view, clear), view);
});

test("new observer may reuse sequence numbers, never previous inspector/history rows", () => {
  const before = reconcileLive(empty(), state("a", 10, [8, 9, 10]));
  const next = { ...state("b", 1, [1]), normalized_events: null };
  const after = reconcileLive(before, next, [{ ...base.normalized_events[0], sequence: 1 }]);
  assert.deepEqual([...after.events.keys()], [1]);
  assert.equal(after.cursor, undefined);
  assert.equal(after.state.observer_lifetime.instance_id, "b");
});

test("summary high-water marks do not advance an undelivered cursor", () => {
  const before = reconcileLive(empty(), state("a", 1, [1]));
  const summary = { ...state("a", 2, [1, 2, 3]), normalized_events: null };
  const after = reconcileLive(before, summary);
  assert.equal(after.cursor.after, 1);
  assert.deepEqual([...after.events.keys()], [1]);
});

test("older lifetime-free services still reconcile complete empty and retained snapshots", () => {
  let view = reconcileLive(empty(), state(undefined, 0, [1, 2, 3]));
  view = reconcileLive(view, state(undefined, 0, [3]));
  assert.deepEqual([...view.events.keys()], [3]);
  view = reconcileLive(view, state(undefined, 0, []));
  assert.equal(view.events.size, 0);
  assert.equal(view.cursor, undefined);
});

test("stream frames reject malformed counters, foreign cursors and broken envelopes", () => {
  const snapshot = state("a", 1, [1]);
  const cursor = { instance_id: "a", revision: 1, after: 1 };
  assert.equal(parseStreamMessage({ type: "snapshot", state: snapshot, cursor }).cursor.after, 1);
  for (const invalid of [{ ...cursor, instance_id: "b" }, { ...cursor, revision: 0 }, { ...cursor, after: 2 }, { ...cursor, after: true }])
    assert.equal(parseStreamMessage({ type: "snapshot", state: snapshot, cursor: invalid }), undefined);
  assert.equal(parseStreamMessage({ type: "events", state: snapshot, events: [null] }), undefined);
  assert.equal(parseStreamMessage({ type: "gap", retention: {} }), undefined);
  assert.equal(parseStreamMessage({ type: "heartbeat", cursor: { after: 1 } }), undefined);
  assert.deepEqual(parseStreamMessage({ type: "heartbeat" }), { type: "heartbeat" });
});

test("transport priority depends on independent capabilities and advertised paths", () => {
  const contract = { capabilities: { websocket: { state: "available" }, sse: { state: "partial" } },
    api_paths: { websocket: "/custom/ws", event_stream: "/custom/sse" } };
  assert.deepEqual(streamTransports(contract), ["websocket", "sse"]);
  assert.deepEqual(streamTransports({ ...contract, capabilities: { ...contract.capabilities, websocket: { state: "unsupported" } } }), ["sse"]);
  assert.deepEqual(streamTransports({ ...contract, api_paths: {} }), []);
});
