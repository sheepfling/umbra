"""Persistent time state and capacity-bounded correlation, under the store lock."""

# pyright: strict
from __future__ import annotations

import math
from dataclasses import replace
from time import monotonic

from .attribution import (
    event_participant_metadata,
    participant_event_ended,
    participant_event_perspective,
)
from .models import EventRecord, JsonObject, to_json_object
from .storage import EvidenceBudget, EvidenceIndex
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
    def __init__(
        self,
        capacity: int,
        snapshot_capacity: int = 2000,
        *,
        budget: EvidenceBudget | None = None,
    ) -> None:
        self.capacity = capacity
        shared = budget if budget is not None else EvidenceBudget()
        self._snapshots = EvidenceIndex[TimeScope, TimeSnapshot](
            "time_snapshots", snapshot_capacity, shared, lambda _, row: row.to_json()
        )
        self._requests = EvidenceIndex[tuple[TimeScope, str], TimeRequest](
            "time_requests", capacity, shared, lambda _, row: row.to_json(monotonic())
        )
        self._changed_snapshots: set[TimeScope] = set()
        self._changed_requests: set[tuple[TimeScope, str]] = set()

    def clear(self) -> None:
        self._snapshots.clear()
        self._requests.clear()
        self._changed_snapshots.clear()
        self._changed_requests.clear()

    def record(
        self, event: EventRecord, federate: str | None, federation: str | None
    ) -> None:
        self._changed_snapshots.clear()
        self._changed_requests.clear()
        self._record(event, federate, federation)
        # Mutations happen in the owner, never as a side effect of JSON reads.
        for key in self._changed_snapshots:
            snapshot = self._snapshots.get(key)
            if snapshot is not None and snapshot.last_sequence == event.sequence:
                self._snapshots.refresh(key)
        for key in self._changed_requests:
            row = self._requests.get(key)
            if row is not None and (
                row.request_sequence == event.sequence
                or row.outcome_sequence == event.sequence
            ):
                self._requests.refresh(key)
        self._changed_snapshots.clear()
        self._changed_requests.clear()

    def _record(
        self, event: EventRecord, federate: str | None, federation: str | None
    ) -> None:
        metadata = event_participant_metadata(event)
        perspective = participant_event_perspective(metadata)
        data = time_data(event)
        if data is None:
            session = (
                perspective.attribution.session_id
                if perspective and perspective.attribution
                else metadata.session_id
            )
            if (
                event.event_type in {"session.disconnected", "federate.resigned"}
                and session is not None
            ):

                def affected(scope: TimeScope) -> bool:
                    if scope.perspective is not None:
                        return participant_event_ended(scope.perspective, metadata)
                    return (
                        scope.source == event.source
                        and scope.session_id == session
                        and scope.federation == federation
                        and (
                            federate == scope.federate
                            or (
                                event.event_type == "session.disconnected"
                                and federate is None
                            )
                        )
                    )

                for scope, row in self._snapshots.items():
                    if affected(scope):
                        row.status = "ended"
                        row.last_sequence = event.sequence
                        row.observed_at = event.observed_at
                        self._changed_snapshots.add(scope)
                # Tickets can outlive their latest snapshots. Closing only the
                # snapshot join would let an orphaned ticket accept a late grant.
                for request in self._requests.values():
                    if affected(request.scope) and request.status == "pending":
                        self._finish(
                            request,
                            event,
                            "session-ended",
                            "Explicit session/federate end observed",
                        )
            return
        root_session = metadata.session_id
        session = data.session_id or (
            perspective.session_id
            if perspective
            else root_session
            if isinstance(root_session, str)
            else None
        )
        scope = TimeScope(event.source, session, federation, federate, perspective)
        if any(
            request.scope == scope and request.status == "session-ended"
            for request in self._requests.values()
        ):
            return  # Retained terminal evidence also protects an expired latest scope.
        row = self._snapshots.get(scope)
        if row is None:
            row = TimeSnapshot(scope, event.sequence, event.observed_at)
            self._snapshots[scope] = row
        if row.status == "ended":
            return  # Reconnection requires a fresh session ID, not resurrected pending work.
        self._changed_snapshots.add(scope)
        row.scope = scope
        row.last_sequence = event.sequence
        row.observed_at = event.observed_at
        if data.kind == "state":
            previous = row.reported_state or TimeData(kind="state")
            # None means not reported in this patch; false and zero are facts.
            row.reported_state = replace(
                previous,
                logical_time=data.logical_time
                if data.logical_time is not None
                else previous.logical_time,
                lookahead=data.lookahead
                if data.lookahead is not None
                else previous.lookahead,
                regulating=data.regulating
                if data.regulating is not None
                else previous.regulating,
                constrained=data.constrained
                if data.constrained is not None
                else previous.constrained,
                galt=data.galt if data.galt is not None else previous.galt,
                lbts=data.lbts if data.lbts is not None else previous.lbts,
            )
            row.state_sequence = event.sequence
            return
        if data.kind in {"request", "grant"}:
            sample = TimeSample(
                event.sequence, data.logical_time, event.observed_at, data.request_id
            )
            if data.kind == "request":
                row.last_request = sample
            elif data.kind == "grant":
                row.last_grant = sample
        key = (
            (scope, data.request_id)
            if data.request_id is not None and scope.correlatable
            else None
        )
        if data.kind == "request":
            if key is None:
                row.uncorrelated_requests += 1
                return
            previous_request = self._requests.get(key)
            if previous_request is not None:
                self._finish(
                    previous_request,
                    event,
                    "ambiguous",
                    "Request ID repeated within its retained session scope",
                )
                return
            self._requests[key] = TimeRequest(
                scope,
                key[1],
                event.sequence,
                data.logical_time,
                event.observed_at,
                monotonic(),
            )
        elif data.kind in {"grant", "failed", "cancelled"}:
            request = self._requests.get(key) if key is not None else None
            if request is None or request.status != "pending":
                if data.kind == "grant":
                    row.uncorrelated_grants += 1
                return
            if data.kind == "grant":
                self._finish(request, event, "granted", None)
                duration = monotonic() - request.started_monotonic
                request.latency_seconds = (
                    duration if math.isfinite(duration) and duration >= 0 else None
                )
                request.granted_time = data.logical_time
            else:
                self._finish(request, event, data.kind, data.reason)

    def _finish(
        self,
        request: TimeRequest,
        event: EventRecord,
        status: RequestStatus,
        reason: str | None,
    ) -> None:
        # Only this internal literal set calls the helper; retain a typed status.
        if status not in (
            "granted",
            "failed",
            "cancelled",
            "ambiguous",
            "session-ended",
        ):
            raise ValueError("unknown time request outcome")
        request.status = status
        request.outcome_sequence = event.sequence
        request.outcome_at = event.observed_at
        request.reason = reason
        self._changed_requests.add((request.scope, request.request_id))
        if status != "granted":
            request.latency_seconds = None
            request.granted_time = None

    def to_json(self) -> JsonObject:
        now = monotonic()
        return to_json_object(
            {
                "snapshots": [row.to_json() for row in self._snapshots.values()],
                "requests": [
                    {
                        **row.to_json(now),
                        "snapshot_retained": row.scope in self._snapshots,
                    }
                    for row in self._requests.values()
                ],
                "request_capacity": self.capacity,
                "dropped_requests": self._requests.dropped,
                "snapshot_capacity": self._snapshots.capacity,
                "dropped_snapshots": self._snapshots.dropped,
                "duration_basis": "observer-ingress-monotonic",
                "scope_basis": "explicit-observations",
            }
        )
