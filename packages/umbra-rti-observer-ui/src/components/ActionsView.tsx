import { useState } from "react";
import { ObserverApiError } from "../api/client";
import type { CommandDefinition, CommandHistory, CommandRecord, CommandRequest, CommandTarget, ResignPolicy } from "../api/types";
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
        <div className="federate-content"><SessionActions key={target.target_id} target={target} paused={paused} stale={Boolean(commands.error)} submit={commands.submit} /></div>
      </div>
    </>}
    {commands.history && <CommandHistoryPanel history={commands.history} />}
  </>;
}

function SessionActions({ target, paused, stale, submit }: { target: CommandTarget; paused: boolean; stale: boolean; submit: SubmitCommand }) {
  const [action, setAction] = useState<CommandDefinition["action"]>("session.connect");
  const command = target.commands.find((value) => value.action === action) ?? target.commands[0];
  return <>
    <article aria-label="Controlled session state" className="panel"><div className="panel-heading"><div><p className="eyebrow">Session state</p><h2>{target.label}</h2></div><span className={`state-pill ${target.busy ? "partial" : "available"}`}>{target.busy ? "busy" : target.state}</span></div>
      {target.synthetic && <div className="notice-banner">Synthetic showcase: these controls affect a separate demo session.</div>}
      {target.fault && <div className="error-banner" role="alert">{target.fault}</div>}
      <div className="context-grid"><div className="context-item"><label>Target</label><span>{target.target_id}</span></div><div className="context-item"><label>Federation</label><span>{target.federation ?? "Not joined"}</span></div><div className="context-item"><label>Federate</label><span>{target.federate ?? "Not joined"}</span></div><div className="context-item"><label>Authority</label><span>This session only</span></div></div>
    </article>
    <article aria-label="Session lifecycle actions" className="panel"><div className="panel-heading"><div><p className="eyebrow">Lifecycle</p><h2>Available actions</h2></div></div>
      <div className="action-choices">{target.commands.map((value) => <button type="button" key={value.action} className={value.action === command?.action ? "selected" : ""} onClick={() => setAction(value.action)}><strong>{value.title}</strong><span className={`state-pill ${value.state}`}>{value.state}</span></button>)}</div>
      {command && <CommandForm key={`${target.target_id}-${command.action}`} target={target} command={command} paused={paused} stale={stale} submit={submit} />}
    </article>
  </>;
}

function CommandForm({ target, command, paused, stale, submit }: {
  target: CommandTarget; command: CommandDefinition; paused: boolean; stale: boolean; submit: SubmitCommand;
}) {
  const [fields, setFields] = useState<Record<string, string>>(() => Object.fromEntries(command.fields.map((field) => [field.name, field.default_value])));
  const [confirmed, setConfirmed] = useState(false);
  const [attempt, setAttempt] = useState<CommandRequest>();
  const [submitting, setSubmitting] = useState(false);
  const [feedback, setFeedback] = useState<string>();
  const available = command.state === "available" && !target.busy && !paused && !stale;
  const onSubmit = async () => {
    setSubmitting(true);
    setFeedback(undefined);
    try {
      const request = attempt ?? buildCommand(command.action, target.target_id, fields, confirmed);
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
    {command.fields.map((field) => <label key={field.name}>{field.label}{field.input === "select"
      ? <select aria-label={field.label} disabled={submitting || paused} value={fields[field.name] ?? ""} required={field.required} onChange={(event) => { setFields({ ...fields, [field.name]: event.target.value }); setAttempt(undefined); }}>{field.choices.map((choice) => <option key={choice}>{choice}</option>)}</select>
      : <input aria-label={field.label} value={fields[field.name] ?? ""} maxLength={256} required={field.required} disabled={submitting || paused} onChange={(event) => { setFields({ ...fields, [field.name]: event.target.value }); setAttempt(undefined); }} />}</label>)}
    {command.requires_confirmation && <label className="check-control command-confirmation"><input type="checkbox" disabled={!available || submitting} checked={confirmed} onChange={(event) => { setConfirmed(event.target.checked); setAttempt(undefined); }} />I confirm {command.title.toLowerCase()} for {target.label}.</label>}
    <button className="primary-button" type="submit" disabled={!available || submitting || (command.requires_confirmation && !confirmed)}>{submitting ? "Submitting…" : attempt ? "Retry same request" : command.title}</button>
    {feedback && <div className="notice-banner" role="status">{feedback}</div>}
  </form>;
}

function buildCommand(action: CommandDefinition["action"], targetId: string, fields: Record<string, string>, confirmed: boolean): CommandRequest {
  const base = { request_id: crypto.randomUUID(), target_id: targetId, confirmed };
  switch (action) {
    case "session.connect": return { ...base, action, parameters: {} };
    case "session.disconnect": return { ...base, action, parameters: {} };
    case "federate.join": return { ...base, action, parameters: { federation: fields.federation ?? "", federate_name: fields.federate_name ?? "", federate_type: fields.federate_type ?? "Observer" } };
    case "federate.resign": return { ...base, action, parameters: { policy: resignPolicy(fields.policy ?? "no_action") } };
  }
}

function resignPolicy(value: string): ResignPolicy {
  if (value === "no_action" || value === "unconditionally_divest_attributes" || value === "delete_objects"
    || value === "cancel_pending_ownership_acquisitions" || value === "delete_objects_then_divest"
    || value === "cancel_then_delete_then_divest") return value;
  throw new Error("Select a supported resign policy.");
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
