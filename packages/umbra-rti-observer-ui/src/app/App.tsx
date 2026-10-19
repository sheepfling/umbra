import { Fragment, useEffect, useState } from "react";
import { AlertsView } from "../components/AlertsView";
import type { AnalyticsResponse, EventEnvelope, PortableRecording, StateResponse } from "../api/types";
import { DiagnosticsView } from "../components/DiagnosticsView";
import { DeclarationsView } from "../components/DeclarationsView";
import { CoordinationView } from "../components/CoordinationView";
import { ActionsView } from "../components/ActionsView";
import { EventTable } from "../components/EventTable";
import { FederateView } from "../components/FederateView";
import { FederationView } from "../components/FederationView";
import { FomView } from "../components/FomView";
import { InspectorTables } from "../components/InspectorTables";
import { InteractionFlowView } from "../components/InteractionFlowView";
import { LiveCharts } from "../components/LiveCharts";
import { MetricCard } from "../components/MetricCard";
import { MomView } from "../components/MomView";
import { ObjectsView } from "../components/ObjectsView";
import { OwnershipView } from "../components/OwnershipView";
import { RegionsView } from "../components/RegionsView";
import { RecordingsView, ReplayBanner } from "../components/RecordingsView";
import { EmptyPanel } from "../components/ChartPanel";
import { TimeWorkspace } from "../components/TimeWorkspace";
import { TopologyView } from "../components/TopologyView";
import { useAnalytics } from "./useAnalytics";
import { useObserver } from "./useObserver";
import { useRecordingAnalytics, useReplay } from "./useReplay";
import { StorageNotice } from "../components/StorageCoverage";
import type { MetricSample } from "./useObserver";
import { formatTime } from "./observations";
import { useWorkspaceRoot, useWorkspaces } from "../workspace/WorkspaceContext";
import { WorkspacePanel } from "../workspace/WorkspacePanel";
import type { WorkspaceSettings } from "../api/types";

const tabs = ["federation", "federates", "objects", "declarations", "topology", "interactions", "ownership", "regions", "mom", "fom", "time", "synchronization", "persistence", "events", "inspectors", "recordings", "alerts", "actions", "diagnostics"] as const;
type Tab = typeof tabs[number];
const labels: Record<Tab, string> = { federation: "Federation", federates: "Federates", objects: "Objects", declarations: "Declarations", topology: "Topology", interactions: "Interaction flow", ownership: "Ownership", regions: "Regions / DDM", mom: "MOM reports", fom: "FOM browser", time: "Logical time", synchronization: "Synchronization", persistence: "Save / restore", events: "Events", inspectors: "Inspectors", recordings: "Recordings", alerts: "Alerts", actions: "Actions", diagnostics: "Diagnostics" };

interface FrozenView {
  state: StateResponse;
  events: EventEnvelope[];
  samples: MetricSample[];
  analytics?: AnalyticsResponse;
  at: number;
}

function tabFromHash(): Tab | undefined {
  const value = window.location.hash.slice(1);
  return tabs.find((tab) => tab === value);
}

export function App() {
  const replay = useReplay();
  const isReplay = Boolean(replay.document);
  const observer = useObserver(!isReplay);
  const workspaces = useWorkspaces();
  const [tab, setTab] = useWorkspaceRoot("view");
  const [windowSeconds, updateWindowSeconds] = useWorkspaceRoot("window_seconds");
  const setWindowSeconds = (value: number) => { if ([30, 300, 900, 3600].includes(value)) updateWindowSeconds(value as WorkspaceSettings["window_seconds"]); };
  const [selectedFederate, setSelectedFederate] = useWorkspaceRoot("selected_federate");
  const [showWorkspaces, setShowWorkspaces] = useState(false);
  const [frozen, setFrozen] = useState<FrozenView>();
  const sourceState = isReplay ? replay.frame?.state : observer.state;
  const displayContract = replay.document?.contract ?? observer.contract;
  const analyticsCapability = sourceState?.capabilities.analytics;
  const supportsAnalytics = (analyticsCapability?.state === "available" || analyticsCapability?.state === "partial")
    && Boolean(displayContract?.api_paths.analytics);
  const liveAnalytics = useAnalytics(!isReplay && supportsAnalytics, windowSeconds);
  const replayAnalytics = useRecordingAnalytics(replay.frame, isReplay && !replay.loading && supportsAnalytics, windowSeconds);
  const state = frozen?.state ?? sourceState;
  const events = frozen?.events ?? (isReplay ? replay.frame?.state.normalized_events ?? [] : observer.events);
  const samples = frozen?.samples ?? (isReplay ? replay.samples : observer.samples);
  const activeAnalytics = isReplay ? replayAnalytics : liveAnalytics;
  const analytics = frozen ? frozen.analytics : activeAnalytics.data;
  const federate = selectedFederate || state?.federate_roster[0]?.federate || analytics?.participants[0]?.federate || "";

  useEffect(() => {
    const initial = tabFromHash();
    if (initial) setTab(initial);
    const update = () => { const selected = tabFromHash(); if (selected) setTab(selected); };
    window.addEventListener("hashchange", update);
    return () => window.removeEventListener("hashchange", update);
  }, [setTab]);

  useEffect(() => {
    if (window.location.hash.slice(1) !== tab) window.history.replaceState(null, "", `#${tab}`);
  }, [tab]);

  const navigate = (value: Tab) => { setTab(value); window.location.hash = value; };
  const inspectFederate = (name: string) => { setSelectedFederate(name); navigate("federates"); };
  const togglePause = () => {
    if (frozen) setFrozen(undefined);
    else if (state) setFrozen({ state, events, samples, analytics, at: Date.now() / 1_000 });
  };
  const openReplay = (document: PortableRecording) => { setFrozen(undefined); replay.load(document); };
  const closeReplay = () => { setFrozen(undefined); replay.close(); };

  return <main className="app-shell">
    <header className="topbar">
      <div className="brand"><div className="brand-mark" aria-hidden="true" /><div><p className="eyebrow">Umbra / observer</p><h1>Federation workspace</h1></div></div>
      <div className="toolbar"><div className={`connection-badge ${isReplay ? "replay" : observer.connection}`} role="status"><span className="status-dot" />{isReplay ? "replay" : observer.connection}</div><button className="secondary-button" type="button" aria-expanded={showWorkspaces} aria-controls="saved-workspaces" onClick={() => setShowWorkspaces((value) => !value)}>Saved workspaces</button><button className="secondary-button" type="button" disabled={!state || isReplay} onClick={togglePause}>{frozen ? "Resume live view" : "Pause view"}</button><button className="primary-button" type="button" disabled={Boolean(frozen) || isReplay} onClick={() => void observer.refresh()}>Sync now</button></div>
    </header>
    {workspaces.error && <div className="notice-banner" role="status">{workspaces.error}</div>}
    {workspaces.notice && <div className="notice-banner" role="status">{workspaces.notice}<button type="button" className="secondary-button" onClick={() => workspaces.announce(undefined)}>Dismiss workspace notice</button></div>}
    {showWorkspaces && <WorkspacePanel canApply={!frozen && !isReplay} displayedParticipant={tab === "federates" ? federate : undefined} />}
    <ReplayBanner replay={replay} onClose={closeReplay} />

    <section className="hero"><div><p className="eyebrow">Observe · investigate · understand</p><h2>{String(state?.context.federation ?? "Your federation, in focus.")}</h2><p>Follow activity across the federation, then step into an individual federate’s perspective.</p></div><div className="hero-meta"><div className="hero-stat"><label>Session</label><span>{state?.status ?? "Connecting"}</span></div><div className="hero-stat"><label>Retained history</label><span>{state?.retention.retained_events ?? 0} events</span></div><div className="hero-stat"><label>Perspective</label><span>Observer visibility</span></div><div className="hero-stat"><label>Chart window</label><select aria-label="Chart time window" value={windowSeconds} disabled={Boolean(frozen)} onChange={(event) => setWindowSeconds(Number(event.target.value))}><option value={30}>Last 30 seconds</option><option value={300}>Last 5 minutes</option><option value={900}>Last 15 minutes</option><option value={3600}>Last hour</option></select></div></div></section>

    <nav className="tabs" aria-label="Observer views">{tabs.map((value) => <button aria-current={tab === value ? "page" : undefined} className={tab === value ? "active" : ""} key={value} type="button" onClick={() => navigate(value)}>{labels[value]}</button>)}</nav>
    {!isReplay && observer.error && <div className="error-banner" role="alert">{observer.error}</div>}
    {!isReplay && observer.notice && <div className="notice-banner">{observer.notice}</div>}
    {frozen && <div className="notice-banner" role="status">View paused at {formatTime(frozen.at)}. Collection continues in the background. Resume to see current activity.</div>}
    {!state ? <section className="panel loading">{!replay.error && <div className="spinner" />}<h2>{isReplay ? replay.error ? "Recorded state unavailable" : "Preparing recorded state" : "Connecting to observer"}</h2><p>{isReplay ? "Only the validated recording is used. No live snapshot is substituted. You can return to live using the replay controls." : "Waiting for the federation snapshot."}</p></section> : <Fragment key={workspaces.generation}>
      <section className="metrics" aria-label="Runtime metrics">
        <MetricCard label={analytics ? "Window events" : "Retained events"} value={analytics?.events_in_window ?? state.live_metrics.event_count} />
        <MetricCard label="Roster entries" value={state.federate_roster.length} tone="blue" />
        <MetricCard label="Objects retained" value={state.live_metrics.object_count ?? 0} tone="violet" unavailable={!usable(state, "objects")} />
        <MetricCard label="Interactions" value={state.live_metrics.interaction_count ?? 0} tone="blue" unavailable={!usable(state, "interactions")} />
        <MetricCard label="Errors retained" value={state.live_metrics.error_count} tone="red" />
        <MetricCard label="Event rate" value={`${state.live_metrics.event_rate_per_second}/s`} tone="amber" />
        <MetricCard label="Unattributed" value={analytics?.unattributed_events ?? "—"} />
        <MetricCard label="Dropped history" value={state.retention.dropped_events} tone="amber" />
      </section>
      {analytics?.history_truncated && <p className="coverage-note">Some history has expired or been cleared. Counts and charts describe retained observations in the selected window.</p>}
      <StorageNotice data={usable(state, "storage_inspection") ? state.storage : undefined} />
      {!frozen && activeAnalytics.error && <div className="notice-banner" role="status">{activeAnalytics.error}</div>}
      {tab === "federation" && (analytics ? <FederationView analytics={analytics} state={state} onFederate={inspectFederate} /> : <><EmptyPanel title={supportsAnalytics ? "Preparing activity charts" : "Windowed analytics unavailable"}>{supportsAnalytics ? "Waiting for the first analytics window." : "This backend does not advertise analytics. Live metrics, events, and inspectors remain available."}</EmptyPanel><LiveCharts events={events} samples={samples} /></>)}
      {tab === "federates" && <FederateView state={state} analytics={analytics} events={events} selected={federate} onSelect={setSelectedFederate} />}
      {tab === "objects" && <ObjectsView state={state} />}
      {tab === "declarations" && <DeclarationsView state={state} />}
      {tab === "topology" && <TopologyView state={state} />}
      {tab === "interactions" && <InteractionFlowView state={state} />}
      {tab === "ownership" && <OwnershipView state={state} />}
      {tab === "regions" && <RegionsView state={state} />}
      {tab === "mom" && <MomView state={state} />}
      {tab === "fom" && <FomView state={state} />}
      {tab === "synchronization" && <CoordinationView key="synchronization" state={state} workspace="synchronization" />}
      {tab === "persistence" && <CoordinationView key="persistence" state={state} workspace="persistence" />}
      {tab === "time" && <TimeWorkspace state={state} analytics={analytics} />}
      {tab === "events" && <EventTable events={events} />}
      {tab === "inspectors" && <InspectorTables state={state} />}
      {tab === "recordings" && <RecordingsView enabled={Boolean(observer.state && usable(observer.state, "recording"))} reason={observer.state?.capabilities.recording?.reason} onReplay={openReplay} />}
      {tab === "alerts" && <AlertsView enabled={usable(state, "alerts")} controls={usable(state, "alert_controls")} paused={Boolean(frozen)} replay={isReplay} reason={state.capabilities.alerts?.reason} />}
      {tab === "actions" && <ActionsView enabled={!isReplay && usable(state, "commands")} reason={isReplay ? "Replay is read-only. Captured capabilities do not grant command authority." : state.capabilities.commands?.reason} paused={Boolean(frozen) || isReplay} />}
      {tab === "diagnostics" && <DiagnosticsView state={state} events={events} known={displayContract?.capability_schema.known ?? []} connection={isReplay ? "replay" : observer.connection} />}
    </Fragment>}
    <footer className="app-footer">Observation coverage may be partial. Actions affect only registered sessions owned by this backend.</footer>
  </main>;
}

function usable(state: StateResponse, name: string): boolean {
  return state.capabilities[name]?.state === "available" || state.capabilities[name]?.state === "partial";
}
