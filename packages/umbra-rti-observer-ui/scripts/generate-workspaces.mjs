// Generated validators consume trusted local schemas only; files never supply schemas.
import { readFile, writeFile } from "node:fs/promises";
import { fileURLToPath } from "node:url";
import { createHash } from "node:crypto";
import Ajv2020 from "ajv/dist/2020.js";
import standaloneCode from "ajv/dist/standalone/index.js";

const input = fileURLToPath(new URL("../src/api/workspace-contract.json", import.meta.url));
const output = fileURLToPath(new URL("../src/api/workspace-validator.cjs", import.meta.url));
const content = await readFile(input, "utf8");
const contract = JSON.parse(content);
const ajv = new Ajv2020({ strict: true, ownProperties: true, useDefaults: true, code: { source: true, lines: true } });
ajv.addSchema(contract.workspace_schema);
ajv.addSchema(contract.library_schema);
const hash = createHash("sha256").update(content.replace(/\r\n/g, "\n")).digest("hex");
const code = `// Generated from the named Python workspace models. Do not edit.\n// Contract SHA-256: ${hash}\n`
  + standaloneCode(ajv, { validateWorkspace: contract.workspace_schema.$id, validateLibrary: contract.library_schema.$id }).trimEnd() + "\n";
if (process.argv.includes("--check")) {
  if (await readFile(output, "utf8").catch(() => "") !== code) {
    throw new Error("Workspace validator drift. Review the Python contracts, regenerate their snapshots, then run generate:workspaces.");
  }
  console.log("Offline workspace validators are current.");
} else {
  await writeFile(output, code);
  console.log("Generated offline workspace validators.");
}
