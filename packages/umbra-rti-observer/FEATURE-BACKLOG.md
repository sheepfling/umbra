# Observer feature backlog

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
The opt-in native-provider smoke remains skipped. OBS-127 through OBS-129 and
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
increment above delivers OBS-126; OBS-127 through OBS-129 and the separate
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
membership-error recovery. Live native-provider verification remains pending;
the opt-in `test_native_control_smoke.py` supplies that check when a built 2025
extension is available. This is internal adapter evidence, not IEEE conformance.

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

**Next concrete handoff: OBS-127 identity/provenance migration.** Start with a
named, optional event-attribution model and the ordinary roster/analytics path.
Reuse `identities.py`'s explicit namespace/federation/participant/membership IDs;
keep source/session reporting provenance distinct from participant identity.
`core.py` currently keys `_federates` by name, and `analytics.py` merges roster/
observations by that same name. Names/handles and nested MOM/topology subjects
must never synthesize ordinary attribution or authorize commands. Prove same-name
participants across producers/federations, alias changes within one join, reused
membership IDs in different namespaces and a fresh membership on rejoin.
Carry the migration through object/time keys, analytics, frontend selections,
saved preferences, alerts and recording validation; keep missing-ID legacy
coverage explicit and limited RTIs useful. Deliver models, generated schemas,
Python/Java/browser evidence and guides together. Then continue OBS-128 transport
negotiation/resumption and OBS-129 owned-session recovery; real-provider commands
and OBS-102's native smoke remain separate gates.

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
| OBS-102 | P0 | Native check pending | Umbra command adapter | Provider-specific driver and strict fake integration pass. Factory, evoked callback pump and all lifecycle calls share one owner thread. Configuration/credentials and federation allowlists remain server-owned. Run the opt-in native smoke before completing this row. |
| OBS-103 | P1 | Showcase verified; native check pending | Observer federate lifecycle | Working synthetic connect/join/resign/disconnect flow; real translation implemented under OBS-102. No create/destroy command is advertised. Applications register only owned sessions and explicitly supported actions; other adapter shapes implement the CommandSession seam. |
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
handoff is now OBS-127's identity/provenance migration; startup
checks do not replace UI regressions.

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
| OBS-122 | P1 | HTTP-read contract conformance (done) | Seven shared profiles against recorded/live-Python/independent-Java responses, negative payload/discovery checks, portable schema and no-write generated-type drift gates. Command execution and stream semantics remain separate lanes. |
| OBS-123 | P1 | Frontend regression suite (done) | 32 desktop/mobile browser checks cover navigation, charts, perspective isolation, pause/resume, filters/export, empty and unavailable states, lifecycle/history, retention clearing, reconnect, and keyboard/mobile behavior. Run `pnpm test:e2e`; no native RTI required. |
| OBS-124 | P1 | Analytics scale and retention — delivered | Explicit measured reference envelope; exact analytics/runtime projections, UI paging, compact recording and shared count/encoded-byte budgets for every observation collection. Whole-record loss, independent roster/object/time expiry, orphaned tickets and empty-window cursors have typed/generated coverage and limited-backend UI behavior. Six serial current profiles, four reference passes, two honest diagnostic cases with bounded delivery and disclosed shorter retention. Not RSS, arbitrary-input/full-duration retention or provider/SLA evidence. |
| OBS-125 | P2 | User-defined alerts — delivered | Strict versioned rules for stale observations, bounded error windows, exact dropped-history baselines and cached pending-command ages/IDs; explicit unknown/partial/idle/ended states. Process-local configuration/evidence, original rule generations, acknowledgements and configuration-only export/import; conditional instance/revision guards, independent read/control capabilities, Python/Java/browser evidence and no replay/live substitution or provider calls. |
| OBS-126 | P2 | Saved workspaces — delivered | Versioned named models, generated/offline validation, bounded browser drafts/profiles and explicit files/apply. Global and per-federate settings, exact numeric charts and windows persist independently; unavailable choices stay dormant. Corrupt/quota/conflicting storage, safe reset, delayed imports, pause/replay, limited Java and privacy have Python/browser evidence; CI, guides, wheel and playtest schemas ship together. |
| OBS-127 | P2 | Federate identity and provenance — migration in progress | The topology foundation is delivered. Additive identity-scoped roster, object, time, declaration and analytics models are implemented with focused backend checks; frontend consumers, generated artifacts and end-to-end migration are not yet complete. This checkpoint is not a release. |
| OBS-128 | P2 | Streaming negotiation (partial) | Discovered REST/WS paths and unsupported-WS polling fallback are delivered. SSE selection, cursor resumption, refresh of cached discovery after capability/backend changes and dynamic transport transitions remain; test without duplicate or resurrected events. |
| OBS-129 | P1 | Unknown-session recovery | Add an adapter-supported inspection/recovery path for unclassified provider failures. Reconcile actual connection/membership before enabling actions; preserve owner thread, target generation and previous command history. The current controller conservatively disables an unknown session until application/operator recovery. |

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

### Work-in-progress checkpoint: OBS-127

The saved-workspace increment above is complete. The following identity migration
is deliberately checkpointed before completion: exact source/session/namespace/
federation/participant/membership keys isolate joins; aliases do not create new
joins, and a reporting session ending does not prove participant resignation.
Explicit object contexts remain separate from legacy unscoped observations.

Current focused checks: 60 backend identity/workspace/object tests and eight
offline workspace tests pass. The frontend type check currently fails at the
two `participantLabel` call sites in `FederateView.tsx`: `TimeWorkspace` and
`DeclarationsView` have not yet adopted the new props. This is not a new
full-suite, browser, native-provider or performance validation result. The local
playtest ZIP and wheel still represent the earlier verified OBS-126 build.

Resume by completing identity-key selection/filtering across objects, time,
declarations, federation charts, inspectors and alerts, preserving readable
labels and explicit unresolved legacy choices. Then regenerate/check the wire
schema, frontend types and shared Python/Java fixtures; add duplicate-name,
alias, rejoin and older-backend browser evidence; rerun the relevant regression
and scale checks before marking OBS-127 delivered or refreshing showcase builds.

Continue with OBS-127 identity/provenance migration, then the remaining operational/portability rows.
Complete the OBS-102
native verification gate when its built provider is available; never substitute
synthetic observations for real-provider verification.
