from __future__ import annotations

import threading
import time

import pytest

pytest.importorskip("httpx")
pytest.importorskip("fastapi")

from fastapi.testclient import TestClient
from umbra_rti_observer import ObserverStore
from umbra_rti_observer.commands import CommandDispatcher
from umbra_rti_observer.server import create_app
from umbra_rti_observer.session_commands import SessionController
from umbra_rti_observer.showcase_commands import ShowcaseSessionDriver


def test_read_only_backend_does_not_infer_control_from_observation_capabilities() -> (
    None
):
    store = ObserverStore()
    store.set_capabilities({"mom": "available", "logical_time": "available"})
    with TestClient(create_app(store)) as client:
        assert client.get("/api/commands/catalog").json()["enabled"] is False
        assert client.get("/api/commands").json()["records"] == []
        assert "commands" not in client.get("/api/contract").json()["api_paths"]
        response = client.post(
            "/api/commands",
            json={
                "action": "session.connect",
                "target_id": "test",
                "request_id": "x",
                "parameters": {},
            },
        )
        assert response.status_code == 503


def test_command_api_validation_idempotency_and_pollable_history() -> None:
    commands = CommandDispatcher()
    commands.register(
        SessionController(
            target_id="test",
            label="Test",
            adapter="fake",
            federations=("Allowed",),
            driver_factory=ShowcaseSessionDriver,
            synthetic=True,
        )
    )
    with TestClient(create_app(commands=commands)) as client:
        catalog = client.get("/api/commands/catalog").json()
        assert catalog["targets"][0]["synthetic"]
        assert (
            client.get("/api/contract").json()["api_paths"]["commands"]
            == "/api/commands"
        )
        payload = {
            "action": "session.connect",
            "target_id": "test",
            "request_id": "connect",
            "parameters": {},
        }
        accepted = client.post("/api/commands", json=payload)
        assert accepted.status_code == 202
        command_id = accepted.json()["command_id"]
        assert (
            client.post("/api/commands", json=payload).json()["command_id"]
            == command_id
        )
        deadline = time.monotonic() + 2
        while time.monotonic() < deadline:
            result = client.get(f"/api/commands/{command_id}").json()
            if result["status"] == "succeeded":
                break
            threading.Event().wait(0.005)
        assert result["status"] == "succeeded"
        assert (
            client.get("/api/commands").json()["records"][0]["command_id"] == command_id
        )
        assert (
            client.post(
                "/api/commands", json={**payload, "confirmed": True}
            ).status_code
            == 409
        )
        assert (
            client.post(
                "/api/commands",
                json={**payload, "parameters": {"credentials": "forbidden"}},
            ).status_code
            == 422
        )
        assert (
            client.post(
                "/api/commands", json={**payload, "action": "arbitrary.provider.call"}
            ).status_code
            == 422
        )
        assert (
            client.post(
                "/api/commands",
                json={**payload, "request_id": "bad", "confirmed": "true"},
            ).status_code
            == 422
        )
        assert client.get("/api/commands/missing").status_code == 404
        schema = client.get("/openapi.json").json()
        assert {"JoinParameters", "ResignParameters", "CommandRecord"} <= schema[
            "components"
        ]["schemas"].keys()
