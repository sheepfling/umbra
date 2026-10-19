"""Internal view-configuration contract evidence, not RTI conformance."""
from __future__ import annotations

import json
from pathlib import Path
from tempfile import TemporaryDirectory

import pytest
from fastapi.testclient import TestClient
from pydantic import ValidationError
from umbra_rti_observer.conformance import HttpReply, ObserverConformance
from umbra_rti_observer.core import ObserverStore
from umbra_rti_observer.server import create_app
from umbra_rti_observer.workspace_models import (
    WorkspaceDocument,
    WorkspaceLibrary,
    default_library,
)
from umbra_rti_observer.workspace_schema import main as schema_main
from umbra_rti_observer.workspace_schema import workspace_cases, workspace_contract


def document(**values):
    return {"workspace_version": "umbra-observer-workspace-v1", "name": "Miguel", **values}


def test_workspace_defaults_are_named_and_normalized_without_any_observation_data():
    config = WorkspaceDocument.model_validate(document(settings={"events": {"search": "load"}}))
    assert config.settings.events.search == "load" and config.settings.events.family == ""
    assert config.settings.window_seconds == 300 and config.settings.mom.counter is None
    schema = workspace_contract()
    assert schema["defaults"] == default_library().model_dump(mode="json")
    library = WorkspaceLibrary.model_validate({"library_version": "umbra-observer-workspace-library-v1", "saved": [document()]})
    assert library.saved[0].settings == default_library().current


@pytest.mark.parametrize("values", [
    {"workspace_version": "future"}, {"state": {}}, {"frames": []}, {"api_paths": {"state": "https://never-follow.invalid"}},
    {"settings": {"credentials": "not-config"}}, {"settings": {"commands": {"confirmed": True}}},
    {"settings": {"window_seconds": "300"}}, {"settings": {"window_seconds": True}},
    {"settings": {"time": {"grants_only": "true"}}}, {"settings": {"view": "https://never-follow.invalid"}},
    {"name": " "}, {"name": "\ud800"}, {"settings": {"events": {"search": "x" * 2049}}},
])
def test_workspace_files_reject_authority_data_unknown_fields_types_and_text_limits(values):
    with pytest.raises(ValidationError): WorkspaceDocument.model_validate(document(**values))


def test_numeric_chart_targets_are_bounded_and_perspective_specific():
    choices = [{"scope": None, "object_key": "same", "attribute": "a"}, {"scope": "Alice", "object_key": "same", "attribute": "b"}]
    assert len(WorkspaceDocument.model_validate(document(settings={"object_charts": choices})).settings.object_charts) == 2
    with pytest.raises(ValidationError): WorkspaceDocument.model_validate(document(settings={"object_charts": choices + [choices[0]]}))
    with pytest.raises(ValidationError): WorkspaceDocument.model_validate(document(settings={"object_charts": [
        {"scope": None, "object_key": str(index), "attribute": "a"} for index in range(65)]}))


def test_saved_libraries_reject_duplicate_names_and_excess_profiles():
    root = {"library_version": "umbra-observer-workspace-library-v1"}
    with pytest.raises(ValidationError): WorkspaceLibrary.model_validate({**root, "saved": [document(), document()]})
    with pytest.raises(ValidationError): WorkspaceLibrary.model_validate({**root, "saved": [document(name=str(index)) for index in range(13)]})


def test_workspace_http_only_normalizes_configuration_and_can_be_omitted_independently():
    store = ObserverStore()
    store.publish({"event_type": "runtime.error", "details": {"private": "must-not-export"}})
    before = store.state()
    with TestClient(create_app(store, enable_alerts=False, enable_recording=False)) as client:
        defaults = client.get("/api/workspaces/defaults")
        assert defaults.status_code == 200 and "must-not-export" not in defaults.text
        config = client.post("/api/workspaces/validate", json=document(settings={"view": "actions"}))
        assert config.status_code == 200 and config.json()["settings"]["view"] == "actions"
        assert client.post("/api/workspaces/validate", json=document(api_paths={"state": "https://never-follow.invalid"})).status_code == 422
        assert store.state()["normalized_events"] == before["normalized_events"]
    with TestClient(create_app(store, enable_workspace_validation=False)) as client:
        assert "workspace_validate" not in client.get("/api/contract").json()["api_paths"]
        assert client.get("/api/workspaces/defaults").status_code == 404
        assert client.get("/api/state").status_code == 200


def test_default_factory_schema_supports_empty_saved_arrays_without_inventing_required_names():
    contract = workspace_contract()
    library = contract["library_schema"]
    assert library["$defs"]["WorkspaceDocument"]["properties"]["settings"]["default"] == default_library().current.model_dump(mode="json")
    assert "default" not in library["$defs"]["WorkspaceDocument"]["properties"]["name"]
    assert len(json.dumps(contract).encode()) < 128 * 1024


def test_workspace_policy_corpus_matches_models_and_checked_in_frontend_without_rewriting():
    frontend = Path(__file__).resolve().parents[2] / "umbra-rti-observer-ui"
    cases = frontend / "scripts/fixtures/workspaces.json"
    snapshot = frontend / "src/api/workspace-contract.json"
    original = cases.read_bytes()
    assert json.loads(original) == workspace_cases()
    assert schema_main(["--frontend", str(snapshot), "--cases", str(cases)]) == 0
    with TemporaryDirectory(prefix="observer-workspace-corpus-") as temporary:
        stale = Path(temporary) / "stale.json"
        stale.write_text("{}", encoding="utf-8")
        assert schema_main(["--cases", str(stale)]) == 1
        assert stale.read_text(encoding="utf-8") == "{}" and cases.read_bytes() == original


def test_independent_normalized_document_and_library_byte_budgets():
    # Every individual field fits, but complete documents/libraries must fit too.
    choices = [{"object_key": str(index), "attribute": "🔎" * 8192} for index in range(2)]
    with pytest.raises(ValidationError):
        WorkspaceDocument.model_validate(document(settings={"object_charts": choices}))
    saved = [document(name=str(index), settings={"object_charts": [{"object_key": "one", "attribute": "x" * 8192}],
        "selected_federate": "x" * 8192, "objects": {"selected": "x" * 8192}, "time": {"selected": "x" * 8192},
        "mom": {"counter": "x" * 8192}, "fom": {"selected": "x" * 8192}}) for index in range(12)]
    assert all(WorkspaceDocument.model_validate(row) for row in saved)
    with pytest.raises(ValidationError):
        WorkspaceLibrary.model_validate({"library_version": "umbra-observer-workspace-library-v1", "saved": saved})


def test_http_read_conformance_validates_defaults_but_does_not_claim_file_normalization():
    reads = []
    with TestClient(create_app()) as client:
        def fetch(path):
            reads.append(path)
            response = client.get(path)
            return HttpReply(status=response.status_code, headers=dict(response.headers), body=response.text)
        report = ObserverConformance("http://fixture", fetch=fetch).run()
    assert report.passed, report.model_dump_json(indent=2)
    assert "/api/workspaces/defaults" in reads and "/api/workspaces/validate" not in reads
    assert "workspace-import-validation" in report.unchecked_features


def test_federate_preferences_are_named_bounded_and_distinct_from_global_views():
    settings = {"federate_views": [{"participant": "Alice", "events": {"search": "alice-only"}, "time": {"selected": "alice-scope"}},
        {"participant": "Bob", "events": {"search": "bob-only"}, "time": {"selected": "bob-scope"}}]}
    config = WorkspaceDocument.model_validate(document(settings=settings))
    assert config.settings.events.search == "" and config.settings.time.selected is None
    assert config.settings.federate_views[0].events.search == "alice-only"
    assert config.settings.federate_views[1].time.selected == "bob-scope"
    with pytest.raises(ValidationError):
        WorkspaceDocument.model_validate(document(settings={"federate_views": [{"participant": "Alice"}] * 2}))
    with pytest.raises(ValidationError):
        WorkspaceDocument.model_validate(document(settings={"federate_views": [{"participant": str(index)} for index in range(65)]}))
