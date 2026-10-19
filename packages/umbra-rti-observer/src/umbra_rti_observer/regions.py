"""Explicit region extents, associations and routing reports, not overlap inference."""

# pyright: strict
from __future__ import annotations

from collections.abc import Mapping
from dataclasses import dataclass
from typing import Literal, TypeAlias, cast

from .identities import (
    ObservationScope,
    ParticipantIdentity,
    decimal_bound,
    identifier_array,
    identity_text,
    observation_reason,
    observation_scope,
    participant_identity,
)
from .inspection_index import LatestHistory
from .models import EventRecord, JsonObject, to_json_object
from .storage import EvidenceBudget

RegionEvidence = Literal["declared", "observed", "unknown"]
AssociationStatus = Literal["active", "ended", "unknown"]
TargetKind = Literal["object", "interaction"]
RegionKey = tuple[str, str, str, str, str, str]


@dataclass(frozen=True, slots=True)
class DimensionRange:
    dimension_id: str
    lower_bound: str
    upper_bound: str
    dimension_upper_bound: str | None

    def to_json(self) -> JsonObject:
        return {"dimension_id": self.dimension_id, "lower_bound": self.lower_bound,
                "upper_bound": self.upper_bound, "dimension_upper_bound": self.dimension_upper_bound}


@dataclass(frozen=True, slots=True)
class RegionExtent:
    extent_id: str
    dimensions: tuple[DimensionRange, ...]

    def to_json(self) -> JsonObject:
        return {"extent_id": self.extent_id, "dimensions": [row.to_json() for row in self.dimensions]}


def region_extents(raw: object) -> tuple[RegionExtent, ...] | None:
    if raw is None:
        return None
    if not isinstance(raw, (list, tuple)):
        raise TypeError("region extents must be an explicit array or unknown")
    items = cast(list[object] | tuple[object, ...], raw)
    if len(items) > 256:
        raise ValueError("region extent reports contain at most 256 extents")
    extents: list[RegionExtent] = []
    total_dimensions = 0
    for item in items:
        if not isinstance(item, Mapping):
            raise TypeError("extent must be a named extent record")
        extent = cast(Mapping[str, object], item)
        if set(extent) - {"extent_id", "dimensions"}:
            raise ValueError("unknown extent fields")
        extent_id = cast(str, identity_text(extent.get("extent_id")))
        raw_dimensions = extent.get("dimensions")
        if not isinstance(raw_dimensions, (list, tuple)):
            raise TypeError("extent dimensions require an explicit array")
        dimensions: list[DimensionRange] = []
        dimension_items = cast(list[object] | tuple[object, ...], raw_dimensions)
        if len(dimension_items) > 256:
            raise ValueError("an extent can report at most 256 dimensions")
        total_dimensions += len(dimension_items)
        if total_dimensions > 4096:
            raise ValueError("a region report can contain at most 4096 dimension ranges")
        for raw_dimension in dimension_items:
            if not isinstance(raw_dimension, Mapping):
                raise TypeError("dimension range must be a named record")
            values = cast(Mapping[str, object], raw_dimension)
            if set(values) - {"dimension_id", "lower_bound", "upper_bound", "dimension_upper_bound"}:
                raise ValueError("unknown dimension range fields")
            lower = cast(str, decimal_bound(values.get("lower_bound")))
            upper = cast(str, decimal_bound(values.get("upper_bound")))
            limit = decimal_bound(values.get("dimension_upper_bound"), optional=True)
            if int(lower) > int(upper) or limit is not None and int(upper) > int(limit):
                raise ValueError("region ranges must be ordered and within an explicitly reported dimension limit")
            dimensions.append(DimensionRange(cast(str, identity_text(values.get("dimension_id"))), lower, upper, limit))
        if len({row.dimension_id for row in dimensions}) != len(dimensions):
            raise ValueError("dimension IDs must be unique within an extent")
        extents.append(RegionExtent(extent_id, tuple(dimensions)))
    if len({row.extent_id for row in extents}) != len(extents):
        raise ValueError("extent IDs must be unique within a region report")
    return tuple(extents)


@dataclass(frozen=True, slots=True)
class RegionContext:
    scope: ObservationScope
    evidence: RegionEvidence
    subject: ParticipantIdentity | None
    reason: str | None

    def to_json(self) -> JsonObject:
        return {**self.scope.to_json(), "evidence": self.evidence, "subject": self.subject.to_json() if self.subject else None, "reason": self.reason}


@dataclass(frozen=True, slots=True)
class RegionStateData:
    context: RegionContext
    region_id: str
    status: Literal["active", "deleted", "unknown"]
    extents: tuple[RegionExtent, ...] | None

    def record_key(self) -> tuple[str, str]:
        return "region", self.region_id

    def to_json(self) -> JsonObject:
        return {**self.context.to_json(), "kind": "region", "region_id": self.region_id, "status": self.status,
                "extents": [row.to_json() for row in self.extents] if self.extents is not None else None}


@dataclass(frozen=True, slots=True)
class RegionAssociationData:
    context: RegionContext
    association_id: str
    status: AssociationStatus
    target_kind: TargetKind
    target_id: str
    direction: Literal["publication", "subscription", "update", "send", "unknown"]
    region_ids: tuple[str, ...] | None
    attribute_keys: tuple[str, ...] | None

    def record_key(self) -> tuple[str, str]:
        return "association", self.association_id

    def to_json(self) -> JsonObject:
        return {**self.context.to_json(), "kind": "association", "association_id": self.association_id, "status": self.status,
                "target_kind": self.target_kind, "target_id": self.target_id, "direction": self.direction,
                "region_ids": list(self.region_ids) if self.region_ids is not None else None,
                "attribute_keys": list(self.attribute_keys) if self.attribute_keys is not None else None}


@dataclass(frozen=True, slots=True)
class RegionRoutingData:
    context: RegionContext
    routing_id: str
    status: AssociationStatus
    region_ids: tuple[str, ...] | None
    target_kind: TargetKind | None
    target_id: str | None
    producer: ParticipantIdentity | None
    recipient: ParticipantIdentity | None

    def record_key(self) -> tuple[str, str]:
        return "routing", self.routing_id

    def to_json(self) -> JsonObject:
        return {**self.context.to_json(), "kind": "routing", "routing_id": self.routing_id, "status": self.status,
                "region_ids": list(self.region_ids) if self.region_ids is not None else None,
                "target_kind": self.target_kind, "target_id": self.target_id,
                "producer": self.producer.to_json() if self.producer else None, "recipient": self.recipient.to_json() if self.recipient else None}


RegionData: TypeAlias = RegionStateData | RegionAssociationData | RegionRoutingData


def region_data(event: Mapping[str, object]) -> RegionData | None:
    raw = event.get("region_observation")
    if raw is None:
        return None
    if not isinstance(raw, Mapping):
        raise TypeError("region_observation must be a named record")
    values = cast(Mapping[str, object], raw)
    evidence, kind, status = values.get("evidence"), values.get("kind"), values.get("status")
    if evidence not in ("declared", "observed", "unknown"):
        raise ValueError("region evidence must explicitly distinguish declared, observed and unknown")
    subject = values.get("subject")
    context = RegionContext(observation_scope(values), evidence, participant_identity(subject) if subject is not None else None,
                            observation_reason(values.get("reason")))
    common = {"kind", "session_id", "identity_namespace", "federation_id", "evidence", "status", "subject", "reason"}
    if kind == "region":
        if set(values) - (common | {"region_id", "extents"}) or status not in ("active", "deleted", "unknown"):
            raise ValueError("invalid region state fields/status")
        return RegionStateData(context, cast(str, identity_text(values.get("region_id"))), status, region_extents(values.get("extents")))
    if status not in ("active", "ended", "unknown"):
        raise ValueError("association/routing status must be explicit")
    if kind == "association":
        if set(values) - (common | {"association_id", "target_kind", "target_id", "direction", "region_ids", "attribute_keys"}):
            raise ValueError("unknown region association fields")
        target_kind, direction = values.get("target_kind"), values.get("direction")
        if target_kind not in ("object", "interaction") or direction not in ("publication", "subscription", "update", "send", "unknown"):
            raise ValueError("association target kind and direction must be explicit")
        attributes = identifier_array(values.get("attribute_keys"))
        if target_kind == "interaction" and attributes is not None:
            raise ValueError("interaction associations cannot report object attribute selections")
        return RegionAssociationData(context, cast(str, identity_text(values.get("association_id"))), status, target_kind,
                                     cast(str, identity_text(values.get("target_id"))), direction, identifier_array(values.get("region_ids")), attributes)
    if kind != "routing" or set(values) - (common | {"routing_id", "region_ids", "target_kind", "target_id", "producer", "recipient"}):
        raise ValueError("unknown region observation kind/fields")
    target_kind = values.get("target_kind")
    if target_kind not in (None, "object", "interaction"):
        raise ValueError("invalid routing target kind")
    producer, recipient = values.get("producer"), values.get("recipient")
    return RegionRoutingData(context, cast(str, identity_text(values.get("routing_id"))), status, identifier_array(values.get("region_ids")), target_kind,
                             identity_text(values.get("target_id"), optional=True), participant_identity(producer) if producer is not None else None,
                             participant_identity(recipient) if recipient is not None else None)


@dataclass(frozen=True, slots=True)
class RegionReport:
    data: RegionData
    source: str
    sequence: int
    observed_at: float

    def key(self) -> RegionKey:
        return (*self.data.context.scope.key(self.source), *self.data.record_key())

    def to_json(self) -> JsonObject:
        return {**self.data.to_json(), "source": self.source, "sequence": self.sequence, "observed_at": self.observed_at}


class RegionViews:
    def __init__(self, capacity: int, history_capacity: int, *, budget: EvidenceBudget | None = None) -> None:
        self._index: LatestHistory[RegionKey, RegionReport] = LatestHistory(capacity, history_capacity, name="regions", budget=budget)

    def record(self, event: EventRecord) -> None:
        data = region_data(event)
        if data is not None:
            row = RegionReport(data, event.source, event.sequence, event.observed_at)
            self._index.record(row.key(), row)

    def clear(self) -> None:
        self._index.clear()

    def to_json(self) -> JsonObject:
        return to_json_object({"snapshots": [row.to_json() for row in self._index.snapshots],
                               "transitions": [row.to_json() for row in self._index.transitions],
                               "capacity": self._index.capacity, "history_capacity": self._index.history_capacity,
                               "dropped_snapshots": self._index.dropped_snapshots, "dropped_transitions": self._index.dropped_transitions,
                               "basis": "explicit-region-observations"})
