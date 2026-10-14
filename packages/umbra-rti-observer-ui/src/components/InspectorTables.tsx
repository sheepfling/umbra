import type { StateResponse } from "../api/types";

interface InspectorTablesProps {
  state: StateResponse;
}

export function InspectorTables({ state }: InspectorTablesProps) {
  return (
    <section className="inspector-grid">
      <TablePanel title="Federates" note="participants observed by the backend">
        <TableHead columns={["Federate", "Role", "Status", "Source"]} />
        {state.federate_roster.map((row) => (
          <div className="table-row" key={`${row.federate}-${row.source}`}>
            <span>{row.federate}</span><span>{row.role ?? "—"}</span><span>{row.status}</span><span>{row.source}</span>
          </div>
        ))}
      </TablePanel>
      <TablePanel title="Objects" note="derived from normalized object events">
        <TableHead columns={["Object", "Class", "Family", "Updates", "Status"]} />
        {state.inspectors.objects.map((row) => (
          <div className="table-row" key={row.object_key}>
            <span>{row.object_name}</span><span>{row.class_name ?? "—"}</span><span>{row.family}</span><span>{row.update_count}</span><span>{row.status}</span>
          </div>
        ))}
      </TablePanel>
      <TablePanel title="Interactions" note="latest interaction observations">
        <TableHead columns={["Key", "Class", "Source", "Federate"]} />
        {state.inspectors.interactions.map((row) => (
          <div className="table-row" key={`${row.sequence}-${row.interaction_key}`}>
            <span>{row.interaction_key}</span><span>{row.interaction_class}</span><span>{row.source}</span><span>{row.federate ?? "—"}</span>
          </div>
        ))}
      </TablePanel>
    </section>
  );
}

function TablePanel({ title, note, children }: { title: string; note: string; children: React.ReactNode }) {
  return (
    <article className="panel table-panel">
      <div className="panel-heading"><div><p className="eyebrow">Inspector</p><h2>{title}</h2></div><span className="panel-note">{note}</span></div>
      <div className="table-scroll"><div className="table-body">{children}</div></div>
    </article>
  );
}

function TableHead({ columns }: { columns: string[] }) {
  return <div className="table-row table-head">{columns.map((column) => <span key={column}>{column}</span>)}</div>;
}
