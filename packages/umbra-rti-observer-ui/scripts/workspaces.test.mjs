import test from "node:test";
import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";
import { decodeLibrary, decodeWorkspace, freshLibrary, parseLibrary, parseWorkspace, workspaceDocument } from "../src/workspace/codec.ts";

test("the offline runtime agrees with Python normalization and rejection policy across the shared corpus", async () => {
  const corpus = JSON.parse(await readFile(new URL("./fixtures/workspaces.json", import.meta.url), "utf8"));
  assert.equal(corpus.case_version, "umbra-observer-workspace-cases-v1");
  assert.ok(corpus.cases.length >= 25);
  for (const row of corpus.cases) {
    const original = structuredClone(row.input);
    const parse = row.kind === "workspace" ? parseWorkspace : parseLibrary;
    if (row.valid) assert.deepEqual(parse(row.input), row.normalized, row.name);
    else assert.throws(() => parse(row.input), undefined, row.name);
    assert.deepEqual(row.input, original, `${row.name}: input was mutated`);
  }
});

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

test("complete encoded documents and libraries are bounded independently of individual field lengths", () => {
  const doc = (name, settings) => ({ workspace_version: "umbra-observer-workspace-v1", name, settings });
  assert.throws(() => parseWorkspace(doc("Too large", { object_charts: [0, 1].map((index) => ({ object_key: String(index), attribute: "🔎".repeat(8192) })) })));
  const settings = { object_charts: [{ object_key: "one", attribute: "x".repeat(8192) }], selected_federate: "x".repeat(8192),
    objects: { selected: "x".repeat(8192) }, time: { selected: "x".repeat(8192) }, mom: { counter: "x".repeat(8192) }, fom: { selected: "x".repeat(8192) } };
  const saved = Array.from({ length: 12 }, (_, index) => parseWorkspace(doc(String(index), settings)));
  assert.throws(() => parseLibrary({ library_version: "umbra-observer-workspace-library-v1", saved }));
});

test("federate setting collections are bounded without merging participants or global choices", () => {
  const document = { workspace_version: "umbra-observer-workspace-v1", name: "Perspectives" };
  const result = parseWorkspace({ ...document, settings: { federate_views: [
    { participant: "Alice", time: { selected: "alice-scope" } }, { participant: "Bob", time: { selected: "bob-scope" } }] } });
  assert.equal(result.settings.time.selected, null);
  assert.equal(result.settings.federate_views[0].time.selected, "alice-scope");
  assert.equal(result.settings.federate_views[1].time.selected, "bob-scope");
  assert.throws(() => parseWorkspace({ ...document, settings: { federate_views: [{ participant: "Alice" }, { participant: "Alice" }] } }));
  assert.throws(() => parseWorkspace({ ...document, settings: { federate_views: Array.from({ length: 65 }, (_, index) => ({ participant: String(index) })) } }));
});
