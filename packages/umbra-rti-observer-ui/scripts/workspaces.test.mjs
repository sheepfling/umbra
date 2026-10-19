import test from "node:test";
import assert from "node:assert/strict";
import { decodeLibrary, decodeWorkspace, freshLibrary, parseLibrary, parseWorkspace, workspaceDocument } from "../src/workspace/codec.ts";

test("offline validation fills exactly the named model defaults without changing input", () => {
  const source = { workspace_version: "umbra-observer-workspace-v1", name: "Example", settings: { events: { search: "load" } } };
  const result = parseWorkspace(source);
  assert.equal(result.settings.events.search, "load");
  assert.equal(result.settings.events.family, "");
  assert.equal(result.settings.window_seconds, 300);
  assert.equal(result.settings.mom.counter, null);
  assert.deepEqual(source.settings, { events: { search: "load" } });
  assert.deepEqual(parseLibrary({ library_version: "umbra-observer-workspace-library-v1" }), freshLibrary());
  const library = parseLibrary({ library_version: "umbra-observer-workspace-library-v1", saved: [source] });
  assert.equal(library.saved[0].settings.events.search, "load");
  assert.deepEqual(parseLibrary({ library_version: "umbra-observer-workspace-library-v1", saved: [{ workspace_version: "umbra-observer-workspace-v1", name: "Minimal" }] }).saved[0].settings, freshLibrary().current);
});

test("workspace files reject data/authority fields and invalid primitive types rather than stripping/coercing them", () => {
  const valid = { workspace_version: "umbra-observer-workspace-v1", name: "Example" };
  for (const value of [{ ...valid, state: {} }, { ...valid, api_paths: { state: "https://never-follow.invalid" } },
    { ...valid, settings: { credentials: "not-configuration" } },
    { ...valid, settings: { time: { grants_only: "true" } } }, { ...valid, settings: { window_seconds: "300" } },
    { ...valid, settings: { view: "https://never-follow.invalid" } }, { ...valid, workspace_version: "future" },
    JSON.parse('{"workspace_version":"umbra-observer-workspace-v1","name":"Example","__proto__":{"polluted":true}}')]) {
    assert.throws(() => parseWorkspace(value));
  }
  assert.equal({}.polluted, undefined);
});

test("configuration is bounded and numeric chart targets do not alias different perspectives", () => {
  const base = freshLibrary().current;
  const settings = { ...base, object_charts: [{ scope: null, object_key: "same", attribute: "a" }, { scope: "Alice", object_key: "same", attribute: "b" }] };
  assert.equal(workspaceDocument("Charts", settings).settings.object_charts.length, 2);
  assert.throws(() => workspaceDocument("Bad", { ...settings, object_charts: [...settings.object_charts, { scope: null, object_key: "same", attribute: "c" }] }));
  assert.throws(() => workspaceDocument("Long", { ...base, events: { ...base.events, search: "x".repeat(2049) } }));
  assert.throws(() => decodeWorkspace(" ".repeat(128 * 1024 + 1)));
  assert.throws(() => decodeLibrary(" ".repeat(512 * 1024 + 1)));
});

test("Unicode text stays exact; unpaired surrogates and duplicate saved names are rejected", () => {
  const document = workspaceDocument("🔎 Miguel", freshLibrary().current);
  assert.equal(decodeWorkspace(JSON.stringify(document)).name, "🔎 Miguel");
  assert.throws(() => workspaceDocument("\ud800", freshLibrary().current));
  assert.throws(() => parseLibrary({ ...freshLibrary(), saved: [document, document] }));
});
