"""Internal v2 capture/import evidence, not normative RTI validation."""

from __future__ import annotations

import copy
import json

import pytest
from fastapi.testclient import TestClient
from pydantic import ValidationError
from umbra_rti_observer.core import ObserverStore
from umbra_rti_observer.models import to_json_object
from umbra_rti_observer.recording_codec import apply_edits
from umbra_rti_observer.recording_models import (
    CompactRecordingDocument,
    RecordingDocument,
    RecordingFrame,
)
from umbra_rti_observer.scale_workload import seed_inspectors
from umbra_rti_observer.server import create_app


def compact():
    store = ObserverStore(max_events=2)
    info = store.start_capture("Compact evidence", storage_format="compact-v2")
    store.publish_many(
        [
            {
                "event_type": "object.updated",
                "object_key": "one",
                "attributes": {"v": index},
            }
            for index in range(4)
        ]
    )
    store.set_context(federation="Example")
    store.clear()
    store.publish({"event_type": "callback.invoked"})
    store.stop_capture(info.recording_id)
    return store, store.capture_document(info.recording_id)


def test_every_committed_state_and_arrival_reconstructs_exactly_after_rollover_and_clear():
    store = ObserverStore(max_events=2)
    info = store.start_capture("Exact timeline", storage_format="compact-v2")
    expected = [store.state()]
    for index in range(15):
        store.publish_many(
            [
                {
                    "event_type": "object.updated",
                    "object_key": "one",
                    "attributes": {"v": index, "known": None},
                }
                for _ in range(3)
            ]
        )
        expected.append(store.state())
    store.clear()
    expected.append(store.state())
    store.set_context(federation="After clear")
    expected.append(store.state())
    store.stop_capture(info.recording_id)
    expected.append(store.state())
    data = store.capture_document(info.recording_id)
    doc = CompactRecordingDocument.model_validate(data)
    working = to_json_object(doc.baseline.model_dump(mode="python", exclude_unset=True))
    for row, snapshot in zip(doc.frames, expected, strict=True):
        working = apply_edits(working, [edit.to_edit() for edit in row.edits])
        # Idle wall-clock metrics differ, so compare the persistent evidence and
        # exact event/inspector state rather than asking for a second clock read.
        assert working["normalized_events"] == snapshot["normalized_events"]
        assert (
            working["inspectors"]["object_observations"]
            == snapshot["inspectors"]["object_observations"]
        )
        assert working["context"] == snapshot["context"]
        RecordingFrame.model_validate(
            {**row.model_dump(exclude={"edits", "state_bytes"}), "state": working}
        )
    assert data == doc.model_dump(mode="json", exclude_unset=True)
    assert [event.sequence for row in doc.frames for event in row.arrivals] == list(
        range(1, 46)
    )


@pytest.mark.parametrize(
    "fault",
    [
        "codec",
        "path",
        "index_type",
        "splice",
        "state_model",
        "changed_history",
        "bytes",
        "work",
        "edits",
        "initial",
        "nan",
        "offset",
    ],
)
def test_corrupt_programs_and_accounting_never_enter_replay(fault):
    _, source = compact()
    CompactRecordingDocument.model_validate(source)
    data = copy.deepcopy(source)
    if fault == "codec":
        data["codec"] = "executable-script"
    if fault == "path":
        data["frames"][1]["edits"] = [
            {"op": "set", "path": ["missing", "child"], "value": 1}
        ]
    if fault == "index_type":
        data["frames"][1]["edits"] = [{"op": "set", "path": [True], "value": 1}]
    if fault == "splice":
        data["frames"][1]["edits"] = [
            {
                "op": "splice",
                "path": ["events"],
                "index": 9999,
                "delete_count": 0,
                "items": [],
            }
        ]
    if fault == "state_model":
        data["frames"][1]["edits"].append(
            {"op": "set", "path": ["retention", "max_events"], "value": "2"}
        )
    if fault == "changed_history":
        data["frames"][2]["edits"].append(
            {
                "op": "set",
                "path": ["normalized_events", 0, "source"],
                "value": "rewritten",
            }
        )
    if fault == "bytes":
        data["frames"][0]["state_bytes"] = 1
    if fault == "work":
        data["decoded_state_bytes"] = 1
    if fault == "edits":
        data["edit_count"] = 0
    if fault == "initial":
        data["frames"][0]["edits"] = [{"op": "delete", "path": ["context"]}]
    if fault == "nan":
        data["baseline"]["context"]["bad"] = float("nan")
    if fault == "offset":
        data["frames"][1]["offset_seconds"] = -1
    if fault != "edits":
        data["edit_count"] = sum(len(row["edits"]) for row in data["frames"])
    if fault != "work":
        data["decoded_state_bytes"] = sum(row["state_bytes"] for row in data["frames"])
    with pytest.raises(ValidationError):
        CompactRecordingDocument.model_validate(data)


def test_oversized_evidence_does_not_enlarge_state_and_recording_keeps_complete_arrival():
    store, source = compact()
    info = store.start_capture(
        "State bound", storage_format="compact-v2", replace_id=source["recording_id"]
    )
    store.publish(
        {
            "event_type": "callback.invoked",
            "details": {"large": "x" * (8 * 1024 * 1024)},
        }
    )
    assert store.capture_info().active
    assert store.state()["retention"]["next_sequence"] == 7
    store.stop_capture(info.recording_id)
    doc = CompactRecordingDocument.model_validate(
        store.capture_document(info.recording_id)
    )
    assert (
        len(doc.frames) == 3
        and len(doc.frames[1].arrivals[0].details["large"]) == 8 * 1024 * 1024
    )


def test_compact_codec_still_rejects_oversized_reconstructed_state_independent_of_store_admission():
    from umbra_rti_observer.capture import CompactCaptureBuffer

    store, source = compact()
    buffer = CompactCaptureBuffer("Decode state bound", source["contract"])
    state = store.state()
    buffer.append(state, "start")
    state["context"]["oversized"] = "x" * (8 * 1024 * 1024)
    buffer.append(state, "checkpoint")
    assert buffer.info().stop_reason == "state-limit" and buffer.info().frame_count == 1


def test_optional_inspector_provenance_survives_compact_baseline_and_clock_changes():
    store = ObserverStore(max_events=2)
    seed_inspectors(store, 2, 2)
    baseline = store.state()["inspectors"]
    info = store.start_capture("Rich evidence", storage_format="compact-v2")
    store.checkpoint_capture()
    store.stop_capture(info.recording_id)
    doc = CompactRecordingDocument.model_validate(
        store.capture_document(info.recording_id)
    )
    saved = doc.model_dump(mode="json", exclude_unset=True)
    assert {
        key: value
        for key, value in saved["baseline"]["inspectors"].items()
        if key != "time"
    } == {key: value for key, value in baseline.items() if key != "time"}
    assert (
        saved["baseline"]["inspectors"]["time"]["snapshots"]
        == baseline["time"]["snapshots"]
    )
    assert len(saved["baseline"]["inspectors"]["time"]["requests"]) == 2
    for name in (
        "declarations",
        "relationships",
        "coordination",
        "interaction_flow",
        "ownership",
        "regions",
        "mom",
        "fom",
    ):
        assert saved["baseline"]["inspectors"][name]
    assert store._capture._previous is None  # Stopping releases the working diff tree.


def test_declared_work_and_operation_limits_reject_before_expansion():
    _, source = compact()
    too_much = copy.deepcopy(source)
    too_much["decoded_state_bytes"] = 1073741825
    with pytest.raises(ValidationError):
        CompactRecordingDocument.model_validate(too_much)
    too_many = copy.deepcopy(source)
    too_many["edit_count"] = 100001
    with pytest.raises(ValidationError):
        CompactRecordingDocument.model_validate(too_many)


@pytest.mark.parametrize(
    "field",
    [
        "context",
        "adapters",
        "capabilities",
        "federate_roster",
        "objects",
        "interactions",
    ],
)
@pytest.mark.parametrize("format", ["legacy", "baseline", "delta"])
def test_constructor_defaults_cannot_disguise_missing_wire_evidence(field, format):
    store, source = compact()
    if format == "legacy":
        info = store.start_capture(
            "Full wire evidence", replace_id=source["recording_id"]
        )
        store.stop_capture(info.recording_id)
        source = store.capture_document(info.recording_id)
    data = copy.deepcopy(source)
    path = ["inspectors", field] if field in {"objects", "interactions"} else [field]
    if format == "delta":
        data["frames"][1]["edits"].append({"op": "delete", "path": path})
        data["edit_count"] += 1
    else:
        state = data["baseline"] if format == "baseline" else data["frames"][0]["state"]
        if len(path) == 2:
            del state[path[0]][path[1]]
        else:
            del state[path[0]]
    model = RecordingDocument if format == "legacy" else CompactRecordingDocument
    with pytest.raises(ValidationError, match="wire collections"):
        model.model_validate(data)


def test_named_http_negotiation_legacy_default_and_bounded_compact_validation():
    store = ObserverStore()
    with TestClient(create_app(store)) as client:
        assert (
            client.get("/api/contract").json()["capabilities"]["recording_compact"][
                "state"
            ]
            == "available"
        )
        legacy = client.post("/api/recordings/start", json={"label": "Legacy"}).json()
        assert legacy["storage_format"] == "full-v1"
        client.post(
            "/api/recordings/stop", json={"recording_id": legacy["recording_id"]}
        )
        assert (
            client.get(
                "/api/recordings/export",
                params={"recording_id": legacy["recording_id"]},
            ).json()["recording_version"]
            == "umbra-observer-recording-v1"
        )
        info = client.post(
            "/api/recordings/start",
            json={
                "label": "New",
                "replace_id": legacy["recording_id"],
                "storage_format": "compact-v2",
            },
        ).json()
        store.publish({"event_type": "callback.invoked"})
        client.post("/api/recordings/stop", json={"recording_id": info["recording_id"]})
        assert (
            client.get(
                "/api/recordings/export", params={"recording_id": info["recording_id"]}
            ).status_code
            == 409
        )
        response = client.get(
            "/api/recordings/export",
            params={
                "recording_id": info["recording_id"],
                "storage_format": "compact-v2",
            },
        )
        assert response.status_code == 200
        doc = response.json()
        assert doc["recording_version"] == "umbra-observer-recording-v2"
        assert client.post("/api/recordings/validate", json=doc).json() == doc
        doc["frames"][1]["edits"].append(
            {"op": "set", "path": ["context", "credential"], "value": float("nan")}
        )
        result = client.post(
            "/api/recordings/validate",
            content=json.dumps(doc),
            headers={"Content-Type": "application/json"},
        )
        assert result.status_code == 422 and "credential" not in result.text
        deeply_nested = (
            '{"recording_version":"umbra-observer-recording-v2","baseline":'
            + "[" * 2000
            + "0"
            + "]" * 2000
            + "}"
        )
        rejected = client.post(
            "/api/recordings/validate",
            content=deeply_nested,
            headers={"Content-Type": "application/json"},
        )
        assert rejected.status_code == 422 and len(rejected.content) < 200
