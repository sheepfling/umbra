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

### Participant attribution and object context

Optional root `participant_attribution` identifies the ordinary event actor:

```json
{
  "session_id": "reporting-session-1",
  "identity_namespace": "provider-domain",
  "federation_id": "federation-1",
  "participant": {
    "participant_id": "participant-17",
    "membership_id": "join-3",
    "display_name": "Worker",
    "handle": "0x12"
  }
}
```

The five IDs are nonblank opaque strings (session at most 256 characters;
namespace, federation, participant and membership at most 512). They preserve
whitespace, Unicode and case. Root `session_id`, `time_observation.session_id`
and `declaration.session_id`, when supplied, must agree with each other and
the attribution session. Conflicting input rejects the whole batch.
When a named observation omits its own session, the backend can use that same
event's explicitly supplied reporting session. It never borrows a session from
another event or guesses one from a name. Opaque object keys and reported
participant aliases also remain unchanged through the named boundary models.

`ParticipantPerspectiveResponse` exposes a canonical opaque `key`, `basis`,
source/session/federation display metadata, original reported name and optional
named attribution. An `explicit-join` key includes source, reporting session,
namespace, federation ID, participant ID and membership ID. Names, handles and
federation display labels do not identify that join. A fresh membership ID is
required for each rejoin and a fresh reporting session after reconnect. Known
joins without display names remain usable as unnamed participants.

Missing join IDs produce a `legacy-provenance` bucket scoped by source,
explicitly reported session, federation display label and reported name. This
does not prove stable participant identity. Older backends/files without the
codec or perspective metadata keep the clearly labelled legacy name-only
projection; no reporter or join ID is invented. The observer-owned
`participant_provenance` capability describes codec support, not RTI maturity
or the completeness of the supplied identities. MOM is not required. Nested
MOM subjects, topology endpoints, source names or handles alone never create
ordinary roster actors.

Roster, attributed object snapshots/history, declarations, time scopes,
interactions and analytics carry the optional named perspective. Repeated
aliases update metadata in one join; request history retains its original
reported metadata. Exact time correlation requires the same perspective and
request ID. `federate.resigned` concerns the exact subject; a reporting session
ending is not proof that all observed participants resigned. Identified-domain
session-end handling requires explicit namespace/federation evidence. Terminal
facts are retained only within the disclosed bounded caches, not forever.

An independent root `object_context` may supply `identity_namespace` and
`federation_id`. Its `observation_key` isolates the raw `object_key` by that
explicit context, including global object projections. It is not inferred from
the event actor or topology. Without it, global objects keep legacy unscoped
aggregation, explicitly labelled in the UI; attributed baselines still use
the exact participant perspective. No raw object handle is promoted into a
verified cross-federation object identity.

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
  `last_sequence` and `generation`. Snapshots outlive event eviction, subject to
  their independent scoped-snapshot budget, and clear with `DELETE /api/events`
  or backend restart. A generation starts at the first observed sequence in
  that retained scope, after scoped-index expiry, or on discovery after an
  observed removal. This is not a stable RTI identity or proof of incarnation
  across federates. Explicit participant/object context above is independent
  of this local generation cursor.
- `changes` contains retained attribute observations for retained scopes,
  including repeated values, for global and explicitly attributed scopes. Each
  row has sequence, object key, scope, generation, attribute name, value,
  previous-known flag, previous value, source and observation wall time.
  Previous values come from the same perspective/generation and may predate
  retained history. Unknown previous values use `previous_known: false` and
  `previous_value: null`; a known null uses `previous_known: true`.
- `history_first_sequence` and `history_last_sequence` exactly match the event
  retention window. `history_truncated` reports dropped/cleared history or
  omitted changes belonging to expired scoped baselines, or independently
  expired change batches (`dropped_history_batches`, when reported).
  History retains at most `max_events` event batches, including empty batches
  for non-attribute events; it is not an archive or a time-window aggregate.
  Snapshot and event-history limits are independent; neither missing snapshots
  nor omitted scoped changes imply object removal or absent attributes.

The reference scoped index defaults to 5000 least-recently-observed records
(`max_object_perspectives`). Global `null` and each attributed perspective use
separate slots; this is not a limit on unique federation objects. Optional
`snapshot_capacity`, `dropped_snapshots` and `omitted_changes` are either all
reported or all unreported. The first is positive; eviction count is cumulative
since clear; omitted changes counts the **current** event-window changes whose
scoped baseline is no longer retained. They must not reference invented scopes.
Clear resets the index counters. Reobserved scopes begin a fresh comparison
baseline, never reusing another participant's or a global value. Raw envelopes
and their analytics are not dropped merely by this scoped-index expiry. All
collections also participate in the shared encoded-byte budget below; byte
pressure can independently expire raw envelopes, classic objects or roster
metadata. Their individual coverage makes the different losses explicit.
Older backends without metadata have unknown capacity/loss, not zero loss or an
infinite index. Objects, Federates and Diagnostics disclose the budget/loss;
object navigation and attributed cards initially render 100 rows with explicit
load-more controls. Search still covers all supplied snapshots.

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
- Optional `snapshot_capacity` and `dropped_snapshots` report the separate
  latest-scope budget (2000 by default). A request can outlive its latest
  snapshot. Its `snapshot_retained: false` requires explicit snapshot-expiry
  coverage and no matching latest snapshot; absent/true keeps the older
  matching-snapshot requirement. Retained tickets keep exact scope and may
  still match a later grant, without borrowing expired role/bound/grant state.
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
not granted, even when their latest snapshot has expired. Retained ended
snapshots or session-ended tickets prevent late packets reopening that scope.
Once all such bounded evidence expires, the observer cannot remember an
unlimited history of ended sessions: producers must still use fresh IDs.

Snapshots and pending requests can survive event-history eviction, subject to
their independent count/byte budgets. Explicit history clearing or process
restart resets both. Snapshot expiry is not ticket cancellation or session
termination; the UI includes retained request scopes even without a snapshot.
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
rejoin. Named participant attribution above, when supplied, replaces the legacy
name component with its exact reporting perspective. Missing join/session
identity remains unresolved; the UI never manufactures it from a class name.

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

Snapshots can outlive event eviction, subject to their own count/byte budgets,
and clear on explicit history clear/restart.
The index defaults to 1000 records across all scopes, configurable with
`ObserverStore(max_declarations=...)`. It evicts least-recently-updated rows,
reports loss and never equates a missing row with a withdrawn declaration.
These are last-reported facts, not live RTI queries: old session scopes remain
separate until explicit clear or count/byte expiry. Lifecycle/delivery events
do not invent a declaration status change.

The explorer supports class/key/attribute/reason search, perspective, kind,
direction/status filters, attribute details and initially 100 rows with explicit
load-more controls. Pause freezes its snapshot. Matching declaration class names
never imply ownership, routes, current membership, DDM associations or actual
delivery. Explicit relationships use a separate optional model below; no
routing edges are drawn from declaration coincidence.

## Optional storage coverage

`storage_inspection` is an observer capability, not an RTI service. When
available/partial, `StateResponse.storage` is required and contains named
`StorageCoverageResponse` evidence/context budgets. Missing/disabled/unsupported
coverage stays unknown, not unlimited or zero loss. Other capabilities remain
independent. A paused/replayed view uses its captured coverage.

The reference store bounds retained observation JSON to 6 MiB plus separately
admitted metadata. Every observation collection has both a count capacity and
shared encoded-byte accounting; whole-record LRU uses disclosed soft collection
reservations, not guaranteed minimums. Context uses a separate 256 KiB budget.
This measures encoded evidence with a 64-byte per-record reserve, not RSS or
temporary ingress allocations. Values are never clipped or replaced with null.

`StorageBudgetResponse` reports accounting basis, policy, total/single-record
limits, accounted bytes, reserve, collection coverage and explicitly unbudgeted
collections. `StorageCollectionResponse` reports name/unit, capacity, retained
and accounted amounts, optional soft reservation, total loss and separate
count/byte/oversize/encoding/parent-window reasons. Named models validate sums,
uniqueness and delivered collection counts. These overlapping collection losses
are not additive unique federation/event totals. The reference store lists no
unbudgeted observation collections; another backend must disclose any gaps.

Classic objects/roster/time snapshots default to 5000/2000/2000 rows. Retention
expiry never proves object removal, resignation, disconnection or an RTI
incarnation. See [STORAGE.md](STORAGE.md) for configuration, metadata admission,
loss semantics, orphaned time tickets and exact cursor/recording rules.

## Event pages and streams

`GET /api/events?after=42&limit=500` returns a cursor page with `events`,
`next_after`, `has_more`, `gap`, `first_retained_sequence`,
`last_sequence`, `retained_events`, and `dropped_events`. Optional
`next_sequence` reports the lifetime next sequence, including observations
that immediately exceeded a byte/record budget. Retained envelopes are an exact
contiguous suffix, never a sampled subset with hidden holes. First/last retained
sequence are zero for an empty window, even after previous observations.
For a lost empty window, `gap=true` and `next_after=next_sequence-1` advance
the reader without claiming delivered events; reading again at that cursor
does not repeat the same gap. Streams recheck retention after waiting before
consuming rows, including when no accepted envelope remains retained.

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
messages. A client that receives `gap` reconciles an authoritative snapshot
before continuing from the advertised retained cursor. The reference stream
delivers that snapshot; an older service may require a fresh HTTP read.

### Observer lifetime and negotiated resumption

`ObserverLifetimeResponse` is optional additive metadata on states and event
pages: `{ "instance_id": "opaque-storage-incarnation", "revision": 12 }`.
The instance changes when an observer store is replaced. Its nonnegative
revision advances once per atomic event batch, clear (including an empty
clear), or metadata change. Reads and recording timer checkpoints do not
advance it. This is **not** an RTI reporting session, participant membership,
provider connection or command permission. Revisions from different instances
cannot be ordered; wall clocks do not establish ordering either.

When `contract.stream.resumption` is true, both discovered stream endpoints
accept `after` (exclusive sequence) and `instance_id` (at most 128 characters).
`StreamCursorResponse` contains `instance_id`, `revision` and `after`, meaning
the last delivered envelope position, not merely a server high-water mark.
Snapshot, event and heartbeat messages may carry this named `cursor`.
Snapshot `resume` explains the handshake:

| Result | Meaning |
| --- | --- |
| `fresh` | No previous cursor; the snapshot includes the complete retained event array. |
| `resumed` | Same observer and retained cursor; a summary snapshot keeps the requested cursor, then any missing retained envelopes are delivered from that exact same state cut. |
| `reset` | Unknown/foreign instance or future position; the complete retained snapshot replaces previous live data. |
| `gap` | Same observer, but some requested history was lost or cleared; a complete retained snapshot and gap notice disclose the loss, without reconstructing missing records. |

Every state and its envelope batch come from one atomic store snapshot. A
metadata change or clear can deliver an empty `events` batch immediately;
clients still apply its state and prune expired rows. A heartbeat means the
stream is responsive, not that the federation is advancing. The reference
backend sends an idle heartbeat after 15 seconds. SSE uses JSON `data` frames;
resumption uses the query parameters above, not `Last-Event-ID`.

HTTP event pages also carry optional lifetime metadata and `reset`. Supplying
an `instance_id` from another store, or a future cursor for that store, resets
the page to its retained beginning with `reset=true`. Existing requests without
that parameter keep their original exclusive sequence semantics.

The React client refreshes discovery during its five-second snapshot
reconciliation and on Sync now. It follows advertised safe same-origin paths,
prefers usable WebSocket, falls back to usable SSE, then keeps HTTP polling.
Unavailable transports are not repeatedly opened. Failed advertised transports
retry with bounded backoff (1–15 seconds); an opening timeout and a 45-second
silent-stream watchdog prevent an unresponsive stream appearing live forever.
The client closes SSE before its own reconnect so browser retries do not race
the negotiated cursor. Browser offline/online signals suspend and resume live
transport explicitly. Disabling/changing transport discovery closes the old
connection; late callbacks from it cannot update the new view.

Same-instance older revisions and HTTP replies overtaken by a stream cut are
ignored. Complete snapshots replace their retained event window, with sequence
deduplication for backfill. A new instance replaces old live inspectors/history
and resets metric samples. Older services can omit resumption/lifetime fields;
they remain usable through complete snapshot reconciliation, with lifetime
explicitly unknown rather than inferred from names or sequence reuse.
Paused/replayed display data is not replaced by live transport updates.

`create_app(..., enable_websocket=False, enable_sse=False)` can independently
disable these transports. Discovery omits unusable transport paths. HTTP reads
and RTI capabilities remain independent. `/openapi.json` includes all five
named stream DTOs and `x-observer-stream-messages` references so generated
TypeScript shares the Pydantic model shapes; SSE itself remains a text stream,
not a finite JSON response. HTTP-read conformance does not prove streaming or
provider lifecycle behavior, including for the independent Java read fixture.

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
`ResignCommand`, `DisconnectCommand`, and optional `InspectSessionCommand` /
`RecoverSessionCommand`. Unknown actions, extra parameters,
coerced booleans, credentials and arbitrary provider calls are rejected before
dispatch. Each action has named, strict parameter models. For example:

```json
{
  "request_id": "example-join-1",
  "target_id": "my-owned-session",
  "action": "federate.join",
  "confirmed": false,
  "expected_version": {"instance_id": "copy-the-owned-registration-from-the-catalog", "revision": 0},
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

`command_versions` advertises conditional dispatch. Each catalog target has a
named `CommandTargetVersion` (`instance_id`, nonnegative `revision`). Copy the
actual current version, not the illustrative values above, into
`expected_version`. Every reference-dispatcher operation checks that exact
registration/revision both at acceptance and before owner execution. A missing,
foreign or stale version gives an unavailable result with `effect: none` and
does not call the provider. A registration gets a new opaque instance ID; its
revision advances on execution start/return and callback-pump faults. Observer
store lifetime, source/reporting session and participant membership are separate
identities and cannot satisfy this guard. A retained exact request retry still
returns its original result; changing its version/content under the same retained
request ID is a conflict. After reviewing refreshed state, use a new request ID.

Older command services may omit target versions. The maintained React client
then keeps the original four lifecycle request shapes, without extra version
fields. It never invents a version to enable inspection/recovery. Modern reference
targets require the guard even for the original lifecycle actions; this is an
authorization/precondition check, not proof that an observation identifies an
owned session. Applications still supply authentication/authorization for real
deployments.

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

### Optional owned-session inspection and recovery

`session_recovery` is independent of MOM, analytics, observer streaming and
participant provenance. A configured owned adapter can offer two additional
catalog actions using the same queue/history endpoint:

| Action | Provider operation | Control-state effect |
| --- | --- | --- |
| `session.inspect` | Read the owned driver's actual connection/membership. | Retain named evidence; do not restore permissions or clear faults. |
| `session.recover` | Inspect, fence connected-driver recovery with a healthy owner-thread callback pump, then inspect again. | With explicit confirmation, reconcile only complete facts within the configured federation allowlist. |

Both requests use `parameters: {}` and a required named `expected_version`.
Recovery additionally requires `confirmed: true`. Neither accepts a factory,
credentials, FOM URLs, a participant name to adopt, provider options or an
operation to retry. Pending/timed-out provider work keeps the target busy; an
inspection cannot run around a blocked owner thread or forcibly cancel it.

`SessionStateObservation` has independent `connection`
(`connected`/`disconnected`/`unknown`) and `membership`
(`joined`/`not-joined`/`unknown`), optional federation/federate/own-handle metadata
and a bounded reason. Contradictory disconnected+joined facts and stale member
metadata with not-joined status are invalid.
Provider-reported names and handles preserve whitespace, Unicode and case;
the federation allowlist compares the exact reported name, never a trimmed alias.
A missing name is not a guessed peer: an explicit joined state may have an
unreported name. A joined federation must
be known and in the controller's allowlist before control is restored. Unknown
connection/membership or a missing/out-of-allowlist federation stays unknown,
with lifecycle actions disabled. This optional control limitation never blanks
ordinary observation views.

`SessionInspectionResult` retains `basis: owned-driver-query`, server
`checked_at`, the named observation, `reconciled` and a resolution. The target's
latest `inspection` and the command outcome/late outcome carry this model.
The wall-clock timestamp describes when the observer checked; target revision,
not that timestamp, guards ordering. A valid query can succeed with
`reconciled: false`: service return is not proof that permissions were restored.
Inspection-only results cannot claim reconciliation. Malformed proof becomes a
typed failure, leaves control uncertain and does not terminate the owner worker.
Old successful inspection evidence is historical; a later fault takes precedence.

Recovery never edits a previous failed/timeout result, replays its operation,
creates a replacement driver, or promotes roster/topology data into authority.
`session.inspected` / `session.reconciled` are observer audit events, not invented
join/resign callbacks. Late proof can be recorded after timeout once the same
owner returns; the original timeout remains, and no new work runs while busy.
History remains process-local and count-bounded with explicit expiry.

`SessionController(..., inspection_supported=True)` opts into the optional
`InspectableSessionDriver.inspect()` protocol. Drivers without this capability
keep their existing lifecycle behavior. The Umbra adapter accepts an explicit
server-owned `inspection(ambassador) -> SessionStateObservation` callable and
does not assume universal HLA `isConnected`/`isJoined` methods. Other Python RTI
shapes normalize their own authoritative queries in their adapter. Without a
usable query, keep recovery unsupported/unresolved rather than reading the
controller's own cached state as proof.

React builds these generated DTOs in `app/commandRequests.ts`, keeps separate
inspection/reconciliation controls, and invalidates confirmation on target
incarnation/revision change. Pause, stale catalog and replay prevent submission;
overtaken catalog replies cannot undo a newer local mutation. HTTP-read
conformance checks discovery/version/proof shapes but explicitly leaves
`owned-session-recovery` unchecked: the independent Java fixture is read-only,
not a Java executor or provider-recovery implementation.

## Explicit federation topology (optional)

The independent `relationships` capability enables `inspectors.relationships`:
`RelationshipsResponse` has `snapshots`, positive `capacity`,
`dropped_relationships`, and basis `last-reported-explicit-relationships`.
Older backends may omit both capability and model. Disabled/unsupported states
hide the workspace; partial states retain evidence with a coverage warning.
This feature needs neither MOM, analytics nor commands.

An event's reserved `relationship` payload is a named
`RelationshipObservationInput`. It requires opaque `relationship_id`,
`session_id` (max 256 characters), `identity_namespace`, `federation_id`, explicit
`producer`/`recipient`, `kind` (object/interaction), `evidence`
(declared/observed), `status` (active/ended/unknown), and `viewpoint`
(observer/producer/recipient/unknown). `class_key`, `class_name` and `reason`
are optional. IDs are nonblank bounded text, not trimmed or coerced; other IDs
are at most 512 characters and reason at most 4096. Unknown payload fields are
rejected even though unrelated event extensions remain supported.

Each endpoint has required `participant_id` and `membership_id`; optional
`display_name` and `handle` are display metadata, never correlation keys.
Node identity is `(identity_namespace, federation_id, participant_id,
membership_id)`. Adapters must assign a fresh membership ID on every rejoin,
a fresh reporting session ID on reconnect, and coordinate an identity namespace
only when they can guarantee shared identifiers. A name or handle reused in
another join must not reuse the old identity. Reporters that cannot provide
this evidence should advertise unsupported/partial, not invent identities
from display names. Nested relationship subjects do not automatically identify
ordinary event actors: adapters separately supply root `participant_attribution`
for the roster/object/time/analytics consumers described above.

A relationship snapshot is identified by `(event.source, session_id,
identity_namespace, federation_id, relationship_id)`. Repeated reports are
complete replacements, including clearing omitted optional metadata; distinct
reporters remain distinct edges. Adapters supply stable relationship IDs for
updates and fresh IDs for unrelated relationships. The default 1000-record
index evicts the least recently updated record and reports its own loss count.
It survives event-history eviction; clear/restart resets it. Generic lifecycle
events do not silently end or revive explicit relationship records.

Snapshots add source, sequence and observed_at provenance. Neither an active
status nor observed evidence is proof of current membership, ownership,
message delivery, DDM routing or complete federation coverage. No edge is
inferred from matching class names, publication/subscription declarations,
object updates or receive callbacks. The graph draws the newest 40 filtered
edges with a visible limit; its accessible table renders 100 at a time.
Both use the same explicit data and support pause/clear. Layout positions
do not represent latency or network location. The shared Java fixture includes
these named endpoint and relationship models without importing Python.

## Synchronization and save/restore inspection (optional, read-only)

Independent `synchronization` and `persistence_inspection` capabilities enable their
respective workspaces over the additive `inspectors.coordination` model.
`CoordinationResponse` contains latest `snapshots`, retained `transitions`,
positive `capacity` and `history_capacity`, `dropped_operations` and
`dropped_transitions`, and basis `explicit-coordination-observations`. If either
feature is advertised available/partial, the model (including empty arrays)
must be supplied. Older backends may omit the feature and model. These views
do not require MOM, analytics, commands or native RTI bindings.

Legacy provider-level `persistence` retains its earlier meaning; it does not
promise this additive typed inspector. A v1 backend advertising only that
older capability may omit `coordination` and still pass conformance. The new
workspace remains explicitly unavailable, without disabling its other views.

The reserved `coordination` event payload is a discriminated named input:

- `SynchronizationObservation`: kind `synchronization`; phase `requested`,
  `registered`, `registration-failed`, `announced`, `achieved`, `completed` or
  `failed`.
- `PersistenceObservation`: kind `save` or `restore`; phase `requested`,
  `begun`, `completed` or `failed`.
- Both require opaque `operation_id`, fresh reporter `session_id`,
  `identity_namespace`, `federation_id`, `label`, and `scope` (`local` or
  `federation`). Identifier limits and preservation match relationship inputs.
- Optional `subject` uses the explicit participant/join identity and is allowed
  only for local scope. A missing local subject is unknown, not global. Optional
  `participants` is a complete replacement of the supplied progress report,
  **not** necessarily complete federation membership. Null/omitted means
  unreported; `[]` means an explicitly empty report. Each named entry contains
  `identity`, `status` (pending/achieved/begun/completed/failed/unknown), and an
  optional reason. Join IDs must be unique within the array (max 4096 entries).
  Reasons are at most 4096 text characters. Extra/coerced fields are rejected.

The adapter assigns operation IDs that distinguish repeated uses of a label.
Latest-report identity is `(source, session_id, identity_namespace,
federation_id, kind, operation_id, scope, subject participant/membership IDs)`.
Neither identical labels nor names merge operations. A fresh local join gets
a fresh membership ID. Complete replacements discard omitted optional reasons
and participant lists rather than carrying stale data forward.

Snapshots add `source`, `sequence` and `observed_at`. The default independent
index retains 1000 scoped snapshots and 2000 phase reports; least-recently
updated snapshots and oldest transitions expire separately. Configure these
with `max_coordination` / `max_coordination_transitions`. `dropped_operations`
counts evicted **scoped snapshots**, not unique collective operations.
Clear/restart resets both indexes and loss counters. Ordinary event-history
eviction does not erase them. The combined capacities/loss counters cover all
three kinds, even when the UI filters to one workspace.

These are reported facts, not a reconstructed RTI state machine. Missing
intermediate phases are never filled in; arrival order is not an assertion of
provider execution order. A local completed/achieved report, a list of successful
participants, or an accepted request never establishes collective completion.
Only an explicit federation-scoped completed report displays that outcome.
Raw callbacks with similar names do not create normalized phases automatically.

The UI has operation selection, label/source/identity/reason search, latest
phase/scope filters, participant failures and a retained phase sequence with
report counts. The sequence shows all scopes/phases for the selected operation
independently of the latest-report table filters. Tables and sequences initially
render 100 records with explicit load-more controls. Pause freezes them; clear
preserves a missing selection instead of silently choosing another operation.
The Java fixture supplies the same models unchanged. No register, achieve,
save or restore command is advertised; observation does not grant authority.

Optional inspector infrastructure support does not upgrade an adapter's
unsupported/disabled feature to partial. Actual adapter reports aggregate with
one another; an available reporter plus an unsupported reporter is partial,
whereas disabled plus unsupported remains unusable. Application/operator
capability overrides remain authoritative.

## Interaction-flow inspection (optional)

The independent `interaction_flow` capability enables the additive
`inspectors.interaction_flow: InteractionFlowResponse`. It contains bounded
`records`, positive `capacity` (default 500), `dropped_records`, cumulative
`send_observations`/`receive_observations` since clear, and basis
`explicit-interaction-observations`. An available/partial feature must include
the model, even when empty. Older, disabled or unsupported backends keep their
other views. The feature needs neither MOM, analytics nor commands.

Reserved event field `interaction_flow` uses named
`InteractionFlowObservationInput`: required `kind` (send/receive) and explicit
`order` (receive-order/timestamped/unknown); optional opaque `message_id`,
`identity_namespace`, `federation_id`, reporter `session_id`, `producer`,
`recipient`, `class_key` and finite numeric `logical_time`. Endpoint models
require participant and membership IDs, with optional display names/handles.
Identifier bounds/preservation match topology. Unknown fields and number/string/
boolean coercion are rejected. Direction must agree with a canonical
`interaction.sent` or `interaction.received` envelope, including its `kind`
alias, before any batch mutation. A logical timestamp never implies timestamped
delivery order. Opaque/unavailable numeric times can be omitted.

Pairing requires the complete exact key `(identity_namespace, federation_id,
message_id, producer participant/membership IDs, recipient participant/membership
IDs)`. Names, handles, classes, timestamps and reporter source/session are not
pairing keys. Independent sender/receiver reporters can therefore contribute
evidence when their adapter-supplied identity namespace is genuinely shared;
their original source/session metadata remain separate. Adapters must guarantee
a fresh message ID for each transmission in that namespace, federation and
endpoint-join scope, including across reporter reconnects. Never invent IDs or
shared namespaces from names. Each rejoin requires a fresh membership ID.
Deduplication cannot detect reuse after capacity eviction; uniqueness is a
producer obligation, not a claim of infinite retained memory.

Each named record retains `record_id` (its first observation sequence),
`last_sequence`, first `send` and/or `receive` report with source/sequence/
observed_at provenance, send/receive counts, status and optional
`arrival_delay_seconds`:

- `uncorrelated`: an incomplete key; each observation stays a separate record.
- `send-only` / `receive-only`: one side of a complete explicit key is observed.
  Neither establishes delivery failure or absence of the other observation.
- `paired`: exactly one send and one receive with matching identity. This means
  paired adapter observations, not proof of RTI delivery correctness.
- `ambiguous`: a retained key has repeated send or receive observations. Counts
  increase; original evidence is retained; prior pairing and delay are invalidated.
  Do not treat a duplicate as an additional delivered transmission.

Delay is the observer's monotonic receive-arrival minus send-arrival interval,
only for an unambiguous send-before-receive pair with a finite nonnegative
duration. It is **not network latency**, a provider service duration or a
remote-clock measurement. Receive-before-send evidence may pair, but has no
invented positive interval. Invalid/backwards clocks produce unknown, not zero;
a genuine measured zero remains zero. Logical times are unrelated to this clock.

Exact legacy `interaction.sent` / `interaction.received` events without the
typed field still contribute uncorrelated observations, with unknown endpoints
and order. Similar callback names, matching classes or a numeric root timestamp
do not establish a pair or order. Explicit flow does not create topology edges:
the separate relationship model remains the only topology source.

The least-recently-updated flow record expires when capacity is reached;
cumulative observation totals still include expired reports. Retained statuses
and charts cover only retained evidence, not a delivery success ratio. Configure
`max_interaction_flow`; clear/restart resets records, totals and loss. Event
history expiry alone does not erase flow evidence. The UI adds search, pairing/
order filters, paged evidence tables, per-report details and a dynamic arrival-
interval chart. Pause freezes the view. Java full/no-ID/no-analytics fixtures
use these unchanged named models, including a real measured-zero interval.

## Attribute ownership inspection (optional, read-only)

`ownership_inspection` advertises the additive `inspectors.ownership` model.
Legacy provider-level `ownership` does not promise this new resource shape.
`OwnershipResponse` contains `states`, `requests`, `transitions`, positive
`capacity`/`request_capacity`/`history_capacity`, separate `dropped_states`,
`dropped_requests`, `dropped_transitions`, `unmatched_outcomes`, and basis
`explicit-ownership-observations`. Available/partial inspection includes the
model and its empty arrays; older/disabled/unsupported backends keep other views.
MOM, analytics, commands and native RTI bindings are not dependencies.

Reserved event field `ownership` is a discriminated named input. All three
kinds require reporter `session_id`, `identity_namespace`, `federation_id`,
adapter-supplied object-instance-lifetime `object_id` and `attribute_key`:

- `OwnershipStateObservation`: kind `state`, explicit `owner_state`
  (known/unowned/rti/unknown) and `transition` (reported/acquired/divested).
  Only known state has an `owner` with explicit participant/membership IDs;
  all other states have no owner identity. Optional `reported_owner_name` and
  `reported_owner_handle` preserve unresolved provider metadata without
  establishing a join identity or changing unknown into known. Omitted metadata
  is cleared by the next complete state report, not carried forward.
- `OwnershipRequestObservation`: kind `request`, `request_id`, action
  (acquire/divest), and identified `requester` participant join.
- `OwnershipOutcomeObservation`: kind `outcome`, the same ticket/action/
  requester fields, and outcome completed/failed/cancelled. Completion describes
  the reported request outcome, **not** an inferred change in owner.

All identifiers preserve opaque nonblank bounded text (512 characters; reporter
session 256), reasons are optional text up to 4096 characters, and extra or
inapplicable fields are rejected atomically. Unknown ownership is different from
unowned and RTI-owned. State identity includes source, session, namespace,
federation, object instance and attribute; repeated state reports replace the
complete report. No publisher, last updater, callback source or successful
service call establishes an owner. Object/federate lifecycle events do not
silently rewrite these last-reported facts.

Ticket correlation additionally requires exact request ID, action and requester
participant/membership IDs in that reporting scope. A fresh join/session gets
fresh IDs. Duplicate retained request IDs become ambiguous and invalidate any
matched completion. Outcomes before requests or without an exact retained
pending ticket increment `unmatched_outcomes` and remain explicit history,
without retroactive completion. Adapters must not reuse tickets for unrelated
requests; deduplication cannot detect reuse after ledger eviction.

Default independent capacities are 500 latest attribute scopes, 500 requests
and 1000 arrival reports (`max_ownership_states`, `max_ownership_requests`,
`max_ownership_transitions`). States expire least-recently-updated; oldest
recorded requests expire first, including pending ones; history expires oldest
arrivals. Expiry is not a successful outcome. Ordinary event-history eviction
does not erase these indexes; clear/restart resets them. Pending means last
reported evidence, not a live RTI query. The UI provides separate owner and
request tables with independent row limits, scoped search/selection, a reported-
state distribution and a retained report sequence. No ownership command is
advertised. Python/Java fixtures cover identified, unknown and pending evidence.

## Regions and DDM inspection (optional, read-only)

`regions_inspection` advertises additive `inspectors.regions: RegionsResponse`;
the older provider-level `regions` capability keeps its earlier meaning. The
model has `snapshots`, `transitions`, positive `capacity`/`history_capacity`,
`dropped_snapshots`, `dropped_transitions`, and basis
`explicit-region-observations`. Available/partial inspection must supply this
model. It is independent of MOM, analytics, commands and topology relationships.

Reserved `region_observation` is a discriminated named input. Common fields
are session/namespace/federation IDs, required `evidence`
(declared/observed/unknown), optional identified `subject` and bounded reason:

- `RegionStateObservation`: kind `region`, explicit region-instance `region_id`,
  status active/deleted/unknown, and optional `extents`. An extent has a unique
  `extent_id` and explicit `dimensions` array. Each range has a unique-in-extent
  `dimension_id`, `lower_bound`, `upper_bound` and optional
  `dimension_upper_bound`. Null/omitted extents mean unreported; `[]` explicitly
  reports an empty set. No missing ranges are fabricated or retained from a
  previous complete replacement.
- `RegionAssociationObservation`: kind `association`, `association_id`, status
  active/ended/unknown, explicit target kind (object/interaction), `target_id`,
  direction (publication/subscription/update/send/unknown), optional `region_ids`
  and object `attribute_keys`. Null/omitted sets mean unreported; empty arrays
  are explicit empty sets. Interaction associations cannot report object
  attribute selections. These are adapter-reported labels, not an authorization
  to invoke a corresponding RTI service.
- `RegionRoutingObservation`: kind `routing`, `routing_id`, status
  active/ended/unknown, optional region references, target metadata and identified
  producer/recipient joins. Missing endpoints stay unknown. A routing report is
  not a delivery receipt and never creates a topology relationship implicitly.

Range bounds are canonical nonnegative decimal **strings**, up to 128 digits,
with no whitespace, sign, leading zero (except `0`), exponent or numeric coercion.
Lower must not exceed upper; an explicitly supplied dimension limit must cover
upper. This preserves large provider bounds exactly through Python/Java/JSON/
TypeScript. The portable schema rejects nondecimal characters, including a final
newline. Up to 256 extents, 256 dimensions per extent and 4096 total ranges per
region report are accepted; reference/attribute arrays are unique and bounded
at 4096. Identifier/reason limits match the other typed inspectors.

Latest identity is source + reporter session + namespace + federation + kind +
explicit region/association/routing ID. Reused display names or IDs in another
scope do not merge. Region IDs identify instance lifetimes; fresh lifetimes need
fresh IDs or a fresh capture scope. References resolve only against explicitly
reported regions in that exact scope; an unresolved reference does not create a
phantom region. Deletion never silently changes a separate association/routing
snapshot. Neither extent overlap nor matching class names infer routing,
subscription coverage, current membership or delivery.

Default capacities are 500 latest reports and 1000 arrivals (`max_regions`,
`max_region_transitions`). They expire separately with loss notices and survive
ordinary event-history expiry. Clear/restart resets both. The UI adds kind/
status/evidence/search filters, paged reports, exact-scope reference navigation,
selected report details and retained sequences. Range plots use only explicitly
reported dimension limits, approximate ratios at 0.01% display precision and a
minimum-width marker; original bounds remain exact text. Unknown/zero limits
do not produce an invented proportional plot. At most 40 dimension ranges are
drawn, with an explicit limit and complete selected report details. Pause freezes
the data; clear preserves a missing selection. No DDM command is advertised.

## MOM reports (optional, read-only)

`mom_inspection` advertises `inspectors.mom: MomResponse`, not the older
provider-level `mom` flag. Available/partial inspection requires this named
model; absent, disabled or unsupported inspection never disables ordinary
roster, attributes, time or session control. It has `snapshots`, `transitions`,
positive `capacity`/`history_capacity`, separate dropped counts and basis
`explicit-management-reports`.

Reserved `mom_observation` is a discriminated input. Every report requires
session/namespace/federation IDs and `report_id`; reason is optional. Output
adds the observer source, sequence and arrival time. Kinds are:

- `members`: explicit coverage complete/partial/unknown and optional members.
  Null/omitted members means unknown; `[]` explicitly reports no entries. A
  complete report requires an array. Each member has status joined/resigned/
  unknown and a `ParticipantReferenceResponse`: an explicit participant/join
  identity, unresolved display name/handle metadata, or both. At least one form
  of evidence is required. Identified joins cannot repeat within a report;
  unresolved same-name entries need not identify the same join. These reports
  never replace the ordinary observer roster, even with complete coverage.
- `service`: service name, invoked/completed/failed/unknown status, optional
  subject reference, invocation ID and error name. These are reported evidence,
  not observer-dispatched commands. No command completion, paired invocation,
  duration or provider service meaning is inferred.
- `counter`: counter key, cumulative/gauge/unknown semantics, optional unit and
  subject reference, and exactly one finite numeric `value` or opaque
  `value_text`. Booleans, numeric strings and nonfinite numbers are rejected.
  Integers outside ±9007199254740991 require text to avoid JavaScript precision
  loss. Zero is a sample, not missing data. Text is never parsed into a chart.

Latest identity is source/session/namespace/federation/kind/report ID. A complete
replacement clears omitted fields rather than carrying stale errors or members
forward. Default bounds are 500 latest reports and 2000 arrivals
(`max_mom_reports`, `max_mom_transitions`), independent of event retention.
Clear/restart resets both indexes. The UI filters reporting scope, service
status and text, pages member/service/counter evidence, retains selected scope
after clear, and charts only exact numeric counter series. Series also separate
counter key, identified subject, unit and semantics. Unresolved subjects are
metadata within the report ID, not invented joins. Opaque arrivals break chart
lines; rates, resets and complete federation totals are not inferred.

## FOM metadata (optional, read-only)

`fom_inspection` advertises `inspectors.fom: FomResponse`, with snapshots,
transitions, independent capacities/dropped counts and basis
`adapter-supplied-fom-metadata`. It is independent of MOM, analytics and native
bindings. Available/partial inspection supplies the model; partial metadata can
be useful even when referenced types or parents are unavailable.

Reserved `fom_observation` requires session/namespace/federation IDs, model ID,
revision, element ID and present/removed/unknown status. Name and description
are optional; an ID display fallback never becomes a reported name. Kinds are:

- `object-class` and `interaction-class`: explicit parent state root/parent/
  unknown, parent ID if and only if state is parent, and declared/effective/
  unknown field scope. Attributes or parameters are optional arrays with unique
  field IDs, optional names, datatype references and descriptions. Null/omitted
  lists are unknown; empty arrays are explicitly empty. No inherited fields
  are synthesized from the reported hierarchy.
- `datatype`: primitive/enumeration/array/record/variant/opaque/unknown encoding
  and optional opaque representation. Only record/variant allow fields; only
  enumeration allows enumerators (unique names with opaque string values;
  aliases may share a value); only array allows element-type reference and fixed
  cardinality. Unknown cardinality does not imply a variable-length array.
- `dimension`: optional exact upper bound and unit. Dimension upper bounds and
  array cardinalities use the same canonical nonnegative decimal strings as
  region bounds, up to 128 digits. Numeric coercion is not accepted.

Latest identity is source/session/namespace/federation/model/revision/kind/
element ID. Revisions are explicit adapter identifiers, not ordered or guessed
from time. References resolve only within that exact model scope and target
kind. A same-name/ID element from a different reporter or revision never fills
a missing reference. Parent cycles are visible reports, not recursively
expanded forever: the UI stops at a cycle or 64 ancestors and discloses it.
No missing parent becomes an invented root. Datatype navigation is one explicit
link at a time; recursive types never trigger recursive decoding.

Each report completely replaces its element snapshot. Defaults are 1000 latest
elements and 2000 arrivals (`max_fom_elements`, `max_fom_transitions`), surviving
event expiry independently and resetting on clear/restart. Field/enumerator
arrays are bounded at 4096. Search, model/kind/status filters, paged elements,
fields and history, selected metadata, parent/child/type navigation and exact
bounds use generated types unchanged with Java. Selection survives clear as a
missing element rather than silently switching revision. Present/removed is
adapter-reported metadata status, not proof of the currently loaded federation
schema. The generic observer does not parse provider FOM files, infer domain
semantics, decode runtime bytes or advertise a schema-editing command.

## Recording and replay (optional, read-only with respect to the federation)

`recording` advertises an observer feature, not an RTI service. Missing MOM,
ownership, DDM, analytics or command support does not prevent capture. With the
feature disabled (`create_app(..., enable_recording=False)` or backend CLI
`--no-recording`), its paths and
routes are absent and independent observation remains usable. Discovered paths:

| Resource key | Method | Named boundary |
| --- | --- | --- |
| `recordings` | GET | `RecordingStatusResponse`: process scope and nullable `CaptureStatus` |
| `recording_start` | POST | `RecordingStartRequest` → `CaptureStatus` |
| `recording_stop` | POST | `RecordingTargetRequest` → `CaptureStatus` |
| `recording_export` | GET, `recording_id` / optional `storage_format` query | `RecordingDocument` or negotiated `CompactRecordingDocument`, JSON attachment |
| `recording_validate` | POST | Either named portable document → validated same format, never expanded into a full-state array |
| `recording_analyze` | POST | `RecordingAnalysisRequest` or `RecordedWindowAnalysisRequest` → `AnalyticsResponse` |

The reference paths are `/api/recordings`, `/start`, `/stop`, `/export`,
`/validate`, `/analyze` (the latter five beneath `/api/recordings`). Clients
discover every resource key rather than using a captured document's paths.
Request bodies, including chunked bodies, are bounded at 16 MiB before JSON
parsing. Validation errors do not echo captured federation data. Imports do not
open files, follow URLs, register sessions or dispatch provider operations.

One process-local capture is retained, shared by browser clients. Start takes a
nonblank label (1..120 characters), optional `replace_id`, `max_frames` (1..2000,
default 600), `max_bytes` (65536..16777216, default 16 MiB) and optional
`storage_format` (default `full-v1`). Only a live backend advertising
`recording_compact` accepts a client's `compact-v2` negotiation. An active capture
cannot be replaced. A stopped capture can be replaced only by explicitly naming
its retained ID; the UI confirms the data loss and offers download first. Wrong
IDs/conflicts return 409. An initial state that does not fit is rejected without
discarding an earlier capture. Stop is ID-scoped/idempotent; export requires a
stopped capture so the file has a fixed timeline. Navigation, page refresh or
closing the browser does not stop capture. Backend restart loses an
undownloaded capture; this is not a durable server-side recording archive.

`RecordingDocument` version `umbra-observer-recording-v1` contains recording ID,
label, initial `ContractResponse` with exact aggregated capability provenance,
start/stop wall times, duration, explicit stop reason, bounds/accounted bytes and
ordered `RecordingFrame` records. A frame has index, monotonic offset, capture
wall time, change (start/events/metadata/clear/checkpoint/stop), new `arrivals`
and a complete `StateResponse` including retained `normalized_events`.

`CompactRecordingDocument` version `umbra-observer-recording-v2` preserves the
same complete states with a baseline plus strict named set/delete/splice edits.
Every reconstructed state passes `StateResponse`/`RecordingFrame` validation and
the shared transition rules. Encoded bytes, per-state size/depth/nodes, cumulative
reconstruction work and edit counts have separate fixed limits. New status
fields disclose format/work; absent legacy metadata stays unknown. Compact
export requires the explicit matching `storage_format=compact-v2`; a mismatch
returns 409, not a silent downgrade or new format for an old client. V1 start/
export defaults and imports remain supported. See [RECORDING.md](RECORDING.md)
for the exact codec, bounds, negotiation and independent consumer rules.

The zero-offset baseline preserves all current inspectors, context, capability
coverage and explicit retained-history loss. It does not claim a complete earlier
session. Every committed event batch has one atomic frame and retains **all**
its new envelopes, even those immediately expired from ordinary event history.
Metadata/capability/adapter changes and clear boundaries have their own frames.
One-second checkpoints also preserve clock-dependent inspector ages/rates when
no event arrives; stopping takes a final state. Inspector snapshots are stored,
not reconstructed from guessed event meanings, current clocks or later data.

Limits stop capture rather than evicting earlier frames or interrupting event
collection. Stop reasons are user/frame-limit/byte-limit/invalid-state/backend-
stopped; v2 additionally reports state-limit/decode-limit/edit-limit. Duration
ends at the last accepted frame; a rejected over-budget tail
is not replayed or extrapolated. `captured_bytes` is conservative accounted
UTF-8 document space (including manifest/wrapper reserve), not exact file size
or process RSS. Models verify bounds, finite JSON, versions, manifest/baseline
agreement, frame order, retained-window/count consistency, complete contiguous
arrivals, cursor continuity and no rewriting/resurrection across commits.
Clearing empties retained history without resetting the lifetime next cursor.

The React replay clock uses monotonic offsets, selectable 0.25×..8× speed,
play/pause, time seek, exact frame seek and previous/next state. Equal-offset
commits remain individually inspectable by frame; time seek uses the last state
at/before its clock. Between frames, evidence and pending ages remain as captured,
never extrapolated. Analytics is recomputed through a named validated request
using that frame's captured wall time and selected window, not today's time.
Missing analytics still leaves recorded events, metrics and inspectors usable;
failed/stale analysis never substitutes another frame's or a live result.

A prominent replay banner and connection badge remain across workspaces. Live
snapshot/stream/analytics subscriptions stop while replay is displayed; returning
to live reconciles from a fresh snapshot. The underlying backend/federation can
continue collecting. No captured URL is followed, no captured capability grants
authority, and Actions cannot load or submit provider commands during replay.
The file may contain sensitive context, names/handles and provider extensions;
it is **not credential-scrubbed** and is different from a shareable configuration.
Review it before sharing. Validation/import/analyze do not persist the file.
Compact replay expands only the current state in a dedicated worker, not all
states on the page. Seeking yields to newer requests; a pending banner names
the actual displayed state separately from its requested target. Recorded
analytics pauses during reconstruction, and pending/error replay never falls
back to a live snapshot. Changing/closing the recording terminates its worker.

Adding recorded states as model inputs makes FastAPI expose distinct `-Input`
and `-Output` schema components where serialization differs. These are generated
OpenAPI names, not changed JSON field names or a contract version bump. The UI
aliases response schemas explicitly and keeps the recursive JSON extension
boundary separate. The portable response schema also includes the recording
models. HTTP-read conformance checks advertised recording status only; capture,
import, clock/seek and command-isolation tests are separate internal lanes.

## Optional observer alerts

`alerts` advertises `GET api_paths.alerts` returning `AlertReport` independently
of MOM, analytics and provider commands. Separate `alert_controls` advertises
`alert_configuration` (PUT `AlertConfigurationUpdate`), `alert_reset` (POST
`AlertResetRequest`) and `alert_acknowledge` (POST `AlertAcknowledgeRequest`).
Each returns the validated report. Missing/disabled controls keep read-only
evidence useful. Discovery is not authentication or provider authority; a static
Java read fixture does not promise mutations.

`AlertConfiguration` requires version `umbra-observer-alerts-v1` and at most 64
unique discriminated rules within 64 KiB. Kinds are `stale-observations`,
`errors`, `dropped-history` and `pending-commands`, with only their applicable
parameters. Named finite models reject unknown fields and coercible counters.
Every mutation carries `expected_instance_id`; configuration/reset also carry
`expected_revision`. Restart/conflict returns 409 without applying stale input.
Configuration files cannot choose resource URLs.

Results distinguish disabled, unknown, clear, firing and explicitly ended;
metric bounds, threshold/unit, coverage and reason agree with the actual rule.
Evidence retains its rule/generation, original proof and acknowledgement in a
separate 500-record history with explicit expiry. Error detail is bounded to
512 marks per rule; overflow preserves exact counts where measurable and honest
lower/upper bounds across uncertain window boundaries. History loss retains
baseline/current counters; pending work identifies the oldest cached command/
target and any unknown effect. Provider payloads/arguments are not copied.

Rules observe committed ingress, not just retained events. Never observed is
unknown; idle is not disconnected. An ended subject requires an exact source/
session filter and explicit lifecycle evidence; resignation also requires a
federate filter. Late ordinary observations cannot reopen that watch. Root
filters do not silently combine nested protocol identities. Pending checks use
dispatcher caches only; unsupported commands do not disable other rule kinds.
Exact legacy textual scope fields remain supported. Optional `scope.perspective`
uses the same named perspective model as the inspectors; missing actor/join
attribution is not inferred from display names or nested MOM subjects.

Configuration/evidence last for this process, not a backend restart. Download/
import explicitly keeps rules, not evidence/credentials. Pause freezes the view
while evaluation continues. Recordings omit this separate resource; replay
never fetches/mutates live alerts. See [ALERTS.md](ALERTS.md) for exact limits.

## Saved frontend workspace configuration

`WorkspaceDocument` is `umbra-observer-workspace-v1`, with a name and named
`WorkspaceSettings`; `WorkspaceLibrary` is separately versioned and contains
the current settings plus at most 12 uniquely named documents. Models reject
unknown data/authority fields and type coercion; defaults are normalized into
required response properties. Complete canonical UTF-8 document/library bounds
are 64/512 KiB, independent of field, 64-chart-target and 64-federate-preference
limits. `federate_views` contains uniquely keyed, named event/declaration/time
settings for each selected participant. Switching participants does not inherit
another participant's selected time scope or discard their filters; global
views remain independent. Older `federate_*` fields remain default templates
for participants without a saved entry, not a shared mutable view owner.

The browser validates trusted generated schemas using precompiled offline
validators, supplemented by exact-target/name uniqueness and full-document
size checks. It never evaluates an imported schema or follows imported URLs.
Python and frontend runtimes share a normalization/rejection corpus and drift
checks. See [WORKSPACES.md](WORKSPACES.md) for the schema export/check commands.

The optional `workspace_validation` capability independently discovers
`workspace_defaults` (GET) and `workspace_validate` (POST). These return named
`WorkspaceDefaultsResponse` and normalized `WorkspaceDocument` respectively;
they neither persist settings nor apply actions, captures or alert rules. Omit
this capability/routes on servers that do not implement them: frontend-local
workspace storage and validation still work with Java/static/read-only servers.

Browser storage holds configuration per-origin, not backend authority or a
cross-device database. Invalid/unknown versions are protected from overwrite;
observed cross-tab changes require explicit reload. Quota failures do not stop
observation. Applying configuration is explicit and disabled while paused or
replaying. Unsupported feature choices stay dormant, and missing exact choices
do not fall back to other participants, reports or values. Legacy participant
name filters still have the OBS-127 identity migration boundary.

## Backend replacement rules

### Optional observation analytics

`GET /api/analytics?window_seconds=300&bucket_count=30&federate=Example`
returns `AnalyticsResponse`. The backend advertises `analytics` as available
or partial and adds `api_paths.analytics` only when it implements this resource.
Backends without it continue to support events and inspectors; the React UI
shows an unavailable analytics state without treating the whole service as
offline. The reference service accepts windows of 30..3600 seconds and
10..120 buckets. `federate` is an optional exact identity filter.

The reference backend takes a dedicated `AnalyticsWindow` projection under the
same store lock: contract version, complete retained normalized envelopes,
persistent federate roster, retention and optional storage coverage. It does not build, validate
or serialize unrelated object-history/MOM/FOM/DDM inspectors merely to count
events. This is not sampling or downsampling. Window/scoped attribution, idle
roster entries, explicit unknown/numeric times and loss semantics are unchanged;
tests compare complete aggregate outputs against full-state aggregation.

Recorded analysis can use `RecordedWindowAnalysisRequest`: `window` with those
four original named fields and optional `storage`, finite `captured_at`, bounded
window seconds and bucket count. Clients include storage only when the live
backend separately advertises usable `storage_inspection`; older strict
backends still receive the original shape.
Window validation checks complete ordered retained cursors, counts, unique
roster identities and finite JSON. A backend advertises
`recording_analytics_window` only with usable `recording` and the discovered
`recording_analyze` resource. Clients use it when advertised; otherwise they
continue sending the original `RecordingAnalysisRequest` with a full frame.
The reference backend accepts both and produces identical analytics at the
captured wall time. A captured document never chooses the live backend's
request shape or endpoint. This keeps older/Java recording implementations
compatible rather than silently making recorded analysis unavailable.

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
- Participants are the union of the persistent observed roster (including idle
  rows) and names explicitly attributed to events within the requested retained
  window. Each has
  identity/role/status/source/last-seen metadata, window counts,
  observed `object_keys`, and the latest numeric logical timestamp in event
  sequence order. Unreported values are null, not zero. A scoped response
  includes only that identity and explicitly attributed events. A name without
  cached roster metadata has `roster_retained: false`, status `observed`, no
  inferred role/handle/membership, and last-seen/source from actual observations.
  Optional `roster_storage` carries projected roster coverage; missing coverage
  on an older backend is unknown, not zero loss. With participant provenance,
  union/grouping uses the exact reporting perspective rather than a name.
  Optional `scoped_perspective` echoes the exact `perspective` query filter;
  it never substitutes a name match. The older `federate` query intentionally
  remains a display-name filter and can include several reporting perspectives.
  A row's current alias may differ from the older matching event label; its
  attributed count still accounts for those retained observations exactly.
- Legacy label lookup uses `federate` or `participant`, including a structured
  `name`/`id`/`handle`, then explicit `federate_name`/`participant_name` fields.
  Root named attribution takes precedence when present. Source names never
  imply participant identity. Attributed object observations
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
2. Implement `/api/contract`, `/api/state` and `/api/events` with the shapes
   above. Implement streaming only when advertising a supported transport;
   HTTP-only backends can use snapshot polling.
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
