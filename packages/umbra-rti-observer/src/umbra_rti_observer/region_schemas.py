"""Typed request models for explicit region observations."""

from __future__ import annotations

from collections.abc import Sequence
from typing import Annotated, Literal

from pydantic import Field, StringConstraints, field_validator, model_validator

from .identities import decimal_bound, observation_scope_from_fields
from .regions import (
    DimensionRange,
    RegionContext,
    RegionData,
    RegionExtent,
    dimension_range_from_fields,
    region_association_from_fields,
    region_context_from_fields,
    region_extent_from_fields,
    region_routing_from_fields,
    region_state_from_fields,
)
from .schema_common import (
    DeclarationId,
    JsonText,
    ObserverModel,
    ParticipantIdentityResponse,
    TimeId,
)

DecimalBound = Annotated[
    str,
    StringConstraints(
        strip_whitespace=False,
        min_length=1,
        max_length=128,
        pattern=r"^(0|[1-9][0-9]*)$",
    ),
    Field(json_schema_extra={"not": {"pattern": r"[^0-9]"}}),
]


class DimensionRangeResponse(ObserverModel):
    dimension_id: DeclarationId
    lower_bound: DecimalBound
    upper_bound: DecimalBound
    dimension_upper_bound: DecimalBound | None = None

    @field_validator(
        "lower_bound", "upper_bound", "dimension_upper_bound", mode="before"
    )
    @classmethod
    def canonical_decimal(cls, value: object) -> str | None:
        return decimal_bound(value, optional=True)

    @model_validator(mode="after")
    def ordered_bounds(self) -> DimensionRangeResponse:
        if (
            int(self.lower_bound) > int(self.upper_bound)
            or self.dimension_upper_bound is not None
            and int(self.upper_bound) > int(self.dimension_upper_bound)
        ):
            raise ValueError(
                "range bounds must be ordered and inside an explicitly reported dimension limit"
            )
        return self

    def to_domain(self) -> DimensionRange:
        return dimension_range_from_fields(
            self.dimension_id,
            self.lower_bound,
            self.upper_bound,
            self.dimension_upper_bound,
        )


class RegionExtentResponse(ObserverModel):
    extent_id: DeclarationId
    dimensions: Annotated[Sequence[DimensionRangeResponse], Field(max_length=256)]

    def to_domain(self) -> RegionExtent:
        return region_extent_from_fields(
            self.extent_id, (row.to_domain() for row in self.dimensions)
        )


class RegionObservationBase(ObserverModel):
    session_id: TimeId
    identity_namespace: DeclarationId
    federation_id: DeclarationId
    evidence: Literal["declared", "observed", "unknown"]
    subject: ParticipantIdentityResponse | None = None
    reason: Annotated[JsonText | None, Field(default=None, max_length=4096)]

    def _context(self) -> RegionContext:
        scope = observation_scope_from_fields(
            self.session_id,
            self.identity_namespace,
            self.federation_id,
        )
        subject = self.subject.to_domain() if self.subject is not None else None
        return region_context_from_fields(scope, self.evidence, subject, self.reason)

    def to_domain(self) -> RegionData:
        context = self._context()
        if isinstance(self, RegionStateObservation):
            extents = (
                (extent.to_domain() for extent in self.extents)
                if self.extents is not None
                else None
            )
            return region_state_from_fields(
                context, self.region_id, self.status, extents
            )
        if isinstance(self, RegionAssociationObservation):
            return region_association_from_fields(
                context,
                self.association_id,
                self.status,
                self.target_kind,
                self.target_id,
                self.direction,
                self.region_ids,
                self.attribute_keys,
            )
        if isinstance(self, RegionRoutingObservation):
            producer = self.producer.to_domain() if self.producer is not None else None
            recipient = (
                self.recipient.to_domain() if self.recipient is not None else None
            )
            return region_routing_from_fields(
                context,
                self.routing_id,
                self.status,
                self.region_ids,
                self.target_kind,
                self.target_id,
                producer,
                recipient,
            )
        raise TypeError(
            "region input must select a named state, association, or routing kind"
        )

    @model_validator(mode="after")
    def explicit_region(self) -> RegionObservationBase:
        self.to_domain()
        return self


class RegionStateObservation(RegionObservationBase):
    kind: Literal["region"]
    region_id: DeclarationId
    status: Literal["active", "deleted", "unknown"]
    extents: Annotated[
        Sequence[RegionExtentResponse] | None, Field(default=None, max_length=256)
    ]


class RegionAssociationObservation(RegionObservationBase):
    kind: Literal["association"]
    association_id: DeclarationId
    status: Literal["active", "ended", "unknown"]
    target_kind: Literal["object", "interaction"]
    target_id: DeclarationId
    direction: Literal["publication", "subscription", "update", "send", "unknown"]
    region_ids: Annotated[
        Sequence[DeclarationId] | None, Field(default=None, max_length=4096)
    ]
    attribute_keys: Annotated[
        Sequence[DeclarationId] | None, Field(default=None, max_length=4096)
    ]


class RegionRoutingObservation(RegionObservationBase):
    kind: Literal["routing"]
    routing_id: DeclarationId
    status: Literal["active", "ended", "unknown"]
    region_ids: Annotated[
        Sequence[DeclarationId] | None, Field(default=None, max_length=4096)
    ]
    target_kind: Literal["object", "interaction"] | None = None
    target_id: DeclarationId | None = None
    producer: ParticipantIdentityResponse | None = None
    recipient: ParticipantIdentityResponse | None = None


RegionObservationInput = Annotated[
    RegionStateObservation | RegionAssociationObservation | RegionRoutingObservation,
    Field(discriminator="kind"),
]
