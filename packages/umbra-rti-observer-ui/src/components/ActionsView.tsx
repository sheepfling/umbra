import { useState } from "react";
import { ObserverApiError } from "../api/client";
import type { CommandDefinition, CommandHistory, CommandRecord, CommandRequest, CommandTarget } from "../api/types";
import { buildCommand } from "../app/commandRequests";
import { formatTime } from "../app/observations";
import { useCommands } from "../app/useCommands";
import { EmptyPanel } from "./ChartPanel";

type SubmitCommand = (request: CommandRequest) => Promise<CommandRecord>;

export function ActionsView({ enabled, paused, reason }: { enabled: boolean; paused: boolean; reason?: string | null }) {
  const commands = useCommands(enabled, paused);
  const [selected, setSelected] = useState("");
  const target = commands.catalog?.targets.find((value) => value.target_id === selected) ?? commands.catalog?.targets[0];
  if (!enabled) return <EmptyPanel title="Session controls are not enabled">{reason ?? "This backend has no registered control sessions."} Observation views remain available.</EmptyPanel>;
  return <>
    <div className="section-intro"><div><p className="eyebrow">Actions</p><h2>Controlled sessions</h2><p>Manage the membership of an adapter session owned by this backend.</p></div><span className="panel-note">{commands.catalog?.targets.length ?? 0} registered sessions</span></div>
    {commands.error && <div className="error-banner" role="alert">{commands.error}</div>}
    {paused && <div className="notice-banner">Resume the live view before sending commands.</div>}
    {!commands.catalog ? <EmptyPanel title="Loading session controls">Waiting for the command catalog.</EmptyPanel> : !target ? <EmptyPanel title="No controlled sessions">An application must explicitly register a session before its actions become available.</EmptyPanel> : <>
      <div className="federate-layout command-layout">
        <aside className="panel federate-sidebar"><p className="eyebrow">Owned sessions</p><h2>Select a target</h2><div className="participant-links">{commands.catalog.targets.map((value) => <button key={value.target_id} type="button" className={target.target_id === value.target_id ? "selected" : ""} onClick={() => setSelected(value.target_id)}><span><strong>{value.label}</strong><small>{value.adapter}</small></span><span className="state-pill">{value.busy ? "busy" : value.state}</span></button>)}</div></aside>
        <div className="federate-content"><SessionActions key={`${target.target_id}:${target.version?.instance_id ?? "legacy"}`} target={target} paused={paused} stale={Boolean(commands.error)} submit={commands.submit} /></div>
      </div>
    </>}
    {commands.history && <CommandHistoryPanel history={commands.history} />}
  </>;
}

function SessionActions({ target, paused, stale, submit }: { target: CommandTarget; paused: boolean; stale: boolean; submit: SubmitCommand }) {
  const [action, setAction] = useState<CommandDefinition["action"]>("session.connect");
  const lifecycle = target.commands.filter((value) => value.action !== "session.inspect" && value.action !== "session.recover");
  const recovery = target.commands.filter((value) => value.action === "session.inspect" || value.action === "session.recover");
  const command = lifecycle.find((value) => value.action === action) ?? lifecycle[0];
  return <>
    <article aria-label="Controlled session state" className="panel"><div className="panel-heading"><div><p className="eyebrow">Session state</p><h2>{target.label}</h2></div><span className={`state-pill ${target.busy ? "partial" : target.state === "unknown" ? "unknown" : "available"}`}>{target.busy ? "busy" : target.state}</span></div>
      {target.synthetic && <div className="notice-banner">Synthetic showcase: these controls affect a separate demo session.</div>}
      {target.fault && <div className="error-banner" role="alert">{target.fault}</div>}
      <div className="context-grid"><div className="context-item"><label>Target</label><span>{target.target_id}</span></div><div className="context-item"><label>Federation {target.state === "unknown" ? "(last known)" : ""}</label><span>{target.federation ?? (target.state === "joined" || target.state === "unknown" ? "Not reported" : "Not joined")}</span></div><div className="context-item"><label>Federate {target.state === "unknown" ? "(last known)" : ""}</label><span>{target.federate ?? (target.state === "joined" || target.state === "unknown" ? "Not reported" : "Not joined")}</span></div><div className="context-item"><label>Authority</label><span>This session only</span></div></div>
      <p className="footnote owned-target-version">{target.version ? `Owned registration ${target.version.instance_id} · revision ${target.version.revision}. Submissions are conditional on this target version.` : "This older backend does not report conditional target versions. No version or recovery support is inferred."}</p>
    </article>
    <article aria-label="Session lifecycle actions" className="panel"><div className="panel-heading"><div><p className="eyebrow">Lifecycle</p><h2>Available actions</h2></div></div>
      <div className="action-choices">{lifecycle.map((value) => <button type="button" key={value.action} className={value.action === command?.action ? "selected" : ""} onClick={() => setAction(value.action)}><strong>{value.title}</strong><span className={`state-pill ${value.state}`}>{value.state}</span></button>)}</div>
      {command && <CommandForm key={`${target.target_id}-${command.action}`} target={target} command={command} paused={paused} stale={stale} submit={submit} />}
    </article>
    <article aria-label="Session recovery" className="panel"><div className="panel-heading"><div><p className="eyebrow">Inspect · review · reconcile</p><h2>Owned session recovery</h2></div></div>
      <p className="footnote">Inspection reads this owned driver. Reconciliation can restore controls only from complete current connection/membership facts and a healthy callback pump. It never retries a failed connect/join/resign/disconnect or uses the observed roster as authority.</p>
      {!recovery.length ? <p className="empty">This adapter does not offer session inspection/recovery. Existing lifecycle and observation features remain independent.</p> : recovery.map((definition) => <CommandForm key={definition.action} target={target} command={definition} paused={paused} stale={stale} submit={submit} />)}
      {target.inspection && <InspectionSummary target={target} />}
    </article>
  </>;
}

function InspectionSummary({ target }: { target: CommandTarget }) {
  const inspection = target.inspection;
  if (!inspection) return null;
  const title = inspection.reconciled ? target.state === "unknown"
    ? "Previous reconciliation; current state is unknown" : "Control state reconciled"
    : "Inspection only — permissions unchanged";
  return <section aria-label="Latest owned session inspection" className="session-inspection"><h3>{title}</h3>
    <p>{inspection.resolution}</p><p>Checked {formatTime(inspection.checked_at)} · connection: {inspection.observation.connection} · membership: {inspection.observation.membership}</p>
    <p>Inspected federation: {inspection.observation.federation ?? "not reported"} · federate: {inspection.observation.federate ?? "not reported"} · own handle: {inspection.observation.federate_handle ?? "not reported"}</p>
    {inspection.observation.reason && <p className="footnote">{inspection.observation.reason}</p>}
  </section>;
}

function CommandForm({ target, command, paused, stale, submit }: {
  target: CommandTarget; command: CommandDefinition; paused: boolean; stale: boolean; submit: SubmitCommand;
}) {
  const [fields, setFields] = useState<Record<string, string>>(() => Object.fromEntries(command.fields.map((field) => [field.name, field.default_value])));
  const [confirmed, setConfirmed] = useState(false);
  const [confirmationVersion, setConfirmationVersion] = useState("");
  const [attempt, setAttempt] = useState<CommandRequest>();
  const [submitting, setSubmitting] = useState(false);
  const [feedback, setFeedback] = useState<string>();
  const needsVersion = command.action === "session.inspect" || command.action === "session.recover";
  const available = command.state === "available" && !target.busy && !paused && !stale && (!needsVersion || Boolean(target.version));
  const currentVersion = target.version ? `${target.version.instance_id}:${target.version.revision}` : "legacy";
  const confirmationValid = confirmed && confirmationVersion === currentVersion;
  const onSubmit = async () => {
    setSubmitting(true);
    setFeedback(undefined);
    try {
      const request = attempt ?? buildCommand(command.action, target, fields, confirmationValid);
      setAttempt(request);
      const result = await submit(request);
      setFeedback(`Request ${result.request.request_id}: ${result.status}. Follow its execution in command history.`);
      setAttempt(undefined);
      setConfirmed(false);
    } catch (error) {
      setFeedback(error instanceof ObserverApiError ? error.message
        : "Delivery could not be confirmed. Retry uses the same request ID to avoid duplicate execution.");
    } finally {
      setSubmitting(false);
    }
  };
  return <form className="command-form" onSubmit={(event) => { event.preventDefault(); void onSubmit(); }}>
    <p className="footnote">{command.description}</p>
    {command.state !== "available" && <div className="notice-banner">{command.reason ?? `This action is ${command.state}.`}</div>}
    {needsVersion && !target.version && <div className="notice-banner">Recovery is unavailable because the backend did not provide an owned target version.</div>}
    {command.fields.map((field) => <label key={field.name}>{field.label}{field.input === "select"
      ? <select aria-label={field.label} disabled={submitting || paused} value={fields[field.name] ?? ""} required={field.required} onChange={(event) => { setFields({ ...fields, [field.name]: event.target.value }); setAttempt(undefined); }}>{field.choices.map((choice) => <option key={choice}>{choice}</option>)}</select>
      : <input aria-label={field.label} value={fields[field.name] ?? ""} maxLength={256} required={field.required} disabled={submitting || paused} onChange={(event) => { setFields({ ...fields, [field.name]: event.target.value }); setAttempt(undefined); }} />}</label>)}
    {command.requires_confirmation && <label className="check-control command-confirmation"><input type="checkbox" disabled={!available || submitting} checked={confirmationValid} onChange={(event) => { setConfirmed(event.target.checked); setConfirmationVersion(currentVersion); setAttempt(undefined); }} />I confirm {command.title.toLowerCase()} for {target.label}.</label>}
    <button className="primary-button" type="submit" disabled={!available || submitting || (command.requires_confirmation && !confirmationValid)}>{submitting ? "Submitting…" : attempt ? "Retry same request" : command.title}</button>
    {feedback && <div className="notice-banner" role="status">{feedback}</div>}
  </form>;
}

function CommandHistoryPanel({ history }: { history: CommandHistory }) {
  return <article aria-label="Command results" className="panel"><div className="panel-heading"><div><p className="eyebrow">Execution history</p><h2>Command results</h2></div><span className="panel-note">{history.records.length} retained · {history.evicted_records} expired</span></div>
    <p className="footnote">Accepted means queued; succeeded means the local service call returned. History survives a page refresh and lasts for this backend process.</p>
    <div className="table-scroll"><table className="events-table command-table"><thead><tr><th>Accepted</th><th>Target / action</th><th>State</th><th>Result</th></tr></thead><tbody>
      {history.records.map((record) => <tr key={record.command_id}><td>{formatTime(record.accepted_at)}<small title={record.command_id}>{record.command_id.slice(0, 8)}</small></td><td><strong>{record.request.target_id}</strong><small>{record.request.action}</small></td><td><span className={`state-pill command-status ${record.status}`}>{record.status}</span></td><td><strong>{record.outcome?.summary ?? record.error?.message ?? "Waiting for execution"}</strong>
        {record.error?.effect === "unknown" && <small className="unknown-effect">Effect unknown. Check the session state; timeout does not cancel a running provider call.</small>}
        {record.late_finished_at != null && <div className="late-result">Late result at {formatTime(record.late_finished_at)}: {record.late_outcome?.summary ?? record.late_error?.message}</div>}
        <details><summary>Request and result details</summary><pre>{JSON.stringify(record, null, 2)}</pre></details>
      </td></tr>)}
    </tbody></table>{!history.records.length && <div className="empty">No commands have been submitted in this backend session.</div>}</div>
  </article>;
}
