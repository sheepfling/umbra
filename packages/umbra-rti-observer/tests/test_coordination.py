"""Internal observer coordination models, not normative HLA service evidence."""
from __future__ import annotations

import pytest
from fastapi.testclient import TestClient
from pydantic import ValidationError

from umbra_rti_observer.core import ObserverStore
from umbra_rti_observer.schemas import CoordinationResponse, EventInput, StateResponse
from umbra_rti_observer.server import create_app


def event(**overrides: object) -> dict[str, object]:
    return {"event_type": "coordination.observed", "source": "adapter", "coordination": {
        "kind": "save", "operation_id": "checkpoint", "session_id": "session", "identity_namespace": "provider",
        "federation_id": "fed", "label": "Same label", "phase": "requested", "scope": "federation", **overrides,
    }}


def inspect(store: ObserverStore) -> CoordinationResponse:
    result = StateResponse.model_validate(store.state()).inspectors.coordination
    assert result is not None
    return result


def test_local_completion_never_completes_federation_and_phase_order_is_not_inferred() -> None:
    store = ObserverStore(max_events=1)
    store.publish(event())
    store.publish(event(scope="local", subject={"participant_id": "a", "membership_id": "a1"}, phase="completed"))
    store.publish({"event_type": "federation.saved", "label": "Same label"})
    rows = inspect(store).snapshots
    assert len(rows) == 2 and rows[0].phase == "requested" and rows[1].phase == "completed"
    assert len(inspect(store).transitions) == 2
    store.publish(event(phase="failed", reason="A different participant failed"))
    rows = inspect(store).snapshots
    assert rows[0].scope == "local" and rows[0].phase == "completed"
    assert rows[1].scope == "federation" and rows[1].phase == "failed"
    store.publish(event(phase="begun"))
    assert inspect(store).snapshots[-1].phase == "begun"  # Latest report, not invented state-machine success.


@pytest.mark.parametrize("field", ["operation_id", "session_id", "identity_namespace", "federation_id", "kind", "scope"])
def test_explicit_scope_prevents_label_reuse_or_reporter_merges(field: str) -> None:
    store = ObserverStore()
    store.publish(event())
    value = "restore" if field == "kind" else "local" if field == "scope" else "different"
    store.publish(event(**{field: value}))
    store.publish({**event(), "source": "other-reporter"})
    assert len(inspect(store).snapshots) == 3


def test_subject_memberships_and_progress_are_explicit_complete_replacements() -> None:
    store = ObserverStore()
    a = {"participant_id": "a", "membership_id": "join-a", "display_name": "Same"}
    b = {"participant_id": "b", "membership_id": "join-b", "display_name": "Same"}
    store.publish(event(kind="synchronization", phase="registration-failed", participants=[{"identity": a, "status": "failed", "reason": "Rejected"}, {"identity": b, "status": "unknown"}]))
    assert inspect(store).snapshots[0].participants[0].reason == "Rejected"
    store.publish(event(kind="synchronization", phase="announced", participants=[]))
    assert inspect(store).snapshots[0].participants is not None and not inspect(store).snapshots[0].participants
    store.publish(event(kind="synchronization", phase="achieved", scope="local", subject=a))
    store.publish(event(kind="synchronization", phase="achieved", scope="local", subject={**a, "membership_id": "rejoin"}))
    assert len(inspect(store).snapshots) == 3
    store.publish(event(kind="synchronization", phase="completed"))
    assert inspect(store).snapshots[-1].participants is None


def test_index_and_transition_history_have_independent_capacity_and_clear() -> None:
    store = ObserverStore(max_coordination=2, max_coordination_transitions=3)
    for operation in ("one", "two", "one", "three"):
        store.publish(event(operation_id=operation))
    data = inspect(store)
    assert {row.operation_id for row in data.snapshots} == {"one", "three"}
    assert [row.sequence for row in data.transitions] == [2, 3, 4]
    assert data.dropped_operations == 1 and data.dropped_transitions == 1
    store.clear()
    data = inspect(store)
    assert not data.snapshots and not data.transitions and not data.dropped_operations and not data.dropped_transitions


@pytest.mark.parametrize("invalid", [
    {"operation_id": " "}, {"session_id": None}, {"label": 1}, {"kind": "unknown"}, {"scope": None},
    {"phase": "announced"}, {"kind": "synchronization", "phase": "begun"}, {"reason": True},
    {"subject": {"participant_id": "a", "membership_id": "j"}}, {"extra": True},
    {"participants": [{"identity": {"participant_id": "a", "membership_id": "j"}, "status": "guessed"}]},
    {"participants": [{"identity": {"participant_id": "a", "membership_id": "j"}, "status": "failed"}] * 2},
])
def test_invalid_observations_reject_atomic_batches_and_named_inputs(invalid: dict[str, object]) -> None:
    store = ObserverStore()
    with pytest.raises(ValueError):
        store.publish_many([{"event_type": "runtime.started"}, event(**invalid)])
    assert not store.events() and not inspect(store).snapshots
    with pytest.raises(ValidationError):
        EventInput.model_validate(event(**invalid))


@pytest.mark.parametrize("field", ["max_coordination", "max_coordination_transitions"])
@pytest.mark.parametrize("value", [0, -1, True, 1.5])
def test_capacities_are_positive_integers(field: str, value: object) -> None:
    with pytest.raises(ValueError):
        ObserverStore(**{field: value})


def test_response_checks_history_uniqueness_capacity_and_latest_consistency() -> None:
    store = ObserverStore()
    store.publish(event())
    wire = inspect(store).model_dump(mode="json")
    for invalid in ({"transitions": wire["transitions"] * 2}, {"snapshots": wire["snapshots"] * 2}, {"history_capacity": 0}):
        with pytest.raises(ValidationError):
            CoordinationResponse.model_validate({**wire, **invalid})
    wire["snapshots"][0]["phase"] = "failed"
    with pytest.raises(ValidationError):
        CoordinationResponse.model_validate(wire)


def test_http_and_stream_preserve_named_phase_models_without_analytics() -> None:
    store = ObserverStore()
    with TestClient(create_app(store, enable_analytics=False)) as client:
        assert client.post("/api/events", json=event(kind="synchronization", phase="registered")).status_code == 200
        assert client.post("/api/events", json=event(phase="announced")).status_code == 422
        response = client.get("/api/state?include_events=true").json()
        parsed = StateResponse.model_validate(response)
        assert parsed.inspectors.coordination is not None
        assert parsed.inspectors.coordination.snapshots[0].phase == "registered"
        with client.websocket_connect("/ws/events") as socket:
            assert socket.receive_json()["state"]["inspectors"]["coordination"] == response["inspectors"]["coordination"]
