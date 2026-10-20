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
            "expected_version": catalog["targets"][0]["version"],
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


def test_recovery_discovery_is_dynamic_without_churning_unchanged_store_revisions_and_requests_are_strict() -> None:
    commands = CommandDispatcher()
    store = ObserverStore()
    with TestClient(create_app(store, commands=commands)) as client:
        assert client.get("/api/contract").json()["capabilities"]["session_recovery"]["state"] == "unsupported"
        commands.register(SessionController(target_id="owned", label="Owned", adapter="test", federations=("Allowed",),
            driver_factory=lambda: ShowcaseSessionDriver(fault_once="connect"), inspection_supported=True))
        assert client.get("/api/contract").json()["capabilities"]["session_recovery"]["state"] == "available"
        cut = store.state()["observer_lifetime"]
        client.get("/api/contract")
        assert store.state()["observer_lifetime"] == cut
        catalog = client.get("/api/commands/catalog").json()
        version = catalog["targets"][0]["version"]
        payload = {"action": "session.recover", "target_id": "owned", "request_id": "invalid",
            "parameters": {}, "expected_version": version, "confirmed": True}
        for invalid in ({**payload, "parameters": {"credentials": "not accepted"}},
                {**payload, "expected_version": None}, {key: value for key, value in payload.items() if key != "expected_version"},
                {**payload, "expected_version": {**version, "revision": True}}, {**payload, "driver_factory": "not a wire field"}):
            assert client.post("/api/commands", json=invalid).status_code == 422
        for invalid in ({**payload, "expected_version": {**version, "instance_id": " " + version["instance_id"] + " "}},
                {**payload, "target_id": " owned "}, {**payload, "request_id": " padded "}):
            assert client.post("/api/commands", json=invalid).status_code == 422
        assert client.get("/api/commands").json()["records"] == []
        missing_guard = client.post("/api/commands", json={"action": "session.connect", "target_id": "owned",
            "request_id": "missing-version", "parameters": {}}).json()
        assert missing_guard["status"] == "unavailable" and missing_guard["error"]["effect"] == "none"
