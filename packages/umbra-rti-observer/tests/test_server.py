from __future__ import annotations

import pytest

pytest.importorskip("fastapi")
pytest.importorskip("httpx")

from fastapi.testclient import TestClient

from umbra_rti_observer import OBSERVER_CONTRACT_VERSION, ObserverStore
from umbra_rti_observer.server import create_app


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
    response_schemas = openapi["components"]["schemas"]
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
