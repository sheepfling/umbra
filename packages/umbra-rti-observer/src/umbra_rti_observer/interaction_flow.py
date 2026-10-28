"""Bounded explicit send/receive pairing; no class-name or clock inference."""

# pyright: strict
from __future__ import annotations

import math
from collections.abc import Mapping
from dataclasses import dataclass
from time import monotonic
from typing import Literal, cast

from .identities import ParticipantIdentity, identity_text, participant_identity
from .models import (
    EventRecord,
    InteractionFlowProjection,
    JsonObject,
    RawInteractionFlowObservation,
    RawObserverEvent,
    to_json_object,
)
from .storage import EvidenceBudget, EvidenceIndex

FlowKind = Literal["send", "receive"]
FlowOrder = Literal["receive-order", "timestamped", "unknown"]
FlowStatus = Literal["uncorrelated", "send-only", "receive-only", "paired", "ambiguous"]
FlowKey = tuple[str, str, str, tuple[str, str], tuple[str, str]]


@dataclass(frozen=True, slots=True)
class InteractionFlowData:
    kind: FlowKind
    order: FlowOrder
    message_id: str | None
    identity_namespace: str | None
    federation_id: str | None
    session_id: str | None
    producer: ParticipantIdentity | None
    recipient: ParticipantIdentity | None
    class_key: str | None
    logical_time: int | float | None

    @property
    def correlation_key(self) -> FlowKey | None:
        if (
            self.message_id is None
            or self.identity_namespace is None
            or self.federation_id is None
            or self.producer is None
            or self.recipient is None
        ):
            return None
        return (
            self.identity_namespace,
            self.federation_id,
            self.message_id,
            (self.producer.participant_id, self.producer.membership_id),
            (self.recipient.participant_id, self.recipient.membership_id),
        )

    def to_json(self) -> JsonObject:
        return to_json_object(
            {
                "kind": self.kind,
                "order": self.order,
                "message_id": self.message_id,
                "identity_namespace": self.identity_namespace,
                "federation_id": self.federation_id,
                "session_id": self.session_id,
                "producer": self.producer.to_json() if self.producer else None,
                "recipient": self.recipient.to_json() if self.recipient else None,
                "class_key": self.class_key,
                "logical_time": self.logical_time,
            }
        )


def flow_number(value: object) -> int | float | None:
    if value is None:
        return None
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise ValueError(  # noqa: TRY004 -- Pydantic must surface invalid input as a validation error
            "reported logical time must be a finite JSON number or unknown"
        )
    try:
        finite = math.isfinite(value)
    except OverflowError:
        finite = False
    if not finite:
        raise ValueError("reported logical time must be finite")
    return value


def validate_interaction_flow_direction(
    event_type: object, event_kind: object, flow_kind: object
) -> None:
    """Require explicit send/receive data to agree with the event type aliases."""
    canonical_event_type = next(
        (
            str(value).strip()
            for value in (event_type, event_kind)
            if value is not None and str(value).strip()
        ),
        None,
    )
    if canonical_event_type in {"interaction.sent", "interaction.received"} and (
        flow_kind
        != ("send" if canonical_event_type == "interaction.sent" else "receive")
    ):
        raise ValueError("interaction envelope and explicit flow direction disagree")


def interaction_flow_from_fields(
    *,
    kind: object,
    order: object,
    message_id: object,
    identity_namespace: object,
    federation_id: object,
    session_id: object,
    producer: ParticipantIdentity | None,
    recipient: ParticipantIdentity | None,
    class_key: object,
    logical_time: object,
) -> InteractionFlowData:
    """Build one validated flow record from named fields, without a JSON map."""
    if kind not in ("send", "receive") or order not in (
        "receive-order",
        "timestamped",
        "unknown",
    ):
        raise ValueError(
            "interaction flow direction and reported order must be explicit"
        )
    return InteractionFlowData(
        kind,
        order,
        identity_text(message_id, optional=True),
        identity_text(identity_namespace, optional=True),
        identity_text(federation_id, optional=True),
        identity_text(session_id, optional=True, limit=256),
        producer,
        recipient,
        identity_text(class_key, optional=True),
        flow_number(logical_time),
    )


# opaque-boundary: Parse raw interaction-flow input before JSON normalization.
def interaction_flow_data(event: RawObserverEvent) -> InteractionFlowData | None:
    raw = event.get("interaction_flow")
    if raw is None:
        return None
    if not isinstance(raw, Mapping):
        raise TypeError("interaction_flow must be a named observation")
    values = cast(RawInteractionFlowObservation, raw)
    if set(values) - {
        "kind",
        "order",
        "message_id",
        "identity_namespace",
        "federation_id",
        "session_id",
        "producer",
        "recipient",
        "class_key",
        "logical_time",
    }:
        raise ValueError("unknown interaction flow fields")
    kind, order = values.get("kind"), values.get("order")
    if kind not in ("send", "receive") or order not in (
        "receive-order",
        "timestamped",
        "unknown",
    ):
        raise ValueError(
            "interaction flow direction and reported order must be explicit"
        )
    validate_interaction_flow_direction(
        event.get("event_type"), event.get("kind"), kind
    )
    logical_time = flow_number(values.get("logical_time"))
    message_id = identity_text(values.get("message_id"), optional=True)
    identity_namespace = identity_text(values.get("identity_namespace"), optional=True)
    federation_id = identity_text(values.get("federation_id"), optional=True)
    session_id = identity_text(values.get("session_id"), optional=True, limit=256)
    producer, recipient = values.get("producer"), values.get("recipient")
    producer_identity = participant_identity(producer) if producer is not None else None
    recipient_identity = (
        participant_identity(recipient) if recipient is not None else None
    )
    class_key = identity_text(values.get("class_key"), optional=True)
    return interaction_flow_from_fields(
        kind=kind,
        order=order,
        message_id=message_id,
        identity_namespace=identity_namespace,
        federation_id=federation_id,
        session_id=session_id,
        producer=producer_identity,
        recipient=recipient_identity,
        class_key=class_key,
        logical_time=logical_time,
    )


@dataclass(frozen=True, slots=True)
class InteractionFlowReport:
    observation: InteractionFlowProjection
    source: str
    sequence: int
    observed_at: float
    arrival: float

    def to_json(self) -> JsonObject:
        return {
            **self.observation.to_json(),
            "source": self.source,
            "sequence": self.sequence,
            "observed_at": self.observed_at,
        }


@dataclass(slots=True)
class InteractionFlowEntry:
    record_id: int
    send: InteractionFlowReport | None = None
    receive: InteractionFlowReport | None = None
    send_count: int = 0
    receive_count: int = 0
    last_sequence: int = 0

    @property
    def status(self) -> FlowStatus:
        sample = self.send or self.receive
        assert sample is not None
        if sample.observation.correlation_key is None:
            return "uncorrelated"
        if self.send_count > 1 or self.receive_count > 1:
            return "ambiguous"
        if self.send is not None and self.receive is not None:
            return "paired"
        return "send-only" if self.send else "receive-only"

    def to_json(self) -> JsonObject:
        delay: float | None = None
        if (
            self.status == "paired"
            and self.send
            and self.receive
            and self.send.sequence < self.receive.sequence
        ):
            duration = self.receive.arrival - self.send.arrival
            if math.isfinite(duration) and duration >= 0:
                delay = duration
        return to_json_object(
            {
                "record_id": self.record_id,
                "last_sequence": self.last_sequence,
                "status": self.status,
                "send": self.send.to_json() if self.send else None,
                "receive": self.receive.to_json() if self.receive else None,
                "send_count": self.send_count,
                "receive_count": self.receive_count,
                "arrival_delay_seconds": delay,
            }
        )


class InteractionFlowViews:
    def __init__(self, capacity: int, *, budget: EvidenceBudget | None = None) -> None:
        self.capacity = capacity
        self._rows = EvidenceIndex[FlowKey | int, InteractionFlowEntry](
            "interaction_flow",
            capacity,
            budget if budget is not None else EvidenceBudget(),
            lambda _, row: row.to_json(),
        )
        self._send_count = 0
        self._receive_count = 0

    def record(self, event: EventRecord) -> None:
        data = event.interaction_flow_observation
        if data is None:
            if event.event_type not in {"interaction.sent", "interaction.received"}:
                return
            # Exact legacy event types count as observations, not correlated
            # delivery. No endpoints, order or IDs are guessed from other data.
            class_key = event.get("interaction_class")
            data = InteractionFlowData(
                "send" if event.event_type == "interaction.sent" else "receive",
                "unknown",
                None,
                None,
                None,
                None,
                None,
                None,
                class_key
                if isinstance(class_key, str)
                and class_key.strip()
                and len(class_key) <= 512
                else None,
                None,
            )
        key = data.correlation_key or event.sequence
        row = self._rows.get(key) or InteractionFlowEntry(event.sequence)
        report = InteractionFlowReport(
            data, event.source, event.sequence, event.observed_at, monotonic()
        )
        if data.kind == "send":
            row.send_count += 1
            self._send_count += 1
            row.send = (
                row.send or report
            )  # Duplicate observations cannot replace the original evidence.
        else:
            row.receive_count += 1
            self._receive_count += 1
            row.receive = row.receive or report
        row.last_sequence = event.sequence
        self._rows[key] = row

    def clear(self) -> None:
        self._rows.clear()
        self._send_count = self._receive_count = 0

    def to_json(self) -> JsonObject:
        return to_json_object(
            {
                "records": [row.to_json() for row in self._rows.values()],
                "capacity": self.capacity,
                "dropped_records": self._rows.dropped,
                "send_observations": self._send_count,
                "receive_observations": self._receive_count,
                "basis": "explicit-interaction-observations",
            }
        )
