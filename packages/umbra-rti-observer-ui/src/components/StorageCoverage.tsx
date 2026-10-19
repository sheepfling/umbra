import type { StorageCoverage } from "../api/types";

const amount = (value: number) => `${(value / 1024).toLocaleString(undefined, { maximumFractionDigits: 1 })} KiB`;

export function StorageNotice({ data }: { data?: StorageCoverage | null }) {
  if (!data) return null;
  const losses = [...data.evidence.collections, ...data.context.collections].filter((row) => row.dropped_records > 0);
  if (!losses.length) return null;
  return <p className="coverage-note" role="status">Storage limits have expired some cached evidence. This is observation loss, not an object removal, federate resignation or feature becoming unsupported. Whole records expire; values are not clipped and global values are not substituted. See Diagnostics for per-collection coverage.</p>;
}

export function StorageDetails({ data, unavailableReason }: { data?: StorageCoverage | null; unavailableReason?: string }) {
  if (!data) return <p className="coverage-note">{unavailableReason ?? "Storage budgets and expiry reasons are not reported by this backend. Unknown is not unlimited or zero loss."}</p>;
  return <section className="storage-coverage" aria-label="Storage coverage">
    <div className="section-intro"><div><p className="eyebrow">Bounded observations · independent of RTI features</p><h2>Storage coverage</h2><p>Encoded JSON plus a per-record reserve, not process memory or complete federation membership. Counts in different collections may describe the same observation; do not add them into a unique event total.</p></div></div>
    {[{ name: "Evidence", data: data.evidence }, { name: "Context", data: data.context }].map((group) => <article className="panel" key={group.name} aria-label={`${group.name} storage budget`}>
      <div className="panel-heading"><h3>{group.name} budget</h3><span className="panel-note">{amount(group.data.accounted_bytes)} / {amount(group.data.max_bytes)}</span></div>
      <meter aria-label={`${group.name} encoded storage`} className="retention-meter" min={0} max={group.data.max_bytes} value={group.data.accounted_bytes} />
      <p className="footnote">Whole-record LRU with soft collection reservations · single-record maximum {amount(group.data.max_record_bytes)} · {group.data.record_reserve_bytes}-byte accounting reserve per record. Unused reservations consume no space and can be breached when whole records cannot fit. Expiry is not a new RTI incarnation; oversized/invalid JSON evidence is explicitly unretained.</p>
      <div className="table-scroll"><table className="events-table"><thead><tr><th>Collection / units</th><th>Retained / slots</th><th>Accounted</th><th>Count / byte expiry</th><th>Oversized / encoding / window expiry</th></tr></thead><tbody>{group.data.collections.map((row) => <tr key={row.collection}><td><strong>{row.collection}</strong><small>{row.unit}</small></td><td>{row.retained_records} / {row.capacity}</td><td>{amount(row.accounted_bytes)}</td><td>{row.count_limit_drops} / {row.byte_limit_drops}</td><td>{row.oversized_drops} / {row.encoding_drops} / {row.window_drops}</td></tr>)}</tbody></table></div>
      {group.data.unbudgeted_collections.length > 0 && <p className="coverage-note">Not covered by this byte budget: {group.data.unbudgeted_collections.join(", ")}. Their row bounds do not guarantee bounded payload bytes.</p>}
    </article>)}
  </section>;
}
