"""Explicit observer domain models and JSON boundary helpers.

The observer accepts provider-specific input at one deliberate boundary, then
stores typed records internally. ``to_json`` is the only place where those
records become wire-shaped dictionaries. This keeps the dashboard contract
flexible without making the store itself a collection of opaque mappings.
"""

from __future__ import annotations

from collections.abc import Iterator, Mapping
from dataclasses import dataclass, field
from typing import Literal, TypeAlias

JsonScalar: TypeAlias = None | bool | int | float | str
JsonValue: TypeAlias = JsonScalar | list["JsonValue"] | dict[str, "JsonValue"]
JsonObject: TypeAlias = dict[str, JsonValue]
CapabilityState: TypeAlias = Literal[
    "available",
    "partial",
    "disabled",
    "unsupported",
    "unknown",
]


def to_json_value(value: object) -> JsonValue:
    """Convert an adapter value into a bounded JSON-compatible value."""

    if value is None or isinstance(value, (bool, int, float, str)):
        return value
    if isinstance(value, (bytes, bytearray, memoryview)):
        return {"encoding": "hex", "value": bytes(value).hex()}
    if isinstance(value, Mapping):
        return {str(key): to_json_value(item) for key, item in value.items()}
    if isinstance(value, (list, tuple, set, frozenset)):
        return [to_json_value(item) for item in value]
    return str(value)


def to_json_object(value: object) -> JsonObject:
    """Return an object value, rejecting non-object values at the boundary."""

    if not isinstance(value, Mapping):
        raise TypeError("expected a JSON object")
    converted = to_json_value(value)
    if not isinstance(converted, dict):  # pragma: no cover - guarded above
        raise TypeError("expected a JSON object")
    return converted


@dataclass(frozen=True, slots=True)
class EventRecord(Mapping[str, JsonValue]):
    """Normalized event envelope plus provider-specific extension fields."""

    sequence: int
    event_type: str
    family: str
    source: str
    observed_at: float
    extensions: JsonObject = field(default_factory=dict)

    def to_json(self) -> JsonObject:
        result = dict(self.extensions)
        result.update(
            {
                "sequence": self.sequence,
                "event_type": self.event_type,
                "family": self.family,
                "source": self.source,
                "observed_at": self.observed_at,
            }
        )
        return result

    def __getitem__(self, key: str) -> JsonValue:
        return self.to_json()[key]

    def __iter__(self) -> Iterator[str]:
        return iter(self.to_json())

    def __len__(self) -> int:
        return len(self.to_json())


@dataclass(frozen=True, slots=True)
class CapabilityRecord(Mapping[str, JsonValue]):
    """Normalized capability declaration with provenance and extensions."""

    state: CapabilityState
    label: str
    description: str
    owner: str
    source: str | None = None
    owners: tuple[str, ...] = ()
    sources: tuple[str, ...] = ()
    observations: tuple[JsonObject, ...] = ()
    reason: str | None = None
    reasons: tuple[str, ...] = ()
    event_types: tuple[str, ...] = ()
    extensions: JsonObject = field(default_factory=dict)

    def to_json(self) -> JsonObject:
        result = dict(self.extensions)
        result.update(
            {
                "state": self.state,
                "label": self.label,
                "description": self.description,
                "owner": self.owner,
            }
        )
        if self.source is not None:
            result["source"] = self.source
        if self.owners:
            result["owners"] = list(self.owners)
        if self.sources:
            result["sources"] = list(self.sources)
        if self.observations:
            result["observations"] = [dict(item) for item in self.observations]
        if self.reason is not None:
            result["reason"] = self.reason
        if self.reasons:
            result["reasons"] = list(self.reasons)
        if self.event_types:
            result["event_types"] = list(self.event_types)
        return result

    def __getitem__(self, key: str) -> JsonValue:
        return self.to_json()[key]

    def __iter__(self) -> Iterator[str]:
        return iter(self.to_json())

    def __len__(self) -> int:
        return len(self.to_json())


@dataclass(slots=True)
class ObjectInspection:
    object_key: str
    object_name: str
    object_handle: str | None
    class_name: str | None
    class_handle: str | None
    family: str
    attributes: JsonObject = field(default_factory=dict)
    update_count: int = 0
    discovery_count: int = 0
    sources: list[str] = field(default_factory=list)
    status: str = "active"
    last_seen: float | None = None

    def to_json(self) -> JsonObject:
        return {
            "object_key": self.object_key,
            "object_name": self.object_name,
            "object_handle": self.object_handle,
            "class_name": self.class_name,
            "class_handle": self.class_handle,
            "family": self.family,
            "attributes": dict(self.attributes),
            "update_count": self.update_count,
            "discovery_count": self.discovery_count,
            "sources": list(self.sources),
            "status": self.status,
            "last_seen": self.last_seen,
        }


@dataclass(frozen=True, slots=True)
class InteractionInspection:
    sequence: int
    interaction_key: str
    interaction_class: str
    class_handle: str | None
    family: str
    source: str
    federate: str | None
    parameters: JsonObject
    tag: JsonValue
    observed_at: float

    def to_json(self) -> JsonObject:
        return {
            "sequence": self.sequence,
            "interaction_key": self.interaction_key,
            "interaction_class": self.interaction_class,
            "class_handle": self.class_handle,
            "family": self.family,
            "source": self.source,
            "federate": self.federate,
            "parameters": dict(self.parameters),
            "tag": self.tag,
            "observed_at": self.observed_at,
        }


@dataclass(slots=True)
class FederateInspection:
    federate: str
    role: str | None
    handle: str | None
    status: str
    source: str
    last_seen: float | None

    def to_json(self) -> JsonObject:
        return {
            "federate": self.federate,
            "role": self.role,
            "handle": self.handle,
            "status": self.status,
            "source": self.source,
            "last_seen": self.last_seen,
        }


@dataclass(frozen=True, slots=True)
class LiveMetrics:
    event_count: int
    event_rate_per_second: float
    error_count: int
    event_types: dict[str, int]
    families: dict[str, int]
    sources: dict[str, int]
    first_sequence: int
    last_sequence: int
    object_count: int = 0
    interaction_count: int = 0
    federate_count: int = 0

    def to_json(self) -> JsonObject:
        return {
            "event_count": self.event_count,
            "event_rate_per_second": self.event_rate_per_second,
            "error_count": self.error_count,
            "event_types": dict(self.event_types),
            "families": dict(self.families),
            "sources": dict(self.sources),
            "first_sequence": self.first_sequence,
            "last_sequence": self.last_sequence,
            "object_count": self.object_count,
            "interaction_count": self.interaction_count,
            "federate_count": self.federate_count,
        }


@dataclass(frozen=True, slots=True)
class Retention:
    max_events: int
    retained_events: int
    dropped_events: int
    cleared_events: int
    first_retained_sequence: int
    last_sequence: int
    next_sequence: int

    def to_json(self) -> JsonObject:
        return {
            "max_events": self.max_events,
            "retained_events": self.retained_events,
            "dropped_events": self.dropped_events,
            "cleared_events": self.cleared_events,
            "first_retained_sequence": self.first_retained_sequence,
            "last_sequence": self.last_sequence,
            "next_sequence": self.next_sequence,
        }


@dataclass(frozen=True, slots=True)
class AdapterStatus:
    name: str
    status: str
    capabilities: tuple[str, ...] = ()

    def to_json(self) -> JsonObject:
        result: JsonObject = {"name": self.name, "status": self.status}
        if self.capabilities:
            result["capabilities"] = list(self.capabilities)
        return result


@dataclass(frozen=True, slots=True)
class EventPage:
    events: tuple[EventRecord, ...]
    next_after: int
    has_more: bool
    gap: bool
    first_retained_sequence: int
    last_sequence: int
    retained_events: int
    dropped_events: int
    cleared_events: int
    next_sequence: int

    def to_json(self, contract_version: str) -> JsonObject:
        return {
            "contract_version": contract_version,
            "events": [event.to_json() for event in self.events],
            "next_after": self.next_after,
            "has_more": self.has_more,
            "gap": self.gap,
            "first_retained_sequence": self.first_retained_sequence,
            "last_sequence": self.last_sequence,
            "retained_events": self.retained_events,
            "dropped_events": self.dropped_events,
            "cleared_events": self.cleared_events,
            "next_sequence": self.next_sequence,
        }


__all__ = [
    "AdapterStatus",
    "CapabilityRecord",
    "CapabilityState",
    "EventPage",
    "EventRecord",
    "FederateInspection",
    "InteractionInspection",
    "JsonObject",
    "JsonScalar",
    "JsonValue",
    "LiveMetrics",
    "ObjectInspection",
    "Retention",
    "to_json_object",
    "to_json_value",
]
