"""Pydantic schemas for the optional observer HTTP transport.

The core store intentionally has no web-framework dependency.  This module is
the opposite boundary: every request body and every finite JSON response that
FastAPI exposes has a named, inspectable model.  Provider-specific fields are
allowed only where the wire contract explicitly calls them extensions.
"""

from __future__ import annotations

import json
from collections.abc import Mapping, Sequence
from typing import TYPE_CHECKING, Annotated, ClassVar, Literal, TypeAlias, cast

from pydantic import (
    BaseModel,
    ConfigDict,
    Field,
    RootModel,
    SerializerFunctionWrapHandler,
    StringConstraints,
    TypeAdapter,
    field_validator,
    model_serializer,
    model_validator,
)
from typing_extensions import TypeAliasType

from .coordination import coordination_data
from .declarations import declaration_data
from .fom import fom_data
from .identities import decimal_bound, participant_reference
from .interaction_flow import flow_number, interaction_flow_data
from .mom import counter_number, mom_data
from .ownership import ownership_data
from .regions import region_data
from .relationships import relationship_data

JsonText = Annotated[str, StringConstraints(strip_whitespace=False)]

if TYPE_CHECKING:
    JsonValue: TypeAlias = None | bool | int | float | str | list["JsonValue"] | dict[str, "JsonValue"]
else:
    # A named runtime alias gives OpenAPI a recursive schema; the static alias
    # above also works with checkers that cannot resolve TypeAliasType recursion.
    JsonValue = TypeAliasType(
        "JsonValue",
        None | bool | int | float | JsonText | list["JsonValue"] | dict[JsonText, "JsonValue"],
    )
JsonObject = dict[str, JsonValue]
# Pyright cannot express the recursive TypeForm in the TypeAdapter constructor.
_JSON_VALUE_ADAPTER = TypeAdapter(JsonValue)  # pyright: ignore[reportArgumentType]
CapabilityState = Literal[
    "available",
    "partial",
    "disabled",
    "unsupported",
    "unknown",
]
NonEmptyText = Annotated[str, Field(min_length=1)]
EventTypeText = Annotated[str, Field(min_length=1, max_length=256)]
SequenceNumber = Annotated[int, Field(ge=1)]
NonNegativeInt = Annotated[int, Field(ge=0)]
PositiveInt = Annotated[int, Field(ge=1)]
NonNegativeFloat = Annotated[float, Field(ge=0)]

# Constructor defaults aid Python callers, not HTTP/file producers. Consumers
# need these actual wire collections, even when they are explicitly empty.
WIRE_REQUIRED_FIELDS: dict[str, set[str]] = {
    "ContractResponse": {"capabilities"},
    "StateResponse": {"context", "adapters", "capabilities", "federate_roster", "inspectors"},
    "InspectorsResponse": {"objects", "interactions"},
    "HealthResponse": {"capabilities"}, "MetricsResponse": {"adapters"},
    "EventPageResponse": {"events"}, "CommandCatalog": {"targets"}, "CommandHistory": {"records"},
}


class ObserverModel(BaseModel):
    """Strict, immutable Pydantic boundary model."""

    model_config = ConfigDict(
        extra="forbid",
        frozen=True,
        populate_by_name=True,
        strict=True,
        str_strip_whitespace=True,
    )


class WireObserverModel(ObserverModel):
    """Response model that preserves unknown wire fields at the top level."""

    model_config = ConfigDict(extra="allow")


class ExtensibleObserverModel(ObserverModel):
    """Strict model with one explicit provider-extension escape hatch."""

    _reject_batch_shape: ClassVar[bool] = False
    extensions: Annotated[Mapping[JsonText, JsonValue], Field(default_factory=dict)]

    @model_validator(mode="before")
    @classmethod
    def collect_extensions(cls, value: object) -> object:
        """Collect unknown wire fields into the named extension mapping."""

        if not isinstance(value, Mapping):
            return value
        if cls._reject_batch_shape and "events" in value:
            raise ValueError("an object containing 'events' must be an event batch")
        fields = set(cls.model_fields) - {"extensions"}
        raw_extensions = value.get("extensions", {})
        if not isinstance(raw_extensions, Mapping):
            raise TypeError("extensions must be a JSON object")
        extensions = {str(key): _JSON_VALUE_ADAPTER.validate_python(item) for key, item in raw_extensions.items()}
        fields_in_extensions = {
            str(key): _JSON_VALUE_ADAPTER.validate_python(item)
            for key, item in value.items()
            if key not in fields and key != "extensions"
        }
        extensions.update(fields_in_extensions)
        return {
            **{key: item for key, item in value.items() if key in fields},
            "extensions": extensions,
        }

    @model_serializer(mode="wrap")
    def flatten_extensions(self, handler: SerializerFunctionWrapHandler) -> object:
        """Keep the existing top-level wire shape while storing extensions explicitly."""

        serialized = cast(dict[str, object], handler(self))
        extensions = serialized.pop("extensions", {})
        if not isinstance(extensions, Mapping):  # pragma: no cover - validated above
            raise TypeError("extensions must serialize as a JSON object")
        return {**dict(extensions), **serialized}


TimeId = Annotated[str, StringConstraints(strip_whitespace=False, min_length=1, max_length=256, pattern=r"\S")]
TimeNumber = Annotated[int | float, Field(allow_inf_nan=False)]


class TimeRequestObservation(ObserverModel):
    kind: Literal["request"]
    logical_time: TimeNumber | None = None
    session_id: TimeId | None = None
    request_id: TimeId | None = None


class TimeGrantObservation(ObserverModel):
    kind: Literal["grant"]
    logical_time: TimeNumber | None = None
    session_id: TimeId | None = None
    request_id: TimeId | None = None


class TimeStateObservation(ObserverModel):
    kind: Literal["state"]
    session_id: TimeId | None = None
    logical_time: TimeNumber | None = None
    lookahead: TimeNumber | None = None
    regulating: bool | None = None
    constrained: bool | None = None
    galt: TimeNumber | None = None
    lbts: TimeNumber | None = None


class TimeOutcomeObservation(ObserverModel):
    kind: Literal["failed", "cancelled"]
    session_id: TimeId | None = None
    request_id: TimeId | None = None
    reason: JsonText | None = None


TimeObservationInput = Annotated[
    TimeRequestObservation | TimeGrantObservation | TimeStateObservation | TimeOutcomeObservation,
    Field(discriminator="kind"),
]


DeclarationId = Annotated[str, StringConstraints(strip_whitespace=False, min_length=1, max_length=512, pattern=r"\S")]


class DeclarationObservationBase(ObserverModel):
    direction: Literal["publication", "subscription"]
    status: Literal["declared", "withdrawn", "unknown"]
    class_key: DeclarationId
    class_name: DeclarationId | None = None
    session_id: TimeId | None = None
    reason: JsonText | None = None


class ObjectDeclarationObservation(DeclarationObservationBase):
    kind: Literal["object"]
    attribute_scope: Literal["selected", "all", "unknown"] = "unknown"
    attributes: Annotated[Sequence[DeclarationId] | None, Field(default=None, max_length=4096)]

    @model_validator(mode="after")
    def explicit_attribute_set(self) -> ObjectDeclarationObservation:
        declaration_data({"declaration": self.model_dump(mode="python", exclude_none=True)})
        return self


class InteractionDeclarationObservation(DeclarationObservationBase):
    kind: Literal["interaction"]


DeclarationObservationInput = Annotated[
    ObjectDeclarationObservation | InteractionDeclarationObservation, Field(discriminator="kind"),
]


class ParticipantIdentityResponse(ObserverModel):
    """Opaque IDs identify a participant join; names and handles never do."""

    participant_id: DeclarationId
    membership_id: DeclarationId
    display_name: DeclarationId | None = None
    handle: DeclarationId | None = None


class RelationshipObservationInput(ObserverModel):
    relationship_id: DeclarationId
    session_id: TimeId
    identity_namespace: DeclarationId
    federation_id: DeclarationId
    producer: ParticipantIdentityResponse
    recipient: ParticipantIdentityResponse
    kind: Literal["object", "interaction"]
    evidence: Literal["declared", "observed"]
    status: Literal["active", "ended", "unknown"]
    viewpoint: Literal["observer", "producer", "recipient", "unknown"]
    class_key: DeclarationId | None = None
    class_name: DeclarationId | None = None
    reason: Annotated[JsonText | None, Field(default=None, max_length=4096)]

    @model_validator(mode="after")
    def explicit_relationship(self) -> RelationshipObservationInput:
        relationship_data({"relationship": {
            name: value for name, value in self.model_dump(mode="python", exclude_none=True).items()
            if name in RelationshipObservationInput.model_fields
        }})
        return self


class InteractionFlowObservationInput(ObserverModel):
    kind: Literal["send", "receive"]
    order: Literal["receive-order", "timestamped", "unknown"]
    message_id: DeclarationId | None = None
    identity_namespace: DeclarationId | None = None
    federation_id: DeclarationId | None = None
    session_id: TimeId | None = None
    producer: ParticipantIdentityResponse | None = None
    recipient: ParticipantIdentityResponse | None = None
    class_key: DeclarationId | None = None
    logical_time: Annotated[int | float | None, Field(default=None, allow_inf_nan=False)]

    @field_validator("logical_time", mode="before")
    @classmethod
    def finite_logical_time(cls, value: object) -> int | float | None:
        # Catch oversized Python integers before Pydantic's float finiteness
        # check can raise an uncaught OverflowError at the HTTP boundary.
        return flow_number(value)

    @model_validator(mode="after")
    def explicit_flow(self) -> InteractionFlowObservationInput:
        interaction_flow_data({"interaction_flow": {name: value for name, value in self.model_dump(mode="python", exclude_none=True).items()
                                                   if name in InteractionFlowObservationInput.model_fields}})
        return self


class CoordinationParticipantResponse(ObserverModel):
    identity: ParticipantIdentityResponse
    status: Literal["pending", "achieved", "begun", "completed", "failed", "unknown"]
    reason: Annotated[JsonText | None, Field(default=None, max_length=4096)]


class CoordinationObservationBase(ObserverModel):
    operation_id: DeclarationId
    session_id: TimeId
    identity_namespace: DeclarationId
    federation_id: DeclarationId
    label: DeclarationId
    scope: Literal["local", "federation"]
    subject: ParticipantIdentityResponse | None = None
    participants: Annotated[Sequence[CoordinationParticipantResponse] | None, Field(default=None, max_length=4096)]
    reason: Annotated[JsonText | None, Field(default=None, max_length=4096)]

    @model_validator(mode="after")
    def explicit_coordination(self) -> CoordinationObservationBase:
        fields = set(CoordinationObservationBase.model_fields) | {"kind", "phase"}
        coordination_data({"coordination": {name: value for name, value in self.model_dump(mode="python", exclude_none=True).items() if name in fields}})
        return self


class SynchronizationObservation(CoordinationObservationBase):
    kind: Literal["synchronization"]
    phase: Literal["requested", "registered", "registration-failed", "announced", "achieved", "completed", "failed"]


class PersistenceObservation(CoordinationObservationBase):
    kind: Literal["save", "restore"]
    phase: Literal["requested", "begun", "completed", "failed"]


CoordinationObservationInput = Annotated[SynchronizationObservation | PersistenceObservation, Field(discriminator="kind")]


class OwnershipObservationBase(ObserverModel):
    session_id: TimeId
    identity_namespace: DeclarationId
    federation_id: DeclarationId
    object_id: DeclarationId
    attribute_key: DeclarationId
    reason: Annotated[JsonText | None, Field(default=None, max_length=4096)]

    def scope_key(self, source: str) -> tuple[str, str, str, str, str, str]:
        return source, self.session_id, self.identity_namespace, self.federation_id, self.object_id, self.attribute_key

    @model_validator(mode="after")
    def explicit_ownership(self) -> OwnershipObservationBase:
        fields = set(OwnershipObservationBase.model_fields) | {"kind", "owner_state", "owner", "transition", "request_id", "action", "requester", "outcome", "reported_owner_name", "reported_owner_handle"}
        ownership_data({"ownership": {name: value for name, value in self.model_dump(mode="python", exclude_none=True).items() if name in fields}})
        return self


class OwnershipStateObservation(OwnershipObservationBase):
    kind: Literal["state"]
    owner_state: Literal["known", "unowned", "rti", "unknown"]
    owner: ParticipantIdentityResponse | None = None
    transition: Literal["reported", "acquired", "divested"]
    reported_owner_name: DeclarationId | None = None
    reported_owner_handle: DeclarationId | None = None


class OwnershipRequestBase(OwnershipObservationBase):
    request_id: DeclarationId
    action: Literal["acquire", "divest"]
    requester: ParticipantIdentityResponse

    def request_key(self, source: str) -> tuple[tuple[str, str, str, str, str, str], str, str, str, str]:
        return self.scope_key(source), self.request_id, self.action, self.requester.participant_id, self.requester.membership_id


class OwnershipRequestObservation(OwnershipRequestBase):
    kind: Literal["request"]


class OwnershipOutcomeObservation(OwnershipRequestBase):
    kind: Literal["outcome"]
    outcome: Literal["completed", "failed", "cancelled"]


OwnershipObservationInput = Annotated[OwnershipStateObservation | OwnershipRequestObservation | OwnershipOutcomeObservation, Field(discriminator="kind")]


DecimalBound = Annotated[str, StringConstraints(strip_whitespace=False, min_length=1, max_length=128, pattern=r"^(0|[1-9][0-9]*)$"),
                         Field(json_schema_extra={"not": {"pattern": r"[^0-9]"}})]


class DimensionRangeResponse(ObserverModel):
    dimension_id: DeclarationId
    lower_bound: DecimalBound
    upper_bound: DecimalBound
    dimension_upper_bound: DecimalBound | None = None

    @field_validator("lower_bound", "upper_bound", "dimension_upper_bound", mode="before")
    @classmethod
    def canonical_decimal(cls, value: object) -> str | None:
        return decimal_bound(value, optional=True)

    @model_validator(mode="after")
    def ordered_bounds(self) -> DimensionRangeResponse:
        if int(self.lower_bound) > int(self.upper_bound) or self.dimension_upper_bound is not None and int(self.upper_bound) > int(self.dimension_upper_bound):
            raise ValueError("range bounds must be ordered and inside an explicitly reported dimension limit")
        return self


class RegionExtentResponse(ObserverModel):
    extent_id: DeclarationId
    dimensions: Annotated[Sequence[DimensionRangeResponse], Field(max_length=256)]


class RegionObservationBase(ObserverModel):
    session_id: TimeId
    identity_namespace: DeclarationId
    federation_id: DeclarationId
    evidence: Literal["declared", "observed", "unknown"]
    subject: ParticipantIdentityResponse | None = None
    reason: Annotated[JsonText | None, Field(default=None, max_length=4096)]

    @model_validator(mode="after")
    def explicit_region(self) -> RegionObservationBase:
        fields = set(RegionObservationBase.model_fields) | {"kind", "status", "region_id", "extents", "association_id", "routing_id", "target_kind", "target_id", "direction", "region_ids", "attribute_keys", "producer", "recipient"}
        region_data({"region_observation": {name: value for name, value in self.model_dump(mode="python", exclude_none=True).items() if name in fields}})
        return self


class RegionStateObservation(RegionObservationBase):
    kind: Literal["region"]
    region_id: DeclarationId
    status: Literal["active", "deleted", "unknown"]
    extents: Annotated[Sequence[RegionExtentResponse] | None, Field(default=None, max_length=256)]


class RegionAssociationObservation(RegionObservationBase):
    kind: Literal["association"]
    association_id: DeclarationId
    status: Literal["active", "ended", "unknown"]
    target_kind: Literal["object", "interaction"]
    target_id: DeclarationId
    direction: Literal["publication", "subscription", "update", "send", "unknown"]
    region_ids: Annotated[Sequence[DeclarationId] | None, Field(default=None, max_length=4096)]
    attribute_keys: Annotated[Sequence[DeclarationId] | None, Field(default=None, max_length=4096)]


class RegionRoutingObservation(RegionObservationBase):
    kind: Literal["routing"]
    routing_id: DeclarationId
    status: Literal["active", "ended", "unknown"]
    region_ids: Annotated[Sequence[DeclarationId] | None, Field(default=None, max_length=4096)]
    target_kind: Literal["object", "interaction"] | None = None
    target_id: DeclarationId | None = None
    producer: ParticipantIdentityResponse | None = None
    recipient: ParticipantIdentityResponse | None = None


RegionObservationInput = Annotated[RegionStateObservation | RegionAssociationObservation | RegionRoutingObservation, Field(discriminator="kind")]


class ParticipantReferenceResponse(ObserverModel):
    identity: ParticipantIdentityResponse | None = None
    display_name: DeclarationId | None = None
    handle: DeclarationId | None = None

    @model_validator(mode="after")
    def actual_reference_evidence(self) -> ParticipantReferenceResponse:
        participant_reference(self.model_dump(mode="python", exclude_none=True))
        return self


class MomMemberResponse(ObserverModel):
    participant: ParticipantReferenceResponse
    status: Literal["joined", "resigned", "unknown"]


class MomObservationBase(ObserverModel):
    session_id: TimeId
    identity_namespace: DeclarationId
    federation_id: DeclarationId
    report_id: DeclarationId
    reason: Annotated[JsonText | None, Field(default=None, max_length=4096)]

    @model_validator(mode="after")
    def explicit_management_report(self) -> MomObservationBase:
        fields = set(MomObservationBase.model_fields) | {"kind", "coverage", "members", "service_name", "status", "subject", "invocation_id", "error_name", "counter_key", "semantics", "value", "value_text", "unit"}
        mom_data({"mom_observation": {name: value for name, value in self.model_dump(mode="python", exclude_none=True).items() if name in fields}})
        return self


class MomMembersObservation(MomObservationBase):
    kind: Literal["members"]
    coverage: Literal["complete", "partial", "unknown"]
    members: Annotated[Sequence[MomMemberResponse] | None, Field(default=None, max_length=4096)]


class MomServiceObservation(MomObservationBase):
    kind: Literal["service"]
    service_name: DeclarationId
    status: Literal["invoked", "completed", "failed", "unknown"]
    subject: ParticipantReferenceResponse | None = None
    invocation_id: DeclarationId | None = None
    error_name: DeclarationId | None = None


class MomCounterObservation(MomObservationBase):
    kind: Literal["counter"]
    counter_key: DeclarationId
    semantics: Literal["cumulative", "gauge", "unknown"]
    value: Annotated[int | float | None, Field(default=None, allow_inf_nan=False)]
    value_text: Annotated[JsonText | None, Field(default=None, max_length=4096)]
    unit: DeclarationId | None = None
    subject: ParticipantReferenceResponse | None = None

    @field_validator("value", mode="before")
    @classmethod
    def finite_counter(cls, value: object) -> int | float | None:
        return counter_number(value)


MomObservationInput = Annotated[MomMembersObservation | MomServiceObservation | MomCounterObservation, Field(discriminator="kind")]


class FomFieldResponse(ObserverModel):
    field_id: DeclarationId
    name: DeclarationId | None = None
    datatype_id: DeclarationId | None = None
    description: Annotated[JsonText | None, Field(default=None, max_length=4096)]


class FomEnumeratorResponse(ObserverModel):
    name: DeclarationId
    value: DeclarationId


class FomObservationBase(ObserverModel):
    session_id: TimeId
    identity_namespace: DeclarationId
    federation_id: DeclarationId
    model_id: DeclarationId
    revision: DeclarationId
    element_id: DeclarationId
    name: DeclarationId | None = None
    status: Literal["present", "removed", "unknown"]
    description: Annotated[JsonText | None, Field(default=None, max_length=4096)]

    @model_validator(mode="after")
    def explicit_fom_metadata(self) -> FomObservationBase:
        fields = set(FomObservationBase.model_fields) | {"kind", "parent_state", "parent_id", "field_scope", "attributes", "parameters", "encoding", "representation", "fields", "enumerators", "element_type_id", "cardinality", "upper_bound", "unit"}
        fom_data({"fom_observation": {name: value for name, value in self.model_dump(mode="python", exclude_none=True).items() if name in fields}})
        return self


class FomClassObservationBase(FomObservationBase):
    parent_state: Literal["root", "parent", "unknown"]
    parent_id: DeclarationId | None = None
    field_scope: Literal["declared", "effective", "unknown"]


class FomObjectObservation(FomClassObservationBase):
    kind: Literal["object-class"]
    attributes: Annotated[Sequence[FomFieldResponse] | None, Field(default=None, max_length=4096)]


class FomInteractionObservation(FomClassObservationBase):
    kind: Literal["interaction-class"]
    parameters: Annotated[Sequence[FomFieldResponse] | None, Field(default=None, max_length=4096)]


class FomDatatypeObservation(FomObservationBase):
    kind: Literal["datatype"]
    encoding: Literal["primitive", "enumeration", "array", "record", "variant", "opaque", "unknown"]
    representation: Annotated[JsonText | None, Field(default=None, max_length=4096)]
    fields: Annotated[Sequence[FomFieldResponse] | None, Field(default=None, max_length=4096)]
    enumerators: Annotated[Sequence[FomEnumeratorResponse] | None, Field(default=None, max_length=4096)]
    element_type_id: DeclarationId | None = None
    cardinality: DecimalBound | None = None

    @field_validator("cardinality", mode="before")
    @classmethod
    def canonical_cardinality(cls, value: object) -> str | None:
        return decimal_bound(value, optional=True)


class FomDimensionObservation(FomObservationBase):
    kind: Literal["dimension"]
    upper_bound: DecimalBound | None = None
    unit: DeclarationId | None = None

    @field_validator("upper_bound", mode="before")
    @classmethod
    def canonical_bound(cls, value: object) -> str | None:
        return decimal_bound(value, optional=True)


FomObservationInput = Annotated[FomObjectObservation | FomInteractionObservation | FomDatatypeObservation | FomDimensionObservation, Field(discriminator="kind")]


class EventInput(ExtensibleObserverModel):
    """Normalized event input plus JSON-compatible provider extensions."""

    sequence: Annotated[int | None, Field(default=None, ge=1)]
    event_type: Annotated[EventTypeText | None, Field(default=None)]
    kind: Annotated[EventTypeText | None, Field(default=None)]
    family: Annotated[NonEmptyText | None, Field(default=None)]
    source: Annotated[str | None, Field(default=None, min_length=1, max_length=512)]
    observed_at: float | None = None
    connection: str | None = None
    federation: str | None = None
    federate: str | dict[str, JsonValue] | None = None
    participant: str | dict[str, JsonValue] | None = None
    federate_name: str | None = None
    participant_name: str | None = None
    federate_handle: str | None = None
    participant_handle: str | None = None
    role: str | None = None
    object_key: str | None = None
    object_name: str | None = None
    object_handle: str | None = None
    class_name: str | None = None
    class_handle: str | None = None
    attributes: Annotated[Mapping[JsonText, JsonValue] | None, Field(default=None)]
    interaction_key: str | None = None
    interaction_class: str | None = None
    interaction_handle: str | None = None
    parameters: Annotated[Mapping[JsonText, JsonValue] | None, Field(default=None)]
    tag: Annotated[JsonValue, Field(default=None)]
    details: Annotated[Mapping[JsonText, JsonValue] | None, Field(default=None)]
    logical_time: int | float | None = None
    operation: str | None = None
    time_observation: TimeObservationInput | None = None
    declaration: DeclarationObservationInput | None = None
    relationship: RelationshipObservationInput | None = None
    coordination: CoordinationObservationInput | None = None
    interaction_flow: InteractionFlowObservationInput | None = None
    ownership: OwnershipObservationInput | None = None
    region_observation: RegionObservationInput | None = None
    mom_observation: MomObservationInput | None = None
    fom_observation: FomObservationInput | None = None

    _reject_batch_shape: ClassVar[bool] = True

    @model_validator(mode="after")
    def consistent_interaction_direction(self) -> EventInput:
        if self.interaction_flow is not None:
            interaction_flow_data({"event_type": self.event_type or self.kind,
                                   "interaction_flow": self.interaction_flow.model_dump(mode="python", exclude_none=True)})
        return self


class EventBatchRequest(BaseModel):
    """Atomic event batch request.

    Extra keys are forbidden so an event object containing an ``events`` field
    cannot be silently interpreted as a batch.  Single events remain
    extensible through :class:`EventInput`.
    """

    model_config = ConfigDict(extra="forbid", frozen=True, strict=True)

    events: Annotated[Sequence[EventInput], Field(min_length=1, max_length=10_000)]


EventIngestRequest = Annotated[
    EventBatchRequest | EventInput,
    Field(description="One event object or an atomic events batch."),
]


class ContextRequest(RootModel[dict[str, JsonValue]]):
    """JSON object merged into the observer context."""

    model_config = ConfigDict(frozen=True, strict=True)


class StatusRequest(ExtensibleObserverModel):
    status: Annotated[str, Field(min_length=1, max_length=128)]
    context: Annotated[Mapping[str, JsonValue], Field(default_factory=dict)]


class CapabilityResponse(WireObserverModel):
    state: CapabilityState
    label: NonEmptyText
    description: NonEmptyText
    owner: NonEmptyText
    source: str | None = None
    owners: Annotated[Sequence[str], Field(default_factory=tuple)]
    sources: Annotated[Sequence[str], Field(default_factory=tuple)]
    observations: Annotated[Sequence[Mapping[str, JsonValue]], Field(default_factory=tuple)]
    reason: str | None = None
    reasons: Annotated[Sequence[str], Field(default_factory=tuple)]
    event_types: Annotated[Sequence[str], Field(default_factory=tuple)]


class ObjectInspectionResponse(ObserverModel):
    object_key: NonEmptyText
    object_name: NonEmptyText
    object_handle: str | None = None
    class_name: str | None = None
    class_handle: str | None = None
    family: NonEmptyText
    attributes: Annotated[Mapping[JsonText, JsonValue], Field(default_factory=dict)]
    update_count: NonNegativeInt
    discovery_count: NonNegativeInt
    sources: Annotated[Sequence[str], Field(default_factory=tuple)]
    status: NonEmptyText
    last_seen: float | None = None


class InteractionInspectionResponse(ObserverModel):
    sequence: SequenceNumber
    interaction_key: NonEmptyText
    interaction_class: NonEmptyText
    class_handle: str | None = None
    family: NonEmptyText
    source: NonEmptyText
    federate: str | None = None
    parameters: Annotated[Mapping[JsonText, JsonValue], Field(default_factory=dict)]
    tag: Annotated[JsonValue, Field(default=None)]
    observed_at: float


class FederateInspectionResponse(ObserverModel):
    federate: NonEmptyText
    role: str | None = None
    handle: str | None = None
    status: NonEmptyText
    source: NonEmptyText
    last_seen: float | None = None


class LiveMetricsResponse(ObserverModel):
    event_count: NonNegativeInt
    event_rate_per_second: NonNegativeFloat
    error_count: NonNegativeInt
    event_types: Annotated[Mapping[str, NonNegativeInt], Field(default_factory=dict)]
    families: Annotated[Mapping[str, NonNegativeInt], Field(default_factory=dict)]
    sources: Annotated[Mapping[str, NonNegativeInt], Field(default_factory=dict)]
    first_sequence: NonNegativeInt
    last_sequence: NonNegativeInt
    object_count: Annotated[NonNegativeInt, Field(default=0)]
    interaction_count: Annotated[NonNegativeInt, Field(default=0)]
    federate_count: Annotated[NonNegativeInt, Field(default=0)]


class RetentionResponse(ObserverModel):
    max_events: PositiveInt
    retained_events: NonNegativeInt
    dropped_events: NonNegativeInt
    cleared_events: NonNegativeInt
    first_retained_sequence: NonNegativeInt
    last_sequence: NonNegativeInt
    next_sequence: PositiveInt


class RetentionGapResponse(ObserverModel):
    first_retained_sequence: NonNegativeInt
    last_sequence: NonNegativeInt
    retained_events: NonNegativeInt
    dropped_events: NonNegativeInt
    cleared_events: NonNegativeInt
    next_sequence: PositiveInt | None = None


class AdapterResponse(ObserverModel):
    name: NonEmptyText
    status: NonEmptyText
    capabilities: Annotated[Sequence[str], Field(default_factory=tuple)]


class EventEnvelope(WireObserverModel):
    sequence: SequenceNumber
    event_type: NonEmptyText
    family: NonEmptyText
    source: NonEmptyText
    observed_at: float
    federate: str | dict[str, JsonValue] | None = None
    participant: str | dict[str, JsonValue] | None = None
    federate_name: str | None = None
    participant_name: str | None = None
    logical_time: Annotated[int | float | None, Field(default=None, allow_inf_nan=False)]
    operation: str | None = None
    details: Mapping[str, JsonValue] | None = None
    object_key: str | None = None
    time_observation: TimeObservationInput | None = None
    declaration: DeclarationObservationInput | None = None
    relationship: RelationshipObservationInput | None = None
    coordination: CoordinationObservationInput | None = None
    interaction_flow: InteractionFlowObservationInput | None = None
    ownership: OwnershipObservationInput | None = None
    region_observation: RegionObservationInput | None = None
    mom_observation: MomObservationInput | None = None
    fom_observation: FomObservationInput | None = None

    @model_validator(mode="after")
    def consistent_interaction_direction(self) -> EventEnvelope:
        if self.interaction_flow is not None:
            interaction_flow_data({"event_type": self.event_type,
                                   "interaction_flow": self.interaction_flow.model_dump(mode="python", exclude_none=True)})
        return self


class ObjectPerspectiveResponse(ObserverModel):
    """Null federate is global; a name is explicit adapter attribution only."""

    federate: NonEmptyText | None
    generation: SequenceNumber
    last_sequence: SequenceNumber
    object: ObjectInspectionResponse

    @model_serializer(mode="wrap")
    def preserve_scope(self, handler: SerializerFunctionWrapHandler):
        # No return annotation: preserve the named model's serialization schema.
        # State/stream transports omit optional nulls, but this null is required
        # and means global scope, not missing attribution support.
        serialized = cast(dict[str, object], handler(self))
        serialized["federate"] = self.federate
        return serialized


class ObjectAttributeChangeResponse(ObserverModel):
    sequence: SequenceNumber
    object_key: NonEmptyText
    federate: NonEmptyText | None
    generation: SequenceNumber
    attribute: JsonText
    value: JsonValue
    previous_known: bool
    previous_value: JsonValue
    source: NonEmptyText
    observed_at: Annotated[float, Field(allow_inf_nan=False)]

    @model_serializer(mode="wrap")
    def preserve_null_values(self, handler: SerializerFunctionWrapHandler):
        # Keep required nullable fields even under parent exclude_none=True.
        # Leave the return unannotated so JSON Schema still describes this DTO.
        serialized = cast(dict[str, object], handler(self))
        serialized["federate"] = self.federate
        if self.value is None:
            serialized["value"] = None
        if self.previous_value is None:
            serialized["previous_value"] = None
        return serialized


class ObjectObservationsResponse(ObserverModel):
    snapshots: Sequence[ObjectPerspectiveResponse]
    changes: Sequence[ObjectAttributeChangeResponse]
    history_truncated: bool
    history_first_sequence: NonNegativeInt
    history_last_sequence: NonNegativeInt
    snapshot_capacity: PositiveInt | None = None
    dropped_snapshots: NonNegativeInt | None = None
    omitted_changes: NonNegativeInt | None = None
    dropped_history_batches: NonNegativeInt | None = None

    @model_validator(mode="after")
    def consistent_snapshot_budget(self) -> ObjectObservationsResponse:
        metadata = self.snapshot_capacity, self.dropped_snapshots, self.omitted_changes
        if any(value is not None for value in metadata) and any(value is None for value in metadata):
            raise ValueError("object snapshot budget metadata must be complete or unreported")
        if self.snapshot_capacity is not None and len(self.snapshots) > self.snapshot_capacity:
            raise ValueError("object perspectives exceed their advertised snapshot capacity")
        keys = {(row.federate, row.object.object_key) for row in self.snapshots}
        if len(keys) != len(self.snapshots):
            raise ValueError("object perspective keys must be unique")
        if (self.omitted_changes or self.dropped_history_batches) and not self.history_truncated:
            raise ValueError("omitted object changes require an incomplete-history notice")
        return self


class TimeScopeResponse(ObserverModel):
    source: NonEmptyText
    session_id: TimeId | None
    federation: str | None
    federate: str | None

    @model_serializer(mode="wrap")
    def preserve_scope(self, handler: SerializerFunctionWrapHandler):
        serialized = cast(dict[str, object], handler(self))
        serialized.update(session_id=self.session_id, federation=self.federation, federate=self.federate)
        return serialized


class TimeSampleResponse(ObserverModel):
    sequence: SequenceNumber
    logical_time: TimeNumber | None = None
    observed_at: Annotated[float, Field(allow_inf_nan=False)]
    request_id: TimeId | None = None


class ReportedTimeStateResponse(ObserverModel):
    logical_time: TimeNumber | None = None
    lookahead: TimeNumber | None = None
    regulating: bool | None = None
    constrained: bool | None = None
    galt: TimeNumber | None = None
    lbts: TimeNumber | None = None


class TimeSnapshotResponse(ObserverModel):
    scope: TimeScopeResponse
    last_sequence: SequenceNumber
    observed_at: Annotated[float, Field(allow_inf_nan=False)]
    status: Literal["observed", "ended"]
    last_grant: TimeSampleResponse | None = None
    last_request: TimeSampleResponse | None = None
    reported_state: ReportedTimeStateResponse | None = None
    state_sequence: SequenceNumber | None = None
    uncorrelated_requests: NonNegativeInt
    uncorrelated_grants: NonNegativeInt


class TimeRequestResponse(ObserverModel):
    scope: TimeScopeResponse
    request_id: TimeId
    snapshot_retained: bool | None = None
    request_sequence: SequenceNumber
    requested_time: TimeNumber | None = None
    requested_at: Annotated[float, Field(allow_inf_nan=False)]
    status: Literal["pending", "granted", "failed", "cancelled", "ambiguous", "session-ended"]
    outcome_sequence: SequenceNumber | None = None
    outcome_at: Annotated[float | None, Field(default=None, allow_inf_nan=False)]
    granted_time: TimeNumber | None = None
    latency_seconds: Annotated[float | None, Field(default=None, ge=0, allow_inf_nan=False)]
    pending_seconds: Annotated[float | None, Field(default=None, ge=0, allow_inf_nan=False)]
    reason: JsonText | None = None


class TimeInspectionResponse(ObserverModel):
    snapshots: Sequence[TimeSnapshotResponse]
    requests: Sequence[TimeRequestResponse]
    request_capacity: PositiveInt
    dropped_requests: NonNegativeInt
    snapshot_capacity: PositiveInt | None = None
    dropped_snapshots: NonNegativeInt | None = None
    duration_basis: Literal["observer-ingress-monotonic"]
    scope_basis: Literal["explicit-observations"]

    @model_validator(mode="after")
    def consistent_inspection(self) -> TimeInspectionResponse:
        def key(scope: TimeScopeResponse) -> tuple[str, str | None, str | None, str | None]:
            return scope.source, scope.session_id, scope.federation, scope.federate

        snapshots = {key(row.scope): row for row in self.snapshots}
        if (self.snapshot_capacity is None) != (self.dropped_snapshots is None):
            raise ValueError("time snapshot capacity/loss must be reported together")
        if self.snapshot_capacity is not None and len(snapshots) > self.snapshot_capacity:
            raise ValueError("time snapshots exceed their independent capacity")
        if len(snapshots) != len(self.snapshots) or len(self.requests) > self.request_capacity:
            raise ValueError("time scopes must be unique and request capacity must be respected")
        requests = {(key(row.scope), row.request_id) for row in self.requests}
        if len(requests) != len(self.requests):
            raise ValueError("duplicate scoped time request ID")
        for snapshot in self.snapshots:
            if any(sample.sequence > snapshot.last_sequence for sample in (snapshot.last_grant, snapshot.last_request) if sample):
                raise ValueError("time samples cannot be newer than their snapshot")
            if snapshot.state_sequence is not None and snapshot.state_sequence > snapshot.last_sequence:
                raise ValueError("reported time state cannot be newer than its snapshot")
        for row in self.requests:
            snapshot = snapshots.get(key(row.scope))
            if row.scope.source == "unknown" or row.scope.session_id is None or row.scope.federate is None:
                raise ValueError("correlated requests require a known source, session and federate snapshot")
            if row.snapshot_retained is False:
                if snapshot is not None or not self.dropped_snapshots:
                    raise ValueError("a missing time snapshot requires explicit independent expiry coverage")
            elif snapshot is None:
                raise ValueError("requests without explicit snapshot expiry require their snapshot")
            if snapshot is not None and row.request_sequence > snapshot.last_sequence:
                raise ValueError("request is newer than its snapshot")
            if row.status == "pending":
                if any(value is not None for value in (row.outcome_sequence, row.outcome_at, row.granted_time, row.latency_seconds)):
                    raise ValueError("pending requests cannot claim an outcome")
            else:
                if (row.outcome_sequence is None or not row.request_sequence < row.outcome_sequence
                        or snapshot is not None and row.outcome_sequence > snapshot.last_sequence
                        or row.outcome_at is None or row.pending_seconds is not None):
                    raise ValueError("finished requests require ordered outcomes and no pending duration")
                if row.status != "granted" and (row.granted_time is not None or row.latency_seconds is not None):
                    raise ValueError("only matched grants can claim granted time or latency")
        return self


class DeclarationScopeResponse(ObserverModel):
    source: NonEmptyText
    session_id: TimeId | None
    federation: str | None
    federate: str | None

    @model_serializer(mode="wrap")
    def preserve_scope(self, handler: SerializerFunctionWrapHandler):
        serialized = cast(dict[str, object], handler(self))
        serialized.update(session_id=self.session_id, federation=self.federation, federate=self.federate)
        return serialized


class DeclarationSnapshotResponse(ObserverModel):
    scope: DeclarationScopeResponse
    kind: Literal["object", "interaction"]
    direction: Literal["publication", "subscription"]
    class_key: DeclarationId
    class_name: DeclarationId | None = None
    status: Literal["declared", "withdrawn", "unknown"]
    attribute_scope: Literal["selected", "all", "unknown", "not-applicable"]
    attributes: Annotated[Sequence[DeclarationId] | None, Field(default=None, max_length=4096)]
    reason: JsonText | None = None
    sequence: SequenceNumber
    observed_at: Annotated[float, Field(allow_inf_nan=False)]

    @model_validator(mode="after")
    def consistent_attribute_scope(self) -> DeclarationSnapshotResponse:
        selected = self.attribute_scope == "selected"
        if selected != (self.attributes is not None):
            raise ValueError("only a selected scope has an explicit attribute array")
        if (self.kind == "interaction") != (self.attribute_scope == "not-applicable"):
            raise ValueError("interaction attributes are not applicable, object attributes are not implicit")
        if self.attributes is not None and len(set(self.attributes)) != len(self.attributes):
            raise ValueError("selected attributes must be unique")
        return self


class DeclarationsResponse(ObserverModel):
    snapshots: Sequence[DeclarationSnapshotResponse]
    capacity: PositiveInt
    dropped_declarations: NonNegativeInt
    basis: Literal["last-reported-declarations"]

    @model_validator(mode="after")
    def unique_bounded_snapshots(self) -> DeclarationsResponse:
        keys = {(row.scope.source, row.scope.session_id, row.scope.federation, row.scope.federate,
                 row.kind, row.direction, row.class_key) for row in self.snapshots}
        if len(keys) != len(self.snapshots) or len(keys) > self.capacity:
            raise ValueError("declaration snapshots must be unique and bounded by capacity")
        return self


class RelationshipSnapshotResponse(RelationshipObservationInput):
    source: NonEmptyText
    sequence: SequenceNumber
    observed_at: Annotated[float, Field(allow_inf_nan=False)]


class RelationshipsResponse(ObserverModel):
    snapshots: Sequence[RelationshipSnapshotResponse]
    capacity: PositiveInt
    dropped_relationships: NonNegativeInt
    basis: Literal["last-reported-explicit-relationships"]

    @model_validator(mode="after")
    def unique_bounded_relationships(self) -> RelationshipsResponse:
        keys = {(row.source, row.session_id, row.identity_namespace, row.federation_id, row.relationship_id)
                for row in self.snapshots}
        if len(keys) != len(self.snapshots) or len(keys) > self.capacity:
            raise ValueError("relationship snapshots must be unique and bounded by capacity")
        return self


class CoordinationSnapshotResponse(CoordinationObservationBase):
    kind: Literal["synchronization", "save", "restore"]
    phase: Literal["requested", "registered", "registration-failed", "announced", "achieved", "begun", "completed", "failed"]
    source: NonEmptyText
    sequence: SequenceNumber
    observed_at: Annotated[float, Field(allow_inf_nan=False)]

    def identity_key(self) -> tuple[str, str, str, str, str, str, str, tuple[str, str] | None]:
        subject = (self.subject.participant_id, self.subject.membership_id) if self.subject else None
        return self.source, self.session_id, self.identity_namespace, self.federation_id, self.kind, self.operation_id, self.scope, subject


class CoordinationResponse(ObserverModel):
    snapshots: Sequence[CoordinationSnapshotResponse]
    transitions: Sequence[CoordinationSnapshotResponse]
    capacity: PositiveInt
    history_capacity: PositiveInt
    dropped_operations: NonNegativeInt
    dropped_transitions: NonNegativeInt
    basis: Literal["explicit-coordination-observations"]

    @model_validator(mode="after")
    def consistent_coordination(self) -> CoordinationResponse:
        latest = {row.identity_key(): row for row in self.snapshots}
        sequences = [row.sequence for row in self.transitions]
        if len(latest) != len(self.snapshots) or len(latest) > self.capacity or len(sequences) > self.history_capacity:
            raise ValueError("coordination snapshots and transitions must be unique and capacity bounded")
        if sequences != sorted(set(sequences)):
            raise ValueError("coordination transition sequences must be unique and increasing")
        for row in self.transitions:
            snapshot = latest.get(row.identity_key())
            if snapshot is not None and (snapshot.sequence < row.sequence or snapshot.sequence == row.sequence and snapshot != row):
                raise ValueError("coordination snapshots must agree with the latest retained report")
        return self


class InteractionFlowReportResponse(InteractionFlowObservationInput):
    source: NonEmptyText
    sequence: SequenceNumber
    observed_at: Annotated[float, Field(allow_inf_nan=False)]

    def correlation_key(self) -> tuple[str, str, str, tuple[str, str], tuple[str, str]] | None:
        if self.message_id is None or self.identity_namespace is None or self.federation_id is None or self.producer is None or self.recipient is None:
            return None
        return (self.identity_namespace, self.federation_id, self.message_id,
                (self.producer.participant_id, self.producer.membership_id), (self.recipient.participant_id, self.recipient.membership_id))


class InteractionFlowRecordResponse(ObserverModel):
    record_id: SequenceNumber
    last_sequence: SequenceNumber
    status: Literal["uncorrelated", "send-only", "receive-only", "paired", "ambiguous"]
    send: InteractionFlowReportResponse | None = None
    receive: InteractionFlowReportResponse | None = None
    send_count: NonNegativeInt
    receive_count: NonNegativeInt
    arrival_delay_seconds: Annotated[float | None, Field(default=None, ge=0, allow_inf_nan=False)]

    @model_validator(mode="after")
    def consistent_pair(self) -> InteractionFlowRecordResponse:
        sample = self.send or self.receive
        if sample is None or self.record_id != min(row.sequence for row in (self.send, self.receive) if row):
            raise ValueError("an interaction record starts at its first actual observation")
        if self.send_count < int(self.send is not None) or self.receive_count < int(self.receive is not None):
            raise ValueError("interaction report counts cannot omit their stored evidence")
        if (self.send is None) != (self.send_count == 0) or (self.receive is None) != (self.receive_count == 0):
            raise ValueError("interaction report counts require corresponding evidence")
        if self.send and self.send.kind != "send" or self.receive and self.receive.kind != "receive":
            raise ValueError("interaction report direction disagrees with its record slot")
        reports = [row for row in (self.send, self.receive) if row]
        if len({row.sequence for row in reports}) != len(reports):
            raise ValueError("a single observation cannot prove both send and receive")
        if self.last_sequence < max(row.sequence for row in reports):
            raise ValueError("interaction record cursor precedes its evidence")
        if self.send_count <= 1 and self.receive_count <= 1 and self.last_sequence != max(row.sequence for row in reports):
            raise ValueError("a record without duplicates ends at its last actual observation")
        key = sample.correlation_key()
        if any(row.correlation_key() != key for row in reports):
            raise ValueError("interaction endpoints and transmission identity must agree exactly")
        expected = "uncorrelated" if key is None else "ambiguous" if self.send_count > 1 or self.receive_count > 1 else "paired" if self.send and self.receive else "send-only" if self.send else "receive-only"
        if self.status != expected:
            raise ValueError("interaction status must follow actual reports, not guessed delivery")
        if key is None and self.send_count + self.receive_count != 1:
            raise ValueError("unidentified interaction observations cannot be merged")
        if self.arrival_delay_seconds is not None and (self.status != "paired" or self.send is None or self.receive is None or self.receive.sequence <= self.send.sequence):
            raise ValueError("arrival delay requires an unambiguous send-before-receive pair")
        return self


class InteractionFlowResponse(ObserverModel):
    records: Sequence[InteractionFlowRecordResponse]
    capacity: PositiveInt
    dropped_records: NonNegativeInt
    send_observations: NonNegativeInt
    receive_observations: NonNegativeInt
    basis: Literal["explicit-interaction-observations"]

    @model_validator(mode="after")
    def consistent_flow(self) -> InteractionFlowResponse:
        ids = {row.record_id for row in self.records}
        samples = [sample for row in self.records if (sample := row.send or row.receive) is not None]
        keys = [sample.correlation_key() for sample in samples]
        known = [key for key in keys if key is not None]
        sequences = [report.sequence for row in self.records for report in (row.send, row.receive) if report]
        if len(ids) != len(self.records) or len(ids) > self.capacity or len(set(known)) != len(known):
            raise ValueError("interaction flow records must be unique and capacity bounded")
        if len(set(sequences)) != len(sequences):
            raise ValueError("interaction observation evidence cannot appear in multiple records")
        if sum(row.send_count for row in self.records) > self.send_observations or sum(row.receive_count for row in self.records) > self.receive_observations:
            raise ValueError("interaction totals cannot omit retained report counts")
        if self.dropped_records == 0 and (sum(row.send_count for row in self.records) != self.send_observations or sum(row.receive_count for row in self.records) != self.receive_observations):
            raise ValueError("without index loss, totals equal retained observation counts")
        return self


class OwnershipStateReportResponse(OwnershipStateObservation):
    source: NonEmptyText
    sequence: SequenceNumber
    observed_at: Annotated[float, Field(allow_inf_nan=False)]


class OwnershipRequestReportResponse(OwnershipRequestObservation):
    source: NonEmptyText
    sequence: SequenceNumber
    observed_at: Annotated[float, Field(allow_inf_nan=False)]


class OwnershipOutcomeReportResponse(OwnershipOutcomeObservation):
    source: NonEmptyText
    sequence: SequenceNumber
    observed_at: Annotated[float, Field(allow_inf_nan=False)]


OwnershipReportResponse = Annotated[OwnershipStateReportResponse | OwnershipRequestReportResponse | OwnershipOutcomeReportResponse, Field(discriminator="kind")]


class OwnershipTicketResponse(ObserverModel):
    request: OwnershipRequestReportResponse
    status: Literal["pending", "completed", "failed", "cancelled", "ambiguous"]
    last_sequence: SequenceNumber
    outcome: OwnershipOutcomeReportResponse | None = None
    duplicate_requests: NonNegativeInt

    @model_validator(mode="after")
    def consistent_ticket(self) -> OwnershipTicketResponse:
        if self.status == "pending":
            valid = self.outcome is None and self.duplicate_requests == 0 and self.last_sequence == self.request.sequence
        elif self.status == "ambiguous":
            valid = self.outcome is None and self.duplicate_requests > 0 and self.last_sequence > self.request.sequence
        else:
            valid = (self.duplicate_requests == 0 and self.outcome is not None and self.outcome.outcome == self.status
                     and self.outcome.request_key(self.outcome.source) == self.request.request_key(self.request.source)
                     and self.last_sequence == self.outcome.sequence and self.outcome.sequence > self.request.sequence)
        if not valid:
            raise ValueError("ownership request outcomes require exact scoped tickets; ambiguity never establishes completion")
        return self


class OwnershipResponse(ObserverModel):
    states: Sequence[OwnershipStateReportResponse]
    requests: Sequence[OwnershipTicketResponse]
    transitions: Sequence[OwnershipReportResponse]
    capacity: PositiveInt
    request_capacity: PositiveInt
    history_capacity: PositiveInt
    dropped_states: NonNegativeInt
    dropped_requests: NonNegativeInt
    dropped_transitions: NonNegativeInt
    unmatched_outcomes: NonNegativeInt
    basis: Literal["explicit-ownership-observations"]

    @model_validator(mode="after")
    def consistent_ownership(self) -> OwnershipResponse:
        states = {row.scope_key(row.source): row for row in self.states}
        requests = {row.request.request_key(row.request.source): row for row in self.requests}
        sequences = [row.sequence for row in self.transitions]
        if (len(states) != len(self.states) or len(states) > self.capacity or len(requests) != len(self.requests)
                or len(requests) > self.request_capacity or len(sequences) > self.history_capacity or sequences != sorted(set(sequences))):
            raise ValueError("ownership scopes, tickets and transition sequences must be unique and capacity bounded")
        for row in self.transitions:
            if (isinstance(row, OwnershipStateReportResponse) and (current := states.get(row.scope_key(row.source))) is not None
                    and (current.sequence < row.sequence or current.sequence == row.sequence and current != row)):
                raise ValueError("ownership snapshots must agree with their latest retained report")
        return self


class RegionStateReportResponse(RegionStateObservation):
    source: NonEmptyText
    sequence: SequenceNumber
    observed_at: Annotated[float, Field(allow_inf_nan=False)]


class RegionAssociationReportResponse(RegionAssociationObservation):
    source: NonEmptyText
    sequence: SequenceNumber
    observed_at: Annotated[float, Field(allow_inf_nan=False)]


class RegionRoutingReportResponse(RegionRoutingObservation):
    source: NonEmptyText
    sequence: SequenceNumber
    observed_at: Annotated[float, Field(allow_inf_nan=False)]


RegionReportResponse = Annotated[RegionStateReportResponse | RegionAssociationReportResponse | RegionRoutingReportResponse, Field(discriminator="kind")]


def region_report_key(row: RegionReportResponse) -> tuple[str, str, str, str, str, str]:
    identity = row.region_id if row.kind == "region" else row.association_id if row.kind == "association" else row.routing_id
    return row.source, row.session_id, row.identity_namespace, row.federation_id, row.kind, identity


class RegionsResponse(ObserverModel):
    snapshots: Sequence[RegionReportResponse]
    transitions: Sequence[RegionReportResponse]
    capacity: PositiveInt
    history_capacity: PositiveInt
    dropped_snapshots: NonNegativeInt
    dropped_transitions: NonNegativeInt
    basis: Literal["explicit-region-observations"]

    @model_validator(mode="after")
    def consistent_regions(self) -> RegionsResponse:
        snapshots = {region_report_key(row): row for row in self.snapshots}
        sequences = [row.sequence for row in self.transitions]
        if len(snapshots) != len(self.snapshots) or len(snapshots) > self.capacity or len(sequences) > self.history_capacity or sequences != sorted(set(sequences)):
            raise ValueError("region snapshots and arrival sequences must be unique and capacity bounded")
        for row in self.transitions:
            current = snapshots.get(region_report_key(row))
            if current is not None and (current.sequence < row.sequence or current.sequence == row.sequence and current != row):
                raise ValueError("region snapshots must agree with their latest retained report")
        return self


class MomMembersReportResponse(MomMembersObservation):
    source: NonEmptyText
    sequence: SequenceNumber
    observed_at: Annotated[float, Field(allow_inf_nan=False)]


class MomServiceReportResponse(MomServiceObservation):
    source: NonEmptyText
    sequence: SequenceNumber
    observed_at: Annotated[float, Field(allow_inf_nan=False)]


class MomCounterReportResponse(MomCounterObservation):
    source: NonEmptyText
    sequence: SequenceNumber
    observed_at: Annotated[float, Field(allow_inf_nan=False)]


MomReportResponse = Annotated[MomMembersReportResponse | MomServiceReportResponse | MomCounterReportResponse, Field(discriminator="kind")]


def mom_report_key(row: MomReportResponse) -> tuple[str, str, str, str, str, str]:
    return row.source, row.session_id, row.identity_namespace, row.federation_id, row.kind, row.report_id


class MomResponse(ObserverModel):
    snapshots: Sequence[MomReportResponse]
    transitions: Sequence[MomReportResponse]
    capacity: PositiveInt
    history_capacity: PositiveInt
    dropped_snapshots: NonNegativeInt
    dropped_transitions: NonNegativeInt
    basis: Literal["explicit-management-reports"]

    @model_validator(mode="after")
    def consistent_management_reports(self) -> MomResponse:
        snapshots = {mom_report_key(row): row for row in self.snapshots}
        sequences = [row.sequence for row in self.transitions]
        if len(snapshots) != len(self.snapshots) or len(snapshots) > self.capacity or len(sequences) > self.history_capacity or sequences != sorted(set(sequences)):
            raise ValueError("management snapshots and arrivals must be unique and capacity bounded")
        for row in self.transitions:
            current = snapshots.get(mom_report_key(row))
            if current is not None and (current.sequence < row.sequence or current.sequence == row.sequence and current != row):
                raise ValueError("management snapshots must agree with their latest retained report")
        return self


class FomObjectReportResponse(FomObjectObservation):
    source: NonEmptyText
    sequence: SequenceNumber
    observed_at: Annotated[float, Field(allow_inf_nan=False)]


class FomInteractionReportResponse(FomInteractionObservation):
    source: NonEmptyText
    sequence: SequenceNumber
    observed_at: Annotated[float, Field(allow_inf_nan=False)]


class FomDatatypeReportResponse(FomDatatypeObservation):
    source: NonEmptyText
    sequence: SequenceNumber
    observed_at: Annotated[float, Field(allow_inf_nan=False)]


class FomDimensionReportResponse(FomDimensionObservation):
    source: NonEmptyText
    sequence: SequenceNumber
    observed_at: Annotated[float, Field(allow_inf_nan=False)]


FomReportResponse = Annotated[FomObjectReportResponse | FomInteractionReportResponse | FomDatatypeReportResponse | FomDimensionReportResponse, Field(discriminator="kind")]


def fom_report_key(row: FomReportResponse) -> tuple[str, str, str, str, str, str, str, str]:
    return row.source, row.session_id, row.identity_namespace, row.federation_id, row.model_id, row.revision, row.kind, row.element_id


class FomResponse(ObserverModel):
    snapshots: Sequence[FomReportResponse]
    transitions: Sequence[FomReportResponse]
    capacity: PositiveInt
    history_capacity: PositiveInt
    dropped_snapshots: NonNegativeInt
    dropped_transitions: NonNegativeInt
    basis: Literal["adapter-supplied-fom-metadata"]

    @model_validator(mode="after")
    def consistent_fom(self) -> FomResponse:
        snapshots = {fom_report_key(row): row for row in self.snapshots}
        sequences = [row.sequence for row in self.transitions]
        if len(snapshots) != len(self.snapshots) or len(snapshots) > self.capacity or len(sequences) > self.history_capacity or sequences != sorted(set(sequences)):
            raise ValueError("FOM metadata snapshots and arrivals must be unique and capacity bounded")
        for row in self.transitions:
            current = snapshots.get(fom_report_key(row))
            if current is not None and (current.sequence < row.sequence or current.sequence == row.sequence and current != row):
                raise ValueError("FOM snapshots must agree with their latest retained report")
        return self


class InspectorsResponse(ObserverModel):
    objects: Annotated[Sequence[ObjectInspectionResponse], Field(default_factory=tuple)]
    interactions: Annotated[Sequence[InteractionInspectionResponse], Field(default_factory=tuple)]
    object_observations: ObjectObservationsResponse | None = None
    time: TimeInspectionResponse | None = None
    declarations: DeclarationsResponse | None = None
    relationships: RelationshipsResponse | None = None
    coordination: CoordinationResponse | None = None
    interaction_flow: InteractionFlowResponse | None = None
    ownership: OwnershipResponse | None = None
    regions: RegionsResponse | None = None
    mom: MomResponse | None = None
    fom: FomResponse | None = None


class StorageCollectionResponse(ObserverModel):
    collection: NonEmptyText
    unit: NonEmptyText
    capacity: PositiveInt
    retained_records: NonNegativeInt
    accounted_bytes: NonNegativeInt
    soft_reservation_bytes: NonNegativeInt | None = None
    dropped_records: NonNegativeInt
    dropped_bytes: NonNegativeInt
    count_limit_drops: NonNegativeInt
    byte_limit_drops: NonNegativeInt
    oversized_drops: NonNegativeInt
    encoding_drops: NonNegativeInt
    window_drops: NonNegativeInt

    @model_validator(mode="after")
    def consistent_coverage(self) -> StorageCollectionResponse:
        if self.retained_records > self.capacity or self.dropped_records != sum((self.count_limit_drops, self.byte_limit_drops, self.oversized_drops, self.encoding_drops, self.window_drops)):
            raise ValueError("storage counts/loss reasons must agree")
        return self


class StorageBudgetResponse(ObserverModel):
    basis: Literal["encoded-json-plus-record-reserve"]
    policy: Literal["shared-evidence-lru", "shared-evidence-lru-with-soft-reservations"]
    max_bytes: PositiveInt
    max_record_bytes: PositiveInt
    accounted_bytes: NonNegativeInt
    record_reserve_bytes: NonNegativeInt
    collections: Sequence[StorageCollectionResponse]
    unbudgeted_collections: Sequence[NonEmptyText]

    @model_validator(mode="after")
    def consistent_budget(self) -> StorageBudgetResponse:
        names = [row.collection for row in self.collections]
        if (self.max_record_bytes > self.max_bytes or self.accounted_bytes > self.max_bytes
                or self.accounted_bytes != sum(row.accounted_bytes for row in self.collections)
                or len(names) != len(set(names)) or set(names) & set(self.unbudgeted_collections)
                or len(self.unbudgeted_collections) != len(set(self.unbudgeted_collections))):
            raise ValueError("storage budget must cover its exact, unique accounted collections")
        return self


class StorageCoverageResponse(ObserverModel):
    evidence: StorageBudgetResponse
    context: StorageBudgetResponse


class StateResponse(ObserverModel):
    contract_version: NonEmptyText
    schema_version: NonEmptyText
    status: NonEmptyText
    context: Annotated[Mapping[str, JsonValue], Field(default_factory=dict)]
    live_metrics: LiveMetricsResponse
    retention: RetentionResponse
    adapters: Annotated[Sequence[AdapterResponse], Field(default_factory=tuple)]
    capabilities: Annotated[Mapping[str, CapabilityResponse], Field(default_factory=dict)]
    federate_roster: Annotated[Sequence[FederateInspectionResponse], Field(default_factory=tuple)]
    inspectors: InspectorsResponse
    normalized_events: Annotated[Sequence[EventEnvelope] | None, Field(default=None)]
    storage: StorageCoverageResponse | None = None

    @model_validator(mode="after")
    def consistent_storage(self) -> StateResponse:
        feature = self.capabilities.get("storage_inspection")
        if self.storage is None:
            if feature is not None and feature.state in {"available", "partial"}:
                raise ValueError("available storage inspection requires its coverage model")
            return self
        if (not WIRE_REQUIRED_FIELDS["StateResponse"].issubset(self.model_fields_set)
                or not WIRE_REQUIRED_FIELDS["InspectorsResponse"].issubset(self.inspectors.model_fields_set)):
            raise ValueError("storage-backed states require explicit wire collections")
        rows = {row.collection: row for row in self.storage.evidence.collections}
        expected = {"events": self.retention.retained_events, "objects": len(self.inspectors.objects),
                    "interactions": len(self.inspectors.interactions), "federates": len(self.federate_roster)}
        data = self.inspectors
        if data.object_observations is not None:
            expected["object_snapshots"] = len(data.object_observations.snapshots)
        if data.time is not None:
            expected.update(time_snapshots=len(data.time.snapshots), time_requests=len(data.time.requests))
        for name, count in (("declarations", len(data.declarations.snapshots) if data.declarations else None),
                            ("relationships", len(data.relationships.snapshots) if data.relationships else None),
                            ("interaction_flow", len(data.interaction_flow.records) if data.interaction_flow else None)):
            if count is not None:
                expected[name] = count
        for name, reports in (("regions", data.regions), ("mom", data.mom), ("fom", data.fom)):
            if reports is not None:
                expected[name + "_snapshots"] = len(reports.snapshots)
                expected[name + "_history"] = len(reports.transitions)
        if data.coordination is not None:
            expected.update(coordination_snapshots=len(data.coordination.snapshots), coordination_history=len(data.coordination.transitions))
        if data.ownership is not None:
            expected.update(ownership_states=len(data.ownership.states), ownership_requests=len(data.ownership.requests), ownership_history=len(data.ownership.transitions))
        unbudgeted = set(self.storage.evidence.unbudgeted_collections)
        for name, count in expected.items():
            if name not in rows and name not in unbudgeted or name in rows and rows[name].retained_records != count:
                raise ValueError("storage coverage must identify each delivered collection and its exact retained count")
        context = next((row for row in self.storage.context.collections if row.collection == "context"), None)
        if context is None or context.retained_records != len(self.context):
            raise ValueError("context storage coverage must match delivered context keys")
        return self


class AnalyticsWindow(ObserverModel):
    """Consistent retained observations/roster, without unrelated inspectors."""

    contract_version: NonEmptyText
    retention: RetentionResponse
    federate_roster: Sequence[FederateInspectionResponse]
    normalized_events: Sequence[EventEnvelope]
    storage: StorageCoverageResponse | None = None

    @model_validator(mode="after")
    def consistent_window(self) -> AnalyticsWindow:
        retention = self.retention
        sequences = [row.sequence for row in self.normalized_events]
        if self.contract_version != "umbra-observer-contract-v1":
            raise ValueError("analytics window has an incompatible contract")
        if len(sequences) != retention.retained_events or sequences != sorted(set(sequences)) or len(sequences) > retention.max_events:
            raise ValueError("analytics window envelopes must be unique, ordered and capacity bounded")
        if sequences and (sequences[0] != retention.first_retained_sequence or sequences[-1] != retention.last_sequence
                          or len(sequences) != sequences[-1] - sequences[0] + 1 or sequences[-1] != retention.next_sequence - 1):
            raise ValueError("analytics window must include its complete retained cursor range")
        if retention.next_sequence <= retention.last_sequence:
            raise ValueError("analytics next cursor must follow retained history")
        if len({row.federate for row in self.federate_roster}) != len(self.federate_roster):
            raise ValueError("analytics roster identities must be unique")
        if self.storage is not None:
            rows = {row.collection: row for row in self.storage.evidence.collections}
            for name, count in (("events", retention.retained_events), ("federates", len(self.federate_roster))):
                if name in rows and rows[name].retained_records != count:
                    raise ValueError("projected storage coverage must agree with envelopes and roster")
        json.dumps(self.model_dump(mode="python", exclude_unset=True), allow_nan=False)
        return self


class RuntimeSummaryResponse(ObserverModel):
    contract_version: NonEmptyText
    schema_version: NonEmptyText
    status: NonEmptyText
    live_metrics: LiveMetricsResponse
    retention: RetentionResponse
    adapters: Sequence[AdapterResponse]
    capabilities: Mapping[str, CapabilityResponse]


class ActivityBucket(ObserverModel):
    started_at: float
    events: NonNegativeInt
    object_updates: NonNegativeInt
    interactions: NonNegativeInt
    callbacks: NonNegativeInt
    errors: NonNegativeInt


class FederateActivity(ObserverModel):
    roster_retained: bool | None = None
    federate: NonEmptyText
    role: str | None
    handle: str | None
    status: NonEmptyText
    source: NonEmptyText
    last_seen: float | None
    event_count: NonNegativeInt
    object_updates: NonNegativeInt
    interaction_count: NonNegativeInt
    error_count: NonNegativeInt
    object_keys: Sequence[str]
    latest_logical_time: int | float | None


class TimeObservation(ObserverModel):
    sequence: SequenceNumber
    federate: str | None
    logical_time: Annotated[int | float, Field(allow_inf_nan=False)]
    observed_at: float
    event_type: NonEmptyText
    operation: str | None
    observation_kind: Literal["timestamp", "request", "grant", "state"] = "timestamp"


class AnalyticsResponse(ObserverModel):
    contract_version: NonEmptyText
    basis: Literal["retained-observations"]
    generated_at: float
    window_start: float
    window_seconds: PositiveInt
    bucket_seconds: Annotated[float, Field(gt=0)]
    scoped_federate: str | None
    events_in_window: NonNegativeInt
    unattributed_events: NonNegativeInt
    history_truncated: bool
    retention: RetentionResponse
    participants: Sequence[FederateActivity]
    activity: Sequence[ActivityBucket]
    logical_times: Sequence[TimeObservation]
    event_types: Mapping[str, NonNegativeInt]
    sources: Mapping[str, NonNegativeInt]
    families: Mapping[str, NonNegativeInt]
    roster_storage: StorageCollectionResponse | None = None


class HealthResponse(ObserverModel):
    service: NonEmptyText
    status: NonEmptyText
    contract_version: NonEmptyText
    schema_version: NonEmptyText
    event_count: NonNegativeInt
    retention: RetentionResponse
    capabilities: Annotated[Mapping[str, CapabilityResponse], Field(default_factory=dict)]


class MetricsResponse(ObserverModel):
    contract_version: NonEmptyText
    schema_version: NonEmptyText
    status: NonEmptyText
    live_metrics: LiveMetricsResponse
    retention: RetentionResponse
    adapters: Annotated[Sequence[AdapterResponse], Field(default_factory=tuple)]


class EventPageResponse(ObserverModel):
    contract_version: NonEmptyText
    events: Annotated[Sequence[EventEnvelope], Field(default_factory=tuple)]
    next_after: NonNegativeInt
    has_more: bool
    gap: bool
    first_retained_sequence: NonNegativeInt
    last_sequence: NonNegativeInt
    retained_events: NonNegativeInt
    dropped_events: NonNegativeInt
    cleared_events: NonNegativeInt
    next_sequence: PositiveInt | None = None


class IngestResponse(ObserverModel):
    accepted: NonNegativeInt
    state: StateResponse


class ContextResponse(ObserverModel):
    context: Annotated[Mapping[str, JsonValue], Field(default_factory=dict)]


class InspectorResponse(ObserverModel):
    contract_version: NonEmptyText
    kind: Literal["objects", "interactions", "federates"]
    items: Annotated[
        Sequence[
            ObjectInspectionResponse
            | InteractionInspectionResponse
            | FederateInspectionResponse
        ],
        Field(default_factory=tuple),
    ]


class CapabilitySchemaResponse(ObserverModel):
    states: Sequence[CapabilityState]
    fields: Sequence[str]
    known: Sequence[str]
    state_meanings: Mapping[CapabilityState, str]


class ResourceDescription(ObserverModel):
    method: Literal["GET", "POST", "PUT", "DELETE"]
    path: NonEmptyText
    query: Annotated[Mapping[str, str], Field(default_factory=dict)]
    required_fields: Annotated[Sequence[str], Field(default_factory=tuple)]


class EventEnvelopeDescription(ObserverModel):
    required_fields: Sequence[str]
    extension_rule: NonEmptyText


class StreamDescription(ObserverModel):
    websocket_path: NonEmptyText
    sse_path: NonEmptyText
    message_types: Sequence[Literal["snapshot", "events", "heartbeat", "gap", "error"]]
    messages: Mapping[str, str]


class ContractResponse(ObserverModel):
    contract_version: NonEmptyText
    media_type: NonEmptyText
    event_schema_version: NonEmptyText
    api_paths: Mapping[str, str]
    capability_schema: CapabilitySchemaResponse
    capabilities: Annotated[Mapping[str, CapabilityResponse], Field(default_factory=dict)]
    snapshot: ResourceDescription
    events: ResourceDescription
    event_envelope: EventEnvelopeDescription
    stream: StreamDescription
    event_types: Sequence[str]
    backend_rule: NonEmptyText


class CatalogResponse(ObserverModel):
    contract_version: NonEmptyText
    schema_version: NonEmptyText
    event_schema: Mapping[str, JsonValue]
    contract: ContractResponse
    capabilities: Annotated[Mapping[str, CapabilityResponse], Field(default_factory=dict)]
    features: Annotated[Sequence[str], Field(default_factory=tuple)]


class JsonObjectResponse(RootModel[JsonObject]):
    """Validated object response for the JSON schema endpoint."""


class SnapshotMessage(ObserverModel):
    type: Literal["snapshot"]
    state: StateResponse


class EventsMessage(ObserverModel):
    type: Literal["events"]
    events: Annotated[Sequence[EventEnvelope], Field(default_factory=tuple)]
    state: StateResponse


class HeartbeatMessage(ObserverModel):
    type: Literal["heartbeat"]


class GapMessage(ObserverModel):
    type: Literal["gap"]
    retention: RetentionGapResponse


class ErrorMessage(ObserverModel):
    type: Literal["error"]
    message: str


StreamMessage = (
    SnapshotMessage
    | EventsMessage
    | HeartbeatMessage
    | GapMessage
    | ErrorMessage
)


def event_input_mapping(event: EventInput) -> dict[str, object]:
    """Convert a validated request model to the core adapter seam."""

    return event.model_dump(mode="python", exclude_none=True)


def json_model(model: BaseModel | RootModel[JsonObject]) -> JsonObject:
    """Serialize a validated model as a JSON-compatible object."""

    value = model.model_dump(mode="json", exclude_none=True)
    if not isinstance(value, dict):  # pragma: no cover - guarded by model type
        raise TypeError("observer transport model must serialize to an object")
    return value


__all__ = [
    "AdapterResponse",
    "CapabilityResponse",
    "CapabilitySchemaResponse",
    "CatalogResponse",
    "ContextRequest",
    "ContextResponse",
    "ContractResponse",
    "ErrorMessage",
    "EventBatchRequest",
    "EventEnvelope",
    "EventEnvelopeDescription",
    "EventIngestRequest",
    "EventInput",
    "EventPageResponse",
    "EventsMessage",
    "FederateInspectionResponse",
    "GapMessage",
    "HealthResponse",
    "HeartbeatMessage",
    "IngestResponse",
    "InspectorResponse",
    "InspectorsResponse",
    "InteractionInspectionResponse",
    "JsonObjectResponse",
    "LiveMetricsResponse",
    "MetricsResponse",
    "ObjectInspectionResponse",
    "ResourceDescription",
    "RetentionGapResponse",
    "RetentionResponse",
    "SnapshotMessage",
    "StateResponse",
    "StatusRequest",
    "StreamDescription",
    "StreamMessage",
    "event_input_mapping",
    "json_model",
]
