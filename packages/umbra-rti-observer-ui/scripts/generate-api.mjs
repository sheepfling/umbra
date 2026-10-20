import { readFile, writeFile } from "node:fs/promises";
import { parseArgs } from "node:util";
import openapiTS, { astToString } from "openapi-typescript";

const { values } = parseArgs({ options: { check: { type: "boolean" }, schema: { type: "string" } } });
const url = values.schema ?? process.env.OBSERVER_OPENAPI_URL ?? "http://127.0.0.1:8765/openapi.json";
const document = values.schema ? JSON.parse(await readFile(values.schema, "utf8")) : await (async () => {
  const response = await fetch(url);
  if (!response.ok) throw new Error(`Unable to fetch ${url}: HTTP ${response.status}`);
  return response.json();
})();
// openapi-typescript currently emits a self-referential indexed-access alias
// for Pydantic's named recursive JsonValue schema. That is valid OpenAPI but
// triggers TS2502 in the generated interface. The UI supplies its own bounded
// JsonValue alias in api/types.ts; keep the backend document authoritative for
// validation while reducing only this code-generation input to `unknown`.
for (const name of Object.keys(document.components?.schemas ?? {})) {
  if (name.startsWith("JsonValue-")) document.components.schemas[name] = {};
}
const ast = await openapiTS(document, { alphabetize: true });
const output = astToString(ast, { fileName: "src/api/generated.ts" });
const generated = `/**\n * This file is generated from the FastAPI /openapi.json document.\n * Do not edit it directly; run pnpm generate:api.\n */\n\n${output}`;
const maxLines = 7000;
if (generated.split("\n").length - 1 > maxLines) {
  throw new Error(`Generated API exceeds its ${maxLines}-line growth guard. Review contract ownership/code generation before expanding it; do not hand-split or minify the artifact.`);
}
if (values.check) {
  const existing = await readFile("src/api/generated.ts", "utf8");
  if (existing.replaceAll("\r\n", "\n") !== generated.replaceAll("\r\n", "\n")) {
    console.error("Generated API types have drifted. Review the backend change and regenerate src/api/generated.ts.");
    process.exitCode = 1;
  } else console.log("Generated API types are current.");
} else {
  await writeFile("src/api/generated.ts", generated, "utf8");
  console.log(`Generated src/api/generated.ts from ${url}`);
}
