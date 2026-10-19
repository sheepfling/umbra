import { useState } from "react";
import type { WorkspaceDocument } from "../api/types";
import { decodeWorkspace, downloadWorkspace, workspaceDocument } from "./codec";
import { RetainedSelect } from "./RetainedSelect";
import { useWorkspaces } from "./WorkspaceContext";

export function WorkspacePanel({ canApply, displayedParticipant }: { canApply: boolean; displayedParticipant?: string }) {
  const owner = useWorkspaces();
  const [name, setName] = useState("Workspace");
  const [selected, setSelected] = useState("");
  const [error, setError] = useState<string>();
  const choice = selected || owner.saved[0]?.name || "";
  const saved = owner.saved.find((row) => row.name === choice);
  const act = (operation: () => void) => {
    try { operation(); setError(undefined); }
    catch (failure) { setError(failure instanceof Error ? failure.message : "Workspace operation failed. Existing settings were kept."); }
  };
  const capture = () => workspaceDocument(name.trim(), { ...owner.settings,
    selected_federate: owner.settings.selected_federate || displayedParticipant || "" });
  const save = (document: WorkspaceDocument) => {
    if (owner.saved.some((row) => row.name === document.name)
      && !window.confirm(`Replace the saved workspace “${document.name}”? No federation data or backend settings will change.`)) return;
    owner.save(document); setSelected(document.name);
  };
  return <section id="saved-workspaces" className="panel workspace-manager" aria-label="Saved workspaces">
    <div className="panel-heading"><div><p className="eyebrow">View configuration only</p><h2>Saved workspaces</h2></div><span className="panel-note">{owner.saved.length}/12 saved in this browser/origin</span></div>
    <p>Remember views, participants, filters, numeric-chart choices and time windows. Files contain selections, not observations, credentials, endpoints, command drafts or authority. Identifying filter/selection text is included; review it before sharing.</p>
    <p className="footnote">Current choices are saved automatically when browser storage permits. Named configurations are portable backups, not recordings. Unsupported feature choices stay dormant. Browser storage is not a durable database or a secret store.</p>
    {!canApply && <div className="notice-banner">Resume live or return from replay before applying/reloading/resetting settings. Saving or downloading configuration does not change the paused/recorded view.</div>}
    {error && <div className="error-banner" role="alert">{error}</div>}
    <div className="workspace-fields">
      <label>Workspace name<input maxLength={128} value={name} onChange={(event) => setName(event.target.value)} /></label>
      <div className="recording-actions"><button type="button" className="primary-button" disabled={owner.blocked || !name.trim()} onClick={() => act(() => save(capture()))}>Save current workspace</button><button type="button" className="secondary-button" disabled={!name.trim()} onClick={() => act(() => downloadWorkspace(capture()))}>Download current workspace</button></div>
      <label>Saved workspace<RetainedSelect aria-label="Saved workspace" value={choice} onChange={(event) => setSelected(event.target.value)}><option value="">Choose a saved workspace</option>{owner.saved.map((row) => <option key={row.name} value={row.name}>{row.name}</option>)}</RetainedSelect></label>
      <div className="recording-actions"><button type="button" disabled={!saved || !canApply} onClick={() => act(() => { if (saved) owner.apply(saved); })}>Apply saved workspace</button><button type="button" disabled={!saved} onClick={() => act(() => { if (saved) downloadWorkspace(saved); })}>Download saved workspace</button><button type="button" disabled={!saved || owner.blocked} onClick={() => act(() => { if (saved && window.confirm(`Delete the saved workspace “${saved.name}” from this browser?`)) owner.remove(saved.name); })}>Delete saved workspace</button></div>
      <label>Import workspace configuration<input type="file" accept="application/json,.json" disabled={owner.blocked} onChange={(event) => {
        const file = event.target.files?.[0]; event.target.value = "";
        if (!file) return;
        if (file.size > 128 * 1024) { setError("Workspace file exceeds 128 KiB."); return; }
        void file.text().then((raw) => act(() => save(decodeWorkspace(raw)))).catch(() => setError("Workspace file could not be read. Existing settings were kept."));
      }} /></label>
      <p className="footnote">Import validates and saves a configuration; it does not apply it automatically. Use Apply explicitly. At most 64 KiB per normalized workspace and 512 KiB for the browser library.</p>
      <div className="recording-actions"><button type="button" disabled={!canApply} onClick={() => act(owner.reload)}>Reload browser copy</button><button type="button" disabled={!canApply} onClick={() => act(() => {
        if (window.confirm("Reset this application's current and named workspace settings in this browser? Only the Umbra workspace storage key is removed. Export any configuration you need first.")) owner.reset();
      })}>Reset browser workspaces</button></div>
    </div>
  </section>;
}
