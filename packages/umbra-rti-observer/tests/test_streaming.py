"""Application delivery evidence only; no RTI/IEEE conformance claim."""
from __future__ import annotations

import asyncio

import pytest

pytest.importorskip("fastapi")
from fastapi.testclient import TestClient
from pydantic import ValidationError
from umbra_rti_observer.core import ObserverStore
from umbra_rti_observer.schemas import (
    EventsMessage,
    SnapshotMessage,
    StateResponse,
    StreamCursorResponse,
)
from umbra_rti_observer.server import create_app
from umbra_rti_observer.streaming import stream_messages


def emit(store: ObserverStore, count: int = 1) -> None:
    store.publish_many([{"event_type": "callback.invoked", "source": "stream-test", "operation": str(index)} for index in range(count)])


def identity(store: ObserverStore) -> str:
    lifetime = StateResponse.model_validate(store.state()).observer_lifetime
    assert lifetime is not None
    return lifetime.instance_id


def test_observer_lifetime_is_independent_monotonic_and_does_not_count_reads_or_rejected_batches() -> None:
    store = ObserverStore()
    first = store.state()["observer_lifetime"]
    assert isinstance(first, dict)
    assert first != ObserverStore().state()["observer_lifetime"]
    store.state()
    store.checkpoint_capture()
    assert store.state()["observer_lifetime"] == first
    emit(store, 3)
    store.clear()
    store.clear()
    store.set_context(note="metadata wakes a stream")
    before = store.state()["observer_lifetime"]
    assert isinstance(before, dict) and before["revision"] == 4
    with pytest.raises(TypeError):
        store.publish_many([{"event_type": "callback.invoked"}, {"event_type": "callback.invoked", "federate": []}])
    assert store.state()["observer_lifetime"] == before
    assert identity(store) == first["instance_id"]


def test_fresh_snapshot_is_complete_and_heartbeat_has_the_delivered_cursor() -> None:
    async def check() -> None:
        store = ObserverStore()
        emit(store, 3)
        stream = stream_messages(store, heartbeat_seconds=0.001)
        first = await anext(stream)
        assert isinstance(first, SnapshotMessage) and first.resume == "fresh"
        assert [row.sequence for row in first.state.normalized_events or ()] == [1, 2, 3]
        assert first.cursor is not None and first.cursor.after == 3
        heartbeat = await anext(stream)
        assert heartbeat.type == "heartbeat" and heartbeat.cursor == first.cursor
        await stream.aclose()
    asyncio.run(check())


def test_resumed_snapshot_and_backfill_share_a_cut_even_if_more_events_arrive_between_yields() -> None:
    async def check() -> None:
        store = ObserverStore()
        emit(store, 3)
        stream = stream_messages(store, after=1, instance_id=identity(store))
        first = await anext(stream)
        assert isinstance(first, SnapshotMessage) and first.resume == "resumed"
        assert first.state.normalized_events is None and first.cursor is not None and first.cursor.after == 1
        emit(store, 2)
        backfill = await anext(stream)
        assert isinstance(backfill, EventsMessage) and [row.sequence for row in backfill.events] == [2, 3]
        assert backfill.cursor is not None and backfill.cursor.after == 3
        assert backfill.cursor.revision == first.cursor.revision
        live = await anext(stream)
        assert isinstance(live, EventsMessage) and [row.sequence for row in live.events] == [4, 5]
        assert live.state.retention.next_sequence == 6
        await stream.aclose()
    asyncio.run(check())


@pytest.mark.parametrize("foreign,ahead,expected", [(True, False, "reset"), (False, True, "reset"), (None, False, "reset")])
def test_unknown_or_future_cursors_reset_without_borrowing_a_previous_store(foreign: bool | None, ahead: bool, expected: str) -> None:
    async def check() -> None:
        store = ObserverStore()
        emit(store, 2)
        identifier = "previous-observer" if foreign else identity(store) if foreign is False else None
        stream = stream_messages(store, after=100 if ahead else 1, instance_id=identifier)
        first = await anext(stream)
        assert isinstance(first, SnapshotMessage) and first.resume == expected
        assert first.state.normalized_events is not None and len(first.state.normalized_events) == 2
        assert first.cursor is not None and first.cursor.instance_id == identity(store) and first.cursor.after == 2
        await stream.aclose()
    asyncio.run(check())


@pytest.mark.parametrize("cleared", [False, True])
def test_lost_resume_window_is_explicit_and_reconciles_the_actual_retained_window(cleared: bool) -> None:
    async def check() -> None:
        store = ObserverStore(max_events=2)
        emit(store, 5)
        if cleared:
            store.clear()
        stream = stream_messages(store, after=1, instance_id=identity(store))
        first = await anext(stream)
        assert isinstance(first, SnapshotMessage) and first.resume == "gap"
        assert [row.sequence for row in first.state.normalized_events or ()] == ([] if cleared else [4, 5])
        assert first.cursor is not None and first.cursor.after == 5
        assert (await anext(stream)).type == "gap"
        await stream.aclose()
    asyncio.run(check())


def test_empty_clear_and_metadata_are_delivered_without_waiting_for_another_event() -> None:
    async def check() -> None:
        store = ObserverStore()
        emit(store)
        stream = stream_messages(store)
        first = await anext(stream)
        assert isinstance(first, SnapshotMessage)
        for operation in (store.clear, lambda: store.set_context(note="changed"), store.clear):
            waiting = asyncio.create_task(anext(stream))
            await asyncio.sleep(0)
            operation()
            frame = await asyncio.wait_for(waiting, timeout=1)
            assert isinstance(frame, EventsMessage) and not frame.events
            assert frame.state.retention.retained_events == 0 and not frame.state.inspectors.objects
        await stream.aclose()
    asyncio.run(check())


def test_ws_resume_and_disabled_transport_discovery() -> None:
    store = ObserverStore()
    emit(store, 3)
    with TestClient(create_app(store, enable_sse=False)) as client:
        contract = client.get("/api/contract").json()
        assert "event_stream" not in contract["api_paths"] and contract["stream"]["resumption"]
        assert client.get("/api/events/stream").status_code == 501
        with client.websocket_connect(f"/ws/events?after=2&instance_id={identity(store)}") as socket:
            first = socket.receive_json()
            assert first["resume"] == "resumed" and first["cursor"]["after"] == 2
            assert [row["sequence"] for row in socket.receive_json()["events"]] == [3]
    with TestClient(create_app(ObserverStore(), enable_websocket=False, enable_sse=False)) as client:
        contract = client.get("/api/contract").json()
        assert not contract["stream"]["resumption"] and "websocket" not in contract["api_paths"]


def test_named_stream_models_reject_foreign_revision_cursor_and_duplicate_or_out_of_window_rows() -> None:
    store = ObserverStore()
    emit(store, 2)
    state = StateResponse.model_validate(store.state())
    for identifier, revision, after in (("other", 1, 2), (identity(store), 0, 2), (identity(store), 1, 3)):
        with pytest.raises(ValidationError):
            SnapshotMessage(type="snapshot", state=state,
                cursor=StreamCursorResponse(instance_id=identifier, revision=revision, after=after))
    assert state.normalized_events is not None
    with pytest.raises(ValidationError):
        EventsMessage(type="events", state=state, events=[state.normalized_events[0], state.normalized_events[0]])


def test_http_cursor_identifies_its_store_and_resets_foreign_or_future_positions() -> None:
    store = ObserverStore()
    emit(store, 3)
    with TestClient(create_app(store)) as client:
        for identifier, after, expected, sequences in ((identity(store), 2, False, [3]),
                ("old-instance", 100, True, [1, 2, 3]), (identity(store), 100, True, [1, 2, 3])):
            response = client.get("/api/events", params={"instance_id": identifier, "after": after}).json()
            assert response["reset"] is expected
            assert response["observer_lifetime"]["instance_id"] == identity(store)
            assert [row["sequence"] for row in response["events"]] == sequences
        assert client.get("/api/events/stream?after=-1").status_code == 422
        assert client.get("/api/events/stream?instance_id=").status_code == 422


def test_openapi_carries_named_stream_models_and_resolvable_serialization_references() -> None:
    from jsonschema import Draft202012Validator

    store = ObserverStore()
    emit(store)
    document = create_app(store).openapi()
    models = document["components"]["schemas"]
    assert set(document["x-observer-stream-messages"]) == {
        "SnapshotMessage", "EventsMessage", "GapMessage", "HeartbeatMessage", "ErrorMessage"}
    state = StateResponse.model_validate(store.state())
    lifetime = state.observer_lifetime
    assert lifetime is not None
    frame = SnapshotMessage(type="snapshot", state=state,
        cursor=StreamCursorResponse(instance_id=lifetime.instance_id, revision=lifetime.revision, after=1))
    Draft202012Validator({"$ref": "#/components/schemas/SnapshotMessage", **document}).validate(frame.model_dump(mode="json"))
    assert "cursor" in models["SnapshotMessage"]["properties"]
