"""Adapter-supplied FOM metadata; no provider file parsing or value decoding."""

# pyright: strict
from __future__ import annotations

from collections.abc import Mapping, Sequence
from dataclasses import dataclass
from typing import Literal, TypeAlias, cast

from .identities import (
    ObservationScope,
    decimal_bound,
    identity_text,
    observation_reason,
    observation_scope_from_fields,
)
from .inspection_index import LatestHistory
from .models import EventRecord, FomObservation, JsonObject, to_json_object
from .storage import EvidenceBudget

FomKind = Literal["object-class", "interaction-class", "datatype", "dimension"]
FomClassKind = Literal["object-class", "interaction-class"]
FomElementStatus = Literal["present", "removed", "unknown"]
FomParentState = Literal["root", "parent", "unknown"]
FomFieldScope = Literal["declared", "effective", "unknown"]
FomEncoding = Literal[
    "primitive", "enumeration", "array", "record", "variant", "opaque", "unknown"
]
FomKey = tuple[str, str, str, str, str, str, str, str]


@dataclass(frozen=True, slots=True)
class FomContext:
    scope: ObservationScope
    model_id: str
    revision: str
    element_id: str
    name: str | None
    status: FomElementStatus
    description: str | None

    def to_json(self) -> JsonObject:
        return {
            **self.scope.to_json(),
            "model_id": self.model_id,
            "revision": self.revision,
            "element_id": self.element_id,
            "name": self.name,
            "status": self.status,
            "description": self.description,
        }


def fom_context_from_fields(
    session_id: object,
    identity_namespace: object,
    federation_id: object,
    model_id: object,
    revision: object,
    element_id: object,
    name: object,
    status: object,
    description: object,
) -> FomContext:
    """Build an explicit FOM element context from its named fields."""
    if status not in ("present", "removed", "unknown"):
        raise ValueError("FOM element status must be explicit")
    return FomContext(
        observation_scope_from_fields(
            session_id,
            identity_namespace,
            federation_id,
        ),
        cast(str, identity_text(model_id)),
        cast(str, identity_text(revision)),
        cast(str, identity_text(element_id)),
        identity_text(name, optional=True),
        status,
        observation_reason(description),
    )


@dataclass(frozen=True, slots=True)
class FomField:
    field_id: str
    name: str | None
    datatype_id: str | None
    description: str | None

    def to_json(self) -> JsonObject:
        return {
            "field_id": self.field_id,
            "name": self.name,
            "datatype_id": self.datatype_id,
            "description": self.description,
        }


def fom_field_from_fields(
    field_id: object,
    name: object,
    datatype_id: object,
    description: object,
) -> FomField:
    return FomField(
        cast(str, identity_text(field_id)),
        identity_text(name, optional=True),
        identity_text(datatype_id, optional=True),
        observation_reason(description),
    )


def fom_fields_from_values(
    fields: Sequence[FomField] | None,
) -> tuple[FomField, ...] | None:
    if fields is None:
        return None
    if len(fields) > 4096:
        raise ValueError("FOM field arrays contain at most 4096 entries")
    if len({row.field_id for row in fields}) != len(fields):
        raise ValueError("FOM field IDs must be unique within an element report")
    return tuple(fields)


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
        fields.append(
            fom_field_from_fields(
                values.get("field_id"),
                values.get("name"),
                values.get("datatype_id"),
                values.get("description"),
            )
        )
    return fom_fields_from_values(fields)


@dataclass(frozen=True, slots=True)
class FomEnumerator:
    name: str
    value: str

    def to_json(self) -> JsonObject:
        return {"name": self.name, "value": self.value}


def fom_enumerator_from_fields(name: object, value: object) -> FomEnumerator:
    return FomEnumerator(
        cast(str, identity_text(name)),
        cast(str, identity_text(value)),
    )


def fom_enumerators_from_values(
    enumerators: Sequence[FomEnumerator] | None,
) -> tuple[FomEnumerator, ...] | None:
    if enumerators is None:
        return None
    if len(enumerators) > 4096:
        raise ValueError("FOM enumerator arrays contain at most 4096 entries")
    if len({row.name for row in enumerators}) != len(enumerators):
        raise ValueError(
            "FOM enumerator names must be unique; aliases may share a reported value"
        )
    return tuple(enumerators)


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
        rows.append(fom_enumerator_from_fields(values.get("name"), values.get("value")))
    return fom_enumerators_from_values(rows)


@dataclass(frozen=True, slots=True)
class FomClassData:
    context: FomContext
    class_kind: FomClassKind
    parent_state: FomParentState
    parent_id: str | None
    field_scope: FomFieldScope
    fields: tuple[FomField, ...] | None

    def kind(self) -> FomKind:
        return self.class_kind

    def to_json(self) -> JsonObject:
        return {
            **self.context.to_json(),
            "kind": self.class_kind,
            "parent_state": self.parent_state,
            "parent_id": self.parent_id,
            "field_scope": self.field_scope,
            "attributes" if self.class_kind == "object-class" else "parameters": [
                row.to_json() for row in self.fields
            ]
            if self.fields is not None
            else None,
        }


@dataclass(frozen=True, slots=True)
class FomDatatypeData:
    context: FomContext
    encoding: FomEncoding
    representation: str | None
    fields: tuple[FomField, ...] | None
    enumerators: tuple[FomEnumerator, ...] | None
    element_type_id: str | None
    cardinality: str | None

    def kind(self) -> FomKind:
        return "datatype"

    def to_json(self) -> JsonObject:
        return {
            **self.context.to_json(),
            "kind": "datatype",
            "encoding": self.encoding,
            "representation": self.representation,
            "fields": [row.to_json() for row in self.fields]
            if self.fields is not None
            else None,
            "enumerators": [row.to_json() for row in self.enumerators]
            if self.enumerators is not None
            else None,
            "element_type_id": self.element_type_id,
            "cardinality": self.cardinality,
        }


@dataclass(frozen=True, slots=True)
class FomDimensionData:
    context: FomContext
    upper_bound: str | None
    unit: str | None

    def kind(self) -> FomKind:
        return "dimension"

    def to_json(self) -> JsonObject:
        return {
            **self.context.to_json(),
            "kind": "dimension",
            "upper_bound": self.upper_bound,
            "unit": self.unit,
        }


FomData: TypeAlias = FomClassData | FomDatatypeData | FomDimensionData


def fom_class_scope_from_fields(
    parent_state: object,
    parent_id: str | None,
    field_scope: object,
) -> tuple[FomParentState, str | None, FomFieldScope]:
    if parent_state not in ("root", "parent", "unknown") or (
        parent_state == "parent"
    ) != (parent_id is not None):
        raise ValueError(
            "FOM parent identity must distinguish an explicit root, parent and unknown ancestry"
        )
    if field_scope not in ("declared", "effective", "unknown"):
        raise ValueError("FOM class field scope must be explicit")
    return (
        parent_state,
        parent_id,
        field_scope,
    )


def fom_class_from_fields(
    context: FomContext,
    class_kind: FomClassKind,
    parent_state: object,
    parent_id: str | None,
    field_scope: object,
    fields: Sequence[FomField] | None,
) -> FomClassData:
    normalized_parent, normalized_id, normalized_scope = fom_class_scope_from_fields(
        parent_state,
        parent_id,
        field_scope,
    )
    return FomClassData(
        context,
        class_kind,
        normalized_parent,
        normalized_id,
        normalized_scope,
        fom_fields_from_values(fields),
    )


def fom_dimension_from_fields(
    context: FomContext,
    upper_bound: object,
    unit: object,
) -> FomDimensionData:
    return FomDimensionData(
        context,
        decimal_bound(upper_bound, optional=True),
        identity_text(unit, optional=True),
    )


def fom_datatype_from_fields(
    context: FomContext,
    encoding: FomEncoding,
    representation: object,
    fields: Sequence[FomField] | None,
    enumerators: Sequence[FomEnumerator] | None,
    element_type_id: object,
    cardinality: object,
) -> FomDatatypeData:
    normalized_fields = fom_fields_from_values(fields)
    normalized_enumerators = fom_enumerators_from_values(enumerators)
    element_type = identity_text(element_type_id, optional=True)
    array_cardinality = decimal_bound(cardinality, optional=True)
    if (
        normalized_fields is not None
        and encoding not in ("record", "variant")
        or (normalized_enumerators is not None and encoding != "enumeration")
    ):
        raise ValueError(
            "datatype fields/enumerators do not apply to the reported encoding"
        )
    if (
        element_type is not None or array_cardinality is not None
    ) and encoding != "array":
        raise ValueError(
            "array element/cardinality metadata applies only to array encodings"
        )
    return FomDatatypeData(
        context,
        encoding,
        identity_text(representation, optional=True, limit=4096),
        normalized_fields,
        normalized_enumerators,
        element_type,
        array_cardinality,
    )


# opaque-boundary: Parse provider FOM fields before normalized event storage.
def fom_data(event: Mapping[str, object]) -> FomData | None:
    raw = event.get("fom_observation")
    if raw is None:
        return None
    if not isinstance(raw, Mapping):
        raise TypeError("fom_observation must be named adapter-supplied metadata")
    values = cast(Mapping[str, object], raw)
    status, kind = values.get("status"), values.get("kind")
    context = fom_context_from_fields(
        values.get("session_id"),
        values.get("identity_namespace"),
        values.get("federation_id"),
        values.get("model_id"),
        values.get("revision"),
        values.get("element_id"),
        values.get("name"),
        status,
        values.get("description"),
    )
    common = {
        "kind",
        "session_id",
        "identity_namespace",
        "federation_id",
        "model_id",
        "revision",
        "element_id",
        "name",
        "status",
        "description",
    }
    if kind in ("object-class", "interaction-class"):
        field = "attributes" if kind == "object-class" else "parameters"
        if set(values) - (common | {"parent_state", "parent_id", "field_scope", field}):
            raise ValueError("unknown or inapplicable FOM class fields")
        parent_state, field_scope = (
            values.get("parent_state"),
            values.get("field_scope"),
        )
        parent_id = identity_text(values.get("parent_id"), optional=True)
        fom_class_scope_from_fields(parent_state, parent_id, field_scope)
        return fom_class_from_fields(
            context,
            kind,
            parent_state,
            parent_id,
            field_scope,
            fom_fields(values.get(field)),
        )
    if kind == "dimension":
        if set(values) - (common | {"upper_bound", "unit"}):
            raise ValueError("unknown FOM dimension fields")
        return fom_dimension_from_fields(
            context,
            values.get("upper_bound"),
            values.get("unit"),
        )
    encoding = values.get("encoding")
    if kind != "datatype" or encoding not in (
        "primitive",
        "enumeration",
        "array",
        "record",
        "variant",
        "opaque",
        "unknown",
    ):
        raise ValueError("FOM datatype kind and encoding must be explicit")
    if set(values) - (
        common
        | {
            "encoding",
            "representation",
            "fields",
            "enumerators",
            "element_type_id",
            "cardinality",
        }
    ):
        raise ValueError("unknown FOM datatype fields")
    return fom_datatype_from_fields(
        context,
        encoding,
        values.get("representation"),
        fom_fields(values.get("fields")),
        fom_enumerators(values.get("enumerators")),
        values.get("element_type_id"),
        values.get("cardinality"),
    )


@dataclass(frozen=True, slots=True)
class FomReport:
    data: FomObservation
    source: str
    sequence: int
    observed_at: float

    def key(self) -> FomKey:
        context = self.data.context
        return (
            *context.scope.key(self.source),
            context.model_id,
            context.revision,
            self.data.kind(),
            context.element_id,
        )

    def to_json(self) -> JsonObject:
        return {
            **self.data.to_json(),
            "source": self.source,
            "sequence": self.sequence,
            "observed_at": self.observed_at,
        }


class FomViews:
    def __init__(
        self,
        capacity: int,
        history_capacity: int,
        *,
        budget: EvidenceBudget | None = None,
    ) -> None:
        self._index: LatestHistory[FomKey, FomReport] = LatestHistory(
            capacity, history_capacity, name="fom", budget=budget
        )

    def record(self, event: EventRecord) -> None:
        data = event.fom_observation
        if data is not None:
            row = FomReport(data, event.source, event.sequence, event.observed_at)
            self._index.record(row.key(), row)

    def clear(self) -> None:
        self._index.clear()

    def to_json(self) -> JsonObject:
        return to_json_object(
            {
                "snapshots": [row.to_json() for row in self._index.snapshots],
                "transitions": [row.to_json() for row in self._index.transitions],
                "capacity": self._index.capacity,
                "history_capacity": self._index.history_capacity,
                "dropped_snapshots": self._index.dropped_snapshots,
                "dropped_transitions": self._index.dropped_transitions,
                "basis": "adapter-supplied-fom-metadata",
            }
        )
