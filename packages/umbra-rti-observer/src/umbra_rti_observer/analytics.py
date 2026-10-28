"""Read-only, provider-neutral analytics over one consistent observer snapshot.

Counts describe retained observations, never RTI-wide traffic or inferred
ownership. Participant identity comes only from explicit event fields; a source
name is not a participant. This module belongs to the optional web boundary.
"""

from __future__ import annotations

import math
import time
from collections import Counter

from .attribution import (
    ParticipantAttribution,
    ParticipantEventMetadata,
    ParticipantPerspective,
    participant_event_perspective,
)
from .identities import ParticipantIdentity
from .schemas import (
    ActivityBucket,
    AnalyticsResponse,
    AnalyticsWindow,
    EventEnvelope,
    FederateActivity,
    FederateInspectionResponse,
    ParticipantPerspectiveResponse,
    StateResponse,
    TimeGrantObservation,
    TimeObservation,
    TimeRequestObservation,
    TimeStateObservation,
)


def event_federate(event: EventEnvelope) -> str | None:
    perspective = event_perspective(event)
    return perspective.label if perspective else None


def event_perspective(event: EventEnvelope) -> ParticipantPerspective | None:
    attribution = event.participant_attribution
    identity = (
        ParticipantAttribution(
            attribution.session_id,
            attribution.identity_namespace,
            attribution.federation_id,
            ParticipantIdentity(
                attribution.participant.participant_id,
                attribution.participant.membership_id,
                attribution.participant.display_name,
                attribution.participant.handle,
            ),
        )
        if attribution
        else None
    )
    return participant_event_perspective(
        ParticipantEventMetadata(
            source=event.source,
            federation=event.federation,
            session_id=event.session_id,
            federate=event.federate,
            participant=event.participant,
            federate_name=event.federate_name,
            participant_name=event.participant_name,
            time_observation_session_id=(
                event.time_observation.session_id if event.time_observation else None
            ),
            declaration_session_id=(
                event.declaration.session_id if event.declaration else None
            ),
            attribution=identity,
        )
    )


def _roster_perspective(
    row: FederateInspectionResponse,
) -> ParticipantPerspectiveResponse:
    return row.perspective or ParticipantPerspectiveResponse.model_validate(
        ParticipantPerspective(row.source, None, None, row.federate, None).to_json()
    )


def event_logical_time(event: EventEnvelope) -> int | float | None:
    if isinstance(
        event.time_observation,
        (TimeGrantObservation, TimeRequestObservation, TimeStateObservation),
    ):
        return event.time_observation.logical_time
    value: object = event.logical_time
    if value is None and event.details is not None:
        value = event.details.get("logical_time")
    if (
        isinstance(value, (int, float))
        and not isinstance(value, bool)
        and math.isfinite(value)
    ):
        return value
    return None


def build_analytics(
    snapshot: StateResponse | AnalyticsWindow,
    *,
    window_seconds: int = 300,
    bucket_count: int = 30,
    federate: str | None = None,
    perspective: str | None = None,
    now: float | None = None,
) -> AnalyticsResponse:
    """Aggregate observed wall-clock buckets; logical timestamps stay separate."""
    if not 30 <= window_seconds <= 3600 or not 10 <= bucket_count <= 120:
        raise ValueError("window_seconds must be 30..3600 and bucket_count 10..120")
    generated_at = time.time() if now is None else now
    start = generated_at - window_seconds
    bucket_seconds = window_seconds / bucket_count
    capability = (
        snapshot.capabilities.get("participant_provenance")
        if isinstance(snapshot, StateResponse)
        else None
    )
    scoped = (
        (
            snapshot.participant_provenance
            if isinstance(snapshot, AnalyticsWindow)
            else capability is not None and capability.state in {"available", "partial"}
        )
        or perspective is not None
        or any(row.perspective for row in snapshot.federate_roster)
        or any(row.participant_attribution for row in snapshot.normalized_events or ())
    )
    observations = [
        (event, subject)
        for event in snapshot.normalized_events or ()
        for subject in (event_perspective(event),)
        if start <= event.observed_at <= generated_at
        and (federate is None or subject is not None and subject.label == federate)
        and (perspective is None or subject is not None and subject.key == perspective)
    ]
    events = [event for event, _ in observations]
    buckets: list[list[EventEnvelope]] = [[] for _ in range(bucket_count)]
    by_federate: dict[str, list[EventEnvelope]] = {}
    times: list[TimeObservation] = []
    unattributed = 0
    subjects: dict[str, ParticipantPerspective] = {}
    for event, subject in observations:
        index = min(int((event.observed_at - start) / bucket_seconds), bucket_count - 1)
        buckets[index].append(event)
        name = subject.label if subject else None
        if subject is None:
            unattributed += 1
        else:
            key = subject.key if scoped else subject.label
            by_federate.setdefault(key, []).append(event)
            subjects[key] = subject
        logical_time = event_logical_time(event)
        if logical_time is not None:
            times.append(
                TimeObservation(
                    perspective=ParticipantPerspectiveResponse.model_validate(
                        subject.to_json()
                    )
                    if subject and scoped
                    else None,
                    sequence=event.sequence,
                    federate=name,
                    logical_time=logical_time,
                    observed_at=event.observed_at,
                    event_type=event.event_type,
                    operation=event.operation,
                    observation_kind=(
                        event.time_observation.kind
                        if isinstance(
                            event.time_observation,
                            (
                                TimeGrantObservation,
                                TimeRequestObservation,
                                TimeStateObservation,
                            ),
                        )
                        else "grant"
                        if event.event_type == "callback.invoked"
                        and event.operation == "timeAdvanceGrant"
                        else "timestamp"
                    ),
                )
            )

    participants: list[FederateActivity] = []
    roster = {
        _roster_perspective(row).key if scoped else row.federate: row
        for row in snapshot.federate_roster
    }
    retained_roster = set(roster)
    for key, rows in by_federate.items():
        subject = subjects[key]
        if key not in roster:
            latest = rows[-1]
            roster[key] = FederateInspectionResponse(
                federate=subject.label,
                status="observed",
                source=latest.source,
                last_seen=latest.observed_at,
                perspective=ParticipantPerspectiveResponse.model_validate(
                    subject.to_json()
                )
                if scoped
                else None,
            )
    for row in sorted(roster.values(), key=lambda item: item.federate):
        subject = _roster_perspective(row)
        if (
            federate is not None
            and row.federate != federate
            and (subject.key if scoped else row.federate) not in by_federate
        ):
            continue
        if perspective is not None and subject.key != perspective:
            continue
        observed = by_federate.get(subject.key if scoped else row.federate, [])
        logical = [event_logical_time(event) for event in observed]
        object_keys = {
            key
            for event in observed
            if event.event_type.startswith("object.")
            for key in [event.object_key]
            if isinstance(key, str)
        }
        participants.append(
            FederateActivity(
                perspective=subject if scoped else None,
                federate=row.federate,
                role=row.role,
                handle=row.handle,
                status=row.status,
                source=row.source,
                last_seen=row.last_seen,
                roster_retained=(subject.key if scoped else row.federate)
                in retained_roster,
                event_count=len(observed),
                object_updates=sum(
                    event.event_type == "object.updated" for event in observed
                ),
                interaction_count=sum(
                    event.event_type.startswith("interaction.") for event in observed
                ),
                error_count=sum(
                    event.event_type == "runtime.error" for event in observed
                ),
                object_keys=tuple(sorted(object_keys)),
                latest_logical_time=next(
                    (value for value in reversed(logical) if value is not None), None
                ),
            )
        )

    return AnalyticsResponse(
        contract_version=snapshot.contract_version,
        basis="retained-observations",
        generated_at=generated_at,
        window_start=start,
        window_seconds=window_seconds,
        bucket_seconds=bucket_seconds,
        scoped_federate=federate,
        scoped_perspective=perspective,
        events_in_window=len(events),
        unattributed_events=unattributed,
        history_truncated=snapshot.retention.dropped_events > 0
        or snapshot.retention.cleared_events > 0,
        retention=snapshot.retention,
        participants=tuple(participants),
        roster_storage=next(
            (
                row
                for row in snapshot.storage.evidence.collections
                if row.collection == "federates"
            ),
            None,
        )
        if snapshot.storage is not None
        else None,
        activity=tuple(
            ActivityBucket(
                started_at=start + index * bucket_seconds,
                events=len(rows),
                object_updates=sum(
                    event.event_type == "object.updated" for event in rows
                ),
                interactions=sum(
                    event.event_type.startswith("interaction.") for event in rows
                ),
                callbacks=sum(
                    event.event_type.startswith("callback.") for event in rows
                ),
                errors=sum(event.event_type == "runtime.error" for event in rows),
            )
            for index, rows in enumerate(buckets)
        ),
        logical_times=tuple(times),
        event_types=dict(Counter(event.event_type for event in events)),
        sources=dict(Counter(event.source for event in events)),
        families=dict(Counter(event.family for event in events)),
    )
