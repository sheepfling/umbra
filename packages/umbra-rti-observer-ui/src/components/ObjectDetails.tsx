import { useMemo, useState } from "react";
import { CartesianGrid, Line, LineChart, ResponsiveContainer, Tooltip, XAxis, YAxis } from "recharts";
import type { JsonValue, ObjectAttributeChange, ObjectPerspective } from "../api/types";
import { formatTime } from "../app/observations";
import { objectKey, objectPerspectiveKey, perspectiveDescription } from "../app/participants";
import { ChartPanel, chartColors, chartTick, chartTooltip, EmptyPanel } from "./ChartPanel";
import { useObjectChart } from "../workspace/WorkspaceContext";
import { RetainedSelect } from "../workspace/RetainedSelect";

function numeric(value: JsonValue | undefined): value is number {
  return typeof value === "number" && Number.isFinite(value);
}

function valueFormat(value: JsonValue): string {
  if (value !== null && typeof value === "object" && !Array.isArray(value)
    && value.encoding === "hex" && typeof value.value === "string" && /^(?:[0-9a-f]{2})*$/i.test(value.value)) {
    return `Undecoded bytes · ${value.value.length / 2} B · hex`;
  }
  return "Reported JSON · encoding not verified";
}

function Value({ value }: { value: JsonValue }) {
  return <code className="attribute-value">{JSON.stringify(value)}</code>;
}

export function ObjectDetails({ snapshot, changes }: { snapshot: ObjectPerspective; changes: ObjectAttributeChange[] }) {
  const [attribute, setAttribute] = useObjectChart(objectPerspectiveKey(snapshot), objectKey(snapshot.object));
  const [limit, setLimit] = useState(100);
  const row = snapshot.object;
  const history = useMemo(() => changes.filter((change) => objectKey(change) === objectKey(row)
    && objectPerspectiveKey(change) === objectPerspectiveKey(snapshot) && change.generation === snapshot.generation), [changes, row, snapshot]);
  const latest = useMemo(() => new Map(history.map((change) => [change.attribute, change])), [history]);
  const numericAttributes = useMemo(() => [...new Set([
    ...Object.entries(row.attributes ?? {}).filter(([, value]) => numeric(value)).map(([name]) => name),
    ...history.filter((change) => numeric(change.value)).map((change) => change.attribute),
  ])].sort(), [row.attributes, history]);
  const selected = attribute || numericAttributes[0] || "";
  const samples = useMemo(() => history.filter((change) => change.attribute === selected).slice(-500).map((change) => ({
    sequence: change.sequence, value: numeric(change.value) ? change.value : null,
  })), [history, selected]);
  const numericSamples = samples.filter((sample) => sample.value !== null).length;
  const comparisons = [...history].reverse();
  const attributes = Object.entries(row.attributes ?? {}).sort(([left], [right]) => left.localeCompare(right));
  return <>
    <article className="panel object-detail" aria-label="Selected object">
      <div className="panel-heading"><div><p className="eyebrow">{snapshot.federate === null ? "Latest global values" : `Attributed to ${snapshot.federate}`}</p><h2>{row.object_name}</h2></div><span className="state-pill">{row.status}</span></div>
      <p className="panel-note">{row.class_name ?? "Class not reported"} · key {row.object_key} · last observed {formatTime(row.last_seen)} · sequence #{snapshot.last_sequence}</p>
      <p className="coverage-note participant-provenance">{row.object_context ? `Object context: ${row.object_context.identity_namespace} / ${row.object_context.federation_id}` : "Object federation context unreported — legacy unscoped object aggregation"}. {snapshot.federate === null ? "Global observer projection, not a participant's local view." : perspectiveDescription(snapshot.perspective)}</p>
      <p className="footnote">Sources: {row.sources?.join(", ") || "Not reported"}. Observation generation #{snapshot.generation}; rediscovery after removal starts a new comparison baseline.</p>
      {attributes.length ? <div className="table-scroll"><table className="events-table attribute-table"><caption>Latest observed attributes</caption><thead><tr><th>Attribute</th><th>Latest value</th><th>Representation</th><th>Previous observed value</th></tr></thead><tbody>{attributes.map(([name, value]) => {
        const change = latest.get(name);
        return <tr key={name}><td><strong>{name}</strong></td><td><Value value={value} /></td><td>{valueFormat(value)}</td><td>{change ? change.previous_known ? <Value value={change.previous_value} /> : "Unknown — first observation" : "No retained comparison"}</td></tr>;
      })}</tbody></table></div> : <p className="empty">No attribute values have been reported in this perspective.</p>}
    </article>
    <div className="object-chart-control"><label>Numeric attribute<RetainedSelect aria-label="Numeric attribute" value={selected} disabled={!numericAttributes.length} onChange={(event) => setAttribute(event.target.value)}>{!numericAttributes.length && <option value="">None reported</option>}{numericAttributes.map((name) => <option key={name} value={name}>{name}</option>)}</RetainedSelect></label><p className="footnote">Finite JSON numbers only. Strings, booleans and opaque bytes are never converted into chart values. Saved attribute choices stay selected when temporarily unreported.</p></div>
    {numericSamples ? <ChartPanel title="Numeric attribute observations" eyebrow={selected} note={`${numericSamples} numeric samples · latest 500 observations maximum`}>
      <ResponsiveContainer width="100%" height="100%"><LineChart data={samples} margin={{ top: 8, right: 16, bottom: 8, left: 0 }}><CartesianGrid stroke="#1d3852" strokeDasharray="3 3" /><XAxis dataKey="sequence" tick={chartTick} label={{ value: "Observation sequence", position: "insideBottom", offset: -5, fill: "#8ea5b8" }} /><YAxis tick={chartTick} domain={["auto", "auto"]} /><Tooltip contentStyle={chartTooltip} labelFormatter={(value) => `Sequence #${value}`} /><Line dataKey="value" name={selected} stroke={chartColors[0]} dot={{ r: 3 }} isAnimationActive={false} connectNulls={false} /></LineChart></ResponsiveContainer>
    </ChartPanel> : <EmptyPanel title={history.some((change) => change.attribute === selected && numeric(change.value)) ? "Numeric samples outside chart limit" : "No retained numeric samples"}>Only the latest 500 attribute observations are charted. Choose an attribute with numeric observations in that range. A latest snapshot value alone does not reconstruct historical samples.</EmptyPanel>}
    <article className="panel" aria-label="Attribute comparisons"><div className="panel-heading"><div><p className="eyebrow">Current observation generation</p><h2>Retained attribute comparisons</h2></div><span className="panel-note">{comparisons.length} observations · newest first</span></div>
      {comparisons.length ? <><div className="table-scroll"><table className="events-table"><thead><tr><th>Sequence / observed</th><th>Attribute / source</th><th>Previous</th><th>Observed</th><th>Numeric change</th></tr></thead><tbody>{comparisons.slice(0, limit).map((change) => <tr key={JSON.stringify([change.sequence, change.attribute])}><td><code>#{change.sequence}</code><small>{formatTime(change.observed_at)}</small></td><td><strong>{change.attribute}</strong><small>{change.source}</small></td><td>{change.previous_known ? <Value value={change.previous_value} /> : "Unknown"}</td><td><Value value={change.value} /></td><td>{change.previous_known && numeric(change.previous_value) && numeric(change.value) ? finiteDelta(change.value, change.previous_value) : "—"}</td></tr>)}</tbody></table></div>{comparisons.length > limit && <button className="secondary-button load-more" type="button" onClick={() => setLimit((count) => count + 100)}>Show 100 more comparisons</button>}</> : <p className="empty">No attribute observations remain in this history window for this object and perspective.</p>}
    </article>
  </>;
}

function finiteDelta(value: number, previous: number): string {
  const delta = value - previous;
  return Number.isFinite(delta) ? `${delta > 0 ? "+" : ""}${Number(delta.toPrecision(8))}` : "Outside numeric range";
}
