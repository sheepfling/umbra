import { useMemo, useState } from "react";
import type { EventEnvelope } from "../api/types";

export function EventTable({ events }: { events: EventEnvelope[] }) {
  const [search, setSearch] = useState("");
  const [family, setFamily] = useState("all");
  const [type, setType] = useState("all");
  const families = useMemo(() => [...new Set(events.map((event) => event.family))].sort(), [events]);
  const types = useMemo(() => [...new Set(events.map((event) => event.event_type))].sort(), [events]);
  const rows = useMemo(() => {
    const needle = search.trim().toLowerCase();
    return events.filter((event) => {
      const haystack = JSON.stringify(event).toLowerCase();
      return (family === "all" || event.family === family)
        && (type === "all" || event.event_type === type)
        && (!needle || haystack.includes(needle));
    }).reverse();
  }, [events, family, search, type]);

  return (
    <section className="panel events-panel">
      <div className="panel-heading"><div><p className="eyebrow">History</p><h2>Normalized events</h2></div><span className="panel-note">{rows.length} shown</span></div>
      <div className="filters">
        <input aria-label="Search events" placeholder="Search event payloads" value={search} onChange={(event) => setSearch(event.target.value)} />
        <select aria-label="Filter by family" value={family} onChange={(event) => setFamily(event.target.value)}><option value="all">All families</option>{families.map((value) => <option key={value}>{value}</option>)}</select>
        <select aria-label="Filter by type" value={type} onChange={(event) => setType(event.target.value)}><option value="all">All event types</option>{types.map((value) => <option key={value}>{value}</option>)}</select>
      </div>
      <div className="table-scroll event-scroll"><div className="table-body">
        <div className="table-row table-head"><span>Sequence</span><span>Type</span><span>Source</span><span>Details</span></div>
        {rows.length ? rows.map((event) => <div className="table-row event-row" key={event.sequence}><span>{event.sequence}</span><span>{event.event_type}</span><span>{event.source}</span><code>{JSON.stringify(event.details ?? event.attributes ?? event.parameters ?? {})}</code></div>) : <div className="empty">No events match the current filters.</div>}
      </div></div>
    </section>
  );
}
