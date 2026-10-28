"""Pydantic schemas for the optional observer HTTP transport.

The core store intentionally has no web-framework dependency.  This module is
the opposite boundary: every request body and every finite JSON response that
FastAPI exposes has a named, inspectable model.  Provider-specific fields are
allowed only where the wire contract explicitly calls them extensions.
"""

from __future__ import annotations

import json
from collections.abc import Mapping, Sequence
from typing import Annotated, ClassVar, Literal, cast

from pydantic import (
    BaseModel,
    ConfigDict,
    Field,
    RootModel,
    SerializerFunctionWrapHandler,
    field_validator,
    model_serializer,
    model_validator,
)

from .attribution import (
    ParticipantAttribution,
    ParticipantPerspective,
    participant_attribution_from_fields,
    validate_participant_sessions,
)
from .coordination import (
    CoordinationData,
    CoordinationKind,
    CoordinationParticipant,
    CoordinationPhase,
    coordination_from_fields,
)
from .declarations import DeclarationData, declaration_from_fields
from .fom import (
    FomClassData,
    FomClassKind,
    FomContext,
    FomDatatypeData,
    FomDimensionData,
    FomEnumerator,
    FomField,
    fom_class_from_fields,
    fom_context_from_fields,
    fom_datatype_from_fields,
    fom_dimension_from_fields,
    fom_enumerator_from_fields,
    fom_field_from_fields,
)
from .identities import (
    ParticipantIdentity as ParticipantIdentity,  # noqa: PLC0414 -- preserve schemas module export
)
from .identities import (
    ParticipantReference,
    decimal_bound,
    participant_reference_from_fields,
)
from .interaction_flow import (
    InteractionFlowData,
    flow_number,
    interaction_flow_from_fields,
    validate_interaction_flow_direction,
)
from .models import RawObserverEvent, ReportedParticipant
from .mom import (
    MomContext,
    MomCounterData,
    MomMember,
    MomMembersData,
    MomServiceData,
    counter_number,
    mom_context_from_fields,
    mom_counter_from_fields,
    mom_member_from_fields,
    mom_members_from_fields,
    mom_service_from_fields,
)
from .object_views import ObjectContext, object_context_from_fields
from .ownership import (
    OwnershipData,
    ownership_outcome_from_fields,
    ownership_request_from_fields,
    ownership_scope_from_fields,
    ownership_state_from_fields,
)
from .region_schemas import (
    DecimalBound,
    RegionAssociationObservation,
    RegionObservationInput,
    RegionRoutingObservation,
    RegionStateObservation,
)
from .region_schemas import (
    DimensionRangeResponse as DimensionRangeResponse,  # noqa: PLC0414 -- preserve schemas module export
)
from .region_schemas import (
    RegionExtentResponse as RegionExtentResponse,  # noqa: PLC0414 -- preserve schemas module export
)
from .region_schemas import (
    RegionObservationBase as RegionObservationBase,  # noqa: PLC0414 -- preserve schemas module export
)
from .relationships import RelationshipData, relationship_from_fields
from .schema_common import (
    WIRE_REQUIRED_FIELDS,
    CapabilityState,
    DeclarationId,
    EventTypeText,
    ExtensibleObserverModel,
    JsonObject,
    JsonText,
    JsonValue,
    NonEmptyJsonText,
    NonEmptyText,
    NonNegativeFloat,
    NonNegativeInt,
    ObserverModel,
    ParticipantIdentityResponse,
    PositiveInt,
    SequenceNumber,
    TimeId,
    TimeNumber,
    WireObserverModel,
)


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
    TimeRequestObservation
    | TimeGrantObservation
    | TimeStateObservation
    | TimeOutcomeObservation,
    Field(discriminator="kind"),
]


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
    attributes: Annotated[
        Sequence[DeclarationId] | None, Field(default=None, max_length=4096)
    ]

    def to_domain(self) -> DeclarationData:
        return declaration_from_fields(
            self.kind,
            self.direction,
            self.status,
            self.class_key,
            class_name=self.class_name,
            session_id=self.session_id,
            reason=self.reason,
            attribute_scope=self.attribute_scope,
            raw_attributes=self.attributes,
        )

    @model_validator(mode="after")
    def explicit_attribute_set(self) -> ObjectDeclarationObservation:
        self.to_domain()
        return self


class InteractionDeclarationObservation(DeclarationObservationBase):
    kind: Literal["interaction"]


DeclarationObservationInput = Annotated[
    ObjectDeclarationObservation | InteractionDeclarationObservation,
    Field(discriminator="kind"),
]


class ParticipantAttributionInput(ObserverModel):
    session_id: TimeId
    identity_namespace: DeclarationId
    federation_id: DeclarationId
    participant: ParticipantIdentityResponse

    def to_domain(self) -> ParticipantAttribution:
        """Return explicit join provenance as a typed domain value."""
        return participant_attribution_from_fields(
            self.session_id,
            self.identity_namespace,
            self.federation_id,
            self.participant.to_domain(),
        )

    @model_validator(mode="after")
    def valid_identity(self) -> ParticipantAttributionInput:
        self.to_domain()
        return self


class ParticipantPerspectiveResponse(ObserverModel):
    key: NonEmptyText
    basis: Literal["explicit-join", "legacy-provenance"]
    source: NonEmptyText
    session_id: TimeId | None = None
    federation: str | None = None
    reported_name: str | None = None
    attribution: ParticipantAttributionInput | None = None

    @property
    def label(self) -> str:
        return (
            (self.attribution.participant.display_name if self.attribution else None)
            or self.reported_name
            or "Unnamed participant"
        )

    def spec(self) -> ParticipantPerspective:
        identity = self.attribution.to_domain() if self.attribution else None
        return ParticipantPerspective(
            self.source, self.session_id, self.federation, self.reported_name, identity
        )

    @model_validator(mode="after")
    def canonical_perspective(self) -> ParticipantPerspectiveResponse:
        expected = self.spec()
        identity = expected.attribution
        if identity is None and not self.reported_name:
            raise ValueError(
                "legacy reporting perspectives require a reported name, not an invented join"
            )
        if (
            self.key != expected.key
            or self.basis != expected.basis
            or identity is not None
            and self.session_id != identity.session_id
        ):
            raise ValueError(
                "participant perspective key, provenance and identity must agree"
            )
        return self


class ObjectContextInput(ObserverModel):
    identity_namespace: DeclarationId
    federation_id: DeclarationId

    @model_validator(mode="after")
    def explicit_context(self) -> ObjectContextInput:
        object_context_from_fields(self.identity_namespace, self.federation_id)
        return self


def _object_scope_key(
    key: str, context: ObjectContextInput | None
) -> tuple[str | None, str | None, str]:
    return (
        context.identity_namespace if context else None,
        context.federation_id if context else None,
        key,
    )


def _participant_scope_key(
    perspective: ParticipantPerspectiveResponse | None, name: str | None
) -> tuple[str, str | None]:
    return ("perspective", perspective.key) if perspective else ("legacy-name", name)


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

    def to_domain(self) -> RelationshipData:
        """Return the validated relationship without a JSON-map round trip."""
        return relationship_from_fields(
            relationship_id=self.relationship_id,
            session_id=self.session_id,
            identity_namespace=self.identity_namespace,
            federation_id=self.federation_id,
            producer=self.producer.to_domain(),
            recipient=self.recipient.to_domain(),
            kind=self.kind,
            evidence=self.evidence,
            status=self.status,
            viewpoint=self.viewpoint,
            class_key=self.class_key,
            class_name=self.class_name,
            reason=self.reason,
        )

    @model_validator(mode="after")
    def explicit_relationship(self) -> RelationshipObservationInput:
        self.to_domain()
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
    logical_time: Annotated[
        int | float | None, Field(default=None, allow_inf_nan=False)
    ]

    def to_domain(self) -> InteractionFlowData:
        """Return the validated flow record without rebuilding a JSON mapping."""
        return interaction_flow_from_fields(
            kind=self.kind,
            order=self.order,
            message_id=self.message_id,
            identity_namespace=self.identity_namespace,
            federation_id=self.federation_id,
            session_id=self.session_id,
            producer=self.producer.to_domain() if self.producer else None,
            recipient=self.recipient.to_domain() if self.recipient else None,
            class_key=self.class_key,
            logical_time=self.logical_time,
        )

    @field_validator("logical_time", mode="before")
    @classmethod
    def finite_logical_time(cls, value: object) -> int | float | None:
        # Catch oversized Python integers before Pydantic's float finiteness
        # check can raise an uncaught OverflowError at the HTTP boundary.
        return flow_number(value)

    @model_validator(mode="after")
    def explicit_flow(self) -> InteractionFlowObservationInput:
        self.to_domain()
        return self


class CoordinationParticipantResponse(ObserverModel):
    identity: ParticipantIdentityResponse
    status: Literal["pending", "achieved", "begun", "completed", "failed", "unknown"]
    reason: Annotated[JsonText | None, Field(default=None, max_length=4096)]

    def to_domain(self) -> CoordinationParticipant:
        return CoordinationParticipant(
            self.identity.to_domain(), self.status, self.reason
        )


class CoordinationObservationBase(ObserverModel):
    kind: CoordinationKind
    phase: CoordinationPhase
    operation_id: DeclarationId
    session_id: TimeId
    identity_namespace: DeclarationId
    federation_id: DeclarationId
    label: DeclarationId
    scope: Literal["local", "federation"]
    subject: ParticipantIdentityResponse | None = None
    participants: Annotated[
        Sequence[CoordinationParticipantResponse] | None,
        Field(default=None, max_length=4096),
    ]
    reason: Annotated[JsonText | None, Field(default=None, max_length=4096)]

    def _to_domain(
        self, *, kind: CoordinationKind, phase: CoordinationPhase
    ) -> CoordinationData:
        if self.scope == "federation" and self.subject is not None:
            raise ValueError(
                "a federation-wide observation cannot claim a single local subject"
            )
        participants = (
            (participant.to_domain() for participant in self.participants)
            if self.participants is not None
            else None
        )
        return coordination_from_fields(
            kind=kind,
            operation_id=self.operation_id,
            session_id=self.session_id,
            identity_namespace=self.identity_namespace,
            federation_id=self.federation_id,
            label=self.label,
            phase=phase,
            scope=self.scope,
            subject=self.subject.to_domain() if self.subject is not None else None,
            participants=participants,
            reason=self.reason,
        )

    def to_domain(self) -> CoordinationData:
        return self._to_domain(kind=self.kind, phase=self.phase)

    @model_validator(mode="after")
    def explicit_coordination(self) -> CoordinationObservationBase:
        self.to_domain()
        return self


class SynchronizationObservation(CoordinationObservationBase):
    kind: Literal["synchronization"]  # pyright: ignore[reportIncompatibleVariableOverride]
    phase: Literal[  # pyright: ignore[reportIncompatibleVariableOverride]
        "requested",
        "registered",
        "registration-failed",
        "announced",
        "achieved",
        "completed",
        "failed",
    ]


class PersistenceObservation(CoordinationObservationBase):
    kind: Literal["save", "restore"]  # pyright: ignore[reportIncompatibleVariableOverride]
    phase: Literal[  # pyright: ignore[reportIncompatibleVariableOverride]
        "requested", "begun", "completed", "failed"
    ]


CoordinationObservationInput = Annotated[
    SynchronizationObservation | PersistenceObservation, Field(discriminator="kind")
]


class OwnershipObservationBase(ObserverModel):
    session_id: TimeId
    identity_namespace: DeclarationId
    federation_id: DeclarationId
    object_id: DeclarationId
    attribute_key: DeclarationId
    reason: Annotated[JsonText | None, Field(default=None, max_length=4096)]

    def scope_key(self, source: str) -> tuple[str, str, str, str, str, str]:
        return (
            source,
            self.session_id,
            self.identity_namespace,
            self.federation_id,
            self.object_id,
            self.attribute_key,
        )

    def to_domain(self) -> OwnershipData:
        scope = ownership_scope_from_fields(
            self.session_id,
            self.identity_namespace,
            self.federation_id,
            self.object_id,
            self.attribute_key,
        )
        if isinstance(self, OwnershipStateObservation):
            if (self.owner_state == "known") != (self.owner is not None):
                raise ValueError(
                    "only a known owner has an explicit participant join identity"
                )
            return ownership_state_from_fields(
                scope,
                self.owner_state,
                self.owner.to_domain() if self.owner is not None else None,
                self.transition,
                self.reason,
                self.reported_owner_name,
                self.reported_owner_handle,
            )
        if isinstance(self, OwnershipOutcomeObservation):
            return ownership_outcome_from_fields(
                scope,
                self.request_id,
                self.action,
                self.requester.to_domain(),
                self.outcome,
                self.reason,
            )
        if isinstance(self, OwnershipRequestObservation):
            return ownership_request_from_fields(
                scope,
                self.request_id,
                self.action,
                self.requester.to_domain(),
                self.reason,
            )
        raise TypeError(
            "ownership input must select a named state, request, or outcome"
        )

    @model_validator(mode="after")
    def explicit_ownership(self) -> OwnershipObservationBase:
        self.to_domain()
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

    def request_key(
        self, source: str
    ) -> tuple[tuple[str, str, str, str, str, str], str, str, str, str]:
        return (
            self.scope_key(source),
            self.request_id,
            self.action,
            self.requester.participant_id,
            self.requester.membership_id,
        )


class OwnershipRequestObservation(OwnershipRequestBase):
    kind: Literal["request"]


class OwnershipOutcomeObservation(OwnershipRequestBase):
    kind: Literal["outcome"]
    outcome: Literal["completed", "failed", "cancelled"]


OwnershipObservationInput = Annotated[
    OwnershipStateObservation
    | OwnershipRequestObservation
    | OwnershipOutcomeObservation,
    Field(discriminator="kind"),
]


class ParticipantReferenceResponse(ObserverModel):
    identity: ParticipantIdentityResponse | None = None
    display_name: DeclarationId | None = None
    handle: DeclarationId | None = None

    def to_domain(self) -> ParticipantReference:
        return participant_reference_from_fields(
            self.identity.to_domain() if self.identity is not None else None,
            self.display_name,
            self.handle,
        )

    @model_validator(mode="after")
    def actual_reference_evidence(self) -> ParticipantReferenceResponse:
        self.to_domain()
        return self


class MomMemberResponse(ObserverModel):
    participant: ParticipantReferenceResponse
    status: Literal["joined", "resigned", "unknown"]

    def to_domain(self) -> MomMember:
        return mom_member_from_fields(self.participant.to_domain(), self.status)


class MomObservationBase(ObserverModel):
    session_id: TimeId
    identity_namespace: DeclarationId
    federation_id: DeclarationId
    report_id: DeclarationId
    reason: Annotated[JsonText | None, Field(default=None, max_length=4096)]

    def _context(self) -> MomContext:
        return mom_context_from_fields(
            self.session_id,
            self.identity_namespace,
            self.federation_id,
            self.report_id,
            self.reason,
        )


class MomMembersObservation(MomObservationBase):
    kind: Literal["members"]
    coverage: Literal["complete", "partial", "unknown"]
    members: Annotated[
        Sequence[MomMemberResponse] | None, Field(default=None, max_length=4096)
    ]

    def to_domain(self) -> MomMembersData:
        members = (
            tuple(row.to_domain() for row in self.members)
            if self.members is not None
            else None
        )
        return mom_members_from_fields(self._context(), self.coverage, members)

    @model_validator(mode="after")
    def explicit_members_report(self) -> MomMembersObservation:
        self.to_domain()
        return self


class MomServiceObservation(MomObservationBase):
    kind: Literal["service"]
    service_name: DeclarationId
    status: Literal["invoked", "completed", "failed", "unknown"]
    subject: ParticipantReferenceResponse | None = None
    invocation_id: DeclarationId | None = None
    error_name: DeclarationId | None = None

    def to_domain(self) -> MomServiceData:
        subject = self.subject.to_domain() if self.subject is not None else None
        return mom_service_from_fields(
            self._context(),
            self.service_name,
            self.status,
            subject,
            self.invocation_id,
            self.error_name,
        )

    @model_validator(mode="after")
    def explicit_service_report(self) -> MomServiceObservation:
        self.to_domain()
        return self


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

    def to_domain(self) -> MomCounterData:
        subject = self.subject.to_domain() if self.subject is not None else None
        return mom_counter_from_fields(
            self._context(),
            self.counter_key,
            self.semantics,
            self.value,
            self.value_text,
            self.unit,
            subject,
        )

    @model_validator(mode="after")
    def explicit_counter_report(self) -> MomCounterObservation:
        self.to_domain()
        return self


MomObservationInput = Annotated[
    MomMembersObservation | MomServiceObservation | MomCounterObservation,
    Field(discriminator="kind"),
]


class FomFieldResponse(ObserverModel):
    field_id: DeclarationId
    name: DeclarationId | None = None
    datatype_id: DeclarationId | None = None
    description: Annotated[JsonText | None, Field(default=None, max_length=4096)]

    def to_domain(self) -> FomField:
        return fom_field_from_fields(
            self.field_id,
            self.name,
            self.datatype_id,
            self.description,
        )


class FomEnumeratorResponse(ObserverModel):
    name: DeclarationId
    value: DeclarationId

    def to_domain(self) -> FomEnumerator:
        return fom_enumerator_from_fields(self.name, self.value)


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

    def _context(self) -> FomContext:
        return fom_context_from_fields(
            self.session_id,
            self.identity_namespace,
            self.federation_id,
            self.model_id,
            self.revision,
            self.element_id,
            self.name,
            self.status,
            self.description,
        )


class FomClassObservationBase(FomObservationBase):
    parent_state: Literal["root", "parent", "unknown"]
    parent_id: DeclarationId | None = None
    field_scope: Literal["declared", "effective", "unknown"]

    def _class_domain(
        self,
        class_kind: FomClassKind,
        fields: Sequence[FomFieldResponse] | None,
    ) -> FomClassData:
        domain_fields = (
            tuple(row.to_domain() for row in fields) if fields is not None else None
        )
        return fom_class_from_fields(
            self._context(),
            class_kind,
            self.parent_state,
            self.parent_id,
            self.field_scope,
            domain_fields,
        )


class FomObjectObservation(FomClassObservationBase):
    kind: Literal["object-class"]
    attributes: Annotated[
        Sequence[FomFieldResponse] | None, Field(default=None, max_length=4096)
    ]

    def to_domain(self) -> FomClassData:
        return self._class_domain("object-class", self.attributes)

    @model_validator(mode="after")
    def explicit_object_class_metadata(self) -> FomObjectObservation:
        self.to_domain()
        return self


class FomInteractionObservation(FomClassObservationBase):
    kind: Literal["interaction-class"]
    parameters: Annotated[
        Sequence[FomFieldResponse] | None, Field(default=None, max_length=4096)
    ]

    def to_domain(self) -> FomClassData:
        return self._class_domain("interaction-class", self.parameters)

    @model_validator(mode="after")
    def explicit_interaction_class_metadata(self) -> FomInteractionObservation:
        self.to_domain()
        return self


class FomDatatypeObservation(FomObservationBase):
    kind: Literal["datatype"]
    encoding: Literal[
        "primitive", "enumeration", "array", "record", "variant", "opaque", "unknown"
    ]
    representation: Annotated[JsonText | None, Field(default=None, max_length=4096)]
    fields: Annotated[
        Sequence[FomFieldResponse] | None, Field(default=None, max_length=4096)
    ]
    enumerators: Annotated[
        Sequence[FomEnumeratorResponse] | None, Field(default=None, max_length=4096)
    ]
    element_type_id: DeclarationId | None = None
    cardinality: DecimalBound | None = None

    @field_validator("cardinality", mode="before")
    @classmethod
    def canonical_cardinality(cls, value: object) -> str | None:
        return decimal_bound(value, optional=True)

    def to_domain(self) -> FomDatatypeData:
        fields = (
            tuple(row.to_domain() for row in self.fields)
            if self.fields is not None
            else None
        )
        enumerators = (
            tuple(row.to_domain() for row in self.enumerators)
            if self.enumerators is not None
            else None
        )
        return fom_datatype_from_fields(
            self._context(),
            self.encoding,
            self.representation,
            fields,
            enumerators,
            self.element_type_id,
            self.cardinality,
        )

    @model_validator(mode="after")
    def explicit_datatype_metadata(self) -> FomDatatypeObservation:
        self.to_domain()
        return self


class FomDimensionObservation(FomObservationBase):
    kind: Literal["dimension"]
    upper_bound: DecimalBound | None = None
    unit: DeclarationId | None = None

    @field_validator("upper_bound", mode="before")
    @classmethod
    def canonical_bound(cls, value: object) -> str | None:
        return decimal_bound(value, optional=True)

    def to_domain(self) -> FomDimensionData:
        return fom_dimension_from_fields(
            self._context(),
            self.upper_bound,
            self.unit,
        )

    @model_validator(mode="after")
    def explicit_dimension_metadata(self) -> FomDimensionObservation:
        self.to_domain()
        return self


FomObservationInput = Annotated[
    FomObjectObservation
    | FomInteractionObservation
    | FomDatatypeObservation
    | FomDimensionObservation,
    Field(discriminator="kind"),
]


def _validate_event_participant_sessions(
    attribution: ParticipantAttributionInput | None,
    session_id: object,
    time_observation: TimeObservationInput | None,
    declaration: DeclarationObservationInput | None,
) -> None:
    """Validate session agreement from the event's named schema fields."""
    identity = attribution.to_domain() if attribution is not None else None
    validate_participant_sessions(
        identity,
        (
            session_id,
            time_observation.session_id if time_observation is not None else None,
            declaration.session_id if declaration is not None else None,
        ),
    )


class EventInput(ExtensibleObserverModel):
    """Normalized event input plus JSON-compatible provider extensions."""

    sequence: Annotated[int | None, Field(default=None, ge=1)]
    event_type: Annotated[EventTypeText | None, Field(default=None)]
    kind: Annotated[EventTypeText | None, Field(default=None)]
    family: Annotated[NonEmptyText | None, Field(default=None)]
    source: Annotated[str | None, Field(default=None, min_length=1, max_length=512)]
    observed_at: float | None = None
    connection: str | None = None
    session_id: TimeId | None = None
    federation: str | None = None
    federate: ReportedParticipant = None
    participant: ReportedParticipant = None
    federate_name: str | None = None
    participant_name: str | None = None
    participant_attribution: ParticipantAttributionInput | None = None
    federate_handle: str | None = None
    participant_handle: str | None = None
    role: str | None = None
    object_key: JsonText | None = None
    object_context: ObjectContextInput | None = None
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
        _validate_event_participant_sessions(
            self.participant_attribution,
            self.session_id,
            self.time_observation,
            self.declaration,
        )
        if self.interaction_flow is not None:
            validate_interaction_flow_direction(
                self.event_type,
                self.kind,
                self.interaction_flow.kind,
            )
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
    observations: Annotated[
        Sequence[Mapping[str, JsonValue]], Field(default_factory=tuple)
    ]
    reason: str | None = None
    reasons: Annotated[Sequence[str], Field(default_factory=tuple)]
    event_types: Annotated[Sequence[str], Field(default_factory=tuple)]


class ObjectInspectionResponse(ObserverModel):
    object_key: NonEmptyJsonText
    object_context: ObjectContextInput | None = None
    observation_key: NonEmptyText | None = None
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

    @model_validator(mode="after")
    def scoped_object_key(self) -> ObjectInspectionResponse:
        if self.object_context is None:
            if self.observation_key is not None:
                raise ValueError(
                    "an object observation key requires its explicit federation context"
                )
        elif self.observation_key != ObjectContext(
            self.object_context.identity_namespace, self.object_context.federation_id
        ).key(self.object_key):
            raise ValueError("object observation key and federation context must agree")
        return self


class InteractionInspectionResponse(ObserverModel):
    sequence: SequenceNumber
    interaction_key: NonEmptyText
    interaction_class: NonEmptyText
    class_handle: str | None = None
    family: NonEmptyText
    source: NonEmptyText
    federate: JsonText | None = None
    parameters: Annotated[Mapping[JsonText, JsonValue], Field(default_factory=dict)]
    tag: Annotated[JsonValue, Field(default=None)]
    observed_at: float
    perspective: ParticipantPerspectiveResponse | None = None

    @model_validator(mode="after")
    def reporting_source_matches(self) -> InteractionInspectionResponse:
        if self.perspective is not None and (
            self.source != self.perspective.source
            or self.federate != self.perspective.label
        ):
            raise ValueError(
                "interaction reporting source and participant perspective must agree"
            )
        return self


class FederateInspectionResponse(ObserverModel):
    federate: NonEmptyJsonText
    role: str | None = None
    handle: str | None = None
    status: NonEmptyText
    source: NonEmptyText
    last_seen: float | None = None
    perspective: ParticipantPerspectiveResponse | None = None

    @model_validator(mode="after")
    def consistent_reporting_source(self) -> FederateInspectionResponse:
        if self.perspective is not None and (
            self.source != self.perspective.source
            or self.federate != self.perspective.label
        ):
            raise ValueError(
                "roster reporting source must agree with its participant perspective"
            )
        return self


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
    federation: str | None = None
    session_id: TimeId | None = None
    participant_attribution: ParticipantAttributionInput | None = None
    federate: ReportedParticipant = None
    participant: ReportedParticipant = None
    federate_name: str | None = None
    participant_name: str | None = None
    logical_time: Annotated[
        int | float | None, Field(default=None, allow_inf_nan=False)
    ]
    operation: str | None = None
    details: Mapping[str, JsonValue] | None = None
    object_key: JsonText | None = None
    object_context: ObjectContextInput | None = None
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
        _validate_event_participant_sessions(
            self.participant_attribution,
            self.session_id,
            self.time_observation,
            self.declaration,
        )
        if self.interaction_flow is not None:
            validate_interaction_flow_direction(
                self.event_type,
                None,
                self.interaction_flow.kind,
            )
        return self


class ObjectPerspectiveResponse(ObserverModel):
    """Null federate is global; a name is explicit adapter attribution only."""

    federate: NonEmptyJsonText | None
    generation: SequenceNumber
    last_sequence: SequenceNumber
    object: ObjectInspectionResponse
    perspective: ParticipantPerspectiveResponse | None = None

    @property
    def cache_key(
        self,
    ) -> tuple[tuple[str, str | None], tuple[str | None, str | None, str]]:
        return _participant_scope_key(
            self.perspective, self.federate
        ), _object_scope_key(self.object.object_key, self.object.object_context)

    @model_validator(mode="after")
    def attributed_perspective(self) -> ObjectPerspectiveResponse:
        if self.perspective is not None and self.federate != self.perspective.label:
            raise ValueError(
                "global objects must not claim a participant reporting perspective"
            )
        return self

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
    object_key: NonEmptyJsonText
    federate: NonEmptyJsonText | None
    generation: SequenceNumber
    attribute: JsonText
    value: JsonValue
    previous_known: bool
    previous_value: JsonValue
    source: NonEmptyText
    observed_at: Annotated[float, Field(allow_inf_nan=False)]
    perspective: ParticipantPerspectiveResponse | None = None
    object_context: ObjectContextInput | None = None
    observation_key: NonEmptyText | None = None

    @property
    def cache_key(
        self,
    ) -> tuple[tuple[str, str | None], tuple[str | None, str | None, str]]:
        return _participant_scope_key(
            self.perspective, self.federate
        ), _object_scope_key(self.object_key, self.object_context)

    @model_validator(mode="after")
    def scoped_change(self) -> ObjectAttributeChangeResponse:
        if self.perspective is not None and (
            self.federate != self.perspective.label
            or self.source != self.perspective.source
        ):
            raise ValueError(
                "attributed object changes require matching source/perspective metadata"
            )
        if self.object_context is None:
            if self.observation_key is not None:
                raise ValueError(
                    "scoped change keys require an explicit object context"
                )
        elif self.observation_key != ObjectContext(
            self.object_context.identity_namespace, self.object_context.federation_id
        ).key(self.object_key):
            raise ValueError("object change key and federation context must agree")
        return self

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
        if any(value is not None for value in metadata) and any(
            value is None for value in metadata
        ):
            raise ValueError(
                "object snapshot budget metadata must be complete or unreported"
            )
        if (
            self.snapshot_capacity is not None
            and len(self.snapshots) > self.snapshot_capacity
        ):
            raise ValueError(
                "object perspectives exceed their advertised snapshot capacity"
            )
        keys = {row.cache_key for row in self.snapshots}
        if len(keys) != len(self.snapshots):
            raise ValueError("object perspective keys must be unique")
        if (
            self.omitted_changes or self.dropped_history_batches
        ) and not self.history_truncated:
            raise ValueError(
                "omitted object changes require an incomplete-history notice"
            )
        return self


class TimeScopeResponse(ObserverModel):
    source: NonEmptyText
    session_id: TimeId | None
    federation: str | None
    federate: JsonText | None
    perspective: ParticipantPerspectiveResponse | None = None

    @model_validator(mode="after")
    def consistent_perspective(self) -> TimeScopeResponse:
        if self.perspective is not None:
            if self.source != self.perspective.source:
                raise ValueError(
                    "time source must agree with its participant perspective"
                )
            if (
                self.perspective.attribution is not None
                and self.session_id != self.perspective.session_id
            ):
                raise ValueError(
                    "explicit time correlation requires the same reporting session"
                )
            if (
                self.session_id is not None
                and self.session_id != self.perspective.session_id
            ):
                raise ValueError(
                    "reported time session and participant provenance must agree"
                )
            if self.federate != self.perspective.label:
                raise ValueError(
                    "time attribution cannot borrow another participant label"
                )
        return self

    @model_serializer(mode="wrap")
    def preserve_scope(self, handler: SerializerFunctionWrapHandler):
        serialized = cast(dict[str, object], handler(self))
        serialized.update(
            session_id=self.session_id,
            federation=self.federation,
            federate=self.federate,
        )
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
    status: Literal[
        "pending", "granted", "failed", "cancelled", "ambiguous", "session-ended"
    ]
    outcome_sequence: SequenceNumber | None = None
    outcome_at: Annotated[float | None, Field(default=None, allow_inf_nan=False)]
    granted_time: TimeNumber | None = None
    latency_seconds: Annotated[
        float | None, Field(default=None, ge=0, allow_inf_nan=False)
    ]
    pending_seconds: Annotated[
        float | None, Field(default=None, ge=0, allow_inf_nan=False)
    ]
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
        def key(scope: TimeScopeResponse) -> tuple[str | None, ...]:
            return (
                ("perspective", scope.perspective.key)
                if scope.perspective
                else (
                    "legacy-scope",
                    scope.source,
                    scope.session_id,
                    scope.federation,
                    scope.federate,
                )
            )

        snapshots = {key(row.scope): row for row in self.snapshots}
        if (self.snapshot_capacity is None) != (self.dropped_snapshots is None):
            raise ValueError("time snapshot capacity/loss must be reported together")
        if (
            self.snapshot_capacity is not None
            and len(snapshots) > self.snapshot_capacity
        ):
            raise ValueError("time snapshots exceed their independent capacity")
        if (
            len(snapshots) != len(self.snapshots)
            or len(self.requests) > self.request_capacity
        ):
            raise ValueError(
                "time scopes must be unique and request capacity must be respected"
            )
        requests = {(key(row.scope), row.request_id) for row in self.requests}
        if len(requests) != len(self.requests):
            raise ValueError("duplicate scoped time request ID")
        for snapshot in self.snapshots:
            if any(
                sample.sequence > snapshot.last_sequence
                for sample in (snapshot.last_grant, snapshot.last_request)
                if sample
            ):
                raise ValueError("time samples cannot be newer than their snapshot")
            if (
                snapshot.state_sequence is not None
                and snapshot.state_sequence > snapshot.last_sequence
            ):
                raise ValueError(
                    "reported time state cannot be newer than its snapshot"
                )
        for row in self.requests:
            snapshot = snapshots.get(key(row.scope))
            if (
                row.scope.source == "unknown"
                or row.scope.session_id is None
                or row.scope.federate is None
                and not (row.scope.perspective and row.scope.perspective.attribution)
            ):
                raise ValueError(
                    "correlated requests require a known source, session and federate snapshot"
                )
            if row.snapshot_retained is False:
                if snapshot is not None or not self.dropped_snapshots:
                    raise ValueError(
                        "a missing time snapshot requires explicit independent expiry coverage"
                    )
            elif snapshot is None:
                raise ValueError(
                    "requests without explicit snapshot expiry require their snapshot"
                )
            if snapshot is not None and row.request_sequence > snapshot.last_sequence:
                raise ValueError("request is newer than its snapshot")
            if row.status == "pending":
                if any(
                    value is not None
                    for value in (
                        row.outcome_sequence,
                        row.outcome_at,
                        row.granted_time,
                        row.latency_seconds,
                    )
                ):
                    raise ValueError("pending requests cannot claim an outcome")
            else:
                if (
                    row.outcome_sequence is None
                    or not row.request_sequence < row.outcome_sequence
                    or snapshot is not None
                    and row.outcome_sequence > snapshot.last_sequence
                    or row.outcome_at is None
                    or row.pending_seconds is not None
                ):
                    raise ValueError(
                        "finished requests require ordered outcomes and no pending duration"
                    )
                if row.status != "granted" and (
                    row.granted_time is not None or row.latency_seconds is not None
                ):
                    raise ValueError(
                        "only matched grants can claim granted time or latency"
                    )
        return self


class DeclarationScopeResponse(ObserverModel):
    source: NonEmptyText
    session_id: TimeId | None
    federation: str | None
    federate: JsonText | None
    perspective: ParticipantPerspectiveResponse | None = None

    @model_validator(mode="after")
    def consistent_perspective(self) -> DeclarationScopeResponse:
        if self.perspective is not None and (
            self.source != self.perspective.source
            or self.perspective.attribution is not None
            and self.session_id != self.perspective.session_id
            or self.session_id is not None
            and self.session_id != self.perspective.session_id
            or self.federate != self.perspective.label
        ):
            raise ValueError("declaration scope and reporting perspective must agree")
        return self

    @model_serializer(mode="wrap")
    def preserve_scope(self, handler: SerializerFunctionWrapHandler):
        serialized = cast(dict[str, object], handler(self))
        serialized.update(
            session_id=self.session_id,
            federation=self.federation,
            federate=self.federate,
        )
        return serialized


class DeclarationSnapshotResponse(ObserverModel):
    scope: DeclarationScopeResponse
    kind: Literal["object", "interaction"]
    direction: Literal["publication", "subscription"]
    class_key: DeclarationId
    class_name: DeclarationId | None = None
    status: Literal["declared", "withdrawn", "unknown"]
    attribute_scope: Literal["selected", "all", "unknown", "not-applicable"]
    attributes: Annotated[
        Sequence[DeclarationId] | None, Field(default=None, max_length=4096)
    ]
    reason: JsonText | None = None
    sequence: SequenceNumber
    observed_at: Annotated[float, Field(allow_inf_nan=False)]

    @model_validator(mode="after")
    def consistent_attribute_scope(self) -> DeclarationSnapshotResponse:
        selected = self.attribute_scope == "selected"
        if selected != (self.attributes is not None):
            raise ValueError("only a selected scope has an explicit attribute array")
        if (self.kind == "interaction") != (self.attribute_scope == "not-applicable"):
            raise ValueError(
                "interaction attributes are not applicable, object attributes are not implicit"
            )
        if self.attributes is not None and len(set(self.attributes)) != len(
            self.attributes
        ):
            raise ValueError("selected attributes must be unique")
        return self


class DeclarationsResponse(ObserverModel):
    snapshots: Sequence[DeclarationSnapshotResponse]
    capacity: PositiveInt
    dropped_declarations: NonNegativeInt
    basis: Literal["last-reported-declarations"]

    @model_validator(mode="after")
    def unique_bounded_snapshots(self) -> DeclarationsResponse:
        keys = {
            (
                ("perspective", row.scope.perspective.key)
                if row.scope.perspective
                else (
                    "legacy-scope",
                    row.scope.source,
                    row.scope.session_id,
                    row.scope.federation,
                    row.scope.federate,
                ),
                row.kind,
                row.direction,
                row.class_key,
            )
            for row in self.snapshots
        }
        if len(keys) != len(self.snapshots) or len(keys) > self.capacity:
            raise ValueError(
                "declaration snapshots must be unique and bounded by capacity"
            )
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
        keys = {
            (
                row.source,
                row.session_id,
                row.identity_namespace,
                row.federation_id,
                row.relationship_id,
            )
            for row in self.snapshots
        }
        if len(keys) != len(self.snapshots) or len(keys) > self.capacity:
            raise ValueError(
                "relationship snapshots must be unique and bounded by capacity"
            )
        return self


class CoordinationSnapshotResponse(CoordinationObservationBase):
    kind: Literal["synchronization", "save", "restore"]
    phase: Literal[
        "requested",
        "registered",
        "registration-failed",
        "announced",
        "achieved",
        "begun",
        "completed",
        "failed",
    ]
    source: NonEmptyText
    sequence: SequenceNumber
    observed_at: Annotated[float, Field(allow_inf_nan=False)]

    def identity_key(
        self,
    ) -> tuple[str, str, str, str, str, str, str, tuple[str, str] | None]:
        subject = (
            (self.subject.participant_id, self.subject.membership_id)
            if self.subject
            else None
        )
        return (
            self.source,
            self.session_id,
            self.identity_namespace,
            self.federation_id,
            self.kind,
            self.operation_id,
            self.scope,
            subject,
        )


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
        if (
            len(latest) != len(self.snapshots)
            or len(latest) > self.capacity
            or len(sequences) > self.history_capacity
        ):
            raise ValueError(
                "coordination snapshots and transitions must be unique and capacity bounded"
            )
        if sequences != sorted(set(sequences)):
            raise ValueError(
                "coordination transition sequences must be unique and increasing"
            )
        for row in self.transitions:
            snapshot = latest.get(row.identity_key())
            if snapshot is not None and (
                snapshot.sequence < row.sequence
                or snapshot.sequence == row.sequence
                and snapshot != row
            ):
                raise ValueError(
                    "coordination snapshots must agree with the latest retained report"
                )
        return self


class InteractionFlowReportResponse(InteractionFlowObservationInput):
    source: NonEmptyText
    sequence: SequenceNumber
    observed_at: Annotated[float, Field(allow_inf_nan=False)]

    def correlation_key(
        self,
    ) -> tuple[str, str, str, tuple[str, str], tuple[str, str]] | None:
        if (
            self.message_id is None
            or self.identity_namespace is None
            or self.federation_id is None
            or self.producer is None
            or self.recipient is None
        ):
            return None
        return (
            self.identity_namespace,
            self.federation_id,
            self.message_id,
            (self.producer.participant_id, self.producer.membership_id),
            (self.recipient.participant_id, self.recipient.membership_id),
        )


class InteractionFlowRecordResponse(ObserverModel):
    record_id: SequenceNumber
    last_sequence: SequenceNumber
    status: Literal["uncorrelated", "send-only", "receive-only", "paired", "ambiguous"]
    send: InteractionFlowReportResponse | None = None
    receive: InteractionFlowReportResponse | None = None
    send_count: NonNegativeInt
    receive_count: NonNegativeInt
    arrival_delay_seconds: Annotated[
        float | None, Field(default=None, ge=0, allow_inf_nan=False)
    ]

    @model_validator(mode="after")
    def consistent_pair(self) -> InteractionFlowRecordResponse:
        sample = self.send or self.receive
        if sample is None or self.record_id != min(
            row.sequence for row in (self.send, self.receive) if row
        ):
            raise ValueError(
                "an interaction record starts at its first actual observation"
            )
        if self.send_count < int(self.send is not None) or self.receive_count < int(
            self.receive is not None
        ):
            raise ValueError(
                "interaction report counts cannot omit their stored evidence"
            )
        if (self.send is None) != (self.send_count == 0) or (self.receive is None) != (
            self.receive_count == 0
        ):
            raise ValueError("interaction report counts require corresponding evidence")
        if (
            self.send
            and self.send.kind != "send"
            or self.receive
            and self.receive.kind != "receive"
        ):
            raise ValueError(
                "interaction report direction disagrees with its record slot"
            )
        reports = [row for row in (self.send, self.receive) if row]
        if len({row.sequence for row in reports}) != len(reports):
            raise ValueError("a single observation cannot prove both send and receive")
        if self.last_sequence < max(row.sequence for row in reports):
            raise ValueError("interaction record cursor precedes its evidence")
        if (
            self.send_count <= 1
            and self.receive_count <= 1
            and self.last_sequence != max(row.sequence for row in reports)
        ):
            raise ValueError(
                "a record without duplicates ends at its last actual observation"
            )
        key = sample.correlation_key()
        if any(row.correlation_key() != key for row in reports):
            raise ValueError(
                "interaction endpoints and transmission identity must agree exactly"
            )
        expected = (
            "uncorrelated"
            if key is None
            else "ambiguous"
            if self.send_count > 1 or self.receive_count > 1
            else "paired"
            if self.send and self.receive
            else "send-only"
            if self.send
            else "receive-only"
        )
        if self.status != expected:
            raise ValueError(
                "interaction status must follow actual reports, not guessed delivery"
            )
        if key is None and self.send_count + self.receive_count != 1:
            raise ValueError("unidentified interaction observations cannot be merged")
        if self.arrival_delay_seconds is not None and (
            self.status != "paired"
            or self.send is None
            or self.receive is None
            or self.receive.sequence <= self.send.sequence
        ):
            raise ValueError(
                "arrival delay requires an unambiguous send-before-receive pair"
            )
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
        samples = [
            sample
            for row in self.records
            if (sample := row.send or row.receive) is not None
        ]
        keys = [sample.correlation_key() for sample in samples]
        known = [key for key in keys if key is not None]
        sequences = [
            report.sequence
            for row in self.records
            for report in (row.send, row.receive)
            if report
        ]
        if (
            len(ids) != len(self.records)
            or len(ids) > self.capacity
            or len(set(known)) != len(known)
        ):
            raise ValueError(
                "interaction flow records must be unique and capacity bounded"
            )
        if len(set(sequences)) != len(sequences):
            raise ValueError(
                "interaction observation evidence cannot appear in multiple records"
            )
        if (
            sum(row.send_count for row in self.records) > self.send_observations
            or sum(row.receive_count for row in self.records)
            > self.receive_observations
        ):
            raise ValueError("interaction totals cannot omit retained report counts")
        if self.dropped_records == 0 and (
            sum(row.send_count for row in self.records) != self.send_observations
            or sum(row.receive_count for row in self.records)
            != self.receive_observations
        ):
            raise ValueError(
                "without index loss, totals equal retained observation counts"
            )
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


OwnershipReportResponse = Annotated[
    OwnershipStateReportResponse
    | OwnershipRequestReportResponse
    | OwnershipOutcomeReportResponse,
    Field(discriminator="kind"),
]


class OwnershipTicketResponse(ObserverModel):
    request: OwnershipRequestReportResponse
    status: Literal["pending", "completed", "failed", "cancelled", "ambiguous"]
    last_sequence: SequenceNumber
    outcome: OwnershipOutcomeReportResponse | None = None
    duplicate_requests: NonNegativeInt

    @model_validator(mode="after")
    def consistent_ticket(self) -> OwnershipTicketResponse:
        if self.status == "pending":
            valid = (
                self.outcome is None
                and self.duplicate_requests == 0
                and self.last_sequence == self.request.sequence
            )
        elif self.status == "ambiguous":
            valid = (
                self.outcome is None
                and self.duplicate_requests > 0
                and self.last_sequence > self.request.sequence
            )
        else:
            valid = (
                self.duplicate_requests == 0
                and self.outcome is not None
                and self.outcome.outcome == self.status
                and self.outcome.request_key(self.outcome.source)
                == self.request.request_key(self.request.source)
                and self.last_sequence == self.outcome.sequence
                and self.outcome.sequence > self.request.sequence
            )
        if not valid:
            raise ValueError(
                "ownership request outcomes require exact scoped tickets; ambiguity never establishes completion"
            )
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
        requests = {
            row.request.request_key(row.request.source): row for row in self.requests
        }
        sequences = [row.sequence for row in self.transitions]
        if (
            len(states) != len(self.states)
            or len(states) > self.capacity
            or len(requests) != len(self.requests)
            or len(requests) > self.request_capacity
            or len(sequences) > self.history_capacity
            or sequences != sorted(set(sequences))
        ):
            raise ValueError(
                "ownership scopes, tickets and transition sequences must be unique and capacity bounded"
            )
        for row in self.transitions:
            if (
                isinstance(row, OwnershipStateReportResponse)
                and (current := states.get(row.scope_key(row.source))) is not None
                and (
                    current.sequence < row.sequence
                    or current.sequence == row.sequence
                    and current != row
                )
            ):
                raise ValueError(
                    "ownership snapshots must agree with their latest retained report"
                )
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


RegionReportResponse = Annotated[
    RegionStateReportResponse
    | RegionAssociationReportResponse
    | RegionRoutingReportResponse,
    Field(discriminator="kind"),
]


def region_report_key(row: RegionReportResponse) -> tuple[str, str, str, str, str, str]:
    identity = (
        row.region_id
        if row.kind == "region"
        else row.association_id
        if row.kind == "association"
        else row.routing_id
    )
    return (
        row.source,
        row.session_id,
        row.identity_namespace,
        row.federation_id,
        row.kind,
        identity,
    )


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
        if (
            len(snapshots) != len(self.snapshots)
            or len(snapshots) > self.capacity
            or len(sequences) > self.history_capacity
            or sequences != sorted(set(sequences))
        ):
            raise ValueError(
                "region snapshots and arrival sequences must be unique and capacity bounded"
            )
        for row in self.transitions:
            current = snapshots.get(region_report_key(row))
            if current is not None and (
                current.sequence < row.sequence
                or current.sequence == row.sequence
                and current != row
            ):
                raise ValueError(
                    "region snapshots must agree with their latest retained report"
                )
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


MomReportResponse = Annotated[
    MomMembersReportResponse | MomServiceReportResponse | MomCounterReportResponse,
    Field(discriminator="kind"),
]


def mom_report_key(row: MomReportResponse) -> tuple[str, str, str, str, str, str]:
    return (
        row.source,
        row.session_id,
        row.identity_namespace,
        row.federation_id,
        row.kind,
        row.report_id,
    )


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
        if (
            len(snapshots) != len(self.snapshots)
            or len(snapshots) > self.capacity
            or len(sequences) > self.history_capacity
            or sequences != sorted(set(sequences))
        ):
            raise ValueError(
                "management snapshots and arrivals must be unique and capacity bounded"
            )
        for row in self.transitions:
            current = snapshots.get(mom_report_key(row))
            if current is not None and (
                current.sequence < row.sequence
                or current.sequence == row.sequence
                and current != row
            ):
                raise ValueError(
                    "management snapshots must agree with their latest retained report"
                )
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


FomReportResponse = Annotated[
    FomObjectReportResponse
    | FomInteractionReportResponse
    | FomDatatypeReportResponse
    | FomDimensionReportResponse,
    Field(discriminator="kind"),
]


def fom_report_key(
    row: FomReportResponse,
) -> tuple[str, str, str, str, str, str, str, str]:
    return (
        row.source,
        row.session_id,
        row.identity_namespace,
        row.federation_id,
        row.model_id,
        row.revision,
        row.kind,
        row.element_id,
    )


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
        if (
            len(snapshots) != len(self.snapshots)
            or len(snapshots) > self.capacity
            or len(sequences) > self.history_capacity
            or sequences != sorted(set(sequences))
        ):
            raise ValueError(
                "FOM metadata snapshots and arrivals must be unique and capacity bounded"
            )
        for row in self.transitions:
            current = snapshots.get(fom_report_key(row))
            if current is not None and (
                current.sequence < row.sequence
                or current.sequence == row.sequence
                and current != row
            ):
                raise ValueError(
                    "FOM snapshots must agree with their latest retained report"
                )
        return self


class InspectorsResponse(ObserverModel):
    objects: Annotated[Sequence[ObjectInspectionResponse], Field(default_factory=tuple)]
    interactions: Annotated[
        Sequence[InteractionInspectionResponse], Field(default_factory=tuple)
    ]
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
        if self.retained_records > self.capacity or self.dropped_records != sum(
            (
                self.count_limit_drops,
                self.byte_limit_drops,
                self.oversized_drops,
                self.encoding_drops,
                self.window_drops,
            )
        ):
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
        if (
            self.max_record_bytes > self.max_bytes
            or self.accounted_bytes > self.max_bytes
            or self.accounted_bytes
            != sum(row.accounted_bytes for row in self.collections)
            or len(names) != len(set(names))
            or set(names) & set(self.unbudgeted_collections)
            or len(self.unbudgeted_collections) != len(set(self.unbudgeted_collections))
        ):
            raise ValueError(
                "storage budget must cover its exact, unique accounted collections"
            )
        return self


class StorageCoverageResponse(ObserverModel):
    evidence: StorageBudgetResponse
    context: StorageBudgetResponse


class ObserverLifetimeResponse(ObserverModel):
    """Storage incarnation and atomic mutation cut; not provider authority."""

    instance_id: Annotated[JsonText, Field(min_length=1, max_length=128)]
    revision: NonNegativeInt


class StreamCursorResponse(ObserverLifetimeResponse):
    after: NonNegativeInt


class StateResponse(ObserverModel):
    observer_lifetime: ObserverLifetimeResponse | None = None
    contract_version: NonEmptyText
    schema_version: NonEmptyText
    status: NonEmptyText
    context: Annotated[Mapping[str, JsonValue], Field(default_factory=dict)]
    live_metrics: LiveMetricsResponse
    retention: RetentionResponse
    adapters: Annotated[Sequence[AdapterResponse], Field(default_factory=tuple)]
    capabilities: Annotated[
        Mapping[str, CapabilityResponse], Field(default_factory=dict)
    ]
    federate_roster: Annotated[
        Sequence[FederateInspectionResponse], Field(default_factory=tuple)
    ]
    inspectors: InspectorsResponse
    normalized_events: Annotated[Sequence[EventEnvelope] | None, Field(default=None)]
    storage: StorageCoverageResponse | None = None

    @model_validator(mode="after")
    def consistent_storage(self) -> StateResponse:
        roster = [
            ("perspective", row.perspective.key)
            if row.perspective
            else ("legacy-reporter", row.source, row.federate)
            for row in self.federate_roster
        ]
        objects = [
            _object_scope_key(row.object_key, row.object_context)
            for row in self.inspectors.objects
        ]
        if len(set(roster)) != len(roster) or len(set(objects)) != len(objects):
            raise ValueError("observed roster joins and object contexts must be unique")
        feature = self.capabilities.get("storage_inspection")
        if self.storage is None:
            if feature is not None and feature.state in {"available", "partial"}:
                raise ValueError(
                    "available storage inspection requires its coverage model"
                )
            return self
        if not WIRE_REQUIRED_FIELDS["StateResponse"].issubset(
            self.model_fields_set
        ) or not WIRE_REQUIRED_FIELDS["InspectorsResponse"].issubset(
            self.inspectors.model_fields_set
        ):
            raise ValueError("storage-backed states require explicit wire collections")
        rows = {row.collection: row for row in self.storage.evidence.collections}
        expected = {
            "events": self.retention.retained_events,
            "objects": len(self.inspectors.objects),
            "interactions": len(self.inspectors.interactions),
            "federates": len(self.federate_roster),
        }
        data = self.inspectors
        if data.object_observations is not None:
            expected["object_snapshots"] = len(data.object_observations.snapshots)
        if data.time is not None:
            expected.update(
                time_snapshots=len(data.time.snapshots),
                time_requests=len(data.time.requests),
            )
        for name, count in (
            (
                "declarations",
                len(data.declarations.snapshots) if data.declarations else None,
            ),
            (
                "relationships",
                len(data.relationships.snapshots) if data.relationships else None,
            ),
            (
                "interaction_flow",
                len(data.interaction_flow.records) if data.interaction_flow else None,
            ),
        ):
            if count is not None:
                expected[name] = count
        for name, reports in (
            ("regions", data.regions),
            ("mom", data.mom),
            ("fom", data.fom),
        ):
            if reports is not None:
                expected[name + "_snapshots"] = len(reports.snapshots)
                expected[name + "_history"] = len(reports.transitions)
        if data.coordination is not None:
            expected.update(
                coordination_snapshots=len(data.coordination.snapshots),
                coordination_history=len(data.coordination.transitions),
            )
        if data.ownership is not None:
            expected.update(
                ownership_states=len(data.ownership.states),
                ownership_requests=len(data.ownership.requests),
                ownership_history=len(data.ownership.transitions),
            )
        unbudgeted = set(self.storage.evidence.unbudgeted_collections)
        for name, count in expected.items():
            if (
                name not in rows
                and name not in unbudgeted
                or name in rows
                and rows[name].retained_records != count
            ):
                raise ValueError(
                    "storage coverage must identify each delivered collection and its exact retained count"
                )
        context = next(
            (
                row
                for row in self.storage.context.collections
                if row.collection == "context"
            ),
            None,
        )
        if context is None or context.retained_records != len(self.context):
            raise ValueError(
                "context storage coverage must match delivered context keys"
            )
        return self


class AnalyticsWindow(ObserverModel):
    """Consistent retained observations/roster, without unrelated inspectors."""

    contract_version: NonEmptyText
    retention: RetentionResponse
    federate_roster: Sequence[FederateInspectionResponse]
    normalized_events: Sequence[EventEnvelope]
    storage: StorageCoverageResponse | None = None
    participant_provenance: bool = False

    @model_validator(mode="after")
    def consistent_window(self) -> AnalyticsWindow:
        retention = self.retention
        sequences = [row.sequence for row in self.normalized_events]
        if self.contract_version != "umbra-observer-contract-v1":
            raise ValueError("analytics window has an incompatible contract")
        if (
            len(sequences) != retention.retained_events
            or sequences != sorted(set(sequences))
            or len(sequences) > retention.max_events
        ):
            raise ValueError(
                "analytics window envelopes must be unique, ordered and capacity bounded"
            )
        if sequences and (
            sequences[0] != retention.first_retained_sequence
            or sequences[-1] != retention.last_sequence
            or len(sequences) != sequences[-1] - sequences[0] + 1
            or sequences[-1] != retention.next_sequence - 1
        ):
            raise ValueError(
                "analytics window must include its complete retained cursor range"
            )
        if retention.next_sequence <= retention.last_sequence:
            raise ValueError("analytics next cursor must follow retained history")
        if len(
            {
                _participant_scope_key(row.perspective, row.federate)
                for row in self.federate_roster
            }
        ) != len(self.federate_roster):
            raise ValueError("analytics roster identities must be unique")
        if self.storage is not None:
            rows = {row.collection: row for row in self.storage.evidence.collections}
            for name, count in (
                ("events", retention.retained_events),
                ("federates", len(self.federate_roster)),
            ):
                if name in rows and rows[name].retained_records != count:
                    raise ValueError(
                        "projected storage coverage must agree with envelopes and roster"
                    )
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
    perspective: ParticipantPerspectiveResponse | None = None
    roster_retained: bool | None = None
    federate: NonEmptyJsonText
    role: str | None
    handle: str | None
    status: NonEmptyText
    source: NonEmptyText
    last_seen: float | None
    event_count: NonNegativeInt
    object_updates: NonNegativeInt
    interaction_count: NonNegativeInt
    error_count: NonNegativeInt
    object_keys: Sequence[JsonText]
    latest_logical_time: int | float | None

    @model_validator(mode="after")
    def consistent_perspective(self) -> FederateActivity:
        if self.perspective is not None and (
            self.federate != self.perspective.label
            or self.source != self.perspective.source
        ):
            raise ValueError("participant activity and reporting provenance must agree")
        return self


class TimeObservation(ObserverModel):
    perspective: ParticipantPerspectiveResponse | None = None
    sequence: SequenceNumber
    federate: JsonText | None
    logical_time: Annotated[int | float, Field(allow_inf_nan=False)]
    observed_at: float
    event_type: NonEmptyText
    operation: str | None
    observation_kind: Literal["timestamp", "request", "grant", "state"] = "timestamp"

    @model_validator(mode="after")
    def consistent_perspective(self) -> TimeObservation:
        if self.perspective is not None and self.federate != self.perspective.label:
            raise ValueError(
                "logical-time label must agree with its reporting perspective"
            )
        return self


class AnalyticsResponse(ObserverModel):
    scoped_perspective: str | None = None
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

    @model_validator(mode="after")
    def unique_perspectives(self) -> AnalyticsResponse:
        keys = {
            _participant_scope_key(row.perspective, row.federate)
            for row in self.participants
        }
        if len(keys) != len(self.participants):
            raise ValueError("analytics reporting perspectives must be unique")
        if (
            sum(row.event_count for row in self.participants) + self.unattributed_events
            != self.events_in_window
        ):
            raise ValueError(
                "participant activity must account for the exact observation window"
            )
        if self.scoped_perspective is not None and (
            any(
                row.perspective is None
                or row.perspective.key != self.scoped_perspective
                for row in self.participants
            )
            or any(
                row.perspective is None
                or row.perspective.key != self.scoped_perspective
                for row in self.logical_times
            )
        ):
            raise ValueError(
                "exact analytics filters cannot borrow another reporting perspective"
            )
        return self


class HealthResponse(ObserverModel):
    service: NonEmptyText
    status: NonEmptyText
    contract_version: NonEmptyText
    schema_version: NonEmptyText
    event_count: NonNegativeInt
    retention: RetentionResponse
    capabilities: Annotated[
        Mapping[str, CapabilityResponse], Field(default_factory=dict)
    ]


class MetricsResponse(ObserverModel):
    contract_version: NonEmptyText
    schema_version: NonEmptyText
    status: NonEmptyText
    live_metrics: LiveMetricsResponse
    retention: RetentionResponse
    adapters: Annotated[Sequence[AdapterResponse], Field(default_factory=tuple)]


class EventPageResponse(ObserverModel):
    observer_lifetime: ObserverLifetimeResponse | None = None
    reset: bool = False
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
    resumption: bool = False
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
    capabilities: Annotated[
        Mapping[str, CapabilityResponse], Field(default_factory=dict)
    ]
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
    capabilities: Annotated[
        Mapping[str, CapabilityResponse], Field(default_factory=dict)
    ]
    features: Annotated[Sequence[str], Field(default_factory=tuple)]


class JsonObjectResponse(RootModel[JsonObject]):
    """Validated object response for the JSON schema endpoint."""


class StateStreamMessage(ObserverModel):
    state: StateResponse
    cursor: StreamCursorResponse | None = None

    @model_validator(mode="after")
    def consistent_cursor(self) -> StateStreamMessage:
        if self.cursor is not None:
            lifetime = self.state.observer_lifetime
            if (
                lifetime is None
                or self.cursor.instance_id != lifetime.instance_id
                or self.cursor.revision != lifetime.revision
                or self.cursor.after >= self.state.retention.next_sequence
            ):
                raise ValueError(
                    "stream cursor must describe this exact observer state cut"
                )
            if (
                self.state.normalized_events is not None
                and self.cursor.after != self.state.retention.next_sequence - 1
            ):
                raise ValueError(
                    "complete snapshots must advance through their delivered window"
                )
        return self


class SnapshotMessage(StateStreamMessage):
    type: Literal["snapshot"]
    resume: Literal["fresh", "resumed", "reset", "gap"] | None = None


class EventsMessage(StateStreamMessage):
    type: Literal["events"]
    events: Annotated[Sequence[EventEnvelope], Field(default_factory=tuple)]

    @model_validator(mode="after")
    def consistent_events(self) -> EventsMessage:
        sequences = [row.sequence for row in self.events]
        retention = self.state.retention
        if sequences != sorted(set(sequences)) or any(
            not retention.retained_events
            or sequence < retention.first_retained_sequence
            or sequence > retention.last_sequence
            or self.cursor is not None
            and sequence > self.cursor.after
            for sequence in sequences
        ):
            raise ValueError(
                "stream envelopes must be ordered unique records inside this retained state cut"
            )
        return self


class HeartbeatMessage(ObserverModel):
    type: Literal["heartbeat"]
    cursor: StreamCursorResponse | None = None


class GapMessage(ObserverModel):
    type: Literal["gap"]
    retention: RetentionGapResponse
    observer_lifetime: ObserverLifetimeResponse | None = None


class ErrorMessage(ObserverModel):
    type: Literal["error"]
    message: str


StreamMessage = (
    SnapshotMessage | EventsMessage | HeartbeatMessage | GapMessage | ErrorMessage
)


def event_input_to_raw_event(event: EventInput) -> RawObserverEvent:
    """Flatten a validated request at the core's raw-event ingress seam."""

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
    "ObserverLifetimeResponse",
    "ResourceDescription",
    "RetentionGapResponse",
    "RetentionResponse",
    "SnapshotMessage",
    "StateResponse",
    "StatusRequest",
    "StreamCursorResponse",
    "StreamDescription",
    "StreamMessage",
    "event_input_to_raw_event",
    "json_model",
]
