"""Explicit topology evidence and join identities, independent of any RTI API.

Names and RTI handles are display metadata. Only an adapter's namespace,
federation ID, participant ID and membership ID identify a topology node.
"""

# pyright: strict
from __future__ import annotations

from collections.abc import Mapping
from dataclasses import dataclass
from typing import Literal, cast

from .identities import ParticipantIdentity, identity_text, participant_identity
from .models import EventRecord, JsonObject, to_json_object
from .storage import EvidenceBudget, EvidenceIndex


@dataclass(frozen=True, slots=True)
class RelationshipData:
    relationship_id: str
    session_id: str
    identity_namespace: str
    federation_id: str
    producer: ParticipantIdentity
    recipient: ParticipantIdentity
    kind: Literal["object", "interaction"]
    evidence: Literal["declared", "observed"]
    status: Literal["active", "ended", "unknown"]
    viewpoint: Literal["observer", "producer", "recipient", "unknown"]
    class_key: str | None
    class_name: str | None
    reason: str | None

    def to_json(self) -> JsonObject:
        return to_json_object({
            "relationship_id": self.relationship_id, "session_id": self.session_id,
            "identity_namespace": self.identity_namespace, "federation_id": self.federation_id,
            "producer": self.producer.to_json(), "recipient": self.recipient.to_json(),
            "kind": self.kind, "evidence": self.evidence, "status": self.status, "viewpoint": self.viewpoint,
            "class_key": self.class_key, "class_name": self.class_name, "reason": self.reason,
        })


def relationship_data(event: Mapping[str, object]) -> RelationshipData | None:
    raw = event.get("relationship")
    if raw is None:
        return None
    if not isinstance(raw, Mapping):
        raise TypeError("relationship must be a named relationship object")
    values = cast(Mapping[str, object], raw)
    if set(values) - {"relationship_id", "session_id", "identity_namespace", "federation_id", "producer", "recipient",
                      "kind", "evidence", "status", "viewpoint", "class_key", "class_name", "reason"}:
        raise ValueError("unknown relationship fields")
    kind, evidence, status, viewpoint = values.get("kind"), values.get("evidence"), values.get("status"), values.get("viewpoint")
    if kind not in ("object", "interaction") or evidence not in ("declared", "observed"):
        raise ValueError("relationship kind and evidence must be explicit")
    if status not in ("active", "ended", "unknown") or viewpoint not in ("observer", "producer", "recipient", "unknown"):
        raise ValueError("relationship status and reporting viewpoint must be explicit")
    reason = values.get("reason")
    if reason is not None and (not isinstance(reason, str) or len(reason) > 4096):
        raise ValueError("relationship reason must be at most 4096 text characters")
    return RelationshipData(
        cast(str, identity_text(values.get("relationship_id"))), cast(str, identity_text(values.get("session_id"), limit=256)),
        cast(str, identity_text(values.get("identity_namespace"))), cast(str, identity_text(values.get("federation_id"))),
        participant_identity(values.get("producer")), participant_identity(values.get("recipient")),
        kind, evidence, status, viewpoint, identity_text(values.get("class_key"), optional=True),
        identity_text(values.get("class_name"), optional=True), reason,
    )


@dataclass(frozen=True, slots=True)
class RelationshipSnapshot:
    source: str
    relationship: RelationshipData
    sequence: int
    observed_at: float

    def to_json(self) -> JsonObject:
        return {**self.relationship.to_json(), "source": self.source,
                "sequence": self.sequence, "observed_at": self.observed_at}


class RelationshipViews:
    def __init__(self, capacity: int, *, budget: EvidenceBudget | None = None) -> None:
        self.capacity = capacity
        self._rows = EvidenceIndex[tuple[str, str, str, str, str], RelationshipSnapshot]("relationships", capacity, budget if budget is not None else EvidenceBudget(), lambda _, row: row.to_json())

    def record(self, event: EventRecord) -> None:
        data = relationship_data(event)
        if data is None:
            return
        key = event.source, data.session_id, data.identity_namespace, data.federation_id, data.relationship_id
        self._rows[key] = RelationshipSnapshot(event.source, data, event.sequence, event.observed_at)

    def clear(self) -> None:
        self._rows.clear()

    def to_json(self) -> JsonObject:
        return to_json_object({"snapshots": [row.to_json() for row in self._rows.values()],
                               "capacity": self.capacity, "dropped_relationships": self._rows.dropped,
                               "basis": "last-reported-explicit-relationships"})
