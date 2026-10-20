# Observer feature backlog

## Coordination, topology and demo increment (2026-10-20)

This extends the completed 2026-10-19 checkpoint. The dated native, browser and
scale evidence below still describes that earlier source snapshot.

| ID | Status | Feature | Delivered scope |
| --- | --- | --- | --- |
| OBS-130 | Done | Owned coordination and progress | Seven strict versioned/confirmed sync/save/restore requests, named service-return receipts, explicit driver opt-in, synthetic executor and paged scope/participant cards. Limited/lifecycle-only adapters omit the services. |
| OBS-131 | Done | Complex topology | Stable directed-layer, connected-group and circle layouts; keyboard edge inspection; bounded evidence; cycles, parallel/reciprocal links, self-loop, rejoin and disconnected demo group. |
| OBS-132 | Done | Demo modalities | All 19 workspaces linked with exercises; paced coordination success/failure/unknown states; nominal configuration, pause, capture/replay and alerts; limited and existing Java/failure profiles. |
| OBS-133 | Next adapter increment | Native coordination opt-in | Map only supported services/callbacks in the separate Umbra adapter. Establish official-API/native evidence for local requests and collective results; preserve exact labels, owner execution, rejection and unknown effects. |

Verification: full Python application/adapter/launcher/source-policy run **823
passed, one optional native module skipped**; after final rejoin/checkpoint-scope
hardening, 40 related checks passed. The final receipt/schema/conformance slice
passed 123 checks. All 32 pure frontend checks, strict typing/lint and generated
schema/type drift gates passed.

The 392-case full browser run passed 390; two mobile cases loaded the fallback
page during a concurrent local asset rebuild. With stable assets, all 60
identity/topology/workspace/new-demo cases passed, including both failures.
The final 30 coordination/demo checks passed after scope/participant paging
was added, including two new paging cases. All 394 current browser cases are
covered across the full run and focused reruns; no failure remains unresolved.
Visual review covered topology, local/collective progress and coordination
forms. These are internal observer checks, not normative RTI conformance evidence.

The default real Umbra adapter remains lifecycle-only. New services run in the
rich synthetic driver or an application driver implementing the typed seam.
Read-only Java fixture success does not claim Java execution. Timed/subset
services and application-state persistence remain separate adapter work.
See [coordination controls](COORDINATION-CONTROLS.md) and
[demo coverage](DEMO-COVERAGE.md) for the contract and junior walkthrough.

## Real-provider lifecycle gate (2026-10-19)

OBS-102/103 now pass the opt-in test against an installed, real Umbra 2025
extension. Its unique federation is created/cleaned by an independent test
administrator; the observer advertises no create/destroy command. The owned
session connects, safely rejects an allowlisted but nonexistent federation,
joins the actual federation, resigns and disconnects. Catalog states, a returned
federate handle, unchanged target incarnation, lifecycle audit events and one
non-caller owner thread for factory/calls/evoked polling are asserted. Default
native inspection remains unadvertised: no universal query or fabricated state
was added. See the
[native control smoke](../umbra-rti-observer-umbra/tests/test_native_control_smoke.py).

Two binding integration defects were corrected: stale private handle-header
paths, and the 2025 facade incorrectly calling its two-argument exception factory
with the separate 2010 signature. Shared 2025 edge translation now preserves
the native error through Python `__cause__`; the public API and 2010 facade are
unchanged. Four focused edge tests with nine subcases and the actual lifecycle
test pass together. The separate C++14/2010 extension passes 19 tests with 376
subcases. These are internal application/provider-boundary checks, not IEEE
normative evidence.

Verification used Windows, CPython 3.14.6, GCC 15.2.0, Ninja, pybind11 3.1.0,
scikit-build-core 1.1.1 and Pydantic 2.14.1 in
`.cache/observer-native-d39b72c0/venv`. Both extensions were imported before
testing, with the GCC DLL directory explicitly registered. The 2025 target uses
C++20; the separate 2010 target explicitly uses C++14. Installed facade bytes
match the source. Native wheel SHA-256:
`95b4acbb6c9233a6da5e458f04e3bef340a87a71a4c3981921e795f63195d1d7`.
Loaded 2025 extension SHA-256:
`210830697a595fea2ca55e77e4dcf4f87ae31dd1fffd9ca2bfdd040ba668ac08`;
2010 extension SHA-256:
`6296f59dd79a39337ed939b788a308a185330fb7b8fdbf3e6255b54cefdd089c`.
This source-checkout build includes the current unrelated RTI working tree;
it is not a standalone SDK-wheel or clean-release certification.

The broader native suite is **not green**: 76 tests and 513 subcases pass,
while 12 failures remain outside the observer lifecycle gate. They concern
two floating/logical-time assertions, seven conveyed-region/object-scope
assertions, a time-regulation pending callback, and two configured-connection
subcases that supply a non-TCP address. Exact cases are in
`packages/umbra-rti-native/tests/test_native.py`; rerun that native package's
tests to investigate. None was silently skipped, relabeled a pass or fixed by
changing unrelated RTI/C++ work. The observer cannot promise those RTI services
or real native recovery-query behavior. Limited/unknown capabilities remain
independent of the successfully verified lifecycle seam.

## Current acceptance audit (2026-10-19)

The current federation visualization backlog is delivered at the acceptance
scope below. Application verification is 814 passing Python checks (the native
module is skipped in that environment), all 384 current desktop/mobile browser
checks, 27 frontend unit checks, strict typing/lint and no-write generated wire/
OpenAPI/TypeScript/workspace checks. The actual native lifecycle and edge checks
are separate successful runs, not an inference from those synthetic tests.
Eight recorded/live-Python/independent-Java HTTP profiles cover full, limited,
partial, empty, retention-loss, no-analytics, reconnect and explicit identity.

| Acceptance scope | Backend evidence | Frontend / integration evidence |
| --- | --- | --- |
| OBS-001–007: federation/federate workspace, events, analytics and diagnostics | `test_core.py`, `test_analytics.py`, `test_server.py` | `workspace.spec.ts`, `degradation.spec.ts`, `portability.spec.ts` |
| OBS-100/101: validated commands, history, confirmation, timeout and late outcomes | `test_commands.py`, `test_command_api.py` | `actions.spec.ts`, `session-recovery.spec.ts`, `scripts/commands.test.mjs` |
| OBS-102/103: explicitly owned Umbra lifecycle | Strict adapter tests plus the installed native smoke and binary evidence above | Synthetic rich/limited Actions workflows; actual one-owner native state transitions, rejection and cleanup |
| OBS-104/105: read-only synchronization and persistence | `test_coordination.py` | `coordination.spec.ts`, independent Java responses |
| OBS-110: declarations and explicit topology | `test_declarations.py`, `test_relationships.py` | `declarations.spec.ts`, `topology.spec.ts` |
| OBS-111: independent object/attribute perspectives and charts | `test_object_views.py` | `objects.spec.ts`, identity and storage scenarios |
| OBS-112: explicit interaction pairing and honest intervals | `test_interaction_flow.py` | `interaction-flow.spec.ts` |
| OBS-113/120: persistent time and request/grant correlation | `test_time_views.py` | `time.spec.ts`, independent Java grant-only/correlated models |
| OBS-114: explicit ownership state/tickets | `test_ownership.py` | `ownership.spec.ts` |
| OBS-115: exact DDM ranges and reported associations | `test_regions.py` | `regions.spec.ts` |
| OBS-116: optional MOM evidence, never ordinary membership/authority | `test_mom.py` | `mom.spec.ts` |
| OBS-117: supplied FOM metadata, exact references/bounds | `test_fom.py` | `fom.spec.ts` |
| OBS-121: bounded exact v1/v2 recording/replay | `test_recording.py`, `test_compact_recording.py`, `test_recording_codec.py` | `recording.spec.ts`, `compact-recording.spec.ts`, replay unit checks |
| OBS-122/123: portable validated contract and repeatable frontend | `test_conformance.py`, portable schema, eight HTTP profiles | Full 384-case browser run, generated client checks, `.github/workflows/observer.yml` |
| OBS-124: bounded storage and measured scale | `test_storage.py`, `test_scale.py`, five final source-matched reports | `storage.spec.ts`, `scale.spec.ts`; unchanged reference budgets and disclosed loss |
| OBS-125: typed alerts and independent configuration authority | `test_alerts.py`, `test_participant_alerts.py` | `alerts.spec.ts`, `alerts-replay.spec.ts` |
| OBS-126: private, portable saved workspace configuration | `test_workspaces.py`, shared normalization/rejection corpus | `saved-workspaces.spec.ts`, eight workspace unit checks and offline validator drift checks |
| OBS-127: scoped actor identity, aliases and provenance | `test_participant_keys.py`, `test_participant_provenance.py` | `participant-identity.spec.ts`, shared Python/TypeScript key corpus |
| OBS-128: optional discovered transports and safe resumption | `test_streaming.py` | `streaming.spec.ts`, seven streaming unit checks, older HTTP-only Java backends |
| OBS-129: capability-driven owned inspection/reconciliation | `test_session_recovery.py`, typed strict-fake adapter checks | `session-recovery.spec.ts`, complete/partial/disabled inspection and stale-version/pause isolation |

Backend test names above are under `tests/`; browser tests are under
`../umbra-rti-observer-ui/e2e/`. Their assertions cover each family's partial/
unknown/unsupported states, scoped evidence and independent retention rather
than requiring a full-featured RTI. Named models, generated client types,
[CONTRACT.md](CONTRACT.md), [JUNIOR-GUIDE.md](JUNIOR-GUIDE.md) and the family
guides travel together. Core/UI source has no Radar/legacy-track dependency;
no provider import was added to the standalone package.

Five final serial reference profiles pass on source `29b6942a5b92…`; see
[SCALE.md](SCALE.md) for raw reports, exact shorter retained windows and limits.
The actual standalone wheel contains 52 byte-matched package files and zero
unconditional runtime dependencies. Its Python 3.11.9 core passes with site
packages disabled and no Pydantic/web/HLA/provider imports. Wheel SHA-256:
`bcb283dfca2082399c23409bfdc1b9d370efa47e1d2c3edc54e8fc2abf2346ee`.
The 119-entry Miguel ZIP contains source, schemas, current guides/reports and
the production React assets, not native binaries, credentials or developer
environments. Every archive entry is checked against source bytes. A cold
extraction into a path containing spaces creates its private Python 3.11 web
environment and starts both rich/limited profiles with Node/pnpm/Java removed
from PATH and no checkout imports. Both show advancing events and stop their
own temporary-port servers. Repository-root rich/limited startup checks also
pass. See [SHOWCASE.md](SHOWCASE.md) for the user-facing launch flow.

This closes the observer backlog, not every RTI implementation task. Java is
a read-only interoperability fixture, not a production Java executor; native
inspection is unavailable until a real authoritative query is configured;
collective management commands are not advertised. The 12 broader native
failures above and 44 unrelated repository-wide source-size findings remain
explicit. Observer/native task paths have no source-size violations, the seven
source-policy regressions pass, and unrelated RTI/C++ work was not altered.

Earlier increment narratives retain their checkpoint-specific counts and
then-open handoffs. This audit and the current acceptance tables supersede
those historical status statements.

## Owned-session recovery increment (2026-10-19)

OBS-129 is delivered for explicitly configured owned adapters: named target
incarnation/revision guards, inspection-only and confirmed fresh reconciliation,
typed provider facts, healthy owner-pump fencing and unchanged original history.
Incomplete/missing/out-of-allowlist facts never restore authority. A blocked
provider call remains busy, including after timeout; late proof stays attached
to that original result. Recovery neither replaces a driver nor retries a failed
operation. Observation identities, MOM and topology do not grant control.

The Umbra boundary accepts a server-owned typed inspector for bindings that
actually provide a suitable query. No universal HLA connection/membership API
is invented. Unsupported inspection preserves existing lifecycle/observation
features; partial queries remain explicitly unresolved. The showcase supports
complete/partial/disabled inspection and one-shot synthetic after-effect failures.
The Actions panel separates inspect/review/reconcile, invalidates stale
confirmation and preserves pause/replay isolation and historical evidence.

Verification: the current complete 814-check backend/adapter/launcher/source-
policy run passes with one opt-in native smoke skipped in that non-native
environment. The separate native smoke above now passes. After exact provider-
name and command-ID hardening, the complete 384-case desktop/mobile run passes.
Twenty-seven frontend unit checks include exact current/legacy request shapes.
The seven source-policy utility checks are included in the 814 total;
generated DTO/schema growth guards reject
oversized writes and registered artifacts are no longer misreported missing.
The repository-wide size audit still has 44 unrelated RTI/planning findings;
observer artifact guards pass and those unrelated files were left unchanged.
Strict typing/lint, generated schema/type drift, all eight recorded/live-Python/
independent-Java HTTP profiles, and dependency-free Python 3.11 core behavior are
covered. Java remains read-only; native lifecycle/query behavior is not inferred
from synthetic or strict-fake tests. See CONTRACT.md and JUNIOR-GUIDE.md.
Five final serial seven-sample reference profiles pass against unchanged source
`29b6942a5b92…`, including exact provider-name/command-ID validation hardening,
with unchanged budgets and explicit shorter retained windows; see
[SCALE.md](SCALE.md). These do not measure recovery/provider calls, command
history memory, network/browser timing or RSS. Older reports retain their own
historical source fingerprints.

The actual-provider lifecycle and final source-matched reference scale gates
are verified above. The actual standalone wheel has 52 byte-matched package
files and zero unconditional runtime dependencies; its Python 3.11.9 core works
with site packages disabled and without importing Pydantic, FastAPI, uvicorn or
HLA/provider packages. Both repository-root rich/limited one-command startup
checks pass with advancing events and clean temporary-port shutdown. Final ZIP
verification and the complete current acceptance audit are recorded above.

## Transport/resumption increment (2026-10-19)

OBS-128 is delivered: discovered WebSocket/SSE/HTTP negotiation, refreshed
discovery, optional named observer-lifetime revisions and confirmed cursors,
same-cut resumption/backfill, explicit foreign/future resets and retained-history
gaps. Clears and metadata changes wake both streams even without new events.
Older lifetime-free HTTP backends remain useful; observer storage identity never
becomes a reporting session, participant join or command permission. Closed
connection callbacks and overtaken HTTP replies cannot resurrect older rows.
Offline/online signals, bounded opening/idle timeouts and retry backoff keep
interruption states explicit. Diagnostics displays transport/lifetime separately.
All stream DTOs are exposed in OpenAPI and generated TypeScript; core use still
has no web/model/RTI dependencies.

Verification: 785 backend/adapter/launcher checks pass, with the opt-in native
provider smoke still skipped. The full 370-case desktop/mobile run passes.
A focused 12-case follow-up adds two older lifetime-free backend cases and
strengthens SSE clear and live REST/SSE path migration checks: 372 unique browser
cases across those runs, not 382. Twenty-three frontend unit checks, strict
typing/lint, production build, no-write schema/type/workspace checks and eight
recorded/live-Python/independent-Java HTTP profiles are covered. Java remains a
read-only HTTP fixture, not a verified Java streaming or RTI implementation.
Python 3.11 still uses lifetime/clear behavior without optional dependencies.
Five fresh serial seven-sample reference workloads pass against source
`78b436e80ffb…`, with unchanged budgets and honest shorter retained windows;
see [SCALE.md](SCALE.md). These timings do not measure stream/network delivery.
These are internal application checks, not normative IEEE RTI evidence.

The recovery increment above delivers OBS-129; the separate real-provider gate
remains active.

## Identity/provenance increment (2026-10-19)

OBS-127 is delivered for explicit observer data: named optional actor attribution,
canonical source/session/namespace/federation/participant/membership keys, alias-
stable roster and selections, independent object contexts/comparisons/charts,
time/declaration/interaction/analytics consumers, and exact alert filters. Nested
MOM/topology subjects never create ordinary actors or command authority. Missing
IDs remain labelled legacy reporting/name-only views; limited RTIs stay useful.
Opaque keys and session IDs survive validation unchanged; malformed known actor
shapes reject atomically. A reporting session end is not participant resignation.

Verification: 772 backend/tool checks pass (one opt-in native smoke skipped).
All 360 desktop/mobile cases are verified: 359 passed in the full run, and the
remaining case passed in a serial retry after a host `ERR_NO_BUFFER_SPACE`
navigation failure before any app assertion. The original failure artifacts
remain available; no test or assertion was relaxed. Rich/limited showcase tests
also prove one reporting view combines ordinary objects, time and declarations
without fabricating join IDs. Sixteen frontend unit checks include the shared
Python/TypeScript key corpus. Eight Python/Java HTTP profiles, strict typing,
generated-contract drift, dependency-free Python 3.11 and packaged schemas are
covered. Seven serial source-matched scale reports retain honest count/byte loss;
five reference profiles pass, including explicit identities, while wide and
large-index remain diagnostic. See [SCALE.md](SCALE.md) for exact retained counts.
The transport increment above delivers OBS-128; OBS-129 and the separate native
gate remain active.

## Saved-workspace increment (2026-10-19)

OBS-126 is delivered: bounded named Pydantic models, generated TypeScript and
precompiled offline validators, optional normalization endpoints, browser-local
draft/named configurations, and explicit import/apply/export. Global settings,
each federate's event/declaration/time preferences, and exact object/perspective
chart choices remain independent. Unsupported choices stay dormant; missing
selections do not silently borrow another participant or report. Captured data,
credentials, backend authority, command drafts and alert rules are excluded.
See [WORKSPACES.md](WORKSPACES.md) for privacy, storage and recovery boundaries.

Verification: the complete 710-check backend/tool suite and 340-check desktop/
mobile suite pass. Thirteen frontend unit checks include 27 shared policy cases,
complete size/uniqueness bounds and no-write schema/validator drift tests. Strict
Python, the production build, Python/Java HTTP corpus, packaged schemas and a
Python 3.11 wheel/core without web/model/RTI dependencies are verified. The full
browser run exposed a cross-participant time-selection regression; per-federate
settings fixed it and repeat runs cover Alice/Bob switching and reload. CI,
the junior/contract guides and playtest packaging include this increment. Both
rich and limited one-command startup checks pass with advancing synthetic events.
The opt-in native-provider smoke remains skipped. OBS-128 through OBS-129 and
the separate native-provider gate still belong to the active overall goal.

## Alert increment (2026-10-19)

OBS-125 is delivered: typed discriminated rules, bounded error-window counts/
uncertainty, exact history-loss baselines, cached pending-command proof and
explicit idle-versus-ended semantics. Separate read/control capabilities keep
limited/read-only backends usable. Conditional backend-instance/revision checks
prevent stale edits and acknowledgements after restart. The Alerts workspace
adds state counts, edit/disable/reset/delete, configuration-only files, searchable
bounded evidence and acknowledgements that never resolve a rule or call an RTI.
Old evidence keeps its original rule generation/filter. See [ALERTS.md](ALERTS.md).

Verification: 688 backend/tool checks pass (one opt-in native smoke skipped);
the complete 308-check desktop/mobile run passes. A final 22-check focused run
also covers four newly added replay/first-paused-view cases and reruns the alert
workflow plus all-screen overflow checks: 312 unique browser checks across the
runs, not 330 unique cases. Strict Python, generated schemas/types, core-only
Python 3.11 and the read-only Java fixture remain covered. Alert configuration/
evidence lifetime is server-process; file export preserves configuration, not
captured observations, credentials or backend URLs. There are now 19 screens.

## Storage verification (2026-10-19)

OBS-124 now includes shared count/encoded-byte evidence budgets, whole-record
expiry, independent roster/object/time bounds, typed storage coverage and
capability-driven frontend diagnostics. See [STORAGE.md](STORAGE.md). Retained
event attribution and pending-request inspection survive latest-cache expiry;
expiry does not imply a resignation, object removal or new RTI incarnation.
The complete browser suite passes all 292 desktop/mobile checks; 662 backend/
tool checks pass, with the opt-in native-provider smoke still skipped. A new
regression closes orphaned pending time tickets on explicit disconnect/resign
and prevents a late grant reopening retained ended work. Python 3.11 still uses
the core without web/RTI imports. The contract, junior and storage guides agree
on the new budgets; the playtest ZIP includes linked guides and measurements.

OBS-124 is delivered within its explicit measured reference envelope. Six new
serial seven-sample profiles share source fingerprint `d434065ee3f8…`; four
reference loads pass. Wide/large-index remain outside that envelope, with usable
bounded states and disclosed expiry, not relabeled full-retention passes.
Historical reports retain their original source fingerprints. See SCALE.md for
actual retained counts, sizes, costs and the limits of this evidence.
Storage verification did not finish the overall goal. The later workspace
increments above deliver OBS-126/127; OBS-128 through OBS-129 and the separate
native-provider gate remain active.

The observer has two related workflows: understand the federation that the
adapter can see, and investigate the observations attributed to one federate.
Management is a third workflow, enabled only when an adapter explicitly offers
commands for a session it controls. Python, Java, and other backends share the
same HTTP models and capability meanings.

## Implemented first increment

| ID | Feature | Delivered behavior |
| --- | --- | --- |
| OBS-001 | Federation workspace | Traffic buckets, stacked participant activity, and event/source/family composition; selectable 30-second, 5-minute, 15-minute, and one-hour windows. |
| OBS-002 | Federate perspective | Persistent roster, participant selection, attributed event timeline, related object observations, and scoped metrics. Association never implies ownership or subscription. |
| OBS-003 | Logical-time view | Per-participant timestamp traces, latest samples, explicit grant-only filtering, and unavailable/partial capability states. Timestamped payloads never imply granted time or RTI time bounds. |
| OBS-004 | Event investigation | Payload search, family/type/source/federate filters, expandable full envelopes, bounded row rendering, and filtered JSON export. |
| OBS-005 | Diagnostics | Adapter status, runtime errors, feature coverage, history capacity, dropped/cleared counts, and stream-gap notices. |
| OBS-006 | Repeatable analytics boundary | Named Pydantic response models, generated TypeScript types, bounded query parameters, consistent snapshot aggregation, explicit attribution and retention coverage. |
| OBS-007 | Live-view resilience | WebSocket reconnect with backoff, periodic snapshot reconciliation, retryable contract loading, stale-analytics notices, and local pause/resume. Pausing a view does not pause the federation. |

Implementation: `src/umbra_rti_observer/analytics.py`, `schemas.py`, `server.py`,
and the sibling `umbra-rti-observer-ui` package. Analytics is additive and
optional; an older backend can continue providing events and inspectors.

## Command increment

OBS-100, OBS-101 and the connect/join/resign/disconnect slice of OBS-103 are
implemented. The Actions tab uses named request/parameter/result models,
capability-driven controls, per-session execution ownership, confirmation,
request-ID deduplication, timeout/late-result reporting and bounded history.
The synthetic showcase has completed the full lifecycle in the browser,
including retaining history after a page refresh. New command Python modules
run under strict Pyright checking.

OBS-102's Umbra lifecycle driver is implemented and verified with strict fake
ambassadors, including factory/call/callback-pump thread ownership and known
membership-error recovery. The opt-in real 2025 lifecycle smoke now passes as
recorded above, including actual typed rejection, state transitions, cleanup and
owner-thread execution. This is internal adapter evidence, not IEEE conformance
or proof that the broader native RTI suite passes.

**Active goal: complete the federation visualization backlog.** OBS-110's
explicit relationships are delivered with scoped participant/join identities,
typed backend/generated frontend models, independent capability, bounded
snapshots, provenance, reported lifecycle and Python/Java/browser evidence.
The Topology tab draws only explicit directed edges, with search, filters,
keyboard node focus and a complete paged evidence table. The graph limit and
index loss are disclosed. No routing, ownership or delivery is inferred.
OBS-104 / OBS-105 read-only coordination workspaces are also implemented:
independent synchronization/persistence capabilities, explicit operation IDs,
separate local/federation reports, participant progress/failure reasons and
bounded persistent phase sequences. Search/filter/follow-operation, retained
report distributions, pause/clear and loss notices use the generated contract
unchanged with Java. No collective-management commands are advertised.
OBS-112 explicit interaction flow is implemented: bounded named send/receive
records, exact transmission/namespace/federation/endpoint-join pairing, original
reporter provenance, uncorrelated legacy/no-ID counts, duplicate ambiguity,
explicit order and dynamic monotonic observer-interval charts. Receive-first
reports do not invent intervals. Python/Java/no-analytics fixtures and desktop/
mobile tests travel with the generated contract.
OBS-114 / OBS-115 are implemented: explicit owner state, unresolved owner-handle
metadata, acquire/divest ticket evidence, bounded history and independent
`ownership_inspection`; exact decimal region ranges, scoped references,
associations and declared/observed routing under `regions_inspection`.
Neither service completion nor publishers/updaters imply an owner; neither
extent overlap nor matching names imply routing or delivery. Legacy provider
flags retain their earlier meanings. Python/Java and desktop/mobile tests
exercise unknown/partial/older/disabled data, expiry, clear and pause.
OBS-116 / OBS-117 are implemented: independent typed MOM member/service/counter
reports and adapter-supplied FOM class/field/datatype/dimension metadata.
Management coverage never replaces ordinary membership, service evidence never
becomes command authority, and wide/opaque counters never become rounded
numeric samples. FOM references stay within explicit reporting/model/revision
scope; missing/cyclic parents, unknown ancestry/fields and declared/effective
field scope remain distinct. No provider FOM parsing or runtime-byte decoding
enters the generic observer. Both use strict named/generated models, independent
latest/history bounds and loss, search/filter/navigation, clear/pause and
Python/Java/desktop/mobile evidence. Limited FOM metadata remains useful without
MOM or referenced datatypes; legacy `mom` keeps its earlier meaning.

OBS-121 is implemented: a bounded, versioned state/envelope journal with exact
initial contract/capability/context/inspector provenance, atomic batch arrivals,
metadata/clear boundaries and idle clock checkpoints. Capacity stops recording,
not collection; files disclose earlier history loss and uncaptured over-budget
tails. Strict named models validate import/export, finite values, bounds,
retention/cursor continuity and no event rewriting/resurrection. Replay has a
monotonic play/speed/time/exact-state clock, persistent banner, captured-time
analytics and no live subscriptions, captured URL following or provider command
execution. Process-local shared capture/replacement and sensitive-file lifetime
are explicit. Limited/no-analytics/HTTP-only fallback and the portable full-model
corpus are covered; the Java static fixture does not pretend to capture/import.

OBS-124's original checkpoint used six source-fingerprinted profiles to measure
ingress, snapshots, model validation/serialization, analytics, allocations and
recording limits. Four pass the explicit Python snapshot/analytics reference
envelope; two diagnostic loads expose oversized states, rather than being
reported as passes. Exact analytics and health/metrics projections avoid
unrelated inspector walks. Scoped object baselines now have an independent
5000-record LRU budget, explicit loss/omission metadata and paged UI coverage.
The rich analytics median fell from 137.09 to 17.45 ms on the measured host,
without event sampling or changed attribution. See [SCALE.md](SCALE.md).

Compact recording is also delivered: negotiated v2 baseline/edit files preserve
every accepted state, batch, boundary and provenance, with strict named edits,
full reconstructed-model validation, explicit byte/state/work limits and old v1
defaults/files. An independent TypeScript worker expands only the current state;
seek/loading/error behavior never substitutes live data or grants provider
authority. See [RECORDING.md](RECORDING.md) and the richer comparison in SCALE.

**Next concrete handoff: separate native-provider triage, not an open observer row.** The
reference visualization/portability rows through OBS-129 and the explicit
OBS-102/103 native lifecycle gate now have the current application, scale and
packaging evidence above. A future RTI-focused task can start by reproducing
`test_native_floating_time_edges_match_reference_factory` against its official
API lane, before changing native semantics or old test expectations. Preserve
unrelated RTI/C++ work; broader native failures are not silently absorbed into
this observer goal.
Recovery's injected inspector remains optional and still needs separate real
query evidence before being advertised by a native binding.

The storage visualization checkpoint verified 662 backend/tool checks and 292 desktop/mobile
browser checks, including independent Java models, handle-only ownership,
exact large/maximum bounds, MOM report isolation, scoped/cyclic FOM references
and legacy provider-flag compatibility. Its 18 screens passed mobile overflow checks;
the newer alert checkpoint above covers all 19.
Four pure replay/codec checks cover exact seek, tied offsets, own-key safety and
bounded current-state decoding. Recording checks
include concurrent batches, finite/bounded imports, clock changes, clear
boundaries, command isolation and limited/no-analytics models. Compact capture
adds malformed edits/work/state rejection, inspector provenance, parser depth,
legacy negotiation, worker failure recovery and eighteen missing-wire guards
across v1, v2 baselines and v2 edited states. Scale regressions
include exact projection equivalence, lightweight legacy request fallback,
20,000 rotating observations, explicit scope expiry and old unknown metadata.
The latest complete browser run used one worker after a Windows socket-buffer
failure interrupted a stylesheet in the parallel run; missing stylesheets now
have an explicit fixture diagnostic rather than appearing only as layout loss.
Ruff, Pyright, portable-schema and generated-TypeScript drift
checks pass. Python 3.11 still imports and uses the core without web/RTI
dependencies. Rich and limited playtest startup checks pass; the local playtest
ZIP is refreshed. The opt-in native-provider smoke is skipped, not satisfied.
These are internal application checks, not normative RTI coverage. This is
progress under the active goal, not completion of the remaining backlog.

OBS-110 first increment: named object/interaction declaration inputs and
capacity-bounded persistent snapshots, full source/session/federation/federate
scope, explicit publication/subscription direction and declared/withdrawn/unknown
status. Selected (including empty), all, unknown and not-applicable attribute
scopes remain distinct. Complete reports replace sets rather than merging
partial provider calls. Declarations and per-federate views add searchable,
filtered tables with provenance, pause/clear, row limits and loss notices.
The optional model works without MOM, time or analytics; older/disabled/
unsupported/partial backends keep independent views. Topology now uses the
separate explicit relationship model described above.
Verified coverage is now 211 backend/tool checks and 94 desktop/mobile browser
checks; the optional native-provider smoke remains skipped.

OBS-113 / OBS-120 are delivered for explicit observer data: persistent scoped
grant/request/state snapshots, lookahead/role flags/reported bounds, independent
bounded request history, monotonic pending ages and matched-grant delay charts.
Full source/session/federation/federate + request IDs are required for matches;
missing IDs, duplicate retained IDs, terminal outcomes and expired records never
invent successful completions. Opaque/unreported numeric times remain unknown
without blocking explicit-ID correlation. MOM and analytics are optional.
Grant-only, older, disabled and partial backends keep useful independent views.
Python/Java fixtures, strict models, generated types and HTTP/stream/browser
regressions travel together. Duplicate React sibling keys were corrected so
switching federates cannot leave the previous participant's time panel visible.
This is observer application evidence, not native RTI time-service conformance.

OBS-111 is delivered: Objects search/filtering, persistent global and attributed
snapshots, same-perspective previous-value comparisons, unknown-versus-null
handling, byte/representation labels and selectable finite-number charts.
History is bounded by event retention; snapshots outlive event eviction but
scoped baselines also have an independent disclosed capacity. Rediscovery after
removal or scope expiry starts a fresh local baseline, and clear/restart resets
snapshots. Scope expiry is not evidence of RTI object removal or a new incarnation.
Federates no longer substitutes global values for attributed ones. The additive
capability works without analytics or MOM and degrades explicitly when absent
or disabled. Python/Java fixtures, schemas, generated types and regressions
travel together. Stable cross-producer/name-reuse identities remain OBS-127;
the local generation cursor is not that identity.

| ID | Priority | Status | Feature | Acceptance criteria / dependencies |
| --- | --- | --- | --- | --- |
| OBS-100 | P0 | Done | Typed command catalog and dispatcher | Named discriminated requests and parameters, registered target scope, availability/reasons, accepted/running/succeeded/failed/unavailable results, retained-ID deduplication, bounded history, timeouts and late outcomes. Unsupported commands never enter a provider. |
| OBS-101 | P0 | Done | Command history and action workspace | Catalog-driven fields and action states; confirmed resign/disconnect, request/result details, unknown effects, late results and history that survives browser refresh. History lasts for the backend process and reports eviction. |
| OBS-102 | P0 | Lifecycle verified | Umbra command adapter | Strict-fake and installed real 2025 lifecycle checks pass: factory, evoked pump and lifecycle calls share one owner thread; typed rejection preserves connected state. Configuration/credentials/allowlists stay server-owned. Native inspection is not advertised; the broader native-suite failures above remain explicit. |
| OBS-103 | P1 | Showcase and native lifecycle verified | Observer federate lifecycle | Synthetic browser and real owned connect/join/resign/disconnect flows pass, including audit states and unique federation cleanup. No create/destroy command is advertised. Applications register only owned sessions and explicitly supported actions; other adapter shapes implement the CommandSession seam. |
| OBS-104 | P1 | Read-only delivered | Synchronization workspace | Typed labels/operation IDs, registration outcomes, announcement/achievement/completion reports and failed participants, separate local/federation scopes, retained sequences and coverage. No register/achieve command is advertised; future controls need explicit owned scope. |
| OBS-105 | P1 | Read-only delivered | Save/restore workspace | Typed requested/begun/completed/failed reports and reasons; local completion is never promoted to collective completion. Separate latest/transition capacities, timeline/filtering and Python/Java/browser evidence. Save/restore commands are not advertised. |

## Investigation and richer graphs

| ID | Priority | Feature | Acceptance criteria / dependencies |
| --- | --- | --- | --- |
| OBS-110 | P1 | Publication/subscription and topology explorer — delivered | Typed scoped declarations; explicit adapter-supplied relationship IDs/endpoints with separate capability, provenance, reported lifecycle and loss. Search/filter, keyboard node focus, bounded graph and paged evidence table; no edges inferred from matching names, declarations or delivery. Python/Java and desktop/mobile verified. |
| OBS-111 | P1 | Object and attribute workspace — delivered | Object/class/handle search, lifecycle filtering, independent global/attributed snapshots beyond history eviction, scoped comparisons and explicit opaque bytes; user-selected finite-number charts. Optional named models/capability; no global substitution on older/disabled backends. |
| OBS-112 | P1 | Interaction flow — delivered | Exact explicit transmission/namespace/federation/endpoint-join pairing; missing identities remain counted/unpaired, duplicates ambiguous, receive-first intervals unknown. Independent reporter provenance, receive-order/timestamped/unknown, bounded records/loss and dynamic monotonic observer-interval chart; no delivery ratio or network-latency inference. Python/Java/desktop/mobile verified. |
| OBS-113 | P1 | Time request/grant analysis — delivered | Explicit scoped IDs, bounded pending/outcome ledger, monotonic observer delay charts and honest unknown/partial values. No request, grant, role or bound inference; no MOM/analytics dependency. |
| OBS-114 | P1 | Attribute ownership view — delivered | Explicit known/unowned/RTI/unknown state and unresolved name/handle metadata, acquire/divest reports and exact scoped tickets. Duplicate ambiguity, independent capacities/loss and generated-model tables/chart/history; no owner inferred from requests, publisher, updater or source. Optional inspection capability and Python/Java/browser evidence. |
| OBS-115 | P2 | Region/DDM inspector — delivered | Exact decimal ranges, extents, associations, scoped references and declared/observed/unknown routing reports. Independent latest/history bounds, range diagrams, search/filter, clear/pause and loss. No overlap/delivery inference; legacy flags unchanged; Python/Java/browser evidence. |
| OBS-116 | P2 | MOM workspace — delivered | Typed member/service/counter evidence with explicit coverage, unresolved name/handle provenance, numeric/opaque representations and exact-series charts; independent loss, search/filter and pause/clear. Never replaces ordinary roster or command authority; separate optional capability, Python/Java/browser evidence. |
| OBS-117 | P2 | FOM browser — delivered | Explicit revision-scoped class/attribute/parameter/datatype/dimension metadata; search, filters, parent/child/type navigation and bounded evidence. Unknown/empty/removed and declared/effective scopes remain distinct; missing/cyclic references visible, exact bounds preserved, no provider FOM parsing/decoding. Independent capability and Python/Java/browser evidence. |

## Operational quality and portability

The playtest startup increment is delivered: repository-root Windows launchers,
`python tools/observer_playtest.py`, a private reusable environment, cached UI
builds, advancing-event readiness checks, safe busy-port fallback, rich/limited
profiles and a small prebuilt-dashboard ZIP. The ZIP needs Python 3.11+ and
first-run downloads, not Node, native bindings or an RTI. Focused launcher tests
cover setup recovery, paths with spaces, bundle contents, readiness and process
cleanup. See [SHOWCASE.md](SHOWCASE.md) for the Miguel-facing flow. The next
handoff is recorded above; startup checks do not replace UI regressions.

OBS-123 is delivered with 16 browser scenarios in desktop and 390px mobile
projects (32 checks), using a fresh synthetic FastAPI backend per test. Coverage
includes all seven screens, chart controls, keyboard/history navigation,
participant isolation, filters/payloads/JSON export, bounded rows, clear-history
reconciliation, WebSocket reconnect without duplicate rows, lifecycle commands
and confirmation, refreshed history, pause safety, stale/unsupported controls,
and limited/read-only/no-analytics/empty backends. Named panels and explicit
empty inspector messages improve accessibility and avoid ambiguous blank views.
The separate observer CI workflow runs the Python suite and browser projects;
failures retain traces, screenshots and server logs. This is internal
application evidence, not normative RTI coverage. Broader accessibility audits,
other browser engines and scale testing are not implied by these checks.

OBS-122's HTTP-read conformance increment is delivered: the strict checker uses
named validated models, required wire collections, version/media headers,
discovered same-origin paths, optional-resource checks, retention/cursor rules
and analytics invariants. Seven deterministic profiles run against recorded
responses, live Python and an independent JDK 17 HTTP fixture. Negative cases
reject malformed payloads and dishonest capability discovery. A portable JSON
Schema bundle and no-write TypeScript drift checks run in CI. The browser suite
now includes Java limited/partial/empty/no-analytics backends, relocated REST
paths and version rejection (44 desktop/mobile checks total).

OBS-111 extends this to 58 desktop/mobile checks, including all eight screens,
shared-object scope isolation, snapshots beyond eviction, bytes and numeric
selection, known-null comparisons, object search/status/clear, pause safety,
Java object models and older/disabled capability fallback. HTTP and stream
regressions preserve required nullable scope/value fields under serialization.

OBS-113 / OBS-120 add persistent scoped time inspection, request correlation
and delay analysis (76 desktop/mobile checks total; 178 backend/tool checks).
New desktop/mobile scenarios cover scope isolation, snapshot
eviction/clear, pause, duplicate IDs, outcomes, separate reconnect sessions,
Java correlated/grant-only/retention-loss fixtures, and older/disabled/partial
features. Numeric time can remain unknown. Backend tests also cover clock
changes, capacity, atomic validation and exact HTTP/stream model serialization.

The frontend now follows discovered REST/WS paths and uses HTTP polling without
socket retries when WS is unsupported. This delivers the static HTTP-only part
of OBS-128, not SSE selection or all transport-transition cases. The Java server
is a read-only fixture, not a production Java federate or command/stream
implementation. HTTP-read reports explicitly list these unverified scopes;
they are not proof of RTI conformance.

| ID | Priority | Feature | Acceptance criteria / dependencies |
| --- | --- | --- | --- |
| OBS-120 | P1 | Persistent federate time inspector — delivered | Latest explicit grants/state and pending requests survive event eviction, scoped by source/session/federation/federate, with provenance and independent request-ledger coverage. Clear/restart resets snapshots. |
| OBS-121 | P1 | Recording and replay — delivered | V1 complete-state and negotiated v2 baseline/edit files; every reconstructed state uses strict named models with independent encoded/state/work/operation bounds. Atomic arrivals, explicit loss/clear/checkpoints and process lifetime; worker seek with coherent pending/error states, captured-time analytics, no captured URL/provider command/live substitution. Legacy defaults, import/body/error bounds, conformance and desktop/mobile/limited/no-analytics/full-inspector evidence. |
| OBS-122 | P1 | HTTP-read contract conformance (done) | Eight shared profiles against recorded/live-Python/independent-Java responses, negative payload/discovery checks, portable schema and no-write generated-type drift gates. Command execution and stream semantics remain separate lanes. |
| OBS-123 | P1 | Frontend regression suite (done) | All 384 current desktop/mobile browser checks pass across the workspaces, portability/degradation, independent Java fixtures, pause/replay, controls, streaming and saved configuration. Run `pnpm test:e2e`; no native RTI required. |
| OBS-124 | P1 | Analytics scale and retention — delivered | Explicit measured reference envelope; exact projections, UI paging, compact recording and shared count/encoded-byte budgets for every collection. Whole-record loss, independent roster/object/time expiry, orphaned tickets and empty-window cursors have typed/generated and limited-backend UI evidence. Five final source-matched reference profiles pass; historical wide/large-index diagnostics retain their own fingerprints and honest non-pass limits. Not RSS, arbitrary-input/full-duration retention or provider/SLA evidence. |
| OBS-125 | P2 | User-defined alerts — delivered | Strict versioned rules for stale observations, bounded error windows, exact dropped-history baselines and cached pending-command ages/IDs; explicit unknown/partial/idle/ended states. Process-local configuration/evidence, original rule generations, acknowledgements and configuration-only export/import; conditional instance/revision guards, independent read/control capabilities, Python/Java/browser evidence and no replay/live substitution or provider calls. |
| OBS-126 | P2 | Saved workspaces — delivered | Versioned named models, generated/offline validation, bounded browser drafts/profiles and explicit files/apply. Global and per-federate settings, exact numeric charts and windows persist independently; unavailable choices stay dormant. Corrupt/quota/conflicting storage, safe reset, delayed imports, pause/replay, limited Java and privacy have Python/browser evidence; CI, guides, wheel and playtest schemas ship together. |
| OBS-127 | P2 | Federate identity and provenance — delivered | Named reporting/join models reach roster, object contexts/comparisons, time, declarations, interactions, analytics, UI keys/preferences and exact alerts. Same-name joins, aliases, opaque keys/handles, legacy fallback, paused/replay views, bounded lists and independent Java fixtures have Python/browser evidence. Source-matched scale reports disclose shorter retained windows; no MOM or native identity guarantee is implied. |
| OBS-128 | P2 | Streaming negotiation — delivered | Discovered WebSocket/SSE/HTTP fallback, refreshed discovery, named observer lifetime and delivered cursors, atomic resumption/backfill, clear/loss/reset handling and old-backend fallback. Bounded retry/watchdogs, offline/online recovery, stale-reply rejection, diagnostics, generated DTOs and desktop/mobile/Java HTTP evidence preserve independent RTI capabilities and authority. |
| OBS-129 | P1 | Unknown-session recovery — delivered | Optional typed owned-driver inspection and confirmed fresh reconciliation, conditional target incarnation/revision, healthy pump fence, explicit incomplete/unsupported states and retained failures/late results. Owner-thread, malformed-proof, timeout and desktop/mobile complete/partial/disabled workflows are verified; no automatic retry, replacement driver, MOM/roster authority or fabricated universal/native query. |

## Delivery rules

- Every increment ships together: named boundary models, documented wire shape,
  capability behavior, generated frontend types, UI states, and focused tests.
- Empty, unavailable, unknown, partial, stale and truncated have different
  meanings. A zero on a graph must not stand for unsupported data.
- Aggregates report the retained observation window. Persisted live inspectors
  and complete federation membership have separate provenance and lifetime.
- Commands never call provider libraries from React. Backends own authority,
  validation, dispatch, execution state and provider-error translation.
- No new provider imports enter the standalone observer package. RTI-specific
  adapters own binding shapes and version differences.

### Current handoff: optional native coordination adapter

Resume OBS-133 at `umbra-rti-observer-umbra/commands.py`: choose one supported
coordination service, inspect its official 2025 binding/callback shape, then add
explicit opt-in owner-thread mapping and actual provider evidence. Use
`CoordinatingSessionDriver` and named requests/receipts; observation or MOM
capabilities cannot enable controls. Publish scoped callback reports separately
from local service return. The synthetic executor is a reference workflow.

The earlier native lifecycle gate and twelve broader native failures remain
documented above. This application increment creates no Requirements Lab
revision or normative mapping. Broader native triage can independently resume
the exact floating-time failure named in the earlier gate.
