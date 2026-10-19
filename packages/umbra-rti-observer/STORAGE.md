# Observation storage and coverage

Storage limits are an observer concern, not an HLA service. Missing MOM, DDM,
ownership or time services does not disable storage, ordinary attributes,
events or registered lifecycle controls. The optional `storage_inspection`
capability means the backend reports the named `StorageCoverageResponse` in
`StateResponse.storage`. Missing/unsupported/disabled coverage stays unknown in
the frontend; it is never displayed as unlimited or zero loss.

## What is bounded

The reference store combines independent row limits with a shared **6 MiB
encoded-evidence budget**. It accounts retained JSON plus a 64-byte reserve per
record, not process RSS, Python object overhead, request parsing or temporary
normalization allocations. A single retained record defaults to at most 1 MiB.
Both numbers can be reduced; the record limit can be raised within the shared
budget. The reference store does not accept a shared budget above 6 MiB. This
leaves room below the default 8 MiB HTTP conformance/compact-state envelope for
metadata, collection wrappers and clock-dependent fields. It is not a promise
to retain every configured slot or arbitrarily large provider values.

Every observation collection participates: raw envelopes, classic objects and
roster, attributed/global object baselines and change batches, time snapshots
and requests, interactions, declarations, relationships, synchronization and
save/restore phases, explicit flow, ownership, DDM, MOM and FOM reports/history.
Rows have the existing independent count capacities. Previously unbounded
classic object/roster/time indexes now default to 5000/2000/2000 entries.

Context has its own 256 KiB budget, 128 keys and 128 KiB per encoded key, so an
operator metadata update cannot evict event history. Capability and adapter
metadata are separately admitted at at most 128 KiB each. Status is bounded to
256 UTF-8 bytes; at most 64 adapters with names up to 1024 UTF-8 bytes can attach.
An invalid explicit context/status/capability update is rejected atomically,
before changing retained configuration or calling a newly attached provider.
Event-derived context instead expires a whole key and reports that loss; a
previous value is not quietly carried forward as if it described the new event.

Python applications configure `ObserverStore(max_objects=..., max_federates=...,
max_time_snapshots=..., max_evidence_bytes=..., max_evidence_record_bytes=...)`.
The CLI exposes matching `--max-objects`, `--max-federates`,
`--max-time-snapshots`, `--max-evidence-bytes` and `--max-evidence-record-bytes`.
These settings do not load or call an RTI. A different backend can implement
the same coverage DTOs with its own explicitly reported limits and policy.

Derived alert configuration/evidence is a separate bounded resource, not an
extra observation array hidden in `StateResponse.storage`. It has 64 rules/
64 KiB of configuration, 500 transition records and 512 error time marks per
rule, with fixed named fields rather than provider payload copies. Its history
does not compete with the main evidence LRU or turn raw-event expiry into a
healthy zero. See [ALERTS.md](ALERTS.md) for those independent limits/lifetimes.

## Whole-record expiry, not modified facts

`storage.py` owns typed count/byte collections under the store lock. Records
are measured when written or refreshed after an owned mutation. Reads do not
extend their retention. Caller-visible context, event and page results are
detached snapshots: modifying a returned dictionary cannot rewrite stored
envelopes, bypass accounting or alter later recording frames.

Expiry always removes a whole record. Attribute values, opaque bytes, region
bounds, counters and FOM definitions are not clipped, converted to null or
replaced with global/another-participant data. An oversized or unencodable
record is explicitly unretained. Its insertion does not evict unrelated facts
to make room for something that can never fit the single-record limit.
Structural input validation still happens before an entire batch commits;
accepted/committed does not mean every record remains in the bounded cache.

The policy is whole-record LRU with **soft collection reservations**. Recent
observations compete for shared space, but eviction first considers collections
that can remain above their soft floor. This helps stable schema/management/time
context coexist with busy attribute traffic. Empty/unsupported families consume
no reserved space. Reservations are not guaranteed minimums: when whole records
cannot collectively fit, ordinary global LRU is the fallback. The exact floors
are reported; they are not hidden RTI-specific priorities. History and latest
reports still expire independently, and references to an expired FOM/region/
participant report remain unresolved rather than inventing a replacement.

## The contract and UI

`StorageCoverageResponse` contains separate evidence/context
`StorageBudgetResponse` groups. Each reports policy, accounting basis, total and
single-record limits, accounted bytes, per-record reserve, collection coverage
and any explicitly unbudgeted collections. The reference implementation lists
no unbudgeted observation collections. A partial implementation must list what
is not covered; it cannot silently claim full byte accounting.

Each `StorageCollectionResponse` has its name/unit, slot capacity, retained and
accounted counts, optional soft reservation, total dropped records/accounted
bytes and separate count/byte/oversize/encoding/parent-window loss counts.
Different collections often describe the same observation. Their loss counts
must not be summed into a unique event or federation-entity total. Encoding
loss may have no known new encoded size; dropped bytes are accounted bytes, not
the size of an arbitrary rejected payload.

Named models validate unique collections, budget sums/limits and loss-reason
totals. Storage-backed states require actual wire collections, not constructor
defaults. Coverage for delivered objects, roster, time and optional inspectors
must match their exact retained counts. Analytics projects the same cheap
coverage under the same lock, without walking unrelated inspector trees.

Diagnostics shows live budget meters and per-collection reasons. A common loss
notice distinguishes observation expiry from object removal, resignation,
disconnection or unsupported services. Paused/replayed views show their captured
coverage, not current live limits. Optional/disabled capabilities continue to
control their own views independently.

## Roster, objects and time

Roster expiry removes cached metadata, not the participant's identity from
retained observations. Analytics includes names explicitly present in retained
events in the selected analytics window even without a retained roster row,
with `roster_retained=false`, status
`observed`, and no invented role/handle/membership. `roster_storage` separately
reports the supplied coverage, or remains unknown on an older request/backend.
Federates includes explicit event-attributed names, preserves a missing user
selection and marks missing metadata; it does not silently select somebody else.
These are still legacy name-based consumers pending OBS-127 identity migration.

Object snapshots have independent capacity/byte expiry. Re-observing an expired
perspective starts a fresh local comparison baseline, not an RTI incarnation or
borrowed global values. Change batches can expire independently and disclose
`dropped_history_batches`; displayed changes stay within the retained event
window. A known null value remains distinct from an unknown previous baseline.

Time snapshots and request tickets have independent limits. A retained request
contains its full explicit scope and can outlive the latest-state snapshot.
`snapshot_retained=false` requires explicit snapshot-expiry coverage; an older
missing flag retains the earlier requirement for a matching snapshot. Such a
request is not silently cancelled or granted. The exact later scoped ID may
still match; expired role/bound/grant state is not borrowed. The time selector
includes retained request scopes and the ledger stays visible without a latest
snapshot. A genuinely expired ticket still makes later outcomes uncorrelated.
Explicit disconnect/resignation also closes matching pending tickets whose
latest snapshot has expired. Retained ended snapshots or session-ended tickets
reject late packets for that scope. After all terminal evidence expires there
is no unlimited tombstone registry; fresh producer session IDs remain required.

## Event windows, cursors and recordings

Retained raw envelopes always form an exact contiguous suffix. Byte pressure
removes a prefix; if the newest envelope cannot be retained, the older prefix
is also expired instead of leaving a hidden hole. `dropped_events` counts all
expired envelopes, `cleared_events` counts explicit clears, and `next_sequence`
remains the lifetime next cursor. An empty window has first/last retained
sequence zero; that does not mean no observations have occurred.

`EventPageResponse.next_sequence` and gap metadata report this high-water mark.
For a lost empty window, `gap=true` and `next_after=next_sequence-1` advance a
reader without claiming delivered envelopes. A reader at that cursor does not
receive the same gap forever. The stream rechecks retention after a wait before
consuming rows, and waiters wake on an empty lost window too. Older responses
without the new optional field keep their previous cursor behavior. Full SSE/
dynamic transport resumption is still OBS-128, not implied by these checks.

Both recording versions preserve every committed arrival, including immediately
unretained envelopes. A recorded event frame validates the exact retained suffix
and the number of envelopes that expired, rather than assuming count capacity
was the only limit. Empty retained windows still end their arrivals at the
lifetime cursor. V2 independently bounds decoded state and work; the store's
byte budget does not remove those import safeguards. See [RECORDING.md](RECORDING.md).

## Evidence and limits

Tests cover accounting/replacement, LRU/fairness, count/byte/encoding loss,
oversized records, exact suffixes and empty cursors, missing snapshots with
retained requests, metadata admission, detached public results and all inspector
families. Python/Java HTTP models, generated types and desktop/mobile scenarios
travel together. The Java server is a static read fixture, not evidence of live
Java eviction or RTI behavior. Native-provider checks remain separate.

Use the repeatable profiles in [SCALE.md](SCALE.md) to compare actual retained
counts, disclosed loss, state sizes, throughput and recording costs. The wider
diagnostic load is not promoted into the narrower performance reference merely
because its state now fits: its byte-limited retained window must be reported.
