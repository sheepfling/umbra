# Umbra Runtime Observer

`umbra-rti-observer` is a standalone provider-neutral runtime event store and
dashboard package. It deliberately does not import an RTI provider, parse a
FOM, connect to an RTI, or assign meaning from provider or domain class names. An application or
provider-specific adapter owns that boundary and publishes normalized JSON
events to the observer.

The dashboard consumes the versioned, language-neutral wire contract documented
in [CONTRACT.md](CONTRACT.md). The Python FastAPI service is one backend
implementation; a Java federate service can replace it without changing the
frontend if it implements that contract.

For a newcomer-friendly explanation of the layers and data flow, see
[JUNIOR-GUIDE.md](JUNIOR-GUIDE.md).

The [feature backlog](FEATURE-BACKLOG.md) tracks delivered federation/federate
workspaces and the next command, object, time, and portability increments.

The Objects workspace now separates global values from persistent per-federate
attributed snapshots, with object/class search, retained comparisons, byte
representation labels and selectable numeric charts. This optional model does
not require MOM, analytics, or an Umbra-specific RTI adapter.

Persistent time inspection keeps explicit grants, role state and pending
requests beyond event eviction. Request/grant analysis uses scoped IDs and
observer-monotonic durations, not timestamp guesses or remote clock differences.
Grant-only and opaque-time RTIs retain useful views; numeric fields and advanced
features can remain unknown. See CONTRACT.md for the optional model and limits.

The declaration explorer adds optional scoped publication/subscription
snapshots, searchable classes/attributes and explicit reported status. Unknown,
all, selected-empty and interaction-not-applicable attribute sets stay distinct.
It needs no MOM or analytics and never guesses routes from matching classes.

For a live walkthrough, use the [observer showcase](SHOWCASE.md). It includes
rich and limited capability profiles so the same frontend can be demonstrated
against both a mature and a smaller RTI surface.

For a no-RTI playtest, double-click `Start-Observer.cmd` at the repository root,
or run `python tools/observer_playtest.py`. It handles the dashboard build,
private environment, synthetic traffic, controls, and browser for you. Use
`--bundle` to make a ZIP Miguel can run without Node or a repository checkout.
See the [showcase startup guide](SHOWCASE.md#easiest-start-including-first-time-setup)
for prerequisites and stopping the demo.

The package has no runtime dependencies for its core. The HTTP dashboard is an
optional FastAPI/WebSocket layer:

```powershell
python -m pip install -e "packages/umbra-rti-observer[web]"
python -m umbra_rti_observer --demo
```

Open <http://127.0.0.1:8765/> after starting the server. The demo emits only
generic federation, object, interaction, and callback events so the dashboard
can be smoke-tested without a provider or a domain-specific scenario.

The web extra includes FastAPI and Pydantic. The HTTP surface publishes its
OpenAPI document at `/openapi.json`; request bodies and finite responses use
named strict Pydantic models, while SSE and WebSocket messages are validated
before serialization. Provider-specific event fields are kept through an
explicit extension model field internally and retain their existing top-level
wire representation.

The production dashboard is maintained as a separate React/TypeScript package
at `../umbra-rti-observer-ui`. Build it with `pnpm build` and set
`UMBRA_OBSERVER_UI_DIST` to its `dist` directory before starting this service.
The FastAPI root serves that build when present and falls back to the bundled
zero-build dashboard when it is absent. Refresh the UI's generated client types
from the backend with `pnpm generate:api`.

Windowed analytics can be disabled with `--no-analytics`, or with
`create_app(store, enable_analytics=False)`. The optional route is then absent
and advertised as disabled; events and live inspectors continue to work.

The optional Actions workspace uses an explicitly registered command
dispatcher. Try its synthetic lifecycle without an RTI:

```powershell
python -m umbra_rti_observer --showcase --enable-controls
```

Connect, join, resign and disconnect affect only the labelled synthetic
control session. The default service remains read-only. Real applications
register owned sessions using the separate
[Umbra control adapter](../umbra-rti-observer-umbra/README.md); other provider
shapes implement `CommandSession`. The dispatcher and lifecycle controller use
the optional `control` extra (Pydantic); the dependency-free event store does
not import them. See [CONTRACT.md](CONTRACT.md) for command results, deduplication,
timeouts, late effects and process-lifetime history.

## Provider integration

Analytics now reads a named retained-window projection without traversing
unrelated inspectors; recorded analytics negotiates the same lightweight shape
while retaining the original full-frame request for older backends. All source
events, idle roster entries, attribution and retention semantics are preserved.
Scoped object baselines have an independent 5000-record default budget and
visible loss metadata; reobserved scopes start fresh comparisons. Object lists
page in groups of 100 without limiting search. See [SCALE.md](SCALE.md) for
reproducible measurements, the tested snapshot/analytics envelope and open
recording/oversized-state work. These are not hardware-independent guarantees.

The Recordings tab captures a bounded, versioned journal of complete observer
states and committed envelope batches. It preserves existing inspectors, partial
capabilities, context, clear boundaries and explicit history loss. Stop and
download the file, then inspect it using play/speed/time/exact-state controls
across the same workspaces. Replay is visibly separate from live observation,
never follows captured URLs and cannot submit provider commands. Imports and
recorded analytics run through named validated models; no provider is needed.
Limits stop capture, not collection. One capture is shared/retained by the
backend process until restart or explicit replacement; download it to keep it.
Files contain federation data and are not credential-scrubbed. Recording is an
optional backend capability, independent of MOM and analytics.
When the live backend advertises compact recording, the UI negotiates lossless
v2 baseline/edit files with bounded reconstruction; older backends/files keep
the v1 flow. A worker expands only the current state, not the complete timeline.
See [RECORDING.md](RECORDING.md) for the format and safety limits.

MOM reports and the FOM browser are independently optional, read-only screens.
Management member reports retain coverage/provenance without replacing the
ordinary roster; service/error evidence never becomes an observer command.
Provider counters keep numeric and opaque representations distinct, with
exact-series numeric charts and explicit retention loss. FOM search/navigation
uses supplied class/attribute/parameter/datatype/dimension metadata scoped to an
explicit model revision. Missing/cyclic references, unknown ancestry and unknown
field lists stay visible. The generic observer never parses provider FOMs or
decodes runtime bytes. `mom_inspection` and `fom_inspection` are separate
capabilities; the limited playtest has partial FOM metadata and no MOM while
ordinary attributes, time and lifecycle controls keep working.

Ownership and Regions / DDM add independently optional, read-only workspaces.
Ownership separates explicit owner state from acquire/divest tickets, preserving
unresolved handle metadata without guessing a participant. DDM preserves exact
large range bounds, explicit associations and declared/observed routing, with
exact-scope reference navigation and no inferred overlap/delivery. Both retain
bounded evidence beyond event expiry and report loss. Legacy provider flags
keep their meanings; typed inspection uses separate capabilities. No native
ownership or DDM command is implied.

The optional Topology tab draws only explicit adapter-reported relationships.
Participant/join IDs (not names or handles) identify nodes; each edge retains
source/session/federation provenance, declared/observed evidence and reported
lifecycle. Search, filters, keyboard node focus, bounded drawing, paged tables
and coverage notices work without MOM or analytics. See CONTRACT.md before
adding an adapter: matching declarations never establish a route or delivery.

Synchronization and Save / restore add read-only operation timelines, scoped
latest reports, participant failures and retained phase counts. A local
completion never becomes a federation-wide completion. These independently
optional workspaces use explicit operation and join IDs, survive event-history
expiry, disclose index/history loss and work with the unchanged Java fixture.
No collective-management commands are implied or advertised.

Interaction flow pairs explicit transmission and endpoint join IDs, retaining
send/receive provenance and reported order. Missing identities stay counted but
uncorrelated; duplicates are ambiguous. The dynamic chart shows local observer
arrival intervals, never guessed network latency or delivery percentages. Its
bounded evidence index, cumulative counts and loss notices work without MOM or
analytics, including receive-only and independent Java backends.

An adapter can use the zero-dependency core directly:

```python
from umbra_rti_observer import ObserverStore

store = ObserverStore()
store.set_context(
    connection="tcp://localhost:9000",
    federation="ExampleFederation",
    source="umbra-native",
)
store.set_status("running")
store.publish(
    {
        "event_type": "object.updated",
        "source": "umbra-native",
        "object_key": "object-17",
        "object_name": "ExampleObject",
        "class_name": "ExampleClass",
        "attributes": {"position": [1.0, 2.0]},
    }
)
```

The HTTP layer accepts one event or an object containing an `events` array at
`POST /api/events`. Event history is bounded, sequence-numbered, and exposed
with cursor and retention metadata so clients can detect when a slow consumer
has fallen behind. Object, interaction, and participant inspectors are
maintained separately from the event deque, so bounded history does not make a
long-running live view forget active entities. It exposes state, inspectors,
metrics, Server-Sent Events, and WebSocket endpoints:

```text
GET  /api/health
GET  /api/catalog
GET  /api/contract
GET  /api/schema
GET  /api/state
GET  /api/metrics
GET  /api/analytics
GET  /api/commands/catalog
GET  /api/commands
POST /api/commands
GET  /api/commands/{command_id}
GET  /api/inspectors/{objects|interactions|federates}
GET  /api/events
POST /api/events
PUT  /api/context
POST /api/control/status
DELETE /api/events
GET  /api/events/stream
WS   /ws/events
```

Any backend can be checked without provider-specific dependencies:

```powershell
python -m umbra_rti_observer.probe http://127.0.0.1:8765
```

The probe validates the advertised contract, capability states, required
snapshot and metrics fields, and ordered event envelopes. It is useful when a
Java or vendor-specific service replaces the Python backend.

For strict acceptance checks, use the optional `conformance` extra and
`python -m umbra_rti_observer.conformance http://127.0.0.1:8765`.
The [backend interoperability matrix](conformance/README.md) covers nested
models, version/media headers, discovered paths, optional features, retention
and cursors, shared Python/Java fixtures, portable JSON Schema and generated
TypeScript drift checks. It never invokes session commands or an RTI.

`GET /api/events?after=42&limit=500` returns `events`, `next_after`,
`has_more`, and retention fields. Optional comma-separated `event_type`,
`family`, and `source` filters keep the contract generic while avoiding a
provider-specific query language. `GET /api/state?include_events=false` is
available for clients that only need the derived inspectors and metrics.

The event contract uses explicit fields such as `event_type`, `source`,
`federate`, `object_key`, `interaction_key`, `attributes`, `parameters`, and
`details`. Unknown fields are retained so provider adapters can add diagnostics
without changing the dashboard. The contract also advertises capability state
for features such as logical time, MOM, ownership, and regions, so a less
mature RTI can leave only those panels unavailable while the rest remains useful.

Adapters should publish `available`, `partial`, `disabled`, `unsupported`, or
`unknown` capability states rather than pretending unsupported features are
empty data.

Provider adapters may implement `ObserverAdapter.start(sink)` and
`ObserverAdapter.stop()`, then attach themselves with
`ObserverStore.attach_adapter(adapter)`. The generic package never needs to
know whether an adapter uses callbacks, polling, a vendor session object, or a
transport-specific event stream. Batch ingestion is atomic, callbacks are
thread-safe, repeated adapter attachment is idempotent, and `ObserverStore.close()`
stops all attached adapters on shutdown.
