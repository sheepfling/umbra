"""Internal management-report observer evidence, not IEEE MOM conformance."""
from __future__ import annotations

import pytest
from fastapi.testclient import TestClient
from pydantic import ValidationError

from umbra_rti_observer.core import ObserverStore
from umbra_rti_observer.schemas import EventInput, MomResponse, StateResponse
from umbra_rti_observer.server import create_app


def event(kind: str = "members", **fields: object) -> dict[str, object]:
    data: dict[str, object] = {"kind": kind, "session_id": "s", "identity_namespace": "ns", "federation_id": "fed", "report_id": "report"}
    if kind == "members": data.update(coverage="partial", members=[{"participant": {"display_name": "Unresolved", "handle": "0x45"}, "status": "unknown"}])
    elif kind == "service": data.update(service_name="ExampleService", status="failed", error_name="ReportedError")
    else: data.update(counter_key="counter", semantics="cumulative", value=0)
    return {"event_type": "mom.observed", "source": "adapter", "mom_observation": {**data, **fields}}


def inspect(store: ObserverStore) -> MomResponse:
    result = StateResponse.model_validate(store.state()).inspectors.mom
    assert result is not None
    return result


def test_management_roster_never_replaces_ordinary_membership_or_guesses_joins() -> None:
    store = ObserverStore(max_events=1)
    store.publish(event())
    assert not store.state()["federate_roster"]
    row = inspect(store).snapshots[0]
    assert row.kind == "members" and row.coverage == "partial" and row.members[0].participant.identity is None
    store.publish({"event_type": "callback.invoked", "operation": "reportFederationExecutionMembers"})
    assert len(inspect(store).snapshots) == 1
    store.publish(event(coverage="complete", members=[]))
    assert inspect(store).snapshots[0].members is not None and not inspect(store).snapshots[0].members
    store.publish(event(coverage="unknown", members=None))
    assert inspect(store).snapshots[0].members is None


def test_identified_rejoins_stay_separate_and_unresolved_names_are_not_keys() -> None:
    store = ObserverStore()
    members = [{"participant": {"identity": {"participant_id": "a", "membership_id": join, "display_name": "Same"}}, "status": "joined"} for join in ("j1", "j2")]
    store.publish(event(members=members))
    assert len(inspect(store).snapshots[0].members) == 2
    with pytest.raises(ValueError): store.publish(event(members=members * 2))


@pytest.mark.parametrize("field", ["session_id", "identity_namespace", "federation_id", "report_id"])
def test_management_scopes_do_not_merge(field: str) -> None:
    store = ObserverStore()
    store.publish(event())
    store.publish(event(**{field: "other"}))
    store.publish({**event(), "source": "other-reporter"})
    assert len(inspect(store).snapshots) == 3


def test_services_and_counter_representations_replace_without_inferred_rates_or_commands() -> None:
    store = ObserverStore()
    store.publish(event("service"))
    store.publish(event("service", status="completed", error_name=None))
    row = inspect(store).snapshots[0]
    assert row.kind == "service" and row.status == "completed" and row.error_name is None
    store.publish(event("counter"))
    assert inspect(store).snapshots[-1].value == 0
    store.publish(event("counter", value=None, value_text="18446744073709551615"))
    row = inspect(store).snapshots[-1]
    assert row.kind == "counter" and row.value is None and row.value_text == "18446744073709551615"
    store.publish(event("counter", value=2, value_text=None))
    assert inspect(store).snapshots[-1].value == 2


@pytest.mark.parametrize("fields", [{"coverage": "complete", "members": None}, {"coverage": None}, {"arbitrary": True}, {"members": [{"participant": {}, "status": "unknown"}]}, {"members": [{"participant": {"handle": True}, "status": "joined"}]}])
def test_invalid_members_reject_atomic_core_batches_and_models(fields: dict[str, object]) -> None:
    store = ObserverStore()
    with pytest.raises(ValueError): store.publish_many([{"event_type": "runtime.started"}, event(**fields)])
    with pytest.raises(ValidationError): EventInput.model_validate(event(**fields))
    assert not store.events()


@pytest.mark.parametrize("value", [True, "5", float("inf"), 10 ** 1000, 9_007_199_254_740_992])
def test_invalid_or_unsafe_numeric_counters_are_rejected_without_coercion(value: object) -> None:
    with pytest.raises(ValueError): ObserverStore().publish(event("counter", value=value))
    with pytest.raises(ValidationError): EventInput.model_validate(event("counter", value=value))


@pytest.mark.parametrize("fields", [{"value": None}, {"value_text": "5"}, {"semantics": "inferred"}, {"subject": {}}, {"unit": True}])
def test_counter_requires_exactly_one_representation_and_typed_subject(fields: dict[str, object]) -> None:
    with pytest.raises(ValueError): ObserverStore().publish(event("counter", **fields))


def test_independent_latest_and_history_capacity_and_clear() -> None:
    store = ObserverStore(max_mom_reports=2, max_mom_transitions=3)
    for identity in ("one", "two", "one", "three"): store.publish(event(report_id=identity))
    data = inspect(store)
    assert {row.report_id for row in data.snapshots} == {"one", "three"}
    assert data.dropped_snapshots == data.dropped_transitions == 1
    store.clear()
    assert not inspect(store).snapshots and not inspect(store).transitions


@pytest.mark.parametrize("field", ["max_mom_reports", "max_mom_transitions"])
@pytest.mark.parametrize("value", [0, -1, True, 1.5])
def test_positive_integer_capacities(field: str, value: object) -> None:
    with pytest.raises(ValueError): ObserverStore(**{field: value})


def test_http_stream_preserve_typed_reports_and_ordinary_roster_independence() -> None:
    store = ObserverStore()
    with TestClient(create_app(store, enable_analytics=False)) as client:
        assert client.post("/api/events", json={"events": [event(), event("service"), event("counter")]}).status_code == 200
        payload = client.get("/api/state?include_events=false").json()
        assert not payload["federate_roster"]
        with client.websocket_connect("/ws/events") as socket:
            assert socket.receive_json()["state"]["inspectors"]["mom"] == payload["inspectors"]["mom"]
