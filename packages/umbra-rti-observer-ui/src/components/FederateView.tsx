import { useMemo, useState } from "react";
import type { AnalyticsResponse, EventEnvelope, StateResponse } from "../api/types";
import type { ParticipantChoice } from "../app/participants";
import { eventParticipantKey, objectKey, objectPerspectiveKey, participantKey, perspectiveDescription, supportsParticipantProvenance, unavailableParticipant } from "../app/participants";
import { formatTime } from "../app/observations";
import { useWorkspaceField } from "../workspace/WorkspaceContext";
import { EmptyPanel } from "./ChartPanel";
import { EventTable } from "./EventTable";
import { MetricCard } from "./MetricCard";
import { TimeWorkspace } from "./TimeWorkspace";
import { DeclarationsView } from "./DeclarationsView";
import { ObjectCoverage } from "./ObjectCoverage";

export function FederateView({ state, analytics, events, selected, choices, onSelect }: {
  state: StateResponse; analytics?: AnalyticsResponse; events: EventEnvelope[];
  selected: string; choices: ParticipantChoice[]; onSelect: (key: string) => void;
}) {
  const [search, setSearch] = useWorkspaceField("federates", "search");
  const [basis, setBasis] = useWorkspaceField("federates", "basis");
  const [limit, setLimit] = useState(100);
  const [objectPage, setObjectPage] = useState({ federate: selected, limit: 100 });
  const objectLimit = objectPage.federate === selected ? objectPage.limit : 100;
  const choice = choices.find((row) => row.key === selected) ?? unavailableParticipant(selected);
  const participant = analytics?.participants.find((row) => participantKey(row) === selected);
  const roster = state.federate_roster.find((row) => participantKey(row) === selected);
  const provenance = supportsParticipantProvenance(state, events);
  const candidates = useMemo(() => {
    const complete = selected && !choices.some((row) => row.key === selected) ? [...choices, unavailableParticipant(selected)] : choices;
    const needle = search.trim().toLocaleLowerCase();
    return complete.filter((row) => row.unresolved || ((basis === "all" || (row.perspective?.basis ?? "legacy-name") === basis)
      && [row.label, row.key, row.handle, perspectiveDescription(row.perspective)].join(" ").toLocaleLowerCase().includes(needle)));
  }, [choices, selected, search, basis]);
  const scopedEvents = useMemo(() => events.filter((event) => eventParticipantKey(event, provenance) === selected
    && (!analytics || (event.observed_at >= analytics.window_start && event.observed_at <= analytics.generated_at))), [events, selected, analytics, provenance]);
  const objects = useMemo(() => {
    const capability = state.capabilities.object_observations;
    if (capability?.state !== "available" && capability?.state !== "partial") return undefined;
    return state.inspectors.object_observations?.snapshots.filter((row) => objectPerspectiveKey(row) === selected).map((row) => row.object);
  }, [selected, state.inspectors.object_observations, state.capabilities]);

  if (!choices.length && !selected) return <EmptyPanel title="No federates observed">Participant evidence appears here when directly reported. MOM is optional; nested management/topology subjects do not create ordinary roster members.</EmptyPanel>;
  return <div className="federate-layout">
    <aside aria-label="Observed federates" className="panel federate-sidebar">
      <p className="eyebrow">Reporting perspectives · not a membership census</p><h2>Federates</h2>
      <label>Find a perspective<input aria-label="Search federate perspectives" value={search} placeholder="Name, handle, join ID or reporter" onChange={(event) => { setSearch(event.target.value); setLimit(100); }} /></label>
      <label>Identity evidence<select aria-label="Participant identity evidence" value={basis} onChange={(event) => { setBasis(event.target.value as typeof basis); setLimit(100); }}><option value="all">All reported perspectives</option><option value="explicit-join">Explicit join IDs</option><option value="legacy-provenance">Legacy reporting scope</option><option value="legacy-name">Older name-only projection</option></select></label>
      <p className="panel-note">{candidates.length} matching choices · {Math.min(limit, candidates.length)} shown</p>
      <div className="participant-links">{candidates.slice(0, limit).map((row) => {
        const metadata = state.federate_roster.find((entry) => participantKey(entry) === row.key);
        return <button type="button" className={row.key === selected ? "selected" : ""} key={row.key} onClick={() => onSelect(row.key)}>
          <span><strong>{row.label}</strong><small>{metadata?.role ?? "Role not reported"}</small><small className="participant-provenance">{row.unresolved ? "Saved choice not currently reported" : perspectiveDescription(row.perspective)}</small></span>
          <span className="state-pill">{metadata?.status ?? "Metadata not retained"}</span>
        </button>;
      })}</div>
      {candidates.length > limit && <button className="secondary-button load-more" type="button" onClick={() => setLimit((value) => value + 100)}>Show 100 more perspectives</button>}
      {!candidates.length && <p className="empty">No reported perspectives match these filters. The selected investigation was not replaced.</p>}
    </aside>
    <div className="federate-content">
      {!selected ? <EmptyPanel title="Choose a federate">Select reported evidence to investigate its perspective.</EmptyPanel> : <>
        <div className="section-intro"><div><p className="eyebrow">Federate workspace</p><h2>{choice.label}</h2>
          {roster ? <p>{roster.role ?? "Role not reported"} · {roster.status} · last seen {formatTime(roster.last_seen)}</p>
            : <p className="coverage-note">Roster metadata is not retained for this selection. This does not establish resignation or disconnection. Explicitly attributed retained events and independent inspectors remain usable.</p>}
          <p className="coverage-note participant-provenance">{choice.unresolved ? "This saved selection is not currently reported. No participant with a matching name was substituted." : perspectiveDescription(choice.perspective)}</p>
          {choice.perspective?.basis !== "explicit-join" && !choice.unresolved && <p className="coverage-note">Join identity is unreported. This legacy bucket must not be treated as one verified participant across reconnects or name reuse.</p>}
        </div>{roster && <span className="panel-note">{roster.source}</span>}</div>
        <section className="metrics focus-metrics">
          <MetricCard label="Attributed events" value={participant?.event_count ?? scopedEvents.length} />
          <MetricCard label="Object updates" value={participant?.object_updates ?? scopedEvents.filter((event) => event.event_type === "object.updated").length} tone="blue" />
          <MetricCard label="Interactions" value={participant?.interaction_count ?? scopedEvents.filter((event) => event.event_type.startsWith("interaction.")).length} tone="violet" />
          <MetricCard label="Errors" value={participant?.error_count ?? scopedEvents.filter((event) => event.event_type === "runtime.error").length} tone="red" />
        </section>
        <article className="panel"><div className="panel-heading"><div><p className="eyebrow">Object observations</p><h2>Objects in this perspective</h2></div><span className="panel-note">Association does not establish ownership or subscription</span></div>
          <ObjectCoverage data={objects ? state.inspectors.object_observations : undefined} />
          {objects?.length ? <div className="object-cards">{objects.slice(0, objectLimit).map((row) => <details key={objectKey(row)}><summary><strong>{row.object_name}</strong><span>{row.class_name ?? "Class not reported"}</span><span className="state-pill">{row.status}</span></summary><pre>{JSON.stringify(row.attributes, null, 2)}</pre><small className="participant-provenance">{row.object_context ? `${row.object_context.identity_namespace} / ${row.object_context.federation_id}` : "Object federation context unreported"} · latest values attributed to {choice.label} · {formatTime(row.last_seen)} · independent of chart window</small></details>)}</div>
            : <p className="empty">{objects ? "No attributed object snapshots for this federate." : "Persistent attributed values unavailable on this backend. Global values are not substituted."}</p>}
          {objects && objects.length > objectLimit && <button className="secondary-button load-more" type="button" onClick={() => setObjectPage({ federate: selected, limit: objectLimit + 100 })}>Show 100 more attributed objects</button>}
        </article>
        <TimeWorkspace key={`time:${selected}`} state={state} analytics={analytics} federate={selected} participantLabel={choice.label} />
        <DeclarationsView key={`declarations:${selected}`} state={state} federate={selected} participantLabel={choice.label} />
        <EventTable key={`events:${selected}`} events={scopedEvents} title="Federate event timeline" participant={selected} provenance={provenance} />
      </>}
    </div>
  </div>;
}
