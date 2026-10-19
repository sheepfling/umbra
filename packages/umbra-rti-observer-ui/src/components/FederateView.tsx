import { useMemo, useState } from "react";
import type { AnalyticsResponse, EventEnvelope, StateResponse } from "../api/types";
import { eventFederate, formatTime } from "../app/observations";
import { EmptyPanel } from "./ChartPanel";
import { EventTable } from "./EventTable";
import { MetricCard } from "./MetricCard";
import { TimeWorkspace } from "./TimeWorkspace";
import { DeclarationsView } from "./DeclarationsView";
import { ObjectCoverage } from "./ObjectCoverage";

export function FederateView({ state, analytics, events, selected, onSelect }: {
  state: StateResponse; analytics?: AnalyticsResponse; events: EventEnvelope[];
  selected: string; onSelect: (name: string) => void;
}) {
  const [objectPage, setObjectPage] = useState({ federate: selected, limit: 100 });
  const objectLimit = objectPage.federate === selected ? objectPage.limit : 100;
  const participant = analytics?.participants.find((row) => row.federate === selected);
  const roster = state.federate_roster.find((row) => row.federate === selected);
  const names = useMemo(() => [...new Set([...state.federate_roster.map((row) => row.federate),
    ...events.map(eventFederate).filter((name): name is string => Boolean(name)),
    ...(analytics?.participants.map((row) => row.federate) ?? []), ...(selected ? [selected] : [])])].sort(), [state.federate_roster, events, analytics, selected]);
  const scopedEvents = useMemo(() => events.filter((event) => eventFederate(event) === selected
    && (!analytics || (event.observed_at >= analytics.window_start && event.observed_at <= analytics.generated_at))), [events, selected, analytics]);
  const objects = useMemo(() => {
    const capability = state.capabilities.object_observations;
    if (capability?.state !== "available" && capability?.state !== "partial") return undefined;
    return state.inspectors.object_observations?.snapshots.filter((row) => row.federate === selected).map((row) => row.object);
  }, [selected, state.inspectors.object_observations, state.capabilities]);

  if (!names.length) return <EmptyPanel title="No federates observed">Participant identities appear here when the adapter reports them. MOM is optional.</EmptyPanel>;
  return <div className="federate-layout">
    <aside aria-label="Observed federates" className="panel federate-sidebar"><p className="eyebrow">Perspective</p><h2>Federates</h2><div className="participant-links">{names.map((name) => { const row = state.federate_roster.find((entry) => entry.federate === name); return <button type="button" className={name === selected ? "selected" : ""} key={name} onClick={() => onSelect(name)}><span><strong>{name}</strong><small>{row?.role ?? "Role not reported"}</small></span><span className="state-pill">{row?.status ?? "Metadata not retained"}</span></button>; })}</div></aside>
    <div className="federate-content">
      {!selected ? <EmptyPanel title="Choose a federate">Select a participant to see attributed observations.</EmptyPanel> : <>
        <div className="section-intro"><div><p className="eyebrow">Federate workspace</p><h2>{selected}</h2>{roster ? <p>{roster.role ?? "Role not reported"} · {roster.status} · last seen {formatTime(roster.last_seen)}</p> : <p className="coverage-note">Roster metadata is not retained for this selection. This does not establish resignation or disconnection. Explicitly attributed retained events and independent inspectors remain usable.</p>}</div>{roster && <span className="panel-note">{roster.source}</span>}</div>
        <section className="metrics focus-metrics"><MetricCard label="Attributed events" value={participant?.event_count ?? scopedEvents.length} /><MetricCard label="Object updates" value={participant?.object_updates ?? scopedEvents.filter((event) => event.event_type === "object.updated").length} tone="blue" /><MetricCard label="Interactions" value={participant?.interaction_count ?? scopedEvents.filter((event) => event.event_type.startsWith("interaction.")).length} tone="violet" /><MetricCard label="Errors" value={participant?.error_count ?? scopedEvents.filter((event) => event.event_type === "runtime.error").length} tone="red" /></section>
        <article className="panel"><div className="panel-heading"><div><p className="eyebrow">Object observations</p><h2>Objects in this perspective</h2></div><span className="panel-note">Association does not establish ownership or subscription</span></div>
          <ObjectCoverage data={objects ? state.inspectors.object_observations : undefined} />
          {objects?.length ? <div className="object-cards">{objects.slice(0, objectLimit).map((row) => <details key={row.object_key}><summary><strong>{row.object_name}</strong><span>{row.class_name ?? "Class not reported"}</span><span className="state-pill">{row.status}</span></summary><pre>{JSON.stringify(row.attributes, null, 2)}</pre><small>Latest values attributed to {selected} · {formatTime(row.last_seen)} · independent of chart window</small></details>)}</div> : <p className="empty">{objects ? "No attributed object snapshots for this federate." : "Persistent attributed values unavailable on this backend. Global values are not substituted."}</p>}
          {objects && objects.length > objectLimit && <button className="secondary-button load-more" type="button" onClick={() => setObjectPage({ federate: selected, limit: objectLimit + 100 })}>Show 100 more attributed objects</button>}
        </article>
        <TimeWorkspace key={`time:${selected}`} state={state} analytics={analytics} federate={selected} />
        <DeclarationsView key={`declarations:${selected}`} state={state} federate={selected} />
        <EventTable key={`events:${selected}`} events={scopedEvents} title="Federate event timeline" preference="federate_events" />
      </>}
    </div>
  </div>;
}
