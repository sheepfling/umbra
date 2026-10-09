from __future__ import annotations

import threading

import pytest

from umbra_rti_observer import (
    OBSERVER_CONTRACT_VERSION,
    CallbackEventSource,
    ObserverStore,
    build_contract_document,
    build_event_schema,
)


def test_store_keeps_generic_semantics_and_builds_inspectors() -> None:
    store = ObserverStore(max_events=20)
    store.set_context(connection="memory://test", federation="Example")
    store.set_status("running")
    store.publish(
        {
            "event_type": "federate.joined",
            "federate": "one",
            "role": "publisher",
            "source": "test-source",
        }
    )
    store.publish(
        {
            "event_type": "object.discovered",
            "object_key": "object-1",
            "object_name": "Object One",
            "class_name": "Class One",
            "attributes": {"value": 7},
        }
    )
    store.publish(
        {
            "event_type": "interaction.received",
            "interaction_key": "interaction-1",
            "interaction_class": "Class Interaction",
            "parameters": {"value": 8},
        }
    )

    state = store.state()
    assert state["contract_version"] == OBSERVER_CONTRACT_VERSION
    assert state["status"] == "running"
    assert state["context"]["federation"] == "Example"
    assert state["live_metrics"]["event_count"] == 3
    assert state["live_metrics"]["object_count"] == 1
    assert state["live_metrics"]["interaction_count"] == 1
    assert state["federate_roster"][0]["federate"] == "one"
    assert state["inspectors"]["objects"][0]["family"] == "generic"
    assert state["normalized_events"][1]["family"] == "generic"


def test_callback_source_can_feed_store() -> None:
    source = CallbackEventSource()
    store = ObserverStore()
    store.attach(source)
    source.emit({"event_type": "callback.invoked", "operation": "receive"})
    assert store.events()[0]["event_type"] == "callback.invoked"
    store.detach(source)
    source.emit({"event_type": "callback.invoked", "operation": "ignored"})
    assert len(store.events()) == 1


def test_provider_adapter_lifecycle_is_kept_out_of_the_store() -> None:
    class FakeAdapter:
        def start(self, sink) -> None:
            self.sink = sink
            sink({"event_type": "runtime.started", "source": "fake"})

        def stop(self) -> None:
            self.stopped = True

    adapter = FakeAdapter()
    store = ObserverStore()
    store.attach_adapter(adapter)
    store.detach_adapter(adapter)
    assert store.events()[0]["source"] == "fake"
    assert adapter.stopped is True


def test_wait_for_events_unblocks_on_publish() -> None:
    store = ObserverStore()
    received: list[dict[str, object]] = []

    def wait() -> None:
        received.extend(store.wait_for_events(timeout=2.0))

    thread = threading.Thread(None, wait)
    thread.start()
    store.publish({"event_type": "runtime.started"})
    thread.join(timeout=2.0)
    assert received and received[0]["event_type"] == "runtime.started"


def test_schema_is_provider_neutral() -> None:
    schema = build_event_schema()
    assert schema["schema_version"] == "umbra-runtime-observer-v1"
    assert "event_type" in schema["required"]
    assert schema["properties"]["family"]["description"].startswith("Explicit")


def test_contract_is_language_neutral_and_declares_stream_messages() -> None:
    contract = build_contract_document(["custom.event"])
    assert contract["contract_version"] == OBSERVER_CONTRACT_VERSION
    assert contract["event_types"] == ["custom.event"]
    assert contract["api_paths"]["websocket"] == "/ws/events"
    assert "snapshot" in contract["stream"]["message_types"]


def test_bounded_history_reports_cursor_gaps_and_filters() -> None:
    store = ObserverStore(max_events=2)
    store.publish({"event_type": "runtime.started", "family": "lifecycle"})
    store.publish({"event_type": "callback.invoked", "family": "callback"})
    store.publish({"event_type": "runtime.stopped", "family": "lifecycle"})

    page = store.event_page(after_sequence=0, event_types=["runtime.stopped"])
    assert page["gap"] is True
    assert page["dropped_events"] == 1
    assert [event["sequence"] for event in page["events"]] == [3]
    assert page["next_after"] == 3
    assert store.state(include_events=False)["retention"]["first_retained_sequence"] == 2


def test_live_inspectors_survive_event_history_eviction() -> None:
    store = ObserverStore(max_events=1)
    store.publish(
        {
            "event_type": "object.discovered",
            "object_key": "object-1",
            "object_name": "Persistent object",
        }
    )
    store.publish({"event_type": "runtime.started"})

    state = store.state(include_events=False)
    assert state["retention"]["retained_events"] == 1
    assert state["inspectors"]["objects"][0]["object_name"] == "Persistent object"


def test_publish_many_is_atomic_when_a_row_is_invalid() -> None:
    store = ObserverStore()
    store.publish({"event_type": "runtime.started"})

    with pytest.raises(TypeError):
        store.publish_many(
            [
                {"event_type": "callback.invoked"},
                "not an event",
            ]
        )

    assert [event["sequence"] for event in store.events()] == [1]


def test_callback_source_isolates_sink_failures() -> None:
    source = CallbackEventSource()
    received: list[dict[str, object]] = []

    def broken_sink(_event) -> None:
        raise RuntimeError("diagnostic sink failed")

    source.subscribe(broken_sink)
    source.subscribe(received.append)
    source.emit({"event_type": "callback.invoked"})

    assert received == [{"event_type": "callback.invoked"}]


def test_adapter_attachment_is_idempotent_and_close_stops_it() -> None:
    class FakeAdapter:
        name = "fake"

        def __init__(self) -> None:
            self.starts = 0
            self.stops = 0

        def start(self, sink) -> None:
            self.starts += 1
            sink({"event_type": "runtime.started", "source": "fake"})

        def stop(self) -> None:
            self.stops += 1

    adapter = FakeAdapter()
    store = ObserverStore()
    store.attach_adapter(adapter)
    store.attach_adapter(adapter)
    store.close()

    assert adapter.starts == 1
    assert adapter.stops == 1
    assert store.state(include_events=False)["status"] == "stopped"
    assert store.state(include_events=False)["adapters"] == []
