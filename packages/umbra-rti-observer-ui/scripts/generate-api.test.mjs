import assert from "node:assert/strict";
import { execFile } from "node:child_process";
import { mkdtemp, mkdir, readFile, writeFile, rm } from "node:fs/promises";
import { tmpdir } from "node:os";
import { basename, dirname, join, resolve } from "node:path";
import test from "node:test";
import { fileURLToPath } from "node:url";
import { promisify } from "node:util";

test("API drift checking detects schema/output changes and never rewrites in check mode", async (t) => {
  const root = await mkdtemp(join(tmpdir(), "umbra-api-drift-"));
  t.after(async () => {
    // Only remove the fresh, owned fixture directory, never a caller's cwd.
    assert.equal(resolve(dirname(root)), resolve(tmpdir()));
    assert.ok(basename(root).startsWith("umbra-api-drift-"));
    await rm(root, { recursive: true, force: true });
  });
  await mkdir(join(root, "src/api"), { recursive: true });
  const schema = join(root, "openapi.json");
  const output = join(root, "src/api/generated.ts");
  const document = { openapi: "3.1.0", info: { title: "Drift fixture", version: "1" }, paths: {},
    components: { schemas: { Sample: { type: "object", required: ["value"], properties: { value: { type: "string" } } } } } };
  await writeFile(schema, JSON.stringify(document));
  const generator = fileURLToPath(new URL("./generate-api.mjs", import.meta.url));
  const run = (...args) => promisify(execFile)(process.execPath, [generator, "--schema", schema, ...args], { cwd: root });
  await run();
  const original = await readFile(output, "utf8");
  await run("--check");
  await writeFile(output, original + "\n// Deliberately stale fixture\n");
  await assert.rejects(run("--check"), { code: 1 });
  assert.equal(await readFile(output, "utf8"), original + "\n// Deliberately stale fixture\n");
  await writeFile(output, original);
  document.components.schemas.Sample.properties.value.type = "number";
  await writeFile(schema, JSON.stringify(document));
  await assert.rejects(run("--check"), { code: 1 });
  assert.equal(await readFile(output, "utf8"), original);
});
