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

## Backend replacement rules

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
