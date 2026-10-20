import type { StateResponse } from "../api/types";
import { objectKey, participantKey, perspectiveDescription } from "../app/participants";

interface InspectorTablesProps {
  state: StateResponse;
}

export function InspectorTables({ state }: InspectorTablesProps) {
  return (
    <section className="inspector-grid">
      <TablePanel title="Federates" note="participants observed by the backend" empty={!state.federate_roster.length}>
        <TableHead columns={["Federate", "Role", "Status", "Source"]} />
        {state.federate_roster.map((row) => (
          <div className="table-row" key={participantKey(row)}>
            <span>{row.federate}<small className="participant-provenance">{perspectiveDescription(row.perspective)}</small></span><span>{row.role ?? "—"}</span><span>{row.status}</span><span>{row.source}</span>
          </div>
        ))}
      </TablePanel>
      <TablePanel title="Objects" note="derived from normalized object events" empty={!state.inspectors.objects.length}>
        <TableHead columns={["Object", "Class", "Family", "Updates", "Status"]} />
        {state.inspectors.objects.map((row) => (
          <div className="table-row five-columns" key={objectKey(row)}>
            <span>{row.object_name}<small className="participant-provenance">{row.object_context ? `${row.object_context.identity_namespace} / ${row.object_context.federation_id}` : "Object context unreported"}</small></span><span>{row.class_name ?? "—"}</span><span>{row.family}</span><span>{row.update_count}</span><span>{row.status}</span>
          </div>
        ))}
      </TablePanel>
      <TablePanel title="Interactions" note="latest interaction observations" empty={!state.inspectors.interactions.length}>
        <TableHead columns={["Key", "Class", "Source", "Federate"]} />
        {state.inspectors.interactions.map((row) => (
          <div className="table-row" key={`${row.sequence}-${row.interaction_key}`}>
            <span>{row.interaction_key}</span><span>{row.interaction_class}</span><span>{row.source}</span><span>{row.federate ?? "—"}<small className="participant-provenance">{perspectiveDescription(row.perspective)}</small></span>
          </div>
        ))}
      </TablePanel>
    </section>
  );
}

function TablePanel({ title, note, empty, children }: { title: string; note: string; empty: boolean; children: React.ReactNode }) {
  return (
    <article aria-label={title} className="panel table-panel">
      <div className="panel-heading"><div><p className="eyebrow">Inspector</p><h2>{title}</h2></div><span className="panel-note">{note}</span></div>
      <div className="table-scroll"><div className="table-body">{children}</div></div>
      {empty && <p className="empty">No {title.toLowerCase()} observed.</p>}
    </article>
  );
}

function TableHead({ columns }: { columns: string[] }) {
  return <div className={`table-row table-head ${columns.length === 5 ? "five-columns" : ""}`}>{columns.map((column) => <span key={column}>{column}</span>)}</div>;
}
