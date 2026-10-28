"""Explicit attribute ownership facts and scoped request evidence; no inference."""

# pyright: strict
from __future__ import annotations

from collections.abc import Mapping
from dataclasses import dataclass
from typing import Generic, Literal, TypeAlias, TypeVar, cast

from .identities import (
    ParticipantIdentity,
    identity_text,
    observation_reason,
    participant_identity,
)
from .models import (
    EventRecord,
    JsonObject,
    OwnershipObservationProjection,
    OwnershipOutcomeProjection,
    OwnershipRequestProjection,
    OwnershipStateProjection,
    to_json_object,
)
from .storage import EvidenceBudget, EvidenceIndex, EvidenceQueue

OwnerState = Literal["known", "unowned", "rti", "unknown"]
OwnershipAction = Literal["acquire", "divest"]
OwnershipOutcome = Literal["completed", "failed", "cancelled"]
OwnershipStatus = Literal["pending", "completed", "failed", "cancelled", "ambiguous"]
ScopeKey = tuple[str, str, str, str, str, str]
TicketKey = tuple[ScopeKey, str, str, str, str]


@dataclass(frozen=True, slots=True)
class OwnershipScope:
    session_id: str
    identity_namespace: str
    federation_id: str
    object_id: str
    attribute_key: str

    def key(self, source: str) -> ScopeKey:
        return (
            source,
            self.session_id,
            self.identity_namespace,
            self.federation_id,
            self.object_id,
            self.attribute_key,
        )

    def to_json(self) -> JsonObject:
        return {
            "session_id": self.session_id,
            "identity_namespace": self.identity_namespace,
            "federation_id": self.federation_id,
            "object_id": self.object_id,
            "attribute_key": self.attribute_key,
        }


@dataclass(frozen=True, slots=True)
class OwnershipStateData:
    scope: OwnershipScope
    owner_state: OwnerState
    owner: ParticipantIdentity | None
    transition: Literal["reported", "acquired", "divested"]
    reason: str | None
    reported_owner_name: str | None = None
    reported_owner_handle: str | None = None

    @property
    def kind(self) -> Literal["state"]:
        return "state"

    def to_json(self) -> JsonObject:
        return {
            **self.scope.to_json(),
            "kind": "state",
            "owner_state": self.owner_state,
            "owner": self.owner.to_json() if self.owner else None,
            "transition": self.transition,
            "reason": self.reason,
            "reported_owner_name": self.reported_owner_name,
            "reported_owner_handle": self.reported_owner_handle,
        }


@dataclass(frozen=True, slots=True)
class OwnershipRequestData:
    scope: OwnershipScope
    request_id: str
    action: OwnershipAction
    requester: ParticipantIdentity
    reason: str | None

    @property
    def kind(self) -> Literal["request"]:
        return "request"

    def to_json(self) -> JsonObject:
        return {
            **self.scope.to_json(),
            "kind": "request",
            "request_id": self.request_id,
            "action": self.action,
            "requester": self.requester.to_json(),
            "reason": self.reason,
        }


@dataclass(frozen=True, slots=True)
class OwnershipOutcomeData:
    scope: OwnershipScope
    request_id: str
    action: OwnershipAction
    requester: ParticipantIdentity
    outcome: OwnershipOutcome
    reason: str | None

    @property
    def kind(self) -> Literal["outcome"]:
        return "outcome"

    def to_json(self) -> JsonObject:
        return {
            **self.scope.to_json(),
            "kind": "outcome",
            "request_id": self.request_id,
            "action": self.action,
            "requester": self.requester.to_json(),
            "outcome": self.outcome,
            "reason": self.reason,
        }


OwnershipData: TypeAlias = (
    OwnershipStateData | OwnershipRequestData | OwnershipOutcomeData
)


def ownership_scope_from_fields(
    session_id: object,
    identity_namespace: object,
    federation_id: object,
    object_id: object,
    attribute_key: object,
) -> OwnershipScope:
    """Build the shared scope from fields on a named observation."""
    return OwnershipScope(
        cast(str, identity_text(session_id, limit=256)),
        cast(str, identity_text(identity_namespace)),
        cast(str, identity_text(federation_id)),
        cast(str, identity_text(object_id)),
        cast(str, identity_text(attribute_key)),
    )


def ownership_state_from_fields(
    scope: OwnershipScope,
    owner_state: object,
    owner: ParticipantIdentity | None,
    transition: object,
    reason: object,
    reported_owner_name: object,
    reported_owner_handle: object,
) -> OwnershipStateData:
    """Build a state fact after the input has selected the state variant."""
    if owner_state not in ("known", "unowned", "rti", "unknown") or transition not in (
        "reported",
        "acquired",
        "divested",
    ):
        raise ValueError("ownership state and transition must be explicit")
    if (owner_state == "known") != (owner is not None):
        raise ValueError("only a known owner has an explicit participant join identity")
    return OwnershipStateData(
        scope,
        owner_state,
        owner,
        transition,
        observation_reason(reason),
        identity_text(reported_owner_name, optional=True),
        identity_text(reported_owner_handle, optional=True),
    )


def ownership_request_from_fields(
    scope: OwnershipScope,
    request_id: object,
    action: object,
    requester: ParticipantIdentity,
    reason: object,
) -> OwnershipRequestData:
    """Build a request fact after the input has selected the request variant."""
    normalized_request_id = cast(str, identity_text(request_id))
    if action not in ("acquire", "divest"):
        raise ValueError("ownership request action must be explicit")
    return OwnershipRequestData(
        scope, normalized_request_id, action, requester, observation_reason(reason)
    )


def ownership_outcome_from_fields(
    scope: OwnershipScope,
    request_id: object,
    action: object,
    requester: ParticipantIdentity,
    outcome: object,
    reason: object,
) -> OwnershipOutcomeData:
    """Build an outcome fact after the input has selected the outcome variant."""
    normalized_request_id = cast(str, identity_text(request_id))
    if action not in ("acquire", "divest"):
        raise ValueError("ownership request action must be explicit")
    if outcome not in ("completed", "failed", "cancelled"):
        raise ValueError("ownership request outcome must be explicit")
    return OwnershipOutcomeData(
        scope,
        normalized_request_id,
        action,
        requester,
        outcome,
        observation_reason(reason),
    )


# opaque-boundary: Parse provider ownership records before JSON normalization.
def ownership_data(event: Mapping[str, object]) -> OwnershipData | None:
    raw = event.get("ownership")
    if raw is None:
        return None
    if not isinstance(raw, Mapping):
        raise TypeError("ownership must be a named observation")
    values = cast(Mapping[str, object], raw)
    common = {
        "kind",
        "session_id",
        "identity_namespace",
        "federation_id",
        "object_id",
        "attribute_key",
        "reason",
    }
    scope = ownership_scope_from_fields(
        values.get("session_id"),
        values.get("identity_namespace"),
        values.get("federation_id"),
        values.get("object_id"),
        values.get("attribute_key"),
    )
    reason = observation_reason(values.get("reason"))
    kind = values.get("kind")
    if kind == "state":
        if set(values) - (
            common
            | {
                "owner_state",
                "owner",
                "transition",
                "reported_owner_name",
                "reported_owner_handle",
            }
        ):
            raise ValueError("unknown ownership state fields")
        owner_state, transition = values.get("owner_state"), values.get("transition")
        if owner_state not in (
            "known",
            "unowned",
            "rti",
            "unknown",
        ) or transition not in ("reported", "acquired", "divested"):
            raise ValueError("ownership state and transition must be explicit")
        raw_owner = values.get("owner")
        if (owner_state == "known") != (raw_owner is not None):
            raise ValueError(
                "only a known owner has an explicit participant join identity"
            )
        return ownership_state_from_fields(
            scope,
            owner_state,
            participant_identity(raw_owner) if raw_owner is not None else None,
            transition,
            reason,
            values.get("reported_owner_name"),
            values.get("reported_owner_handle"),
        )
    if kind not in ("request", "outcome") or set(values) - (
        common
        | {"request_id", "action", "requester"}
        | ({"outcome"} if kind == "outcome" else set())
    ):
        raise ValueError("unknown ownership kind or request fields")
    request_id = cast(str, identity_text(values.get("request_id")))
    action = values.get("action")
    if action not in ("acquire", "divest"):
        raise ValueError("ownership request action must be explicit")
    requester = participant_identity(values.get("requester"))
    if kind == "request":
        return ownership_request_from_fields(
            scope, request_id, action, requester, reason
        )
    outcome = values.get("outcome")
    if outcome not in ("completed", "failed", "cancelled"):
        raise ValueError("ownership request outcome must be explicit")
    return ownership_outcome_from_fields(
        scope, request_id, action, requester, outcome, reason
    )


T_co = TypeVar("T_co", bound=OwnershipObservationProjection, covariant=True)


@dataclass(frozen=True, slots=True)
class OwnershipReport(Generic[T_co]):
    data: T_co
    source: str
    sequence: int
    observed_at: float

    def to_json(self) -> JsonObject:
        return {
            **self.data.to_json(),
            "source": self.source,
            "sequence": self.sequence,
            "observed_at": self.observed_at,
        }


@dataclass(slots=True)
class OwnershipTicket:
    request: OwnershipReport[OwnershipRequestProjection]
    status: OwnershipStatus
    last_sequence: int
    outcome: OwnershipReport[OwnershipOutcomeProjection] | None = None
    duplicate_requests: int = 0

    def to_json(self) -> JsonObject:
        return {
            "request": self.request.to_json(),
            "status": self.status,
            "last_sequence": self.last_sequence,
            "outcome": self.outcome.to_json() if self.outcome else None,
            "duplicate_requests": self.duplicate_requests,
        }


def _ticket_key(
    source: str, data: OwnershipRequestProjection | OwnershipOutcomeProjection
) -> TicketKey:
    return (
        data.scope.key(source),
        data.request_id,
        data.action,
        data.requester.participant_id,
        data.requester.membership_id,
    )


class OwnershipViews:
    def __init__(
        self,
        capacity: int,
        request_capacity: int,
        history_capacity: int,
        *,
        budget: EvidenceBudget | None = None,
    ) -> None:
        self.capacity, self.request_capacity, self.history_capacity = (
            capacity,
            request_capacity,
            history_capacity,
        )
        shared = budget if budget is not None else EvidenceBudget()
        self._states = EvidenceIndex[
            ScopeKey, OwnershipReport[OwnershipStateProjection]
        ]("ownership_states", capacity, shared, lambda _, row: row.to_json())
        self._requests = EvidenceIndex[TicketKey, OwnershipTicket](
            "ownership_requests", request_capacity, shared, lambda _, row: row.to_json()
        )
        self._history = EvidenceQueue[OwnershipReport[OwnershipObservationProjection]](
            "ownership_history", history_capacity, shared, lambda row: row.to_json()
        )
        self._unmatched_outcomes = 0

    def record(self, event: EventRecord) -> None:
        data = event.ownership_observation
        if data is None:
            return
        self._history.append(
            OwnershipReport(data, event.source, event.sequence, event.observed_at)
        )
        if data.kind == "state":
            key = data.scope.key(event.source)
            self._states[key] = OwnershipReport(
                data, event.source, event.sequence, event.observed_at
            )
            return
        ticket_key = _ticket_key(event.source, data)
        ticket = self._requests.get(ticket_key)
        if data.kind == "request":
            if ticket is not None:
                ticket.status, ticket.outcome = "ambiguous", None
                ticket.duplicate_requests += 1
                ticket.last_sequence = event.sequence
                self._requests.refresh(ticket_key)
                return
            self._requests[ticket_key] = OwnershipTicket(
                OwnershipReport(data, event.source, event.sequence, event.observed_at),
                "pending",
                event.sequence,
            )
        elif ticket is not None and ticket.status == "pending":
            ticket.status = data.outcome
            ticket.last_sequence = event.sequence
            ticket.outcome = OwnershipReport(
                data, event.source, event.sequence, event.observed_at
            )
            self._requests.refresh(ticket_key)
        else:
            self._unmatched_outcomes += 1

    def clear(self) -> None:
        self._states.clear()
        self._requests.clear()
        self._history.clear()
        self._unmatched_outcomes = 0

    def to_json(self) -> JsonObject:
        return to_json_object(
            {
                "states": [row.to_json() for row in self._states.values()],
                "requests": [row.to_json() for row in self._requests.values()],
                "transitions": [row.to_json() for row in self._history],
                "capacity": self.capacity,
                "request_capacity": self.request_capacity,
                "history_capacity": self.history_capacity,
                "dropped_states": self._states.dropped,
                "dropped_requests": self._requests.dropped,
                "dropped_transitions": self._history.dropped,
                "unmatched_outcomes": self._unmatched_outcomes,
                "basis": "explicit-ownership-observations",
            }
        )
