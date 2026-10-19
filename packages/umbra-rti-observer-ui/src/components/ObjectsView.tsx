import { useMemo, useState } from "react";
import type { StateResponse } from "../api/types";
import { EmptyPanel } from "./ChartPanel";
import { ObjectDetails } from "./ObjectDetails";
import { ObjectCoverage } from "./ObjectCoverage";
import { useWorkspaceField } from "../workspace/WorkspaceContext";
import { RetainedSelect } from "../workspace/RetainedSelect";

export function ObjectsView({ state }: { state: StateResponse }) {
  const [scope, setScope] = useWorkspaceField("objects", "scope");
  const [search, setSearch] = useWorkspaceField("objects", "search");
  const [status, setStatus] = useWorkspaceField("objects", "status");
  const [selected, setSelected] = useWorkspaceField("objects", "selected");
  const [limit, setLimit] = useState(100);
  const capability = state.capabilities.object_observations;
  const supported = capability?.state === "available" || capability?.state === "partial";
  const data = supported ? state.inspectors.object_observations : undefined;
  const scopes = useMemo(() => [...new Set([
    ...state.federate_roster.map((row) => row.federate),
    ...(data?.snapshots.flatMap((row) => row.federate === null ? [] : [row.federate]) ?? []),
    // Keep a selected perspective after clear/reset; never silently switch
    // its empty result to another scope's global values.
    ...(scope === null ? [] : [scope]),
  ])].sort(), [state.federate_roster, data, scope]);
  const rows = useMemo(() => data?.snapshots.filter((row) => row.federate === scope
    && (status === "all" || row.object.status === status)
    && [row.object.object_key, row.object.object_name, row.object.class_name,
      row.object.object_handle, row.object.class_handle].join(" ").toLocaleLowerCase().includes(search.trim().toLocaleLowerCase())) ?? [],
  [data, scope, status, search]);
  const current = selected ? rows.find((row) => row.object.object_key === selected) : rows[0];

  if (!data) return <EmptyPanel title="Object perspectives unavailable">{capability?.reason
    ?? "This backend does not advertise persistent object perspectives."} The Inspectors screen still shows any reported global object values. No federate-specific values are inferred.</EmptyPanel>;
  return <section aria-label="Object workspace" className="objects-workspace">
    <div className="section-intro"><div><p className="eyebrow">Inspect · compare · trace</p><h2>Object workspace</h2><p>Snapshots persist beyond event retention. Comparisons and charts use retained attribute observations.</p></div><span className="state-pill">{capability?.state}</span></div>
    <div className="panel object-controls">
      <label>Perspective<select aria-label="Object perspective" value={scope === null ? "global" : `f:${scope}`} onChange={(event) => { setScope(event.target.value === "global" ? null : event.target.value.slice(2)); setLimit(100); }}>
        <option value="global">Global observer — all sources</option>{scopes.map((name) => <option key={name} value={`f:${name}`}>{name} — attributed only</option>)}
      </select></label>
      <label>Find an object<input aria-label="Search objects" placeholder="Object name, class or handle" value={search} onChange={(event) => { setSearch(event.target.value); setLimit(100); }} /></label>
      <label>Status<RetainedSelect aria-label="Object status" value={status} onChange={(event) => { setStatus(event.target.value); setLimit(100); }}><option value="all">All observed</option><option value="active">Active</option><option value="removed">Removed</option></RetainedSelect></label>
    </div>
    <p className="coverage-note">{scope === null ? "Global values combine observations from all sources; they are not a federate's local view." : `Only observations explicitly attributed to ${scope}. Attribution does not establish ownership, subscription or complete RTI visibility.`}</p>
    {capability?.state === "partial" && <p className="coverage-note">Partial object perspectives: {capability.reason ?? "Only some observations are available."}</p>}
    <ObjectCoverage data={data} />
    {data.history_truncated && <p className="coverage-note" role="status">History is incomplete: older events or scoped baselines expired or were cleared. Retained snapshots keep their latest observed values; a previous comparison can predate the retained window.</p>}
    <div className="federate-layout object-layout">
      <aside aria-label="Matching objects" className="panel"><p className="eyebrow">{rows.length} matching objects · {Math.min(limit, rows.length)} shown</p><div className="participant-links">{rows.slice(0, limit).map((row) => <button type="button" key={row.object.object_key} className={current === row ? "selected" : ""} aria-pressed={current === row} onClick={() => setSelected(row.object.object_key)}><span><strong>{row.object.object_name}</strong><small>{row.object.class_name ?? "Class not reported"} · {row.object.object_key}</small></span><span className="state-pill">{row.object.status}</span></button>)}</div>{rows.length > limit && <button className="secondary-button load-more" type="button" onClick={() => setLimit((count) => count + 100)}>Show 100 more objects</button>}{!rows.length && <p className="empty">No objects match this perspective and search.</p>}</aside>
      <div className="federate-content">{current ? <ObjectDetails key={JSON.stringify([scope, current.object.object_key, current.generation])} snapshot={current} changes={data.changes} />
        : <EmptyPanel title="No object selected">Try a different search or perspective. An empty result does not imply that the federation has no objects.</EmptyPanel>}</div>
    </div>
  </section>;
}
