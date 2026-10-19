"""Internal view-configuration contract evidence, not RTI conformance."""
from __future__ import annotations

import json

import pytest
from fastapi.testclient import TestClient
from pydantic import ValidationError

from umbra_rti_observer.core import ObserverStore
from umbra_rti_observer.server import create_app
from umbra_rti_observer.workspace_models import WorkspaceDocument, WorkspaceLibrary, default_library
from umbra_rti_observer.workspace_schema import workspace_contract


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
