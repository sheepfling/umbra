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

The frontend is the browser dashboard:

```text
packages/umbra-rti-observer/src/umbra_rti_observer/static/dashboard.html
```

It contains the visual layout, CSS, and browser JavaScript. Its job is to:

1. Discover which observer contract the server supports.
2. Request a state snapshot.
3. Listen for new events.
4. Render generic metrics, participants, objects, interactions, and event
   history.

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
4. Serve the static dashboard file.

The backend is not required to be Python. The Python service is simply the
first implementation.

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

The browser's `ObserverTransport` handles URLs, contract discovery, snapshot
loading, WebSocket connection, reconnect state, and stream messages. The
rendering code consumes the returned contract fields and does not care which
language produced them.

## 3. The contract in plain language

The contract has four important rules.

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

### Rule 2: events are plain JSON

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

### Rule 3: snapshots describe current state

`GET /api/state` returns the current dashboard state. It includes:

```text
status
context
live_metrics
retention
adapters
federate_roster
inspectors.objects
inspectors.interactions
normalized_events (when include_events=true)
```

The dashboard uses this snapshot to paint the page. It does not reconstruct
live object state by replaying provider callbacks.

### Rule 4: streams use envelopes

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

Keep the dashboard file unchanged. Implement the contract in the Java server:

1. Serve the static `dashboard.html`.
2. Implement `GET /api/contract`.
3. Implement `GET /api/state`.
4. Implement `GET /api/events` with cursor and gap metadata.
5. Implement WebSocket or SSE streaming.
6. Translate Java RTI callback values into normalized JSON events.
7. Preserve the contract version and field meanings.

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

### A Java backend works at `/api/state` but not in the browser

Check all of the following:

- `/api/contract` returns the expected version.
- State includes `contract_version` and the required snapshot fields.
- WebSocket/SSE messages use `type` envelopes.
- Cross-origin requests have the required CORS and WebSocket headers.
- Event sequences are numeric and monotonic.

## 8. Safe change checklist

Before changing the frontend or backend contract:

1. Decide whether the change is additive or breaking.
2. Keep existing field meanings stable.
3. Add a contract or integration test.
4. Update [CONTRACT.md](CONTRACT.md).
5. Verify the dashboard with the demo backend.
6. If the change is breaking, create a new contract version instead of
   silently changing the old one.
