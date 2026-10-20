# Junior guide: the observer frontend and backend

This guide explains the observer system without assuming RTI or frontend
experience. The short version is:

```text
RTI/provider -> adapter -> normalized event -> observer backend -> JSON contract -> dashboard
```

The dashboard does not know whether the event came from Python, Java, C++, a
test fixture, or a different RTI. That separation is the feature we protect.

## 1. What are the frontend and backend?

### Frontend

The maintained frontend is the browser dashboard package:

```text
packages/umbra-rti-observer-ui/src/
```

It is React and TypeScript. The old single-file dashboard at
`packages/umbra-rti-observer/src/umbra_rti_observer/static/dashboard.html`
remains as a zero-build fallback for small deployments.

It contains the visual layout, CSS, and browser JavaScript. Its job is to:

1. Discover which observer contract the server supports.
2. Request a state snapshot.
3. Listen for new events.
4. Render generic metrics, participants, objects, interactions, charts, and
   event history.

The frontend must not import an RTI library, parse a FOM, call a Python
callback class, or understand a Java object. It only understands JSON fields
from the observer contract.

### Backend

The backend is the process that serves the dashboard and produces observer
data. The reference implementation is the optional FastAPI layer in:

```text
packages/umbra-rti-observer/src/umbra_rti_observer/server.py
```

The backend's job is to:

1. Receive normalized events from an adapter or another producer.
2. Keep bounded history and live inspector state.
3. Expose the versioned HTTP and streaming contract.
4. Serve the built React dashboard, or fall back to the zero-build dashboard.

The backend is not required to be Python. The Python service is simply the
first implementation.

The FastAPI layer has named, strict Pydantic models at its edge. For example,
`EventBatchRequest` validates the batch shape before calling
`ObserverStore.publish_many`, and `StateResponse` validates the snapshot on
the way out. Event and capability extensions have an explicit `extensions`
field inside the Python model; serialization flattens that field so the
browser keeps the existing contract. This is separate from the core typed
records: Pydantic protects the HTTP boundary, while the core remains usable
without FastAPI or Pydantic. The browser therefore sees the same contract
whether the producer behind it is Python, Java, or a limited RTI with only a
subset of capabilities.

### The Python boundary in one sentence

The adapter may receive provider-shaped values, but `ObserverStore` does not
store arbitrary nested dictionaries. It stores typed observer records and only
turns them into JSON objects when returning a snapshot, event page, or stream
message. This is why replacing the Python backend with Java does not require
the dashboard to learn Python types—and why changes to the Python internals
should preserve the existing JSON contract.

## 2. The complete flow

Imagine that a provider reports an object update.

### Step 1: the provider callback happens

An RTI-specific callback receives provider-shaped values. Those values may be
Python objects, Java objects, handles, byte arrays, or vendor-specific types.

### Step 2: an adapter translates it

The adapter owns provider knowledge. For Umbra's Python RTI, that adapter is:

```text
packages/umbra-rti-observer-umbra/src/umbra_rti_observer_umbra/adapter.py
```

It turns the callback into a plain JSON-compatible event:

```json
{
  "event_type": "object.updated",
  "family": "generic",
  "source": "umbra-python",
  "object_key": "object-17",
  "attributes": {
    "state": "ready"
  }
}
```

The adapter should not send provider callback objects to the browser.

The adapter can also publish a capability manifest next to its event sink. It
should say whether logical time, MOM, ownership, regions, or other services
are `available`, `partial`, `disabled`, `unsupported`, or `unknown`. This is
how a smaller RTI can provide a useful dashboard without pretending to have
Umbra's complete service surface.

### Step 3: the observer store normalizes it

`ObserverStore` adds the fields needed by every backend client:

```json
{
  "sequence": 42,
  "event_type": "object.updated",
  "family": "generic",
  "source": "umbra-python",
  "observed_at": 1760000000.123,
  "object_key": "object-17",
  "attributes": {
    "state": "ready"
  }
}
```

It also updates live object, interaction, and participant inspectors. Those
inspectors are kept separately from the bounded event history, so old events
can be discarded without making active objects disappear.

### Step 4: the backend exposes the contract

The browser uses these resources:

```text
GET /api/contract       What contract version is supported?
GET /api/state          What does the system look like now?
GET /api/events         Which events came after sequence N?
WS  /ws/events          Stream snapshots and new events
GET /api/events/stream  SSE alternative to WebSocket
```

The full protocol is in [CONTRACT.md](CONTRACT.md).

### Step 5: the frontend renders the result

The browser's `ObserverClient` and `useObserver` hook handle URLs, contract
discovery, snapshot loading, WebSocket connection, polling fallback, and
stream messages. The rendering code consumes generated OpenAPI types and does
not care which language produced them. `LiveCharts` derives generic series
from the event stream; it does not require RTI-specific chart data.

## 3. The contract in plain language

The contract has five important rules.

### Rule 1: discover the version first

The browser requests:

```text
GET /api/contract
```

The response must identify:

```json
{
  "contract_version": "umbra-observer-contract-v1"
}
```

If the version is not supported, the frontend should show a clear failure
instead of guessing how to interpret the response.

### Rule 2: capabilities explain what is available

The same contract also reports feature maturity:

```json
{
  "logical_time": {"state": "available"},
  "mom": {"state": "partial", "reason": "Only selected callbacks are observed."},
  "ownership": {"state": "unsupported"}
}
```

The states mean:

```text
available    supported and enabled
partial      only part of the feature surface is observed
disabled     intentionally turned off
unsupported  the provider or backend cannot provide it
unknown      no capability declaration was received
```

An unavailable feature is not an error for the whole dashboard. Its panel
should say why it is unavailable, while unrelated panels continue to work.

### Rule 3: events are plain JSON

Every event needs:

```text
sequence       increasing number assigned by the observer backend
event_type     generic name such as object.updated or runtime.error
family         explicit grouping chosen by the producer
source         producer/connection label
observed_at    backend observation timestamp
```

Additional fields are allowed when they are JSON-compatible. Common optional
fields include `federate`, `object_key`, `interaction_key`, `attributes`,
`parameters`, `tag`, and `details`.

### Rule 4: snapshots describe current state

`GET /api/state` returns the current dashboard state. It includes:

```text
status
context
live_metrics
retention
adapters
capabilities
federate_roster
inspectors.objects
inspectors.interactions
normalized_events (when include_events=true)
```

The dashboard uses this snapshot to paint the page. It does not reconstruct
live object state by replaying provider callbacks.

### Rule 5: streams use envelopes

WebSocket and SSE messages use the same message types:

```json
{"type": "snapshot", "state": {}}
{"type": "events", "events": [], "state": {}}
{"type": "heartbeat"}
{"type": "gap", "retention": {}}
```

If a client is too slow and history has already been evicted, the backend
sends `gap`. The client should request a fresh snapshot and continue from the
retained sequence window.

Think of resuming a stream as a bookmark in **one observer's notebook**.
The optional `observer_lifetime.instance_id` identifies that notebook;
`revision` orders its atomic updates. Neither value identifies an RTI
connection or a federate join. After a backend restart, sequence 1 can be a new
observation: discard the previous notebook rather than merging those rows.

A backend advertising `stream.resumption: true` accepts `instance_id` and
exclusive `after` query parameters on either stream. It reports `fresh`,
`resumed`, `reset` or `gap` in the initial snapshot. Its cursor advances only
through delivered data. A resumed summary cannot skip its subsequent backfill;
a gap cannot be repaired by inventing events. A complete retained snapshot,
including an empty array after clear, is authoritative for that observer cut.

The maintained React client tries advertised WebSocket, then SSE, then HTTP
polling. It refreshes discovery every five seconds and on Sync now, so a
backend can change capabilities or resource paths without a page reload.
Offline/online signals, bounded retries and an idle-stream watchdog handle
interruptions. Slow old HTTP replies and callbacks from a closed connection
cannot undo a newer streamed clear. Diagnostics shows the actual delivery
transport and separately labels the displayed observer lifetime.

Older services can omit these additions. Full snapshots still work, but the
UI does not pretend to know their observer incarnation. MOM, analytics,
commands and stable participant IDs are not prerequisites for streaming.
Use `schemas.py` for the named wire models, `streaming.py` for the shared
Python flow, `src/api/client.ts` in the UI for transport/discovery, and
`src/app/liveState.ts` for retained-window reconciliation. The stream DTOs are
included in OpenAPI and generated TypeScript; do not redefine their fields in
individual components.

## 4. Running the reference backend

From the repository root:

```powershell
python -m pip install -e "packages/umbra-rti-observer[web]"
python -m umbra_rti_observer --demo
```

Then open:

```text
http://127.0.0.1:8765/
```

The demo is intentionally provider-neutral. It proves that the frontend can
work before an RTI adapter exists.

For the maintained React dashboard, run the backend first and then from
`packages/umbra-rti-observer-ui`:

```powershell
pnpm install
pnpm generate:api
pnpm dev
```

The Vite server proxies `/api` and `/ws` to the Python backend. For a
production build, run `pnpm build` and set `UMBRA_OBSERVER_UI_DIST` to the
resulting `dist` directory; FastAPI will serve it at `/`.

For a quick manual check, inspect these URLs:

```text
/api/contract
/api/state?include_events=false
/api/events?after=0&limit=20
/api/metrics
```

## 5. How to replace Python with Java

There are two separate things that might be replaced:

### Replace only the provider adapter

Keep the Python observer backend, but write a Java producer that sends
normalized events to `POST /api/events`. This is the smallest change.

```text
Java RTI callbacks -> Java adapter -> POST /api/events -> Python observer -> dashboard
```

### Replace the backend too

Keep the built React dashboard unchanged. Implement the contract in the Java server:

1. Serve the built React dashboard (or the zero-build fallback).
2. Implement `GET /api/contract`.
3. Implement `GET /api/state`.
4. Implement `GET /api/events` with cursor and gap metadata.
5. Implement WebSocket or SSE streaming.
6. Advertise which features are available, partial, disabled, or unsupported.
7. Translate Java RTI callback values into normalized JSON events.
8. Preserve the contract version and field meanings.

If the Java service is hosted at another URL, configure the dashboard's
`observer-api-base` and `observer-stream-path` meta tags and configure CORS/
WebSocket origin policy as appropriate.

## 6. What belongs in each layer?

| Concern | Correct home |
| --- | --- |
| Java/Python/vendor callback signatures | Provider adapter |
| Handle and byte-array conversion | Provider adapter |
| FOM loading and subscriptions | Application/federate code |
| Event sequence numbers | Observer backend/store |
| Bounded history and gap reporting | Observer backend/store |
| Capability declarations | Provider adapter and observer backend |
| HTTP, WebSocket, and SSE routes | Observer backend |
| Cards, tables, filters, and status display | Frontend |
| Domain-specific meaning | A separate domain application, not this observer |

When deciding where code goes, ask: “Would this still make sense if the RTI
provider and implementation language changed?” If the answer is no, it belongs
outside the frontend contract.

## 7. Common problems and where to look

### The dashboard says `offline`

Check that the backend is running, then open `/api/contract` directly. If that
URL fails, the problem is transport or server startup—not the dashboard
renderer.

### The dashboard says contract mismatch

The backend and static frontend disagree about `umbra-observer-contract-v1`.
Do not patch the frontend to guess. Either use the matching dashboard asset or
intentionally version the contract.

### The page loads but shows no events

Check `/api/state?include_events=true` and `/api/events?after=0`. If the state
is empty, inspect the adapter or producer. If the API contains events, inspect
the browser transport and rendering layer.

### The dashboard reports a gap

The client was slower than the configured bounded history. This is expected
behavior, not necessarily a provider failure. Refresh the snapshot; if gaps
are frequent, increase the backend history limit or improve the consumer.

### A panel says a feature is unsupported

This is a useful result, not a dashboard failure. The adapter or backend has
declared that the connected RTI cannot provide that feature. Use the remaining
panels, or connect a richer adapter when that information is needed.

### A Java backend works at `/api/state` but not in the browser

Check all of the following:

- `/api/contract` returns the expected version.
- State includes `contract_version` and the required snapshot fields.
- WebSocket/SSE messages use `type` envelopes.
- Cross-origin requests have the required CORS and WebSocket headers.
- Event sequences are numeric and monotonic.

## 8. Safe change checklist

The React workspace now has nine views: Federation (traffic and composition),
Federates (one participant's attributed observations), Objects (search, values,
comparisons and numeric attribute charts), Declarations (explicit publication/
subscription snapshots), Logical time (timestamps
and explicit grants), Events (search/filter/export), Inspectors (live snapshots),
Actions (owned-session controls and results), and Diagnostics (adapters, errors,
feature coverage, retention).

`useAnalytics` asks the optional `/api/analytics` endpoint for a selected time
window. `analytics.py` aggregates a validated snapshot into named Pydantic
models. The frontend plots those models; it does not know how an RTI binding
represents a callback. A Java backend can implement the same endpoint, or omit
the analytics capability while retaining the existing events and inspectors.

The time window applies to analytics and the federate timeline. The Events tab
searches all events loaded by the browser (up to 2,000); live inspectors reflect
the backend's latest snapshots and can outlive retained history. The optional
`inspectors.object_observations` model gives Objects and Federates independent
attributed snapshots. If Alice reports value 10, Bob reports 20 and an
unattributed update reports 30, Alice still shows 10, Bob 20 and Global 30.
Missing attributed data never falls back to global values.

`object_views.py` maintains those snapshots without web or RTI dependencies.
`schemas.py` validates the named snapshot/change models; the React components
use generated TypeScript refinements only at the explicit JSON-value boundary.
An older backend can omit the `object_observations` capability; useful events,
metrics and global inspectors remain available. MOM is not needed.

The Objects screen searches object names/classes/handles, selects a perspective
and filters active/removed rows. Comparisons show the prior value from that
same perspective; unknown is different from reported null. Bytes show their
hex and byte count, not an invented decoded datatype. A selected numeric chart
uses only retained finite numbers, not boolean/string conversions or a made-up
history from the latest snapshot. It shows at most 500 recent observations;
comparison tables initially render 100 rows with explicit load-more controls.
These views use the retained event window, not the analytics chart-time menu.
Removal followed by discovery starts a new local comparison generation.
The scoped snapshot index also has its own budget (5000 default), independently
of the retained event window. An expired scope can be reobserved, but its
comparison baseline starts fresh; it must not borrow a global/other federate's
old values. Capacity, cumulative scope evictions and currently omitted scoped
changes are explicit optional fields. Missing metadata on an older backend means
unknown capacity/loss, not zero loss. Objects, Federates and Diagnostics show
coverage; lists initially render 100 entries while search covers all supplied
snapshots. Classic global objects, roster and time scopes now have separate
count capacities too; every collection shares the encoded-evidence budget.
Clearing history clears snapshots too; restarting the backend resets the demo.

Analytics does not need the whole inspector tree. `analytics_window()` takes one
consistent retained-envelope/roster/retention/storage projection under the store lock;
`AnalyticsWindow` validates that named input and `build_analytics` uses exactly
the same aggregation rules as a full snapshot. Do not optimize by silently
dropping participants, samples or unknown values. Recorded analytics negotiates
the lightweight input through `recording_analytics_window`; an older backend
still receives the original full-frame request. The file's captured URLs or
capabilities never decide where the browser sends it.

Use [SCALE.md](SCALE.md) for repeatable workload measurements, explicit reference
budgets and the recorded limitations. Tiny functional tests
prove the model/flow; they are not evidence of high-volume performance.

The Logical time screen also has a persistent inspector, independent of the
analytics time window. `time_models.py` parses explicit provider-neutral time
observations without Pydantic or RTI imports; `time_views.py` maintains snapshots
and a separate bounded request ledger. `schemas.py` validates named input and
response models at the web boundary. `PersistentTimeView` consumes generated
TypeScript models; `TimeView` remains the optional windowed timestamp chart.

Think of a request ID as a ticket, not a timestamp. A request and grant match
only when source, session, federation and federate scope agree and both report
the same ticket. A timestamped object with value 100 does not mean the federate
has been granted time 100. A grant-only RTI still gets a useful last-grant
snapshot, with request analysis explicitly unavailable. If numeric time is
opaque, an adapter can omit it while reporting explicit ticket correlation.
Missing role flags and GALT/LBTS show Unknown, never guessed zeros or bounds.

The optional `time_inspection` capability does not depend on MOM, analytics or
commands. Older/disabled backends keep their other views. Request history has
its own capacity (250 by default); an event disappearing does not erase a
pending request, but ledger eviction can. Delay means the observer's measured
request-to-grant arrival interval, not provider execution latency. Fresh session
IDs on reconnect/rejoin prevent old tickets from closing new work. Duplicate
retained IDs become ambiguous; an explicit failed/cancelled/session-ended
outcome never becomes a successful grant. See CONTRACT.md for the exact wire
rules and producer ID-uniqueness obligation.

Think of latest snapshots as a cache and requests as a separate ticket ledger.
Either can expire first. A pending ticket without its latest snapshot stays
selectable, with missing snapshot coverage shown honestly; a later exact scoped
grant can still close it. Nothing borrows another participant's state. Likewise,
expired roster metadata does not erase names explicitly present in retained
events or silently switch the participant you selected.

`storage.py` supplies the shared count/byte collections, `schemas.py` validates
their coverage, and `StorageCoverage` displays it. The default evidence budget
is 6 MiB encoded JSON, not process memory. Whole records expire; values are not
clipped. Diagnostics separates count, byte, oversized, encoding and window loss.
Missing storage support on a Java/older backend is Unknown, while ordinary
events, attributes and time remain usable. An expired row is not a disconnected
participant or a removed object. See [STORAGE.md](STORAGE.md) before changing
limits or interpreting a missing row; soft reservations are not minimum quotas.

Declarations follow the same layering: `declarations.py` validates provider-
neutral typed records and maintains a capacity-bounded snapshot index;
`schemas.py` provides named object/interaction HTTP inputs and response models;
`DeclarationsView` displays generated TypeScript data. Neither the parser nor
the screen imports an RTI, parses a FOM, guesses from class names or needs MOM.

An attribute set has four distinct meanings: selected (possibly explicitly
empty), all (explicitly reported), unknown (not supplied), or not applicable
for interactions. A new report replaces the previous set; the adapter must
resolve provider delta calls before publishing a full snapshot, or say unknown.
Source/session/federation/federate, kind, direction and class key keep reports
separate. The event's federate is the explicitly reported declaration subject.
Matching a publisher and subscriber class name is not evidence of a route or
delivery. Explicit relationship evidence is a separate optional model.

The Topology screen consumes `inspectors.relationships`. `relationships.py`
validates dependency-free records and keeps a bounded latest-report index;
`schemas.py` validates the same named wire models; `TopologyView` uses their
generated TypeScript shapes. A Java server can supply those shapes unchanged.
Only adapter-supplied producer/recipient endpoints draw arrows. Each endpoint
has a participant ID and a membership (join) ID, scoped by namespace and
federation ID. Display names and RTI handles are labels, not identity: two
"Observer" names or reused handles never merge joins. A fresh join needs a
fresh membership ID; a reconnect needs a fresh reporting session ID.

The graph separates declared/observed evidence and active/ended/unknown reported
status, and offers keyboard-selectable nodes, search and filters. Separate
reporters remain separate edges with provenance in the table. The newest 40
filtered edges are drawn; the complete bounded index is paged in the table.
No edge means unknown evidence, not proof of no route. Old sessions remain
last-reported facts, not a current membership census. This new identity model
does not reinterpret the older name-attributed roster or time/object views.

Pause view freezes the local screen only. Data collection continues; command
submission is disabled until the view resumes. The separate `commands`
capability enables only explicitly registered control sessions. Available MOM
or time observations do not imply permission to send commands.

Synchronization and Save / restore are read-only coordination workspaces.
`coordination.py` validates explicit phases and maintains two bounded indexes:
latest scoped reports and a separate phase sequence. `schemas.py` exposes named
discriminated inputs and response models, and `CoordinationView` consumes the
generated types. Different adapters can translate different RTI callback shapes
into this contract; the generic observer never calls those APIs itself.

An operation ID is a ticket for an attempt; the label is just display text and
may be reused. Keep reporter sessions, federation IDs and local participant join
IDs separate. Local save completion can coexist with a federation-wide failure:
one participant finishing is not everybody finishing. The screen deliberately
shows both scopes and never fills in missing phases or declares success from
counts. An omitted participant report is unknown, while an empty array means
explicitly empty — neither proves full membership coverage. Selecting an
operation follows its retained phases across all scopes; table filters affect
only latest reports. Index/history losses are disclosed separately.

Each workspace has an independent capability and works without MOM or
analytics. Observer infrastructure being able to accept a model does not turn
an unsupported provider feature into partial support. No synchronization or
save/restore controls appear until a future adapter explicitly owns and
advertises those commands.

The new save/restore inspector uses `persistence_inspection`, not the earlier
provider-level `persistence` flag. Existing v1 backends keep that old meaning
without suddenly being required to implement this new model.

Interaction flow is a different question from declarations or topology:
"Which send and receive observations refer to the same explicit transmission?"
`interaction_flow.py` parses named dependency-free records and maintains a
bounded pairing ledger; the HTTP boundary validates named Pydantic models and
`InteractionFlowView` consumes their generated TypeScript types. It works with
the Java fixture and does not depend on MOM or analytics.

An adapter supplies a message ticket plus both endpoint join IDs in a shared
namespace/federation. Missing tickets/endpoints leave counted observations
uncorrelated; two identically named federates or two Signal-class messages are
not enough. A repeated ticket is ambiguous, not another successful delivery.
Separate reporters may supply the two observations, but their source/session
details are retained and the adapter must guarantee truly shared, unique IDs.
Each join needs a new membership ID. Sender/receiver name, handle or timestamp
does not replace that identity.

The chart measures observer arrival intervals on a monotonic local clock, not
network latency or logical-time advancement. Receiving first can still pair
explicit evidence, but does not produce a made-up positive duration. Unknown
durations stay unknown; a genuine zero stays zero. Legacy receive-only RTIs
still show counts and uncorrelated records. Cumulative counts include expired
flow records since clear, while charts/statuses describe only retained records;
loss is visible. Pairing never silently creates a topology edge or changes
ownership. Per-report details preserve the original endpoint metadata.

Ownership is another independent inspector. `ownership.py` keeps explicit owner
states, scoped request tickets and a separate bounded report sequence;
`OwnershipView` consumes generated models. An updater is not necessarily an
owner. Completing an acquire/divest call is not proof that ownership changed.
Only an explicit owner-state report changes the displayed owner. Unknown,
unowned and RTI-owned are distinct. A handle-only RTI can retain its reported
name/handle as unresolved metadata without inventing a participant join.

Tickets match only within the same reporter/session/federation/object/attribute
scope, request ID, action and requester join. Duplicate retained tickets are
ambiguous; a late unmatched outcome does not complete unrelated pending work.
Each object ID identifies an instance lifetime, not just its display name.
The chart counts reported attribute scopes, not a complete federation census.
Separate latest-state, request and history capacities disclose their own loss.
The new `ownership_inspection` capability is independent of MOM and analytics;
the earlier provider `ownership` flag does not promise this additive model.

`regions.py` supplies typed region extents, associations and routing reports.
`RegionsView` preserves exact decimal-string bounds, including integers beyond
JavaScript's safe-number range. BigInt arithmetic converts only a bounded
display ratio; it never rounds the original values through Number. A dimension
limit that was not reported stays unknown and produces no invented scale.
Very narrow ranges have a minimum-width marker, with exact text alongside it.
The graph is a dimension-range diagram, not a geographic map or overlap proof.

Region references resolve only in the same source/session/namespace/federation
scope. Declared associations and observed routing remain separate reports; no
association, overlap or class match establishes message delivery. Deleting a
region does not silently rewrite another report. Null/omitted extents or sets
mean unreported, while empty arrays mean explicitly empty. Search/filter,
selection, exact-scope links, bounded sequences and coverage work with the
unchanged Java fixture. `regions_inspection` is separate from the legacy
provider `regions` flag, and no ownership or DDM controls are advertised.

The MOM reports screen is independent of ordinary membership. `mom.py` keeps
typed member, service and counter reports; `MomView` reads the generated
`MomResponse`. Think of a management roster as a report from a particular
source, not a replacement for everything the observer knows. Its coverage can
be complete, partial or unknown. A name/handle-only entry stays unresolved;
inventing a join ID would make later correlation unsafe. Service errors are
evidence, not a failed command issued by this dashboard.

Counters carry either a finite numeric value or opaque text, plus explicit
semantics and optional unit. The chart never converts strings to numbers or
guesses a rate/reset. Wide integers use text rather than losing precision in
JavaScript. Only the same reporting scope, report ID, counter key, subject
identity, semantics and unit belong to one series. Latest evidence and arrivals
have separate bounds/loss. `mom_inspection` enables this model; the old provider
`mom` flag alone does not. An RTI with no MOM still supports independent
attributes, time, roster and explicitly registered lifecycle commands.

The FOM browser follows the same split. `fom.py` indexes adapter-supplied class,
attribute, parameter, datatype and dimension metadata; `FomView` navigates
generated `FomResponse` models. The adapter—not React or the generic observer—
owns any provider-specific schema API or FOM parsing. Metadata references need
an exact reporter/session/namespace/federation/model/revision match. If a type
was only reported in revision 2, it must not resolve a revision-1 field.

A class explicitly says whether it is a root, has a parent or has unknown
ancestry. Its field list explicitly says declared/effective/unknown scope.
Unknown is not empty, and parent fields are not automatically inherited. Cyclic
or missing parent references show a warning instead of an invented tree. Arrays
and dimension bounds retain exact decimal text; unknown cardinality does not
mean variable length. The screen does not decode any live attribute bytes.
The limited demo deliberately supplies useful class metadata but no referenced
datatypes or MOM: only the relevant panels show partial/unavailable notices.
Both screens preserve a missing selection after clear and disclose bounded
history loss. Their browser tests also use the unchanged Java fixture.

Recording follows the same three-layer split. `capture.py` is a dependency-free,
bounded journal owned by the store lock. It freezes the initial state, every
committed batch, metadata changes, clear boundaries and periodic clock
checkpoints. It stores full inspectors as evidence; replay must not guess their
previous state from event names. New arrivals include a whole committed batch
even when ordinary history immediately evicts some of its events. Limits stop
recording, not observation, and the baseline discloses any earlier history loss.
`recording_codec.py` adds a provider-free v2 format: one complete baseline plus
literal set/delete/splice edits, preserving every committed state rather than
sampling. The live `recording_compact` capability negotiates it; otherwise the
frontend uses the original v1 request/file. Both versions remain supported.
Separate encoded-file, reconstructed-state/work and operation bounds stop an
over-budget capture without stopping collection. See [RECORDING.md](RECORDING.md).

`recording_models.py` validates the portable document, frames, requests and
status. `recording_routes.py` bounds uploads before parsing and exposes only
observer recording operations. It does not call a provider, follow captured URLs
or save an imported file. A recording is process-local until the user downloads
it. Other browser clients share the same backend capture; replacing a stopped
capture requires its exact ID and an explicit confirmation in the UI.

`useReplay` owns a local monotonic playback clock and exact frame selection.
The same workspaces read the selected recorded state. A permanent replay banner
distinguishes it from live data; live subscriptions stop and Actions cannot
submit commands, regardless of captured capabilities. Between frames, pending
ages remain as captured, not "now minus an old timestamp." Recorded analytics
uses the frame's captured time and selected window through another validated
request. Seeking must immediately reject stale/future analysis results.
Returning to live fetches fresh state; it does not rewind or pause the RTI.
A dedicated worker reconstructs only the current state and keeps 60 small
metric samples, rather than expanding all v2 states in page memory. Pending
seeks identify the still-displayed state; analytics waits for the exact target.
Closing/replacing replay terminates its worker. Invalid reconstruction never
substitutes live data or enables Actions.

Start on **Recordings**, capture some traffic, stop, download, then replay the
retained capture or reopen its JSON file. Try a clear boundary, speed control,
time slider and exact-state slider. The latter can inspect commits with equal
clock offsets. An unavailable recording backend does not break other tabs.
Recording files include actual federation/context data and may contain secrets
in provider extensions: review them before sharing. They are not the data-free
workspace configurations described in [WORKSPACES.md](WORKSPACES.md).

The Alerts tab follows the same split: `alerting.py` evaluates typed rules over
committed ingress without Pydantic/RTI dependencies; `alert_models.py` and
`alert_routes.py` validate HTTP; `useAlerts` and `AlertsView` consume generated
models. Java can implement the read model without configuration controls.
Neither alert capability grants provider authority or requires MOM.

An alert means "this rule has threshold evidence," not "the RTI is unhealthy."
Idle, a never-seen subject and explicit disconnect are different states. Error
rules see events even when raw history expires immediately; bounded detail
produces count ranges, not a guessed zero. Acknowledgement does not resolve a
condition. Reset starts a baseline while old evidence keeps its rule version;
it cannot unsee an explicit end for the same filter. Process-local rules can be
downloaded/reimported without payloads; replay never fetches current evidence.
See [ALERTS.md](ALERTS.md) before changing this flow or implementing Java.

Saved workspaces are a different kind of persistence. A recording remembers
reported evidence; an alert file remembers rule definitions; a workspace file
remembers how the person wants to inspect evidence. Use **Saved workspaces** in
the header to name/download a configuration. Import checks it and saves it;
Apply explicitly restores the tab, participant, filters, exact chart choices
and window. It does not replay RTI calls or bring captured observations along.
Filter text can still identify a subject, so review it before sharing.

`workspace_models.py` defines the named configuration models. OpenAPI generates
the TypeScript types, and `workspace_schema.py` exports defaults/JSON Schema and
the cross-language examples. Build-time compilation produces offline browser
validators; `codec.ts` checks inputs, sizes and uniqueness. `WorkspaceProvider`
owns the settings and screens use typed hooks. Keep form drafts and backend
authority out of that owner. The optional normalization endpoints are useful
for other clients, but React does not depend on them, so replacing Python with
Java does not require a browser-storage implementation in Java.

If a work RTI lacks MOM, a saved MOM selection stays dormant while Events still
works. If Alice or a particular object/report is not present, the UI keeps that
choice and an empty/unavailable result instead of borrowing Bob's values.
Global filters and each federate's event/declaration/time preferences remain
separate: switch Alice → Bob → Alice and Alice's own settings return. Storage errors keep the page
usable, and an external-tab change or invalid old copy requires explicit
reload/reset. Browser storage is not a durable backup; download the profile.
See [WORKSPACES.md](WORKSPACES.md) for exact limits and recovery steps.

The Actions tab talks to `useCommands` and `ObserverClient`, which send a named
command request to FastAPI. `command_models.py` validates the action and its
parameters. `commands.py` retains its ID/result and queues it on the target's
owner thread. `session_commands.py` owns the common lifecycle policy. The
provider-specific driver in `umbra-rti-observer-umbra/commands.py` translates
the actual Umbra API and pumps evoked callbacks on that same owner thread.
React never receives an RTI object or chooses a provider method to call.

Accepted and succeeded are different states. After acceptance the browser polls
backend history; navigating away or refreshing does not cancel the command.
History survives for the backend process, independently of event retention.
If a running call times out, its effect is unknown and the target stays busy.
Never display a timeout as proof that the RTI did nothing. A late result is
shown separately. Remaining command and investigation work is tracked in
[FEATURE-BACKLOG.md](FEATURE-BACKLOG.md).

### Recovering an uncertain owned session

An error after joining might mean "the join happened but reporting failed."
Sending join again is not a safe diagnosis. The Actions screen therefore keeps
normal lifecycle actions disabled when control state is unknown.

If the adapter offers it, use this sequence:

1. Choose **Inspect session**. It reads the owned driver on its existing owner
   thread and displays connection/membership facts. This does not restore control.
2. Review those facts and the original failed/timeout record. Unreported values
   remain unknown; the ordinary federation roster is not proof about this owner.
3. Confirm **Reconcile session**. The backend makes a fresh inspection, verifies
   a healthy callback pump for connected recovery, and reads again. Only complete
   current facts within the session's federation allowlist restore appropriate
   actions. An unresolved query can return successfully without enabling controls.

No step retries the failed operation, constructs a substitute driver, resigns a
peer or clears its history. A still-running timed-out call blocks inspection too.
Once it returns, a late result remains attached to the original timeout record.
If no owned driver survived construction, or the RTI has no suitable inspection
query, the application/operator must resolve it; the frontend cannot manufacture
this capability. Missing recovery does not disable ordinary observations or
known-state lifecycle operations.

Each modern command copies the selected target's `version` into
`expected_version`. This identifies an owned dispatcher registration and revision,
not the observer notebook, reporter session or participant join. Stale expectations
do nothing at the provider. Changing target revision invalidates confirmation;
review again instead of silently applying an old intent to a new state. Older
backends omitting versions still receive their original lifecycle request shape.

The named Pydantic types are `CommandTargetVersion`, `SessionStateObservation`
and `SessionInspectionResult`. They generate the frontend DTOs. The generic
controller uses an optional typed driver protocol; the Umbra-specific package
accepts a server-owned inspector that knows its binding's real query shape.
That inspector must query its actual owned ambassador, not echo the controller's
cache or search MOM for a matching name. Python/Java implementations share these
meanings without sharing provider objects.

Before changing the frontend or backend contract:

1. Decide whether the change is additive or breaking.
2. Keep existing field meanings stable.
3. Add a contract or integration test.
4. Update [CONTRACT.md](CONTRACT.md).
5. Update capability declarations if the feature surface changed.
6. Verify the dashboard with the demo backend and an intentionally limited
   capability manifest.
7. Refresh the generated TypeScript API snapshot and run the React typecheck.
8. If the change is breaking, create a new contract version instead of
   silently changing the old one.

## 9. Checking a replacement backend

Keep a participant's label separate from the key used by React, filters and
saved settings. Two rows named “Worker” can be different joins, reporting
sessions, namespaces or federations. A renamed row can still be the same join.
The backend's named `perspective` carries those facts; `participants.ts` shares
the exact event-key codec, and a common Python/TypeScript corpus tests Unicode,
opaque sessions, aliases and unsafe numeric labels. Never use a display name
as an identified participant's React key or saved-preference owner.

For example, Worker/join-1 asks for time request `r1`; Worker/join-2 reporting
a grant for `r1` does not complete join-1's request. Their objects, comparisons,
declarations and timeline stay separate too. Explicit object federation context
is another independent dimension; missing context remains a labelled legacy
bucket, not a guessed domain. MOM reports and topology endpoints do not fill
missing ordinary actor identities. Without the new codec an older backend
still has a useful, explicitly name-only view, not fabricated join IDs.

If an adapter knows its own reporting-session lifetime, stamp that session
consistently at the root of its ordinary events too. This is reporter metadata,
not a participant membership ID. It keeps objects, time and declarations in one
legacy reporting view without inventing RTI join evidence. Both synthetic
playtest profiles do this; a work RTI does not need MOM to do the same.

The federate list supports ID/reporter/name search and bounded paging. A saved
exact selection remains dormant after clear/rejoin instead of attaching to
the reused name. Files therefore include chosen identifying keys even when
the user did not type those IDs; review configuration before sharing it.

Before wiring a Java or vendor-specific service into the frontend, use the
[HTTP conformance walkthrough](conformance/README.md). It validates named wire
models, capability discovery, cursor/retention rules and optional resources
without sending commands. Limited, partial and empty services can pass; a
service claiming an available feature but returning an invalid resource cannot.

The frontend discovers REST, WebSocket and SSE paths from the contract. An HTTP-only
service advertises both streams as unsupported and the browser polls snapshots
instead; it does not need to imitate Python or supply MOM. The
[Java fixture](../umbra-rti-observer-java-fixture/README.md) demonstrates this
with the same production React build. It is a static test double, not a live
Java RTI connection. Real session actions and streaming behavior require their
own integration tests beyond a passing HTTP-read report.
