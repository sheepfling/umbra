import { writeFile } from "node:fs/promises";
import openapiTS, { astToString } from "openapi-typescript";

const url = process.env.OBSERVER_OPENAPI_URL ?? "http://127.0.0.1:8765/openapi.json";
const response = await fetch(url);
if (!response.ok) {
  throw new Error(`Unable to fetch ${url}: HTTP ${response.status}`);
}

const document = await response.json();
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
await writeFile("src/api/generated.ts", `/**\n * This file is generated from the FastAPI /openapi.json document.\n * Do not edit it directly; run pnpm generate:api.\n */\n\n${output}`, "utf8");
console.log(`Generated src/api/generated.ts from ${url}`);
