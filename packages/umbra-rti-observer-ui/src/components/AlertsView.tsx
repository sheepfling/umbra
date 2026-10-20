import { useState } from "react";
import { useWorkspaceField } from "../workspace/WorkspaceContext";
import { RetainedSelect } from "../workspace/RetainedSelect";
import { Bar, BarChart, CartesianGrid, ResponsiveContainer, Tooltip, XAxis, YAxis } from "recharts";
import type { AlertConfiguration, AlertResult, AlertRule, ParticipantPerspective } from "../api/types";
import type { ParticipantChoice } from "../app/participants";
import { perspectiveDescription } from "../app/participants";
import { formatTime } from "../app/observations";
import { useAlerts } from "../app/useAlerts";
import { ChartPanel, chartTick, chartTooltip, EmptyPanel } from "./ChartPanel";

function metric(row: AlertResult): string {
  if (row.metric == null) return "Unknown";
  const render = (value: number) => row.unit === "seconds" ? value.toFixed(2) : String(value);
  return `${render(row.metric)}${row.metric_upper != null && row.metric_upper !== row.metric ? `–${render(row.metric_upper)}` : ""} ${row.unit}`;
}

export function AlertsView({ enabled, controls, paused, replay, reason, perspectives = [], identityFilters = false }: {
  enabled: boolean; controls: boolean; paused: boolean; replay: boolean; reason?: string | null;
  perspectives?: ParticipantChoice[]; identityFilters?: boolean;
}) {
  const alerts = useAlerts(enabled && !replay, paused || replay);
  const [editing, setDraft] = useState<AlertRule>();
  const [editVersion, setEditVersion] = useState<{ revision: number; instance_id: string }>();
  const [search, setSearch] = useWorkspaceField("alerts", "search");
  const [status, setStatus] = useWorkspaceField("alerts", "status");
  const [limit, setLimit] = useState(50);
  const [importError, setImportError] = useState<string>();
  if (replay) return <EmptyPanel title="Live alerts are not part of this recording">Replay does not fetch current alert configuration or evidence, run rules over future data, or change live rules. Return to live to manage alerts.</EmptyPanel>;
  if (!enabled) return <EmptyPanel title="Observer alerts unavailable">{reason ?? "This backend does not advertise alert evidence."} Events, time and other inspectors remain independent.</EmptyPanel>;
  const report = alerts.data;
  const staleEdit = Boolean(editing && editVersion && report && (editVersion.revision !== report.revision || editVersion.instance_id !== report.instance_id));
  const locked = !controls || paused || alerts.busy || Boolean(alerts.error) || !report || staleEdit;
  const setEditing = (rule: AlertRule | undefined) => { setDraft(rule); setEditVersion(rule && report ? { revision: report.revision, instance_id: report.instance_id } : undefined); };
  const configuration = (rules: AlertRule[]): AlertConfiguration => ({ configuration_version: "umbra-observer-alerts-v1", rules });
  const replaceRules = async (rules: AlertRule[]) => report && alerts.mutate(() => alerts.client.configureAlerts(configuration(rules), report.revision, report.instance_id));
  const reset = (ruleId: string) => report && alerts.mutate(() => alerts.client.resetAlert(ruleId, report.revision, report.instance_id));
  const acknowledge = (evidenceId: number) => report && alerts.mutate(() => alerts.client.acknowledgeAlert(evidenceId, report.instance_id));
  const history = report?.history.filter((row) => (status === "all" || row.result.state === status)
    && `${row.rule.name} ${row.rule.rule_id} ${row.result.reason}`.toLowerCase().includes(search.toLowerCase())) ?? [];
  const distribution = ["firing", "unknown", "clear", "ended", "disabled"].map((name) => ({ name, count: report?.results.filter((row) => row.state === name).length ?? 0 }));
  const exportConfiguration = () => {
    if (!report) return;
    const url = URL.createObjectURL(new Blob([JSON.stringify(configuration(report.rules), null, 2)], { type: "application/json" }));
    const anchor = document.createElement("a"); anchor.href = url; anchor.download = "umbra-alert-configuration.json"; anchor.click();
    window.setTimeout(() => URL.revokeObjectURL(url), 1000);
  };
  return <section className="alerts-workspace" aria-label="Observer alerts">
    <div className="section-intro"><div><p className="eyebrow">Observe · evaluate · retain evidence</p><h2>Observer alerts</h2><p>Thresholds describe this observer’s visibility. Idle is not disconnected; unknown coverage is not a healthy zero.</p></div></div>
    <div className="notice-banner">Configuration and bounded evidence are shared for this server process. Download configuration to keep it across restarts. No provider calls or payload copies are made. Pausing this view does not pause evaluation.</div>
    {!controls && <div className="notice-banner">Read-only alert evidence: this backend does not advertise configuration controls.</div>}
    {alerts.error && <div className="error-banner" role="alert">{alerts.error}</div>}
    {alerts.feedback && <div className="error-banner" role="alert">{alerts.feedback}</div>}
    {staleEdit && <div className="notice-banner" role="status">Configuration changed after editing began. Cancel editing and reopen the current rule before saving; the draft was not applied.</div>}
    {importError && <div className="error-banner" role="alert">{importError}</div>}
    {!report ? <EmptyPanel title={paused ? "Resume live to inspect alerts" : "Loading alert evidence"}>No live data is substituted into a paused or replayed view.</EmptyPanel> : <>
      <div className="recording-actions"><button type="button" className="secondary-button" onClick={exportConfiguration}>Download alert configuration</button><label>Import alert configuration<input type="file" accept="application/json,.json" disabled={locked} onChange={(event) => {
        const file = event.target.files?.[0]; event.target.value = ""; setImportError(undefined);
        if (!file) return;
        if (file.size > 128 * 1024) { setImportError("Alert configuration file exceeds 128 KiB."); return; }
        if (!window.confirm("Replace the alert rules with this configuration? Existing evidence stays. Imported files cannot change the backend or call an RTI.")) return;
        void file.text().then((raw) => alerts.mutate(() => alerts.client.importAlerts(raw, report.revision, report.instance_id))).catch(() => setImportError("The configuration file could not be read."));
      }} /></label></div>
      <p className="footnote">Downloads contain only rules, including chosen join identity metadata and identifying filters—not event payloads, evidence, endpoints or server credentials. Review identifying configuration before sharing. Revision {report.revision}; {report.rules.length}/{report.max_rules} rules.</p>
      <ChartPanel title="Current alert states" eyebrow="Configured rules" note="Counts of rules, not federation members"><ResponsiveContainer width="100%" height="100%"><BarChart data={distribution}><CartesianGrid strokeDasharray="3 3" stroke="#20354b" /><XAxis dataKey="name" tick={chartTick} /><YAxis allowDecimals={false} tick={chartTick} /><Tooltip contentStyle={chartTooltip} /><Bar name="Rules" dataKey="count" fill="#7dd3fc" /></BarChart></ResponsiveContainer></ChartPanel>
      <article className="panel" aria-label="Current alert rules"><h2>Current rules</h2><p className="footnote">Error windows start at rule activation; unchanged rules keep their baseline. Drop counts are since activation/reset. Pending age uses cached accepted/running work, including a timed-out call still in flight.</p>
        {!report.rules.length ? <p>No alert rules configured.</p> : <div className="table-scroll"><table><thead><tr><th>Rule</th><th>State / reason</th><th>Evidence / threshold</th><th>Coverage</th><th>Configuration</th></tr></thead><tbody>{report.rules.map((rule) => {
          const result = report.results.find((row) => row.rule_id === rule.rule_id);
          return <tr key={rule.rule_id}><td><strong>{rule.name}</strong><small>{rule.kind} · {rule.severity}</small></td><td><span className={`state-pill ${result?.state === "firing" ? "unsupported" : result?.state === "clear" ? "available" : "partial"}`}>{result?.state ?? "unknown"}</span><small>{result?.reason ?? "Not evaluated"}</small></td><td>{result ? metric(result) : "Unknown"}<small>Threshold: {result?.threshold ?? "Unknown"} {result?.unit ?? ""}</small></td><td>{result?.coverage ?? "Unknown"}<small>Generation {result?.rule_generation ?? "Unknown"}</small></td><td><div className="recording-actions"><button type="button" disabled={locked} onClick={() => setEditing(rule)}>Edit {rule.name}</button><button type="button" disabled={locked} onClick={() => void replaceRules(report.rules.map((value) => value.rule_id === rule.rule_id ? { ...value, enabled: !value.enabled } : value))}>{rule.enabled ? "Disable" : "Enable"} {rule.name}</button><button type="button" disabled={locked} onClick={() => { if (window.confirm(`Reset the baseline for ${rule.name}? Existing evidence stays.`)) void reset(rule.rule_id); }}>Reset baseline {rule.name}</button><button type="button" disabled={locked} onClick={() => { if (window.confirm(`Delete ${rule.name}? Historical evidence stays.`)) void replaceRules(report.rules.filter((value) => value.rule_id !== rule.rule_id)); }}>Delete {rule.name}</button></div></td></tr>;
        })}</tbody></table></div>}
      </article>
      <RuleForm key={editing?.rule_id ?? "new"} rule={editing} perspectives={perspectives} identityFilters={identityFilters} disabled={locked || !editing && report.rules.length >= report.max_rules} cancel={() => setEditing(undefined)} save={async (rule) => {
        const saved = await replaceRules([...report.rules.filter((row) => row.rule_id !== rule.rule_id), rule]);
        if (saved) setEditing(undefined);
        return Boolean(saved);
      }} />
      <article className="panel" aria-label="Alert evidence history"><h2>Evidence history</h2><p className="footnote">{history.length} matching / {report.history.length} retained; {report.evicted_evidence} expired from the independent {report.evidence_capacity}-record alert history. Acknowledging does not resolve or reset a rule. Error detail keeps at most {report.error_marks_per_rule} time marks per rule; overflow bounds remain explicit.</p>
        <div className="filter-bar"><label>Search alert evidence<input aria-label="Search alert evidence" value={search} onChange={(event) => { setSearch(event.target.value); setLimit(50); }} /></label><label>Evidence state<RetainedSelect value={status} onChange={(event) => { setStatus(event.target.value); setLimit(50); }}><option value="all">All states</option>{distribution.map((row) => <option key={row.name}>{row.name}</option>)}</RetainedSelect></label></div>
        <div className="table-scroll"><table><thead><tr><th>Rule at observation</th><th>State / evidence</th><th>Observed</th><th>Acknowledgement</th></tr></thead><tbody>{history.slice(0, limit).map((row) => <tr key={row.evidence_id}><td>{row.rule.name}<small>Generation {row.result.rule_generation} · {row.rule.severity}</small></td><td>{row.result.state} · {row.result.reason}<small>{metric(row.result)} · {row.result.coverage}</small><details><summary>Captured rule and evidence</summary><pre>{JSON.stringify(row, null, 2)}</pre></details></td><td>{formatTime(row.result.captured_at)}<small>Event sequence {row.result.last_sequence ?? "not applicable"}</small></td><td>{row.acknowledged ? "Acknowledged" : <button type="button" disabled={locked} onClick={() => void acknowledge(row.evidence_id)}>Acknowledge evidence {row.evidence_id}</button>}</td></tr>)}</tbody></table></div>
        {!history.length && <p>No matching alert evidence.</p>}{history.length > limit && <button type="button" onClick={() => setLimit((value) => value + 50)}>Show more alert evidence</button>}
      </article>
    </>}
  </section>;
}

function RuleForm({ rule, disabled, cancel, save, perspectives, identityFilters }: { rule?: AlertRule; disabled: boolean; cancel: () => void; save: (rule: AlertRule) => Promise<boolean>; perspectives: ParticipantChoice[]; identityFilters: boolean }) {
  const [name, setName] = useState(rule?.name ?? "");
  const [kind, setKind] = useState<AlertRule["kind"]>(rule?.kind ?? "errors");
  const [severity, setSeverity] = useState<AlertRule["severity"]>(rule?.severity ?? "warning");
  const [threshold, setThreshold] = useState(String(rule ? "threshold_seconds" in rule ? rule.threshold_seconds : rule.threshold_count : 1));
  const [window, setWindow] = useState(String(rule?.kind === "errors" ? rule.window_seconds ?? 300 : 300));
  const [source, setSource] = useState(rule && "scope" in rule ? rule.scope?.source ?? "" : "");
  const [federation, setFederation] = useState(rule && "scope" in rule ? rule.scope?.federation ?? "" : "");
  const [federate, setFederate] = useState(rule && "scope" in rule ? rule.scope?.federate ?? "" : "");
  const [session, setSession] = useState(rule && "scope" in rule ? rule.scope?.session_id ?? "" : "");
  const [perspective, setPerspective] = useState<ParticipantPerspective | undefined>(rule && "scope" in rule ? rule.scope?.perspective ?? undefined : undefined);
  const choices = perspectives.filter((row) => row.perspective?.basis === "explicit-join");
  if (perspective && !choices.some((row) => row.key === perspective.key)) choices.push({ key: perspective.key,
    label: perspective.attribution?.participant.display_name ?? perspective.reported_name ?? "Saved perspective", perspective });
  const [target, setTarget] = useState(rule?.kind === "pending-commands" ? rule.target_id ?? "" : "");
  const age = kind === "stale-observations" || kind === "pending-commands";
  return <article className="panel"><h2>{rule ? "Edit alert rule" : "Add alert rule"}</h2><form className="command-form alert-form" onSubmit={(event) => {
    event.preventDefault();
    if (disabled) return;
    const common = { rule_id: rule?.rule_id ?? crypto.randomUUID(), name: name.trim(), severity, enabled: rule?.enabled ?? true };
    const scope = perspective ? { perspective } : { source: source.trim() || null, federation: federation.trim() || null, federate: federate.trim() || null, session_id: session || null };
    const spec: AlertRule = kind === "errors" ? { ...common, kind, scope, threshold_count: Number(threshold), window_seconds: Number(window) }
      : kind === "stale-observations" ? { ...common, kind, scope, threshold_seconds: Number(threshold) }
        : kind === "dropped-history" ? { ...common, kind, threshold_count: Number(threshold) }
          : { ...common, kind, target_id: target.trim() || null, threshold_seconds: Number(threshold) };
    void save(spec).then((saved) => { if (saved && !rule) setName(""); });
  }}>
    <label>Rule name<input required maxLength={128} value={name} onChange={(event) => setName(event.target.value)} disabled={disabled} /></label>
    <label>Alert condition<select aria-label="Alert condition" value={kind} onChange={(event) => setKind(event.target.value as AlertRule["kind"])} disabled={disabled}><option value="errors">Error observations</option><option value="stale-observations">Stale observations</option><option value="dropped-history">Dropped history</option><option value="pending-commands">Pending commands</option></select></label>
    <label>Alert threshold ({age ? "seconds" : "observations"})<input aria-label="Alert threshold" type="number" required min={age ? .1 : 1} max={age ? 86400 : 1000000000} step={age ? .1 : 1} value={threshold} onChange={(event) => setThreshold(event.target.value)} disabled={disabled} /></label>
    <label>Severity<select aria-label="Severity" value={severity} onChange={(event) => setSeverity(event.target.value as AlertRule["severity"])} disabled={disabled}><option>info</option><option>warning</option><option>critical</option></select></label>
    {kind === "errors" && <label>Error window seconds<input type="number" min={1} max={3600} required value={window} onChange={(event) => setWindow(event.target.value)} disabled={disabled} /></label>}
    {(kind === "errors" || kind === "stale-observations") && <fieldset><legend>Explicit observation filter (blank means any)</legend>
      {(identityFilters || perspective) && <label>Alert participant perspective<select aria-label="Alert participant perspective" value={perspective?.key ?? ""} disabled={disabled} onChange={(event) => setPerspective(choices.find((row) => row.key === event.target.value)?.perspective ?? undefined)}><option value="">Use legacy text filters below</option>{choices.map((row) => <option key={row.key} value={row.key}>{row.label} · {perspectiveDescription(row.perspective)}</option>)}</select></label>}
      <label>Alert source<input maxLength={512} value={source} onChange={(event) => setSource(event.target.value)} disabled={disabled || Boolean(perspective)} /></label><label>Alert federation<input maxLength={512} value={federation} onChange={(event) => setFederation(event.target.value)} disabled={disabled || Boolean(perspective)} /></label><label>Alert federate<input maxLength={512} value={federate} onChange={(event) => setFederate(event.target.value)} disabled={disabled || Boolean(perspective)} /></label><label>Alert session ID<input maxLength={512} value={session} onChange={(event) => setSession(event.target.value)} disabled={disabled || Boolean(perspective)} /></label><p className="footnote">Exact perspectives keep source, session and reported join IDs separate; legacy names are not stable cross-RTI identities. Name-filtered monitors that observe explicit join IDs are not ended by one join's lifecycle event. No event yet means unknown, not disconnected. Session IDs are opaque and are not trimmed.</p></fieldset>}
    {kind === "pending-commands" && <label>Command target (blank means all registered targets)<input maxLength={128} value={target} onChange={(event) => setTarget(event.target.value)} disabled={disabled} /></label>}
    <div className="recording-actions"><button className="primary-button" type="submit" disabled={disabled}>{rule ? "Save alert changes" : "Create alert rule"}</button>{rule && <button type="button" onClick={cancel}>Cancel editing</button>}</div>
  </form></article>;
}
