import { useState } from "react";
import type { CapabilityResponse } from "../api/types";
import { CapabilityGrid } from "../components/CapabilityGrid";
import { EventTable } from "../components/EventTable";
import { InspectorTables } from "../components/InspectorTables";
import { LiveCharts } from "../components/LiveCharts";
import { MetricCard } from "../components/MetricCard";
import { useObserver } from "./useObserver";

type Tab = "overview" | "events" | "inspectors";

export function App() {
  const observer = useObserver();
  const [tab, setTab] = useState<Tab>("overview");
  const state = observer.state;
  const metrics = state?.live_metrics;
  const retention = state?.retention;
  const capabilities = state?.capabilities ?? observer.contract?.capabilities ?? {};
  const knownCapabilities = observer.contract?.capability_schema.known ?? [];

  return (
    <main className="app-shell">
      <header className="topbar">
        <div className="brand"><div className="brand-mark" aria-hidden="true" /><div><p className="eyebrow">Umbra / observer</p><h1>Runtime control surface</h1></div></div>
        <div className="toolbar"><ConnectionBadge state={observer.connection} /><button className="primary-button" type="button" onClick={() => void observer.refresh()}>Sync now</button></div>
      </header>

      <section className="hero">
        <div><p className="eyebrow">Provider-neutral telemetry</p><h2>Runtime events, made legible.</h2><p>React is only the view. The same contract works with Python, Java, or a vendor-specific backend.</p></div>
        <div className="hero-meta"><HeroStat label="Contract" value={state?.contract_version ?? observer.contract?.contract_version ?? "pending"} /><HeroStat label="Schema" value={state?.schema_version ?? "pending"} /><HeroStat label="Last sequence" value={retention?.last_sequence ?? 0} /><HeroStat label="History" value={`${retention?.retained_events ?? 0} retained`} /></div>
      </section>

      <nav className="tabs" aria-label="Observer views">
        {(["overview", "events", "inspectors"] as const).map((value) => <button className={tab === value ? "active" : ""} key={value} type="button" onClick={() => setTab(value)}>{value}</button>)}
      </nav>

      {observer.error && <div className="error-banner">{observer.error}</div>}
      {!state ? <LoadingState /> : <>
        <section className="metrics" aria-label="Runtime metrics">
          <MetricCard label="Events" value={metrics?.event_count ?? 0} />
          <MetricCard label="Federates" value={metrics?.federate_count ?? 0} tone="blue" unavailable={!isUsable(capabilities.federates)} />
          <MetricCard label="Objects" value={metrics?.object_count ?? 0} tone="violet" unavailable={!isUsable(capabilities.objects)} />
          <MetricCard label="Interactions" value={metrics?.interaction_count ?? 0} tone="blue" unavailable={!isUsable(capabilities.interactions)} />
          <MetricCard label="Errors" value={metrics?.error_count ?? 0} tone="red" />
          <MetricCard label="Event rate" value={`${metrics?.event_rate_per_second ?? 0}/s`} tone="amber" />
          <MetricCard label="Retained" value={retention?.retained_events ?? 0} />
          <MetricCard label="Dropped" value={retention?.dropped_events ?? 0} tone="amber" />
        </section>

        {tab === "overview" && <><LiveCharts events={observer.events} samples={observer.samples} /><CapabilityGrid capabilities={capabilities} known={knownCapabilities} /><ContextPanel state={state} /></>}
        {tab === "events" && <EventTable events={observer.events} />}
        {tab === "inspectors" && <InspectorTables state={state} />}
      </>}
    </main>
  );
}

function ConnectionBadge({ state }: { state: string }) {
  return <div className={`connection-badge ${state}`}><span className="status-dot" />{state}</div>;
}

function HeroStat({ label, value }: { label: string; value: string | number }) {
  return <div className="hero-stat"><label>{label}</label><span title={String(value)}>{value}</span></div>;
}

function ContextPanel({ state }: { state: NonNullable<ReturnType<typeof useObserver>["state"]> }) {
  const contextItems: Array<[string, string]> = [
    ["Connection", String(state.context.connection ?? "—")],
    ["Federation", String(state.context.federation ?? "—")],
    ["Source", String(state.context.source ?? "—")],
    ["Status", state.status],
  ];
  return <section className="panel context-panel"><div className="panel-heading"><div><p className="eyebrow">Session</p><h2>Runtime context</h2></div><span className="panel-note">{state.adapters.length} adapter(s)</span></div><div className="context-grid">{contextItems.map(([label, value]) => <div className="context-item" key={label}><label>{label}</label><span>{value}</span></div>)}</div></section>;
}

function LoadingState() {
  return <section className="panel loading"><div className="spinner" /><h2>Connecting to observer</h2><p>Waiting for the versioned backend contract.</p></section>;
}

function isUsable(capability: CapabilityResponse | undefined): boolean {
  return capability?.state === "available" || capability?.state === "partial";
}
