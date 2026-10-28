# Observer scale and retention

The dated reference measurements below apply to their recorded source
fingerprints. The 2026-10-20 coordination-control, topology and demo increment
has not repeated those scale measurements; it does not claim the earlier
reports describe its new source fingerprint.

OBS-124 is delivered within the explicit reference envelope below. Exact
snapshot/analytics projections, lossless compact recording and shared count/
encoded-byte evidence budgets have repeatable measurements. Wider loads now
have bounded delivery and disclosed expiry, not a promise of full retention.
These are internal synthetic application measurements, not IEEE RTI evidence.

## Reproduce a measurement

Use an existing observer web/dev environment, or install the standalone package's
`conformance` extra (Pydantic). No RTI, native bindings, JDK, browser, HTTP server
or captured federation data is needed. From the repository root:

```powershell
python -m umbra_rti_observer.scale --profile rich --repeats 7 --output .cache/observer-scale/rich.json
python -m umbra_rti_observer.scale_reference --enforce .cache/observer-scale/rich.json
```

Profiles are `standard`, `sustained`, `rich`, `identity`, `many-perspectives`, `large-index`,
`wide` and `recording-rich` (100 attempted recording commits). The tool now uses
`--recording-format compact-v2` by default; select `full-v1` for a controlled
legacy comparison. Reports record the actual format, including failed starts.
Run them serially without other test/build workloads when comparing
times. Reports use named strict models and record source fingerprint, Python/
Pydantic/platform/processor details, workload dimensions, timing distributions,
response sizes, capture behavior and semantic checks. The fingerprint covers
package Python sources with normalized line endings; source changes during a run
invalidate reference coverage. It is not a hardware certification.

The ingress loop includes synthetic event construction and committed batches.
One warmup is excluded from each repeated operation; automatic garbage collection
remains enabled. Seven samples use nearest-rank empirical p95 (the maximum with
only seven samples), not a high-confidence latency estimate. A separate optional
`tracemalloc` pass measures Python allocation peak, **not process RSS**; its own
tracing is not included in timings. `--no-memory-probe` omits that pass and reports
unknown allocation rather than zero. Tests use tiny workloads to verify logic;
they do not assert CI timing budgets or prove high-volume performance.

All workloads preserve exact retained counts, attribution and clear behavior.
Optional inspector seeding is outside timed ingress; rich has 576 seed events
across declarations, relationships, coordination, flow, ownership, DDM, MOM, FOM
and pending time requests. The main workload uses numeric `value` and a bounded
text `payload` attribute. A source name is never participant attribution.

Current workloads explicitly supply the same synthetic reporting session on
ordinary and seeded observations; the fixture knows that session, rather than
assuming a missing session matches a reported one. The `identity` profile adds
named participant/membership IDs and object federation contexts to the rich
workload, exercising their actual validation, serialization and storage costs.
Other profiles remain legacy-provenance workloads. Do not relabel their results
as measurements of explicit-join payloads. The reference assessment now checks
actual retained roster/object counts as well as configured workload dimensions.

## Explicit reference envelope

The opt-in `scale_reference` checker assesses saved measurements against:

| Dimension | Reference envelope/budget |
| --- | --- |
| Evidence | At least 20,000 timed ingress events, seven pipeline samples, source fingerprint, no tracing during timings |
| Retained events | At most 2000; exact envelopes, not sampled |
| Persistent dimensions | At most 100 named roster entries, 1000 objects and 5000 scoped object baselines |
| Payload | Two attributes: numeric value and at most 128 bytes of synthetic text |
| Optional inspectors | Up to 64 seeded records in each exercised family |
| Ingress | Batches of 100; measured floor 1000 events/second |
| Full response pipeline | In-process snapshot + model validation + serialization empirical p95 at most 500 ms |
| Analytics projection | In-process projected snapshot + validation + aggregation empirical p95 at most 100 ms |
| Full response size | At most 8 MiB, matching the default HTTP-read checker's bound |

This is a measured **Python snapshot/analytics reference workload**, not a
hardware-independent SLA, arbitrary-payload promise, network/HTTP/browser timing
claim, Java-provider benchmark or recording-duration guarantee. Missing MOM and
other optional inspectors do not invalidate ordinary observations. Wider loads
are explicitly diagnostic rather than promoted into the reference envelope.

Current dated reports and assessments live in [benchmarks](benchmarks/). Check
their exact source/runtime metadata rather than comparing machines as though
they were equivalent. The standard/sustained/rich/identity/many-perspectives assessment
must pass; large-index and wide intentionally expose limits and are assessed
separately. The `--enforce` exit code is opt-in, never part of default CI timings.

## Final observer-source verification: 19 October 2026

Five serial seven-sample reference profiles use unchanged source fingerprint
`29b6942a5b92f094defe25cb1ce26790b99f1b95d686fae5188ec5716db28cdd`.
They include the final exact provider-name/command-ID validation changes on
Windows/CPython 3.14.6/Pydantic 2.14.1. This run launched no concurrent build/test
jobs or allocation tracing during measurements; it does not establish machine
isolation. All five pass the unchanged
[reference assessment](benchmarks/2026-10-19-final-reference.json).

| Profile | Timed events / actual retained | Objects / scoped baselines | Full response MiB | Full pipeline p95 ms | Projected analytics p95 ms |
| --- | --- | --- | --- | --- | --- |
| standard | 20,000 / 2000 | 200 / 400 | 4.07 | 247.01 | 31.21 |
| sustained | 100,000 / 1749 | 1000 / 1977 | 5.37 | 299.12 | 29.62 |
| rich | 20,000 / 1599 | 1000 / 1964 | 5.31 | 312.35 | 96.45 |
| explicit identity | 20,000 / 913 | 914 / 1743 | 5.48 | 428.94 | 35.48 |
| many-perspectives | 20,000 / 2000 | 200 / 2436 | 5.33 | 337.63 | 31.04 |

Ingress spans 3267–5533 events/second on this host. Rich's 96.45 ms empirical
analytics p95 is close to the 100 ms reference budget; seven samples are not a
portable latency guarantee. Rich and identity seed 576 inspector events outside
the timed ingress. Shared count/encoded-byte budgets disclose shorter retained
windows; requested counts are ceilings, not promises that all 2000 envelopes,
1000 objects or 5000 scoped baselines survive together. Whole records expire;
the generator checks exact retained envelopes, attribution and clear semantics.

These remain in-process synthetic snapshot/analytics/recording checks, not
native lifecycle/recovery-call or command-history memory, HTTP/SSE/WebSocket/
browser timing, process RSS, arbitrary federation scale or SLA evidence. No
allocation probe was run. Historical recovery/streaming and diagnostic reports
below retain their original fingerprints and are not relabeled as final-source
measurements.

## Recovery-source verification: 19 October 2026 (historical checkpoint)

Five serial seven-sample reference loads use source fingerprint
`b393aab312bfaa5476dbde2b33f5c9d20053054c87caa58f29ee6383f24e77e1`,
unchanged across the runs on the same Windows/CPython 3.14.6/Pydantic 2.14.1
host. This run launched no other build/test jobs or allocation tracing while
measuring. All five pass the unchanged
[reference assessment](benchmarks/2026-10-19-recovery-reference.json).
Subsequent exact provider-name and command-ID validation changes are not in
that fingerprint. These reports remain evidence for the named checkpoint,
not source-matched measurements of the final commit.
These remain in-process observer snapshot/analytics/recording workloads, not
recovery-call/command-history memory, native/Java RTI, stream/network/browser,
RSS or arbitrary-federation/SLA measurements.

| Profile | Timed events / retained | Scoped baselines | Full response MiB | Full pipeline p95 ms | Projected analytics p95 ms |
| --- | --- | --- | --- | --- | --- |
| standard | 20,000 / 2000 | 400 | 4.07 | 246.19 | 31.00 |
| sustained | 100,000 / 1750 | 1977 | 5.37 | 275.86 | 27.11 |
| rich | 20,000 / 1599 | 1964 | 5.31 | 326.43 | 86.63 |
| explicit identity | 20,000 / 913 | 1743 | 5.48 | 398.60 | 34.82 |
| many-perspectives | 20,000 / 2000 | 2435 | 5.33 | 327.28 | 30.95 |

Measured ingress ranges from 3367 to 5501 events/second. The shared 6 MiB
encoded-evidence budget and whole-record loss policy are unchanged. Identity
retains 914 classic objects and 913 envelopes, not all configured slots;
rich/identity each seed 576 inspector events outside timed ingress. Smaller
retained windows and independent cache expiry remain explicit, not sampled or
interpreted as federation removal/resignation. Wide/large-index remain diagnostic
on their earlier source versions. Every earlier report below keeps its original
fingerprint and measurement; no historical evidence is relabelled current.

## Historical transport-source verification: 19 October 2026

Five serial seven-sample reference loads use source fingerprint
`78b436e80ffbc584bda436b6860bcf7690af7acd973da605997475afe4b334d2`,
unchanged across the runs, on the same Windows/CPython 3.14.6/Pydantic 2.14.1
host. No other tests/builds or allocation tracing ran during these measurements.
All five pass the unchanged [reference assessment](benchmarks/2026-10-19-streaming-reference.json).
These are in-process snapshot/analytics/recording checks of the transport-source
revision, **not** measurements of SSE/WebSocket delivery, HTTP/browser latency,
native/Java RTI behavior, RSS or an arbitrary federation/SLA.

| Profile | Timed events / retained | Scoped baselines | Full response MiB | Full pipeline p95 ms | Projected analytics p95 ms |
| --- | --- | --- | --- | --- | --- |
| standard | 20,000 / 2000 | 400 | 4.07 | 239.78 | 44.07 |
| sustained | 100,000 / 1750 | 1977 | 5.37 | 325.70 | 29.35 |
| rich | 20,000 / 1599 | 1964 | 5.31 | 302.13 | 82.52 |
| explicit identity | 20,000 / 913 | 1743 | 5.48 | 431.51 | 34.46 |
| many-perspectives | 20,000 / 2000 | 2435 | 5.33 | 331.74 | 31.41 |

Ingress ranges from 3291 to 5427 events/second across these measured profiles.
The 6 MiB encoded-evidence budget and whole-record loss policy remain unchanged.
The identity case retains 914 classic objects and 913 envelopes, not its full
configured 1000/2000 slots. Rich/identity each seed 576 inspector observations
outside timed ingress. A small count difference between runs is not a guarantee
of extra history: actual JSON byte sizes still determine bounded retention.
Wide/large-index are not promoted into the reference envelope; their historical
diagnostic evidence and all earlier source fingerprints remain below.

## Historical identity/provenance verification: 19 October 2026

Seven serial seven-sample runs on the same Windows/CPython 3.14.6/Pydantic 2.14.1
host share source fingerprint `fef6ffafe6c1…`, with no source changes or tracing
during measurement. Allocation/RSS was not measured. The explicit-identity case
exercises the real named attribution/object-context payloads, not just a legacy
workload relabelled as identified. The first five profiles pass the
[identity checkpoint assessment](benchmarks/2026-10-19-provenance-reference.json).
Ingress ranged from 3364 to 5421 events/second across those five runs.

| Profile | Timed events / retained | Scoped baselines | Full response MiB | Full pipeline p95 ms | Projected analytics p95 ms |
| --- | --- | --- | --- | --- | --- |
| standard | 20,000 / 2000 | 400 | 4.07 | 224.70 | 29.57 |
| sustained | 100,000 / 1749 | 1977 | 5.37 | 276.68 | 27.04 |
| rich | 20,000 / 1599 | 1964 | 5.31 | 348.63 | 86.67 |
| explicit identity | 20,000 / 913 | 1743 | 5.48 | 409.05 | 42.66 |
| many-perspectives | 20,000 / 2000 | 2436 | 5.33 | 325.72 | 31.37 |
| large-index, diagnostic | 20,000 / 2000 | 2435 | 5.33 | 367.84 | 31.00 |
| wide, diagnostic | 20,000 / 663 | 1266 | 5.67 | 134.94 | 14.99 |

The 6 MiB encoded-evidence budget is unchanged. Richer metadata costs storage:
the identity profile retained 913 envelopes and 914 classic objects, not all
2000/1000 configured slots. Loss and independent cache expiry remain explicit;
neither missing objects nor roster expiry imply RTI removal/resignation. Rich
and sustained also retain shorter windows. These are measured retained suffixes,
not sampled events or promises of full-duration retention. The rich/identity
profiles each seed 576 additional inspector observations outside timed ingress.

The wider cases still [fail the reference shape](benchmarks/2026-10-19-provenance-diagnostic.json)
despite bounded response sizes and timing: their configured capacities/payloads
remain outside the envelope. Their apparent speed must not be promoted into a
full-retention guarantee. Raw reports preserve exact counts, timing summaries,
recording outcomes and environment/source metadata. Existing historical reports
below keep their original source fingerprints and measurements. These remain
in-process synthetic checks, not browser/network, Java RTI or native-provider
performance/conformance evidence.

## Historical snapshot checkpoint: 19 October 2026

These serial runs used Windows 11, CPython 3.14.6, Pydantic 2.14.1 and the
32-logical-CPU AMD host identified in each report. All six have the same source
fingerprint (`1c0509664bf0…`) and report no source changes during measurement.
Numbers are rounded here; the JSON reports retain sample counts/distribution
summaries, workload and source metadata. This is a reference on this host, not
a promise for another machine.

| Profile | Timed events / retained | Scoped baselines | Full response MiB | Full pipeline p95 ms | Projected analytics median / p95 ms |
| --- | --- | --- | --- | --- | --- |
| standard | 20,000 / 2000 | 400 | 2.95 | 132.56 | 16.70 / 17.22 |
| sustained | 100,000 / 2000 | 2000 | 4.59 | 191.73 | 17.70 / 18.06 |
| rich | 20,000 / 2000 | 2000 | 4.92 | 228.52 | 17.45 / 57.08 |
| many-perspectives | 20,000 / 2000 | 5000, with 13,381 explicit evictions | 4.62 | 266.32 | 17.43 / 80.06 |
| large-index, diagnostic | 20,000 / 2000 | 18,381, configured budget 50,000 | 9.73 | 1185.35 | 21.54 / 22.35 |
| wide, diagnostic | 20,000 / 10,000 | 4000 | 65.47 | 1084.78 | 142.56 / 274.90 |

The first four pass [the reference assessment](benchmarks/2026-10-19-reference.json).
The diagnostic cases [do not pass](benchmarks/2026-10-19-wide-assessment.json).
In rich, full-state analytics had a 137.09 ms median versus 17.45 ms projected
(about 7.9× less processing time); the separate allocation probe fell from
25.10 MiB to 6.44 MiB. The small validated summary pipeline measured median
2.62–2.77 ms in the 2000-event profiles. No HTTP/network/browser timing is
implied. Allocation was deliberately not probed for the two diagnostic cases.

The original full-v1 recording capacity finding was not hidden in the green
snapshot assessment: standard retained five complete states; sustained, rich
and many-perspectives retained three; large-index retained one; wide could not
fit its initial state. All limit behavior was explicit and retained observation
counts/attribution/clear checks passed. The compact increment below addresses
repeated-state storage; at that checkpoint it did not remove the oversized-state
finding. The later shared-storage measurements below address bounded delivery.

## Changes justified by measurement

The original analytics route built and validated the full inspector tree even
though aggregation only needs retained envelopes, persistent roster and
retention. `ObserverStore.analytics_window()` now takes that exact projection
under the same lock; `AnalyticsWindow` validates it. Full-state and projected
aggregation are regression-compared at identical times/windows/scopes. There is
no event sampling, time/grant inference, incremental cache or downsampling.

Health and metrics likewise use a named `RuntimeSummaryResponse` projection.
Its metrics helper is shared with full state, preserving counts/status/retention
without traversing historical object attributes or optional inspector trees.
The existing HTTP response models and meanings do not change.

Recorded analytics accepts the lighter named window request only when the live
backend advertises `recording_analytics_window`; otherwise React sends the
original full-frame request. Both forms remain supported by the reference
backend. Captured capabilities/URLs never determine that negotiation.

Many-to-many object perspectives previously grew independently of event
retention. The scoped index now defaults to 5000 least-recently-observed
baselines, including global and attributed scopes as separate records. Capacity,
cumulative scope evictions and currently omitted changes for expired scopes are
explicit. Raw events/analytics are not removed by this expiry. Reobserving a
scope starts a fresh comparison baseline, not borrowed global/other-participant
values and not proof of a new RTI incarnation. Older metadata remains unknown.
The later shared byte budget can independently expire envelopes, classic objects
and roster metadata too; this is separate from a scoped-index count eviction.
Objects, Federates and Diagnostics show coverage; object lists initially render
100 rows and retain complete supplied-snapshot search/load-more access.

## Compact recording increment

V2 baseline/edit storage is explicitly negotiated through the live backend's
`recording_compact` capability. It preserves every accepted full state, envelope
batch, clear/metadata/checkpoint boundary, monotonic offset and provenance. Old
v1 requests/files remain supported; there is no sampling or inferred inspector
state. Both encoded-file and reconstructed-state/work/operation limits are
enforced, not waived by a compact file. Every reconstructed state is validated
through the same named models. See [RECORDING.md](RECORDING.md) for exact limits
and the independently implemented worker decoder.

Reproduce the richer storage comparison (run these serially):

```powershell
python -m umbra_rti_observer.scale --profile recording-rich --recording-format full-v1 --repeats 7 --no-memory-probe --output .cache/observer-scale/legacy-recording-rich.json
python -m umbra_rti_observer.scale --profile recording-rich --recording-format compact-v2 --repeats 7 --no-memory-probe --output .cache/observer-scale/compact-recording-rich.json
```

The same 20,000-event rich workload attempts up to 100 more batches of 100.
Stop-on-limit behavior is intentional; a requested frame capacity is not a
promise that arbitrary payloads fit the separate byte/work budgets. Reports
retain attempted-commit timing counts, accepted state count, actual export size,
validation cost and explicit stop reason. These are in-process measurements,
not browser/HTTP/RSS or long-duration recording guarantees. Compact capture
does more diff/budget work than v1; space reduction is not a claim of lower
capture latency. One-second idle checkpoints still preserve exact clock fields.

The controlled comparison used the same Windows/Python/Pydantic host as above,
with matching source fingerprint `8e112cd80965…` and no source changes during
either serial run. Both intentionally reached the unchanged 16 MiB byte budget:

| Format | Accepted states | Export MiB | Timed publish attempts | Capture commit median / empirical p95 ms | Export model validation ms |
| --- | --- | --- | --- | --- | --- |
| [Full v1](benchmarks/2026-10-19-legacy-recording-rich.json) | 3 | 15.42 | 3 | 118.75 / 162.29 | 688.63 |
| [Compact v2](benchmarks/2026-10-19-compact-recording-rich.json) | 34 | 15.88 | 34 | 380.85 / 448.56 | 7632.90 |

V2 retains about 11.3× as many complete states here, with 40,641 literal edits
and 182,193,684 charged reconstructed bytes. No accepted boundary was sampled;
the uncaptured tail is explicit. Timed attempts include the final rejected
over-budget commit. Different/small sample counts make these cost summaries
diagnostic, not comparable high-confidence latency estimates. Diff/budget work
costs more per commit and validation processes more states; files are not free
to capture/import. No allocation probe was run for this comparison. Older six
snapshot reports above retain their own earlier source fingerprint and v1
recording results; they were not relabeled as current v2 measurements.
Both new reports also pass the separate snapshot/analytics
[reference assessment](benchmarks/2026-10-19-recording-assessment.json); that
assessment does not turn capture/import cost into a latency guarantee.

## Shared-storage verification: 19 October 2026

The reference store now accounts every observation collection within a shared
6 MiB encoded-JSON budget, with independent count limits, single-record limits
and disclosed soft collection reservations. Context and adapter/capability
metadata have separately admitted bounds. Whole records expire; values are not
clipped, sampled or replaced with unknown/null. Latest-cache loss does not erase
retained event attribution or pending time tickets. Explicit disconnect/resign
also closes orphaned tickets; retained ended evidence blocks late grants.
See [STORAGE.md](STORAGE.md) for exact accounting, cursors and limitations.

Six new serial runs use source fingerprint `d434065ee3f8…`, unchanged throughout,
on the same Windows/Python/Pydantic host. Each has seven pipeline samples with
no allocation tracing during timings; the first four additionally have separate
allocation probes. Reports contain actual coverage for every collection.
The generator verifies every retained original envelope, the exact contiguous
suffix/lifetime cursor/drop count, participant/unattributed totals, projection
equivalence and clear behavior. A byte-limited window is not mislabeled as the
configured slot count.

| Profile | Timed events / actual retained | Scoped baselines | Full response MiB | Full pipeline p95 ms | Projected analytics p95 ms |
| --- | --- | --- | --- | --- | --- |
| standard | 20,000 / 2000 | 400 | 2.955 | 162.95 | 22.43 |
| sustained | 100,000 / 2000 | 2000 | 4.594 | 252.03 | 28.70 |
| rich | 20,000 / 2000 | 2000 | 4.932 | 314.78 | 97.70 |
| many-perspectives | 20,000 / 2000 | 5000; 13,381 expiries | 4.629 | 368.91 | 23.33 |
| large-index, diagnostic | 20,000 / 2000 | 6157; 12,224 expiries | 5.074 | 432.87 | 24.80 |
| wide, diagnostic | 20,000 / 739 (10,000 slots configured) | 1410; 36,771 expiries | 5.674 | 133.34 | 59.78 |

The four reference profiles pass the unchanged
[reference assessment](benchmarks/2026-10-19-storage-reference.json).
Ingress spans 6026–8680 events/second across all six loads on this host. Rich's
97.70 ms empirical p95 is close to the 100 ms reference budget; seven samples
are not a portable latency guarantee. Do not treat this as a speed improvement
over historical runs on another source revision.

The [diagnostic assessment](benchmarks/2026-10-19-storage-diagnostic-assessment.json)
remains a non-pass because both loads exceed the defined reference shape. The
large-index configuration asks for 50,000 perspectives; its state is now 5.074
MiB rather than the historical 9.73 MiB, with actual expired baselines disclosed.
Wide is now 5.674 MiB rather than 65.47 MiB, with 19,261 expired envelopes and a
739-event exact suffix, not silent retention of all 10,000 configured slots.
Its accounted evidence is 6,291,360 bytes under the 6,291,456-byte budget.
Expiry counts are record operations, not unique RTI entities or incarnations.

All six compact captures can start with the bounded state. Standard, sustained
and rich retain 14 states through the 12-commit profiling exercise and stop on
request. Many-perspectives, large-index and wide retain 3, 5 and 7 states before
the unchanged encoded byte limit stops capture, not collection. These bounded
capture outcomes do not promise a useful duration for arbitrary payloads or a
lower capture/import latency. The earlier 100-commit v1/v2 comparison remains
historical evidence, not a new storage-revision measurement.

The portable schema and generated types are current; 662 backend/tool checks
and the full 292 desktop/mobile browser checks pass. The two new lifecycle
regressions reproduce and fix disconnect/resign of an expired snapshot's
pending ticket. Python 3.11 core-only and both playtest startup profiles work.
The native-provider smoke remains skipped, not evidence of RTI conformance.

## Limits that remain explicit

The budget is encoded evidence, not process RSS, transient normalization,
unbounded incoming bodies, filesystem durability, network/HTTP/browser timing
or arbitrary recording duration. No limited RTI must provide MOM, DDM or
unrelated optional families to use events, attributes and time. Older backends
with missing storage models stay unknown; Java's static HTTP fixture proves
read-contract portability, not live Java eviction or provider performance.
V2 still rejects an imported baseline over its independent 8 MiB decoded-state
limit. Native-provider command verification remains a separate gate.

Continue alerts, saved workspaces, stable identity migration, transport
negotiation and unknown-session recovery from [FEATURE-BACKLOG.md](FEATURE-BACKLOG.md).
