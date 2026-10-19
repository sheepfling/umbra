"""Internal observer ownership evidence; no normative HLA validation claims."""
from __future__ import annotations

import pytest
from fastapi.testclient import TestClient
from pydantic import ValidationError

from umbra_rti_observer.core import ObserverStore
from umbra_rti_observer.schemas import EventInput, OwnershipResponse, StateResponse
from umbra_rti_observer.server import create_app

ACTOR = {"participant_id": "a", "membership_id": "join-a", "display_name": "Same"}


def event(kind: str = "state", **fields: object) -> dict[str, object]:
    observation: dict[str, object] = {"kind": kind, "session_id": "s", "identity_namespace": "ns", "federation_id": "fed",
                                      "object_id": "instance-1", "attribute_key": "value"}
    if kind == "state":
        observation.update(owner_state="unknown", transition="reported")
    else:
        observation.update(request_id="ticket", action="acquire", requester=ACTOR)
    return {"event_type": "ownership.observed", "source": "adapter", "ownership": {**observation, **fields}}


def inspect(store: ObserverStore) -> OwnershipResponse:
    result = StateResponse.model_validate(store.state()).inspectors.ownership
    assert result is not None
    return result


def test_owner_never_follows_updaters_publishers_or_completed_requests() -> None:
    store = ObserverStore(max_events=1)
    store.publish({"event_type": "object.updated", "federate": "Same", "object_key": "instance-1", "attributes": {"value": 1}})
    assert not inspect(store).states
    store.publish(event(owner_state="known", owner=ACTOR))
    store.publish(event("request", action="divest"))
    store.publish(event("outcome", action="divest", outcome="completed"))
    assert inspect(store).requests[0].status == "completed"
    assert inspect(store).states[0].owner.participant_id == "a"
    store.publish(event(owner_state="unowned", transition="divested"))
    assert inspect(store).states[0].owner is None and inspect(store).states[0].owner_state == "unowned"
    assert len(inspect(store).transitions) == 4


@pytest.mark.parametrize("field", ["session_id", "identity_namespace", "federation_id", "object_id", "attribute_key"])
def test_scopes_separate_snapshots_and_request_outcomes(field: str) -> None:
    store = ObserverStore()
    store.publish(event())
    store.publish(event(**{field: "other"}))
    store.publish(event("request"))
    store.publish(event("outcome", **{field: "other"}, outcome="completed"))
    assert len(inspect(store).states) == 2
    assert inspect(store).requests[0].status == "pending" and inspect(store).unmatched_outcomes == 1


def test_reused_names_requester_rejoins_actions_and_sources_do_not_match() -> None:
    store = ObserverStore()
    store.publish(event("request"))
    store.publish(event("outcome", outcome="completed", requester={**ACTOR, "membership_id": "rejoined"}))
    store.publish(event("outcome", action="divest", outcome="completed"))
    store.publish({**event("outcome", outcome="completed"), "source": "other-adapter"})
    assert inspect(store).requests[0].status == "pending" and inspect(store).unmatched_outcomes == 3
    store.publish(event("outcome", outcome="failed", reason="Rejected"))
    assert inspect(store).requests[0].status == "failed"
    store.publish(event("request"))
    row = inspect(store).requests[0]
    assert row.status == "ambiguous" and row.outcome is None and row.duplicate_requests == 1


def test_outcome_before_request_is_not_retroactively_matched_and_state_is_full_replacement() -> None:
    store = ObserverStore()
    store.publish(event("outcome", outcome="completed"))
    store.publish(event("request"))
    assert inspect(store).requests[0].status == "pending" and inspect(store).unmatched_outcomes == 1
    for owner_state in ("known", "rti", "unknown"):
        store.publish(event(owner_state=owner_state, **({"owner": ACTOR, "reason": "Reported"} if owner_state == "known" else {})))
    row = inspect(store).states[0]
    assert row.owner_state == "unknown" and row.owner is None and row.reason is None


def test_handle_only_reports_remain_unresolved_metadata_and_do_not_establish_an_owner() -> None:
    store = ObserverStore()
    store.publish(event(reported_owner_name="Same", reported_owner_handle="0x45"))
    row = inspect(store).states[0]
    assert row.owner_state == "unknown" and row.owner is None
    assert row.reported_owner_name == "Same" and row.reported_owner_handle == "0x45"
    store.publish(event())
    assert inspect(store).states[0].reported_owner_handle is None


def test_independent_capacities_drop_old_evidence_without_completing_it() -> None:
    store = ObserverStore(max_ownership_states=1, max_ownership_requests=1, max_ownership_transitions=2)
    store.publish(event(attribute_key="A"))
    store.publish(event(attribute_key="B"))
    store.publish(event("request", request_id="one"))
    store.publish(event("request", request_id="two"))
    result = inspect(store)
    assert result.dropped_states == result.dropped_requests == 1 and result.dropped_transitions == 2
    assert result.states[0].attribute_key == "B" and result.requests[0].request.request_id == "two"
    assert result.requests[0].status == "pending"
    store.clear()
    result = inspect(store)
    assert not result.states and not result.requests and not result.transitions and not result.dropped_states


@pytest.mark.parametrize("fields", [
    {"owner_state": "known"}, {"owner_state": "unknown", "owner": ACTOR}, {"owner_state": "unowned", "owner": ACTOR},
    {"transition": "guessed"}, {"object_id": " "}, {"attribute_key": True}, {"session_id": None},
    {"arbitrary": True}, {"owner_state": "publisher"}, {"reason": "x" * 4097},
])
def test_invalid_states_reject_entire_core_batch_and_named_models(fields: dict[str, object]) -> None:
    store = ObserverStore()
    with pytest.raises(ValueError):
        store.publish_many([{"event_type": "runtime.started"}, event(**fields)])
    with pytest.raises(ValidationError):
        EventInput.model_validate(event(**fields))
    assert not store.events() and not inspect(store).states


@pytest.mark.parametrize("field", ["max_ownership_states", "max_ownership_requests", "max_ownership_transitions"])
@pytest.mark.parametrize("value", [0, -1, True, 1.5])
def test_capacities_are_positive_integers(field: str, value: object) -> None:
    with pytest.raises(ValueError):
        ObserverStore(**{field: value})


def test_response_checks_ticket_scope_and_outcome_order() -> None:
    store = ObserverStore()
    store.publish(event("request"))
    store.publish(event("outcome", outcome="completed"))
    data = inspect(store).model_dump(mode="json")
    data["requests"][0]["outcome"]["requester"]["membership_id"] = "unrelated"
    with pytest.raises(ValidationError):
        OwnershipResponse.model_validate(data)


@pytest.mark.parametrize("kind,fields", [
    ("request", {"request_id": None}), ("request", {"action": "guess"}),
    ("request", {"requester": {"display_name": "Only a name"}}), ("request", {"outcome": "completed"}),
    ("outcome", {"outcome": "guessed"}), ("outcome", {"owner_state": "known"}),
])
def test_request_and_outcome_payloads_reject_inapplicable_or_guessed_fields(kind: str, fields: dict[str, object]) -> None:
    store = ObserverStore()
    with pytest.raises(ValueError):
        store.publish_many([{"event_type": "runtime.started"}, event(kind, **fields)])
    assert not store.events()
    with pytest.raises(ValidationError):
        EventInput.model_validate(event(kind, **fields))


def test_http_and_stream_share_ownership_evidence_without_mom_or_analytics() -> None:
    store = ObserverStore()
    with TestClient(create_app(store, enable_analytics=False)) as client:
        assert client.post("/api/events", json={"events": [event(owner_state="known", owner=ACTOR), event("request")]}).status_code == 200
        assert client.post("/api/events", json={"events": [{"event_type": "runtime.started"}, event(owner_state="known")]}).status_code == 422
        payload = client.get("/api/state?include_events=false").json()
        with client.websocket_connect("/ws/events") as socket:
            assert socket.receive_json()["state"]["inspectors"]["ownership"] == payload["inspectors"]["ownership"]
