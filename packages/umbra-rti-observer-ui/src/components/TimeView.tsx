import { useMemo } from "react";
import { useScopedWorkspaceField } from "../workspace/WorkspaceContext";
import { CartesianGrid, Legend, Line, LineChart, ResponsiveContainer, Tooltip, XAxis, YAxis } from "recharts";
import type { AnalyticsResponse, CapabilityResponse } from "../api/types";
import { formatTime } from "../app/observations";
import { participantKey, perspectiveDescription } from "../app/participants";
import { ChartPanel, chartColors, chartTick, chartTooltip, EmptyPanel } from "./ChartPanel";

export function TimeView({ analytics, capability, federate, participantLabel }: {
  analytics: AnalyticsResponse; capability: CapabilityResponse | undefined; federate?: string; participantLabel?: string;
}) {
  const [grantsOnly, setGrantsOnly] = useScopedWorkspaceField("time", "grants_only", federate);
  const points = useMemo(() => analytics.logical_times.filter((point) =>
    (federate === undefined || participantKey(point) === federate) && (!grantsOnly || point.observation_kind === "grant"
      || (!point.observation_kind && point.event_type === "callback.invoked" && point.operation === "timeAdvanceGrant"))), [analytics, federate, grantsOnly]);
  const allSeries = [...new Map(points.map((point) => [participantKey(point) ?? "unattributed", point])).entries()];
  const labelCounts = new Map<string | null, number>();
  for (const [, point] of allSeries) labelCounts.set(point.federate, (labelCounts.get(point.federate) ?? 0) + 1);
  const series = allSeries.slice(0, 30).map(([key, point], index) => ({ key, point, index,
    name: `${point.federate ?? "Unattributed"}${labelCounts.get(point.federate)! > 1 ? ` · perspective ${index + 1}` : ""}` }));
  const indices = new Map(series.map((row) => [row.key, row.index]));
  const rows = points.flatMap((point) => {
    const index = indices.get(participantKey(point) ?? "unattributed");
    return index === undefined ? [] : [{ observed_at: point.observed_at, [`p${index}`]: point.logical_time }];
  });
  if (capability?.state !== "available" && capability?.state !== "partial") {
    return <EmptyPanel title="Logical time is not available">{capability?.reason ?? "The adapter has not advertised logical-time observations."} Other observation views remain available.</EmptyPanel>;
  }
  return <>
    <div className="section-intro"><div><p className="eyebrow">Time</p><h2>{federate ? `${participantLabel ?? "Selected participant"} · logical time` : "Logical-time observations"}</h2><p>Compare reported timestamps against observation time. Timestamped data and grants can represent different values.</p></div>
      <label className="check-control"><input type="checkbox" checked={grantsOnly} onChange={(event) => setGrantsOnly(event.target.checked)} />Time advance grants only</label>
    </div>
    {capability.state === "partial" && <div className="notice-banner">Partial time observations: {capability.reason ?? "some time services are not reported"}</div>}
    {allSeries.length > 30 && <p className="coverage-note">Showing {series.length} of {allSeries.length} reported perspectives in this chart. Select an exact perspective in Federates to inspect one independently; omitted perspectives are not merged.</p>}
    {!points.length ? <EmptyPanel title="No matching logical timestamps">The selected window has no {grantsOnly ? "time advance grants" : "logical timestamp observations"}. Try a longer window or clear the grant filter.</EmptyPanel> : <>
      <ChartPanel title={grantsOnly ? "Time advance grants" : "Reported logical timestamps"} eyebrow="Progress" note={`${points.length} observations · lines connect samples`}>
        <ResponsiveContainer width="100%" height="100%"><LineChart data={rows} margin={{ left: 0, right: 12 }}>
          <CartesianGrid stroke="#1d3852" strokeDasharray="3 3" /><XAxis dataKey="observed_at" type="number" domain={["dataMin", "dataMax"]} tickFormatter={formatTime} tick={chartTick} minTickGap={45} /><YAxis domain={["auto", "auto"]} tick={chartTick} />
          <Tooltip contentStyle={chartTooltip} labelFormatter={(value) => formatTime(Number(value))} /><Legend />
          {series.map(({ key, name, index }) => <Line key={key} type="stepAfter" dataKey={`p${index}`} name={name} stroke={chartColors[index % chartColors.length]} connectNulls dot={{ r: 2 }} isAnimationActive={false} />)}
        </LineChart></ResponsiveContainer>
      </ChartPanel>
      <article className="panel"><div className="panel-heading"><div><p className="eyebrow">Latest samples</p><h2>Time by participant</h2></div><span className="panel-note">Latest by event sequence; no time bounds inferred</span></div>
        <div className="time-cards">{series.map(({ key, name, index, point }) => <div className="context-item" key={key}><label style={{ color: chartColors[index % chartColors.length] }}>{name}</label><strong>{point.logical_time}</strong><small>{point.operation ?? point.event_type} · {formatTime(point.observed_at)}</small><small className="participant-provenance">{point.federate == null ? "Participant identity unreported" : perspectiveDescription(point.perspective)}</small></div>)}</div>
      </article>
    </>}
  </>;
}
