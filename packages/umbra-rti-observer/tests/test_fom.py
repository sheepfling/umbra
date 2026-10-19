"""Internal supplied-FOM metadata behavior; no normative FOM/RTI validation."""
from __future__ import annotations

import pytest
from fastapi.testclient import TestClient
from pydantic import ValidationError

from umbra_rti_observer.core import ObserverStore
from umbra_rti_observer.schemas import EventInput, FomResponse, StateResponse
from umbra_rti_observer.server import create_app


def event(kind: str = "object-class", **fields: object) -> dict[str, object]:
    data: dict[str, object] = {"kind": kind, "session_id": "s", "identity_namespace": "ns", "federation_id": "fed", "model_id": "model", "revision": "1", "element_id": "E", "name": "Name", "status": "present"}
    if kind in ("object-class", "interaction-class"): data.update(parent_state="unknown", field_scope="declared")
    elif kind == "datatype": data.update(encoding="unknown")
    return {"event_type": "fom.observed", "source": "adapter", "fom_observation": {**data, **fields}}


def inspect(store: ObserverStore) -> FomResponse:
    result = StateResponse.model_validate(store.state()).inspectors.fom
    assert result is not None
    return result


def test_only_explicit_metadata_is_indexed_without_parsing_names_or_runtime_values() -> None:
    store = ObserverStore(max_events=1)
    store.publish({"event_type": "object.updated", "class_name": "Name", "object_key": "one", "attributes": {"value": "7"}})
    assert not inspect(store).snapshots
    store.publish(event(attributes=[{"field_id": "value", "datatype_id": "T"}]))
    row = inspect(store).snapshots[0]
    assert row.kind == "object-class" and row.attributes[0].name is None
    store.publish({"event_type": "callback.invoked"})
    assert len(inspect(store).snapshots) == 1


@pytest.mark.parametrize("field", ["session_id", "identity_namespace", "federation_id", "model_id", "revision", "element_id"])
def test_model_revisions_and_reporter_scopes_do_not_merge(field: str) -> None:
    store = ObserverStore()
    store.publish(event())
    store.publish(event(**{field: "different"}))
    store.publish({**event(), "source": "other-reporter"})
    assert len(inspect(store).snapshots) == 3


def test_complete_replacement_distinguishes_empty_unknown_fields_and_removed_metadata() -> None:
    store = ObserverStore()
    store.publish(event(parent_state="parent", parent_id="P", attributes=[{"field_id": "a", "name": "A"}]))
    store.publish(event(parent_state="root", attributes=[]))
    row = inspect(store).snapshots[0]
    assert row.kind == "object-class" and row.parent_id is None and row.attributes is not None and not row.attributes
    store.publish(event(status="removed", name=None))
    row = inspect(store).snapshots[0]
    assert row.name is None and row.status == "removed" and row.attributes is None


def test_encoding_metadata_and_exact_large_dimension_bounds_are_preserved() -> None:
    store = ObserverStore()
    store.publish(event("datatype", encoding="enumeration", enumerators=[{"name": "Off", "value": "0"}, {"name": "Alias", "value": "0"}]))
    store.publish(event("datatype", element_id="Array", encoding="array", cardinality="0", element_type_id="T"))
    store.publish(event("dimension", upper_bound="18446744073709551615"))
    rows = inspect(store).snapshots
    assert rows[0].enumerators[0].value == "0" and rows[1].cardinality == "0" and rows[2].upper_bound == "18446744073709551615"


@pytest.mark.parametrize("fields", [{"parent_state": "root", "parent_id": "P"}, {"parent_state": "parent"}, {"field_scope": None}, {"attributes": [{"field_id": "a"}] * 2}, {"parameters": []}, {"revision": True}, {"arbitrary": True}])
def test_invalid_classes_reject_atomic_batches_and_named_inputs(fields: dict[str, object]) -> None:
    store = ObserverStore()
    with pytest.raises(ValueError): store.publish_many([{"event_type": "runtime.started"}, event(**fields)])
    with pytest.raises(ValidationError): EventInput.model_validate(event(**fields))
    assert not store.events()


@pytest.mark.parametrize("fields", [{"encoding": "primitive", "fields": []}, {"encoding": "array", "enumerators": []}, {"encoding": "record", "cardinality": "4"}, {"encoding": "array", "cardinality": 4}, {"encoding": "array", "cardinality": "01"}, {"encoding": "enumeration", "enumerators": [{"name": "N", "value": "0"}] * 2}])
def test_inapplicable_or_coerced_datatype_fields_are_rejected(fields: dict[str, object]) -> None:
    with pytest.raises(ValueError): ObserverStore().publish(event("datatype", **fields))
    with pytest.raises(ValidationError): EventInput.model_validate(event("datatype", **fields))


def test_bounded_metadata_and_history_have_independent_loss_and_clear() -> None:
    store = ObserverStore(max_fom_elements=2, max_fom_transitions=3)
    for identity in ("one", "two", "one", "three"): store.publish(event(element_id=identity))
    data = inspect(store)
    assert {row.element_id for row in data.snapshots} == {"one", "three"} and data.dropped_snapshots == data.dropped_transitions == 1
    store.clear()
    assert not inspect(store).snapshots and not inspect(store).transitions


@pytest.mark.parametrize("field", ["max_fom_elements", "max_fom_transitions"])
@pytest.mark.parametrize("value", [0, -1, True, 1.5])
def test_positive_integer_capacities(field: str, value: object) -> None:
    with pytest.raises(ValueError): ObserverStore(**{field: value})


def test_http_stream_share_supplied_metadata_without_mom_or_analytics() -> None:
    store = ObserverStore()
    with TestClient(create_app(store, enable_analytics=False)) as client:
        assert client.post("/api/events", json={"events": [event(), event("datatype"), event("dimension", upper_bound="18446744073709551615")]}).status_code == 200
        payload = client.get("/api/state?include_events=false").json()
        assert not payload["federate_roster"]
        with client.websocket_connect("/ws/events") as socket:
            assert socket.receive_json()["state"]["inspectors"]["fom"] == payload["inspectors"]["fom"]
