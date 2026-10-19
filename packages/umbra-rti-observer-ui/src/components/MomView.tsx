import { useEffect, useState } from "react";
import { CartesianGrid, Line, LineChart, ResponsiveContainer, Tooltip, XAxis, YAxis } from "recharts";
import type { MomReport, ParticipantReference, StateResponse } from "../api/types";
import { formatTime } from "../app/observations";
import { ChartPanel, EmptyPanel, chartTick, chartTooltip } from "./ChartPanel";
import { MetricCard } from "./MetricCard";

type Counter = Extract<MomReport, { kind: "counter" }>;
function scopeKey(row: MomReport): string { return JSON.stringify([row.source, row.session_id, row.identity_namespace, row.federation_id]); }
function reportKey(row: MomReport): string { return JSON.stringify([scopeKey(row), row.kind, row.report_id]); }
function counterKey(row: Counter): string {
  return JSON.stringify([scopeKey(row), row.report_id, row.counter_key, row.unit ?? null, row.semantics, row.subject?.identity?.participant_id ?? null, row.subject?.identity?.membership_id ?? null]);
}
function referenceLabel(value: ParticipantReference | null | undefined): string {
  if (!value) return "Subject unreported";
  return [value.display_name ?? value.identity?.display_name ?? value.identity?.participant_id ?? "Name unreported", value.handle ?? value.identity?.handle,
    value.identity ? `${value.identity.participant_id} / ${value.identity.membership_id}` : "join identity unresolved"].filter((item) => item != null).join(" · ");
}
function reportText(row: MomReport): string {
  return [row.report_id, row.source, row.session_id, row.identity_namespace, row.federation_id, row.reason,
    ...(row.kind === "members" ? (row.members ?? []).map((member) => referenceLabel(member.participant)) : [referenceLabel(row.subject), row.kind === "service" ? row.service_name : row.counter_key, row.kind === "service" ? row.error_name : row.value_text])].join(" ").toLocaleLowerCase();
}
function Provenance({ row }: { row: MomReport }) {
  return <><strong>{row.source}</strong><small>{row.report_id} · {row.session_id}</small><small>Namespace: {row.identity_namespace}</small><small>Federation ID: {row.federation_id}</small><code>#{row.sequence}</code><small>{formatTime(row.observed_at)}</small></>;
}

export function MomView({ state }: { state: StateResponse }) {
  const [search, setSearch] = useState("");
  const [scope, setScope] = useState("all");
  const [serviceStatus, setServiceStatus] = useState("all");
  const [counter, setCounter] = useState<string>();
  const [memberLimit, setMemberLimit] = useState(100);
  const [serviceLimit, setServiceLimit] = useState(100);
  const [counterLimit, setCounterLimit] = useState(100);
  const [historyLimit, setHistoryLimit] = useState(100);
  const capability = state.capabilities.mom_inspection;
  const usable = capability?.state === "available" || capability?.state === "partial";
  const data = usable ? state.inspectors.mom : undefined;
  const snapshots = data?.snapshots ?? [];
  const scopes = new Map(snapshots.map((row) => [scopeKey(row), row]));
  const matches = (row: MomReport) => (scope === "all" || scopeKey(row) === scope) && reportText(row).includes(search.trim().toLocaleLowerCase());
  const filtered = snapshots.filter(matches).sort((a, b) => b.sequence - a.sequence);
  const memberReports = filtered.filter((row) => row.kind === "members");
  const members = memberReports.flatMap((row) => (row.members ?? []).map((member, index) => ({ row, member, index })));
  const services = filtered.filter((row) => row.kind === "service" && (serviceStatus === "all" || row.status === serviceStatus));
  const counters = filtered.filter((row) => row.kind === "counter");
  const history = (data?.transitions ?? []).filter(matches);
  const series = new Map([...(data?.transitions ?? []), ...snapshots].filter((row): row is Counter => row.kind === "counter").map((row) => [counterKey(row), row]));
  const sample = series.get(counter ?? "");
  const points = (data?.transitions ?? []).filter((row): row is Counter => row.kind === "counter" && counterKey(row) === counter)
    .map((row) => ({ sequence: row.sequence, value: row.value ?? null }));
  useEffect(() => { if (counter === undefined && series.size) setCounter([...series].find(([, row]) => row.value != null)?.[0] ?? series.keys().next().value); }, [series, counter]);
  const resetLimits = () => { setMemberLimit(100); setServiceLimit(100); setCounterLimit(100); setHistoryLimit(100); };

  if (!data) return <EmptyPanel title="Management inspection unavailable">{capability?.reason ?? "This backend does not advertise typed management reports."} Ordinary roster, time, attributes and owned-session controls remain independent; no MOM data is fabricated from other callbacks.</EmptyPanel>;
  return <section className="mom-workspace" aria-label="Management reports workspace">
    <div className="section-intro"><div><p className="eyebrow">Optional MOM · reported management evidence</p><h2>Management reports</h2><p>Inspect member coverage, service outcomes and provider counters separately from the ordinary observed roster and command authority.</p></div><span className="state-pill">{capability?.state}</span></div>
    {capability?.state === "partial" && <p className="coverage-note">Partial management coverage: {capability.reason ?? (capability.reasons.join("; ") || "Some management reports are not available.")}</p>}
    {(data.dropped_snapshots > 0 || data.dropped_transitions > 0) && <p className="coverage-note" role="status">Management evidence is incomplete: {data.dropped_snapshots} latest reports and {data.dropped_transitions} arrivals expired. Missing data is not a zero counter or proof of absent members/services.</p>}
    <div className="panel object-controls mom-controls"><label>Search reports<input aria-label="Search management reports" value={search} placeholder="Member, handle, service, counter or source" onChange={(event) => { setSearch(event.target.value); resetLimits(); }} /></label><label>Reporting scope<select aria-label="Management report scope" value={scope} onChange={(event) => { setScope(event.target.value); resetLimits(); }}><option value="all">All reporting scopes — separate records</option>{scope !== "all" && !scopes.has(scope) && <option value={scope}>Selected scope no longer reported</option>}{[...scopes].map(([key, row]) => <option value={key} key={key}>{row.source} / {row.session_id} / {row.identity_namespace} / {row.federation_id}</option>)}</select></label><label>Service status<select aria-label="Management service status" value={serviceStatus} onChange={(event) => { setServiceStatus(event.target.value); setServiceLimit(100); }}><option value="all">All reported service statuses</option>{["invoked", "completed", "failed", "unknown"].map((value) => <option key={value}>{value}</option>)}</select></label></div>
    <section className="metrics declaration-metrics" aria-label="Filtered management report metrics"><MetricCard label="Reported member entries" value={members.length} /><MetricCard label="Unresolved member joins" value={members.filter(({ member }) => member.participant.identity == null).length} tone="amber" /><MetricCard label="Service failure reports" value={services.filter((row) => row.kind === "service" && row.status === "failed").length} tone="red" /></section>
    <article className="panel" aria-label="Management member evidence"><div className="panel-heading"><div><p className="eyebrow">Reported coverage · never substituted for ordinary membership</p><h2>Management member evidence</h2></div><span className="panel-note">{memberReports.length} reports · {members.length} entries, not a deduplicated census</span></div>
      {memberReports.map((row) => <p className="coverage-note" key={reportKey(row)}>{row.report_id} · {row.source} / {row.session_id}: provider-reported {row.coverage} coverage; {row.members == null ? "member set unknown" : row.members.length ? `${row.members.length} supplied entries` : "explicit empty member report"}. Not a claim of current federation membership.</p>)}
      {!members.length ? <p className="empty">No supplied member entries in this view. Unknown and explicitly empty reports remain distinct above.</p> : <><div className="table-scroll"><table className="events-table mom-table"><thead><tr><th>Reported participant</th><th>Reported status / coverage</th><th>Provenance</th></tr></thead><tbody>{members.slice(0, memberLimit).map(({ row, member, index }) => <tr key={JSON.stringify([reportKey(row), row.sequence, index])}><td><strong>{referenceLabel(member.participant)}</strong>{!member.participant.identity && <small>Name/handle metadata does not establish a participant join.</small>}</td><td><span className="state-pill">{member.status}</span><small>{row.coverage} reported coverage</small></td><td><Provenance row={row} /></td></tr>)}</tbody></table></div>{members.length > memberLimit && <button className="secondary-button load-more" type="button" onClick={() => setMemberLimit((count) => count + 100)}>Show 100 more management members</button>}</>}
    </article>
    <article className="panel" aria-label="Management service evidence"><div className="panel-heading"><div><p className="eyebrow">Explicit invocation/error reports · not dispatched commands</p><h2>Management service evidence</h2></div><span className="panel-note">{services.length} matching latest reports</span></div>{!services.length ? <p className="empty">No service reports match this view.</p> : <><div className="table-scroll"><table className="events-table mom-table"><thead><tr><th>Service / subject</th><th>Reported outcome</th><th>Provenance</th></tr></thead><tbody>{services.slice(0, serviceLimit).map((row) => row.kind === "service" && <tr key={reportKey(row)}><td><strong>{row.service_name}</strong><small>{referenceLabel(row.subject)}</small><small>Invocation ID: {row.invocation_id ?? "unreported"}</small></td><td><span className="state-pill">{row.status}</span>{row.error_name && <strong>{row.error_name}</strong>}{row.reason && <small>{row.reason}</small>}</td><td><Provenance row={row} /></td></tr>)}</tbody></table></div>{services.length > serviceLimit && <button className="secondary-button load-more" type="button" onClick={() => setServiceLimit((count) => count + 100)}>Show 100 more management services</button>}</>}
    </article>
    <article className="panel" aria-label="Management counter evidence"><div className="panel-heading"><div><p className="eyebrow">Numeric or opaque · never coerced</p><h2>Management counter evidence</h2></div><span className="panel-note">{counters.length} matching latest reports</span></div>{!counters.length ? <p className="empty">No counter reports match this view. No counter is assumed zero.</p> : <><div className="table-scroll"><table className="events-table mom-table"><thead><tr><th>Counter / subject</th><th>Representation / semantics</th><th>Provenance</th></tr></thead><tbody>{counters.slice(0, counterLimit).map((row) => <tr key={reportKey(row)}><td><strong>{row.counter_key}</strong><small>{referenceLabel(row.subject)}</small><button className="secondary-button" type="button" onClick={() => setCounter(counterKey(row))}>Chart this counter series</button></td><td><code>{row.value != null ? row.value : row.value_text}</code><small>{row.value != null ? "Reported numeric value" : "Opaque text — not converted to a number"}</small><small>{row.semantics} · unit {row.unit ?? "unknown"}</small></td><td><Provenance row={row} /></td></tr>)}</tbody></table></div>{counters.length > counterLimit && <button className="secondary-button load-more" type="button" onClick={() => setCounterLimit((count) => count + 100)}>Show 100 more management counters</button>}</>}
    </article>
    <div className="panel region-selection"><label>Counter history<select aria-label="Management counter series" value={counter ?? ""} onChange={(event) => setCounter(event.target.value)}><option value="">Choose an explicit series</option>{counter && !sample && <option value={counter}>Selected series no longer reported</option>}{[...series].map(([key, row]) => <option key={key} value={key}>{row.counter_key} · {row.source} / {row.session_id} / {row.report_id} · {row.unit ?? "unit unknown"} / {row.semantics} · {referenceLabel(row.subject)}</option>)}</select></label><small>Chart selection is independent of table filters; sources, sessions, report IDs, subject joins, units and semantics never mix.</small></div>
    <ChartPanel title="Reported management counter history" eyebrow="Retained numeric samples only" note="No rates, resets or service durations inferred">
      {!points.some((row) => row.value != null) ? <p className="empty">No retained numeric samples in this exact counter series. Opaque text and latest snapshots are not fabricated trend points.</p> : <ResponsiveContainer width="100%" height="100%"><LineChart data={points} margin={{ top: 20, right: 20, left: 0, bottom: 10 }}><CartesianGrid stroke="#23415b" strokeDasharray="3 3" /><XAxis dataKey="sequence" tick={chartTick} /><YAxis tick={chartTick} /><Tooltip contentStyle={chartTooltip} /><Line dataKey="value" name={`Reported value (${sample?.unit ?? "unit unknown"})`} type="linear" stroke="#7dd3fc" dot={{ r: 3 }} isAnimationActive={false} connectNulls={false} /></LineChart></ResponsiveContainer>}
    </ChartPanel>
    <article className="panel" aria-label="Management report sequence"><div className="panel-heading"><div><p className="eyebrow">Retained arrivals · scoped search</p><h2>Management report sequence</h2></div><span className="panel-note">{history.length} matching reports · service-status filter affects only its table</span></div>{!history.length ? <p className="empty">No retained management reports.</p> : <><ol className="coordination-timeline">{history.slice(-historyLimit).map((row) => <li key={row.sequence}><div><code>#{row.sequence}</code><small>{formatTime(row.observed_at)}</small></div><div><strong>{row.kind} · {row.report_id}</strong><small>{row.source} / {row.session_id} / {row.identity_namespace} / {row.federation_id}</small><details><summary>Management report details</summary><pre>{JSON.stringify(row, null, 2)}</pre></details></div></li>)}</ol>{history.length > historyLimit && <button className="secondary-button load-more" type="button" onClick={() => setHistoryLimit((count) => count + 100)}>Show 100 earlier management reports</button>}</>}
    </article>
    <p className="coverage-note">Read-only management evidence, not a live membership query or control grant. A complete roster is the provider's report claim, not an observer census. Unresolved names/handles never merge joins. A service report does not prove this backend sent a command. Counter decreases do not establish reset events; no rates are calculated. Independent bounded latest/history indexes survive event expiry and reset on clear/restart. No MOM command is advertised.</p>
  </section>;
}
