"""Internal topology/transport contract evidence, not normative RTI evidence."""
from __future__ import annotations

import json

import pytest
from fastapi.testclient import TestClient
from pydantic import ValidationError

from umbra_rti_observer.core import ObserverStore
from umbra_rti_observer.schemas import EventInput, RelationshipsResponse, StateResponse
from umbra_rti_observer.server import create_app


def event(**overrides: object) -> dict[str, object]:
    return {"event_type": "relationship.observed", "source": "adapter", "relationship": {
        "relationship_id": "edge", "session_id": "session", "identity_namespace": "provider",
        "federation_id": "federation", "kind": "object", "evidence": "declared", "status": "active", "viewpoint": "observer",
        "producer": {"participant_id": "a", "membership_id": "join-a", "display_name": "Same name", "handle": "1"},
        "recipient": {"participant_id": "b", "membership_id": "join-b", "display_name": "Same name", "handle": "1"},
        **overrides,
    }}


def inspect(store: ObserverStore) -> RelationshipsResponse:
    value = StateResponse.model_validate(store.state()).inspectors.relationships
    assert value is not None
    return value


def test_no_inferred_edges_from_declarations_updates_or_receives() -> None:
    store = ObserverStore()
    store.publish_many([
        {"event_type": "declaration.observed", "federate": name, "declaration": {
            "kind": "object", "class_key": "Class", "direction": direction, "status": "declared"}}
        for name, direction in (("A", "publication"), ("B", "subscription"))
    ] + [{"event_type": "object.updated", "object_key": "one", "federate": "A"},
         {"event_type": "interaction.received", "interaction_class": "Class", "federate": "B"}])
    assert not inspect(store).snapshots


def test_names_handles_rejoins_and_multiple_producers_do_not_merge_edges() -> None:
    store = ObserverStore()
    store.publish(event())
    store.publish({**event(), "source": "second-reporter"})
    store.publish(event(session_id="reconnected"))
    store.publish(event(relationship_id="rejoined", producer={"participant_id": "a", "membership_id": "join-again", "display_name": "Same name", "handle": "1"}))
    rows = inspect(store).snapshots
    assert len(rows) == 4
    assert rows[0].producer.participant_id != rows[0].recipient.participant_id
    assert rows[0].producer.display_name == rows[0].recipient.display_name
    assert rows[0].producer.membership_id != rows[-1].producer.membership_id


@pytest.mark.parametrize("field", ["identity_namespace", "federation_id", "session_id", "relationship_id"])
def test_all_explicit_scope_components_separate_records(field: str) -> None:
    store = ObserverStore()
    store.publish(event())
    store.publish(event(**{field: "different"}))
    assert len(inspect(store).snapshots) == 2


def test_complete_replacements_persist_without_guessing_live_membership() -> None:
    store = ObserverStore(max_events=1)
    store.publish(event(class_key="Class", class_name="A class", reason="Reported"))
    store.publish(event(status="unknown", evidence="observed"))
    store.publish({"event_type": "federate.resigned", "federate": "Same name"})
    row = inspect(store).snapshots[0]
    assert row.sequence == 2 and row.class_key is None and row.reason is None
    assert row.status == "unknown" and row.evidence == "observed"
    store.publish(event(status="ended"))
    assert inspect(store).snapshots[0].status == "ended"
    state = store.state()
    state["inspectors"]["relationships"]["snapshots"].clear()
    assert len(inspect(store).snapshots) == 1
    store.clear()
    assert not inspect(store).snapshots


def test_capacity_uses_least_recently_updated_record_and_clear_resets_loss() -> None:
    store = ObserverStore(max_relationships=2)
    store.publish(event(relationship_id="one"))
    store.publish(event(relationship_id="two"))
    store.publish(event(relationship_id="one", status="ended"))
    store.publish(event(relationship_id="three"))
    data = inspect(store)
    assert {row.relationship_id for row in data.snapshots} == {"one", "three"}
    assert data.dropped_relationships == 1
    store.clear()
    assert inspect(store).dropped_relationships == 0


@pytest.mark.parametrize("invalid", [
    {"session_id": None}, {"session_id": " "}, {"session_id": "x" * 257},
    {"relationship_id": True}, {"identity_namespace": ""}, {"federation_id": 1},
    {"producer": {"display_name": "Not an ID"}}, {"recipient": {"participant_id": "b", "membership_id": "j", "inferred": True}},
    {"producer": {"participant_id": "a", "membership_id": False}}, {"status": "guessed"}, {"evidence": "delivered"},
    {"viewpoint": None}, {"kind": "ownership"}, {"reason": "x" * 4097}, {"arbitrary": True},
])
def test_invalid_typed_evidence_rejects_entire_batch_in_core_and_http_models(invalid: dict[str, object]) -> None:
    store = ObserverStore()
    with pytest.raises(ValueError):
        store.publish_many([{"event_type": "runtime.started"}, event(**invalid)])
    assert not store.events() and not inspect(store).snapshots
    with pytest.raises(ValidationError):
        EventInput.model_validate(event(**invalid))


def test_opaque_ids_are_not_trimmed_and_response_invariants_are_validated() -> None:
    store = ObserverStore()
    store.publish(event(relationship_id=" opaque ", producer={"participant_id": " a ", "membership_id": " j "}))
    value = inspect(store)
    assert value.snapshots[0].relationship_id == " opaque "
    assert value.snapshots[0].producer.participant_id == " a "
    wire = value.model_dump(mode="json")
    with pytest.raises(ValidationError):
        RelationshipsResponse.model_validate({**wire, "snapshots": wire["snapshots"] * 2})
    with pytest.raises(ValidationError):
        RelationshipsResponse.model_validate({**wire, "capacity": 0})


@pytest.mark.parametrize("capacity", [0, -1, True, 1.5])
def test_capacity_is_a_positive_integer(capacity: object) -> None:
    with pytest.raises(ValueError):
        ObserverStore(max_relationships=capacity)


def test_http_snapshot_stream_and_atomic_rejection_share_named_models() -> None:
    store = ObserverStore()
    with TestClient(create_app(store)) as client:
        rejected = client.post("/api/events", json={"events": [{"event_type": "runtime.started"}, event(session_id=None)]})
        assert rejected.status_code == 422 and not store.events()
        assert client.post("/api/events", json=event()).status_code == 200
        response = client.get("/api/state?include_events=true")
        value = StateResponse.model_validate(response.json())
        assert value.inspectors.relationships is not None
        assert value.inspectors.relationships.snapshots[0].producer.participant_id == "a"
        with client.websocket_connect("/ws/events") as socket:
            snapshot = socket.receive_json()
            assert snapshot["state"]["inspectors"]["relationships"] == response.json()["inspectors"]["relationships"]
        assert "ParticipantIdentityResponse" in json.dumps(client.get("/openapi.json").json())
