import { useEffect, useState } from "react";
import type { FomField, FomReport, StateResponse } from "../api/types";
import { formatTime } from "../app/observations";
import { EmptyPanel } from "./ChartPanel";
import { MetricCard } from "./MetricCard";

type ClassReport = Extract<FomReport, { kind: "object-class" | "interaction-class" }>;
function modelKey(row: FomReport): string { return JSON.stringify([row.source, row.session_id, row.identity_namespace, row.federation_id, row.model_id, row.revision]); }
function elementKey(row: FomReport): string { return JSON.stringify([modelKey(row), row.kind, row.element_id]); }
function label(row: FomReport): string { return row.name ?? row.element_id; }
function isClass(row: FomReport): row is ClassReport { return row.kind === "object-class" || row.kind === "interaction-class"; }
function fields(row: FomReport): FomField[] | null | undefined { return row.kind === "object-class" ? row.attributes : row.kind === "interaction-class" ? row.parameters : row.kind === "datatype" ? row.fields : undefined; }
function searchText(row: FomReport): string {
  return [row.name, row.element_id, row.model_id, row.revision, row.source, row.session_id, row.identity_namespace, row.federation_id, row.description,
    ...(fields(row) ?? []).flatMap((field) => [field.field_id, field.name, field.datatype_id, field.description]),
    ...(row.kind === "datatype" ? [row.encoding, row.representation, row.element_type_id, ...(row.enumerators ?? []).flatMap((item) => [item.name, item.value])] : row.kind === "dimension" ? [row.upper_bound, row.unit] : [row.parent_id])].join(" ").toLocaleLowerCase();
}

export function FomView({ state }: { state: StateResponse }) {
  const [search, setSearch] = useState("");
  const [model, setModel] = useState("all");
  const [kind, setKind] = useState("all");
  const [status, setStatus] = useState("all");
  const [selected, setSelected] = useState<string>();
  const [limit, setLimit] = useState(100);
  const [fieldLimit, setFieldLimit] = useState(100);
  const [historyLimit, setHistoryLimit] = useState(100);
  const capability = state.capabilities.fom_inspection;
  const usable = capability?.state === "available" || capability?.state === "partial";
  const data = usable ? state.inspectors.fom : undefined;
  const snapshots = data?.snapshots ?? [];
  const models = new Map(snapshots.map((row) => [modelKey(row), row]));
  const elements = new Map(snapshots.map((row) => [elementKey(row), row]));
  const lookup = (row: FomReport, targetKind: FomReport["kind"], id: string) => elements.get(JSON.stringify([modelKey(row), targetKind, id]));
  const filtered = snapshots.filter((row) => (model === "all" || modelKey(row) === model) && (kind === "all" || row.kind === kind)
    && (status === "all" || row.status === status) && searchText(row).includes(search.trim().toLocaleLowerCase())).sort((a, b) => b.sequence - a.sequence);
  const current = elements.get(selected ?? "");
  const currentFields = current ? fields(current) : undefined;
  const history = (data?.transitions ?? []).filter((row) => elementKey(row) === selected);
  useEffect(() => { if (selected === undefined && snapshots.length) setSelected(elementKey(snapshots.find((row) => row.kind === "object-class" && row.attributes?.length) ?? snapshots[0]!)); }, [snapshots, selected]);
  const focus = (row: FomReport) => { setSelected(elementKey(row)); setFieldLimit(100); setHistoryLimit(100); };
  const typeLink = (row: FomReport, id: string | null | undefined) => {
    if (id == null) return <span>Datatype unreported</span>;
    const target = lookup(row, "datatype", id);
    return target ? <button className="metadata-link" type="button" onClick={() => focus(target)}>Inspect datatype {label(target)} · {target.status}</button> : <span>{id} · datatype not reported in this model revision</span>;
  };
  let ancestors: ClassReport[] = [], hierarchyNotice = "";
  if (current && isClass(current)) {
    ancestors = [current];
    const seen = new Set([elementKey(current)]);
    let cursor = current;
    for (let depth = 0; depth < 64; depth++) {
      if (cursor.parent_state !== "parent") { if (cursor.parent_state === "unknown") hierarchyNotice = "Ancestry unknown — no root is inferred."; break; }
      const parent = cursor.parent_id ? lookup(cursor, cursor.kind, cursor.parent_id) : undefined;
      if (!parent || !isClass(parent)) { hierarchyNotice = `Parent ${cursor.parent_id ?? "unknown"} is not reported in this exact model revision.`; break; }
      if (seen.has(elementKey(parent))) { hierarchyNotice = "Cyclic parent references reported — hierarchy traversal stopped."; break; }
      seen.add(elementKey(parent)); ancestors.unshift(parent); cursor = parent;
      if (depth === 63) hierarchyNotice = "Hierarchy exceeds the 64-reference inspection limit; traversal stopped.";
    }
  }
  const children = current && isClass(current) ? snapshots.filter((row): row is ClassReport => isClass(row) && row.kind === current.kind && modelKey(row) === modelKey(current) && row.parent_state === "parent" && row.parent_id === current.element_id) : [];

  if (!data) return <EmptyPanel title="FOM inspection unavailable">{capability?.reason ?? "This backend does not advertise adapter-supplied FOM metadata."} Runtime attributes, time and session views remain independent. Class names never substitute for metadata, and this browser does not parse provider FOM files.</EmptyPanel>;
  return <section className="fom-workspace" aria-label="FOM metadata workspace">
    <div className="section-intro"><div><p className="eyebrow">Adapter-supplied schema · explicit model revisions</p><h2>FOM metadata browser</h2><p>Browse reported classes, attributes, parameters, datatypes and dimensions. References resolve only within the same reporting scope, model and revision.</p></div><span className="state-pill">{capability?.state}</span></div>
    {capability?.state === "partial" && <p className="coverage-note">Partial FOM coverage: {capability.reason ?? (capability.reasons.join("; ") || "Some elements or references are not reported.")}</p>}
    {(data.dropped_snapshots > 0 || data.dropped_transitions > 0) && <p className="coverage-note" role="status">FOM evidence is incomplete: {data.dropped_snapshots} metadata elements and {data.dropped_transitions} arrivals expired. Missing metadata is not an absent class, field or datatype.</p>}
    <div className="panel object-controls fom-controls"><label>Search metadata<input aria-label="Search FOM metadata" value={search} placeholder="Class, attribute, parameter, type, ID or description" onChange={(event) => { setSearch(event.target.value); setLimit(100); }} /></label><label>Model revision<select aria-label="FOM model revision" value={model} onChange={(event) => { setModel(event.target.value); setLimit(100); }}><option value="all">All model revisions — separate records</option>{model !== "all" && !models.has(model) && <option value={model}>Selected model revision no longer reported</option>}{[...models].map(([key, row]) => <option value={key} key={key}>{row.model_id} / {row.revision} · {row.source} / {row.session_id} / {row.identity_namespace} / {row.federation_id}</option>)}</select></label><label>Element kind<select aria-label="FOM element kind" value={kind} onChange={(event) => { setKind(event.target.value); setLimit(100); }}><option value="all">All metadata kinds</option>{["object-class", "interaction-class", "datatype", "dimension"].map((value) => <option key={value}>{value}</option>)}</select></label><label>Reported status<select aria-label="FOM element status" value={status} onChange={(event) => { setStatus(event.target.value); setLimit(100); }}><option value="all">All reported statuses</option>{["present", "removed", "unknown"].map((value) => <option key={value}>{value}</option>)}</select></label></div>
    <section className="metrics declaration-metrics" aria-label="Filtered FOM metadata metrics"><MetricCard label="Reported classes" value={filtered.filter(isClass).length} /><MetricCard label="Reported datatypes" value={filtered.filter((row) => row.kind === "datatype").length} tone="violet" /><MetricCard label="Reported dimensions" value={filtered.filter((row) => row.kind === "dimension").length} tone="blue" /></section>
    <article className="panel" aria-label="Reported FOM elements"><div className="panel-heading"><div><p className="eyebrow">Latest supplied metadata · not runtime decoding</p><h2>Reported FOM elements</h2></div><span className="panel-note">{filtered.length} matching elements · newest report first</span></div>{!filtered.length ? <p className="empty">No supplied metadata matches this view. Runtime class names do not create a schema.</p> : <><div className="table-scroll"><table className="events-table fom-table"><thead><tr><th>Element / kind</th><th>Reported structure</th><th>Model / status</th><th>Provenance</th></tr></thead><tbody>{filtered.slice(0, limit).map((row) => <tr key={elementKey(row)}><td><strong>{label(row)}</strong><code>{row.element_id}</code><small>{row.kind}</small><button className="secondary-button" type="button" onClick={() => focus(row)}>Inspect metadata</button></td><td>{isClass(row) ? <><strong>Parent: {row.parent_state === "root" ? "explicit root" : row.parent_state === "unknown" ? "unknown" : row.parent_id}</strong><small>{row.field_scope} field scope · {fields(row) == null ? "field list unknown" : `${fields(row)!.length} supplied fields`}</small></> : row.kind === "datatype" ? <><strong>{row.encoding}</strong><small>{row.representation ?? "Representation unreported"}</small></> : <><strong>Upper bound: {row.upper_bound ?? "unknown"}</strong><small>Unit: {row.unit ?? "unknown"}</small></>}</td><td><strong>{row.model_id} / {row.revision}</strong><span className="state-pill">{row.status}</span></td><td><strong>{row.source}</strong><small>{row.session_id} / {row.identity_namespace} / {row.federation_id}</small><code>#{row.sequence}</code><small>{formatTime(row.observed_at)}</small></td></tr>)}</tbody></table></div>{filtered.length > limit && <button className="secondary-button load-more" type="button" onClick={() => setLimit((count) => count + 100)}>Show 100 more FOM elements</button>}</>}
    </article>
    <article className="panel" aria-label="Selected FOM metadata"><div className="panel-heading"><div><p className="eyebrow">Schema navigation · selection independent of table filters</p><h2>Selected FOM metadata</h2></div></div><label className="region-selection">Element<select aria-label="Selected FOM element" value={selected ?? ""} onChange={(event) => { setSelected(event.target.value); setFieldLimit(100); setHistoryLimit(100); }}><option value="">Choose supplied metadata</option>{selected && !current && <option value={selected}>Selected element no longer reported</option>}{snapshots.map((row) => <option value={elementKey(row)} key={elementKey(row)}>{row.kind} · {label(row)} · {row.model_id} / {row.revision} · {row.source} / {row.session_id} / {row.identity_namespace} / {row.federation_id}</option>)}</select></label>
      {!current ? <p className="empty">No latest metadata for this selection. Expired or cleared elements are not replaced with another revision or an invented schema.</p> : <div className="fom-detail"><h3>{label(current)} · {current.kind}</h3><p>{current.model_id} / {current.revision} · <span className="state-pill">{current.status}</span></p><p>{current.source} / {current.session_id} / {current.identity_namespace} / {current.federation_id}</p>{current.name == null && <p>Name unreported; the explicit element ID is used as a display fallback only.</p>}{current.description && <p>{current.description}</p>}
        {isClass(current) && <><h3>Reported class hierarchy</h3><ol className="fom-breadcrumbs">{ancestors.map((row) => <li key={elementKey(row)}><button className="metadata-link" type="button" onClick={() => focus(row)}>Inspect class {label(row)} · {row.status}</button></li>)}</ol>{hierarchyNotice && <p className="coverage-note" role="status">{hierarchyNotice}</p>}<p>Field scope: {current.field_scope}; inherited/effective fields are not synthesized from parents.</p>{children.length > 0 && <div className="fom-children"><h4>Explicit child references ({children.length})</h4>{children.slice(0, 100).map((row) => <button className="metadata-link" key={elementKey(row)} type="button" onClick={() => focus(row)}>Inspect child {label(row)} · {row.status}</button>)}{children.length > 100 && <p>Showing 100 child links; use the complete paged element table for remaining metadata.</p>}</div>}</>}
        {current.kind === "datatype" && <><p>Encoding: {current.encoding} · representation: {current.representation ?? "unreported"}</p>{current.encoding === "array" && <><p>Element type: {typeLink(current, current.element_type_id)}</p><p>Fixed cardinality: {current.cardinality ?? "unknown — variable/fixed is not inferred"}</p></>}{current.encoding === "enumeration" && (current.enumerators == null ? <p>Enumerators unknown</p> : !current.enumerators.length ? <p>Explicit empty enumerator list</p> : <div className="table-scroll"><table className="events-table"><thead><tr><th>Enumerator</th><th>Reported value text</th></tr></thead><tbody>{current.enumerators.slice(0, fieldLimit).map((item) => <tr key={item.name}><td>{item.name}</td><td><code>{item.value}</code></td></tr>)}</tbody></table>{current.enumerators.length > fieldLimit && <button className="secondary-button load-more" type="button" onClick={() => setFieldLimit((count) => count + 100)}>Show 100 more enumerators</button>}</div>)}</>}
        {current.kind === "dimension" && <><p>Exact upper bound: <code>{current.upper_bound ?? "unknown"}</code></p><p>Unit: {current.unit ?? "unknown"}</p></>}
        {(isClass(current) || current.kind === "datatype" && ["record", "variant"].includes(current.encoding)) && <><h3>{current.kind === "object-class" ? "Attributes" : current.kind === "interaction-class" ? "Parameters" : "Datatype fields"}</h3>{currentFields == null ? <p>Field list unknown — not supplied</p> : !currentFields.length ? <p>Explicit empty field list; no inherited fields are assumed.</p> : <><div className="table-scroll"><table className="events-table"><thead><tr><th>Field / name</th><th>Datatype reference</th><th>Description</th></tr></thead><tbody>{currentFields.slice(0, fieldLimit).map((field) => <tr key={field.field_id}><td><strong>{field.name ?? "Name unreported"}</strong><code>{field.field_id}</code></td><td>{typeLink(current, field.datatype_id)}</td><td>{field.description ?? "Unreported"}</td></tr>)}</tbody></table></div>{currentFields.length > fieldLimit && <button className="secondary-button load-more" type="button" onClick={() => setFieldLimit((count) => count + 100)}>Show 100 more metadata fields</button>}</>}</>}
        <details><summary>Complete selected FOM report</summary><pre>{JSON.stringify(current, null, 2)}</pre></details>
      </div>}
      <h3>Retained metadata sequence</h3>{!history.length ? <p>No retained reports for this exact metadata element.</p> : <><ol className="coordination-timeline">{history.slice(-historyLimit).map((row) => <li key={row.sequence}><div><code>#{row.sequence}</code><small>{formatTime(row.observed_at)}</small></div><div><strong>{label(row)} · {row.status}</strong><small>{row.kind} · {row.model_id} / {row.revision} · {row.source} / {row.session_id}</small></div></li>)}</ol>{history.length > historyLimit && <button className="secondary-button load-more" type="button" onClick={() => setHistoryLimit((count) => count + 100)}>Show 100 earlier metadata reports</button>}</>}
    </article>
    <p className="coverage-note">Only adapter-supplied metadata is shown. No FOM file is parsed and no runtime bytes are decoded. Root/unknown ancestry and declared/effective/unknown field scope stay distinct. References never resolve across source/session/namespace/federation/model/revision or class kinds. Cycles and missing references are visible, not recursively guessed. Present/removed status is a report, not proof of a currently loaded federation schema. Clear/restart resets the bounded metadata indexes.</p>
  </section>;
}
