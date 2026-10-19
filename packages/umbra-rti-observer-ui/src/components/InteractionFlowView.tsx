import { useState } from "react";
import { useWorkspaceField } from "../workspace/WorkspaceContext";
import { RetainedSelect } from "../workspace/RetainedSelect";
import { CartesianGrid, Line, LineChart, ResponsiveContainer, Tooltip, XAxis, YAxis } from "recharts";
import type { InteractionFlowRecord, InteractionFlowReport, StateResponse } from "../api/types";
import { formatTime } from "../app/observations";
import { ChartPanel, EmptyPanel } from "./ChartPanel";
import { MetricCard } from "./MetricCard";

function reportText(report: InteractionFlowReport | null | undefined): string {
  return report ? [report.message_id, report.identity_namespace, report.federation_id, report.session_id, report.class_key, report.source,
    report.producer?.participant_id, report.producer?.membership_id, report.producer?.display_name,
    report.recipient?.participant_id, report.recipient?.membership_id, report.recipient?.display_name].join(" ") : "";
}

function Report({ report, count }: { report: InteractionFlowReport | null | undefined; count: number }) {
  if (!report) return <>Not observed — not proof of absence</>;
  return <><strong>{report.source}</strong><code>#{report.sequence}</code><small>{formatTime(report.observed_at)}</small><small>{report.order}</small><small>Logical timestamp: {report.logical_time ?? "unknown"}</small><small>Session: {report.session_id ?? "unreported"}</small><small>{count} observation{count === 1 ? "" : "s"}{count > 1 ? " — original shown; duplicate identity is ambiguous" : ""}</small><details><summary>Report details</summary><pre>{JSON.stringify(report, null, 2)}</pre></details></>;
}

function Endpoints({ row }: { row: InteractionFlowRecord }) {
  const sample = row.send ?? row.receive;
  return <>{([sample?.producer, sample?.recipient] as const).map((endpoint, index) => <div className="flow-endpoint" key={index}><small>{index === 0 ? "Producer" : "Recipient"}</small>{endpoint ? <><strong>{endpoint.display_name ?? "Name unreported"}</strong><code>{endpoint.participant_id} / {endpoint.membership_id}</code></> : <strong>Unknown — not attributed</strong>}</div>)}</>;
}

export function InteractionFlowView({ state }: { state: StateResponse }) {
  const [search, setSearch] = useWorkspaceField("interactions", "search");
  const [status, setStatus] = useWorkspaceField("interactions", "status");
  const [order, setOrder] = useWorkspaceField("interactions", "order");
  const [limit, setLimit] = useState(100);
  const capability = state.capabilities.interaction_flow;
  const usable = capability?.state === "available" || capability?.state === "partial";
  const data = usable ? state.inspectors.interaction_flow : undefined;
  const filtered = (data?.records ?? []).filter((row) => (status === "all" || status === row.status)
    && (order === "all" || [row.send, row.receive].some((report) => report?.order === order))
    && `${reportText(row.send)} ${reportText(row.receive)}`.toLocaleLowerCase().includes(search.trim().toLocaleLowerCase()))
    .sort((a, b) => b.last_sequence - a.last_sequence);
  const delays = filtered.filter((row) => row.status === "paired" && row.arrival_delay_seconds != null)
    .map((row) => ({ sequence: row.receive!.sequence, message: row.send!.message_id, seconds: row.arrival_delay_seconds! }))
    .sort((a, b) => a.sequence - b.sequence);
  const statuses = ["uncorrelated", "send-only", "receive-only", "paired", "ambiguous"] as const;

  if (!data) return <EmptyPanel title="Interaction flow unavailable">{capability?.reason ?? "This backend does not advertise interaction-flow inspection."} Ordinary events and interaction inspectors remain independent. No send/receive path is guessed from matching classes or names.</EmptyPanel>;
  return <section className="flow-workspace" aria-label="Interaction flow workspace">
    <div className="section-intro"><div><p className="eyebrow">Explicit transmission IDs · no guessed delivery</p><h2>Interaction flow</h2><p>Pair reported sends and receives only when transmission ID, namespace, federation and both participant joins agree.</p></div><span className="state-pill">{capability?.state}</span></div>
    {capability?.state === "partial" && <p className="coverage-note">Partial interaction coverage: {capability.reason ?? (capability.reasons.join("; ") || "Some sends, receives or identities are not reported.")}</p>}
    {data.dropped_records > 0 && <p className="coverage-note" role="status">Interaction evidence is incomplete: {data.dropped_records} records expired from the {data.capacity}-record index. Totals below include expired observations since clear; record statuses and charts describe retained evidence only.</p>}
    <section className="metrics declaration-metrics" aria-label="Interaction observation totals"><MetricCard label="Send observations since clear" value={data.send_observations} /><MetricCard label="Receive observations since clear" value={data.receive_observations} tone="blue" /><MetricCard label="Retained records" value={data.records.length} /></section>
    <div className="panel object-controls flow-controls">
      <label>Search interaction evidence<input aria-label="Search interaction evidence" value={search} placeholder="Message, class, endpoint, reporter or scope" onChange={(event) => { setSearch(event.target.value); setLimit(100); }} /></label>
      <label>Pairing status<RetainedSelect aria-label="Interaction pairing status" value={status} onChange={(event) => { setStatus(event.target.value); setLimit(100); }}><option value="all">All evidence statuses</option>{statuses.map((value) => <option key={value}>{value}</option>)}</RetainedSelect></label>
      <label>Reported order<RetainedSelect aria-label="Interaction reported order" value={order} onChange={(event) => { setOrder(event.target.value); setLimit(100); }}><option value="all">All orders</option><option value="receive-order">Receive-order</option><option value="timestamped">Timestamped</option><option value="unknown">Unknown</option></RetainedSelect></label>
    </div>
    <div className="flow-statuses" aria-label="Filtered retained evidence statuses">{statuses.map((value) => <span className={`state-pill flow-status ${value}`} key={value}>{value}: {filtered.filter((row) => row.status === value).length}</span>)}</div>
    <ChartPanel title="Observer send-to-receive arrival interval" eyebrow="Explicit matching observations" note="Filtered unambiguous pairs · monotonic observer clock · not network latency">
      {!delays.length ? <p className="empty">No measured unambiguous send-before-receive intervals. Unknown or out-of-order observations are not zero-delay deliveries.</p> : <div className="chart-wrap"><ResponsiveContainer width="100%" height="100%"><LineChart data={delays} margin={{ top: 20, right: 20, left: 5, bottom: 15 }}><CartesianGrid stroke="#23415b" strokeDasharray="3 3" /><XAxis dataKey="sequence" stroke="#8ea5b8" label={{ value: "Receive observation sequence", position: "insideBottom", offset: -12 }} /><YAxis stroke="#8ea5b8" unit=" s" /><Tooltip contentStyle={{ background: "#0a1a2c", border: "1px solid #23415b", color: "#dbeaf2" }} labelFormatter={(_, payload) => `Message ${payload[0]?.payload?.message ?? "unknown"}`} /><Line dataKey="seconds" name="Observer arrival interval (s)" type="linear" stroke="#5eead4" strokeWidth={2} dot={{ r: 3 }} isAnimationActive={false} connectNulls={false} /></LineChart></ResponsiveContainer></div>}
    </ChartPanel>
    <article className="panel" aria-label="Interaction transmission evidence"><div className="panel-heading"><div><p className="eyebrow">Retained evidence · newest observation first</p><h2>Interaction transmission evidence</h2></div><span className="panel-note">{filtered.length} matching records · no delivery percentage inferred</span></div>
      {!filtered.length ? <p className="empty">No interaction records match this view. Missing observations do not establish dropped messages or disconnected participants.</p> : <><div className="table-scroll"><table className="events-table flow-table"><thead><tr><th>Transmission / scope</th><th>Explicit endpoints</th><th>Send evidence</th><th>Receive evidence</th><th>Pairing / interval</th></tr></thead><tbody>{filtered.slice(0, limit).map((row) => {
        const sample = row.send ?? row.receive;
        return <tr key={row.record_id}><td><strong>{sample?.message_id ?? "Transmission ID unknown"}</strong><code>Record #{row.record_id}</code><small>Class: {sample?.class_key ?? "unreported"}</small><small>Namespace: {sample?.identity_namespace ?? "unknown"}</small><small>Federation ID: {sample?.federation_id ?? "unknown"}</small></td>
          <td><Endpoints row={row} /></td><td><Report report={row.send} count={row.send_count} /></td><td><Report report={row.receive} count={row.receive_count} /></td>
          <td><span className={`state-pill flow-status ${row.status}`}>{row.status}</span><small>{row.arrival_delay_seconds == null ? "Arrival interval unknown" : `${row.arrival_delay_seconds.toPrecision(4)} s observer interval`}</small>{row.status === "paired" && row.arrival_delay_seconds == null && <small>Pairing evidence exists; no usable send-before-receive interval.</small>}{row.status === "ambiguous" && <small>Repeated transmission identity — pairing and interval invalidated.</small>}</td></tr>;
      })}</tbody></table></div>{filtered.length > limit && <button className="secondary-button load-more" type="button" onClick={() => setLimit((count) => count + 100)}>Show 100 more interaction records</button>}</>}
    </article>
    <p className="coverage-note">Paired means two matching adapter observations, not proof of RTI delivery correctness. Reporter sessions and metadata are preserved independently; display names, handles, class names and timestamps never establish a pair. IDs must be unique per transmission within the explicit endpoint joins and namespace. Missing IDs/endpoints remain separate counted observations. Duplicate IDs are ambiguous; receiving first never creates a positive interval. Receive-order and timestamped order are explicitly reported, not inferred from a numeric payload. Clear/restart resets totals and records.</p>
  </section>;
}
