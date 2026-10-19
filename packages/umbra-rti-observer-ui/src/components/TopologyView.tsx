import { useId, useMemo, useState } from "react";
import type { ParticipantIdentity, RelationshipSnapshot, StateResponse } from "../api/types";
import { formatTime } from "../app/observations";
import { EmptyPanel } from "./ChartPanel";
import { MetricCard } from "./MetricCard";

export function topologyNodeKey(row: RelationshipSnapshot, endpoint: ParticipantIdentity): string {
  return JSON.stringify([row.identity_namespace, row.federation_id, endpoint.participant_id, endpoint.membership_id]);
}

function edgeKey(row: RelationshipSnapshot): string {
  return JSON.stringify([row.source, row.session_id, row.identity_namespace, row.federation_id, row.relationship_id]);
}

function identityLabel(endpoint: ParticipantIdentity): string {
  return `${endpoint.display_name ?? endpoint.participant_id} · ${endpoint.participant_id} / ${endpoint.membership_id}`;
}

function Endpoint({ value }: { value: ParticipantIdentity }) {
  return <><strong>{value.display_name ?? "Name unreported"}</strong><code>{value.participant_id}</code><small>Join: {value.membership_id}</small><small>Handle: {value.handle ?? "unreported (not identity)"}</small></>;
}

function RelationshipGraph({ rows, selected, onSelect }: { rows: RelationshipSnapshot[]; selected: string; onSelect: (key: string) => void }) {
  const marker = useId().replace(/:/g, "");
  // A bounded view, not a downsampled claim about the federation. The table
  // retains every filtered record and explicitly reports the drawing limit.
  const drawn = rows.slice(0, 40);
  const nodes = new Map<string, ParticipantIdentity>();
  for (const row of drawn) for (const endpoint of [row.producer, row.recipient]) {
    const key = topologyNodeKey(row, endpoint);
    if (!nodes.has(key)) nodes.set(key, endpoint);
  }
  const positions = new Map([...nodes].sort(([a], [b]) => a.localeCompare(b)).map(([key, endpoint], index, all) => {
    const angle = index * Math.PI * 2 / all.length - Math.PI / 2;
    return [key, { endpoint, x: 450 + 290 * Math.cos(angle), y: 240 + 165 * Math.sin(angle) }] as const;
  }));

  return <article className="panel topology-panel" aria-label="Explicit relationship graph">
    <div className="panel-heading"><div><p className="eyebrow">Directed · adapter supplied</p><h2>Explicit relationship graph</h2></div><span className="panel-note">{drawn.length} of {rows.length} filtered edges · select a node to focus</span></div>
    {rows.length > 40 && <p className="coverage-note">Drawing limited to the newest 40 matching relationships. Narrow the filters or use the complete paged table below; no unshown edge is implied absent.</p>}
    {!drawn.length ? <p className="empty">No explicit relationships match this view. Publications, subscriptions and receive events are never paired into guessed edges.</p> : <div className="topology-scroll"><svg className="topology-graph" viewBox="0 0 900 490" role="group" aria-label="Producer to recipient relationships">
      <defs><marker id={marker} markerWidth="8" markerHeight="8" refX="7" refY="4" orient="auto"><path d="M 0 0 L 8 4 L 0 8 Z" fill="context-stroke" /></marker></defs>
      {drawn.map((row, index) => {
        const a = positions.get(topologyNodeKey(row, row.producer))!;
        const b = positions.get(topologyNodeKey(row, row.recipient))!;
        const dx = b.x - a.x, dy = b.y - a.y;
        const length = Math.hypot(dx, dy);
        const offset = 24 + (index % 4) * 18;
        const path = length < 1 ? `M ${a.x - 18} ${a.y - 20} C ${a.x - 100} ${a.y - 120}, ${a.x + 100} ${a.y - 120}, ${a.x + 18} ${a.y - 20}`
          : `M ${a.x + dx / length * 29} ${a.y + dy / length * 29} Q ${(a.x + b.x) / 2 - dy / length * offset} ${(a.y + b.y) / 2 + dx / length * offset}, ${b.x - dx / length * 32} ${b.y - dy / length * 32}`;
        return <path key={edgeKey(row)} d={path} className={`topology-edge ${row.evidence} ${row.status}`} markerEnd={`url(#${marker})`} data-testid="topology-edge"><title>{row.relationship_id}: {identityLabel(row.producer)} → {identityLabel(row.recipient)} · {row.kind} · {row.evidence} · {row.status} · {row.source}</title></path>;
      })}
      {[...positions].map(([key, point]) => <g key={key} transform={`translate(${point.x} ${point.y})`} role="button" tabIndex={0} aria-label={`Focus ${identityLabel(point.endpoint)}`} aria-pressed={selected === key} className={`topology-node ${selected === key ? "selected" : ""}`} onClick={() => onSelect(selected === key ? "all" : key)} onKeyDown={(event) => { if (event.key === "Enter" || event.key === " ") { event.preventDefault(); onSelect(selected === key ? "all" : key); } }}>
        <title>{identityLabel(point.endpoint)} · {key}</title><circle r="25" /><text y="5" textAnchor="middle" aria-hidden="true">{(point.endpoint.display_name ?? point.endpoint.participant_id).slice(0, 2).toUpperCase()}</text>
        <text y="48" textAnchor="middle" className="topology-label">{(point.endpoint.display_name ?? point.endpoint.participant_id).slice(0, 24)}</text><text y="65" textAnchor="middle" className="topology-join">{point.endpoint.membership_id.slice(0, 24)}</text>
      </g>)}
    </svg></div>}
    <p className="coverage-note">Arrow = reported producer → recipient. Teal = declared; blue = observed relationship, not a delivery receipt. Dashed = ended or unknown reported status. Positions have no timing, distance or network meaning.</p>
  </article>;
}

export function TopologyView({ state }: { state: StateResponse }) {
  const [search, setSearch] = useState("");
  const [source, setSource] = useState("all");
  const [kind, setKind] = useState("all");
  const [status, setStatus] = useState("all");
  const [evidence, setEvidence] = useState("all");
  const [focused, setFocused] = useState("all");
  const [limit, setLimit] = useState(100);
  const capability = state.capabilities.relationships;
  const usable = capability?.state === "available" || capability?.state === "partial";
  const data = usable ? state.inspectors.relationships : undefined;
  const rows = useMemo(() => [...(data?.snapshots ?? [])].sort((a, b) => b.sequence - a.sequence), [data]);
  const nodes = useMemo(() => {
    const identities = new Map<string, string>();
    for (const row of rows) for (const endpoint of [row.producer, row.recipient]) {
      const key = topologyNodeKey(row, endpoint);
      if (!identities.has(key)) identities.set(key, `${identityLabel(endpoint)} · ${row.identity_namespace} / ${row.federation_id}`);
    }
    return identities;
  }, [rows]);
  const filtered = useMemo(() => rows.filter((row) => (source === "all" || row.source === source)
    && (kind === "all" || row.kind === kind) && (status === "all" || row.status === status) && (evidence === "all" || row.evidence === evidence)
    && (focused === "all" || [row.producer, row.recipient].some((endpoint) => topologyNodeKey(row, endpoint) === focused))
    && [row.relationship_id, row.source, row.session_id, row.identity_namespace, row.federation_id, row.class_key, row.class_name, row.reason,
      identityLabel(row.producer), identityLabel(row.recipient)].join(" ").toLocaleLowerCase().includes(search.trim().toLocaleLowerCase())), [rows, source, kind, status, evidence, focused, search]);
  const focus = (key: string) => { setFocused(key); setLimit(100); };

  if (!data) return <EmptyPanel title="Topology unavailable">{capability?.reason ?? "This backend does not advertise explicit relationship evidence."} Other observer views remain available. Matching declarations, names or callback activity never establish an edge.</EmptyPanel>;
  return <section className="topology-workspace" aria-label="Federation topology">
    <div className="section-intro"><div><p className="eyebrow">Explicit evidence · scoped join identities</p><h2>Federation topology</h2><p>Explore reported relationships without guessing routing, ownership, delivery or current federation membership.</p></div><span className="state-pill">{capability?.state}</span></div>
    {capability?.state === "partial" && <p className="coverage-note">Partial relationship coverage: {capability.reason ?? (capability.reasons.join("; ") || "Some relationships are not reported.")}</p>}
    {data.dropped_relationships > 0 && <p className="coverage-note" role="status">Relationship coverage is incomplete: {data.dropped_relationships} records expired from the {data.capacity}-record index.</p>}
    <div className="panel object-controls topology-controls">
      <label>Search relationships<input aria-label="Search relationships" placeholder="Participant, class, ID or source" value={search} onChange={(event) => { setSearch(event.target.value); setLimit(100); }} /></label>
      <label>Reporting source<select aria-label="Relationship source" value={source} onChange={(event) => { setSource(event.target.value); setLimit(100); }}><option value="all">All sources — separate edges</option>{source !== "all" && !rows.some((row) => row.source === source) && <option value={source}>Selected source no longer reported</option>}{[...new Set(rows.map((row) => row.source))].map((name) => <option key={name}>{name}</option>)}</select></label>
      <label>Participant join<select aria-label="Topology focus" value={focused} onChange={(event) => focus(event.target.value)}><option value="all">All explicit participants</option>{focused !== "all" && !nodes.has(focused) && <option value={focused}>Selected join no longer reported</option>}{[...nodes].map(([key, label]) => <option key={key} value={key}>{label}</option>)}</select></label>
      <label>Kind<select aria-label="Relationship kind" value={kind} onChange={(event) => { setKind(event.target.value); setLimit(100); }}><option value="all">Objects and interactions</option><option value="object">Objects</option><option value="interaction">Interactions</option></select></label>
      <label>Reported status<select aria-label="Relationship status" value={status} onChange={(event) => { setStatus(event.target.value); setLimit(100); }}><option value="all">All reported statuses</option><option value="active">Active</option><option value="ended">Ended</option><option value="unknown">Unknown</option></select></label>
      <label>Evidence<select aria-label="Relationship evidence" value={evidence} onChange={(event) => { setEvidence(event.target.value); setLimit(100); }}><option value="all">Declared and observed</option><option value="declared">Declared</option><option value="observed">Observed</option></select></label>
    </div>
    <section className="metrics declaration-metrics" aria-label="Filtered topology metrics"><MetricCard label="Explicit relationships" value={filtered.length} /><MetricCard label="Participant joins" value={new Set(filtered.flatMap((row) => [topologyNodeKey(row, row.producer), topologyNodeKey(row, row.recipient)])).size} tone="blue" /><MetricCard label="Unknown status" value={filtered.filter((row) => row.status === "unknown").length} tone="amber" /></section>
    <RelationshipGraph rows={filtered} selected={focused} onSelect={focus} />
    <article className="panel" aria-label="Reported relationships"><div className="panel-heading"><div><p className="eyebrow">Last reported · not live queries</p><h2>Relationship evidence</h2></div><span className="panel-note">{filtered.length} matching records · newest first</span></div>
      {!filtered.length ? <p className="empty">No explicit relationships match this view. Unknown coverage is not proof of no relationships.</p> : <><div className="table-scroll"><table className="events-table relationship-table"><thead><tr><th>Producer</th><th>Recipient</th><th>Relationship</th><th>Evidence / lifecycle</th><th>Provenance</th></tr></thead><tbody>{filtered.slice(0, limit).map((row) => <tr key={edgeKey(row)}>
        <td><Endpoint value={row.producer} /></td><td><Endpoint value={row.recipient} /></td>
        <td><strong>{row.class_name ?? row.class_key ?? "Class unreported"}</strong><code>{row.relationship_id}</code><small>{row.kind}</small>{row.class_name && row.class_key && <small>Class key: {row.class_key}</small>}</td>
        <td><strong>{row.evidence}</strong><span className="state-pill">{row.status}</span><small>Viewpoint: {row.viewpoint}</small>{row.reason && <small>{row.reason}</small>}</td>
        <td><strong>{row.source}</strong><small>Session: {row.session_id}</small><small>Namespace: {row.identity_namespace}</small><small>Federation ID: {row.federation_id}</small><code>#{row.sequence}</code><small>{formatTime(row.observed_at)}</small></td>
      </tr>)}</tbody></table></div>{filtered.length > limit && <button className="secondary-button load-more" type="button" onClick={() => setLimit((count) => count + 100)}>Show 100 more relationships</button>}</>}
    </article>
    <p className="coverage-note">Node identity = namespace + federation ID + participant ID + membership ID. Reused names and handles never merge joins. Separate reporters remain separate edges. Ended/unknown rows and old sessions remain last-reported facts until replaced, cleared or capacity-evicted; missing edges establish nothing.</p>
  </section>;
}
