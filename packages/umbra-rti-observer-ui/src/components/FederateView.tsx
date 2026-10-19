import { useMemo } from "react";
import type { AnalyticsResponse, EventEnvelope, StateResponse } from "../api/types";
import { eventFederate, formatTime } from "../app/observations";
import { EmptyPanel } from "./ChartPanel";
import { EventTable } from "./EventTable";
import { MetricCard } from "./MetricCard";
import { TimeWorkspace } from "./TimeWorkspace";

export function FederateView({ state, analytics, events, selected, onSelect }: {
  state: StateResponse; analytics?: AnalyticsResponse; events: EventEnvelope[];
  selected: string; onSelect: (name: string) => void;
}) {
  const participant = analytics?.participants.find((row) => row.federate === selected);
  const roster = state.federate_roster.find((row) => row.federate === selected);
  const scopedEvents = useMemo(() => events.filter((event) => eventFederate(event) === selected
    && (!analytics || (event.observed_at >= analytics.window_start && event.observed_at <= analytics.generated_at))), [events, selected, analytics]);
  const objects = useMemo(() => {
    const capability = state.capabilities.object_observations;
    if (capability?.state !== "available" && capability?.state !== "partial") return undefined;
    return state.inspectors.object_observations?.snapshots.filter((row) => row.federate === selected).map((row) => row.object);
  }, [selected, state.inspectors.object_observations, state.capabilities]);

  if (!state.federate_roster.length) return <EmptyPanel title="No federates observed">Participant identities appear here when the adapter reports them. MOM is optional.</EmptyPanel>;
  return <div className="federate-layout">
    <aside aria-label="Observed federates" className="panel federate-sidebar"><p className="eyebrow">Perspective</p><h2>Federates</h2><div className="participant-links">{state.federate_roster.map((row) => <button type="button" className={row.federate === selected ? "selected" : ""} key={row.federate} onClick={() => onSelect(row.federate)}><span><strong>{row.federate}</strong><small>{row.role ?? "Role not reported"}</small></span><span className="state-pill">{row.status}</span></button>)}</div></aside>
    <div className="federate-content">
      {!roster ? <EmptyPanel title="Choose a federate">Select a participant to see attributed observations.</EmptyPanel> : <>
        <div className="section-intro"><div><p className="eyebrow">Federate workspace</p><h2>{roster.federate}</h2><p>{roster.role ?? "Role not reported"} · {roster.status} · last seen {formatTime(roster.last_seen)}</p></div><span className="panel-note">{roster.source}</span></div>
        <section className="metrics focus-metrics"><MetricCard label="Attributed events" value={participant?.event_count ?? scopedEvents.length} /><MetricCard label="Object updates" value={participant?.object_updates ?? scopedEvents.filter((event) => event.event_type === "object.updated").length} tone="blue" /><MetricCard label="Interactions" value={participant?.interaction_count ?? scopedEvents.filter((event) => event.event_type.startsWith("interaction.")).length} tone="violet" /><MetricCard label="Errors" value={participant?.error_count ?? scopedEvents.filter((event) => event.event_type === "runtime.error").length} tone="red" /></section>
        <article className="panel"><div className="panel-heading"><div><p className="eyebrow">Object observations</p><h2>Objects in this perspective</h2></div><span className="panel-note">Association does not establish ownership or subscription</span></div>
          {objects?.length ? <div className="object-cards">{objects.map((row) => <details key={row.object_key}><summary><strong>{row.object_name}</strong><span>{row.class_name ?? "Class not reported"}</span><span className="state-pill">{row.status}</span></summary><pre>{JSON.stringify(row.attributes, null, 2)}</pre><small>Latest values attributed to {selected} · {formatTime(row.last_seen)} · independent of chart window</small></details>)}</div> : <p className="empty">{objects ? "No attributed object snapshots for this federate." : "Persistent attributed values unavailable on this backend. Global values are not substituted."}</p>}
        </article>
        <TimeWorkspace key={`time:${selected}`} state={state} analytics={analytics} federate={selected} />
        <EventTable key={`events:${selected}`} events={scopedEvents} title="Federate event timeline" />
      </>}
    </div>
  </div>;
}
