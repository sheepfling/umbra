import {
  Area,
  AreaChart,
  Bar,
  BarChart,
  CartesianGrid,
  ResponsiveContainer,
  Tooltip,
  XAxis,
  YAxis,
} from "recharts";
import type { EventEnvelope } from "../api/types";
import type { MetricSample } from "../app/useObserver";

interface LiveChartsProps {
  events: EventEnvelope[];
  samples: MetricSample[];
}

export function LiveCharts({ events, samples }: LiveChartsProps) {
  const typeCounts = Object.entries(
    events.reduce<Record<string, number>>((counts, event) => {
      counts[event.event_type] = (counts[event.event_type] ?? 0) + 1;
      return counts;
    }, {}),
  )
    .sort(([, left], [, right]) => right - left)
    .slice(0, 8)
    .map(([name, count]) => ({ name: name.replace(".", "·"), count }));

  const chartSamples = samples.map((sample) => ({
    ...sample,
    time: new Date(sample.at).toLocaleTimeString([], { minute: "2-digit", second: "2-digit" }),
  }));

  return (
    <section className="chart-grid">
      <article className="panel chart-panel">
        <div className="panel-heading">
          <div><p className="eyebrow">Flow</p><h2>Event rate</h2></div>
          <span className="panel-note">last {samples.length} samples</span>
        </div>
        <div className="chart-wrap">
          <ResponsiveContainer width="100%" height="100%">
            <AreaChart data={chartSamples} margin={{ top: 8, right: 8, bottom: 0, left: -20 }}>
              <defs>
                <linearGradient id="rate-fill" x1="0" y1="0" x2="0" y2="1">
                  <stop offset="0%" stopColor="#5eead4" stopOpacity={0.4} />
                  <stop offset="100%" stopColor="#5eead4" stopOpacity={0} />
                </linearGradient>
              </defs>
              <CartesianGrid stroke="#1d3852" strokeDasharray="3 3" />
              <XAxis dataKey="time" tick={{ fill: "#8ea5b8", fontSize: 10 }} />
              <YAxis tick={{ fill: "#8ea5b8", fontSize: 10 }} />
              <Tooltip contentStyle={{ background: "#102238", border: "1px solid #2a4b68" }} />
              <Area type="monotone" dataKey="eventRate" name="events/s" stroke="#5eead4" fill="url(#rate-fill)" />
            </AreaChart>
          </ResponsiveContainer>
        </div>
      </article>
      <article className="panel chart-panel">
        <div className="panel-heading">
          <div><p className="eyebrow">Distribution</p><h2>Observed event types</h2></div>
          <span className="panel-note">retained window</span>
        </div>
        <div className="chart-wrap">
          <ResponsiveContainer width="100%" height="100%">
            <BarChart data={typeCounts} layout="vertical" margin={{ top: 8, right: 8, bottom: 0, left: 20 }}>
              <CartesianGrid stroke="#1d3852" strokeDasharray="3 3" />
              <XAxis type="number" allowDecimals={false} tick={{ fill: "#8ea5b8", fontSize: 10 }} />
              <YAxis dataKey="name" type="category" width={90} tick={{ fill: "#8ea5b8", fontSize: 10 }} />
              <Tooltip contentStyle={{ background: "#102238", border: "1px solid #2a4b68" }} />
              <Bar dataKey="count" name="events" fill="#7dd3fc" radius={[0, 4, 4, 0]} />
            </BarChart>
          </ResponsiveContainer>
        </div>
      </article>
    </section>
  );
}
