"""Reported synchronization and save/restore phases, never inferred outcomes."""

# pyright: strict
from __future__ import annotations

from collections.abc import Mapping
from dataclasses import dataclass
from typing import Literal, cast

from .identities import ParticipantIdentity, identity_text, participant_identity
from .models import EventRecord, JsonObject, to_json_object
from .storage import EvidenceBudget, EvidenceIndex, EvidenceQueue

CoordinationKind = Literal["synchronization", "save", "restore"]
CoordinationPhase = Literal["requested", "registered", "registration-failed", "announced", "achieved", "begun", "completed", "failed"]
ParticipantProgress = Literal["pending", "achieved", "begun", "completed", "failed", "unknown"]


@dataclass(frozen=True, slots=True)
class CoordinationParticipant:
    identity: ParticipantIdentity
    status: ParticipantProgress
    reason: str | None

    def to_json(self) -> JsonObject:
        return {"identity": self.identity.to_json(), "status": self.status, "reason": self.reason}


@dataclass(frozen=True, slots=True)
class CoordinationData:
    kind: CoordinationKind
    operation_id: str
    session_id: str
    identity_namespace: str
    federation_id: str
    label: str
    phase: CoordinationPhase
    scope: Literal["local", "federation"]
    subject: ParticipantIdentity | None
    participants: tuple[CoordinationParticipant, ...] | None
    reason: str | None

    def to_json(self) -> JsonObject:
        return to_json_object({
            "kind": self.kind, "operation_id": self.operation_id, "session_id": self.session_id,
            "identity_namespace": self.identity_namespace, "federation_id": self.federation_id,
            "label": self.label, "phase": self.phase, "scope": self.scope,
            "subject": self.subject.to_json() if self.subject else None,
            "participants": [row.to_json() for row in self.participants] if self.participants is not None else None,
            "reason": self.reason,
        })


def _reason(value: object) -> str | None:
    if value is None or isinstance(value, str) and len(value) <= 4096:
        return value
    raise ValueError("coordination reasons must be at most 4096 text characters")


def coordination_data(event: Mapping[str, object]) -> CoordinationData | None:
    raw = event.get("coordination")
    if raw is None:
        return None
    if not isinstance(raw, Mapping):
        raise TypeError("coordination must be a named observation")
    values = cast(Mapping[str, object], raw)
    if set(values) - {"kind", "operation_id", "session_id", "identity_namespace", "federation_id", "label", "phase", "scope", "subject", "participants", "reason"}:
        raise ValueError("unknown coordination fields")
    kind, phase, scope = values.get("kind"), values.get("phase"), values.get("scope")
    phases = ("requested", "registered", "registration-failed", "announced", "achieved", "completed", "failed") if kind == "synchronization" else ("requested", "begun", "completed", "failed")
    if kind not in ("synchronization", "save", "restore") or phase not in phases or scope not in ("local", "federation"):
        raise ValueError("coordination kind, applicable phase and local/federation scope must be explicit")
    raw_subject, raw_participants = values.get("subject"), values.get("participants")
    if scope == "federation" and raw_subject is not None:
        raise ValueError("a federation-wide observation cannot claim a single local subject")
    participants: tuple[CoordinationParticipant, ...] | None = None
    if raw_participants is not None:
        if not isinstance(raw_participants, (list, tuple)):
            raise TypeError("coordination participants must be an array or unknown")
        items = cast(list[object] | tuple[object, ...], raw_participants)
        if len(items) > 4096:
            raise ValueError("coordination can report at most 4096 participants")
        parsed: list[CoordinationParticipant] = []
        seen: set[tuple[str, str]] = set()
        for item in items:
            if not isinstance(item, Mapping):
                raise TypeError("coordination participant must be a named progress record")
            fields = cast(Mapping[str, object], item)
            status = fields.get("status")
            if set(fields) - {"identity", "status", "reason"} or status not in ("pending", "achieved", "begun", "completed", "failed", "unknown"):
                raise ValueError("unknown participant progress fields/status")
            identity = participant_identity(fields.get("identity"))
            key = identity.participant_id, identity.membership_id
            if key in seen:
                raise ValueError("coordination participant join IDs must be unique")
            seen.add(key)
            parsed.append(CoordinationParticipant(identity, status, _reason(fields.get("reason"))))
        participants = tuple(parsed)
    return CoordinationData(
        kind, cast(str, identity_text(values.get("operation_id"))), cast(str, identity_text(values.get("session_id"), limit=256)),
        cast(str, identity_text(values.get("identity_namespace"))), cast(str, identity_text(values.get("federation_id"))),
        cast(str, identity_text(values.get("label"))), cast(CoordinationPhase, phase), scope,
        participant_identity(raw_subject) if raw_subject is not None else None, participants, _reason(values.get("reason")),
    )


@dataclass(frozen=True, slots=True)
class CoordinationSnapshot:
    source: str
    observation: CoordinationData
    sequence: int
    observed_at: float

    def to_json(self) -> JsonObject:
        return {**self.observation.to_json(), "source": self.source, "sequence": self.sequence, "observed_at": self.observed_at}


def coordination_key(source: str, data: CoordinationData) -> tuple[str, str, str, str, str, str, str, tuple[str, str] | None]:
    subject = (data.subject.participant_id, data.subject.membership_id) if data.subject else None
    return source, data.session_id, data.identity_namespace, data.federation_id, data.kind, data.operation_id, data.scope, subject


class CoordinationViews:
    def __init__(self, capacity: int, history_capacity: int, *, budget: EvidenceBudget | None = None) -> None:
        self.capacity = capacity
        self.history_capacity = history_capacity
        shared = budget if budget is not None else EvidenceBudget()
        self._rows = EvidenceIndex[tuple[str, str, str, str, str, str, str, tuple[str, str] | None], CoordinationSnapshot]("coordination_snapshots", capacity, shared, lambda _, row: row.to_json())
        self._history = EvidenceQueue[CoordinationSnapshot]("coordination_history", history_capacity, shared, lambda row: row.to_json())

    def record(self, event: EventRecord) -> None:
        data = coordination_data(event)
        if data is None:
            return
        key = coordination_key(event.source, data)
        row = CoordinationSnapshot(event.source, data, event.sequence, event.observed_at)
        self._rows[key] = row
        self._history.append(row)

    def clear(self) -> None:
        self._rows.clear()
        self._history.clear()

    def to_json(self) -> JsonObject:
        return to_json_object({"snapshots": [row.to_json() for row in self._rows.values()],
                               "transitions": [row.to_json() for row in self._history],
                               "capacity": self.capacity, "history_capacity": self.history_capacity,
                               "dropped_operations": self._rows.dropped, "dropped_transitions": self._history.dropped,
                               "basis": "explicit-coordination-observations"})
