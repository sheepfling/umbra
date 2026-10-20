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
    contract = build_contract_document(
        ["custom.event"],
        capabilities={"logical_time": {"state": "available", "source": "test"}},
    )
    assert contract["contract_version"] == OBSERVER_CONTRACT_VERSION
    assert contract["event_types"] == ["custom.event"]
    assert contract["stream"]["websocket_path"] == "/ws/events"
    assert "websocket" not in contract["api_paths"]  # Unknown is not usable support.
    assert "snapshot" in contract["stream"]["message_types"]
    assert "unsupported" in contract["capability_schema"]["states"]
    assert contract["capabilities"]["logical_time"]["state"] == "available"


@pytest.mark.parametrize("feature", ["declarations", "relationships", "synchronization", "persistence_inspection", "time_inspection", "interaction_flow", "ownership_inspection", "regions_inspection", "mom_inspection", "fom_inspection"])
def test_optional_inspector_support_does_not_upgrade_unsupported_reporters(feature: str) -> None:
    from umbra_rti_observer.capabilities import merge_capabilities

    class Adapter:
        name = "unsupported-reporter"
        capabilities = {feature: "unsupported"}

        def start(self, sink: object) -> None:
            pass

        def stop(self) -> None:
            pass

    store = ObserverStore()
    adapter = Adapter()
    store.attach_adapter(adapter)
    assert store.capabilities()[feature]["state"] == "unsupported"
    assert store.capabilities()["event_history"]["state"] == "available"
    store.set_capabilities({feature: "disabled"})
    assert store.capabilities()[feature]["state"] == "disabled"
    store.close()
    assert merge_capabilities({feature: "unsupported"}, {feature: "disabled"})[feature].state == "disabled"
    assert merge_capabilities({feature: "available"}, {feature: "unsupported"})[feature].state == "partial"


def test_capabilities_distinguish_provider_features_from_observer_features() -> None:
    store = ObserverStore(
        capabilities={
            "logical_time": {
                "state": "partial",
                "reason": "Only receive-side timestamps are exposed.",
            },
            "mom": "unsupported",
        }
    )

    capabilities = store.capabilities()

    assert capabilities["event_history"]["state"] == "available"
    assert capabilities["logical_time"]["state"] == "partial"
    assert capabilities["mom"]["state"] == "unsupported"


def test_application_capability_state_can_disable_adapter_surface() -> None:
    class FakeAdapter:
        capabilities = {"mom": "available"}

        def start(self, sink) -> None:
            pass

        def stop(self) -> None:
            pass

    store = ObserverStore(capabilities={"mom": "disabled"})
    store.attach_adapter(FakeAdapter())

    assert store.capabilities()["mom"]["state"] == "disabled"


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
        capabilities = {"logical_time": "available"}

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


def test_adapter_capabilities_follow_adapter_lifecycle() -> None:
    class FakeAdapter:
        name = "capability-fake"
        capabilities = {
            "logical_time": "available",
            "mom": {"state": "partial", "reason": "limited test surface"},
        }

        def start(self, sink) -> None:
            self.sink = sink

        def stop(self) -> None:
            pass

    adapter = FakeAdapter()
    store = ObserverStore()
    store.attach_adapter(adapter)

    capabilities = store.state(include_events=False)["capabilities"]
    assert capabilities["logical_time"]["state"] == "available"
    assert capabilities["mom"]["state"] == "partial"
    assert "capability-fake" in capabilities["mom"]["sources"]

    store.detach_adapter(adapter)
    assert "logical_time" not in store.state(include_events=False)["capabilities"]
