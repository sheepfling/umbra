"""Read-only, provider-neutral analytics over one consistent observer snapshot.

Counts describe retained observations, never RTI-wide traffic or inferred
ownership. Participant identity comes only from explicit event fields; a source
name is not a participant. This module belongs to the optional web boundary.
"""

from __future__ import annotations

import math
import time
from collections import Counter

from .schemas import (
    ActivityBucket,
    AnalyticsResponse,
    EventEnvelope,
    FederateActivity,
    StateResponse,
    TimeGrantObservation,
    TimeObservation,
    TimeRequestObservation,
    TimeStateObservation,
)


def event_federate(event: EventEnvelope) -> str | None:
    value = event.federate if event.federate is not None else event.participant
    if isinstance(value, dict):
        for key in ("name", "id", "handle"):
            item = value.get(key)
            if isinstance(item, (str, int)) and not isinstance(item, bool) and str(item).strip():
                return str(item).strip()
    elif isinstance(value, str) and value.strip():
        return value.strip()
    return event.federate_name or event.participant_name


def event_logical_time(event: EventEnvelope) -> int | float | None:
    if isinstance(event.time_observation, (TimeGrantObservation, TimeRequestObservation, TimeStateObservation)):
        return event.time_observation.logical_time
    value: object = event.logical_time
    if value is None and event.details is not None:
        value = event.details.get("logical_time")
    if isinstance(value, (int, float)) and not isinstance(value, bool) and math.isfinite(value):
        return value
    return None


def build_analytics(
    snapshot: StateResponse,
    *,
    window_seconds: int = 300,
    bucket_count: int = 30,
    federate: str | None = None,
    now: float | None = None,
) -> AnalyticsResponse:
    """Aggregate observed wall-clock buckets; logical timestamps stay separate."""
    if not 30 <= window_seconds <= 3600 or not 10 <= bucket_count <= 120:
        raise ValueError("window_seconds must be 30..3600 and bucket_count 10..120")
    generated_at = time.time() if now is None else now
    start = generated_at - window_seconds
    bucket_seconds = window_seconds / bucket_count
    events = [
        event for event in snapshot.normalized_events or ()
        if start <= event.observed_at <= generated_at
        and (federate is None or event_federate(event) == federate)
    ]
    buckets: list[list[EventEnvelope]] = [[] for _ in range(bucket_count)]
    by_federate: dict[str, list[EventEnvelope]] = {}
    times: list[TimeObservation] = []
    unattributed = 0
    for event in events:
        index = min(int((event.observed_at - start) / bucket_seconds), bucket_count - 1)
        buckets[index].append(event)
        name = event_federate(event)
        if name is None:
            unattributed += 1
        else:
            by_federate.setdefault(name, []).append(event)
        logical_time = event_logical_time(event)
        if logical_time is not None:
            times.append(TimeObservation(
                sequence=event.sequence, federate=name, logical_time=logical_time,
                observed_at=event.observed_at, event_type=event.event_type,
                operation=event.operation,
                observation_kind=(event.time_observation.kind if isinstance(event.time_observation,
                    (TimeGrantObservation, TimeRequestObservation, TimeStateObservation)) else
                    "grant" if event.event_type == "callback.invoked" and event.operation == "timeAdvanceGrant" else "timestamp"),
            ))

    participants: list[FederateActivity] = []
    for row in snapshot.federate_roster:
        if federate is not None and row.federate != federate:
            continue
        observed = by_federate.get(row.federate, [])
        logical = [event_logical_time(event) for event in observed]
        object_keys = {
            key for event in observed
            if event.event_type.startswith("object.")
            for key in [event.object_key]
            if isinstance(key, str)
        }
        participants.append(FederateActivity(
            federate=row.federate, role=row.role, handle=row.handle,
            status=row.status, source=row.source, last_seen=row.last_seen,
            event_count=len(observed),
            object_updates=sum(event.event_type == "object.updated" for event in observed),
            interaction_count=sum(event.event_type.startswith("interaction.") for event in observed),
            error_count=sum(event.event_type == "runtime.error" for event in observed),
            object_keys=tuple(sorted(object_keys)),
            latest_logical_time=next((value for value in reversed(logical) if value is not None), None),
        ))

    return AnalyticsResponse(
        contract_version=snapshot.contract_version, basis="retained-observations",
        generated_at=generated_at, window_start=start, window_seconds=window_seconds,
        bucket_seconds=bucket_seconds, scoped_federate=federate,
        events_in_window=len(events), unattributed_events=unattributed,
        history_truncated=snapshot.retention.dropped_events > 0 or snapshot.retention.cleared_events > 0,
        retention=snapshot.retention, participants=tuple(participants),
        activity=tuple(ActivityBucket(
            started_at=start + index * bucket_seconds, events=len(rows),
            object_updates=sum(event.event_type == "object.updated" for event in rows),
            interactions=sum(event.event_type.startswith("interaction.") for event in rows),
            callbacks=sum(event.event_type.startswith("callback.") for event in rows),
            errors=sum(event.event_type == "runtime.error" for event in rows),
        ) for index, rows in enumerate(buckets)),
        logical_times=tuple(times),
        event_types=dict(Counter(event.event_type for event in events)),
        sources=dict(Counter(event.source for event in events)),
        families=dict(Counter(event.family for event in events)),
    )
