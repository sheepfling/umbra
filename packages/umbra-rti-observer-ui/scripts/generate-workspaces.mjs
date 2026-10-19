// Generated validators consume trusted local schemas only; files never supply schemas.
import { readFile, writeFile } from "node:fs/promises";
import { fileURLToPath } from "node:url";
import { createHash } from "node:crypto";
import { parseArgs } from "node:util";
import Ajv2020 from "ajv/dist/2020.js";
import standaloneCode from "ajv/dist/standalone/index.js";

const { values } = parseArgs({ options: { schema: { type: "string" }, out: { type: "string" }, check: { type: "boolean", default: false } } });
const input = values.schema ?? fileURLToPath(new URL("../src/api/workspace-contract.json", import.meta.url));
const output = values.out ?? fileURLToPath(new URL("../src/api/workspace-validator.cjs", import.meta.url));
const content = await readFile(input, "utf8");
const contract = JSON.parse(content);
const ajv = new Ajv2020({ strict: true, ownProperties: true, useDefaults: true, inlineRefs: false, code: { source: true, lines: true } });
ajv.addSchema(contract.workspace_schema);
ajv.addSchema(contract.library_schema);
const hash = createHash("sha256").update(content.replace(/\r\n/g, "\n")).digest("hex");
const code = `// Generated from the named Python workspace models. Do not edit.\n// Contract SHA-256: ${hash}\n`
  + standaloneCode(ajv, { validateWorkspace: contract.workspace_schema.$id, validateLibrary: contract.library_schema.$id }).trimEnd() + "\n";
if (code.split("\n").length - 1 > 6000) throw new Error("Workspace validators exceed their 6,000-line growth guard; review schema ownership before extending the generated artifact.");
if (values.check) {
  if ((await readFile(output, "utf8").catch(() => "")).replace(/\r\n/g, "\n") !== code) {
    throw new Error("Workspace validator drift. Review the Python contracts, regenerate their snapshots, then run generate:workspaces.");
  }
  console.log("Offline workspace validators are current.");
} else {
  await writeFile(output, code);
  console.log("Generated offline workspace validators.");
}
