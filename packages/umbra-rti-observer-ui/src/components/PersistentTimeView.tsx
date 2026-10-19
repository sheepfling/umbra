import { useEffect, useMemo, useState } from "react";
import { useWorkspaceChoice } from "../workspace/WorkspaceContext";
import { CartesianGrid, Line, LineChart, ResponsiveContainer, Tooltip, XAxis, YAxis } from "recharts";
import type { StateResponse, TimeRequest, TimeScope } from "../api/types";
import { formatTime } from "../app/observations";
import { ChartPanel, chartColors, chartTick, chartTooltip, EmptyPanel } from "./ChartPanel";

function scopeKey(scope: TimeScope): string {
  return JSON.stringify([scope.source, scope.session_id, scope.federation, scope.federate]);
}

function duration(request: TimeRequest): string {
  if (request.status === "pending") return request.pending_seconds == null ? "Duration unknown" : `${request.pending_seconds.toFixed(2)} s pending`;
  return request.latency_seconds == null ? "—" : `${(request.latency_seconds * 1_000).toFixed(2)} ms observed`;
}

function reportedFlag(value: boolean | null | undefined): string {
  return value == null ? "Unknown" : value ? "Enabled" : "Disabled";
}

export function PersistentTimeView({ state, federate }: { state: StateResponse; federate?: string }) {
  const [selected, setSelected] = useWorkspaceChoice(federate === undefined ? "time" : "federate_time");
  const [limit, setLimit] = useState(100);
  const capability = state.capabilities.time_inspection;
  const usable = capability?.state === "available" || capability?.state === "partial";
  const data = usable ? state.inspectors.time : undefined;
  const snapshots = useMemo(() => data?.snapshots.filter((row) => federate === undefined || row.scope.federate === federate)
    .sort((left, right) => Number(right.scope.session_id !== null) - Number(left.scope.session_id !== null)) ?? [], [data, federate]);
  const perspectives = useMemo(() => {
    const scopes = new Map(snapshots.map((row) => [scopeKey(row.scope), row.scope]));
    for (const row of data?.requests ?? []) if (federate === undefined || row.scope.federate === federate) scopes.set(scopeKey(row.scope), row.scope);
    return [...scopes.values()];
  }, [data, snapshots, federate]);
  const selectedKey = selected ?? (perspectives[0] ? scopeKey(perspectives[0]) : "");
  const snapshot = snapshots.find((row) => scopeKey(row.scope) === selectedKey);
  useEffect(() => {
    if (selected === undefined && perspectives[0]) setSelected(scopeKey(perspectives[0]));
  }, [selected, perspectives]);
  const requests = useMemo(() => data?.requests.filter((row) => scopeKey(row.scope) === selectedKey) ?? [], [data, selectedKey]);
  const pending = requests.filter((row) => row.status === "pending");
  const grants = requests.filter((row) => row.status === "granted");
  const points = grants.filter((row) => row.latency_seconds != null).sort((left, right) => (left.outcome_sequence ?? 0) - (right.outcome_sequence ?? 0)).slice(-250).map((row) => ({
    sequence: row.outcome_sequence, milliseconds: row.latency_seconds! * 1_000, request: row.request_id,
  }));
  const table = [...requests].reverse();

  if (!data) return <EmptyPanel title="Persistent time inspection unavailable">{capability?.reason ?? "This backend does not advertise the optional time-inspection model."} Existing timestamp charts and other observation views remain independent.</EmptyPanel>;
  return <section aria-label="Persistent time inspection" className="persistent-time">
    <div className="section-intro"><div><p className="eyebrow">Persistent inspector</p><h2>Time state and requests</h2><p>Last reported state outlives event history. Requests match grants only with explicit IDs in the same source/session/federation/federate scope.</p></div><span className="state-pill">{capability?.state}</span></div>
    {capability?.state === "partial" && <div className="notice-banner">Partial time inspection: {capability.reason ?? (capability.reasons.join("; ") || "Some time observations are unavailable.")}</div>}
    {data.dropped_requests > 0 && <p className="coverage-note" role="status">Request history is incomplete: {data.dropped_requests} records expired from the {data.request_capacity}-record ledger. Pending counts and delay charts cover retained requests only; late grants for evicted requests remain uncorrelated.</p>}
    {(data.dropped_snapshots ?? 0) > 0 && <p className="coverage-note" role="status">Time snapshot coverage is incomplete: {data.dropped_snapshots} latest-state records expired independently of the request ledger. This is not a session end or a new RTI incarnation. Retained requests keep their explicit scope below.</p>}
    <div className="panel time-perspective"><label>Time perspective<select aria-label="Time perspective" value={selectedKey} disabled={!perspectives.length && !selected} onChange={(event) => { setSelected(event.target.value); setLimit(100); }}>
      {!perspectives.some((scope) => scopeKey(scope) === selectedKey) && <option value={selectedKey}>{selected ? "Selected perspective no longer reported" : "No time perspective reported"}</option>}
      {perspectives.map((scope) => <option value={scopeKey(scope)} key={scopeKey(scope)}>{scope.federate ?? "Unattributed"} · {scope.source} · {scope.session_id ?? "session unknown"} · {scope.federation ?? "federation unreported"}{snapshots.some((row) => scopeKey(row.scope) === scopeKey(scope)) ? "" : " · snapshot expired"}</option>)}
    </select></label></div>
    {!snapshot ? <EmptyPanel title="No persistent time observations">{selected ? "The chosen perspective has no retained latest-state snapshot. Retained requests remain inspectable below; global values are not substituted." : "Grants, explicit time-role state or time requests appear when reported. Timestamped objects/interactions alone do not establish granted time."}</EmptyPanel> : <>
      <article className="panel" aria-label="Reported time state">
        <div className="panel-heading"><div><p className="eyebrow">{snapshot.scope.federate ?? "Unattributed"} · {snapshot.scope.source}</p><h2>Last reported time state</h2></div><span className="state-pill">{snapshot.status}</span></div>
        <p className="panel-note">Session {snapshot.scope.session_id ?? "not reported"} · {snapshot.scope.federation ?? "Federation not reported"} · observed {formatTime(snapshot.observed_at)} · sequence #{snapshot.last_sequence}</p>
        <div className="time-cards">
          <div className="context-item"><label>Last granted time</label><strong>{snapshot.last_grant?.logical_time ?? "Unknown"}</strong><small>{snapshot.last_grant ? `Grant #${snapshot.last_grant.sequence} · ${formatTime(snapshot.last_grant.observed_at)}` : "No explicit grant reported"}</small></div>
          <div className="context-item"><label>Last requested target</label><strong>{snapshot.last_request?.logical_time ?? "Unknown"}</strong><small>Not necessarily the last granted time</small></div>
          <div className="context-item"><label>Retained pending requests</label><strong>{pending.length}</strong><small>Not an RTI-wide pending count</small></div>
          <div className="context-item"><label>Matched grants retained</label><strong>{grants.length}</strong><small>Explicit scoped request IDs only</small></div>
        </div>
        <div className="time-facts">
          <div className="context-item"><label>Lookahead</label><span>{snapshot.reported_state?.lookahead ?? "Unknown"}</span></div>
          <div className="context-item"><label>Regulating</label><span>{reportedFlag(snapshot.reported_state?.regulating)}</span></div>
          <div className="context-item"><label>Constrained</label><span>{reportedFlag(snapshot.reported_state?.constrained)}</span></div>
          <div className="context-item"><label>Reported logical time</label><span>{snapshot.reported_state?.logical_time ?? "Unknown"}</span></div>
          <div className="context-item"><label>GALT</label><span>{snapshot.reported_state?.galt ?? "Unknown"}</span></div>
          <div className="context-item"><label>LBTS</label><span>{snapshot.reported_state?.lbts ?? "Unknown"}</span></div>
        </div>
        <p className="footnote">State fields are last reported values, not live queries or inferred bounds. State update: {snapshot.state_sequence == null ? "not reported" : `#${snapshot.state_sequence}`}. This inspector is independent of the analytics chart-time menu.</p>
        {(snapshot.uncorrelated_requests > 0 || snapshot.uncorrelated_grants > 0) && <p className="coverage-note">Uncorrelated observations: {snapshot.uncorrelated_requests} requests · {snapshot.uncorrelated_grants} grants. Missing scope/IDs, duplicate outcomes or expired requests never imply completion or zero delay.</p>}
      </article>
    </>}
    {(snapshot || requests.length > 0) && <>
      {points.length ? <ChartPanel title="Observed request-to-grant delay" eyebrow="Explicitly matched requests" note={`${points.length} retained matches · latest 250 maximum`}>
        <ResponsiveContainer width="100%" height="100%"><LineChart data={points} margin={{ left: 5, right: 15 }}><CartesianGrid stroke="#1d3852" strokeDasharray="3 3" /><XAxis dataKey="sequence" tick={chartTick} /><YAxis tick={chartTick} unit=" ms" /><Tooltip contentStyle={chartTooltip} labelFormatter={(value) => `Grant sequence #${value}`} /><Line dataKey="milliseconds" name="Observer delay" stroke={chartColors[1]} dot={{ r: 3 }} isAnimationActive={false} /></LineChart></ResponsiveContainer>
      </ChartPanel> : <EmptyPanel title="No correlated grant delays">Grant-only RTIs remain useful above. A delay sample requires a retained request and a later matching grant with the same explicit scope and request ID.</EmptyPanel>}
      <article className="panel" aria-label="Time request ledger"><div className="panel-heading"><div><p className="eyebrow">Capacity-bounded · newest requests first</p><h2>Time requests</h2></div><span className="panel-note">{requests.length} retained in this perspective</span></div>
        <p className="footnote">Durations measure observer-ingress monotonic time, not provider execution latency or remote clock differences. Ages refresh with server snapshots; Pause view freezes the display.</p>
        {table.length ? <><div className="table-scroll"><table className="events-table"><thead><tr><th>Request</th><th>Target / requested</th><th>Status</th><th>Granted / outcome</th><th>Observed duration</th></tr></thead><tbody>{table.slice(0, limit).map((row) => <tr key={row.request_id}><td><code>{row.request_id}</code></td><td><strong>{row.requested_time ?? "Unknown"}</strong><small>#{row.request_sequence} · {formatTime(row.requested_at)}</small></td><td><span className="state-pill">{row.status}</span>{row.reason && <small>{row.reason}</small>}</td><td>{row.granted_time ?? (row.status === "granted" ? "Unknown" : "—")}{row.outcome_sequence != null && <small>#{row.outcome_sequence} · {formatTime(row.outcome_at)}</small>}</td><td>{duration(row)}</td></tr>)}</tbody></table></div>{table.length > limit && <button type="button" className="secondary-button load-more" onClick={() => setLimit((count) => count + 100)}>Show 100 more requests</button>}</> : <p className="empty">No correlated request records in this perspective. Missing requests are not proof that the RTI has no pending work.</p>}
      </article>
    </>}
  </section>;
}
