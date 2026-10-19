# Observer feature backlog

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

**Next handoff: OBS-110 explicit relationships.** The declaration snapshot and
searchable explorer increment is delivered. Next define adapter-supplied stable
relationship IDs and explicit producer/recipient endpoints, with independent
capability, provenance, lifecycle/coverage and Java migration fixtures. Only
then draw relationship/topology edges; do not pair matching class names or
infer delivery, ownership or DDM routing from declarations. OBS-102's native
smoke remains a separate gate for real-provider controls.

OBS-110 first increment: named object/interaction declaration inputs and
capacity-bounded persistent snapshots, full source/session/federation/federate
scope, explicit publication/subscription direction and declared/withdrawn/unknown
status. Selected (including empty), all, unknown and not-applicable attribute
scopes remain distinct. Complete reports replace sets rather than merging
partial provider calls. Declarations and per-federate views add searchable,
filtered tables with provenance, pause/clear, row limits and loss notices.
The optional model works without MOM, time or analytics; older/disabled/
unsupported/partial backends keep independent views. No topology is claimed.
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
History is bounded by event retention, snapshots outlive eviction, rediscovery
after removal starts a fresh local baseline, and clear/restart resets snapshots.
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
| OBS-104 | P1 | Planned | Synchronization workspace | Show synchronization labels, registration outcomes, announcement/achievement state, and failed participants when reported. Register/achieve actions appear only for supported command scopes. Requires OBS-100; read-only observations can ship first. |
| OBS-105 | P1 | Planned | Save/restore workspace | Observe request/begun/completed/failed phases with labels and reasons. Distinguish local and federation-wide completion. Commands require explicit persistence support. Requires OBS-100. |

## Investigation and richer graphs

| ID | Priority | Feature | Acceptance criteria / dependencies |
| --- | --- | --- | --- |
| OBS-110 | P1 | Publication/subscription explorer — snapshots/explorer delivered; explicit relationships next | Typed scoped declarations, distinct attribute scopes, search/filter and capacity coverage. No topology edges inferred; adapter-supplied relationship IDs/endpoints and lifecycle remain. |
| OBS-111 | P1 | Object and attribute workspace — delivered | Object/class/handle search, lifecycle filtering, independent global/attributed snapshots beyond history eviction, scoped comparisons and explicit opaque bytes; user-selected finite-number charts. Optional named models/capability; no global substitution on older/disabled backends. |
| OBS-112 | P1 | Interaction flow | Correlate send/receive observations only using adapter-supplied message IDs and explicit endpoints. Show counts without claiming delivery or latency when correlation is absent. Works with receive-order and timestamped traffic. |
| OBS-113 | P1 | Time request/grant analysis — delivered | Explicit scoped IDs, bounded pending/outcome ledger, monotonic observer delay charts and honest unknown/partial values. No request, grant, role or bound inference; no MOM/analytics dependency. |
| OBS-114 | P1 | Attribute ownership view | Show owner/unknown, acquisition/divestiture transitions, and pending requests from explicit ownership observations. No inference from the publisher, last updater, or source name. Optional ownership capability. |
| OBS-115 | P2 | Region/DDM inspector | Region extents, dimension ranges, associations, and routing observations; clear declared-versus-observed distinction. Optional regions capability. |
| OBS-116 | P2 | MOM workspace | Typed management reports, member roster provenance, service invocation/error reports, and provider counters. Keep MOM optional; ordinary roster, time, attributes and join flow remain usable without it. |
| OBS-117 | P2 | FOM browser | Adapter-supplied class/attribute/interaction/datatype metadata; search and schema navigation. The generic observer must not parse provider FOMs or introduce domain-specific class meanings. |

## Operational quality and portability

The playtest startup increment is delivered: repository-root Windows launchers,
`python tools/observer_playtest.py`, a private reusable environment, cached UI
builds, advancing-event readiness checks, safe busy-port fallback, rich/limited
profiles and a small prebuilt-dashboard ZIP. The ZIP needs Python 3.11+ and
first-run downloads, not Node, native bindings or an RTI. Focused launcher tests
cover setup recovery, paths with spaces, bundle contents, readiness and process
cleanup. See [SHOWCASE.md](SHOWCASE.md) for the Miguel-facing flow. The next
handoff is now OBS-110 explicit relationships; startup checks do not replace UI regressions.

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
| OBS-121 | P1 | Recording and replay | Versioned export includes contract, capability manifest, context, and envelopes. Replay has deterministic clock/speed/seek and is visibly distinguished from a live connection. |
| OBS-122 | P1 | HTTP-read contract conformance (done) | Seven shared profiles against recorded/live-Python/independent-Java responses, negative payload/discovery checks, portable schema and no-write generated-type drift gates. Command execution and stream semantics remain separate lanes. |
| OBS-123 | P1 | Frontend regression suite (done) | 32 desktop/mobile browser checks cover navigation, charts, perspective isolation, pause/resume, filters/export, empty and unavailable states, lifecycle/history, retention clearing, reconnect, and keyboard/mobile behavior. Run `pnpm test:e2e`; no native RTI required. |
| OBS-124 | P1 | Analytics scale and retention | Profile bounded-store snapshots and response sizes; establish a measured event-volume target. Add incremental aggregates/downsampling only when required, preserving window coverage and attribution semantics. |
| OBS-125 | P2 | User-defined alerts | Explicit thresholds for stale observations, errors, dropped history, and pending commands. Distinguish an idle participant from one confirmed disconnected. Store alert configuration and evidence. |
| OBS-126 | P2 | Saved workspaces | Persist selected tabs, participant, filters, numeric charts, and time window; share configuration without embedding captured federation data or credentials. |
| OBS-127 | P2 | Federate identity and provenance | Add explicit stable IDs, display names, handles, producer/recipient viewpoint, and optional federation IDs. Test name reuse, rejoin, and multiple producers without merging unrelated participants. Requires additive identity models and migration fixtures. |
| OBS-128 | P2 | Streaming negotiation (partial) | Discovered REST/WS paths and unsupported-WS polling fallback are delivered. SSE selection, cursor resumption and dynamic transport transitions remain; test without duplicate or resurrected events. |
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

Continue with object exploration (OBS-111), then time analysis (OBS-113/120). Complete the
OBS-102 native verification gate when its built provider is available.
