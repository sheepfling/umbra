"""Internal observer wire evidence; no IEEE RTI normative mappings apply."""

from __future__ import annotations

import json
import queue
import shutil
import subprocess
import tempfile
import threading
from collections.abc import Iterator
from pathlib import Path

import pytest
from fastapi.testclient import TestClient

from umbra_rti_observer.conformance import HttpReply, ObserverConformance, local_path
from umbra_rti_observer.conformance_profiles import (
    FIXTURE_ROOT,
    PROFILES,
    capture,
    fixture_fetch,
    scenario,
)
from umbra_rti_observer.conformance_schema import SCHEMA_PATH, wire_schema_text


@pytest.mark.parametrize("profile", PROFILES)
def test_shared_fixture_matrix(profile: str) -> None:
    runner = ObserverConformance("http://fixture", fetch=fixture_fetch(profile))
    if profile == "reconnect":
        assert runner.run().passed is False
    report = runner.run()
    assert report.passed, report.model_dump_json(indent=2)
    assert "commands" in report.omitted_features
    assert ("analytics" in report.omitted_features) is (profile == "no-analytics")


@pytest.mark.parametrize("profile", PROFILES)
def test_live_python_uses_the_same_contract_checks(profile: str) -> None:
    with scenario(profile) as app, TestClient(app) as client:
        def fetch(path: str) -> HttpReply:
            response = client.get(path)
            return HttpReply(status=response.status_code, body=response.text, headers=dict(response.headers))
        runner = ObserverConformance("http://fixture", fetch=fetch)
        if profile == "reconnect":
            assert not runner.run().passed
        report = runner.run()
        assert report.passed, report.model_dump_json(indent=2)


def test_portable_schema_is_current() -> None:
    assert SCHEMA_PATH.read_text(encoding="utf-8") == wire_schema_text()


def test_portable_schema_is_valid_and_checks_wire_collections() -> None:
    from jsonschema import Draft202012Validator
    from jsonschema.exceptions import ValidationError
    schema = json.loads(SCHEMA_PATH.read_text(encoding="utf-8"))
    Draft202012Validator.check_schema(schema)
    validator = Draft202012Validator({**schema, "$ref": "#/$defs/StateResponse"})
    state = json.loads(fixture_fetch("limited")("/api/state?include_events=true").body)
    validator.validate(state)
    state.pop("federate_roster")
    with pytest.raises(ValidationError):
        validator.validate(state)


def test_schema_drift_check_fails_without_overwriting(monkeypatch: pytest.MonkeyPatch) -> None:
    from umbra_rti_observer import conformance_schema
    with tempfile.TemporaryDirectory(prefix="observer schema drift ") as temporary:
        stale = Path(temporary) / "schema.json"
        stale.write_text("{}", encoding="utf-8")
        monkeypatch.setattr(conformance_schema, "SCHEMA_PATH", stale)
        assert conformance_schema.main([]) == 1
        assert stale.read_text(encoding="utf-8") == "{}"


def test_offline_transport_is_a_failed_report_not_an_exception() -> None:
    def offline(_path: str) -> HttpReply:
        raise ConnectionRefusedError("Synthetic offline backend")
    report = ObserverConformance("http://fixture", fetch=offline).run()
    assert not report.passed
    assert "Synthetic offline backend" in report.checks[0].detail


def test_existing_probe_exposes_the_strict_checker(monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]) -> None:
    from umbra_rti_observer.probe import main
    replay = fixture_fetch("limited")
    monkeypatch.setattr(ObserverConformance, "_fetch_http", lambda self, path: replay(path))
    assert main(["http://fixture", "--strict"]) == 0
    report = json.loads(capsys.readouterr().out)
    assert report["scope"] == "observer-http-read-v1"
    assert report["passed"] is True


def test_registered_commands_are_validated_but_never_executed() -> None:
    from umbra_rti_observer.commands import CommandDispatcher
    from umbra_rti_observer.core import ObserverStore
    from umbra_rti_observer.server import create_app
    from umbra_rti_observer.session_commands import SessionController
    store = ObserverStore()
    commands = CommandDispatcher(sink=store.publish)
    commands.register(SessionController(target_id="test-session", label="Owned synthetic session", adapter="test",
        driver_factory=lambda: pytest.fail("Read-only conformance must never create an RTI driver"),
        federations=("FixtureFederation",), sink=store.publish, synthetic=True))
    try:
        with TestClient(create_app(store, commands=commands)) as client:
            def fetch(path: str) -> HttpReply:
                response = client.get(path)
                return HttpReply(status=response.status_code, headers=dict(response.headers), body=response.text)
            report = ObserverConformance("http://fixture", fetch=fetch).run()
            assert report.passed, report.model_dump_json(indent=2)
            assert "commands" not in report.omitted_features
            assert not commands.history().records
    finally:
        commands.close()
        store.close()


@pytest.mark.parametrize("profile", PROFILES)
def test_recorded_http_fixtures_are_reproducible(profile: str) -> None:
    for name, content in capture(profile).items():
        assert (FIXTURE_ROOT / profile / name).read_text(encoding="utf-8") == content, f"{profile}/{name}"


@pytest.mark.parametrize("fault", ["duplicate", "boolean_sequence", "next_cursor", "gap", "nested_count",
                                   "missing_events", "bad_version", "bad_capability", "missing_optional_path",
                                   "lying_optional_endpoint", "cross_origin", "nan_extension", "wrong_header",
                                   "missing_roster", "missing_inspectors", "missing_states", "analytics_counts",
                                   "missing_perspectives", "duplicate_perspective", "history_scope", "history_bounds",
                                   "missing_time", "time_scope", "time_outcome", "time_cursor", "time_nan"])
def test_malformed_backends_fail_without_coercion_or_unsafe_discovery(fault: str) -> None:
    replay = fixture_fetch("full")
    requested: list[str] = []

    def fetch(path: str) -> HttpReply:
        requested.append(path)
        reply = replay(path)
        body = json.loads(reply.body)
        if path == "/api/contract":
            if fault == "bad_version": body["contract_version"] = "incompatible-v2"
            if fault == "bad_capability": body["capabilities"]["mom"]["state"] = "maybe"
            if fault == "missing_optional_path": body["api_paths"].pop("analytics")
            if fault == "cross_origin": body["api_paths"]["state"] = "//untrusted.invalid/state"
            if fault == "missing_states": body["capability_schema"]["states"] = ["available"]
        if path == "/api/state?include_events=true":
            if fault == "nested_count": body["live_metrics"]["event_count"] = "5"
            if fault == "missing_events": body.pop("normalized_events")
            if fault == "nan_extension": body["context"]["vendor"] = float("nan")
            if fault == "missing_roster": body.pop("federate_roster")
            if fault == "missing_inspectors": body["inspectors"].pop("objects")
            if fault == "missing_perspectives": body["inspectors"].pop("object_observations")
            if fault == "duplicate_perspective":
                body["inspectors"]["object_observations"]["snapshots"].append(body["inspectors"]["object_observations"]["snapshots"][0])
            if fault == "history_scope": body["inspectors"]["object_observations"]["changes"][0]["federate"] = "NotObserved"
            if fault == "history_bounds": body["inspectors"]["object_observations"]["history_first_sequence"] += 1
            if fault == "missing_time": body["inspectors"].pop("time")
            if fault == "time_scope": body["inspectors"]["time"]["requests"][0]["scope"]["session_id"] = "unreported-session"
            if fault == "time_outcome": body["inspectors"]["time"]["requests"][-1]["latency_seconds"] = 0
            if fault == "time_cursor": body["inspectors"]["time"]["snapshots"][0]["last_sequence"] = 999
            if fault == "time_nan": body["inspectors"]["time"]["requests"][0]["latency_seconds"] = float("nan")
        if path == "/api/events?after=0&limit=2":
            if fault == "duplicate": body["events"][1]["sequence"] = body["events"][0]["sequence"]
            if fault == "boolean_sequence": body["events"][0]["sequence"] = True
            if fault == "next_cursor": body["next_after"] = 0
            if fault == "gap": body["gap"] = True
        if path == "/api/analytics" and fault == "lying_optional_endpoint":
            return HttpReply(status=404, body="{}")
        if path == "/api/analytics" and fault == "analytics_counts": body["events_in_window"] += 1
        headers = dict(reply.headers)
        if fault == "wrong_header": headers["X-Umbra-Observer-Contract"] = "other"
        return HttpReply(status=reply.status, headers=headers, body=json.dumps(body))

    report = ObserverConformance("http://fixture", fetch=fetch).run()
    assert not report.passed, fault
    assert all(not path.startswith("//") for path in requested)


@pytest.mark.parametrize("path", ["https://other/state", "//other/state", "/state?token=x", "/state#fragment", "/\\host"])
def test_discovered_paths_cannot_escape_the_selected_backend(path: str) -> None:
    with pytest.raises(ValueError):
        local_path(path)


@pytest.fixture(scope="session")
def java_classes() -> Iterator[Path]:
    if not shutil.which("javac") or not shutil.which("java"):
        pytest.skip("JDK 17+ required for the independent Java HTTP fixture lane")
    source = Path(__file__).resolve().parents[2] / "umbra-rti-observer-java-fixture" / "src" / "ObserverFixtureServer.java"
    with tempfile.TemporaryDirectory(prefix="observer Java classes ") as temporary:
        output = Path(temporary)
        subprocess.run(["javac", "--release", "17", "--add-modules", "jdk.httpserver", "-d", str(output), str(source)],
                       check=True, capture_output=True, text=True)
        yield output


@pytest.fixture
def java_server(java_classes: Path):
    processes: list[subprocess.Popen[str]] = []

    def start(profile: str) -> str:
        child = subprocess.Popen(["java", "--add-modules", "jdk.httpserver", "-cp", str(java_classes),
                                  "ObserverFixtureServer", str(FIXTURE_ROOT / profile)],
                                 stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        processes.append(child)
        lines: queue.Queue[str] = queue.Queue()

        def collect() -> None:
            assert child.stdout is not None
            for line in child.stdout:
                lines.put(line)
        threading.Thread(target=collect, daemon=True).start()
        line = lines.get(timeout=15)
        assert line.startswith("OBSERVER_FIXTURE_URL="), line
        return line.strip().split("=", 1)[1]
    yield start
    for child in processes:
        child.terminate()
        try:
            child.wait(timeout=5)
        except subprocess.TimeoutExpired:
            child.kill()
            child.wait()
        if child.stdout:
            child.stdout.close()


@pytest.mark.parametrize("profile", PROFILES)
def test_independent_java_uses_the_same_contract_checks(java_server, profile: str) -> None:
    runner = ObserverConformance(java_server(profile))
    if profile == "reconnect":
        assert not runner.run().passed
    report = runner.run()
    assert report.passed, report.model_dump_json(indent=2)
