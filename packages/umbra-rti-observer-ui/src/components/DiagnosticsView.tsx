import type { EventEnvelope, StateResponse } from "../api/types";
import { CapabilityGrid } from "./CapabilityGrid";
import { EventTable } from "./EventTable";

export function DiagnosticsView({ state, events, known, connection }: {
  state: StateResponse; events: EventEnvelope[]; known: string[]; connection: string;
}) {
  const retention = state.retention;
  return <>
    <div className="section-intro"><div><p className="eyebrow">Diagnostics</p><h2>Observation health</h2><p>Adapter status, feature coverage, and history limits for this session.</p></div><span className="state-pill">{connection}</span></div>
    <section className="chart-grid">
      <article className="panel"><div className="panel-heading"><h2>Event retention</h2><span className="panel-note">{retention.retained_events} / {retention.max_events}</span></div><meter className="retention-meter" min={0} max={retention.max_events} value={retention.retained_events} /><div className="context-grid"><div className="context-item"><label>Dropped</label><span>{retention.dropped_events}</span></div><div className="context-item"><label>Cleared</label><span>{retention.cleared_events}</span></div><div className="context-item"><label>First retained</label><span>{retention.first_retained_sequence}</span></div><div className="context-item"><label>Latest sequence</label><span>{retention.last_sequence}</span></div></div><p className="footnote">Graphs cover retained observations. Idle participants stay in the roster after their events expire.</p></article>
      <article className="panel"><div className="panel-heading"><h2>Attached adapters</h2><span className="panel-note">{state.adapters.length} attached</span></div>{state.adapters.length ? state.adapters.map((adapter) => <div className="adapter-row" key={adapter.name}><strong>{adapter.name}</strong><span className="state-pill">{adapter.status}</span></div>) : <p className="empty">No local adapters attached. Events may arrive through the HTTP API.</p>}<p className="footnote">Observer session status: {state.status}. Adapter attachment does not grant federation management authority.</p></article>
    </section>
    <EventTable events={events.filter((event) => event.event_type === "runtime.error")} title="Runtime errors" />
    <CapabilityGrid capabilities={state.capabilities} known={known} />
  </>;
}
