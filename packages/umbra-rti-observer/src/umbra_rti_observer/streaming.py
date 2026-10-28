"""One atomic, resumable message flow shared by the optional SSE/WS routes."""

from __future__ import annotations

import asyncio
from collections.abc import AsyncIterator

from .core import ObserverStore
from .schemas import (
    EventsMessage,
    GapMessage,
    HeartbeatMessage,
    RetentionGapResponse,
    SnapshotMessage,
    StateResponse,
    StreamCursorResponse,
    StreamMessage,
)


def _cursor(state: StateResponse, after: int) -> StreamCursorResponse:
    lifetime = state.observer_lifetime
    if lifetime is None:
        raise ValueError("resumable streams require observer lifetime evidence")
    return StreamCursorResponse(
        instance_id=lifetime.instance_id, revision=lifetime.revision, after=after
    )


def _gap(state: StateResponse) -> GapMessage:
    retention = state.retention
    return GapMessage(
        type="gap",
        observer_lifetime=state.observer_lifetime,
        retention=RetentionGapResponse(
            first_retained_sequence=retention.first_retained_sequence,
            last_sequence=retention.last_sequence,
            retained_events=retention.retained_events,
            dropped_events=retention.dropped_events,
            cleared_events=retention.cleared_events,
            next_sequence=retention.next_sequence,
        ),
    )


async def stream_messages(
    store: ObserverStore,
    *,
    after: int = 0,
    instance_id: str | None = None,
    heartbeat_seconds: float = 15.0,
) -> AsyncIterator[StreamMessage]:
    """Resume only a cursor from this store; reconcile unknown/lost windows.

    Each delivered state and its envelopes come from one locked store snapshot.
    A cursor describes envelopes delivered, not merely the server high-water
    mark. Thus a resumed initial snapshot cannot skip its pending backfill.
    """
    state = StateResponse.model_validate(store.state(include_events=True))
    highest = state.retention.next_sequence - 1
    cursor = _cursor(state, highest)
    same_store = instance_id == cursor.instance_id
    lost = after < (
        state.retention.first_retained_sequence - 1
        if state.retention.retained_events
        else highest
    )
    if same_store and after <= highest and not lost:
        summary = state.model_copy(update={"normalized_events": None})
        yield SnapshotMessage(
            type="snapshot",
            state=summary,
            cursor=_cursor(state, after),
            resume="resumed",
        )
        rows = [row for row in state.normalized_events or () if row.sequence > after]
        if rows:
            yield EventsMessage(
                type="events", state=summary, events=rows, cursor=cursor
            )
    else:
        resume = (
            "gap"
            if same_store and lost
            else "reset"
            if instance_id is not None or after
            else "fresh"
        )
        yield SnapshotMessage(
            type="snapshot", state=state, cursor=cursor, resume=resume
        )
        if resume == "gap":
            yield _gap(state)
    while True:
        await asyncio.to_thread(
            store.wait_for_change, cursor.revision, timeout=heartbeat_seconds
        )
        state = StateResponse.model_validate(store.state(include_events=True))
        next_cursor = _cursor(state, state.retention.next_sequence - 1)
        if next_cursor.revision == cursor.revision:
            yield HeartbeatMessage(type="heartbeat", cursor=cursor)
            continue
        lost = cursor.after < (
            state.retention.first_retained_sequence - 1
            if state.retention.retained_events
            else next_cursor.after
        )
        if lost:
            yield _gap(state)
            yield SnapshotMessage(
                type="snapshot", state=state, cursor=next_cursor, resume="gap"
            )
        else:
            yield EventsMessage(
                type="events",
                state=state.model_copy(update={"normalized_events": None}),
                events=[
                    row
                    for row in state.normalized_events or ()
                    if row.sequence > cursor.after
                ],
                cursor=next_cursor,
            )
        cursor = next_cursor
