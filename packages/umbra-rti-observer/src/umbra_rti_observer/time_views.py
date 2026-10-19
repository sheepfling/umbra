"""Persistent time state and capacity-bounded correlation, under the store lock."""

# pyright: strict
from __future__ import annotations

import math
from collections import OrderedDict
from dataclasses import replace
from time import monotonic

from .models import EventRecord, JsonObject, to_json_object
from .time_models import (
    RequestStatus,
    TimeData,
    TimeRequest,
    TimeSample,
    TimeScope,
    TimeSnapshot,
    time_data,
)


class TimeViews:
    def __init__(self, capacity: int) -> None:
        self.capacity = capacity
        self._snapshots: dict[TimeScope, TimeSnapshot] = {}
        self._requests: OrderedDict[tuple[TimeScope, str], TimeRequest] = OrderedDict()
        self._dropped = 0

    def clear(self) -> None:
        self._snapshots.clear()
        self._requests.clear()
        self._dropped = 0

    def record(self, event: EventRecord, federate: str | None, federation: str | None) -> None:
        data = time_data(event)
        if data is None:
            session = event.get("session_id")
            if event.event_type in {"session.disconnected", "federate.resigned"} and isinstance(session, str):
                for scope, row in self._snapshots.items():
                    if (scope.source == event.source and scope.session_id == session and scope.federation == federation
                            and (federate == scope.federate or (event.event_type == "session.disconnected" and federate is None))):
                        row.status = "ended"
                        row.last_sequence = event.sequence
                        row.observed_at = event.observed_at
                        for request in self._requests.values():
                            if request.scope == scope and request.status == "pending":
                                self._finish(request, event, "session-ended", "Explicit session/federate end observed")
            return
        scope = TimeScope(event.source, data.session_id, federation, federate)
        row = self._snapshots.get(scope)
        if row is None:
            row = TimeSnapshot(scope, event.sequence, event.observed_at)
            self._snapshots[scope] = row
        if row.status == "ended":
            return  # Reconnection requires a fresh session ID, not resurrected pending work.
        row.last_sequence = event.sequence
        row.observed_at = event.observed_at
        if data.kind == "state":
            previous = row.reported_state or TimeData(kind="state")
            # None means not reported in this patch; false and zero are facts.
            row.reported_state = replace(
                previous,
                logical_time=data.logical_time if data.logical_time is not None else previous.logical_time,
                lookahead=data.lookahead if data.lookahead is not None else previous.lookahead,
                regulating=data.regulating if data.regulating is not None else previous.regulating,
                constrained=data.constrained if data.constrained is not None else previous.constrained,
                galt=data.galt if data.galt is not None else previous.galt,
                lbts=data.lbts if data.lbts is not None else previous.lbts,
            )
            row.state_sequence = event.sequence
            return
        if data.kind in {"request", "grant"}:
            sample = TimeSample(event.sequence, data.logical_time, event.observed_at, data.request_id)
            if data.kind == "request":
                row.last_request = sample
            elif data.kind == "grant":
                row.last_grant = sample
        key = (scope, data.request_id) if data.request_id is not None and scope.correlatable else None
        if data.kind == "request":
            if key is None:
                row.uncorrelated_requests += 1
                return
            previous_request = self._requests.get(key)
            if previous_request is not None:
                self._finish(previous_request, event, "ambiguous", "Request ID repeated within its retained session scope")
                return
            if len(self._requests) == self.capacity:
                self._requests.popitem(last=False)
                self._dropped += 1
            self._requests[key] = TimeRequest(scope, key[1], event.sequence, data.logical_time, event.observed_at, monotonic())
        elif data.kind in {"grant", "failed", "cancelled"}:
            request = self._requests.get(key) if key is not None else None
            if request is None or request.status != "pending":
                if data.kind == "grant":
                    row.uncorrelated_grants += 1
                return
            if data.kind == "grant":
                self._finish(request, event, "granted", None)
                duration = monotonic() - request.started_monotonic
                request.latency_seconds = duration if math.isfinite(duration) and duration >= 0 else None
                request.granted_time = data.logical_time
            else:
                self._finish(request, event, data.kind, data.reason)

    @staticmethod
    def _finish(request: TimeRequest, event: EventRecord, status: RequestStatus, reason: str | None) -> None:
        # Only this internal literal set calls the helper; retain a typed status.
        if status not in ("granted", "failed", "cancelled", "ambiguous", "session-ended"):
            raise ValueError("unknown time request outcome")
        request.status = status
        request.outcome_sequence = event.sequence
        request.outcome_at = event.observed_at
        request.reason = reason
        if status != "granted":
            request.latency_seconds = None
            request.granted_time = None

    def to_json(self) -> JsonObject:
        now = monotonic()
        return to_json_object({
            "snapshots": [row.to_json() for row in self._snapshots.values()],
            "requests": [row.to_json(now) for row in self._requests.values()],
            "request_capacity": self.capacity, "dropped_requests": self._dropped,
            "duration_basis": "observer-ingress-monotonic", "scope_basis": "explicit-observations",
        })
