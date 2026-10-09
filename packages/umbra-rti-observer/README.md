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

The package has no runtime dependencies for its core. The HTTP dashboard is an
optional FastAPI/WebSocket layer:

```powershell
python -m pip install -e "packages/umbra-rti-observer[web]"
python -m umbra_rti_observer --demo
```

Open <http://127.0.0.1:8765/> after starting the server. The demo emits only
generic federation, object, interaction, and callback events so the dashboard
can be smoke-tested without a provider or a domain-specific scenario.

## Provider integration

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
GET  /api/inspectors/{objects|interactions|federates}
GET  /api/events
POST /api/events
PUT  /api/context
POST /api/control/status
DELETE /api/events
GET  /api/events/stream
WS   /ws/events
```

`GET /api/events?after=42&limit=500` returns `events`, `next_after`,
`has_more`, and retention fields. Optional comma-separated `event_type`,
`family`, and `source` filters keep the contract generic while avoiding a
provider-specific query language. `GET /api/state?include_events=false` is
available for clients that only need the derived inspectors and metrics.

The event contract uses explicit fields such as `event_type`, `source`,
`federate`, `object_key`, `interaction_key`, `attributes`, `parameters`, and
`details`. Unknown fields are retained so provider adapters can add diagnostics
without changing the dashboard.

Provider adapters may implement `ObserverAdapter.start(sink)` and
`ObserverAdapter.stop()`, then attach themselves with
`ObserverStore.attach_adapter(adapter)`. The generic package never needs to
know whether an adapter uses callbacks, polling, a vendor session object, or a
transport-specific event stream. Batch ingestion is atomic, callbacks are
thread-safe, repeated adapter attachment is idempotent, and `ObserverStore.close()`
stops all attached adapters on shutdown.
