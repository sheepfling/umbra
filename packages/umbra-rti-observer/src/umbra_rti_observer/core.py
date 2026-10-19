"""Thread-safe, provider-neutral event store used by the observer dashboard.

The core intentionally knows only about a small normalized event vocabulary.
It does not import HLA bindings or infer domain semantics from class names.
Provider adapters may retain arbitrary extra fields in each event.
"""

from __future__ import annotations

import threading
import time
from collections import Counter, deque
from collections.abc import Callable, Iterable, Mapping
from typing import Protocol

from .capabilities import (
    OBSERVER_CAPABILITIES,
    merge_capabilities,
    normalize_capabilities,
)
from .contract import OBSERVER_CONTRACT_VERSION
from .models import (
    AdapterStatus,
    CapabilityRecord,
    EventPage,
    EventRecord,
    FederateInspection,
    InteractionInspection,
    JsonObject,
    JsonValue,
    LiveMetrics,
    ObjectInspection,
    Retention,
    to_json_object,
    to_json_value,
)
from .object_views import ObjectViews
from .time_models import time_data
from .time_views import TimeViews

OBSERVER_SCHEMA_VERSION = "umbra-runtime-observer-v1"

EVENT_TYPES = (
    "runtime.started",
    "runtime.stopped",
    "runtime.error",
    "federate.joined",
    "federate.resigned",
    "federate.updated",
    "object.discovered",
    "object.updated",
    "object.removed",
    "interaction.received",
    "callback.invoked",
    "time.observed",
    "service.completed",
    "service.failed",
    "session.connected",
    "session.disconnected",
    "command.accepted",
    "command.running",
    "command.succeeded",
    "command.failed",
    "command.unavailable",
    "command.late_result",
    "event.raw",
)

EventSink = Callable[[Mapping[str, object]], object]


class EventSource(Protocol):
    """Minimal source contract an RTI/provider adapter may implement."""

    def subscribe(self, sink: EventSink) -> None:
        """Send normalized events to ``sink`` until unsubscribed."""

    def unsubscribe(self, sink: EventSink) -> None:
        """Stop sending events to ``sink``."""


class ObserverAdapter(Protocol):
    """Lifecycle contract for a provider-specific adapter package.

    The adapter owns all provider imports and callback/session differences. The
    observer only receives normalized event mappings through ``sink``. Adapters
    may additionally expose a ``capabilities`` mapping; older adapters remain
    valid and are reported as unknown for provider-specific features.
    """

    def start(self, sink: EventSink) -> None:
        """Start forwarding provider events to ``sink``."""

    def stop(self) -> None:
        """Stop forwarding provider events and release provider resources."""


class CapabilitySource(Protocol):
    """Optional adapter surface for provider capability declarations."""

    @property
    def capabilities(self) -> Mapping[str, object]:
        """Describe provider features available to the observer."""
        ...

def _jsonable(value: object) -> JsonValue:
    return to_json_value(value)


def _text(value: object) -> str | None:
    if value is None:
        return None
    text = str(value).strip()
    return text or None


def _first_text(event: Mapping[str, object], *names: str) -> str | None:
    for name in names:
        value = _text(event.get(name))
        if value:
            return value
    return None


def _event_family(event: Mapping[str, object]) -> str:
    # Family is always explicit. In particular, this does not classify an
    # event from an HLA class name or a provider-specific naming convention.
    return _first_text(event, "family") or "generic"


def _source_name(event: Mapping[str, object]) -> str:
    return _first_text(event, "source", "connection") or "unknown"


def _event_type(event: Mapping[str, object]) -> str:
    return _first_text(event, "event_type", "kind") or "event.raw"


def _object_key(event: Mapping[str, object]) -> str | None:
    return _first_text(event, "object_key", "object_handle", "object_name")


def _interaction_key(event: Mapping[str, object]) -> str | None:
    return _first_text(event, "interaction_key", "interaction_handle", "interaction_class")


def _federate_name(event: Mapping[str, object]) -> str | None:
    value = event.get("federate", event.get("participant"))
    if isinstance(value, Mapping):
        return _first_text(value, "name", "id", "handle")
    return _text(value) or _first_text(event, "federate_name", "participant_name")


def _copy_mapping(value: object) -> JsonObject:
    if not isinstance(value, Mapping):
        return {}
    return {str(key): to_json_value(item) for key, item in value.items()}


def _normalise_event(event: Mapping[str, object], sequence: int) -> EventRecord:
    if not isinstance(event, Mapping):
        raise TypeError("observer events must be mappings")
    time_data(event)  # Validate explicit observations before any batch mutation.
    normalized = _copy_mapping(event)
    event_type = _event_type(normalized)
    if len(event_type) > 256:
        raise ValueError("event_type must be at most 256 characters")
    return EventRecord(
        sequence=sequence,
        event_type=event_type,
        family=_event_family(normalized),
        source=_source_name(normalized),
        observed_at=time.time(),
        extensions=normalized,
    )


def _status_for_event(event_type: str) -> str:
    if event_type == "federate.resigned":
        return "resigned"
    if event_type == "runtime.error":
        return "error"
    return "active"


def _derive_objects(events: Iterable[Mapping[str, object]]) -> list[ObjectInspection]:
    rows: dict[str, ObjectInspection] = {}
    for event in events:
        key = _object_key(event)
        if not key:
            continue
        event_type = _event_type(event)
        if not (
            event_type.startswith("object.")
            or "object_key" in event
            or "object_handle" in event
            or "object_name" in event
        ):
            continue
        row = rows.setdefault(
            key,
            ObjectInspection(
                object_key=key,
                object_name=_first_text(event, "object_name") or key,
                object_handle=_first_text(event, "object_handle"),
                class_name=_first_text(event, "class_name"),
                class_handle=_first_text(event, "class_handle"),
                family=_event_family(event),
                last_seen=_timestamp(event.get("observed_at")),
            ),
        )
        row.object_name = _first_text(event, "object_name") or row.object_name
        row.object_handle = _first_text(event, "object_handle") or row.object_handle
        row.class_name = _first_text(event, "class_name") or row.class_name
        row.class_handle = _first_text(event, "class_handle") or row.class_handle
        row.family = _event_family(event)
        attributes = event.get("attributes")
        if isinstance(attributes, Mapping):
            row.attributes.update(_copy_mapping(attributes))
        if event_type == "object.discovered":
            row.discovery_count += 1
        elif event_type == "object.removed":
            row.status = "removed"
        else:
            row.update_count += 1
        source = _source_name(event)
        if source not in row.sources:
            row.sources.append(source)
        row.last_seen = _timestamp(event.get("observed_at"))
    return sorted(rows.values(), key=lambda row: row.object_key)


def _derive_interactions(events: Iterable[Mapping[str, object]]) -> list[InteractionInspection]:
    rows: list[InteractionInspection] = []
    for event in events:
        key = _interaction_key(event)
        event_type = _event_type(event)
        if not key or not (
            event_type == "interaction.received"
            or "interaction_key" in event
            or "interaction_handle" in event
            or "interaction_class" in event
        ):
            continue
        rows.append(
            InteractionInspection(
                sequence=_sequence(event.get("sequence")),
                interaction_key=key,
                interaction_class=_first_text(event, "interaction_class") or key,
                class_handle=_first_text(event, "class_handle", "interaction_handle"),
                family=_event_family(event),
                source=_source_name(event),
                federate=_federate_name(event),
                parameters=_copy_mapping(event.get("parameters")),
                tag=to_json_value(event.get("tag")),
                observed_at=_timestamp(event.get("observed_at")) or 0.0,
            )
        )
    return rows[-250:]


def _derive_federates(events: Iterable[Mapping[str, object]]) -> list[FederateInspection]:
    rows: dict[str, FederateInspection] = {}
    for event in events:
        name = _federate_name(event)
        event_type = _event_type(event)
        if not name or not (event_type.startswith("federate.") or "federate" in event or "participant" in event):
            continue
        row = rows.setdefault(
            name,
            FederateInspection(
                federate=name,
                role=_first_text(event, "role"),
                handle=_first_text(event, "federate_handle", "participant_handle"),
                status="active",
                source=_source_name(event),
                last_seen=_timestamp(event.get("observed_at")),
            ),
        )
        row.role = _first_text(event, "role") or row.role
        row.handle = _first_text(event, "federate_handle", "participant_handle") or row.handle
        row.source = _source_name(event)
        row.status = _status_for_event(event_type)
        row.last_seen = _timestamp(event.get("observed_at"))
    return sorted(rows.values(), key=lambda row: row.federate)


def _derive_metrics(events: Iterable[EventRecord]) -> LiveMetrics:
    events = list(events)
    types = Counter(_event_type(event) for event in events)
    families = Counter(_event_family(event) for event in events)
    sources = Counter(_source_name(event) for event in events)
    timestamps = [
        event.observed_at for event in events
    ]
    elapsed = max(timestamps) - min(timestamps) if len(timestamps) > 1 else 0.0
    error_count = sum(
        count
        for event_type, count in types.items()
        if event_type.endswith((".error", ".failed"))
    )
    return LiveMetrics(
        event_count=len(events),
        event_rate_per_second=round(len(events) / elapsed, 3) if elapsed > 0 else 0.0,
        error_count=error_count,
        event_types=dict(sorted(types.items())),
        families=dict(sorted(families.items())),
        sources=dict(sorted(sources.items())),
        first_sequence=events[0].sequence if events else 0,
        last_sequence=events[-1].sequence if events else 0,
    )


def _sequence(value: object) -> int:
    if isinstance(value, bool) or not isinstance(value, int):
        return 0
    return value


def _timestamp(value: object) -> float | None:
    return float(value) if isinstance(value, (int, float)) and not isinstance(value, bool) else None


def build_event_schema() -> JsonObject:
    """Return the stable JSON schema advertised by the HTTP service."""

    return to_json_object({
        "$schema": "https://json-schema.org/draft/2020-12/schema",
        "$id": "https://umbra.invalid/observer/event-schema.json",
        "title": "Umbra Runtime Observer Event",
        "contract_version": OBSERVER_CONTRACT_VERSION,
        "schema_version": OBSERVER_SCHEMA_VERSION,
        "description": "Provider-neutral event envelope; unknown fields are retained.",
        "type": "object",
        "required": ["event_type"],
        "properties": {
            "sequence": {"type": "integer", "minimum": 1},
            "event_type": {"type": "string"},
            "family": {"type": "string", "description": "Explicit caller-defined grouping."},
            "source": {"type": "string"},
            "observed_at": {"type": "number"},
            "connection": {"type": "string"},
            "federation": {"type": "string"},
            "federate": {"type": ["string", "object"]},
            "object_key": {"type": "string"},
            "interaction_key": {"type": "string"},
            "attributes": {"type": "object"},
            "parameters": {"type": "object"},
            "details": {"type": "object"},
        },
        "event_types": list(EVENT_TYPES),
    })


class CallbackEventSource:
    """Small bridge for adapters that already receive provider callbacks."""

    def __init__(self) -> None:
        self._lock = threading.Lock()
        self._sinks: list[EventSink] = []

    def subscribe(self, sink: EventSink) -> None:
        with self._lock:
            if sink not in self._sinks:
                self._sinks.append(sink)

    def unsubscribe(self, sink: EventSink) -> None:
        with self._lock:
            if sink in self._sinks:
                self._sinks.remove(sink)

    def emit(self, event: Mapping[str, object]) -> None:
        with self._lock:
            sinks = tuple(self._sinks)
        for sink in sinks:
            try:
                sink(event)
            except Exception:  # noqa: BLE001,S112 - one diagnostic sink must not block others
                # A diagnostic consumer must not prevent another consumer from
                # receiving the same provider callback. Provider adapters are
                # still responsible for reporting their own sink failures.
                continue


class ObserverStore:
    """Bounded, thread-safe state store for normalized runtime events.

    Sequence numbers are monotonic for the lifetime of a store. Once the
    bounded history is full, older rows are evicted and the retention metadata
    tells clients when their cursor has fallen behind the retained window.
    """

    def __init__(
        self,
        *,
        max_events: int = 2_000,
        max_batch_size: int = 10_000,
        max_time_requests: int = 250,
        capabilities: Mapping[str, object] | None = None,
    ) -> None:
        if max_events < 1:
            raise ValueError("max_events must be at least 1")
        if max_batch_size < 1:
            raise ValueError("max_batch_size must be at least 1")
        if isinstance(max_time_requests, bool) or not isinstance(max_time_requests, int) or max_time_requests < 1:
            raise ValueError("max_time_requests must be a positive integer")
        self._events: deque[EventRecord] = deque(maxlen=max_events)
        self._max_events = max_events
        self._max_batch_size = max_batch_size
        self._next_sequence = 1
        self._dropped_events = 0
        self._cleared_events = 0
        self._status = "idle"
        self._context: JsonObject = {}
        # These indexes are intentionally independent from the bounded event
        # deque. A long-running observer must not forget a live object or
        # participant merely because its older event history was evicted.
        self._objects: dict[str, ObjectInspection] = {}
        self._object_views = ObjectViews(max_events)
        self._time_views = TimeViews(max_time_requests)
        self._federates: dict[str, FederateInspection] = {}
        self._interactions: deque[InteractionInspection] = deque(maxlen=250)
        self._adapters: dict[int, tuple[ObserverAdapter, str, str]] = {}
        self._manual_capabilities: dict[str, CapabilityRecord] = normalize_capabilities(
            capabilities,
            owner="application",
            source="store",
        )
        self._adapter_capabilities: dict[int, dict[str, CapabilityRecord]] = {}
        self._condition = threading.Condition()

    def set_context(self, **values: object) -> JsonObject:
        with self._condition:
            self._context.update({str(key): _jsonable(value) for key, value in values.items() if value is not None})
            return dict(self._context)

    def set_status(self, status: str, **context: object) -> JsonObject:
        status_text = _text(status)
        if not status_text:
            raise ValueError("status must not be empty")
        with self._condition:
            self._status = status_text
            self._context.update({str(key): _jsonable(value) for key, value in context.items() if value is not None})
            self._condition.notify_all()
            return self._state_locked(include_events=False)

    def set_capabilities(
        self,
        capabilities: Mapping[str, object],
        *,
        owner: str = "application",
        source: str | None = None,
    ) -> dict[str, JsonObject]:
        """Add or replace application/backend capability observations."""

        normalized = normalize_capabilities(capabilities, owner=owner, source=source)
        with self._condition:
            self._manual_capabilities.update(normalized)
            self._condition.notify_all()
            return self._capabilities_locked()

    def capabilities(self) -> dict[str, JsonObject]:
        """Return the effective capability manifest for this observer."""

        with self._condition:
            return self._capabilities_locked()

    def publish(self, event: Mapping[str, object]) -> JsonObject:
        with self._condition:
            normalized = _normalise_event(event, self._next_sequence)
            self._append_locked([normalized])
            self._condition.notify_all()
            return normalized.to_json()

    def publish_many(self, events: Iterable[Mapping[str, object]]) -> int:
        rows = list(events)
        if len(rows) > self._max_batch_size:
            raise ValueError(f"event batch exceeds max_batch_size ({self._max_batch_size})")
        if not rows:
            return 0
        with self._condition:
            # Normalize the complete batch before mutating the store. A bad
            # row therefore cannot leave a half-ingested batch behind.
            normalized = [
                _normalise_event(event, self._next_sequence + offset)
                for offset, event in enumerate(rows)
            ]
            self._append_locked(normalized)
            self._condition.notify_all()
            return len(normalized)

    def attach(self, source: EventSource) -> None:
        source.subscribe(self.publish)

    def detach(self, source: EventSource) -> None:
        source.unsubscribe(self.publish)

    def attach_adapter(self, adapter: ObserverAdapter) -> None:
        """Start a provider-specific adapter against this store."""

        key = id(adapter)
        name = _text(getattr(adapter, "name", None)) or type(adapter).__name__
        adapter_capabilities = normalize_capabilities(
            getattr(adapter, "capabilities", None),
            owner="provider",
            source=name,
        )
        with self._condition:
            if key in self._adapters:
                # Attaching the same object twice is harmless and avoids two
                # provider sessions being opened by a repeated setup call.
                return
            self._adapters[key] = (adapter, name, "starting")
            self._adapter_capabilities[key] = adapter_capabilities
        try:
            adapter.start(self.publish)
        except Exception as error:
            with self._condition:
                self._adapters.pop(key, None)
                self._adapter_capabilities.pop(key, None)
            try:
                adapter.stop()
            except Exception:  # noqa: BLE001,S110 - cleanup continues across adapters
                pass
            self.publish(
                {
                    "event_type": "runtime.error",
                    "source": "observer",
                    "details": {
                        "operation": "attach_adapter",
                        "adapter": name,
                        "error": str(error),
                    },
                }
            )
            raise
        with self._condition:
            if key in self._adapters:
                self._adapters[key] = (adapter, name, "running")

    def detach_adapter(self, adapter: ObserverAdapter) -> None:
        """Stop a provider-specific adapter previously attached to this store."""

        key = id(adapter)
        try:
            adapter.stop()
        finally:
            with self._condition:
                self._adapters.pop(key, None)
                self._adapter_capabilities.pop(key, None)

    def close(self) -> None:
        """Stop all attached adapters and mark the store stopped.

        Cleanup is best-effort across adapters: every adapter gets a stop call,
        while the first failure is re-raised after the remaining adapters have
        been given a chance to release their resources.
        """

        with self._condition:
            adapters = [entry[0] for entry in self._adapters.values()]
        first_error: Exception | None = None
        for adapter in adapters:
            try:
                self.detach_adapter(adapter)
            except Exception as error:  # noqa: BLE001 - cleanup must attempt every adapter
                first_error = first_error or error
        self.set_status("stopped")
        if first_error is not None:
            raise first_error

    def events(self, *, after_sequence: int = 0, limit: int | None = None) -> list[JsonObject]:
        with self._condition:
            return self._events_locked(after_sequence=after_sequence, limit=limit)

    def event_page(
        self,
        *,
        after_sequence: int = 0,
        limit: int = 500,
        event_types: Iterable[str] | None = None,
        families: Iterable[str] | None = None,
        sources: Iterable[str] | None = None,
    ) -> JsonObject:
        """Return a cursor page plus retention metadata for API consumers."""

        if after_sequence < 0:
            raise ValueError("after_sequence must be non-negative")
        if limit < 1:
            raise ValueError("limit must be at least 1")
        if limit > 10_000:
            raise ValueError("limit must be at most 10000")
        type_filter = {str(value) for value in event_types} if event_types else None
        family_filter = {str(value) for value in families} if families else None
        source_filter = {str(value) for value in sources} if sources else None
        with self._condition:
            retained = list(self._events)
            candidates = [
                event
                for event in retained
                if event.sequence > after_sequence
                and (type_filter is None or event.event_type in type_filter)
                and (family_filter is None or event.family in family_filter)
                and (source_filter is None or event.source in source_filter)
            ]
            rows = tuple(candidates[:limit])
            first_sequence = retained[0].sequence if retained else 0
            last_sequence = retained[-1].sequence if retained else 0
            gap = bool(first_sequence and after_sequence < first_sequence - 1)
            next_after = rows[-1].sequence if rows else after_sequence
            if gap and not rows:
                next_after = first_sequence - 1
            return EventPage(
                events=rows,
                next_after=next_after,
                has_more=len(candidates) > len(rows),
                gap=gap,
                first_retained_sequence=first_sequence,
                last_sequence=last_sequence,
                retained_events=len(retained),
                dropped_events=self._dropped_events,
                cleared_events=self._cleared_events,
            ).to_json(OBSERVER_CONTRACT_VERSION)

    def wait_for_events(self, *, after_sequence: int = 0, timeout: float = 15.0) -> list[JsonObject]:
        deadline = time.monotonic() + max(timeout, 0.0)
        with self._condition:
            while not any(event.sequence > after_sequence for event in self._events):
                remaining = deadline - time.monotonic()
                if remaining <= 0:
                    break
                self._condition.wait(remaining)
            return self._events_locked(after_sequence=after_sequence, limit=None)

    def clear(self) -> None:
        with self._condition:
            self._cleared_events += len(self._events)
            self._events.clear()
            self._objects.clear()
            self._object_views.clear()
            self._time_views.clear()
            self._federates.clear()
            self._interactions.clear()
            self._condition.notify_all()

    def state(self, *, include_events: bool = True) -> JsonObject:
        with self._condition:
            return self._state_locked(include_events=include_events)

    def _events_locked(self, *, after_sequence: int, limit: int | None) -> list[JsonObject]:
        rows = [event.to_json() for event in self._events if event.sequence > max(after_sequence, 0)]
        if limit is not None:
            rows = rows[: max(limit, 0)]
        return rows

    def _append_locked(self, normalized: list[EventRecord]) -> None:
        for event in normalized:
            if len(self._events) == self._max_events:
                self._dropped_events += 1
            self._events.append(event)
            self._next_sequence += 1
            self._update_indexes_locked(event)
            self._context.update(
                {
                    key: value
                    for key in ("connection", "federation", "source")
                    if (value := event.get(key)) is not None
                }
            )
            event_type = event.event_type
            if event_type == "runtime.started":
                self._status = "running"
            elif event_type == "runtime.stopped":
                self._status = "stopped"
            elif event_type == "runtime.error":
                self._status = "error"
            elif self._status == "idle":
                self._status = "running"

    def _update_indexes_locked(self, event: EventRecord) -> None:
        self._time_views.record(event, _federate_name(event), _first_text(event, "federation"))
        objects = _derive_objects([event])
        self._object_views.record(event, objects, _federate_name(event))
        for incoming in objects:
            key = incoming.object_key
            current = self._objects.get(key)
            if current is None or (current.status == "removed" and event.event_type == "object.discovered"):
                self._objects[key] = incoming
                continue
            object_name = _first_text(event, "object_name")
            object_handle = _first_text(event, "object_handle")
            class_name = _first_text(event, "class_name")
            class_handle = _first_text(event, "class_handle")
            current.object_name = object_name or current.object_name
            current.object_handle = object_handle or current.object_handle
            current.class_name = class_name or current.class_name
            current.class_handle = class_handle or current.class_handle
            current.family = incoming.family
            current.attributes.update(incoming.attributes)
            current.update_count += incoming.update_count
            current.discovery_count += incoming.discovery_count
            if event.event_type == "object.discovered":
                current.status = "active"
            elif incoming.status == "removed":
                current.status = "removed"
            for source in incoming.sources:
                if source not in current.sources:
                    current.sources.append(source)
            current.last_seen = incoming.last_seen

        for incoming in _derive_federates([event]):
            key = incoming.federate
            current = self._federates.get(key)
            if current is None:
                self._federates[key] = incoming
                continue
            current.role = incoming.role or current.role
            current.handle = incoming.handle or current.handle
            current.status = incoming.status
            current.source = incoming.source
            current.last_seen = incoming.last_seen

        self._interactions.extend(_derive_interactions([event]))

    def _retention_locked(self) -> Retention:
        first_sequence = self._events[0].sequence if self._events else 0
        last_sequence = self._events[-1].sequence if self._events else 0
        return Retention(
            max_events=self._max_events,
            retained_events=len(self._events),
            dropped_events=self._dropped_events,
            cleared_events=self._cleared_events,
            first_retained_sequence=first_sequence,
            last_sequence=last_sequence,
            next_sequence=self._next_sequence,
        )

    def _adapters_locked(self) -> list[AdapterStatus]:
        rows: list[AdapterStatus] = []
        for key, (_, name, status) in sorted(self._adapters.items(), key=lambda entry: entry[1][1]):
            capabilities: tuple[str, ...] = ()
            if self._adapter_capabilities.get(key):
                capabilities = tuple(sorted(self._adapter_capabilities[key]))
            rows.append(AdapterStatus(name=name, status=status, capabilities=capabilities))
        return rows

    def _capabilities_locked(self) -> dict[str, JsonObject]:
        effective = merge_capabilities(
            OBSERVER_CAPABILITIES,
            *self._adapter_capabilities.values(),
        )
        # Store/application declarations are authoritative. This is what lets
        # an operator disable a provider feature without changing the adapter
        # or making the frontend guess whether an empty result is intentional.
        for name, manifest in self._manual_capabilities.items():
            effective[name] = merge_capabilities({name: manifest})[name]
        return {
            name: capability.to_json()
            for name, capability in sorted(effective.items())
        }

    def _state_locked(self, *, include_events: bool) -> JsonObject:
        events = tuple(self._events)
        objects = sorted(
            self._objects.values(),
            key=lambda row: row.object_key,
        )
        interactions = tuple(self._interactions)
        federates = sorted(
            self._federates.values(),
            key=lambda row: row.federate,
        )
        metrics = _derive_metrics(events)
        metrics = LiveMetrics(
            **{
                **metrics.to_json(),
                "object_count": len(objects),
                "interaction_count": len(interactions),
                "federate_count": len(federates),
            }
        )
        state = to_json_object({
            "contract_version": OBSERVER_CONTRACT_VERSION,
            "schema_version": OBSERVER_SCHEMA_VERSION,
            "status": self._status,
            "context": dict(self._context),
            "live_metrics": metrics.to_json(),
            "retention": self._retention_locked().to_json(),
            "adapters": [row.to_json() for row in self._adapters_locked()],
            "capabilities": self._capabilities_locked(),
            "federate_roster": [row.to_json() for row in federates],
            "inspectors": {
                "objects": [row.to_json() for row in objects],
                "interactions": [row.to_json() for row in interactions],
                "object_observations": self._object_views.to_json(self._retention_locked()),
                "time": self._time_views.to_json(),
            },
        })
        if include_events:
            state["normalized_events"] = [event.to_json() for event in events]
        return to_json_object(state)


__all__ = [
    "EVENT_TYPES",
    "OBSERVER_CAPABILITIES",
    "OBSERVER_SCHEMA_VERSION",
    "CallbackEventSource",
    "CapabilitySource",
    "EventSink",
    "EventSource",
    "ObserverAdapter",
    "ObserverStore",
    "build_event_schema",
]
