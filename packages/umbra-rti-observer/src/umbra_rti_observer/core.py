"""Thread-safe, provider-neutral event store used by the observer dashboard.

The core intentionally knows only about a small normalized event vocabulary.
It does not import HLA bindings or infer domain semantics from class names.
Provider adapters may retain arbitrary extra fields in each event.
"""

from __future__ import annotations

import json
import threading
import time
from collections import Counter
from collections.abc import Callable, Iterable, Mapping, Sequence
from dataclasses import replace
from typing import Protocol
from uuid import uuid4

from .alerting import (
    NO_COMMAND_WATCH,
    AlertEngine,
    AlertRuleSpec,
    AlertScope,
    CommandWatch,
)
from .attribution import (
    event_participant_metadata,
    participant_event_ended,
    participant_event_metadata,
    participant_event_perspective,
)
from .capabilities import (
    OBSERVER_CAPABILITIES,
    merge_capabilities,
    normalize_capabilities,
)
from .capture import (
    CaptureBuffer,
    CaptureChange,
    CaptureConflict,
    CaptureInfo,
    CompactCaptureBuffer,
)
from .contract import OBSERVER_CONTRACT_VERSION, build_contract_document
from .coordination import CoordinationViews, coordination_data
from .declarations import DeclarationViews, declaration_data
from .fom import FomViews, fom_data
from .identities import identity_text
from .interaction_flow import InteractionFlowViews, interaction_flow_data
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
    RawObserverEvent,
    Retention,
    to_json_object,
    to_json_value,
)
from .mom import MomViews, mom_data
from .object_views import ObjectViews, object_context
from .ownership import OwnershipViews, ownership_data
from .recording_codec import StorageFormat
from .regions import RegionViews, region_data
from .relationships import RelationshipViews, relationship_data
from .storage import MAX_EVIDENCE_BYTES, EvidenceBudget, EvidenceIndex, EvidenceQueue
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
    "declaration.observed",
    "relationship.observed",
    "coordination.observed",
    "ownership.observed",
    "region.observed",
    "mom.observed",
    "fom.observed",
    "service.completed",
    "service.failed",
    "session.connected",
    "session.disconnected",
    "session.inspected",
    "session.reconciled",
    "command.accepted",
    "command.running",
    "command.succeeded",
    "command.failed",
    "command.unavailable",
    "command.late_result",
    "event.raw",
)

EventSink = Callable[[RawObserverEvent], object]


class EventSource(Protocol):
    """Minimal source contract an RTI/provider adapter may implement."""

    def subscribe(self, sink: EventSink) -> None:
        """Send normalized events to ``sink`` until unsubscribed."""

    def unsubscribe(self, sink: EventSink) -> None:
        """Stop sending events to ``sink``."""


class ObserverAdapter(Protocol):
    """Lifecycle contract for a provider-specific adapter package.

    The adapter owns all provider imports and callback/session differences. The
    sink accepts raw provider event mappings. ``ObserverEventMetadata`` names
    the common envelope fields; family-specific observation records are
    validated by their domain modules, and provider extension fields are
    converted to JSON values before storage. Adapters may additionally expose
    a ``capabilities`` mapping; older adapters remain valid and are reported as
    unknown for provider-specific features.
    """

    def start(self, sink: EventSink) -> None:
        """Start forwarding provider events to ``sink``."""

    def stop(self) -> None:
        """Stop forwarding provider events and release provider resources."""


class CapabilitySource(Protocol):
    """Optional adapter surface for provider capability declarations."""

    # opaque-boundary: Adapters may expose provider-specific capability values.
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


def _first_text(event: Mapping[str, JsonValue], *names: str) -> str | None:
    for name in names:
        value = _text(event.get(name))
        if value:
            return value
    return None


def _event_family(event: Mapping[str, JsonValue]) -> str:
    # Family is always explicit. In particular, this does not classify an
    # event from an HLA class name or a provider-specific naming convention.
    return _first_text(event, "family") or "generic"


def _source_name(event: Mapping[str, JsonValue]) -> str:
    return _first_text(event, "source", "connection") or "unknown"


def _event_type(event: Mapping[str, JsonValue]) -> str:
    return _first_text(event, "event_type", "kind") or "event.raw"


def _object_key(event: Mapping[str, JsonValue]) -> str | None:
    supplied = event.get("object_key")
    if isinstance(supplied, str) and supplied.strip():
        return supplied  # An explicit opaque key is not a display label.
    return _first_text(event, "object_key", "object_handle", "object_name")


def _interaction_key(event: Mapping[str, JsonValue]) -> str | None:
    return _first_text(
        event, "interaction_key", "interaction_handle", "interaction_class"
    )


def _federate_name(event: EventRecord) -> str | None:
    perspective = participant_event_perspective(event_participant_metadata(event))
    return perspective.label if perspective else None


def _copy_mapping(value: object) -> JsonObject:
    if not isinstance(value, Mapping):
        return {}
    return {str(key): to_json_value(item) for key, item in value.items()}


# opaque-boundary: Validate raw adapter events before JSON-compatible normalization.
def _normalise_event(event: RawObserverEvent, sequence: int) -> EventRecord:
    """Validate common metadata and normalize family/provider extension fields."""
    if not isinstance(event, Mapping):
        raise TypeError("observer events must be mappings")
    if any(not isinstance(field, str) for field in event):
        raise TypeError("observer event field names must be strings")
    for field in ("event_type", "kind", "family", "source", "connection"):
        value = event.get(field)
        if value is not None and not isinstance(value, str):
            raise TypeError(f"{field} must be a string")
    for field in ("federate", "participant"):
        value = event.get(field)
        if value is not None and not isinstance(value, (str, Mapping)):
            raise TypeError(f"{field} must be a label or a named metadata record")
        if isinstance(value, Mapping) and any(
            not isinstance(name, str) for name in value
        ):
            raise TypeError(f"{field} metadata field names must be strings")
    for field in ("federation", "federate_name", "participant_name", "object_key"):
        value = event.get(field)
        if value is not None and not isinstance(value, str):
            raise TypeError(f"{field} must be a string")
    if event.get("session_id") is not None:
        identity_text(event.get("session_id"), limit=256)
    participant_event_metadata(event)
    object_context(event)
    time_data(event)  # Validate explicit observations before any batch mutation.
    declaration_observation = declaration_data(event)
    relationship_observation = relationship_data(event)
    coordination_observation = coordination_data(event)
    interaction_flow_observation = interaction_flow_data(event)
    ownership_observation = ownership_data(event)
    region_observation = region_data(event)
    mom_observation = mom_data(event)
    fom_observation = fom_data(event)
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
        fom_observation=fom_observation,
        mom_observation=mom_observation,
        declaration_observation=declaration_observation,
        relationship_observation=relationship_observation,
        coordination_observation=coordination_observation,
        interaction_flow_observation=interaction_flow_observation,
        ownership_observation=ownership_observation,
        region_observation=region_observation,
    )


def _status_for_event(event_type: str) -> str:
    if event_type == "federate.resigned":
        return "resigned"
    if event_type == "runtime.error":
        return "error"
    return "active"


def _derive_objects(events: Iterable[EventRecord]) -> list[ObjectInspection]:
    rows: dict[tuple[str | None, str | None, str], ObjectInspection] = {}
    for event in events:
        key = _object_key(event)
        if not key:
            continue
        event_type = _event_type(event)
        context = object_context(event)
        if not (
            event_type.startswith("object.")
            or "object_key" in event
            or "object_handle" in event
            or "object_name" in event
        ):
            continue
        row = rows.setdefault(
            (
                context.identity_namespace if context else None,
                context.federation_id if context else None,
                key,
            ),
            ObjectInspection(
                object_key=key,
                object_name=_first_text(event, "object_name") or key,
                object_handle=_first_text(event, "object_handle"),
                class_name=_first_text(event, "class_name"),
                class_handle=_first_text(event, "class_handle"),
                family=_event_family(event),
                last_seen=_timestamp(event.get("observed_at")),
                object_context=context,
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


def _derive_interactions(events: Iterable[EventRecord]) -> list[InteractionInspection]:
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
                perspective=participant_event_perspective(
                    event_participant_metadata(event)
                ),
            )
        )
    return rows[-250:]


def _derive_federates(events: Iterable[EventRecord]) -> list[FederateInspection]:
    rows: dict[str, FederateInspection] = {}
    for event in events:
        perspective = participant_event_perspective(event_participant_metadata(event))
        event_type = _event_type(event)
        if perspective is None:
            continue
        row = rows.setdefault(
            perspective.key,
            FederateInspection(
                federate=perspective.label,
                role=_first_text(event, "role"),
                handle=_first_text(event, "federate_handle", "participant_handle"),
                status="active",
                source=_source_name(event),
                last_seen=_timestamp(event.get("observed_at")),
                perspective=perspective,
            ),
        )
        row.federate = perspective.label
        row.perspective = perspective
        row.role = _first_text(event, "role") or row.role
        row.handle = (
            (
                perspective.attribution.participant.handle
                if perspective.attribution
                else None
            )
            or _first_text(event, "federate_handle", "participant_handle")
            or row.handle
        )
        row.source = _source_name(event)
        row.status = _status_for_event(event_type)
        row.last_seen = _timestamp(event.get("observed_at"))
    return sorted(rows.values(), key=lambda row: row.federate)


def _derive_metrics(events: Iterable[EventRecord]) -> LiveMetrics:
    events = list(events)
    types = Counter(_event_type(event) for event in events)
    families = Counter(_event_family(event) for event in events)
    sources = Counter(_source_name(event) for event in events)
    timestamps = [event.observed_at for event in events]
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
    return (
        float(value)
        if isinstance(value, (int, float)) and not isinstance(value, bool)
        else None
    )


def build_event_schema() -> JsonObject:
    """Return the stable JSON schema advertised by the HTTP service."""

    return to_json_object(
        {
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
                "family": {
                    "type": "string",
                    "description": "Explicit caller-defined grouping.",
                },
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
        }
    )


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

    # opaque-boundary: Forward the provider's raw event to registered observers.
    def emit(self, event: RawObserverEvent) -> None:
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

    # opaque-boundary: Initial capability data comes from arbitrary adapters.
    def __init__(
        self,
        *,
        max_events: int = 2_000,
        max_batch_size: int = 10_000,
        max_object_perspectives: int = 5000,
        max_time_requests: int = 250,
        max_declarations: int = 1_000,
        max_relationships: int = 1_000,
        max_coordination: int = 1_000,
        max_coordination_transitions: int = 2_000,
        max_interaction_flow: int = 500,
        max_ownership_states: int = 500,
        max_ownership_requests: int = 500,
        max_ownership_transitions: int = 1_000,
        max_regions: int = 500,
        max_region_transitions: int = 1_000,
        max_mom_reports: int = 500,
        max_mom_transitions: int = 2_000,
        max_fom_elements: int = 1_000,
        max_fom_transitions: int = 2_000,
        max_objects: int = 5_000,
        max_federates: int = 2_000,
        max_time_snapshots: int = 2_000,
        max_evidence_bytes: int = MAX_EVIDENCE_BYTES,
        max_evidence_record_bytes: int | None = None,
        capabilities: Mapping[str, object] | None = None,
    ) -> None:
        if max_events < 1:
            raise ValueError("max_events must be at least 1")
        if max_batch_size < 1:
            raise ValueError("max_batch_size must be at least 1")
        if (
            isinstance(max_time_requests, bool)
            or not isinstance(max_time_requests, int)
            or max_time_requests < 1
        ):
            raise ValueError("max_time_requests must be a positive integer")
        if (
            isinstance(max_declarations, bool)
            or not isinstance(max_declarations, int)
            or max_declarations < 1
        ):
            raise ValueError("max_declarations must be a positive integer")
        if (
            isinstance(max_relationships, bool)
            or not isinstance(max_relationships, int)
            or max_relationships < 1
        ):
            raise ValueError("max_relationships must be a positive integer")
        if (
            isinstance(max_coordination, bool)
            or not isinstance(max_coordination, int)
            or max_coordination < 1
        ):
            raise ValueError("max_coordination must be a positive integer")
        if (
            isinstance(max_coordination_transitions, bool)
            or not isinstance(max_coordination_transitions, int)
            or max_coordination_transitions < 1
        ):
            raise ValueError("max_coordination_transitions must be a positive integer")
        if (
            isinstance(max_interaction_flow, bool)
            or not isinstance(max_interaction_flow, int)
            or max_interaction_flow < 1
        ):
            raise ValueError("max_interaction_flow must be a positive integer")
        for name, value in (
            ("max_ownership_states", max_ownership_states),
            ("max_ownership_requests", max_ownership_requests),
            ("max_ownership_transitions", max_ownership_transitions),
            ("max_regions", max_regions),
            ("max_region_transitions", max_region_transitions),
            ("max_mom_reports", max_mom_reports),
            ("max_mom_transitions", max_mom_transitions),
            ("max_fom_elements", max_fom_elements),
            ("max_fom_transitions", max_fom_transitions),
        ):
            if isinstance(value, bool) or not isinstance(value, int) or value < 1:
                raise ValueError(f"{name} must be a positive integer")
        self._evidence = EvidenceBudget(max_evidence_bytes, max_evidence_record_bytes)
        self._context_budget = EvidenceBudget(256 * 1024, 128 * 1024)
        self._events = EvidenceQueue[EventRecord](
            "events",
            max_events,
            self._evidence,
            lambda row: row.to_json(),
            unit="envelopes",
            contiguous=True,
        )
        self._max_events = max_events
        self._max_batch_size = max_batch_size
        self._next_sequence = 1
        # Observer storage lifetime, never an RTI connection or membership ID.
        self._instance_id = uuid4().hex
        self._revision = 0
        self._dropped_events = 0
        self._cleared_events = 0
        self._status = "idle"
        self._context = EvidenceIndex[str, JsonValue](
            "context",
            128,
            self._context_budget,
            lambda key, value: {key: value},
            unit="keys",
        )
        # These indexes are intentionally independent from the bounded event
        # deque. A long-running observer must not forget a live object or
        # participant merely because its older event history was evicted.
        self._objects = EvidenceIndex[
            tuple[str | None, str | None, str], ObjectInspection
        ]("objects", max_objects, self._evidence, lambda _, row: row.to_json())
        self._object_views = ObjectViews(
            max_events, max_object_perspectives, budget=self._evidence
        )
        self._time_views = TimeViews(
            max_time_requests, max_time_snapshots, budget=self._evidence
        )
        self._declarations = DeclarationViews(max_declarations, budget=self._evidence)
        self._relationships = RelationshipViews(
            max_relationships, budget=self._evidence
        )
        self._coordination = CoordinationViews(
            max_coordination, max_coordination_transitions, budget=self._evidence
        )
        self._interaction_flow = InteractionFlowViews(
            max_interaction_flow, budget=self._evidence
        )
        self._ownership = OwnershipViews(
            max_ownership_states,
            max_ownership_requests,
            max_ownership_transitions,
            budget=self._evidence,
        )
        self._regions = RegionViews(
            max_regions, max_region_transitions, budget=self._evidence
        )
        self._mom = MomViews(
            max_mom_reports, max_mom_transitions, budget=self._evidence
        )
        self._fom = FomViews(
            max_fom_elements, max_fom_transitions, budget=self._evidence
        )
        self._federates = EvidenceIndex[str, FederateInspection](
            "federates", max_federates, self._evidence, lambda _, row: row.to_json()
        )
        self._interactions = EvidenceQueue[InteractionInspection](
            "interactions", 250, self._evidence, lambda row: row.to_json()
        )
        self._adapters: dict[int, tuple[ObserverAdapter, str, str]] = {}
        self._manual_capabilities: dict[str, CapabilityRecord] = normalize_capabilities(
            capabilities,
            owner="application",
            source="store",
        )
        self._adapter_capabilities: dict[int, dict[str, CapabilityRecord]] = {}
        self._condition = threading.Condition()
        self._capture: CaptureBuffer | None = None
        self._alerts = AlertEngine()
        self._validate_metadata_locked()

    @property
    def alert_instance_id(self) -> str:
        return self._alerts.instance_id

    def evaluate_alerts(self, commands: CommandWatch = NO_COMMAND_WATCH) -> None:
        with self._condition:
            self._alerts.evaluate(self._dropped_events + self._events.dropped, commands)

    def alert_report(self, commands: CommandWatch = NO_COMMAND_WATCH) -> JsonObject:
        with self._condition:
            return {
                "contract_version": OBSERVER_CONTRACT_VERSION,
                **self._alerts.report(
                    self._dropped_events + self._events.dropped, commands
                ),
            }

    def configure_alerts(
        self,
        rules: Sequence[AlertRuleSpec],
        expected_revision: int,
        commands: CommandWatch = NO_COMMAND_WATCH,
    ) -> JsonObject:
        with self._condition:
            self._alerts.configure(
                rules, expected_revision, self._dropped_events + self._events.dropped
            )
            return self.alert_report(commands)

    def reset_alert(
        self,
        rule_id: str,
        expected_revision: int,
        commands: CommandWatch = NO_COMMAND_WATCH,
    ) -> JsonObject:
        with self._condition:
            self._alerts.configure(
                tuple(row.spec for row in self._alerts.runs.values()),
                expected_revision,
                self._dropped_events + self._events.dropped,
                reset_id=rule_id,
            )
            return self.alert_report(commands)

    def acknowledge_alert(
        self, evidence_id: int, commands: CommandWatch = NO_COMMAND_WATCH
    ) -> JsonObject:
        with self._condition:
            self._alerts.acknowledge(evidence_id)
            return self.alert_report(commands)

    def set_context(self, **values: object) -> JsonObject:
        with self._condition:
            self._apply_context_locked(
                {
                    str(key): _jsonable(value)
                    for key, value in values.items()
                    if value is not None
                }
            )
            self._capture_locked("metadata")
            return to_json_object(dict(self._context))

    def set_status(self, status: str, **context: object) -> JsonObject:
        status_text = _text(status)
        if not status_text:
            raise ValueError("status must not be empty")
        if len(status_text.encode("utf-8")) > 256:
            raise ValueError("status exceeds 256 UTF-8 bytes")
        with self._condition:
            self._apply_context_locked(
                {
                    str(key): _jsonable(value)
                    for key, value in context.items()
                    if value is not None
                }
            )
            self._status = status_text
            self._capture_locked("metadata")
            self._condition.notify_all()
            return self._state_locked(include_events=False)

    # opaque-boundary: Accept provider capability manifests before normalization.
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
            previous = self._manual_capabilities.copy()
            self._manual_capabilities.update(normalized)
            try:
                self._validate_metadata_locked()
            except (ValueError, TypeError, UnicodeError):
                self._manual_capabilities = previous
                raise
            self._capture_locked("metadata")
            self._condition.notify_all()
            return self._capabilities_locked()

    def capabilities(self) -> dict[str, JsonObject]:
        """Return the effective capability manifest for this observer."""

        with self._condition:
            return self._capabilities_locked()

    # opaque-boundary: Accept raw provider events at the observer's public ingress.
    def publish(self, event: RawObserverEvent) -> JsonObject:
        with self._condition:
            normalized = _normalise_event(event, self._next_sequence)
            self._append_locked([normalized])
            self._condition.notify_all()
            return to_json_object(normalized.to_json())

    # opaque-boundary: Validate each raw provider event before batch insertion.
    def publish_many(self, events: Iterable[RawObserverEvent]) -> int:
        rows = list(events)
        if len(rows) > self._max_batch_size:
            raise ValueError(
                f"event batch exceeds max_batch_size ({self._max_batch_size})"
            )
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
        if len(name.encode("utf-8")) > 1024:
            raise ValueError("adapter name exceeds 1024 UTF-8 bytes")
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
            if len(self._adapters) >= 64:
                raise ValueError("observer supports at most 64 attached adapters")
            self._adapters[key] = (adapter, name, "starting")
            self._adapter_capabilities[key] = adapter_capabilities
            try:
                self._validate_metadata_locked()
            except (ValueError, TypeError, UnicodeError):
                self._adapters.pop(key, None)
                self._adapter_capabilities.pop(key, None)
                raise
            self._capture_locked("metadata")
        try:
            adapter.start(self.publish)
        except Exception as error:
            with self._condition:
                self._adapters.pop(key, None)
                self._adapter_capabilities.pop(key, None)
                self._capture_locked("metadata")
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
                self._capture_locked("metadata")

    def detach_adapter(self, adapter: ObserverAdapter) -> None:
        """Stop a provider-specific adapter previously attached to this store."""

        key = id(adapter)
        try:
            adapter.stop()
        finally:
            with self._condition:
                self._adapters.pop(key, None)
                self._adapter_capabilities.pop(key, None)
                self._capture_locked("metadata")

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
        with self._condition:
            if self._capture is not None:
                self._capture.stop("backend-stopped")
        if first_error is not None:
            raise first_error

    def events(
        self, *, after_sequence: int = 0, limit: int | None = None
    ) -> list[JsonObject]:
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
        instance_id: str | None = None,
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
            reset = instance_id is not None and (
                instance_id != self._instance_id
                or after_sequence >= self._next_sequence
            )
            if reset:
                after_sequence = 0
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
            gap = bool(
                first_sequence
                and after_sequence < first_sequence - 1
                or not retained
                and after_sequence < self._next_sequence - 1
            )
            next_after = rows[-1].sequence if rows else after_sequence
            if gap and not rows:
                next_after = first_sequence - 1 if retained else self._next_sequence - 1
            document = to_json_object(
                EventPage(
                    events=rows,
                    next_after=next_after,
                    has_more=len(candidates) > len(rows),
                    gap=gap,
                    first_retained_sequence=first_sequence,
                    last_sequence=last_sequence,
                    retained_events=len(retained),
                    dropped_events=self._dropped_events + self._events.dropped,
                    cleared_events=self._cleared_events,
                    next_sequence=self._next_sequence,
                ).to_json(OBSERVER_CONTRACT_VERSION)
            )
            document["observer_lifetime"] = {
                "instance_id": self._instance_id,
                "revision": self._revision,
            }
            document["reset"] = reset
            return document

    def wait_for_events(
        self, *, after_sequence: int = 0, timeout: float = 15.0
    ) -> list[JsonObject]:
        deadline = time.monotonic() + max(timeout, 0.0)
        with self._condition:
            while not any(event.sequence > after_sequence for event in self._events):
                if not self._events and after_sequence < self._next_sequence - 1:
                    break
                remaining = deadline - time.monotonic()
                if remaining <= 0:
                    break
                self._condition.wait(remaining)
            return self._events_locked(after_sequence=after_sequence, limit=None)

    def wait_for_change(self, revision: int, *, timeout: float = 15.0) -> None:
        """Wait for any observable mutation, including an empty clear/metadata."""
        with self._condition:
            self._condition.wait_for(
                lambda: self._revision != revision, timeout=max(0.0, timeout)
            )

    def clear(self) -> None:
        with self._condition:
            self._cleared_events += len(self._events)
            self._dropped_events += self._events.dropped
            self._events.clear()
            self._objects.clear()
            self._object_views.clear()
            self._time_views.clear()
            self._declarations.clear()
            self._relationships.clear()
            self._coordination.clear()
            self._interaction_flow.clear()
            self._ownership.clear()
            self._regions.clear()
            self._mom.clear()
            self._fom.clear()
            self._federates.clear()
            self._interactions.clear()
            self._capture_locked("clear")
            self._condition.notify_all()

    def state(self, *, include_events: bool = True) -> JsonObject:
        with self._condition:
            return self._state_locked(include_events=include_events)

    def analytics_window(self) -> JsonObject:
        """Take one consistent projection without building unrelated inspectors.

        Keep the full retained envelope and persistent roster semantics. In
        particular, this is not a sample and an idle roster entry is not lost.
        """
        with self._condition:
            provenance = self._capabilities_locked().get("participant_provenance")
            return to_json_object(
                {
                    "contract_version": OBSERVER_CONTRACT_VERSION,
                    "participant_provenance": provenance is not None
                    and provenance.get("state") in {"available", "partial"},
                    "retention": self._retention_locked().to_json(),
                    "federate_roster": [
                        row.to_json()
                        for row in sorted(
                            self._federates.values(), key=lambda row: row.federate
                        )
                    ],
                    "normalized_events": [event.to_json() for event in self._events],
                    "storage": self._storage_locked(),
                }
            )

    def runtime_summary(self) -> JsonObject:
        """Health/metrics fields only, without walking any inspector history."""
        with self._condition:
            return to_json_object(
                {
                    "contract_version": OBSERVER_CONTRACT_VERSION,
                    "schema_version": OBSERVER_SCHEMA_VERSION,
                    "status": self._status,
                    "live_metrics": self._metrics_locked().to_json(),
                    "retention": self._retention_locked().to_json(),
                    "adapters": [row.to_json() for row in self._adapters_locked()],
                    "capabilities": self._capabilities_locked(),
                }
            )

    def start_capture(
        self,
        label: str,
        *,
        replace_id: str | None = None,
        max_frames: int = 600,
        max_bytes: int = 16 * 1024 * 1024,
        storage_format: StorageFormat = "full-v1",
    ) -> CaptureInfo:
        """Start one process-local journal; replacing retained data requires its ID."""
        with self._condition:
            if self._capture is not None and (
                self._capture.info().active or replace_id != self._capture.recording_id
            ):
                raise CaptureConflict(
                    "A recording is active or retained. Stop/export it and explicitly replace its recording ID."
                )
            manifest = self._capabilities_locked()
            contract = build_contract_document(
                EVENT_TYPES,
                event_schema_version=OBSERVER_SCHEMA_VERSION,
                capabilities=manifest,
            )
            # Ordinary discovery normalizes capability declarations. A capture
            # also preserves the exact aggregated owners/reporter observations.
            contract["capabilities"] = to_json_value(manifest)
            if storage_format not in ("full-v1", "compact-v2"):
                raise ValueError("Unsupported recording storage format.")
            buffer_type = (
                CompactCaptureBuffer
                if storage_format == "compact-v2"
                else CaptureBuffer
            )
            candidate = buffer_type(
                label, contract, max_frames=max_frames, max_bytes=max_bytes
            )
            candidate.append(self._state_locked(include_events=True), "start")
            if not candidate.info().frame_count:
                raise ValueError(
                    "The initial observer snapshot does not fit the recording budget or is not valid JSON."
                )
            self._capture = candidate
            return candidate.info()

    def capture_info(self) -> CaptureInfo | None:
        with self._condition:
            return self._capture.info() if self._capture is not None else None

    def checkpoint_capture(self) -> None:
        """Capture clock-dependent inspector ages without fabricating an event."""
        with self._condition:
            self._capture_locked("checkpoint")

    def stop_capture(self, recording_id: str) -> CaptureInfo:
        with self._condition:
            capture = self._require_capture_locked(recording_id)
            self._capture_locked("stop")
            capture.stop()
            return capture.info()

    def capture_document(self, recording_id: str) -> JsonObject:
        with self._condition:
            return self._require_capture_locked(recording_id).document()

    def _require_capture_locked(self, recording_id: str) -> CaptureBuffer:
        if self._capture is None or self._capture.recording_id != recording_id:
            raise CaptureConflict(
                "That recording is no longer retained by this backend."
            )
        return self._capture

    def _capture_locked(
        self, change: CaptureChange, arrivals: Sequence[EventRecord] | None = None
    ) -> None:
        # Commit a mutation cut even when optional recording is not enabled.
        # Timer/stop journal frames capture a cut; they do not mutate live data.
        if change not in {"checkpoint", "stop"}:
            self._revision += 1
            self._condition.notify_all()
        if self._capture is not None and self._capture.info().active:
            self._capture.append(
                self._state_locked(include_events=True),
                change,
                [event.to_json() for event in arrivals] if arrivals else None,
            )

    def _events_locked(
        self, *, after_sequence: int, limit: int | None
    ) -> list[JsonObject]:
        rows = [
            to_json_object(event.to_json())
            for event in self._events
            if event.sequence > max(after_sequence, 0)
        ]
        if limit is not None:
            rows = rows[: max(limit, 0)]
        return rows

    def _append_locked(self, normalized: Sequence[EventRecord]) -> None:
        for event in normalized:
            self._events.append(event)
            self._next_sequence += 1
            self._update_indexes_locked(event)
            if self._alerts.runs:
                metadata = event_participant_metadata(event)
                perspective = participant_event_perspective(metadata)
                self._alerts.observe(
                    AlertScope(
                        event.source,
                        _first_text(event, "federation"),
                        _federate_name(event),
                        perspective.session_id if perspective else metadata.session_id,
                        perspective,
                    ),
                    event.event_type,
                    event.sequence,
                )
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
        self._object_views.trim_history(self._retention_locked())
        self._alerts.commit(self._dropped_events + self._events.dropped)
        self._capture_locked("events", normalized)

    def _update_indexes_locked(self, event: EventRecord) -> None:
        self._time_views.record(
            event, _federate_name(event), _first_text(event, "federation")
        )
        self._declarations.record(
            event, _federate_name(event), _first_text(event, "federation")
        )
        self._relationships.record(event)
        self._coordination.record(event)
        self._interaction_flow.record(event)
        self._ownership.record(event)
        self._regions.record(event)
        self._mom.record(event)
        self._fom.record(event)
        objects = _derive_objects([event])
        self._object_views.record(event, objects, _federate_name(event))
        for incoming in objects:
            key = incoming.cache_key
            current = self._objects.get(key)
            if current is None or (
                current.status == "removed" and event.event_type == "object.discovered"
            ):
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
            self._objects.refresh(key)

        for incoming in _derive_federates([event]):
            key = (
                incoming.perspective.key if incoming.perspective else incoming.federate
            )
            current = self._federates.get(key)
            if current is None:
                self._federates[key] = incoming
                continue
            current.role = incoming.role or current.role
            current.federate = incoming.federate
            current.perspective = incoming.perspective
            current.handle = incoming.handle or current.handle
            if not (
                current.status in {"resigned", "reporting-ended"}
                and current.perspective is not None
                and current.perspective.attribution is not None
            ):
                current.status = incoming.status
            current.source = incoming.source
            current.last_seen = incoming.last_seen
            self._federates.refresh(key)

        if event.event_type == "session.disconnected":
            for key, current in tuple(self._federates.items()):
                if current.perspective is not None and participant_event_ended(
                    current.perspective, event_participant_metadata(event)
                ):
                    if current.status != "resigned":
                        current.status = "reporting-ended"
                    self._federates.refresh(key)

        self._interactions.extend(_derive_interactions([event]))

    def _retention_locked(self) -> Retention:
        first_sequence = self._events[0].sequence if self._events else 0
        last_sequence = self._events[-1].sequence if self._events else 0
        return Retention(
            max_events=self._max_events,
            retained_events=len(self._events),
            dropped_events=self._dropped_events + self._events.dropped,
            cleared_events=self._cleared_events,
            first_retained_sequence=first_sequence,
            last_sequence=last_sequence,
            next_sequence=self._next_sequence,
        )

    def _adapters_locked(self) -> list[AdapterStatus]:
        rows: list[AdapterStatus] = []
        for key, (_, name, status) in sorted(
            self._adapters.items(), key=lambda entry: entry[1][1]
        ):
            capabilities: tuple[str, ...] = ()
            if self._adapter_capabilities.get(key):
                capabilities = tuple(sorted(self._adapter_capabilities[key]))
            rows.append(
                AdapterStatus(name=name, status=status, capabilities=capabilities)
            )
        return rows

    def _capabilities_locked(self) -> dict[str, JsonObject]:
        adapter_features = {
            name
            for manifest in self._adapter_capabilities.values()
            for name in manifest
        }
        optional_inspectors = {
            "declarations",
            "relationships",
            "synchronization",
            "persistence_inspection",
            "time_inspection",
            "interaction_flow",
            "ownership_inspection",
            "regions_inspection",
            "mom_inspection",
            "fom_inspection",
        }
        effective = merge_capabilities(
            # Accepting a normalized model is infrastructure support, not
            # contrary evidence that an explicitly unsupported RTI feature is
            # partially available. Aggregate actual reporters with each other.
            {
                name: value
                for name, value in OBSERVER_CAPABILITIES.items()
                if name not in optional_inspectors or name not in adapter_features
            },
            *self._adapter_capabilities.values(),
        )
        # Store/application declarations are authoritative. This is what lets
        # an operator disable a provider feature without changing the adapter
        # or making the frontend guess whether an empty result is intentional.
        for name, manifest in self._manual_capabilities.items():
            effective[name] = merge_capabilities({name: manifest})[name]
        return {
            name: capability.to_json() for name, capability in sorted(effective.items())
        }

    def _validate_metadata_locked(self) -> None:
        encoded = json.dumps(
            self._capabilities_locked(),
            ensure_ascii=False,
            allow_nan=False,
            separators=(",", ":"),
        )
        if len(encoded.encode("utf-8")) > 128 * 1024:
            raise ValueError("aggregated capability metadata exceeds 128 KiB")
        adapters = json.dumps(
            [row.to_json() for row in self._adapters_locked()],
            ensure_ascii=False,
            allow_nan=False,
            separators=(",", ":"),
        )
        if len(adapters.encode("utf-8")) > 128 * 1024:
            raise ValueError("aggregated adapter metadata exceeds 128 KiB")

    def _apply_context_locked(self, updates: JsonObject) -> None:
        # Explicit operator/app updates are admitted atomically. Event-derived
        # context is observation evidence and instead reports whole-key expiry.
        candidate = {
            key: value for key, value in self._context.items() if key not in updates
        }
        candidate.update(updates)
        if len(candidate) > self._context.capacity:
            raise ValueError("context exceeds 128 retained keys")
        sizes = [
            len(
                json.dumps(
                    {key: value},
                    ensure_ascii=False,
                    allow_nan=False,
                    separators=(",", ":"),
                ).encode("utf-8")
            )
            + 64
            for key, value in candidate.items()
        ]
        if (
            any(size > self._context_budget.max_record_bytes for size in sizes)
            or sum(sizes) > self._context_budget.max_bytes
        ):
            raise ValueError("context update exceeds its encoded key/total byte budget")
        for key in tuple(self._context):
            del self._context[key]
        self._context.update(candidate)

    def _metrics_locked(self) -> LiveMetrics:
        return replace(
            _derive_metrics(self._events),
            object_count=len(self._objects),
            interaction_count=len(self._interactions),
            federate_count=len(self._federates),
        )

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
        metrics = self._metrics_locked()
        state = to_json_object(
            {
                "contract_version": OBSERVER_CONTRACT_VERSION,
                "observer_lifetime": {
                    "instance_id": self._instance_id,
                    "revision": self._revision,
                },
                "schema_version": OBSERVER_SCHEMA_VERSION,
                "status": self._status,
                "context": dict(self._context),
                "live_metrics": metrics.to_json(),
                "retention": self._retention_locked().to_json(),
                "adapters": [row.to_json() for row in self._adapters_locked()],
                "capabilities": self._capabilities_locked(),
                "storage": self._storage_locked(),
                "federate_roster": [row.to_json() for row in federates],
                "inspectors": {
                    "objects": [row.to_json() for row in objects],
                    "interactions": [row.to_json() for row in interactions],
                    "object_observations": self._object_views.to_json(
                        self._retention_locked()
                    ),
                    "time": self._time_views.to_json(),
                    "declarations": self._declarations.to_json(),
                    "relationships": self._relationships.to_json(),
                    "coordination": self._coordination.to_json(),
                    "interaction_flow": self._interaction_flow.to_json(),
                    "ownership": self._ownership.to_json(),
                    "regions": self._regions.to_json(),
                    "mom": self._mom.to_json(),
                    "fom": self._fom.to_json(),
                },
            }
        )
        if include_events:
            state["normalized_events"] = [event.to_json() for event in events]
        return to_json_object(state)

    def _storage_locked(self) -> JsonObject:
        return {
            "evidence": self._evidence.coverage(),
            "context": self._context_budget.coverage(),
        }


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
