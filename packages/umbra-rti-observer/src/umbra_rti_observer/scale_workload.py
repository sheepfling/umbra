"""Bounded synthetic profiling workloads; no provider APIs or captured data."""

# pyright: strict
from __future__ import annotations

from typing import Annotated

from pydantic import Field

from .core import ObserverStore
from .models import JsonObject
from .schemas import ObserverModel


class Workload(ObserverModel):
    events: Annotated[int, Field(ge=1, le=200000)] = 20000
    retained_events: Annotated[int, Field(ge=1, le=20000)] = 2000
    participants: Annotated[int, Field(ge=1, le=2000)] = 50
    objects: Annotated[int, Field(ge=1, le=5000)] = 200
    payload_bytes: Annotated[int, Field(ge=0, le=4096)] = 64
    batch_size: Annotated[int, Field(ge=1, le=10000)] = 100
    inspector_rows: Annotated[int, Field(ge=0, le=500)] = 0
    perspective_capacity: Annotated[int, Field(ge=1, le=50000)] = 5000
    rotate_perspectives: bool = False
    recording_commits: Annotated[int, Field(ge=0, le=100)] = 12


PROFILES = {
    "standard": Workload(),
    "sustained": Workload(events=100000, participants=100, objects=1000, payload_bytes=128),
    "rich": Workload(events=20000, participants=100, objects=1000, payload_bytes=128, inspector_rows=64),
    "recording-rich": Workload(events=20000, participants=100, objects=1000, payload_bytes=128, inspector_rows=64, recording_commits=100),
    "wide": Workload(events=20000, retained_events=10000, participants=200, objects=2000, payload_bytes=1024),
    "many-perspectives": Workload(events=20000, participants=100, objects=200, rotate_perspectives=True),
    "large-index": Workload(events=20000, participants=100, objects=200, rotate_perspectives=True, perspective_capacity=50000),
}


def observation(index: int, workload: Workload) -> JsonObject:
    participant = (index + (index // workload.objects if workload.rotate_perspectives else 0)) % workload.participants
    return {"event_type": "object.updated", "source": "scale-fixture", "federation": "SyntheticScaleFederation",
            "federate": f"Participant-{participant}" if index % 11 else None,
            "object_key": f"object-{index % workload.objects}", "class_name": "SyntheticClass",
            "attributes": {"value": index, "payload": "x" * workload.payload_bytes}}


def seed_inspectors(store: ObserverStore, count: int, participants: int) -> int:
    """Populate every optional family independently, outside the timed ingress lane."""
    for index in range(count):
        common: JsonObject = {"session_id": "scale-session", "identity_namespace": "scale-provider", "federation_id": "scale-federation"}
        actor: JsonObject = {"participant_id": f"participant-{index}", "membership_id": f"join-{index}", "display_name": f"Participant-{index}"}
        peer: JsonObject = {"participant_id": f"peer-{index}", "membership_id": f"peer-join-{index}"}
        events: list[JsonObject] = [
            {"event_type": "declaration.observed", "declaration": {"kind": "object", "session_id": "scale-session", "class_key": f"class-{index}", "direction": "publication", "status": "declared", "attribute_scope": "selected", "attributes": ["value"]}},
            {"event_type": "relationship.observed", "relationship": {**common, "relationship_id": f"edge-{index}", "kind": "object", "evidence": "declared", "status": "active", "viewpoint": "observer", "producer": actor, "recipient": peer}},
            {"event_type": "coordination.observed", "coordination": {**common, "kind": "save", "operation_id": f"save-{index}", "label": "Synthetic save", "phase": "requested", "scope": "federation"}},
            {"event_type": "interaction.sent", "interaction_flow": {**common, "kind": "send", "order": "receive-order", "message_id": f"message-{index}", "producer": actor, "recipient": peer}},
            {"event_type": "ownership.observed", "ownership": {**common, "kind": "state", "object_id": f"instance-{index}", "attribute_key": "value", "owner_state": "known", "owner": actor, "transition": "reported"}},
            {"event_type": "region.observed", "region_observation": {**common, "kind": "region", "region_id": f"region-{index}", "status": "active", "evidence": "declared", "extents": [{"extent_id": "extent", "dimensions": [{"dimension_id": "dimension", "lower_bound": "0", "upper_bound": "9007199254740993", "dimension_upper_bound": "18446744073709551615"}]}]}},
            {"event_type": "mom.observed", "mom_observation": {**common, "kind": "counter", "report_id": f"counter-{index}", "counter_key": "SyntheticCount", "semantics": "gauge", "value": index}},
            {"event_type": "fom.observed", "fom_observation": {**common, "kind": "object-class", "element_id": f"class-{index}", "model_id": "synthetic-model", "revision": "1", "status": "present", "parent_state": "unknown", "field_scope": "declared", "attributes": [{"field_id": "value"}]}},
            {"event_type": "time.requested", "time_observation": {"kind": "request", "session_id": "scale-session", "request_id": f"request-{index}", "logical_time": index}},
        ]
        for event in events:
            event.update(source="scale-fixture", federation="SyntheticScaleFederation", federate=f"Participant-{index % participants}")
        store.publish_many(events)
    return count * 9
