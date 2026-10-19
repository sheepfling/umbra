"""Provider-neutral object perspectives, independent of event-history eviction.

Attribution means only that the adapter named a federate on an observation.
It does not prove local RTI visibility, ownership, publication or subscription.
The caller holds the store lock; no web framework or provider API is used here.
"""

# pyright: strict
from __future__ import annotations

from collections import deque
from copy import deepcopy
from dataclasses import dataclass

from .models import (
    EventRecord,
    JsonObject,
    JsonValue,
    ObjectInspection,
    Retention,
    to_json_object,
)


@dataclass(slots=True)
class ObjectPerspective:
    federate: str | None
    generation: int
    last_sequence: int
    object: ObjectInspection

    def to_json(self) -> JsonObject:
        return to_json_object({
            "federate": self.federate, "generation": self.generation,
            "last_sequence": self.last_sequence, "object": self.object.to_json(),
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

    def to_json(self) -> JsonObject:
        return to_json_object({
            "sequence": self.sequence, "object_key": self.object_key,
            "federate": self.federate, "generation": self.generation,
            "attribute": self.attribute, "value": self.value,
            "previous_known": self.previous_known, "previous_value": self.previous_value,
            "source": self.source, "observed_at": self.observed_at,
        })


class ObjectViews:
    def __init__(self, max_events: int) -> None:
        self._snapshots: dict[tuple[str | None, str], ObjectPerspective] = {}
        # One slot per event, including events with no attributes. History is
        # therefore bounded by, and exactly aligned with, the event window.
        self._history: deque[list[ObjectAttributeChange]] = deque(maxlen=max_events)

    def record(self, event: EventRecord, objects: list[ObjectInspection], federate: str | None) -> None:
        changes: list[ObjectAttributeChange] = []
        for incoming in objects:
            for scope in (None, federate) if federate is not None else (None,):
                key = (scope, incoming.object_key)
                current = self._snapshots.get(key)
                if current is None or (current.object.status == "removed" and event.event_type == "object.discovered"):
                    current = ObjectPerspective(scope, event.sequence, event.sequence, deepcopy(incoming))
                    current.object.attributes.clear()
                    self._snapshots[key] = current
                else:
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
                    ))
                    current.object.attributes[name] = deepcopy(value)
        self._history.append(changes)

    def clear(self) -> None:
        self._snapshots.clear()
        self._history.clear()

    def to_json(self, retention: Retention) -> JsonObject:
        return to_json_object({
            "snapshots": [row.to_json() for _, row in sorted(
                self._snapshots.items(), key=lambda entry: (entry[0][0] or "", entry[0][1]))],
            "changes": [change.to_json() for batch in self._history for change in batch],
            "history_truncated": bool(retention.dropped_events or retention.cleared_events),
            "history_first_sequence": retention.first_retained_sequence,
            "history_last_sequence": retention.last_sequence,
        })
