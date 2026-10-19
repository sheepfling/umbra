import { useMemo, useState } from "react";
import { useWorkspaceField } from "../workspace/WorkspaceContext";
import { RetainedSelect } from "../workspace/RetainedSelect";
import type { CoordinationSnapshot, StateResponse } from "../api/types";
import { formatTime } from "../app/observations";
import { EmptyPanel } from "./ChartPanel";
import { MetricCard } from "./MetricCard";

export function coordinationOperationKey(row: CoordinationSnapshot): string {
  return JSON.stringify([row.source, row.session_id, row.identity_namespace, row.federation_id, row.kind, row.operation_id]);
}

function scopeKey(row: CoordinationSnapshot): string {
  return JSON.stringify([coordinationOperationKey(row), row.scope, row.subject?.participant_id ?? null, row.subject?.membership_id ?? null]);
}

function subjectLabel(row: CoordinationSnapshot): string {
  return row.scope === "federation" ? "Federation-wide report" : row.subject
    ? `${row.subject.display_name ?? row.subject.participant_id} · ${row.subject.participant_id} / ${row.subject.membership_id}`
    : "Local subject unreported";
}

function Participants({ row }: { row: CoordinationSnapshot }) {
  if (row.participants == null) return <small>Participant progress unknown — not reported</small>;
  return <details><summary>Reported participants ({row.participants.length})</summary>{row.participants.length ? <ul className="coordination-participants">{row.participants.map((participant) => <li key={JSON.stringify([participant.identity.participant_id, participant.identity.membership_id])}><strong>{participant.identity.display_name ?? participant.identity.participant_id}</strong> · {participant.status}<small>{participant.identity.participant_id} / {participant.identity.membership_id}</small>{participant.reason && <small>{participant.reason}</small>}</li>)}</ul> : <p>Explicit empty participant report — not a complete membership census.</p>}</details>;
}

export function CoordinationView({ state, workspace }: { state: StateResponse; workspace: "synchronization" | "persistence" }) {
  const [search, setSearch] = useWorkspaceField(workspace, "search");
  const [kind, setKind] = useWorkspaceField(workspace, "kind");
  const [scope, setScope] = useWorkspaceField(workspace, "scope");
  const [phase, setPhase] = useWorkspaceField(workspace, "phase");
  const [selected, setSelected] = useWorkspaceField(workspace, "selected");
  const [limit, setLimit] = useState(100);
  const [historyLimit, setHistoryLimit] = useState(100);
  const capability = state.capabilities[workspace === "persistence" ? "persistence_inspection" : "synchronization"];
  const usable = capability?.state === "available" || capability?.state === "partial";
  const data = usable ? state.inspectors.coordination : undefined;
  const belongs = (row: CoordinationSnapshot) => workspace === "synchronization" ? row.kind === "synchronization" : row.kind !== "synchronization";
  const snapshots = data?.snapshots.filter(belongs) ?? [];
  const transitions = data?.transitions.filter(belongs) ?? [];
  const operations = new Map([...transitions, ...snapshots].map((row) => [coordinationOperationKey(row), row]));
  const filtered = snapshots.filter((row) => (selected === "all" || coordinationOperationKey(row) === selected)
    && (kind === "all" || row.kind === kind) && (scope === "all" || row.scope === scope) && (phase === "all" || row.phase === phase)
    && [row.label, row.operation_id, row.reason, row.source, row.session_id, row.identity_namespace, row.federation_id, subjectLabel(row),
      ...(row.participants ?? []).flatMap((participant) => [participant.identity.display_name, participant.identity.participant_id, participant.reason])].join(" ").toLocaleLowerCase().includes(search.trim().toLocaleLowerCase()))
    .sort((a, b) => b.sequence - a.sequence);
  // A selected operation's sequence includes all its reported scopes/phases,
  // even when the latest-snapshot table is narrowed to one phase.
  const history = transitions.filter((row) => selected === "all" || coordinationOperationKey(row) === selected);
  const distribution = useMemo(() => [...history.reduce((counts, row) => counts.set(row.phase, (counts.get(row.phase) ?? 0) + 1), new Map<string, number>())], [history]);
  const maxCount = Math.max(1, ...distribution.map(([, count]) => count));
  const title = workspace === "synchronization" ? "Synchronization" : "Save and restore";
  const focus = (key: string) => { setSelected(key); setLimit(100); setHistoryLimit(100); };

  if (!data) return <EmptyPanel title={`${title} unavailable`}>{capability?.reason ?? `This backend does not advertise ${workspace} observations.`} Other views remain usable; no phase or completion is inferred from unrelated callbacks.</EmptyPanel>;
  return <section className="coordination-workspace" aria-label={`${title} workspace`}>
    <div className="section-intro"><div><p className="eyebrow">Read-only · explicit coordination reports</p><h2>{title}</h2><p>{workspace === "synchronization" ? "Follow registration, announcement and achievement evidence by operation and reporting scope." : "Follow requested, begun and terminal phases. Local completion never establishes federation-wide completion."}</p></div><span className="state-pill">{capability?.state}</span></div>
    {capability?.state === "partial" && <p className="coverage-note">Partial {workspace} coverage: {capability.reason ?? (capability.reasons.join("; ") || "Some phases or participants are not reported.")}</p>}
    {(data.dropped_operations > 0 || data.dropped_transitions > 0) && <p className="coverage-note" role="status">Coordination coverage is incomplete: {data.dropped_operations} scoped snapshots and {data.dropped_transitions} transitions expired. Shared capacities: {data.capacity} snapshots / {data.history_capacity} transitions.</p>}
    <div className="panel object-controls coordination-controls">
      <label>Search latest reports<input aria-label="Search coordination reports" value={search} placeholder="Label, ID, source, subject or reason" onChange={(event) => { setSearch(event.target.value); setLimit(100); }} /></label>
      <label>Operation<RetainedSelect aria-label="Coordination operation" value={selected} onChange={(event) => focus(event.target.value)}><option value="all">All reported operations</option>{selected !== "all" && !operations.has(selected) && <option value={selected}>Selected operation no longer reported</option>}{[...operations].map(([key, row]) => <option value={key} key={key}>{row.label} · {row.kind} · {row.operation_id} · {row.source} / {row.session_id} / {row.federation_id}</option>)}</RetainedSelect></label>
      {workspace === "persistence" && <label>Kind<RetainedSelect aria-label="Coordination kind" value={kind} onChange={(event) => { setKind(event.target.value); setLimit(100); }}><option value="all">Save and restore</option><option value="save">Save</option><option value="restore">Restore</option></RetainedSelect></label>}
      <label>Reported scope<RetainedSelect aria-label="Coordination scope" value={scope} onChange={(event) => { setScope(event.target.value); setLimit(100); }}><option value="all">Local and federation-wide</option><option value="local">Local only</option><option value="federation">Federation-wide only</option></RetainedSelect></label>
      <label>Latest reported phase<RetainedSelect aria-label="Coordination phase" value={phase} onChange={(event) => { setPhase(event.target.value); setLimit(100); }}><option value="all">All phases</option>{(workspace === "synchronization" ? ["requested", "registered", "registration-failed", "announced", "achieved", "completed", "failed"] : ["requested", "begun", "completed", "failed"]).map((value) => <option key={value}>{value}</option>)}</RetainedSelect></label>
    </div>
    <section className="metrics declaration-metrics" aria-label="Coordination metrics"><MetricCard label="Matching scoped reports" value={filtered.length} /><MetricCard label="Latest failures" value={filtered.filter((row) => row.phase === "failed" || row.phase === "registration-failed").length} tone="red" /><MetricCard label="Retained phase reports" value={history.length} tone="blue" /></section>
    <article className="panel" aria-label="Latest coordination reports"><div className="panel-heading"><div><p className="eyebrow">Separate local / federation scopes</p><h2>Latest coordination reports</h2></div><span className="panel-note">{filtered.length} matching scoped snapshots · newest first</span></div>
      {!filtered.length ? <p className="empty">No explicit reports match this view. Unknown coverage is not success, failure or absence of coordination.</p> : <><div className="table-scroll"><table className="events-table coordination-table"><thead><tr><th>Operation / label</th><th>Reported phase / scope</th><th>Participants / reason</th><th>Provenance</th></tr></thead><tbody>{filtered.slice(0, limit).map((row) => <tr key={scopeKey(row)}>
        <td><strong>{row.label}</strong><code>{row.operation_id}</code><small>{row.kind}</small><button className="secondary-button" type="button" onClick={() => focus(coordinationOperationKey(row))}>Follow operation</button></td>
        <td><strong>{row.phase}</strong><span className={`state-pill coordination-scope ${row.scope}`}>{row.scope}</span><small>{subjectLabel(row)}</small></td>
        <td>{row.reason && <p>{row.reason}</p>}<Participants row={row} /></td>
        <td><strong>{row.source}</strong><small>Session: {row.session_id}</small><small>Namespace: {row.identity_namespace}</small><small>Federation ID: {row.federation_id}</small><code>#{row.sequence}</code><small>{formatTime(row.observed_at)}</small></td>
      </tr>)}</tbody></table></div>{filtered.length > limit && <button className="secondary-button load-more" type="button" onClick={() => setLimit((count) => count + 100)}>Show 100 more coordination reports</button>}</>}
    </article>
    <article className="panel" aria-label="Coordination phase sequence"><div className="panel-heading"><div><p className="eyebrow">Reported arrival sequence · no inferred phases</p><h2>Coordination phase sequence</h2></div><span className="panel-note">{selected === "all" ? "All operations in this workspace" : "Selected operation — all scopes and phases"}</span></div>
      <p className="coverage-note">Latest-report search, kind, scope and phase filters affect the table above. This sequence and distribution show all retained reports for the selected operation (or this workspace), independently of event history and analytics.</p>
      {!history.length ? <p className="empty">No retained phase reports. An expired or unreported transition is not filled in.</p> : <><div className="phase-distribution" aria-label="Retained phase report counts">{distribution.map(([name, count]) => <div key={name}><span>{name}</span><meter min={0} max={maxCount} value={count} aria-label={`${name} reports`} /><strong>{count}</strong></div>)}</div>
        <ol className="coordination-timeline">{history.slice(-historyLimit).map((row) => <li key={row.sequence}><div><code>#{row.sequence}</code><small>{formatTime(row.observed_at)}</small></div><div><strong>{row.label} · {row.phase}</strong><span className={`state-pill coordination-scope ${row.scope}`}>{row.scope}</span><small>{subjectLabel(row)} · {row.kind} · {row.operation_id}</small><small>{row.source} / {row.session_id} / {row.identity_namespace} / {row.federation_id}</small>{row.reason && <p>{row.reason}</p>}<Participants row={row} /></div></li>)}</ol>
        {history.length > historyLimit && <button className="secondary-button load-more" type="button" onClick={() => setHistoryLimit((count) => count + 100)}>Show 100 earlier phase reports</button>}</>}
    </article>
    <p className="coverage-note">Read-only inspection. No register, achieve, save or restore command is advertised. Reports are latest facts by explicit operation ID, reporter session, federation and local subject join — labels are not identity. Participant lists may be partial; individual achievement or completion never implies collective completion. Missing phases are not synthesized; counts describe retained reports, not successful operations.</p>
  </section>;
}
