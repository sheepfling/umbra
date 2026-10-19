import type { WorkspaceDocument, WorkspaceLibrary } from "./types";
export function validateWorkspace(value: unknown): value is WorkspaceDocument;
export function validateLibrary(value: unknown): value is WorkspaceLibrary;
