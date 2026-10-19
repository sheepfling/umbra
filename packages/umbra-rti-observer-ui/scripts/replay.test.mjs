import { test } from "node:test";
import assert from "node:assert/strict";
import { advancePosition, frameAt } from "../src/app/replay.ts";
import { applyRecordingEdits, RecordingDecoder } from "../src/app/recordingCodec.ts";

test("replay selects the last state at/before the clock, including tied commits", () => {
  const frames = [0, 2, 2, 7].map((offset_seconds) => ({ offset_seconds }));
  for (const [clock, index] of [[0, 0], [1.999, 0], [2, 2], [6, 2], [7, 3], [100, 3], [-1, 0]]) {
    assert.equal(frameAt(frames, clock), index);
  }
});
test("replay speed is deterministic, clamped and independent of wall-clock time", () => {
  assert.equal(advancePosition(2, 3, 0.5, 20), 3.5);
  assert.equal(advancePosition(2, 3, 4, 20), 14);
  assert.equal(advancePosition(2, 30, 8, 20), 20);
  assert.equal(advancePosition(2, -100, 8, 20), 2);
  assert.equal(advancePosition(0, 100, 1, 0), 0);
});

test("compact edits preserve literal proto keys without modifying prototypes or source edits", () => {
  const edits = [
    { op: "set", path: ["__proto__"], value: { safe: 1 } },
    { op: "set", path: ["__proto__", "safe"], value: 2 },
    { op: "splice", path: ["rows"], index: 0, delete_count: 1, items: [{ v: true }] },
    { op: "set", path: ["rows", 0, "v"], value: 1 },
  ];
  const saved = structuredClone(edits), root = { rows: [0] };
  const result = applyRecordingEdits(root, edits);
  assert.equal(Object.getPrototypeOf(result), Object.prototype);
  assert.equal(Object.prototype.safe, undefined);
  assert.deepEqual(result.__proto__, { safe: 2 });
  assert.deepEqual(result.rows, [{ v: 1 }]);
  assert.deepEqual(edits, saved);
  for (const edit of [{ op: "set", path: ["constructor", "prototype", "polluted"], value: true },
    { op: "delete", path: ["rows", 0] }, { op: "splice", path: ["rows"], index: 5, delete_count: 0, items: [] },
    { op: "set", path: ["rows", true], value: 1 }]) assert.throws(() => applyRecordingEdits(result, [edit]));
});

test("compact decoder expands only current state, resets backward and preserves baseline/edits", () => {
  const baseline = { context: { value: 0 }, live_metrics: { event_count: 1, event_rate_per_second: 2, error_count: 0 } };
  const document = { recording_version: "umbra-observer-recording-v2", codec: "observer-json-edits-v1",
    max_state_bytes: 8388608, max_decoded_state_bytes: 1073741824, decoded_state_bytes: 2000, edit_count: 1,
    captured_bytes: 4000, baseline, frames: [
      { index: 0, offset_seconds: 0, captured_at: 5, change: "start", arrivals: [], edits: [], state_bytes: 1000 },
      { index: 1, offset_seconds: 1, captured_at: 4, change: "metadata", arrivals: [], edits: [{ op: "set", path: ["context", "value"], value: 1 }], state_bytes: 1000 },
    ] };
  const saved = structuredClone(document), decoder = new RecordingDecoder(document);
  decoder.advance(); assert.equal(decoder.frame.state.context.value, 0);
  decoder.advance(); assert.equal(decoder.frame.state.context.value, 1);
  assert.deepEqual(decoder.samples.map((sample) => sample.at), [5000, 4000]);
  decoder.reset(); decoder.advance(); assert.equal(decoder.frame.state.context.value, 0);
  assert.deepEqual(document, saved);
});
