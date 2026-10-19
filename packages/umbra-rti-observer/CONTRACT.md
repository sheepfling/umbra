# Observer wire contract

`umbra-rti-observer` has two intentionally separate boundaries:

```text
provider callbacks -> language/provider adapter -> normalized events
                                               -> observer backend
                                               -> versioned HTTP/stream contract
                                               -> repeatable dashboard frontend
```

The Python FastAPI service is the reference backend, not a frontend
dependency. A Java federate service, another Python service, or a different
RTI integration can replace it if it implements the contract below.

## Version and discovery

The contract identifier is `umbra-observer-contract-v1`. A backend exposes it
from `GET /api/contract` and in the `contract_version` field of every state
snapshot. HTTP responses also carry:

```text
X-Umbra-Observer-Contract: umbra-observer-contract-v1
X-Umbra-Observer-Media-Type: application/vnd.umbra.observer+json
```

The dashboard checks `/api/contract` before consuming state. This makes an
incompatible backend fail clearly instead of producing a partially populated
screen.

The maintained dashboard implementation lives in the separate
`umbra-rti-observer-ui` React/TypeScript package. The reference FastAPI backend
publishes `/openapi.json`; the UI uses `openapi-typescript` to refresh its
checked-in REST types. WebSocket/SSE messages use the same contract models but
are kept in a small adjacent stream-type module because WebSocket operations
are not represented by OpenAPI. This gives the browser compile-time types
without making the Python implementation the frontend's runtime dependency.

The backend-independent [conformance lane](conformance/README.md) exports a
portable JSON Schema bundle and checks the same wire expectations against
Python and Java. The React client rejects incompatible versions before using
state and follows discovered same-origin resource paths. Discovered paths are
absolute paths without query strings or fragments; clients add resource-specific
query parameters. They must never direct commands or observations to another
origin. `/api/contract` and `/api/health` remain fixed bootstrap resources.

## Capability discovery

The contract is stable even when the connected RTI is not feature-complete.
`GET /api/contract` and every state snapshot advertise a `capabilities`
mapping. Capability names are generic and their values are descriptive:

```json
{
  "logical_time": {
    "state": "available",
    "label": "Logical time",
    "description": "Time regulation, constraints, grants, and timestamped delivery.",
    "owners": ["provider"],
    "sources": ["umbra-python-rti"]
  },
  "mom": {
    "state": "unsupported",
    "reason": "The connected provider does not expose management callbacks."
  }
}
```

Valid states are `available`, `partial`, `disabled`, `unsupported`, and
`unknown`. `available` means the feature is supported and enabled for
observation; `partial` means only part of the surface is observed. A frontend
must not treat `unsupported` or `disabled` as an empty result. It should keep
the rest of the dashboard usable and label that feature accordingly.

Provider adapters own RTI capability declarations. The observer backend owns
transport capabilities such as event history, metrics, HTTP, WebSocket, and
SSE. This keeps a provider limitation from disabling unrelated observer
features.

The frontend opens a WebSocket only when that capability is `available` or
`partial` and a safe path is advertised. HTTP-only backends use snapshot polling
without repeatedly attempting an unsupported socket. SSE selection and dynamic
transport transitions remain a separate streaming-negotiation increment.

## Normalized event envelope

Every event contains these fields:

```json
{
  "sequence": 42,
  "event_type": "object.updated",
  "family": "generic",
  "source": "provider-adapter",
  "observed_at": 1760000000.123,
  "object_key": "object-17",
  "attributes": {"state": "ready"}
}
```

`sequence` is monotonically increasing within one observer backend. `family`
is explicit caller metadata; the dashboard never infers domain meaning from a
provider class name. Unknown JSON-compatible fields are retained.

The reference event vocabulary is returned by `/api/contract` and is
extensible. A backend may publish additional event types without requiring a
frontend change as long as the envelope remains valid.

## Snapshot resource

`GET /api/state?include_events=true` returns the complete dashboard snapshot:

```json
{
  "contract_version": "umbra-observer-contract-v1",
  "schema_version": "umbra-runtime-observer-v1",
  "status": "running",
  "context": {},
  "live_metrics": {},
  "retention": {},
  "adapters": [],
  "capabilities": {},
  "federate_roster": [],
  "inspectors": {"objects": [], "interactions": []},
  "normalized_events": []
}
```

The dashboard renders only these generic snapshot fields. `include_events=false`
is valid for clients that only need live inspectors and metrics.

### Optional object perspectives

The `object_observations` capability advertises the additive
`inspectors.object_observations` model in snapshots (HTTP, WS and SSE). It is
independent of analytics, MOM and commands. A backend that does not implement
it can omit both the capability and field; the UI keeps global Inspectors
usable and explicitly marks attributed values unavailable. Available/partial
capability declarations must include the model, including empty arrays.

The named models are `ObjectObservationsResponse`, `ObjectPerspectiveResponse`
and `ObjectAttributeChangeResponse`; their exact fields are in OpenAPI and the
portable wire schema. Java implementations use the same models, not Python
objects or provider-specific handle types.

- `snapshots` contains one latest object snapshot per `(federate, object_key)`.
  Required `federate: null` means the **global** observer perspective; a name
  means only events explicitly attributed to that name. Global values can mix
  sources; they must never substitute for an attributed snapshot.
- Each snapshot nests the ordinary object-inspector model and includes
  `last_sequence` and `generation`. Snapshots outlive event eviction, but clear
  with `DELETE /api/events` and reset on backend restart. A generation starts
  at the first observed sequence in that scope, or on discovery after an
  observed removal. This is not a stable RTI identity or proof of incarnation
  across federates. Multi-producer/name-reuse identity is tracked in OBS-127.
- `changes` contains every retained attribute observation, including repeated
  values, for the global scope and, when reported, its attributed scope. Each
  row has sequence, object key, scope, generation, attribute name, value,
  previous-known flag, previous value, source and observation wall time.
  Previous values come from the same perspective/generation and may predate
  retained history. Unknown previous values use `previous_known: false` and
  `previous_value: null`; a known null uses `previous_known: true`.
- `history_first_sequence` and `history_last_sequence` exactly match the event
  retention window. `history_truncated` reports dropped or cleared history.
  History retains at most `max_events` event batches, including empty batches
  for non-attribute events; it is not an archive or a time-window aggregate.
  Snapshot indexes grow with observed object/perspective identities, like the
  existing live inspectors, until clear/restart.

Required nullable fields (`federate`, `value`, `previous_value`) must remain
present even when a transport omits optional null fields. Bytes normalized as
`{"encoding":"hex","value":"00ff"}` remain explicitly undecoded; the generic
observer does not parse a FOM or guess an encoding. Charts accept finite JSON
numbers only, never booleans, numeric-looking strings or bytes. They plot the
latest 500 retained observations for the selected attribute/current generation
by observation sequence, with gaps for non-numeric values; a persisted latest
value alone does not invent historical samples.
Validation preserves JSON string values and attribute keys verbatim; identifier
cleanup must not trim supplied attribute data or merge distinct JSON keys.

Attribution is not evidence of ownership, subscription, delivery or complete
federate-local RTI visibility. Capability switches control feature use, not
data authorization or redaction; secure deployments need a separate access
boundary.

### Optional persistent time inspection

`time_inspection` advertises `inspectors.time`, an additive snapshot model shared
by HTTP, WebSocket and SSE. Available/partial implementations include the model
and its arrays even when empty. Older backends can omit both capability and
model; disabled/unsupported/unknown features show an unavailable inspector.
Windowed analytics and `logical_time` timestamp charts remain independent.
Neither this inspector nor request correlation requires MOM or commands.

An adapter publishes an explicit `time_observation` on an event, conventionally
`time.observed`. The named, discriminated inputs are `TimeRequestObservation`,
`TimeGrantObservation`, `TimeStateObservation` and `TimeOutcomeObservation`:

```json
{
  "event_type": "time.observed",
  "source": "my-adapter",
  "federation": "ExampleFederation",
  "federate": "ExampleFederate",
  "time_observation": {
    "kind": "request",
    "session_id": "fresh-join-session-123",
    "request_id": "advance-7",
    "logical_time": 12.5
  }
}
```

A later `kind: "grant"` with the same scope/ID explicitly completes that
request. Its granted time need not equal the requested target. Numeric time is
optional: an RTI with an opaque/unreported logical-time representation can
omit `logical_time` and still report request/grant identity. Present numbers
must be finite and are never coerced from strings or booleans. Keep opaque
provider values in event extensions rather than guessing a numeric encoding.
IDs are nonblank strings up to 256 characters, preserved verbatim.

`kind: "state"` optionally reports logical time, lookahead, regulating and
constrained booleans, GALT and LBTS. Missing/null fields mean not reported in
this patch; previously reported fields persist. False and zero remain facts,
not unknown. The inspector shows the latest state-patch sequence; individual
fields may predate that patch. No field is derived from payload timestamps,
participant progress, lookahead, wall time or another field. `kind: "failed"`
and `kind: "cancelled"` can close a matching request with an explicit reason,
without inventing a grant or grant delay.

`TimeInspectionResponse` contains:

- `snapshots`: one `TimeSnapshotResponse` per `TimeScopeResponse`, keyed by
  source, session ID, federation (when reported) and federate. Required nullable
  scope fields remain present under transport serialization. Snapshots retain
  last-request/last-grant samples, last-reported state, observation sequence/
  wall time, observed/ended status and uncorrelated-observation counters.
- `requests`: `TimeRequestResponse` records with an explicit scoped request ID,
  request sequence/time, requested numeric target when reported, pending or
  terminal status, outcome sequence/time, granted value when reported, reason,
  pending age and matched-grant delay when measurable.
- `request_capacity` and `dropped_requests`: the independent bounded ledger
  defaults to 250 records across all scopes, configurable with
  `ObserverStore(max_time_requests=...)`. Oldest requests, including pending
  ones, can expire. Eviction is reported; retained counts are not RTI-wide
  counts. Expired requests do not reconstruct matches from event history.
- `duration_basis: "observer-ingress-monotonic"` and
  `scope_basis: "explicit-observations"`: durations use one observer process's
  monotonic ingress clock, not remote timestamps or provider service time.
  Pending age is recomputed in snapshots; negative/unusable durations are
  unknown, not zero. Only explicitly matched grants have grant-delay samples.

Correlation requires a known source, session ID, federate and request ID; the
federation component must agree, including unknown versus reported. Requests
with missing identity remain uncorrelated observations, not guessed pending
records. A grant before its request is never retroactively matched. Repeated
request IDs still in the ledger become ambiguous and cannot produce a delay.
Adapters must make IDs unique throughout a source/session/federation/federate
scope and use a fresh session ID on reconnect/rejoin. Reuse after ledger eviction
cannot be detected; ID uniqueness is a producer contract, not an inferred RTI
property. Stable cross-producer participant identities remain OBS-127.

An exact legacy `callback.invoked` / `operation: "timeAdvanceGrant"` with a
finite root or `details.logical_time` contributes a persistent, uncorrelated
grant sample. Ordinary object/interaction timestamps never update granted time.
No request/session IDs are inferred from legacy callback ordering or names.

Explicit `session.disconnected` / `federate.resigned` events can end matching
scopes when they carry the same source, federation and top-level session ID.
A session disconnect without a federate ends all its reported federate scopes;
a resignation must identify its federate. Pending records become session-ended,
not granted. Late time packets cannot reopen an ended session; use a fresh ID.

Snapshots and pending requests survive event-history eviction, but not explicit
history clearing or process restart. The scope index grows with observed
identities until clear/restart; this is an inspector, not durable storage.
The UI limits request tables to 100 rows initially and delay charts to 250
retained matches. Pause freezes the displayed snapshot while collection
continues. Capabilities are feature switches, not access-control/redaction rules.

### Optional declaration explorer

`declarations` advertises `inspectors.declarations`. This optional model is
independent of MOM, analytics, time inspection, object/interaction delivery and
commands. Available/partial backends provide the named model, even with empty
arrays; older backends may omit capability and model. Unsupported, disabled or
unknown declarations do not disable other screens.

Adapters report `declaration` on an event, conventionally
`declaration.observed`. `ObjectDeclarationObservation` and
`InteractionDeclarationObservation` are discriminated by `kind`:

```json
{
  "event_type": "declaration.observed",
  "source": "my-adapter",
  "federation": "ExampleFederation",
  "federate": "Publisher",
  "declaration": {
    "kind": "object",
    "direction": "publication",
    "status": "declared",
    "class_key": "adapter-owned-class-id",
    "class_name": "ExampleClass",
    "session_id": "fresh-join-session-123",
    "attribute_scope": "selected",
    "attributes": ["state", "load"]
  }
}
```

The canonical federate is the explicitly reported declaration subject, not an
observer inferred from the source name or a recipient inferred from delivery.
Source, optional session/federation/federate, kind, direction and class key form
the snapshot identity. A class name alone never merges participants or RTIs.
IDs/attribute keys preserve supplied text; sessions should use fresh IDs after
rejoin. Without session identity, name reuse cannot be resolved automatically
(OBS-127); the UI labels missing provenance rather than manufacturing it.

Direction is explicitly publication/subscription. Status is explicitly
declared/withdrawn/unknown; omitted status is invalid. A declaration observation
is a **complete replacement**, not a delta: adapters must normalize partial
provider calls into a known set, or report unknown. Attribute meanings differ:

- `selected` requires a unique string array (up to 4096), including an explicit
  empty array. Empty is not unknown and does not itself mean withdrawn.
- `all` is an explicit all-attributes report for this class scope, not inferred
  class inheritance; it has no selected array.
- `unknown` (the default) has no selected array. Unknown is not all or empty.
- Interactions cannot accept object-attribute fields. Their response snapshot
  uses `not-applicable`, not an empty selected set.

`DeclarationsResponse` provides `snapshots`, `capacity`, `dropped_declarations`
and `basis: "last-reported-declarations"`. Each `DeclarationSnapshotResponse`
has an explicit nullable scope, class identity/name, direction/status, attribute
scope/set, optional reason, event sequence and observation wall time. Named
Pydantic models validate shape and scope/set invariants for HTTP and streams;
the core uses dependency-free typed records and atomic input validation.

Snapshots outlive event eviction, but clear on explicit history clear/restart.
The index defaults to 1000 records across all scopes, configurable with
`ObserverStore(max_declarations=...)`. It evicts least-recently-updated rows,
reports loss and never equates a missing row with a withdrawn declaration.
These are last-reported facts, not live RTI queries: old session scopes remain
separate until explicit clear or capacity eviction. Lifecycle/delivery events
do not invent a declaration status change.

The explorer supports class/key/attribute/reason search, perspective, kind,
direction/status filters, attribute details and initially 100 rows with explicit
load-more controls. Pause freezes its snapshot. Matching declaration class names
never imply ownership, routes, current membership, DDM associations or actual
delivery. Explicit relationship/topology models are the remaining OBS-110
increment; no routing edges are drawn from declaration coincidence.

## Event pages and streams

`GET /api/events?after=42&limit=500` returns a cursor page with `events`,
`next_after`, `has_more`, `gap`, `first_retained_sequence`,
`last_sequence`, `retained_events`, and `dropped_events`.

Both streaming transports use the same message vocabulary:

```json
{"type": "snapshot", "state": {}}
{"type": "events", "events": [], "state": {}}
{"type": "heartbeat"}
{"type": "gap", "retention": {}}
{"type": "error", "message": "human-readable diagnostic"}
```

The WebSocket endpoint is `/ws/events`; the Server-Sent Events endpoint is
`/api/events/stream`. SSE `data` payloads are JSON versions of the same
messages. A client that receives `gap` should refresh the snapshot before
continuing from the advertised retained cursor.

## Optional session commands

The `commands` capability and `api_paths.command_catalog` / `api_paths.commands`
advertise explicitly registered control sessions. Observation capabilities
(including MOM) never imply control authority. A backend without this feature
can omit those paths; the Actions tab shows an unavailable state. The Python
reference keeps disabled command routes in OpenAPI so generated models remain
stable, returns an empty disabled catalog/history, and rejects execution with
HTTP 503 when no dispatcher is configured.

```text
GET  /api/commands/catalog       # CommandCatalog: targets and typed action descriptions
GET  /api/commands               # CommandHistory: newest first, bounded, process lifetime
POST /api/commands               # 202 CommandRecord: retained command identity
GET  /api/commands/{command_id}  # CommandRecord: current execution result
```

`CommandRequest` is a discriminated union of `ConnectCommand`, `JoinCommand`,
`ResignCommand`, and `DisconnectCommand`. Unknown actions, extra parameters,
coerced booleans, credentials and arbitrary provider calls are rejected before
dispatch. Each action has named, strict parameter models. For example:

```json
{
  "request_id": "example-join-1",
  "target_id": "my-owned-session",
  "action": "federate.join",
  "confirmed": false,
  "parameters": {
    "federation": "ExampleFederation",
    "federate_name": "Observer",
    "federate_type": "Observer"
  }
}
```

Targets are registered server-side. They are not arbitrary observed federates.
The catalog advertises state/reason, form fields and a named parameter schema
per action. The reference lifecycle controller restricts joins to an application
allowlist and keeps RTI factories, connection configuration and credentials on
the server. Resign and disconnect require `confirmed: true`. Unsupported or
disabled actions return a typed unavailable result with no provider effect.

HTTP 202 means a command resource exists, not that execution succeeded. Results
move through accepted/running to succeeded/failed/unavailable. Succeeded means
the local service returned; future collective actions must track their own
callback completion. Results include the request, timestamps, typed outcome
or error, and its effect classification (`none` or `unknown`).

Request IDs deduplicate only within retained command history. Reusing a retained
ID with different content returns HTTP 409; retrying the same request returns
the same command. Unknown targets/results return 404. Active commands cannot
be evicted, history reports eviction, and browser refresh does not clear it.
Restarting the backend clears this in-memory history. These records have a
separate lifetime from the bounded observation event deque.

Only one command may be in flight per session. A deadline marks a timed-out
running call failed with unknown effect, but does not cancel it or release the
session for another call. The catalog reports busy/unknown until that call
returns. A late outcome or late error is added without erasing the timeout.
An unclassified provider failure makes session state unknown; it is not safe
to guess a successful or unchanged state. Known no-effect RTI rejections can
be translated by the provider adapter without losing a known session state.

The reference dispatcher creates one execution thread per target. Session
description is a thread-safe cache; provider calls, factory creation and the
evoked callback pump belong to the execution owner. A Java replacement can use
its own executor while preserving these wire semantics. Applications own
session authorization at the HTTP boundary when exposing real controls.

## Backend replacement rules

### Optional observation analytics

`GET /api/analytics?window_seconds=300&bucket_count=30&federate=Example`
returns `AnalyticsResponse`. The backend advertises `analytics` as available
or partial and adds `api_paths.analytics` only when it implements this resource.
Backends without it continue to support events and inspectors; the React UI
shows an unavailable analytics state without treating the whole service as
offline. The reference service accepts windows of 30..3600 seconds and
10..120 buckets. `federate` is an optional exact identity filter.

The Python backend can turn this module off with `--no-analytics` or
`create_app(store, enable_analytics=False)`. Its route and OpenAPI path are then
absent, the capability is disabled, and `api_paths` omits the optional resource.

The required response fields are `contract_version`, `basis`
(`retained-observations`), `generated_at`, `window_start`, `window_seconds`,
`bucket_seconds`, `scoped_federate`, `events_in_window`, `unattributed_events`,
`history_truncated`, `retention`, `participants`, `activity`, `logical_times`,
`event_types`, `sources`, and `families`. Named field-level models are published
in `/openapi.json` and generated into the frontend API snapshot.

- One consistent observer snapshot supplies roster, retention, and events.
  Only observations whose Unix `observed_at` lies in the inclusive window
  `[generated_at - window_seconds, generated_at]` contribute. The right edge
  belongs to the final bucket; all buckets are returned, including empty ones.
- Activity buckets count total events, `object.updated`, `interaction.*`,
  `callback.*`, and `runtime.error`. Distribution fields use exact reported
  event types, source names and explicit families. Counters are observations,
  not deduplicated RTI deliveries or actual network throughput.
- Participants come from the persistent observed roster, including idle rows.
  Each has identity/role/status/source/last-seen metadata, window counts,
  observed `object_keys`, and the latest numeric logical timestamp in event
  sequence order. Unreported values are null, not zero. A scoped response
  includes only that roster row and explicitly attributed events.
- Identity uses `federate` or `participant`, including a structured
  `name`/`id`/`handle`, then explicit `federate_name`/`participant_name` fields.
  Source names never imply participant identity. Attributed object observations
  do not establish object ownership or subscriptions.
- Logical timestamps come from finite numeric `logical_time` or
  `details.logical_time`; boolean/string values are not numeric timestamps.
  Samples retain sequence, participant (or null), event type and operation.
  Only an explicit `operation: timeAdvanceGrant` identifies a grant; neither
  a timestamped update nor a latest value establishes granted time, GALT or LBTS.
- `history_truncated` indicates that some events have been dropped or cleared
  during this store's lifetime. Full retention counters remain available. The
  response does not claim a complete federation history or membership census.

This resource is read-only. Observation capabilities do not authorize provider
commands; the planned command catalog is tracked separately in
[FEATURE-BACKLOG.md](FEATURE-BACKLOG.md).

### Replacing the service implementation

To replace the Python server with Java:

1. Keep the built React dashboard static asset unchanged.
2. Implement `/api/contract`, `/api/state`, `/api/events`, and one streaming
   endpoint with the shapes above.
3. Advertise provider and backend capability states in the contract and state
   snapshot. Do not silently omit unsupported features.
4. Translate Java RTI callbacks into the normalized event envelope in a Java
   adapter; do not expose Java RTI callback objects to the dashboard.
5. Preserve cursor monotonicity and report retention gaps.
6. Keep provider/FOM/domain-specific names in adapter-produced event fields,
   not in the dashboard contract.

No Python package, Python callback class, FOM parser, or RTI binding is part
of the frontend contract.

## Python implementation boundary

The reference Python backend keeps the wire dictionaries at the boundary. Its
store uses explicit records for normalized events, object/interaction/federate
inspectors, metrics, retention, adapter status, capability declarations, and
event pages. Each record has a `to_json()` conversion used by the HTTP and
streaming layers.

Provider-specific input is intentionally flexible only at the adapter seam:
an adapter may submit a mapping containing provider extensions, but the store
normalizes the envelope and retains extensions as JSON-compatible values. New
backend code should add a model field or an explicit extension field rather
than passing nested `dict[str, Any]` objects through the store.

The optional FastAPI transport adds a second validation boundary in
`schemas.py`. Request bodies use named, strict Pydantic models (`EventInput`,
`EventBatchRequest`, `ContextRequest`, and `StatusRequest`); finite responses
use named response models; and stream messages are validated before they are
sent over SSE or WebSocket. Extension fields are collected into an explicit
`extensions` model field internally, then flattened back to the existing
top-level event wire shape. Their values must still be recursive JSON values.
A malformed batch is rejected with HTTP 422 before it reaches the store, so
validation cannot partially ingest a batch.

The package-level `pyrightconfig.json` and Ruff checks protect this boundary.
Run the dependency-free core tests plus the optional web extra before swapping
in another backend implementation.

For a separately hosted backend, the static dashboard can override its
`observer-api-base` and `observer-stream-path` meta tags with same-origin or
absolute HTTP/WebSocket URLs. Cross-origin deployments must also configure
the backend's normal CORS and WebSocket-origin policy. The contract version
remains the compatibility check.
