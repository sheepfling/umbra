import { useMemo, useState } from "react";
import { CartesianGrid, Legend, Line, LineChart, ResponsiveContainer, Tooltip, XAxis, YAxis } from "recharts";
import type { AnalyticsResponse, CapabilityResponse } from "../api/types";
import { formatTime } from "../app/observations";
import { ChartPanel, chartColors, chartTick, chartTooltip, EmptyPanel } from "./ChartPanel";

export function TimeView({ analytics, capability, federate }: {
  analytics: AnalyticsResponse; capability: CapabilityResponse | undefined; federate?: string;
}) {
  const [grantsOnly, setGrantsOnly] = useState(false);
  const points = useMemo(() => analytics.logical_times.filter((point) =>
    (!federate || point.federate === federate) && (!grantsOnly || point.observation_kind === "grant"
      || (!point.observation_kind && point.event_type === "callback.invoked" && point.operation === "timeAdvanceGrant"))), [analytics, federate, grantsOnly]);
  const names = [...new Set(points.map((point) => point.federate))];
  const rows = points.map((point) => ({ observed_at: point.observed_at, [`p${names.indexOf(point.federate)}`]: point.logical_time }));
  if (capability?.state !== "available" && capability?.state !== "partial") {
    return <EmptyPanel title="Logical time is not available">{capability?.reason ?? "The adapter has not advertised logical-time observations."} Other observation views remain available.</EmptyPanel>;
  }
  return <>
    <div className="section-intro"><div><p className="eyebrow">Time</p><h2>{federate ? `${federate} · logical time` : "Logical-time observations"}</h2><p>Compare reported timestamps against observation time. Timestamped data and grants can represent different values.</p></div>
      <label className="check-control"><input type="checkbox" checked={grantsOnly} onChange={(event) => setGrantsOnly(event.target.checked)} />Time advance grants only</label>
    </div>
    {capability.state === "partial" && <div className="notice-banner">Partial time observations: {capability.reason ?? "some time services are not reported"}</div>}
    {!points.length ? <EmptyPanel title="No matching logical timestamps">The selected window has no {grantsOnly ? "time advance grants" : "logical timestamp observations"}. Try a longer window or clear the grant filter.</EmptyPanel> : <>
      <ChartPanel title={grantsOnly ? "Time advance grants" : "Reported logical timestamps"} eyebrow="Progress" note={`${points.length} observations · lines connect samples`}>
        <ResponsiveContainer width="100%" height="100%"><LineChart data={rows} margin={{ left: 0, right: 12 }}>
          <CartesianGrid stroke="#1d3852" strokeDasharray="3 3" /><XAxis dataKey="observed_at" type="number" domain={["dataMin", "dataMax"]} tickFormatter={formatTime} tick={chartTick} minTickGap={45} /><YAxis domain={["auto", "auto"]} tick={chartTick} />
          <Tooltip contentStyle={chartTooltip} labelFormatter={(value) => formatTime(Number(value))} /><Legend />
          {names.map((name, index) => <Line key={name ?? "unattributed"} type="stepAfter" dataKey={`p${index}`} name={name ?? "Unattributed"} stroke={chartColors[index % chartColors.length]} connectNulls dot={{ r: 2 }} isAnimationActive={false} />)}
        </LineChart></ResponsiveContainer>
      </ChartPanel>
      <article className="panel"><div className="panel-heading"><div><p className="eyebrow">Latest samples</p><h2>Time by participant</h2></div><span className="panel-note">Latest by event sequence; no time bounds inferred</span></div>
        <div className="time-cards">{names.map((name, index) => { const point = points.filter((item) => item.federate === name).at(-1); return <div className="context-item" key={name ?? "unattributed"}><label style={{ color: chartColors[index % chartColors.length] }}>{name ?? "Unattributed"}</label><strong>{point?.logical_time ?? "—"}</strong><small>{point?.operation ?? point?.event_type} · {formatTime(point?.observed_at)}</small></div>; })}</div>
      </article>
    </>}
  </>;
}
