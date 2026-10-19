"""Dependency-free, explicit time observations. No RTI binding semantics inferred."""

# pyright: strict
from __future__ import annotations

import math
from collections.abc import Mapping
from dataclasses import dataclass
from typing import Literal, cast

from .attribution import ParticipantPerspective
from .models import JsonObject, to_json_object

TimeKind = Literal["request", "grant", "state", "failed", "cancelled"]
RequestStatus = Literal["pending", "granted", "failed", "cancelled", "ambiguous", "session-ended"]


@dataclass(frozen=True, slots=True)
class TimeData:
    kind: TimeKind
    session_id: str | None = None
    request_id: str | None = None
    logical_time: int | float | None = None
    lookahead: int | float | None = None
    regulating: bool | None = None
    constrained: bool | None = None
    galt: int | float | None = None
    lbts: int | float | None = None
    reason: str | None = None


@dataclass(frozen=True, slots=True, eq=False)
class TimeScope:
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
        return isinstance(other, TimeScope) and self.key == other.key

    @property
    def correlatable(self) -> bool:
        return self.source != "unknown" and self.session_id is not None and (self.federate is not None or self.perspective is not None and self.perspective.attribution is not None)

    def to_json(self) -> JsonObject:
        return {"source": self.source, "session_id": self.session_id,
                "federation": self.federation, "federate": self.federate,
                **({"perspective": self.perspective.to_json()} if self.perspective else {})}


@dataclass(frozen=True, slots=True)
class TimeSample:
    sequence: int
    logical_time: int | float | None
    observed_at: float
    request_id: str | None

    def to_json(self) -> JsonObject:
        return {"sequence": self.sequence, "logical_time": self.logical_time,
                "observed_at": self.observed_at, "request_id": self.request_id}


@dataclass(slots=True)
class TimeSnapshot:
    scope: TimeScope
    last_sequence: int
    observed_at: float
    status: Literal["observed", "ended"] = "observed"
    last_grant: TimeSample | None = None
    last_request: TimeSample | None = None
    reported_state: TimeData | None = None
    state_sequence: int | None = None
    uncorrelated_requests: int = 0
    uncorrelated_grants: int = 0

    def to_json(self) -> JsonObject:
        state = self.reported_state
        return to_json_object({
            "scope": self.scope.to_json(), "last_sequence": self.last_sequence,
            "observed_at": self.observed_at, "status": self.status,
            "last_grant": self.last_grant.to_json() if self.last_grant else None,
            "last_request": self.last_request.to_json() if self.last_request else None,
            "state_sequence": self.state_sequence,
            "reported_state": None if state is None else {
                "logical_time": state.logical_time, "lookahead": state.lookahead,
                "regulating": state.regulating, "constrained": state.constrained,
                "galt": state.galt, "lbts": state.lbts,
            },
            "uncorrelated_requests": self.uncorrelated_requests,
            "uncorrelated_grants": self.uncorrelated_grants,
        })


@dataclass(slots=True)
class TimeRequest:
    scope: TimeScope
    request_id: str
    request_sequence: int
    requested_time: int | float | None
    requested_at: float
    started_monotonic: float
    status: RequestStatus = "pending"
    outcome_sequence: int | None = None
    outcome_at: float | None = None
    granted_time: int | float | None = None
    latency_seconds: float | None = None
    reason: str | None = None

    def to_json(self, now: float) -> JsonObject:
        duration = now - self.started_monotonic
        return to_json_object({
            "scope": self.scope.to_json(), "request_id": self.request_id,
            "request_sequence": self.request_sequence, "requested_time": self.requested_time,
            "requested_at": self.requested_at, "status": self.status,
            "outcome_sequence": self.outcome_sequence, "outcome_at": self.outcome_at,
            "granted_time": self.granted_time, "latency_seconds": self.latency_seconds,
            "pending_seconds": duration if self.status == "pending" and math.isfinite(duration) and duration >= 0 else None,
            "reason": self.reason,
        })


def _number(value: object) -> int | float | None:
    if value is None:
        return None
    try:
        if isinstance(value, (int, float)) and not isinstance(value, bool) and math.isfinite(value):
            return value
    except OverflowError:
        pass
    raise ValueError("time values must be finite JSON numbers, not booleans or strings")


def _identity(value: object) -> str | None:
    if value is None:
        return None
    if isinstance(value, str) and value.strip() and len(value) <= 256:
        return value
    raise ValueError("time IDs must be nonblank strings of at most 256 characters")


def time_data(event: Mapping[str, object]) -> TimeData | None:
    raw = event.get("time_observation")
    if raw is None:
        # This exact callback is an explicit grant, not a timestamped payload.
        # Legacy grants are useful snapshots, but never get invented IDs.
        if event.get("event_type") != "callback.invoked" or event.get("operation") != "timeAdvanceGrant":
            return None
        logical = event.get("logical_time")
        details = event.get("details")
        if logical is None and isinstance(details, Mapping):
            logical = cast(Mapping[str, object], details).get("logical_time")
        try:
            value = _number(logical)
        except ValueError:
            return None
        return TimeData(kind="grant", logical_time=value) if value is not None else None
    if not isinstance(raw, Mapping):
        raise TypeError("time_observation must be a named observation object")
    values = cast(Mapping[str, object], raw)
    kind = values.get("kind")
    if kind not in ("request", "grant", "state", "failed", "cancelled"):
        raise ValueError("unknown time_observation kind")
    common = {"kind", "session_id"}
    allowed = common | ({"logical_time", "request_id"} if kind in ("request", "grant") else
                        {"request_id", "reason"} if kind in ("failed", "cancelled") else
                        {"logical_time", "lookahead", "regulating", "constrained", "galt", "lbts"})
    if set(values) - allowed:
        raise ValueError("unknown fields in time_observation")
    logical = _number(values.get("logical_time"))
    for flag in ("regulating", "constrained"):
        if values.get(flag) is not None and not isinstance(values[flag], bool):
            raise ValueError("time role flags must be booleans or null")
    reason = values.get("reason")
    if reason is not None and not isinstance(reason, str):
        raise ValueError("time outcome reason must be a string")
    return TimeData(
        kind=kind, session_id=_identity(values.get("session_id")),
        request_id=_identity(values.get("request_id")), logical_time=logical,
        lookahead=_number(values.get("lookahead")), galt=_number(values.get("galt")), lbts=_number(values.get("lbts")),
        regulating=cast(bool | None, values.get("regulating")), constrained=cast(bool | None, values.get("constrained")),
        reason=reason,
    )
