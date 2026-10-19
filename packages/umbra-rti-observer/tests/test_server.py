from __future__ import annotations

import pytest

pytest.importorskip("fastapi")
pytest.importorskip("httpx")

from fastapi.testclient import TestClient

from umbra_rti_observer import OBSERVER_CONTRACT_VERSION, ObserverStore
from umbra_rti_observer.server import create_app


def test_object_perspectives_have_identical_required_nulls_in_http_and_stream() -> None:
    from umbra_rti_observer.schemas import SnapshotMessage, StateResponse

    store = ObserverStore()
    store.publish({"event_type": "object.updated", "source": "test", "object_key": "shared",
                   "attributes": {"nullable": None}})
    with TestClient(create_app(store)) as client:
        http = client.get("/api/state?include_events=false").json()
        assert "normalized_events" not in http
        validated = StateResponse.model_validate(http).inspectors.object_observations
        assert validated is not None
        assert validated.snapshots[0].federate is None
        assert validated.changes[0].value is None
        assert validated.changes[0].previous_value is None
        with client.websocket_connect("/ws/events") as stream:
            payload = stream.receive_json()
            message = SnapshotMessage.model_validate(payload)
            assert message.state.inspectors.object_observations == validated
            assert payload["state"]["inspectors"]["object_observations"] == http["inspectors"]["object_observations"]


def test_attribute_json_is_validated_without_trimming_or_coercing_supplied_data() -> None:
    attributes = {" padded key ": "  preserve  ", "padded key": "distinct",
                  "nested": {" key ": [" text ", True, None, "123"]}}
    with TestClient(create_app()) as client:
        response = client.post("/api/events", json={"event_type": "object.updated", "source": "test",
                                                    "object_key": "one", "attributes": attributes})
        assert response.status_code == 200
        state = client.get("/api/state").json()
        observations = state["inspectors"]["object_observations"]
        assert observations["snapshots"][0]["object"]["attributes"] == attributes
        assert state["inspectors"]["objects"][0]["attributes"] == attributes
        assert state["normalized_events"][0]["attributes"] == attributes
        changes = {row["attribute"]: row["value"] for row in observations["changes"]}
        assert changes == attributes
        with client.websocket_connect("/ws/events") as stream:
            assert stream.receive_json()["state"]["inspectors"]["object_observations"] == observations


def test_typed_time_inputs_are_atomic_and_persistent_time_does_not_require_analytics() -> None:
    from umbra_rti_observer.schemas import StateResponse

    with TestClient(create_app(enable_analytics=False)) as client:
        for observation in ({"kind": "grant", "logical_time": True}, {"kind": "state", "regulating": "true"},
                            {"kind": "request", "request_id": " "}, {"kind": "state", "vendor_guess": 3}):
            response = client.post("/api/events", json={"events": [{"event_type": "runtime.started"},
                                                                      {"time_observation": observation}]})
            assert response.status_code == 422
            assert client.get("/api/state").json()["retention"]["retained_events"] == 0
        for kind in ("request", "grant"):
            response = client.post("/api/events", json={"event_type": "time.observed", "source": "test", "federate": "Alice",
                "time_observation": {"kind": kind, "session_id": "session", "request_id": " opaque id "}})
            assert response.status_code == 200
        payload = client.get("/api/state?include_events=false").json()
        time = StateResponse.model_validate(payload).inspectors.time
        assert time is not None and time.requests[0].status == "granted"
        assert time.requests[0].request_id == " opaque id " and time.requests[0].granted_time is None
        assert time.snapshots[0].scope.federation is None
        assert client.get("/api/analytics").status_code == 404
        with client.websocket_connect("/ws/events") as stream:
            streamed = StateResponse.model_validate(stream.receive_json()["state"]).inspectors.time
            assert streamed == time


def test_analytics_is_advertised_validated_and_scopable() -> None:
    store = ObserverStore()
    store.publish({"event_type": "callback.invoked", "federate": "A", "logical_time": 0})
    client = TestClient(create_app(store))
    assert client.get("/api/contract").json()["api_paths"]["analytics"] == "/api/analytics"
    response = client.get("/api/analytics", params={"federate": "A", "window_seconds": 30})
    assert response.status_code == 200
    assert response.json()["events_in_window"] == 1
    assert response.json()["logical_times"][0]["logical_time"] == 0
    assert client.get("/api/analytics?window_seconds=0").status_code == 422
    assert client.get("/api/analytics?bucket_count=121").status_code == 422
    assert client.get("/api/analytics?federate=absent").json()["participants"] == []
    schema = client.get("/openapi.json").json()
    assert "AnalyticsResponse" in schema["components"]["schemas"]


def test_http_surface_exposes_cursor_pages_and_derived_views() -> None:
    store = ObserverStore(max_events=3)
    client = TestClient(create_app(store))

    contract = client.get("/api/contract")
    assert contract.status_code == 200
    assert contract.json()["contract_version"] == OBSERVER_CONTRACT_VERSION
    assert contract.headers["x-umbra-observer-contract"] == OBSERVER_CONTRACT_VERSION
    assert contract.json()["capabilities"]["http_api"]["state"] == "available"

    response = client.post(
        "/api/events",
        json={
            "events": [
                {"event_type": "runtime.started", "source": "test"},
                {"event_type": "object.discovered", "object_key": "object-1"},
            ]
        },
    )
    assert response.status_code == 200
    assert response.json()["accepted"] == 2
    assert response.json()["state"]["contract_version"] == OBSERVER_CONTRACT_VERSION

    page = client.get("/api/events", params={"after": 0, "source": "test"})
    assert page.status_code == 200
    assert page.json()["contract_version"] == OBSERVER_CONTRACT_VERSION
    assert [row["sequence"] for row in page.json()["events"]] == [1]
    assert page.json()["next_after"] == 1

    state = client.get("/api/state", params={"include_events": "false"})
    assert state.status_code == 200
    assert "normalized_events" not in state.json()
    assert state.json()["inspectors"]["objects"][0]["object_key"] == "object-1"
    assert state.json()["capabilities"]["event_history"]["state"] == "available"

    metrics = client.get("/api/metrics")
    assert metrics.status_code == 200
    assert metrics.json()["contract_version"] == OBSERVER_CONTRACT_VERSION
    assert metrics.json()["live_metrics"]["event_count"] == 2


def test_disabling_analytics_keeps_existing_resources_and_hides_optional_route() -> None:
    store = ObserverStore()
    store.publish({"event_type": "federate.joined", "federate": "A"})
    client = TestClient(create_app(store, enable_analytics=False))
    contract = client.get("/api/contract").json()
    assert contract["capabilities"]["analytics"]["state"] == "disabled"
    assert "analytics" not in contract["api_paths"]
    assert client.get("/api/analytics").status_code == 404
    assert "/api/analytics" not in client.get("/openapi.json").json()["paths"]
    assert client.get("/api/state").json()["federate_roster"][0]["federate"] == "A"
    assert client.get("/api/events").json()["events"][0]["event_type"] == "federate.joined"


def test_http_ingest_rejects_invalid_batch_without_partial_write() -> None:
    store = ObserverStore()
    client = TestClient(create_app(store))

    response = client.post(
        "/api/events",
        json={"events": [{"event_type": "runtime.started"}, "invalid"]},
    )
    assert response.status_code == 422
    assert store.events() == []


def test_http_contract_uses_named_pydantic_models_at_the_transport_boundary() -> None:
    client = TestClient(create_app())

    openapi = client.get("/openapi.json")
    assert openapi.status_code == 200
    request_schema = (
        openapi.json()["paths"]["/api/events"]["post"]["requestBody"]
        ["content"]["application/json"]["schema"]
    )
    assert request_schema["anyOf"]
    assert {"EventBatchRequest", "EventInput"}.issubset(
        {item["$ref"].rsplit("/", 1)[-1] for item in request_schema["anyOf"]}
    )
    response_schemas = openapi.json()["components"]["schemas"]
    assert {
        "sequence",
        "event_type",
        "family",
        "source",
        "observed_at",
    }.issubset(response_schemas["EventEnvelope"]["properties"])
    assert {
        "state",
        "label",
        "description",
        "owner",
    }.issubset(response_schemas["CapabilityResponse"]["properties"])

    invalid_context = client.put("/api/context", json=["not", "an", "object"])
    assert invalid_context.status_code == 422

    invalid_status = client.post("/api/control/status", json={})
    assert invalid_status.status_code == 422
