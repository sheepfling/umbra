"""Internal observer pairing evidence; not proof of RTI delivery or conformance."""
from __future__ import annotations

from unittest.mock import patch

import pytest
from fastapi.testclient import TestClient
from pydantic import ValidationError

from umbra_rti_observer.core import ObserverStore
from umbra_rti_observer.schemas import EventInput, InteractionFlowResponse, StateResponse
from umbra_rti_observer.server import create_app


def event(kind: str = "send", **overrides: object) -> dict[str, object]:
    return {"event_type": "interaction.sent" if kind == "send" else "interaction.received", "source": "adapter", "interaction_flow": {
        "kind": kind, "order": "receive-order", "message_id": "transmission", "identity_namespace": "provider", "federation_id": "fed",
        "session_id": "session", "producer": {"participant_id": "a", "membership_id": "a1", "display_name": "Same"},
        "recipient": {"participant_id": "b", "membership_id": "b1", "display_name": "Same"}, **overrides,
    }}


def inspect(store: ObserverStore) -> InteractionFlowResponse:
    result = StateResponse.model_validate(store.state()).inspectors.interaction_flow
    assert result is not None
    return result


def test_explicit_ids_and_endpoints_pair_across_reporters_without_wall_clock_latency() -> None:
    store = ObserverStore(max_events=1)
    with patch("umbra_rti_observer.interaction_flow.monotonic", side_effect=[100, 100.25]):
        store.publish(event(order="timestamped", logical_time=5))
        store.publish({**event("receive", order="timestamped", logical_time=5, session_id="recipient-session"), "source": "other-adapter"})
    row = inspect(store).records[0]
    assert row.status == "paired" and row.arrival_delay_seconds == 0.25
    assert row.send.source != row.receive.source and row.send.session_id != row.receive.session_id
    store.publish({"event_type": "runtime.error"})
    assert inspect(store).records[0].status == "paired"


@pytest.mark.parametrize("field", ["message_id", "identity_namespace", "federation_id", "producer", "recipient"])
def test_missing_identity_keeps_useful_counts_without_pairing(field: str) -> None:
    store = ObserverStore()
    store.publish(event(**{field: None}))
    store.publish(event("receive", **{field: None}))
    result = inspect(store)
    assert len(result.records) == 2 and all(row.status == "uncorrelated" for row in result.records)
    assert result.send_observations == result.receive_observations == 1
    assert all(row.arrival_delay_seconds is None for row in result.records)


@pytest.mark.parametrize("field", ["message_id", "identity_namespace", "federation_id", "producer", "recipient"])
def test_different_scope_or_join_never_pairs_even_with_identical_names(field: str) -> None:
    store = ObserverStore()
    store.publish(event())
    value: object = {"participant_id": "a" if field == "producer" else "b", "membership_id": "rejoin", "display_name": "Same"} if field in ("producer", "recipient") else "different"
    store.publish(event("receive", **{field: value}))
    assert {row.status for row in inspect(store).records} == {"send-only", "receive-only"}


def test_duplicate_send_or_receive_invalidates_previous_pair_and_preserves_original_reports() -> None:
    store = ObserverStore()
    store.publish(event())
    store.publish(event("receive"))
    assert inspect(store).records[0].status == "paired"
    store.publish(event())
    store.publish(event("receive"))
    row = inspect(store).records[0]
    assert row.status == "ambiguous" and row.arrival_delay_seconds is None
    assert row.send_count == row.receive_count == 2
    assert row.send.sequence == 1 and row.receive.sequence == 2 and row.last_sequence == 4


@pytest.mark.parametrize("arrivals", [(100, 99), (float("nan"), 100), (100, float("inf"))])
def test_bad_monotonic_intervals_are_unknown_not_zero(arrivals: tuple[float, float]) -> None:
    store = ObserverStore()
    with patch("umbra_rti_observer.interaction_flow.monotonic", side_effect=arrivals):
        store.publish(event())
        store.publish(event("receive"))
    assert inspect(store).records[0].arrival_delay_seconds is None


def test_receive_before_send_pairs_evidence_without_inventing_a_positive_delay() -> None:
    store = ObserverStore()
    store.publish(event("receive"))
    store.publish(event())
    row = inspect(store).records[0]
    assert row.status == "paired" and row.arrival_delay_seconds is None


def test_exact_legacy_types_count_without_inferred_order_endpoints_or_delivery() -> None:
    store = ObserverStore()
    for event_type in ("interaction.sent", "interaction.received", "interaction.invoked", "callback.invoked"):
        store.publish({"event_type": event_type, "interaction_class": "C", "logical_time": 5, "federate": "Same"})
    data = inspect(store)
    assert len(data.records) == 2 and data.send_observations == data.receive_observations == 1
    assert all(row.status == "uncorrelated" for row in data.records)
    assert data.records[0].send.order == "unknown" and data.records[1].receive.logical_time is None


def test_bounded_records_keep_totals_and_clear_resets_all_evidence() -> None:
    store = ObserverStore(max_interaction_flow=2)
    for identity in ("one", "two", "one", "three"):
        store.publish(event(message_id=identity))
    result = inspect(store)
    assert {row.send.message_id for row in result.records} == {"one", "three"}
    assert result.dropped_records == 1 and result.send_observations == 4
    store.clear()
    result = inspect(store)
    assert not result.records and result.send_observations == result.receive_observations == result.dropped_records == 0


@pytest.mark.parametrize("invalid", [
    {"order": None}, {"order": "inferred"}, {"message_id": " "}, {"message_id": True},
    {"producer": {"display_name": "Same"}}, {"logical_time": True}, {"logical_time": "5"}, {"logical_time": float("inf")},
    {"logical_time": 10 ** 1000}, {"arbitrary": True},
])
def test_invalid_flow_rejects_whole_batch_and_named_models(invalid: dict[str, object]) -> None:
    store = ObserverStore()
    with pytest.raises(ValueError):
        store.publish_many([{"event_type": "runtime.started"}, event(**invalid)])
    assert not store.events() and not inspect(store).records
    with pytest.raises(ValidationError):
        EventInput.model_validate(event(**invalid))


@pytest.mark.parametrize("capacity", [0, -1, True, 1.5])
def test_capacity_is_a_positive_integer(capacity: object) -> None:
    with pytest.raises(ValueError):
        ObserverStore(max_interaction_flow=capacity)


def test_response_rejects_guessed_pairs_merged_unknowns_counts_and_duplicate_evidence() -> None:
    store = ObserverStore()
    store.publish(event())
    wire = inspect(store).model_dump(mode="json")
    for changes in ({"status": "paired"}, {"receive_count": 1}, {"arrival_delay_seconds": 0}, {"last_sequence": 2}):
        with pytest.raises(ValidationError):
            InteractionFlowResponse.model_validate({**wire, "records": [{**wire["records"][0], **changes}]})
    with pytest.raises(ValidationError):
        InteractionFlowResponse.model_validate({**wire, "send_observations": 2})
    with pytest.raises(ValidationError):
        InteractionFlowResponse.model_validate({**wire, "records": wire["records"] * 2})


def test_http_stream_share_flow_models_and_conflicting_direction_is_atomic() -> None:
    store = ObserverStore()
    with TestClient(create_app(store, enable_analytics=False)) as client:
        assert client.post("/api/events", json={"events": [{"event_type": "runtime.started"}, {**event(), "event_type": "interaction.received"}]}).status_code == 422
        assert not store.events()
        assert client.post("/api/events", json={"events": [event(), event("receive")]}).status_code == 200
        payload = client.get("/api/state?include_events=false").json()
        assert payload["inspectors"]["interaction_flow"]["records"][0]["status"] == "paired"
        with client.websocket_connect("/ws/events") as socket:
            assert socket.receive_json()["state"]["inspectors"]["interaction_flow"] == payload["inspectors"]["interaction_flow"]


@pytest.mark.parametrize("envelope", [{"event_type": " interaction.received "}, {"event_type": " ", "kind": "interaction.received"}])
def test_canonical_alias_direction_is_checked_before_core_batch_mutation(envelope: dict[str, str]) -> None:
    store = ObserverStore()
    with pytest.raises(ValueError):
        store.publish_many([{"event_type": "runtime.started"}, {**event(), **envelope}])
    assert not store.events()
