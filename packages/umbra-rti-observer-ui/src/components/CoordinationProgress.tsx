import type { CoordinationSnapshot } from "../api/types";
import { useState } from "react";
import { formatTime } from "../app/observations";

function subject(row: CoordinationSnapshot): string {
  return row.scope === "federation" ? "Federation report" : row.subject
    ? `${row.subject.display_name ?? row.subject.participant_id} / ${row.subject.membership_id}`
    : "Unreported local subject";
}

/** Each scope remains a separate report; participant lists are never a census. */
export function CoordinationProgress({ rows }: { rows: CoordinationSnapshot[] }) {
  const [scopeLimit, setScopeLimit] = useState(20);
  const ordered = [...rows].sort((a, b) => b.sequence - a.sequence);
  const latest = ordered[0];
  if (!latest) return null;
  return <article className="panel" aria-label="Selected coordination operation">
    <div className="panel-heading"><div><p className="eyebrow">Operation drill-down</p><h2>{latest.label}</h2></div><span className="state-pill">{latest.kind}</span></div>
    <p className="coverage-note">Each card is a last reported scope. A completed local card does not complete a federation card. Participant lists can be partial or unknown.</p>
    <p className="coverage-note">{ordered.length} retained scoped reports; showing {Math.min(scopeLimit, ordered.length)}.</p>
    <div className="coordination-scope-grid">{ordered.slice(0, scopeLimit).map((row) => <section className="coordination-scope-card" key={JSON.stringify([row.source, row.session_id, row.scope, row.subject?.participant_id, row.subject?.membership_id])}>
      <p className="eyebrow">{row.scope}</p><h3>{subject(row)}</h3><strong className={`state-pill ${row.phase === "failed" || row.phase === "registration-failed" ? "unsupported" : row.phase === "completed" ? "available" : "partial"}`}>{row.phase}</strong>
      <p>Report #{row.sequence} at {formatTime(row.observed_at)}</p>{row.reason && <p>{row.reason}</p>}
      <ParticipantProgress row={row} />
    </section>)}</div>
    {ordered.length > scopeLimit && <button type="button" className="secondary-button load-more" onClick={() => setScopeLimit((value) => value + 20)}>Show 20 more scopes</button>}
  </article>;
}

function ParticipantProgress({ row }: { row: CoordinationSnapshot }) {
  const [limit, setLimit] = useState(25);
  if (row.participants == null) return <p>Participant progress unknown - not supplied in this report.</p>;
  if (!row.participants.length) return <p>Explicit empty participant report.</p>;
  return <><p>{row.participants.length} reported participants; showing {Math.min(limit, row.participants.length)}.</p>
    <ul className="coordination-participant-grid">{row.participants.slice(0, limit).map((item) => <li key={JSON.stringify([item.identity.participant_id, item.identity.membership_id])}><strong>{item.identity.display_name ?? item.identity.participant_id}</strong><span className="state-pill">{item.status}</span><code>{item.identity.participant_id} / {item.identity.membership_id}</code>{item.reason && <small>{item.reason}</small>}</li>)}</ul>
    {row.participants.length > limit && <button type="button" className="secondary-button" onClick={() => setLimit((value) => value + 25)}>Show 25 more participant reports</button>}
  </>;
}
