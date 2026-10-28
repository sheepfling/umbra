"""Typed management reports, independent of ordinary membership and control."""

# pyright: strict
from __future__ import annotations

from collections.abc import Mapping, Sequence
from dataclasses import dataclass
from typing import Literal, TypeAlias, cast

from .identities import (
    ObservationScope,
    ParticipantReference,
    finite_observation_number,
    identity_text,
    observation_reason,
    observation_scope_from_fields,
    participant_reference,
)
from .inspection_index import LatestHistory
from .models import EventRecord, JsonObject, MomObservation, to_json_object
from .storage import EvidenceBudget

MomKey = tuple[str, str, str, str, str, str]
MomCoverage: TypeAlias = Literal["complete", "partial", "unknown"]
MomMemberStatus: TypeAlias = Literal["joined", "resigned", "unknown"]
MomServiceStatus: TypeAlias = Literal["invoked", "completed", "failed", "unknown"]
MomCounterSemantics: TypeAlias = Literal["cumulative", "gauge", "unknown"]


@dataclass(frozen=True, slots=True)
class MomContext:
    scope: ObservationScope
    report_id: str
    reason: str | None

    def to_json(self) -> JsonObject:
        return {
            **self.scope.to_json(),
            "report_id": self.report_id,
            "reason": self.reason,
        }


@dataclass(frozen=True, slots=True)
class MomMember:
    participant: ParticipantReference
    status: MomMemberStatus

    def to_json(self) -> JsonObject:
        return {"participant": self.participant.to_json(), "status": self.status}


@dataclass(frozen=True, slots=True)
class MomMembersData:
    context: MomContext
    coverage: MomCoverage
    members: tuple[MomMember, ...] | None

    def kind(self) -> str:
        return "members"

    def to_json(self) -> JsonObject:
        return {
            **self.context.to_json(),
            "kind": "members",
            "coverage": self.coverage,
            "members": [row.to_json() for row in self.members]
            if self.members is not None
            else None,
        }


@dataclass(frozen=True, slots=True)
class MomServiceData:
    context: MomContext
    service_name: str
    status: MomServiceStatus
    subject: ParticipantReference | None
    invocation_id: str | None
    error_name: str | None

    def kind(self) -> str:
        return "service"

    def to_json(self) -> JsonObject:
        return {
            **self.context.to_json(),
            "kind": "service",
            "service_name": self.service_name,
            "status": self.status,
            "subject": self.subject.to_json() if self.subject else None,
            "invocation_id": self.invocation_id,
            "error_name": self.error_name,
        }


@dataclass(frozen=True, slots=True)
class MomCounterData:
    context: MomContext
    counter_key: str
    semantics: MomCounterSemantics
    value: int | float | None
    value_text: str | None
    unit: str | None
    subject: ParticipantReference | None

    def kind(self) -> str:
        return "counter"

    def to_json(self) -> JsonObject:
        return {
            **self.context.to_json(),
            "kind": "counter",
            "counter_key": self.counter_key,
            "semantics": self.semantics,
            "value": self.value,
            "value_text": self.value_text,
            "unit": self.unit,
            "subject": self.subject.to_json() if self.subject else None,
        }


MomData: TypeAlias = MomMembersData | MomServiceData | MomCounterData


def counter_number(value: object) -> int | float | None:
    number = finite_observation_number(value)
    if isinstance(number, int) and abs(number) > 9_007_199_254_740_991:
        raise ValueError(
            "integer counters beyond the JSON/JavaScript safe range require value_text"
        )
    return number


def mom_context_from_fields(
    session_id: object,
    identity_namespace: object,
    federation_id: object,
    report_id: object,
    reason: object,
) -> MomContext:
    """Build a management-report context from its named wire fields."""
    return MomContext(
        observation_scope_from_fields(
            session_id,
            identity_namespace,
            federation_id,
        ),
        cast(str, identity_text(report_id)),
        observation_reason(reason),
    )


def mom_member_from_fields(
    participant: ParticipantReference, status: object
) -> MomMember:
    if status not in ("joined", "resigned", "unknown"):
        raise ValueError("management member status must be explicit")
    return MomMember(participant, status)


def mom_members_from_fields(
    context: MomContext,
    coverage: object,
    members: Sequence[MomMember] | None,
) -> MomMembersData:
    if coverage not in ("complete", "partial", "unknown"):
        raise ValueError("management member coverage must be explicit and fields named")
    if coverage == "complete" and members is None:
        raise ValueError("a complete reported roster requires an explicit member array")
    if members is not None and len(members) > 4096:
        raise ValueError("management reports contain at most 4096 member entries")
    seen: set[tuple[str, str]] = set()
    for member in members or ():
        identity = member.participant.identity
        if identity is None:
            continue
        key = identity.participant_id, identity.membership_id
        if key in seen:
            raise ValueError(
                "identified member joins cannot be duplicated in one report"
            )
        seen.add(key)
    return MomMembersData(
        context,
        coverage,
        tuple(members) if members is not None else None,
    )


def mom_service_from_fields(
    context: MomContext,
    service_name: str,
    status: object,
    subject: ParticipantReference | None,
    invocation_id: str | None,
    error_name: str | None,
) -> MomServiceData:
    if status not in ("invoked", "completed", "failed", "unknown"):
        raise ValueError("management service fields and status must be explicit")
    return MomServiceData(
        context,
        service_name,
        status,
        subject,
        invocation_id,
        error_name,
    )


def mom_counter_from_fields(
    context: MomContext,
    counter_key: str,
    semantics: object,
    value: float | None,
    value_text: str | None,
    unit: str | None,
    subject: ParticipantReference | None,
) -> MomCounterData:
    if semantics not in ("cumulative", "gauge", "unknown"):
        raise ValueError("management counter fields and semantics must be explicit")
    normalized_value = counter_number(value)
    if (normalized_value is None) == (value_text is None):
        raise ValueError(
            "a counter needs exactly one numeric or opaque-text representation"
        )
    return MomCounterData(
        context,
        counter_key,
        semantics,
        normalized_value,
        value_text,
        unit,
        subject,
    )


# opaque-boundary: Parse provider MOM values before normalized event storage.
def mom_data(event: Mapping[str, object]) -> MomData | None:
    raw = event.get("mom_observation")
    if raw is None:
        return None
    if not isinstance(raw, Mapping):
        raise TypeError("mom_observation must be a named management report")
    values = cast(Mapping[str, object], raw)
    context = mom_context_from_fields(
        values.get("session_id"),
        values.get("identity_namespace"),
        values.get("federation_id"),
        values.get("report_id"),
        values.get("reason"),
    )
    common = {
        "kind",
        "session_id",
        "identity_namespace",
        "federation_id",
        "report_id",
        "reason",
    }
    kind = values.get("kind")
    if kind == "members":
        coverage = values.get("coverage")
        if set(values) - (common | {"coverage", "members"}) or coverage not in (
            "complete",
            "partial",
            "unknown",
        ):
            raise ValueError(
                "management member coverage must be explicit and fields named"
            )
        raw_members = values.get("members")
        if coverage == "complete" and raw_members is None:
            raise ValueError(
                "a complete reported roster requires an explicit member array"
            )
        members: list[MomMember] | None = None
        if raw_members is not None:
            if not isinstance(raw_members, (list, tuple)):
                raise TypeError(
                    "management members must be an explicit array or unknown"
                )
            items = cast(list[object] | tuple[object, ...], raw_members)
            if len(items) > 4096:
                raise ValueError(
                    "management reports contain at most 4096 member entries"
                )
            members, seen = [], set[tuple[str, str]]()
            for item in items:
                if not isinstance(item, Mapping):
                    raise TypeError("management member must be a named record")
                fields = cast(Mapping[str, object], item)
                status = fields.get("status")
                if set(fields) - {"participant", "status"} or status not in (
                    "joined",
                    "resigned",
                    "unknown",
                ):
                    raise ValueError("management member status must be explicit")
                participant = participant_reference(fields.get("participant"))
                if participant.identity:
                    key = (
                        participant.identity.participant_id,
                        participant.identity.membership_id,
                    )
                    if key in seen:
                        raise ValueError(
                            "identified member joins cannot be duplicated in one report"
                        )
                    seen.add(key)
                members.append(mom_member_from_fields(participant, status))
        return mom_members_from_fields(context, coverage, members)
    subject = (
        participant_reference(values["subject"])
        if values.get("subject") is not None
        else None
    )
    if kind == "service":
        status = values.get("status")
        if set(values) - (
            common
            | {"service_name", "status", "subject", "invocation_id", "error_name"}
        ) or status not in ("invoked", "completed", "failed", "unknown"):
            raise ValueError("management service fields and status must be explicit")
        return mom_service_from_fields(
            context,
            cast(str, identity_text(values.get("service_name"))),
            status,
            subject,
            identity_text(values.get("invocation_id"), optional=True),
            identity_text(values.get("error_name"), optional=True),
        )
    semantics = values.get("semantics")
    if (
        kind != "counter"
        or set(values)
        - (
            common
            | {"counter_key", "semantics", "value", "value_text", "unit", "subject"}
        )
        or semantics not in ("cumulative", "gauge", "unknown")
    ):
        raise ValueError("management counter fields and semantics must be explicit")
    value_text = identity_text(values.get("value_text"), optional=True, limit=4096)
    return mom_counter_from_fields(
        context,
        cast(str, identity_text(values.get("counter_key"))),
        semantics,
        counter_number(values.get("value")),
        value_text,
        identity_text(values.get("unit"), optional=True),
        subject,
    )


@dataclass(frozen=True, slots=True)
class MomReport:
    data: MomObservation
    source: str
    sequence: int
    observed_at: float

    def key(self) -> MomKey:
        return (
            *self.data.context.scope.key(self.source),
            self.data.kind(),
            self.data.context.report_id,
        )

    def to_json(self) -> JsonObject:
        return {
            **self.data.to_json(),
            "source": self.source,
            "sequence": self.sequence,
            "observed_at": self.observed_at,
        }


class MomViews:
    def __init__(
        self,
        capacity: int,
        history_capacity: int,
        *,
        budget: EvidenceBudget | None = None,
    ) -> None:
        self._index: LatestHistory[MomKey, MomReport] = LatestHistory(
            capacity, history_capacity, name="mom", budget=budget
        )

    def record(self, event: EventRecord) -> None:
        data = event.mom_observation
        if data is not None:
            row = MomReport(data, event.source, event.sequence, event.observed_at)
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
                "basis": "explicit-management-reports",
            }
        )
