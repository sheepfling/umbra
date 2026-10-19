import { useEffect, useState } from "react";
import type { AnalyticsResponse, EventEnvelope, StateResponse } from "../api/types";
import { DiagnosticsView } from "../components/DiagnosticsView";
import { ActionsView } from "../components/ActionsView";
import { EventTable } from "../components/EventTable";
import { FederateView } from "../components/FederateView";
import { FederationView } from "../components/FederationView";
import { InspectorTables } from "../components/InspectorTables";
import { LiveCharts } from "../components/LiveCharts";
import { MetricCard } from "../components/MetricCard";
import { ObjectsView } from "../components/ObjectsView";
import { EmptyPanel } from "../components/ChartPanel";
import { TimeWorkspace } from "../components/TimeWorkspace";
import { useAnalytics } from "./useAnalytics";
import { useObserver } from "./useObserver";
import type { MetricSample } from "./useObserver";
import { formatTime } from "./observations";

const tabs = ["federation", "federates", "objects", "time", "events", "inspectors", "actions", "diagnostics"] as const;
type Tab = typeof tabs[number];
const labels: Record<Tab, string> = { federation: "Federation", federates: "Federates", objects: "Objects", time: "Logical time", events: "Events", inspectors: "Inspectors", actions: "Actions", diagnostics: "Diagnostics" };

interface FrozenView {
  state: StateResponse;
  events: EventEnvelope[];
  samples: MetricSample[];
  analytics?: AnalyticsResponse;
  at: number;
}

function tabFromHash(): Tab {
  const value = window.location.hash.slice(1);
  return tabs.find((tab) => tab === value) ?? "federation";
}

export function App() {
  const observer = useObserver();
  const [tab, setTab] = useState<Tab>(tabFromHash);
  const [windowSeconds, setWindowSeconds] = useState(300);
  const [selectedFederate, setSelectedFederate] = useState("");
  const [frozen, setFrozen] = useState<FrozenView>();
  const analyticsCapability = observer.state?.capabilities.analytics;
  const supportsAnalytics = (analyticsCapability?.state === "available" || analyticsCapability?.state === "partial")
    && Boolean(observer.contract?.api_paths.analytics);
  const liveAnalytics = useAnalytics(supportsAnalytics, windowSeconds);
  const state = frozen?.state ?? observer.state;
  const events = frozen?.events ?? observer.events;
  const samples = frozen?.samples ?? observer.samples;
  const analytics = frozen ? frozen.analytics : liveAnalytics.data;
  const federate = state?.federate_roster.some((row) => row.federate === selectedFederate)
    ? selectedFederate : state?.federate_roster[0]?.federate ?? "";

  useEffect(() => {
    const update = () => setTab(tabFromHash());
    window.addEventListener("hashchange", update);
    return () => window.removeEventListener("hashchange", update);
  }, []);

  const navigate = (value: Tab) => { setTab(value); window.location.hash = value; };
  const inspectFederate = (name: string) => { setSelectedFederate(name); navigate("federates"); };
  const togglePause = () => {
    if (frozen) setFrozen(undefined);
    else if (state) setFrozen({ state, events, samples, analytics, at: Date.now() / 1_000 });
  };

  return <main className="app-shell">
    <header className="topbar">
      <div className="brand"><div className="brand-mark" aria-hidden="true" /><div><p className="eyebrow">Umbra / observer</p><h1>Federation workspace</h1></div></div>
      <div className="toolbar"><div className={`connection-badge ${observer.connection}`} role="status"><span className="status-dot" />{observer.connection}</div><button className="secondary-button" type="button" disabled={!state} onClick={togglePause}>{frozen ? "Resume live view" : "Pause view"}</button><button className="primary-button" type="button" disabled={Boolean(frozen)} onClick={() => void observer.refresh()}>Sync now</button></div>
    </header>

    <section className="hero"><div><p className="eyebrow">Observe · investigate · understand</p><h2>{String(state?.context.federation ?? "Your federation, in focus.")}</h2><p>Follow activity across the federation, then step into an individual federate’s perspective.</p></div><div className="hero-meta"><div className="hero-stat"><label>Session</label><span>{state?.status ?? "Connecting"}</span></div><div className="hero-stat"><label>Retained history</label><span>{state?.retention.retained_events ?? 0} events</span></div><div className="hero-stat"><label>Perspective</label><span>Observer visibility</span></div><div className="hero-stat"><label>Chart window</label><select aria-label="Chart time window" value={windowSeconds} disabled={Boolean(frozen)} onChange={(event) => setWindowSeconds(Number(event.target.value))}><option value={30}>Last 30 seconds</option><option value={300}>Last 5 minutes</option><option value={900}>Last 15 minutes</option><option value={3600}>Last hour</option></select></div></div></section>

    <nav className="tabs" aria-label="Observer views">{tabs.map((value) => <button aria-current={tab === value ? "page" : undefined} className={tab === value ? "active" : ""} key={value} type="button" onClick={() => navigate(value)}>{labels[value]}</button>)}</nav>
    {observer.error && <div className="error-banner" role="alert">{observer.error}</div>}
    {observer.notice && <div className="notice-banner">{observer.notice}</div>}
    {frozen && <div className="notice-banner" role="status">View paused at {formatTime(frozen.at)}. Collection continues in the background. Resume to see current activity.</div>}
    {!state ? <section className="panel loading"><div className="spinner" /><h2>Connecting to observer</h2><p>Waiting for the federation snapshot.</p></section> : <>
      <section className="metrics" aria-label="Runtime metrics">
        <MetricCard label={analytics ? "Window events" : "Retained events"} value={analytics?.events_in_window ?? state.live_metrics.event_count} />
        <MetricCard label="Federates observed" value={state.federate_roster.length} tone="blue" />
        <MetricCard label="Objects observed" value={state.live_metrics.object_count ?? 0} tone="violet" unavailable={!usable(state, "objects")} />
        <MetricCard label="Interactions" value={state.live_metrics.interaction_count ?? 0} tone="blue" unavailable={!usable(state, "interactions")} />
        <MetricCard label="Errors retained" value={state.live_metrics.error_count} tone="red" />
        <MetricCard label="Event rate" value={`${state.live_metrics.event_rate_per_second}/s`} tone="amber" />
        <MetricCard label="Unattributed" value={analytics?.unattributed_events ?? "—"} />
        <MetricCard label="Dropped history" value={state.retention.dropped_events} tone="amber" />
      </section>
      {analytics?.history_truncated && <p className="coverage-note">Some history has expired or been cleared. Counts and charts describe retained observations in the selected window.</p>}
      {!frozen && liveAnalytics.error && <div className="notice-banner" role="status">{liveAnalytics.error}</div>}
      {tab === "federation" && (analytics ? <FederationView analytics={analytics} state={state} onFederate={inspectFederate} /> : <><EmptyPanel title={supportsAnalytics ? "Preparing activity charts" : "Windowed analytics unavailable"}>{supportsAnalytics ? "Waiting for the first analytics window." : "This backend does not advertise analytics. Live metrics, events, and inspectors remain available."}</EmptyPanel><LiveCharts events={events} samples={samples} /></>)}
      {tab === "federates" && <FederateView state={state} analytics={analytics} events={events} selected={federate} onSelect={setSelectedFederate} />}
      {tab === "objects" && <ObjectsView state={state} />}
      {tab === "time" && <TimeWorkspace state={state} analytics={analytics} />}
      {tab === "events" && <EventTable events={events} />}
      {tab === "inspectors" && <InspectorTables state={state} />}
      {tab === "actions" && <ActionsView enabled={usable(state, "commands")} reason={state.capabilities.commands?.reason} paused={Boolean(frozen)} />}
      {tab === "diagnostics" && <DiagnosticsView state={state} events={events} known={observer.contract?.capability_schema.known ?? []} connection={observer.connection} />}
    </>}
    <footer className="app-footer">Observation coverage may be partial. Actions affect only registered sessions owned by this backend.</footer>
  </main>;
}

function usable(state: StateResponse, name: string): boolean {
  return state.capabilities[name]?.state === "available" || state.capabilities[name]?.state === "partial";
}
