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

The React workspace now has eight views: Federation (traffic and composition),
Federates (one participant's attributed observations), Objects (search, values,
comparisons and numeric attribute charts), Logical time (timestamps
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
Clearing history clears snapshots too; restarting the backend resets the demo.

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

Pause view freezes the local screen only. Data collection continues; command
submission is disabled until the view resumes. The separate `commands`
capability enables only explicitly registered control sessions. Available MOM
or time observations do not imply permission to send commands.

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

Before wiring a Java or vendor-specific service into the frontend, use the
[HTTP conformance walkthrough](conformance/README.md). It validates named wire
models, capability discovery, cursor/retention rules and optional resources
without sending commands. Limited, partial and empty services can pass; a
service claiming an available feature but returning an invalid resource cannot.

The frontend discovers REST and WebSocket paths from the contract. An HTTP-only
service advertises WebSocket as unsupported and the browser polls snapshots
instead; it does not need to imitate Python or supply MOM. The
[Java fixture](../umbra-rti-observer-java-fixture/README.md) demonstrates this
with the same production React build. It is a static test double, not a live
Java RTI connection. Real session actions and streaming behavior require their
own integration tests beyond a passing HTTP-read report.
