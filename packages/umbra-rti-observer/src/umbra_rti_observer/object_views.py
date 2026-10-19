"""Provider-neutral object perspectives, independent of event-history eviction.

Attribution means only that the adapter named a federate on an observation.
It does not prove local RTI visibility, ownership, publication or subscription.
The caller holds the store lock; no web framework or provider API is used here.
"""

# pyright: strict
from __future__ import annotations

import json
from collections.abc import Mapping
from copy import deepcopy
from dataclasses import dataclass
from typing import cast

from .attribution import ParticipantPerspective, event_perspective
from .identities import identity_text
from .models import (
    EventRecord,
    JsonObject,
    JsonValue,
    ObjectInspection,
    Retention,
    to_json_object,
)
from .storage import EvidenceBudget, EvidenceIndex, EvidenceQueue


@dataclass(frozen=True, slots=True)
class ObjectContext:
    identity_namespace: str
    federation_id: str

    def key(self, object_key: str) -> str:
        return json.dumps(["federation-object", self.identity_namespace, self.federation_id, object_key], ensure_ascii=False, separators=(",", ":"))

    def to_json(self) -> JsonObject:
        return {"identity_namespace": self.identity_namespace, "federation_id": self.federation_id}


def object_context(event: Mapping[str, object]) -> ObjectContext | None:
    raw = event.get("object_context")
    if raw is None:
        return None
    if not isinstance(raw, Mapping):
        raise TypeError("object_context must be a named object context")
    values = cast(Mapping[str, object], raw)
    if set(values) != {"identity_namespace", "federation_id"}:
        raise ValueError("object context requires exactly namespace and federation IDs")
    return ObjectContext(cast(str, identity_text(values["identity_namespace"])), cast(str, identity_text(values["federation_id"])))


@dataclass(slots=True)
class ObjectPerspective:
    federate: str | None
    generation: int
    last_sequence: int
    object: ObjectInspection
    perspective: ParticipantPerspective | None = None

    def to_json(self) -> JsonObject:
        return to_json_object({
            "federate": self.federate, "generation": self.generation,
            "last_sequence": self.last_sequence, "object": self.object.to_json(),
            **({"perspective": self.perspective.to_json()} if self.perspective else {}),
        })


@dataclass(frozen=True, slots=True)
class ObjectAttributeChange:
    sequence: int
    object_key: str
    federate: str | None
    generation: int
    attribute: str
    value: JsonValue
    previous_known: bool
    previous_value: JsonValue
    source: str
    observed_at: float
    perspective: ParticipantPerspective | None = None
    object_context: ObjectContext | None = None

    @property
    def cache_key(self) -> tuple[str | None, str | None, str | None, str]:
        context = self.object_context
        return (self.perspective.key if self.perspective else self.federate,
                context.identity_namespace if context else None, context.federation_id if context else None, self.object_key)

    def to_json(self) -> JsonObject:
        return to_json_object({
            "sequence": self.sequence, "object_key": self.object_key,
            "federate": self.federate, "generation": self.generation,
            "attribute": self.attribute, "value": self.value,
            "previous_known": self.previous_known, "previous_value": self.previous_value,
            "source": self.source, "observed_at": self.observed_at,
            **({"perspective": self.perspective.to_json()} if self.perspective else {}),
            **({"object_context": self.object_context.to_json(), "observation_key": self.object_context.key(self.object_key)} if self.object_context else {}),
        })


@dataclass(frozen=True, slots=True)
class ObjectChangeBatch:
    sequence: int
    changes: list[ObjectAttributeChange]

    def to_json(self) -> JsonObject:
        return to_json_object({"sequence": self.sequence, "changes": [row.to_json() for row in self.changes]})


class ObjectViews:
    def __init__(self, max_events: int, snapshot_capacity: int = 5000, *, budget: EvidenceBudget | None = None) -> None:
        self.snapshot_capacity = _snapshot_capacity(snapshot_capacity)
        shared = budget if budget is not None else EvidenceBudget()
        self._snapshots = EvidenceIndex[tuple[str | None, str | None, str | None, str], ObjectPerspective]("object_snapshots", snapshot_capacity, shared, lambda _, row: row.to_json())
        # One slot per event, including events with no attributes. History is
        # therefore bounded by, and exactly aligned with, the event window.
        self._history = EvidenceQueue[ObjectChangeBatch]("object_changes", max_events, shared, lambda row: row.to_json(), unit="event-batches")

    def record(self, event: EventRecord, objects: list[ObjectInspection], federate: str | None) -> None:
        changes: list[ObjectAttributeChange] = []
        subject = event_perspective(event)
        for incoming in objects:
            for perspective in (None, subject) if subject is not None else (None,):
                scope = perspective.label if perspective else None
                key = (perspective.key if perspective else None, *incoming.cache_key)
                current = self._snapshots.get(key)
                if current is None or (current.object.status == "removed" and event.event_type == "object.discovered"):
                    current = ObjectPerspective(scope, event.sequence, event.sequence, deepcopy(incoming), perspective)
                    current.object.attributes.clear()
                else:
                    current.federate = scope
                    current.perspective = perspective
                    row = current.object
                    # Missing metadata in an update must not erase discovery.
                    name = event.get("object_name")
                    if isinstance(name, str) and name.strip():
                        row.object_name = name.strip()
                    row.object_handle = incoming.object_handle or row.object_handle
                    row.class_name = incoming.class_name or row.class_name
                    row.class_handle = incoming.class_handle or row.class_handle
                    row.family = incoming.family
                    row.update_count += incoming.update_count
                    row.discovery_count += incoming.discovery_count
                    if event.event_type == "object.discovered":
                        row.status = "active"
                    elif incoming.status == "removed":
                        row.status = "removed"
                    row.sources = sorted(set(row.sources) | set(incoming.sources))
                    row.last_seen = incoming.last_seen
                    current.last_sequence = event.sequence
                for name, value in incoming.attributes.items():
                    changes.append(ObjectAttributeChange(
                        sequence=event.sequence, object_key=incoming.object_key,
                        federate=scope, generation=current.generation, attribute=name,
                        value=deepcopy(value), previous_known=name in current.object.attributes,
                        previous_value=deepcopy(current.object.attributes.get(name)),
                        source=event.source, observed_at=event.observed_at,
                        perspective=perspective, object_context=incoming.object_context,
                    ))
                    current.object.attributes[name] = deepcopy(value)
                self._snapshots[key] = current
        self._history.append(ObjectChangeBatch(event.sequence, changes))

    def trim_history(self, retention: Retention) -> None:
        self._history.trim(lambda row: retention.retained_events > 0 and retention.first_retained_sequence <= row.sequence <= retention.last_sequence)

    def clear(self) -> None:
        self._snapshots.clear()
        self._history.clear()

    def to_json(self, retention: Retention) -> JsonObject:
        history = [change for batch in self._history for change in batch.changes
                   if retention.first_retained_sequence <= change.sequence <= retention.last_sequence and retention.retained_events > 0]
        changes = [change for change in history if change.cache_key in self._snapshots]
        omitted = len(history) - len(changes)
        return to_json_object({
            "snapshots": [row.to_json() for _, row in sorted(
                self._snapshots.items(), key=lambda entry: tuple(value or "" for value in entry[0]))],
            "changes": [change.to_json() for change in changes],
            "snapshot_capacity": self.snapshot_capacity,
            "dropped_snapshots": self._snapshots.dropped,
            "omitted_changes": omitted,
            "dropped_history_batches": self._history.dropped,
            "history_truncated": bool(retention.dropped_events or retention.cleared_events or omitted or self._history.dropped),
            "history_first_sequence": retention.first_retained_sequence,
            "history_last_sequence": retention.last_sequence,
        })


def _snapshot_capacity(value: object) -> int:
    if isinstance(value, bool) or not isinstance(value, int) or value < 1:
        raise ValueError("max_object_perspectives must be a positive integer")
    return value
