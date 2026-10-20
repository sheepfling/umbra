"""Explicit declaration snapshots; never infer ownership, routing or delivery."""

# pyright: strict
from __future__ import annotations

from collections.abc import Mapping
from dataclasses import dataclass
from typing import Literal, cast

from .attribution import ParticipantPerspective, event_perspective
from .models import EventRecord, JsonObject, to_json_object
from .storage import EvidenceBudget, EvidenceIndex

DeclarationKind = Literal["object", "interaction"]
DeclarationDirection = Literal["publication", "subscription"]
DeclarationStatus = Literal["declared", "withdrawn", "unknown"]
AttributeScope = Literal["selected", "all", "unknown", "not-applicable"]


@dataclass(frozen=True, slots=True)
class DeclarationData:
    kind: DeclarationKind
    direction: DeclarationDirection
    class_key: str
    status: DeclarationStatus
    session_id: str | None
    class_name: str | None
    attribute_scope: AttributeScope
    attributes: tuple[str, ...] | None
    reason: str | None


@dataclass(frozen=True, slots=True, eq=False)
class DeclarationScope:
    source: str
    session_id: str | None
    federation: str | None
    federate: str | None
    perspective: ParticipantPerspective | None = None

    @property
    def key(self) -> tuple[str | None, ...]:
        return ("perspective", self.perspective.key) if self.perspective else ("legacy-scope", self.source, self.session_id, self.federation, self.federate)

    def __hash__(self) -> int:
        return hash(self.key)

    def __eq__(self, other: object) -> bool:
        return isinstance(other, DeclarationScope) and self.key == other.key

    def to_json(self) -> JsonObject:
        return {"source": self.source, "session_id": self.session_id,
                "federation": self.federation, "federate": self.federate,
                **({"perspective": self.perspective.to_json()} if self.perspective else {})}


@dataclass(frozen=True, slots=True)
class DeclarationSnapshot:
    scope: DeclarationScope
    declaration: DeclarationData
    sequence: int
    observed_at: float

    def to_json(self) -> JsonObject:
        data = self.declaration
        return to_json_object({
            "scope": self.scope.to_json(), "kind": data.kind, "direction": data.direction,
            "class_key": data.class_key, "class_name": data.class_name, "status": data.status,
            "attribute_scope": data.attribute_scope,
            "attributes": list(data.attributes) if data.attributes is not None else None,
            "reason": data.reason, "sequence": self.sequence, "observed_at": self.observed_at,
        })


def _text(value: object, *, required: bool = False, limit: int = 512) -> str | None:
    if value is None and not required:
        return None
    if isinstance(value, str) and value.strip() and len(value) <= limit:
        return value
    raise ValueError("declaration identifiers must be bounded nonblank strings")


def declaration_data(event: Mapping[str, object]) -> DeclarationData | None:
    raw = event.get("declaration")
    if raw is None:
        return None
    if not isinstance(raw, Mapping):
        raise TypeError("declaration must be a named declaration object")
    values = cast(Mapping[str, object], raw)
    kind = values.get("kind")
    direction = values.get("direction")
    status = values.get("status")
    if kind not in ("object", "interaction") or direction not in ("publication", "subscription"):
        raise ValueError("declaration kind/direction must be explicit")
    if status not in ("declared", "withdrawn", "unknown"):
        raise ValueError("declaration status must be explicit")
    allowed = {"kind", "direction", "status", "class_key", "class_name", "session_id", "reason"}
    if kind == "object":
        allowed |= {"attributes", "attribute_scope"}
    if set(values) - allowed:
        raise ValueError("unknown or inapplicable declaration fields")
    class_key = _text(values.get("class_key"), required=True)
    assert class_key is not None
    raw_attributes = values.get("attributes")
    attributes: tuple[str, ...] | None = None
    attribute_scope = values.get("attribute_scope", "unknown") if kind == "object" else "not-applicable"
    if attribute_scope not in ("selected", "all", "unknown", "not-applicable") or (kind == "object" and attribute_scope == "not-applicable"):
        raise ValueError("invalid declaration attribute scope")
    if attribute_scope == "selected":
        if not isinstance(raw_attributes, (list, tuple)):
            raise ValueError("selected attributes require an explicit array, including an empty array")
        items = cast(list[object] | tuple[object, ...], raw_attributes)
        if len(items) > 4096:
            raise ValueError("a declaration can report at most 4096 selected attributes")
        attributes = tuple(cast(str, _text(item, required=True)) for item in items)
        if len(set(attributes)) != len(attributes):
            raise ValueError("duplicate selected declaration attributes")
    elif raw_attributes is not None:
        raise ValueError("all/unknown/not-applicable attribute scopes cannot report a selected set")
    reason = values.get("reason")
    if reason is not None and not isinstance(reason, str):
        raise ValueError("declaration reason must be text")
    return DeclarationData(
        kind, direction, class_key, status, _text(values.get("session_id"), limit=256),
        _text(values.get("class_name")), attribute_scope, attributes, reason,
    )


class DeclarationViews:
    def __init__(self, capacity: int, *, budget: EvidenceBudget | None = None) -> None:
        self.capacity = capacity
        self._rows = EvidenceIndex[tuple[DeclarationScope, DeclarationKind, DeclarationDirection, str], DeclarationSnapshot]("declarations", capacity, budget if budget is not None else EvidenceBudget(), lambda _, row: row.to_json())

    def record(self, event: EventRecord, federate: str | None, federation: str | None) -> None:
        data = declaration_data(event)
        if data is None:
            return
        perspective = event_perspective(event)
        root_session = event.get("session_id")
        session = data.session_id or (perspective.session_id if perspective else root_session if isinstance(root_session, str) else None)
        scope = DeclarationScope(event.source, session, federation, federate, perspective)
        key = scope, data.kind, data.direction, data.class_key
        self._rows[key] = DeclarationSnapshot(scope, data, event.sequence, event.observed_at)

    def clear(self) -> None:
        self._rows.clear()

    def to_json(self) -> JsonObject:
        return to_json_object({"snapshots": [row.to_json() for row in self._rows.values()],
                               "capacity": self.capacity, "dropped_declarations": self._rows.dropped,
                               "basis": "last-reported-declarations"})
