import contract from "../api/workspace-contract.json" with { type: "json" };
import { validateLibrary, validateWorkspace } from "../api/workspace-validator.cjs";
import type { WorkspaceDocument, WorkspaceLibrary, WorkspaceSettings } from "../api/types";

export const STORAGE_KEY = "umbra.observer.workspace-library.v1";
export const WORKSPACE_BYTES = 65536;
export const LIBRARY_BYTES = 524288;

function wellFormed(value: unknown): void {
  if (typeof value === "string") {
    for (let index = 0; index < value.length; index += 1) {
      const unit = value.charCodeAt(index);
      if (unit >= 0xd800 && unit <= 0xdbff) {
        const next = value.charCodeAt(++index);
        if (!(next >= 0xdc00 && next <= 0xdfff)) throw new Error("Workspace text contains an unpaired Unicode surrogate.");
      } else if (unit >= 0xdc00 && unit <= 0xdfff) throw new Error("Workspace text contains an unpaired Unicode surrogate.");
    }
  } else if (Array.isArray(value)) value.forEach(wellFormed);
  else if (value !== null && typeof value === "object") Object.values(value).forEach(wellFormed);
}

function bounded(value: unknown, limit: number): void {
  wellFormed(value);
  if (new TextEncoder().encode(JSON.stringify(value)).length > limit) throw new Error("Workspace configuration exceeds its encoded-byte limit.");
}

function chartTargets(settings: WorkspaceSettings): void {
  bounded(settings, WORKSPACE_BYTES);
  const targets = settings.object_charts.map((row) => JSON.stringify([row.scope, row.object_key]));
  if (targets.length !== new Set(targets).size) throw new Error("Workspace numeric chart targets must be unique.");
  const participants = settings.federate_views.map((row) => row.participant);
  if (participants.length !== new Set(participants).size) throw new Error("Workspace federate perspectives must be unique.");
}

export function parseWorkspace(value: unknown): WorkspaceDocument {
  const copy: unknown = structuredClone(value);
  if (!validateWorkspace(copy)) throw new Error("Invalid workspace version, fields, types or bounds. No settings were applied.");
  chartTargets(copy.settings); bounded(copy, WORKSPACE_BYTES);
  return copy;
}

export function parseLibrary(value: unknown): WorkspaceLibrary {
  const copy: unknown = structuredClone(value);
  if (!validateLibrary(copy)) throw new Error("Stored workspace library has invalid fields, version or bounds.");
  chartTargets(copy.current);
  copy.saved.forEach((row) => { chartTargets(row.settings); bounded(row, WORKSPACE_BYTES); });
  if (new Set(copy.saved.map((row) => row.name)).size !== copy.saved.length) throw new Error("Saved workspace names must be unique.");
  bounded(copy, LIBRARY_BYTES);
  return copy;
}

export function decodeWorkspace(raw: string): WorkspaceDocument {
  if (new TextEncoder().encode(raw).length > 128 * 1024) throw new Error("Workspace file exceeds 128 KiB.");
  return parseWorkspace(JSON.parse(raw));
}

export function decodeLibrary(raw: string): WorkspaceLibrary {
  if (new TextEncoder().encode(raw).length > LIBRARY_BYTES) throw new Error("Stored workspace library exceeds 512 KiB.");
  return parseLibrary(JSON.parse(raw));
}

export function freshLibrary(): WorkspaceLibrary {
  return parseLibrary(contract.defaults);
}

export function workspaceDocument(name: string, settings: WorkspaceSettings): WorkspaceDocument {
  return parseWorkspace({ workspace_version: "umbra-observer-workspace-v1", name, settings });
}

export function downloadWorkspace(document: WorkspaceDocument): void {
  const safe = parseWorkspace(document);
  const url = URL.createObjectURL(new Blob([JSON.stringify(safe, null, 2)], { type: "application/json" }));
  const anchor = window.document.createElement("a");
  anchor.href = url; anchor.download = "umbra-workspace.json"; anchor.click();
  window.setTimeout(() => URL.revokeObjectURL(url), 1000);
}
