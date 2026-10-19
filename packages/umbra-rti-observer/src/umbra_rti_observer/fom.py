"""Adapter-supplied FOM metadata; no provider file parsing or value decoding."""

# pyright: strict
from __future__ import annotations

from collections.abc import Mapping
from dataclasses import dataclass
from typing import Literal, TypeAlias, cast

from .identities import (
    ObservationScope,
    decimal_bound,
    identity_text,
    observation_reason,
    observation_scope,
)
from .inspection_index import LatestHistory
from .models import EventRecord, JsonObject, to_json_object
from .storage import EvidenceBudget

FomKind = Literal["object-class", "interaction-class", "datatype", "dimension"]
FomKey = tuple[str, str, str, str, str, str, str, str]


@dataclass(frozen=True, slots=True)
class FomContext:
    scope: ObservationScope
    model_id: str
    revision: str
    element_id: str
    name: str | None
    status: Literal["present", "removed", "unknown"]
    description: str | None

    def to_json(self) -> JsonObject:
        return {**self.scope.to_json(), "model_id": self.model_id, "revision": self.revision, "element_id": self.element_id,
                "name": self.name, "status": self.status, "description": self.description}


@dataclass(frozen=True, slots=True)
class FomField:
    field_id: str
    name: str | None
    datatype_id: str | None
    description: str | None

    def to_json(self) -> JsonObject:
        return {"field_id": self.field_id, "name": self.name, "datatype_id": self.datatype_id, "description": self.description}


def fom_fields(raw: object) -> tuple[FomField, ...] | None:
    if raw is None:
        return None
    if not isinstance(raw, (list, tuple)):
        raise TypeError("FOM field metadata must be an explicit array or unknown")
    items = cast(list[object] | tuple[object, ...], raw)
    if len(items) > 4096:
        raise ValueError("FOM field arrays contain at most 4096 entries")
    fields: list[FomField] = []
    for item in items:
        if not isinstance(item, Mapping):
            raise TypeError("FOM field metadata must be a named record")
        values = cast(Mapping[str, object], item)
        if set(values) - {"field_id", "name", "datatype_id", "description"}:
            raise ValueError("unknown FOM field metadata")
        fields.append(FomField(cast(str, identity_text(values.get("field_id"))), identity_text(values.get("name"), optional=True),
                               identity_text(values.get("datatype_id"), optional=True), observation_reason(values.get("description"))))
    if len({row.field_id for row in fields}) != len(fields):
        raise ValueError("FOM field IDs must be unique within an element report")
    return tuple(fields)


@dataclass(frozen=True, slots=True)
class FomEnumerator:
    name: str
    value: str

    def to_json(self) -> JsonObject:
        return {"name": self.name, "value": self.value}


def fom_enumerators(raw: object) -> tuple[FomEnumerator, ...] | None:
    if raw is None:
        return None
    if not isinstance(raw, (list, tuple)):
        raise TypeError("FOM enumerators must be an explicit array or unknown")
    items = cast(list[object] | tuple[object, ...], raw)
    if len(items) > 4096:
        raise ValueError("FOM enumerator arrays contain at most 4096 entries")
    rows: list[FomEnumerator] = []
    for item in items:
        if not isinstance(item, Mapping):
            raise TypeError("FOM enumerator must be a named record")
        values = cast(Mapping[str, object], item)
        if set(values) - {"name", "value"}:
            raise ValueError("unknown FOM enumerator fields")
        rows.append(FomEnumerator(cast(str, identity_text(values.get("name"))), cast(str, identity_text(values.get("value")))))
    if len({row.name for row in rows}) != len(rows):
        raise ValueError("FOM enumerator names must be unique; aliases may share a reported value")
    return tuple(rows)


@dataclass(frozen=True, slots=True)
class FomClassData:
    context: FomContext
    class_kind: Literal["object-class", "interaction-class"]
    parent_state: Literal["root", "parent", "unknown"]
    parent_id: str | None
    field_scope: Literal["declared", "effective", "unknown"]
    fields: tuple[FomField, ...] | None

    def kind(self) -> FomKind:
        return self.class_kind

    def to_json(self) -> JsonObject:
        return {**self.context.to_json(), "kind": self.class_kind, "parent_state": self.parent_state, "parent_id": self.parent_id,
                "field_scope": self.field_scope, "attributes" if self.class_kind == "object-class" else "parameters":
                [row.to_json() for row in self.fields] if self.fields is not None else None}


@dataclass(frozen=True, slots=True)
class FomDatatypeData:
    context: FomContext
    encoding: Literal["primitive", "enumeration", "array", "record", "variant", "opaque", "unknown"]
    representation: str | None
    fields: tuple[FomField, ...] | None
    enumerators: tuple[FomEnumerator, ...] | None
    element_type_id: str | None
    cardinality: str | None

    def kind(self) -> FomKind:
        return "datatype"

    def to_json(self) -> JsonObject:
        return {**self.context.to_json(), "kind": "datatype", "encoding": self.encoding, "representation": self.representation,
                "fields": [row.to_json() for row in self.fields] if self.fields is not None else None,
                "enumerators": [row.to_json() for row in self.enumerators] if self.enumerators is not None else None,
                "element_type_id": self.element_type_id, "cardinality": self.cardinality}


@dataclass(frozen=True, slots=True)
class FomDimensionData:
    context: FomContext
    upper_bound: str | None
    unit: str | None

    def kind(self) -> FomKind:
        return "dimension"

    def to_json(self) -> JsonObject:
        return {**self.context.to_json(), "kind": "dimension", "upper_bound": self.upper_bound, "unit": self.unit}


FomData: TypeAlias = FomClassData | FomDatatypeData | FomDimensionData


def fom_data(event: Mapping[str, object]) -> FomData | None:
    raw = event.get("fom_observation")
    if raw is None:
        return None
    if not isinstance(raw, Mapping):
        raise TypeError("fom_observation must be named adapter-supplied metadata")
    values = cast(Mapping[str, object], raw)
    status, kind = values.get("status"), values.get("kind")
    if status not in ("present", "removed", "unknown"):
        raise ValueError("FOM element status must be explicit")
    context = FomContext(observation_scope(values), cast(str, identity_text(values.get("model_id"))), cast(str, identity_text(values.get("revision"))),
                         cast(str, identity_text(values.get("element_id"))), identity_text(values.get("name"), optional=True), status,
                         observation_reason(values.get("description")))
    common = {"kind", "session_id", "identity_namespace", "federation_id", "model_id", "revision", "element_id", "name", "status", "description"}
    if kind in ("object-class", "interaction-class"):
        field = "attributes" if kind == "object-class" else "parameters"
        if set(values) - (common | {"parent_state", "parent_id", "field_scope", field}):
            raise ValueError("unknown or inapplicable FOM class fields")
        parent_state, field_scope = values.get("parent_state"), values.get("field_scope")
        parent_id = identity_text(values.get("parent_id"), optional=True)
        if parent_state not in ("root", "parent", "unknown") or (parent_state == "parent") != (parent_id is not None):
            raise ValueError("FOM parent identity must distinguish an explicit root, parent and unknown ancestry")
        if field_scope not in ("declared", "effective", "unknown"):
            raise ValueError("FOM class field scope must be explicit")
        return FomClassData(context, kind, parent_state, parent_id, field_scope, fom_fields(values.get(field)))
    if kind == "dimension":
        if set(values) - (common | {"upper_bound", "unit"}):
            raise ValueError("unknown FOM dimension fields")
        return FomDimensionData(context, decimal_bound(values.get("upper_bound"), optional=True), identity_text(values.get("unit"), optional=True))
    encoding = values.get("encoding")
    if kind != "datatype" or encoding not in ("primitive", "enumeration", "array", "record", "variant", "opaque", "unknown"):
        raise ValueError("FOM datatype kind and encoding must be explicit")
    if set(values) - (common | {"encoding", "representation", "fields", "enumerators", "element_type_id", "cardinality"}):
        raise ValueError("unknown FOM datatype fields")
    fields, enumerators = fom_fields(values.get("fields")), fom_enumerators(values.get("enumerators"))
    element_type = identity_text(values.get("element_type_id"), optional=True)
    cardinality = decimal_bound(values.get("cardinality"), optional=True)
    if fields is not None and encoding not in ("record", "variant") or enumerators is not None and encoding != "enumeration":
        raise ValueError("datatype fields/enumerators do not apply to the reported encoding")
    if (element_type is not None or cardinality is not None) and encoding != "array":
        raise ValueError("array element/cardinality metadata applies only to array encodings")
    return FomDatatypeData(context, encoding, identity_text(values.get("representation"), optional=True, limit=4096), fields, enumerators, element_type, cardinality)


@dataclass(frozen=True, slots=True)
class FomReport:
    data: FomData
    source: str
    sequence: int
    observed_at: float

    def key(self) -> FomKey:
        context = self.data.context
        return (*context.scope.key(self.source), context.model_id, context.revision, self.data.kind(), context.element_id)

    def to_json(self) -> JsonObject:
        return {**self.data.to_json(), "source": self.source, "sequence": self.sequence, "observed_at": self.observed_at}


class FomViews:
    def __init__(self, capacity: int, history_capacity: int, *, budget: EvidenceBudget | None = None) -> None:
        self._index: LatestHistory[FomKey, FomReport] = LatestHistory(capacity, history_capacity, name="fom", budget=budget)

    def record(self, event: EventRecord) -> None:
        data = fom_data(event)
        if data is not None:
            row = FomReport(data, event.source, event.sequence, event.observed_at)
            self._index.record(row.key(), row)

    def clear(self) -> None:
        self._index.clear()

    def to_json(self) -> JsonObject:
        return to_json_object({"snapshots": [row.to_json() for row in self._index.snapshots],
                               "transitions": [row.to_json() for row in self._index.transitions],
                               "capacity": self._index.capacity, "history_capacity": self._index.history_capacity,
                               "dropped_snapshots": self._index.dropped_snapshots, "dropped_transitions": self._index.dropped_transitions,
                               "basis": "adapter-supplied-fom-metadata"})
