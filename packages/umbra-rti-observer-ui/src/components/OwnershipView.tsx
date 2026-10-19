import { useState } from "react";
import { Bar, BarChart, CartesianGrid, ResponsiveContainer, Tooltip, XAxis, YAxis } from "recharts";
import type { OwnershipReport, StateResponse } from "../api/types";
import { formatTime } from "../app/observations";
import { ChartPanel, EmptyPanel, chartTick, chartTooltip } from "./ChartPanel";
import { MetricCard } from "./MetricCard";

function scopeKey(row: OwnershipReport): string {
  return JSON.stringify([row.source, row.session_id, row.identity_namespace, row.federation_id, row.object_id, row.attribute_key]);
}

function reportText(row: OwnershipReport): string {
  return [row.source, row.session_id, row.identity_namespace, row.federation_id, row.object_id, row.attribute_key, row.reason,
    row.kind === "state" ? row.owner?.display_name : row.requester.display_name,
    row.kind === "state" ? row.owner?.participant_id : row.requester.participant_id,
    row.kind === "state" ? row.owner?.membership_id : row.requester.membership_id,
    row.kind === "state" ? row.transition : row.request_id,
    row.kind === "state" ? row.reported_owner_name : undefined, row.kind === "state" ? row.reported_owner_handle : undefined].join(" ");
}

function ownerLabel(row: Extract<OwnershipReport, { kind: "state" }>): string {
  const identity = row.owner_state === "known" && row.owner ? `${row.owner.display_name ?? row.owner.participant_id} · ${row.owner.participant_id} / ${row.owner.membership_id}`
    : row.owner_state === "rti" ? "RTI — explicitly reported" : row.owner_state === "unowned" ? "Unowned — explicitly reported" : "Unknown — not established";
  const metadata = [row.reported_owner_name, row.reported_owner_handle].filter((value) => value != null).join(" / ");
  return `${identity}${metadata ? ` · Reported owner metadata: ${metadata} (not a join identity)` : ""}`;
}

function Provenance({ row }: { row: OwnershipReport }) {
  return <><strong>{row.source}</strong><small>Session: {row.session_id}</small><small>Namespace: {row.identity_namespace}</small><small>Federation ID: {row.federation_id}</small><code>#{row.sequence}</code><small>{formatTime(row.observed_at)}</small></>;
}

export function OwnershipView({ state }: { state: StateResponse }) {
  const [search, setSearch] = useState("");
  const [scope, setScope] = useState("all");
  const [ownerState, setOwnerState] = useState("all");
  const [requestStatus, setRequestStatus] = useState("all");
  const [stateLimit, setStateLimit] = useState(100);
  const [requestLimit, setRequestLimit] = useState(100);
  const [historyLimit, setHistoryLimit] = useState(100);
  const capability = state.capabilities.ownership_inspection;
  const usable = capability?.state === "available" || capability?.state === "partial";
  const data = usable ? state.inspectors.ownership : undefined;
  const scopes = new Map([...(data?.transitions ?? []), ...(data?.states ?? []), ...(data?.requests.map((row) => row.request) ?? [])].map((row) => [scopeKey(row), row]));
  const matches = (row: OwnershipReport) => (scope === "all" || scopeKey(row) === scope) && reportText(row).toLocaleLowerCase().includes(search.trim().toLocaleLowerCase());
  const states = (data?.states ?? []).filter((row) => matches(row) && (ownerState === "all" || row.owner_state === ownerState)).sort((a, b) => b.sequence - a.sequence);
  const requests = (data?.requests ?? []).filter((row) => matches(row.request) && (requestStatus === "all" || row.status === requestStatus)).sort((a, b) => b.last_sequence - a.last_sequence);
  const history = (data?.transitions ?? []).filter(matches);
  const counts = ["known", "unowned", "rti", "unknown"].map((name) => ({ name, reports: states.filter((row) => row.owner_state === name).length }));
  const resetLimits = () => { setStateLimit(100); setRequestLimit(100); setHistoryLimit(100); };
  const focus = (value: string) => { setScope(value); resetLimits(); };

  if (!data) return <EmptyPanel title="Ownership inspection unavailable">{capability?.reason ?? "This backend does not advertise explicit ownership inspection."} Object values, declarations and session controls remain independent. Publishers, updaters and source names never substitute for an owner report.</EmptyPanel>;
  return <section className="ownership-workspace" aria-label="Ownership workspace">
    <div className="section-intro"><div><p className="eyebrow">Explicit owners · request evidence is separate</p><h2>Attribute ownership</h2><p>Inspect last-reported owner state and acquisition/divestiture evidence. A completed request alone does not establish ownership.</p></div><span className="state-pill">{capability?.state}</span></div>
    {capability?.state === "partial" && <p className="coverage-note">Partial ownership coverage: {capability.reason ?? (capability.reasons.join("; ") || "Some owners or service outcomes are unreported.")}</p>}
    {(data.dropped_states > 0 || data.dropped_requests > 0 || data.dropped_transitions > 0) && <p className="coverage-note" role="status">Ownership evidence is incomplete: {data.dropped_states} states, {data.dropped_requests} requests and {data.dropped_transitions} transitions expired. Missing records do not establish unowned attributes or completed requests.</p>}
    {data.unmatched_outcomes > 0 && <p className="coverage-note">{data.unmatched_outcomes} outcome reports had no retained pending ticket with an exact matching scope. They remain visible in retained history, without changing owners or inventing completions.</p>}
    <div className="panel object-controls ownership-controls">
      <label>Search ownership evidence<input aria-label="Search ownership evidence" value={search} placeholder="Object, attribute, participant, request or source" onChange={(event) => { setSearch(event.target.value); resetLimits(); }} /></label>
      <label>Reported attribute scope<select aria-label="Ownership scope" value={scope} onChange={(event) => focus(event.target.value)}><option value="all">All reported scopes — separate records</option>{scope !== "all" && !scopes.has(scope) && <option value={scope}>Selected scope no longer reported</option>}{[...scopes].map(([key, row]) => <option value={key} key={key}>{row.object_id} / {row.attribute_key} · {row.source} / {row.session_id} / {row.identity_namespace} / {row.federation_id}</option>)}</select></label>
      <label>Owner state<select aria-label="Ownership state filter" value={ownerState} onChange={(event) => { setOwnerState(event.target.value); setStateLimit(100); }}><option value="all">All reported owner states</option>{["known", "unowned", "rti", "unknown"].map((value) => <option key={value}>{value}</option>)}</select></label>
      <label>Request status<select aria-label="Ownership request status" value={requestStatus} onChange={(event) => { setRequestStatus(event.target.value); setRequestLimit(100); }}><option value="all">All request evidence</option>{["pending", "completed", "failed", "cancelled", "ambiguous"].map((value) => <option key={value}>{value}</option>)}</select></label>
    </div>
    <section className="metrics declaration-metrics" aria-label="Filtered ownership evidence metrics"><MetricCard label="Reported attribute scopes" value={states.length} /><MetricCard label="Pending tickets" value={requests.filter((row) => row.status === "pending").length} tone="amber" /><MetricCard label="Ambiguous tickets" value={requests.filter((row) => row.status === "ambiguous").length} tone="red" /></section>
    <ChartPanel title="Reported attribute scopes by owner state" eyebrow="Latest filtered evidence · not a federation census" note="Separate reporters and sessions remain separate counts">
      {!states.length ? <p className="empty">No explicit owner-state reports match this view. Unknown is not unowned.</p> : <ResponsiveContainer width="100%" height="100%"><BarChart data={counts} margin={{ top: 20, right: 20, left: 0, bottom: 5 }}><CartesianGrid stroke="#23415b" strokeDasharray="3 3" /><XAxis dataKey="name" tick={chartTick} /><YAxis allowDecimals={false} tick={chartTick} /><Tooltip contentStyle={chartTooltip} /><Bar dataKey="reports" name="Reported scopes" fill="#b9a4ff" isAnimationActive={false} /></BarChart></ResponsiveContainer>}
    </ChartPanel>
    <article className="panel" aria-label="Latest ownership states"><div className="panel-heading"><div><p className="eyebrow">Latest explicit reports · independent of event expiry</p><h2>Latest ownership states</h2></div><span className="panel-note">{states.length} matching scopes</span></div>
      {!states.length ? <p className="empty">No explicit ownership state matches these filters.</p> : <><div className="table-scroll"><table className="events-table ownership-table"><thead><tr><th>Object instance / attribute</th><th>Reported owner</th><th>Transition / reason</th><th>Provenance</th></tr></thead><tbody>{states.slice(0, stateLimit).map((row) => <tr key={scopeKey(row)}><td><strong>{row.object_id}</strong><code>{row.attribute_key}</code><button className="secondary-button" type="button" onClick={() => focus(scopeKey(row))}>Follow attribute scope</button></td><td><span className="state-pill">{row.owner_state}</span><small>{ownerLabel(row)}</small>{row.owner?.handle && <small>Handle: {row.owner.handle} (not identity)</small>}</td><td><strong>{row.transition}</strong>{row.reason && <small>{row.reason}</small>}</td><td><Provenance row={row} /></td></tr>)}</tbody></table></div>{states.length > stateLimit && <button className="secondary-button load-more" type="button" onClick={() => setStateLimit((count) => count + 100)}>Show 100 more ownership states</button>}</>}
    </article>
    <article className="panel" aria-label="Ownership request evidence"><div className="panel-heading"><div><p className="eyebrow">Exact scoped tickets · no inferred owner changes</p><h2>Ownership request evidence</h2></div><span className="panel-note">{requests.length} matching requests · oldest tickets expire first</span></div>
      {!requests.length ? <p className="empty">No retained request evidence matches this view. An owner report does not imply a recorded request.</p> : <><div className="table-scroll"><table className="events-table ownership-table"><thead><tr><th>Request / action</th><th>Requester / attribute</th><th>Reported outcome</th><th>Provenance</th></tr></thead><tbody>{requests.slice(0, requestLimit).map((row) => <tr key={row.request.sequence}><td><strong>{row.request.request_id}</strong><code>{row.request.action}</code><small>Request #{row.request.sequence}</small></td><td><strong>{row.request.requester.display_name ?? row.request.requester.participant_id}</strong><code>{row.request.requester.participant_id} / {row.request.requester.membership_id}</code><small>{row.request.object_id} / {row.request.attribute_key}</small></td><td><span className="state-pill">{row.status}</span>{row.outcome && <><small>Outcome #{row.outcome.sequence} · {formatTime(row.outcome.observed_at)}</small>{row.outcome.reason && <small>{row.outcome.reason}</small>}</>}{row.status === "ambiguous" && <small>Repeated ticket identity ({row.duplicate_requests}); completion is not established.</small>}<small>No owner change is implied.</small></td><td><Provenance row={row.request} /></td></tr>)}</tbody></table></div>{requests.length > requestLimit && <button className="secondary-button load-more" type="button" onClick={() => setRequestLimit((count) => count + 100)}>Show 100 more ownership requests</button>}</>}
    </article>
    <article className="panel" aria-label="Ownership report sequence"><div className="panel-heading"><div><p className="eyebrow">Retained arrivals · all owner states and ticket statuses</p><h2>Ownership report sequence</h2></div><span className="panel-note">{history.length} matching reports · owner/request filters affect only their tables</span></div>
      {!history.length ? <p className="empty">No retained ownership reports in this scope.</p> : <><ol className="coordination-timeline">{history.slice(-historyLimit).map((row) => <li key={row.sequence}><div><code>#{row.sequence}</code><small>{formatTime(row.observed_at)}</small></div><div><strong>{row.kind === "state" ? `${row.transition} · ${ownerLabel(row)}` : `${row.action} · ${row.request_id} · ${row.kind === "request" ? "requested" : row.outcome}`}</strong><small>{row.object_id} / {row.attribute_key} · {row.source} / {row.session_id} / {row.identity_namespace} / {row.federation_id}</small>{row.reason && <p>{row.reason}</p>}<details><summary>Ownership report details</summary><pre>{JSON.stringify(row, null, 2)}</pre></details></div></li>)}</ol>{history.length > historyLimit && <button className="secondary-button load-more" type="button" onClick={() => setHistoryLimit((count) => count + 100)}>Show 100 earlier ownership reports</button>}</>}
    </article>
    <p className="coverage-note">Last-reported facts, not live ownership queries. Object IDs identify adapter-supplied instance lifetimes; display names and handles never merge joins. Unknown, unowned and RTI-owned are distinct. A divest/acquire request or successful service outcome never substitutes for an owner report. Pending is last reported evidence; no phases, owners or membership are guessed. No ownership command is advertised. Clear/restart resets all three bounded indexes.</p>
  </section>;
}
