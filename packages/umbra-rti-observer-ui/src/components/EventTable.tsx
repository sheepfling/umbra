import { useMemo, useState } from "react";
import type { EventEnvelope } from "../api/types";
import { eventFederate, exportEvents, formatTime } from "../app/observations";

export function EventTable({ events, title = "Event explorer" }: { events: EventEnvelope[]; title?: string }) {
  const [search, setSearch] = useState("");
  const [family, setFamily] = useState("");
  const [type, setType] = useState("");
  const [source, setSource] = useState("");
  const [federate, setFederate] = useState("");
  const [limit, setLimit] = useState(100);
  const families = useMemo(() => [...new Set(events.map((event) => event.family))].sort(), [events]);
  const types = useMemo(() => [...new Set(events.map((event) => event.event_type))].sort(), [events]);
  const sources = useMemo(() => [...new Set(events.map((event) => event.source))].sort(), [events]);
  const federates = useMemo(() => [...new Set(events.map(eventFederate).filter((name): name is string => Boolean(name)))].sort(), [events]);
  const rows = useMemo(() => {
    const needle = search.trim().toLowerCase();
    return events.filter((event) => (!family || event.family === family)
      && (!type || event.event_type === type) && (!source || event.source === source)
      && (!federate || eventFederate(event) === federate)
      && (!needle || JSON.stringify(event).toLowerCase().includes(needle))).slice().reverse();
  }, [events, family, search, type, source, federate]);

  return <section aria-label={title} className="panel events-panel">
    <div className="panel-heading"><div><p className="eyebrow">Investigate</p><h2>{title}</h2></div><div className="toolbar"><span className="panel-note">{rows.length} matching / {events.length} loaded</span><button type="button" className="secondary-button" disabled={!rows.length} onClick={() => exportEvents(rows.slice().reverse())}>Export JSON</button></div></div>
    <div className="filters"><input aria-label="Search events" placeholder="Search payload, operation, or object" value={search} onChange={(event) => setSearch(event.target.value)} /><Filter label="family" value={family} options={families} onChange={setFamily} /><Filter label="type" value={type} options={types} onChange={setType} /><Filter label="source" value={source} options={sources} onChange={setSource} /><Filter label="federate" value={federate} options={federates} onChange={setFederate} /></div>
    <div className="table-scroll event-scroll"><table className="events-table"><thead><tr><th>Sequence</th><th>Observed</th><th>Event / operation</th><th>Federate / source</th><th>Payload</th></tr></thead><tbody>
      {rows.slice(0, limit).map((event) => <tr key={event.sequence}><td><code>#{event.sequence}</code></td><td>{formatTime(event.observed_at)}</td><td><strong>{event.event_type}</strong><small>{event.operation ?? ""}</small></td><td><strong>{eventFederate(event) ?? "Unattributed"}</strong><small>{event.source}</small></td><td><details><summary>View payload</summary><pre>{JSON.stringify(event, null, 2)}</pre></details></td></tr>)}
    </tbody></table>{!rows.length && <div className="empty">No events match the current filters.</div>}</div>
    {rows.length > limit && <button type="button" className="secondary-button load-more" onClick={() => setLimit((value) => value + 100)}>Show 100 more</button>}
  </section>;
}

function Filter({ label, value, options, onChange }: { label: string; value: string; options: string[]; onChange: (value: string) => void }) {
  return <select aria-label={`Filter by ${label}`} value={value} onChange={(event) => onChange(event.target.value)}><option value="">All {label === "family" ? "families" : `${label}s`}</option>{options.map((option) => <option key={option}>{option}</option>)}</select>;
}
