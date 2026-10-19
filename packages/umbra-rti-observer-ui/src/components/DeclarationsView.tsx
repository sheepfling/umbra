import { useMemo, useState } from "react";
import { useScopedWorkspaceField } from "../workspace/WorkspaceContext";
import { RetainedSelect } from "../workspace/RetainedSelect";
import type { DeclarationSnapshot, StateResponse } from "../api/types";
import { formatTime } from "../app/observations";
import { EmptyPanel } from "./ChartPanel";
import { MetricCard } from "./MetricCard";

function scopeKey(row: DeclarationSnapshot): string {
  return JSON.stringify([row.scope.source, row.scope.session_id, row.scope.federation, row.scope.federate]);
}

export function DeclarationsView({ state, federate }: { state: StateResponse; federate?: string }) {
  const [search, setSearch] = useScopedWorkspaceField("declarations", "search", federate);
  const [perspective, setPerspective] = useScopedWorkspaceField("declarations", "perspective", federate);
  const [kind, setKind] = useScopedWorkspaceField("declarations", "kind", federate);
  const [direction, setDirection] = useScopedWorkspaceField("declarations", "direction", federate);
  const [status, setStatus] = useScopedWorkspaceField("declarations", "status", federate);
  const [limit, setLimit] = useState(100);
  const capability = state.capabilities.declarations;
  const usable = capability?.state === "available" || capability?.state === "partial";
  const data = usable ? state.inspectors.declarations : undefined;
  const snapshots = useMemo(() => data?.snapshots.filter((row) => federate === undefined || row.scope.federate === federate) ?? [], [data, federate]);
  const scopes = useMemo(() => [...new Map(snapshots.map((row) => [scopeKey(row), row.scope])).entries()], [snapshots]);
  const filtered = useMemo(() => snapshots.filter((row) => (perspective === "all" || scopeKey(row) === perspective)
    && (kind === "all" || row.kind === kind) && (direction === "all" || row.direction === direction)
    && (status === "all" || row.status === status)
    && [row.class_key, row.class_name, ...(row.attributes ?? []), row.reason].join(" ").toLocaleLowerCase().includes(search.trim().toLocaleLowerCase()))
    .sort((left, right) => right.sequence - left.sequence), [snapshots, perspective, kind, direction, status, search]);

  if (!data) return <EmptyPanel title="Declaration inspection unavailable">{capability?.reason ?? "This backend does not advertise declaration snapshots."} Object updates and interaction delivery are not substituted for publication or subscription declarations.</EmptyPanel>;
  return <section aria-label="Declaration explorer" className="declarations-workspace">
    <div className="section-intro"><div><p className="eyebrow">Explicit declarations · not routing</p><h2>{federate ? `${federate} · declarations` : "Declaration explorer"}</h2><p>Explore last-reported publication and subscription state. Matching class names do not establish a delivery path or ownership.</p></div><span className="state-pill">{capability?.state}</span></div>
    {capability?.state === "partial" && <p className="coverage-note">Partial declaration coverage: {capability.reason ?? (capability.reasons.join("; ") || "Some declarations or attribute sets are not reported.")}</p>}
    {data.dropped_declarations > 0 && <p className="coverage-note" role="status">Declaration coverage is incomplete: {data.dropped_declarations} snapshots expired from the {data.capacity}-record index. Missing rows do not establish undeclared classes.</p>}
    <div className="panel object-controls declaration-controls">
      <label>Search classes or attributes<input aria-label="Search declarations" value={search} placeholder="Class name, class key, attribute or reason" onChange={(event) => { setSearch(event.target.value); setLimit(100); }} /></label>
      <label>Perspective<RetainedSelect aria-label="Declaration perspective" value={perspective} onChange={(event) => { setPerspective(event.target.value); setLimit(100); }}><option value="all">All reported perspectives — separate rows</option>{perspective !== "all" && !scopes.some(([key]) => key === perspective) && <option value={perspective}>Selected perspective no longer reported</option>}{scopes.map(([key, scope]) => <option value={key} key={key}>{scope.federate ?? "Unattributed"} · {scope.source} · {scope.session_id ?? "session unknown"} · {scope.federation ?? "federation unreported"}</option>)}</RetainedSelect></label>
      <label>Kind<RetainedSelect aria-label="Declaration kind" value={kind} onChange={(event) => { setKind(event.target.value); setLimit(100); }}><option value="all">Objects and interactions</option><option value="object">Objects</option><option value="interaction">Interactions</option></RetainedSelect></label>
      <label>Direction<RetainedSelect aria-label="Declaration direction" value={direction} onChange={(event) => { setDirection(event.target.value); setLimit(100); }}><option value="all">Publications and subscriptions</option><option value="publication">Publications</option><option value="subscription">Subscriptions</option></RetainedSelect></label>
      <label>Reported status<RetainedSelect aria-label="Declaration status" value={status} onChange={(event) => { setStatus(event.target.value); setLimit(100); }}><option value="all">All reported statuses</option><option value="declared">Declared</option><option value="withdrawn">Withdrawn</option><option value="unknown">Unknown</option></RetainedSelect></label>
    </div>
    <section className="metrics declaration-metrics" aria-label="Filtered declaration metrics"><MetricCard label="Matching snapshots" value={filtered.length} /><MetricCard label="Unknown attribute sets" value={filtered.filter((row) => row.attribute_scope === "unknown").length} tone="amber" /><MetricCard label="Withdrawn records" value={filtered.filter((row) => row.status === "withdrawn").length} tone="violet" /></section>
    <article className="panel" aria-label="Reported declarations"><div className="panel-heading"><div><p className="eyebrow">Latest snapshots · newest first</p><h2>Reported declarations</h2></div><span className="panel-note">{filtered.length} matching records · independent of event retention</span></div>
      {!filtered.length ? <p className="empty">{snapshots.length ? "No declarations match these filters." : "No declarations observed in this perspective. An empty result is not evidence of no publications or subscriptions."}</p>
        : <><div className="table-scroll"><table className="events-table declaration-table"><thead><tr><th>Perspective</th><th>Class / kind</th><th>Direction / status</th><th>Attribute scope</th><th>Observed</th></tr></thead><tbody>{filtered.slice(0, limit).map((row) => <tr key={JSON.stringify([scopeKey(row), row.kind, row.direction, row.class_key])}>
          <td><strong>{row.scope.federate ?? "Unattributed"}</strong><small>{row.scope.source}</small><small>Session: {row.scope.session_id ?? "unknown"}</small><small>Federation: {row.scope.federation ?? "unreported"}</small></td>
          <td><strong>{row.class_name ?? row.class_key}</strong><code>{row.class_key}</code><small>{row.kind}</small></td>
          <td><strong>{row.direction}</strong><span className="state-pill">{row.status}</span>{row.reason && <small>{row.reason}</small>}</td>
          <td>{row.attribute_scope === "selected" ? <details><summary>Selected attributes ({row.attributes?.length ?? 0})</summary>{row.attributes?.length ? <ul>{row.attributes.map((attribute) => <li key={attribute}><code>{attribute}</code></li>)}</ul> : <p>Explicit empty selected set</p>}</details>
            : row.attribute_scope === "all" ? "All attributes — explicitly reported" : row.attribute_scope === "not-applicable" ? "Not applicable to interactions" : "Unknown — selected set not reported"}</td>
          <td><code>#{row.sequence}</code><small>{formatTime(row.observed_at)}</small></td>
        </tr>)}</tbody></table></div>{filtered.length > limit && <button className="secondary-button load-more" type="button" onClick={() => setLimit((count) => count + 100)}>Show 100 more declarations</button>}</>}
    </article>
    <p className="coverage-note">No routing edges are inferred. A declaration is not delivery evidence, ownership, a region association or proof of current membership. Snapshots are last reported facts, not live RTI queries; old session scopes remain separate until clear or capacity eviction.</p>
  </section>;
}
