import type { ObjectObservations } from "../api/types";

export function ObjectCoverage({ data }: { data?: ObjectObservations | null }) {
  if (data?.snapshot_capacity == null) return null;
  const incomplete = Boolean(data.dropped_snapshots || data.omitted_changes);
  return <div className="coverage-note" role={incomplete ? "status" : undefined} aria-label="Object snapshot coverage">
    <p>All object perspectives: {data.snapshots.length} / {data.snapshot_capacity} scoped baselines retained; {data.dropped_snapshots ?? "unreported"} snapshot evictions, {data.omitted_changes ?? "unreported"} currently retained changes omitted for expired scopes.</p>
    {incomplete && <p>Object perspective coverage is incomplete. Expiry is not object removal or proof of absent attributes. Reobserved scopes start a fresh comparison baseline; another perspective's values are never substituted.</p>}
  </div>;
}
