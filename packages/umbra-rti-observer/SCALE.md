# Observer scale and retention

OBS-124 is in progress. The snapshot/analytics path, scoped object budget and
versioned compact recording have been improved and measured. Oversized-state
delivery and remaining persistent-index budgets are still open.
These are internal synthetic application measurements, not IEEE RTI evidence.

## Reproduce a measurement

Use an existing observer web/dev environment, or install the standalone package's
`conformance` extra (Pydantic). No RTI, native bindings, JDK, browser, HTTP server
or captured federation data is needed. From the repository root:

```powershell
python -m umbra_rti_observer.scale --profile rich --repeats 7 --output .cache/observer-scale/rich.json
python -m umbra_rti_observer.scale_reference --enforce .cache/observer-scale/rich.json
```

Profiles are `standard`, `sustained`, `rich`, `many-perspectives`, `large-index`,
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
they were equivalent. The standard/sustained/rich/many-perspectives assessment
must pass; large-index and wide intentionally expose limits and are assessed
separately. The `--enforce` exit code is opt-in, never part of default CI timings.

## Measured checkpoint: 19 October 2026

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
repeated-state storage; it does not remove the oversized-state finding.

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

## Remaining finding — not a completion gate silently waived

Wide payload/history combinations can still make full state exceed the
checker/import envelopes. Compact v2 explicitly rejects a baseline above its
8 MiB decoded-state limit; compact encoding must not conceal unlimited state
expansion. The classic global object/roster and persistent time-scope indexes
also keep their earlier lifetime; the scoped object budget does not bound every
source of growth. Add explicit budgets/delivery behavior and coverage where
needed, without substituting unknown data, quietly dropping providers or
weakening model validation. The `wide`/`large-index` reports remain evidence,
not passes.

Complete the remaining OBS-124 finding before calling scale/retention delivered. Then
continue alerts, saved workspaces, stable identity migration, transport
negotiation and unknown-session recovery from [FEATURE-BACKLOG.md](FEATURE-BACKLOG.md).
Native-provider command verification remains an honest separate gate.
