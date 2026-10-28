"""Explicit observer domain models and JSON boundary helpers.

The observer accepts provider-specific input at one deliberate boundary, then
stores typed records internally. ``to_json`` is the only place where those
records become wire-shaped dictionaries. This keeps the dashboard contract
flexible without making the store itself a collection of opaque mappings.
"""

from __future__ import annotations

from collections.abc import Iterator, Mapping
from dataclasses import dataclass, field
from typing import TYPE_CHECKING, Literal, Protocol, TypeAlias, TypedDict

if TYPE_CHECKING:
    from .attribution import ParticipantPerspective
    from .object_views import ObjectContext

JsonScalar: TypeAlias = None | bool | int | float | str
JsonValue: TypeAlias = JsonScalar | list["JsonValue"] | dict[str, "JsonValue"]
JsonObject: TypeAlias = dict[str, JsonValue]


class EventObservationScope(Protocol):
    def key(self, source: str) -> tuple[str, str, str, str]: ...


class FomObservationContext(Protocol):
    @property
    def scope(self) -> EventObservationScope: ...

    @property
    def model_id(self) -> str: ...

    @property
    def revision(self) -> str: ...

    @property
    def element_id(self) -> str: ...


class MomObservationContext(Protocol):
    @property
    def scope(self) -> EventObservationScope: ...

    @property
    def report_id(self) -> str: ...


class FomObservation(Protocol):
    @property
    def context(self) -> FomObservationContext: ...

    def kind(self) -> str: ...

    def to_json(self) -> JsonObject: ...


class MomObservation(Protocol):
    @property
    def context(self) -> MomObservationContext: ...

    def kind(self) -> str: ...

    def to_json(self) -> JsonObject: ...


class DeclarationProjection(Protocol):
    @property
    def kind(self) -> Literal["object", "interaction"]: ...

    @property
    def direction(self) -> Literal["publication", "subscription"]: ...

    @property
    def class_key(self) -> str: ...

    @property
    def session_id(self) -> str | None: ...

    @property
    def class_name(self) -> str | None: ...

    @property
    def status(self) -> Literal["declared", "withdrawn", "unknown"]: ...

    @property
    def attribute_scope(
        self,
    ) -> Literal["selected", "all", "unknown", "not-applicable"]: ...

    @property
    def attributes(self) -> tuple[str, ...] | None: ...

    @property
    def reason(self) -> str | None: ...


class RelationshipProjection(Protocol):
    @property
    def relationship_id(self) -> str: ...

    @property
    def session_id(self) -> str: ...

    @property
    def identity_namespace(self) -> str: ...

    @property
    def federation_id(self) -> str: ...

    def to_json(self) -> JsonObject: ...


class ParticipantIdentityProjection(Protocol):
    @property
    def participant_id(self) -> str: ...

    @property
    def membership_id(self) -> str: ...


class OwnershipScopeProjection(Protocol):
    def key(self, source: str) -> tuple[str, str, str, str, str, str]: ...


class OwnershipStateProjection(Protocol):
    @property
    def kind(self) -> Literal["state"]: ...

    @property
    def scope(self) -> OwnershipScopeProjection: ...

    def to_json(self) -> JsonObject: ...


class OwnershipRequestProjection(Protocol):
    @property
    def kind(self) -> Literal["request"]: ...

    @property
    def scope(self) -> OwnershipScopeProjection: ...

    @property
    def request_id(self) -> str: ...

    @property
    def action(self) -> Literal["acquire", "divest"]: ...

    @property
    def requester(self) -> ParticipantIdentityProjection: ...

    def to_json(self) -> JsonObject: ...


class OwnershipOutcomeProjection(Protocol):
    @property
    def kind(self) -> Literal["outcome"]: ...

    @property
    def scope(self) -> OwnershipScopeProjection: ...

    @property
    def request_id(self) -> str: ...

    @property
    def action(self) -> Literal["acquire", "divest"]: ...

    @property
    def requester(self) -> ParticipantIdentityProjection: ...

    @property
    def outcome(self) -> Literal["completed", "failed", "cancelled"]: ...

    def to_json(self) -> JsonObject: ...


OwnershipObservationProjection: TypeAlias = (
    OwnershipStateProjection | OwnershipRequestProjection | OwnershipOutcomeProjection
)


class RegionObservationContextProjection(Protocol):
    @property
    def scope(self) -> EventObservationScope: ...


class RegionObservationProjection(Protocol):
    @property
    def context(self) -> RegionObservationContextProjection: ...

    def record_key(self) -> tuple[str, str]: ...

    def to_json(self) -> JsonObject: ...


class CoordinationProjection(Protocol):
    @property
    def kind(self) -> Literal["synchronization", "save", "restore"]: ...

    @property
    def operation_id(self) -> str: ...

    @property
    def session_id(self) -> str: ...

    @property
    def identity_namespace(self) -> str: ...

    @property
    def federation_id(self) -> str: ...

    @property
    def scope(self) -> Literal["local", "federation"]: ...

    @property
    def subject(self) -> ParticipantIdentityProjection | None: ...

    def to_json(self) -> JsonObject: ...


class InteractionFlowProjection(Protocol):
    @property
    def kind(self) -> Literal["send", "receive"]: ...

    @property
    def correlation_key(
        self,
    ) -> tuple[str, str, str, tuple[str, str], tuple[str, str]] | None: ...

    def to_json(self) -> JsonObject: ...


ReportedParticipant: TypeAlias = str | Mapping[str, JsonValue] | None
RawReportedParticipant: TypeAlias = str | Mapping[str, object] | None
ReportedParticipantCandidate: TypeAlias = RawReportedParticipant | int | float


class RawReportedParticipantMetadata(TypedDict, total=False):
    """Recognized labels inside a provider's legacy participant record."""

    name: object
    id: object
    handle: object


class RawObservationSession(TypedDict, total=False):
    """Session field read from a raw time or declaration observation."""

    session_id: object


class RawParticipantIdentity(TypedDict, total=False):
    """Candidate identity values before their identifiers are validated."""

    participant_id: object
    membership_id: object
    display_name: object
    handle: object


class RawInteractionFlowObservation(TypedDict, total=False):
    """Provider-owned interaction-flow fields before runtime validation."""

    kind: object
    order: object
    message_id: object
    identity_namespace: object
    federation_id: object
    session_id: object
    producer: RawParticipantIdentity
    recipient: RawParticipantIdentity
    class_key: object
    logical_time: object


class RawRelationshipObservation(TypedDict, total=False):
    """Provider-owned relationship fields before runtime validation."""

    relationship_id: object
    session_id: object
    identity_namespace: object
    federation_id: object
    producer: RawParticipantIdentity
    recipient: RawParticipantIdentity
    kind: object
    evidence: object
    status: object
    viewpoint: object
    class_key: object
    class_name: object
    reason: object


class RawParticipantAttribution(TypedDict, total=False):
    """Candidate attribution fields before nested identity validation."""

    session_id: object
    identity_namespace: object
    federation_id: object
    participant: RawParticipantIdentity


class ObserverEventMetadata(TypedDict, total=False):
    """Common event keys; family payloads and provider extensions are separate."""

    event_type: str
    kind: str
    family: str
    source: str
    connection: str
    federation: str
    federate: RawReportedParticipant
    participant: RawReportedParticipant
    federate_name: str
    participant_name: str
    object_key: str
    session_id: str
    participant_attribution: RawParticipantAttribution


# Raw provider objects are permitted only at ingress; they are validated and
# converted before storage as EventRecord.extensions.
RawObserverEvent: TypeAlias = Mapping[str, object]


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
    """Normalized event envelope, typed projections, and provider extensions."""

    sequence: int
    event_type: str
    family: str
    source: str
    observed_at: float
    extensions: JsonObject = field(default_factory=dict)
    fom_observation: FomObservation | None = None
    mom_observation: MomObservation | None = None
    declaration_observation: DeclarationProjection | None = None
    relationship_observation: RelationshipProjection | None = None
    coordination_observation: CoordinationProjection | None = None
    interaction_flow_observation: InteractionFlowProjection | None = None
    ownership_observation: OwnershipObservationProjection | None = None
    region_observation: RegionObservationProjection | None = None

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
    object_context: ObjectContext | None = None

    @property
    def cache_key(self) -> tuple[str | None, str | None, str]:
        context = self.object_context
        return (
            context.identity_namespace if context else None,
            context.federation_id if context else None,
            self.object_key,
        )

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
            **(
                {
                    "object_context": self.object_context.to_json(),
                    "observation_key": self.object_context.key(self.object_key),
                }
                if self.object_context
                else {}
            ),
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
    perspective: ParticipantPerspective | None = None

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
            **({"perspective": self.perspective.to_json()} if self.perspective else {}),
        }


@dataclass(slots=True)
class FederateInspection:
    federate: str
    role: str | None
    handle: str | None
    status: str
    source: str
    last_seen: float | None
    perspective: ParticipantPerspective | None = None

    def to_json(self) -> JsonObject:
        return {
            "federate": self.federate,
            "role": self.role,
            "handle": self.handle,
            "status": self.status,
            "source": self.source,
            "last_seen": self.last_seen,
            **({"perspective": self.perspective.to_json()} if self.perspective else {}),
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
