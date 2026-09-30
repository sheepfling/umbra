from __future__ import annotations

import pytest

from hla.rti1516_2025 import CallbackModel
from umbra_rti_observer import ObserverStore
from umbra_rti_observer_umbra import UmbraRtiAdapter


class Handle:
    def __init__(self, value: str) -> None:
        self.value = value


class FakeAmbassador:
    def __init__(self) -> None:
        self.connected_with = None
        self.disconnected = False

    def connect(self, callback, callback_model, **kwargs):
        self.connected_with = (callback, callback_model, kwargs)

    def disconnect(self) -> None:
        self.disconnected = True


def test_umbra_adapter_connects_and_translates_standard_callbacks() -> None:
    store = ObserverStore()
    rti = FakeAmbassador()
    adapter = UmbraRtiAdapter(
        rti,
        callback_model=CallbackModel.HLA_EVOKED,
        source="umbra-test",
        federation="ExampleFederation",
    )

    store.attach_adapter(adapter)
    callback, callback_model, kwargs = rti.connected_with
    callback.discoverObjectInstance(Handle("object-1"), Handle("class-1"), "Object One", Handle("federate-1"))
    callback.reflectAttributeValues(
        Handle("object-1"),
        {Handle("attribute-1"): b"value"},
        b"tag",
        Handle("receive"),
        Handle("federate-1"),
    )
    callback.receiveInteraction(
        Handle("interaction-1"),
        {Handle("parameter-1"): b"value"},
        b"tag",
        Handle("receive"),
        Handle("federate-1"),
    )
    store.detach_adapter(adapter)

    events = store.events()
    assert callback_model is CallbackModel.HLA_EVOKED
    assert kwargs == {}
    assert events[0]["event_type"] == "runtime.started"
    assert any(event["event_type"] == "object.discovered" for event in events)
    assert any(event["event_type"] == "object.updated" for event in events)
    assert any(event["event_type"] == "interaction.received" for event in events)
    assert events[-1]["event_type"] == "runtime.stopped"
    assert rti.disconnected is True
    assert adapter.connected is False


def test_umbra_adapter_translates_directed_and_time_callbacks() -> None:
    store = ObserverStore()
    rti = FakeAmbassador()
    adapter = UmbraRtiAdapter(rti, callback_model=CallbackModel.HLA_EVOKED)
    store.attach_adapter(adapter)
    callback = rti.connected_with[0]

    callback.receiveDirectedInteraction(
        Handle("interaction-1"),
        Handle("object-1"),
        {Handle("parameter-1"): b"value"},
        b"tag",
        Handle("receive"),
        Handle("federate-1"),
    )
    callback.timeAdvanceGrant(12.5)
    store.detach_adapter(adapter)

    events = store.events()
    directed = next(event for event in events if event["event_type"] == "interaction.received")
    assert directed["directed"] is True
    assert directed["object_handle"] == "object-1"
    assert any(event.get("operation") == "timeAdvanceGrant" for event in events)


def test_connect_failure_is_reported_and_adapter_is_cleaned_up() -> None:
    class FailingAmbassador(FakeAmbassador):
        def connect(self, callback, callback_model, **kwargs):
            raise RuntimeError("connection refused")

    store = ObserverStore()
    adapter = UmbraRtiAdapter(
        FailingAmbassador(),
        callback_model=CallbackModel.HLA_EVOKED,
    )

    with pytest.raises(RuntimeError, match="connection refused"):
        store.attach_adapter(adapter)

    assert adapter.connected is False
    errors = [event for event in store.events() if event["event_type"] == "runtime.error"]
    assert errors and any(event["details"]["operation"] == "connect" for event in errors)
