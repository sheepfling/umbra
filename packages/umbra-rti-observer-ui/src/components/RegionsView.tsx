import { useEffect, useState } from "react";
import type { DimensionRange, RegionReport, StateResponse } from "../api/types";
import { formatTime } from "../app/observations";
import { EmptyPanel } from "./ChartPanel";
import { MetricCard } from "./MetricCard";

function reportId(row: RegionReport): string {
  return row.kind === "region" ? row.region_id : row.kind === "association" ? row.association_id : row.routing_id;
}

function scopeKey(row: RegionReport): string {
  return JSON.stringify([row.source, row.session_id, row.identity_namespace, row.federation_id]);
}

function reportKey(row: RegionReport): string {
  return JSON.stringify([scopeKey(row), row.kind, reportId(row)]);
}

function searchText(row: RegionReport): string {
  return [reportId(row), row.source, row.session_id, row.identity_namespace, row.federation_id, row.reason,
    row.subject?.participant_id, row.subject?.membership_id, row.subject?.display_name,
    ...(row.kind === "region" ? (row.extents ?? []).flatMap((extent) => [extent.extent_id, ...extent.dimensions.flatMap((dimension) => [dimension.dimension_id, dimension.lower_bound, dimension.upper_bound, dimension.dimension_upper_bound])])
      : [row.target_id, ...(row.region_ids ?? []), ...(row.kind === "association" ? row.attribute_keys ?? [] : [row.producer?.display_name, row.producer?.participant_id, row.recipient?.display_name, row.recipient?.participant_id])])].join(" ").toLocaleLowerCase();
}

function normalizedRange(row: DimensionRange): { left: number; width: number } | undefined {
  if (row.dimension_upper_bound == null) return undefined;
  try {
    const lower = BigInt(row.lower_bound), upper = BigInt(row.upper_bound), limit = BigInt(row.dimension_upper_bound);
    if (limit <= 0n || lower < 0n || upper < lower || upper > limit) return undefined;
    // Convert only a bounded 0..10000 ratio. Never round the original bounds
    // through Number, even when they exceed the JavaScript safe-integer range.
    const left = Number(lower * 10000n / limit) / 100;
    const right = Number(upper * 10000n / limit) / 100;
    return { left, width: right - left };
  } catch { return undefined; }
}

function Extents({ row }: { row: Extract<RegionReport, { kind: "region" }> }) {
  if (row.extents == null) return <p className="empty">Extents unknown — not reported. No range or routing coverage is invented.</p>;
  if (!row.extents.length) return <p className="empty">Explicit empty extent report — not proof of absent routing.</p>;
  const ranges = row.extents.flatMap((extent) => extent.dimensions.map((range) => ({ extent: extent.extent_id, range })));
  return <div className="region-ranges" aria-label="Exact region dimension ranges">
    <p className="coverage-note">Exact decimal bounds are preserved below. Proportional plots use only explicitly reported dimension limits and are rounded to 0.01% for display; a narrow range uses a minimum-width marker. No overlap or delivery is inferred.</p>
    {ranges.length > 40 && <p className="coverage-note">Showing 40 of {ranges.length} dimension ranges. The selected report details contain the complete bounded extent report.</p>}
    {!ranges.length && <p>Extents were reported with explicit empty dimension arrays.</p>}
    {ranges.slice(0, 40).map(({ extent, range }) => {
      const plot = normalizedRange(range);
      return <div className="dimension-range" key={JSON.stringify([extent, range.dimension_id])}><strong>{extent} / {range.dimension_id}</strong><code>{range.lower_bound} → {range.upper_bound}</code>{plot ? <div className="range-track" role="img" aria-label={`${range.dimension_id} exact range ${range.lower_bound} to ${range.upper_bound}, dimension limit ${range.dimension_upper_bound}`}><span style={{ left: `min(${plot.left}%, calc(100% - 2px))`, width: `${plot.width}%` }} /></div> : <small>{range.dimension_upper_bound === "0" ? "Zero dimension limit — no proportional plot" : "Dimension limit unknown or invalid — no proportional plot"}</small>}<small>Reported dimension limit: {range.dimension_upper_bound ?? "unknown"}</small></div>;
    })}
  </div>;
}

export function RegionsView({ state }: { state: StateResponse }) {
  const [search, setSearch] = useState("");
  const [kind, setKind] = useState("all");
  const [status, setStatus] = useState("all");
  const [evidence, setEvidence] = useState("all");
  const [selected, setSelected] = useState<string>();
  const [limit, setLimit] = useState(100);
  const [historyLimit, setHistoryLimit] = useState(100);
  const capability = state.capabilities.regions_inspection;
  const usable = capability?.state === "available" || capability?.state === "partial";
  const data = usable ? state.inspectors.regions : undefined;
  const snapshots = data?.snapshots ?? [];
  const filtered = snapshots.filter((row) => (kind === "all" || row.kind === kind) && (status === "all" || row.status === status)
    && (evidence === "all" || row.evidence === evidence) && searchText(row).includes(search.trim().toLocaleLowerCase())).sort((a, b) => b.sequence - a.sequence);
  const current = snapshots.find((row) => reportKey(row) === selected);
  const history = (data?.transitions ?? []).filter((row) => reportKey(row) === selected);
  useEffect(() => { if (selected === undefined && snapshots.length) setSelected(reportKey(snapshots.find((row) => row.kind === "region") ?? snapshots[0]!)); }, [snapshots, selected]);
  const focus = (row: RegionReport) => { setSelected(reportKey(row)); setHistoryLimit(100); };

  if (!data) return <EmptyPanel title="Region inspection unavailable">{capability?.reason ?? "This backend does not advertise typed region/DDM inspection."} Attributes, time and lifecycle views remain independent. Geometric overlap and matching classes never establish a route.</EmptyPanel>;
  return <section className="regions-workspace" aria-label="Regions and DDM workspace">
    <div className="section-intro"><div><p className="eyebrow">Exact extents · declared and observed evidence</p><h2>Regions and DDM</h2><p>Inspect reported region lifetimes, associations and routing observations without guessing overlap, subscription coverage or delivery.</p></div><span className="state-pill">{capability?.state}</span></div>
    {capability?.state === "partial" && <p className="coverage-note">Partial region coverage: {capability.reason ?? (capability.reasons.join("; ") || "Some dimensions, associations or routes are not reported.")}</p>}
    {(data.dropped_snapshots > 0 || data.dropped_transitions > 0) && <p className="coverage-note" role="status">Region evidence is incomplete: {data.dropped_snapshots} snapshots and {data.dropped_transitions} transitions expired. Missing regions or associations are not inferred inactive.</p>}
    <div className="panel object-controls region-controls">
      <label>Search reports<input aria-label="Search region evidence" value={search} placeholder="Region, dimension, exact bound, target or source" onChange={(event) => { setSearch(event.target.value); setLimit(100); }} /></label>
      <label>Record kind<select aria-label="Region record kind" value={kind} onChange={(event) => { setKind(event.target.value); setLimit(100); }}><option value="all">Regions, associations and routing</option><option value="region">Region extents</option><option value="association">Associations</option><option value="routing">Routing observations</option></select></label>
      <label>Reported status<select aria-label="Region reported status" value={status} onChange={(event) => { setStatus(event.target.value); setLimit(100); }}><option value="all">All reported statuses</option>{["active", "deleted", "ended", "unknown"].map((value) => <option key={value}>{value}</option>)}</select></label>
      <label>Evidence<select aria-label="Region evidence kind" value={evidence} onChange={(event) => { setEvidence(event.target.value); setLimit(100); }}><option value="all">Declared, observed and unknown</option>{["declared", "observed", "unknown"].map((value) => <option key={value}>{value}</option>)}</select></label>
    </div>
    <section className="metrics declaration-metrics" aria-label="Filtered region report metrics"><MetricCard label="Region records" value={filtered.filter((row) => row.kind === "region").length} /><MetricCard label="Association records" value={filtered.filter((row) => row.kind === "association").length} tone="violet" /><MetricCard label="Routing reports" value={filtered.filter((row) => row.kind === "routing").length} tone="blue" /></section>
    <article className="panel" aria-label="Reported region evidence"><div className="panel-heading"><div><p className="eyebrow">Latest snapshots · separate reporting scopes</p><h2>Reported region evidence</h2></div><span className="panel-note">{filtered.length} matching reports · newest first</span></div>
      {!filtered.length ? <p className="empty">No explicit reports match this view. Unknown coverage is not absence of DDM activity.</p> : <><div className="table-scroll"><table className="events-table region-table"><thead><tr><th>Identity / kind</th><th>Reported status / evidence</th><th>Target / extent coverage</th><th>Provenance</th></tr></thead><tbody>{filtered.slice(0, limit).map((row) => <tr key={reportKey(row)}><td><strong>{reportId(row)}</strong><code>{row.kind}</code><button className="secondary-button" type="button" onClick={() => focus(row)}>Inspect report</button></td><td><span className="state-pill">{row.status}</span><strong>{row.evidence}</strong>{row.reason && <small>{row.reason}</small>}</td><td>{row.kind === "region" ? row.extents == null ? "Extents unknown" : `${row.extents.length} explicitly reported extents` : <><strong>{row.target_id ?? "Target unreported"}</strong><small>{row.target_kind ?? "Target kind unknown"}{row.kind === "association" ? ` · ${row.direction}` : ""}</small><small>Region IDs: {row.region_ids == null ? "unknown" : row.region_ids.length ? row.region_ids.join(", ") : "explicit empty set"}</small></>}</td><td><strong>{row.source}</strong><small>Session: {row.session_id}</small><small>Namespace: {row.identity_namespace}</small><small>Federation ID: {row.federation_id}</small><code>#{row.sequence}</code><small>{formatTime(row.observed_at)}</small></td></tr>)}</tbody></table></div>{filtered.length > limit && <button className="secondary-button load-more" type="button" onClick={() => setLimit((count) => count + 100)}>Show 100 more region reports</button>}</>}
    </article>
    <article className="panel" aria-label="Selected region report"><div className="panel-heading"><div><p className="eyebrow">Selection stays independent of table filters</p><h2>Selected region report</h2></div></div><label className="region-selection">Report<select aria-label="Selected DDM report" value={selected ?? ""} onChange={(event) => { setSelected(event.target.value); setHistoryLimit(100); }}><option value="">Choose a report</option>{selected && !current && <option value={selected}>Selected report no longer available</option>}{snapshots.map((row) => <option key={reportKey(row)} value={reportKey(row)}>{row.kind} · {reportId(row)} · {row.source} / {row.session_id} / {row.identity_namespace} / {row.federation_id}</option>)}</select></label>
      {!current ? <p className="empty">No latest snapshot for this selection. Expired or cleared reports are not silently replaced by another scope.</p> : <div className="region-detail"><h3>{current.kind} · {reportId(current)}</h3><p><span className="state-pill">{current.status}</span> {current.evidence} evidence · {current.source} / {current.session_id}</p>{current.reason && <p>{current.reason}</p>}
        {current.kind === "region" ? <Extents row={current} /> : <><h3>Explicit region references</h3>{current.region_ids == null ? <p>Referenced regions unknown — not reported</p> : !current.region_ids.length ? <p>Explicit empty region reference set</p> : <ul className="region-references">{current.region_ids.map((id) => {
          const match = snapshots.find((row) => row.kind === "region" && row.region_id === id && scopeKey(row) === scopeKey(current));
          return <li key={id}>{match ? <button className="secondary-button" type="button" onClick={() => focus(match)}>Inspect region {id} · {match.status}</button> : <span>{id} · not reported in this exact scope</span>}</li>;
        })}</ul>}{current.kind === "association" ? <p>Attribute selection: {current.target_kind === "interaction" ? "not applicable" : current.attribute_keys == null ? "unknown" : current.attribute_keys.length ? current.attribute_keys.join(", ") : "explicit empty set"}</p> : <><p>Producer: {current.producer ? `${current.producer.display_name ?? current.producer.participant_id} · ${current.producer.participant_id} / ${current.producer.membership_id}` : "unknown"}</p><p>Recipient: {current.recipient ? `${current.recipient.display_name ?? current.recipient.participant_id} · ${current.recipient.participant_id} / ${current.recipient.membership_id}` : "unknown"}</p><p>Routing observation is not a delivery receipt or a topology edge.</p></>}</>}
        <details><summary>Complete selected DDM report</summary><pre>{JSON.stringify(current, null, 2)}</pre></details>
      </div>}
      <h3>Retained report sequence</h3>{!history.length ? <p>No retained sequence for this selection.</p> : <><ol className="coordination-timeline">{history.slice(-historyLimit).map((row) => <li key={row.sequence}><div><code>#{row.sequence}</code><small>{formatTime(row.observed_at)}</small></div><div><strong>{row.kind} · {reportId(row)} · {row.status}</strong><small>{row.evidence} · {row.source} / {row.session_id} / {row.identity_namespace} / {row.federation_id}</small>{row.reason && <p>{row.reason}</p>}</div></li>)}</ol>{history.length > historyLimit && <button className="secondary-button load-more" type="button" onClick={() => setHistoryLimit((count) => count + 100)}>Show 100 earlier region reports</button>}</>}
    </article>
    <p className="coverage-note">Read-only, last-reported evidence. Namespace/federation/reporter/session and explicit instance IDs keep reused region names separate. Extents and association sets are complete replacements of what the adapter reports; missing data stays unknown. Region deletion never silently changes a separate association/routing report. A declared association, observed route or geometric extent does not establish delivery, complete coverage or current membership. No DDM command is advertised.</p>
  </section>;
}
