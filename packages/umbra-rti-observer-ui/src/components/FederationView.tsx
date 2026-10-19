import { useState } from "react";
import { Area, AreaChart, Bar, BarChart, CartesianGrid, Cell, Legend, Pie, PieChart, ResponsiveContainer, Tooltip, XAxis, YAxis } from "recharts";
import type { AnalyticsResponse, StateResponse } from "../api/types";
import { formatTime } from "../app/observations";
import { ChartPanel, chartColors, chartTick, chartTooltip } from "./ChartPanel";

export function FederationView({ analytics, state, onFederate }: {
  analytics: AnalyticsResponse; state: StateResponse; onFederate: (name: string) => void;
}) {
  const [distribution, setDistribution] = useState<"event_types" | "sources" | "families">("event_types");
  const buckets = analytics.activity.map((bucket) => ({ ...bucket,
    other: bucket.events - bucket.object_updates - bucket.interactions - bucket.callbacks - bucket.errors,
  }));
  const breakdown = Object.entries(analytics[distribution]).sort(([, a], [, b]) => b - a);
  const counts = breakdown.slice(0, 7).map(([name, value]) => ({ name, value }));
  if (breakdown.length > 7) counts.push({ name: "Other", value: breakdown.slice(7).reduce((sum, [, count]) => sum + count, 0) });
  const participants = analytics.participants.map((row) => ({ ...row,
    other: row.event_count - row.object_updates - row.interaction_count - row.error_count,
  }));

  return <>
    <div className="section-intro"><div><p className="eyebrow">Federation</p><h2>{String(state.context.federation ?? "Connected federation")}</h2><p>Activity visible to this observer. Select a participant to investigate its perspective.</p></div><span className="panel-note">Updated {formatTime(analytics.generated_at)}</span></div>
    <section className="chart-grid">
      <ChartPanel title="Traffic over time" eyebrow="Activity" note={`${analytics.bucket_seconds}s buckets · wall clock`}>
        <ResponsiveContainer width="100%" height="100%"><AreaChart data={buckets} margin={{ left: -20, right: 10 }}>
          <CartesianGrid stroke="#1d3852" strokeDasharray="3 3" />
          <XAxis dataKey="started_at" tickFormatter={formatTime} minTickGap={35} tick={chartTick} /><YAxis allowDecimals={false} tick={chartTick} />
          <Tooltip contentStyle={chartTooltip} labelFormatter={(value) => formatTime(Number(value))} /><Legend />
          <Area type="stepAfter" dataKey="object_updates" name="Object updates" stackId="traffic" stroke={chartColors[0]} fill={chartColors[0]} fillOpacity={0.5} isAnimationActive={false} />
          <Area type="stepAfter" dataKey="interactions" name="Interactions" stackId="traffic" stroke={chartColors[1]} fill={chartColors[1]} fillOpacity={0.5} isAnimationActive={false} />
          <Area type="stepAfter" dataKey="callbacks" name="Callbacks" stackId="traffic" stroke={chartColors[2]} fill={chartColors[2]} fillOpacity={0.5} isAnimationActive={false} />
          <Area type="stepAfter" dataKey="errors" name="Errors" stackId="traffic" stroke={chartColors[4]} fill={chartColors[4]} isAnimationActive={false} />
          <Area type="stepAfter" dataKey="other" name="Other" stackId="traffic" stroke={chartColors[3]} fill={chartColors[3]} fillOpacity={0.5} isAnimationActive={false} />
        </AreaChart></ResponsiveContainer>
      </ChartPanel>
      <ChartPanel title="Participant activity" eyebrow="Attribution" note={`${analytics.unattributed_events} events have no participant`}>
        {participants.length ? <ResponsiveContainer width="100%" height="100%"><BarChart data={participants} layout="vertical" margin={{ left: 20, right: 12 }}>
          <CartesianGrid stroke="#1d3852" strokeDasharray="3 3" /><XAxis type="number" allowDecimals={false} tick={chartTick} /><YAxis dataKey="federate" type="category" width={125} tick={chartTick} />
          <Tooltip contentStyle={chartTooltip} /><Legend />
          <Bar dataKey="object_updates" name="Updates" stackId="participant" fill={chartColors[0]} isAnimationActive={false} />
          <Bar dataKey="interaction_count" name="Interactions" stackId="participant" fill={chartColors[1]} isAnimationActive={false} />
          <Bar dataKey="other" name="Other" stackId="participant" fill={chartColors[2]} isAnimationActive={false} />
          <Bar dataKey="error_count" name="Errors" stackId="participant" fill={chartColors[4]} isAnimationActive={false} />
        </BarChart></ResponsiveContainer> : <div className="empty">No participant identity has been observed.</div>}
      </ChartPanel>
      <article className="panel chart-panel">
        <div className="panel-heading"><div><p className="eyebrow">Composition</p><h2>Observation mix</h2></div><select aria-label="Distribution grouping" value={distribution} onChange={(event) => setDistribution(event.target.value as typeof distribution)}><option value="event_types">Event types</option><option value="sources">Sources</option><option value="families">Families</option></select></div>
        <div className="chart-wrap">{counts.length ? <ResponsiveContainer width="100%" height="100%"><PieChart><Pie data={counts} dataKey="value" nameKey="name" innerRadius={55} outerRadius={85} paddingAngle={2} isAnimationActive={false}>{counts.map((row, index) => <Cell key={row.name} fill={chartColors[index % chartColors.length]} />)}</Pie><Tooltip contentStyle={chartTooltip} /><Legend verticalAlign="bottom" /></PieChart></ResponsiveContainer> : <div className="empty">Waiting for activity in this window.</div>}</div>
      </article>
      <article className="panel"><div className="panel-heading"><div><p className="eyebrow">Explore</p><h2>Observed participants</h2></div><span className="panel-note">{participants.length} in roster</span></div>
        <div className="participant-links">{participants.map((row) => <button key={row.federate} type="button" onClick={() => onFederate(row.federate)}><span><strong>{row.federate}</strong><small>{row.role ?? "Role not reported"} · {row.status}</small></span><span>{row.event_count}<small>events ↗</small></span></button>)}</div>
        {!participants.length && <p className="empty">The adapter has not reported any federates.</p>}
        <p className="footnote">This roster reflects observations, and may cover only part of the federation.</p>
      </article>
    </section>
  </>;
}
