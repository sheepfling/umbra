"""Internal observer recording evidence, not native RTI or normative validation."""

from __future__ import annotations

import copy
import json
import time
from concurrent.futures import ThreadPoolExecutor
from types import SimpleNamespace

import pytest
from fastapi.testclient import TestClient
from pydantic import ValidationError
from umbra_rti_observer import capture
from umbra_rti_observer.capture import MAX_RECORDING_BYTES, CaptureConflict
from umbra_rti_observer.conformance import HttpReply, ObserverConformance
from umbra_rti_observer.core import ObserverStore
from umbra_rti_observer.models import JsonObject
from umbra_rti_observer.recording_models import RecordingDocument
from umbra_rti_observer.server import create_app


def recording(store: ObserverStore | None = None) -> tuple[ObserverStore, JsonObject]:
    if store is None:
        store = ObserverStore(max_events=2)
    info = store.start_capture("Example capture")
    store.publish_many(
        [
            {"event_type": "callback.invoked", "details": {"value": index}}
            for index in range(3)
        ]
    )
    store.stop_capture(info.recording_id)
    return store, store.capture_document(info.recording_id)


def test_exact_baseline_batches_metadata_clear_and_persistent_inspectors_survive() -> (
    None
):
    store = ObserverStore(max_events=1)
    store.publish(
        {
            "event_type": "object.updated",
            "object_key": "prior",
            "attributes": {"value": 7},
        }
    )
    store.publish({"event_type": "callback.invoked"})
    info = store.start_capture("After history expiry")
    store.publish_many(
        [
            {"event_type": "callback.invoked", "details": {"marker": index}}
            for index in range(3)
        ]
    )
    store.set_context(federation="Changed")
    store.set_status("running")
    store.set_capabilities({"mom_inspection": "unsupported"})
    store.clear()
    store.publish({"event_type": "callback.invoked"})
    store.stop_capture(info.recording_id)
    doc = RecordingDocument.model_validate(store.capture_document(info.recording_id))
    assert [row.sequence for row in doc.frames[0].state.normalized_events] == [2]
    assert doc.frames[0].state.inspectors.objects[0].object_key == "prior"
    assert [row.sequence for row in doc.frames[1].arrivals] == [3, 4, 5]
    assert [row.sequence for row in doc.frames[1].state.normalized_events] == [5]
    assert [row.change for row in doc.frames] == [
        "start",
        "events",
        "metadata",
        "metadata",
        "metadata",
        "clear",
        "events",
        "stop",
    ]
    assert (
        not doc.frames[5].state.normalized_events
        and not doc.frames[5].state.inspectors.objects
    )
    assert doc.frames[-1].state.context["federation"] == "Changed"
    store.publish({"event_type": "runtime.error"})
    assert store.capture_document(info.recording_id) == doc.model_dump(
        mode="json", exclude_unset=True
    )


def test_monotonic_offsets_do_not_follow_wall_clock_changes(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    ticks = [100.0, 50.0]
    monkeypatch.setattr(
        capture,
        "time",
        SimpleNamespace(time=lambda: ticks[1], monotonic=lambda: ticks[0]),
    )
    store = ObserverStore()
    info = store.start_capture("Clock changes")
    ticks[:] = [105.0, 10.0]
    store.publish({"event_type": "callback.invoked"})
    ticks[:] = [110.0, 5.0]
    store.stop_capture(info.recording_id)
    doc = RecordingDocument.model_validate(store.capture_document(info.recording_id))
    assert [row.offset_seconds for row in doc.frames] == [0, 5, 10]
    assert [row.captured_at for row in doc.frames] == [50, 10, 5]
    assert doc.duration_seconds == 10


@pytest.mark.parametrize(
    "field,value",
    [
        ("label", " "),
        ("label", True),
        ("max_frames", 0),
        ("max_frames", True),
        ("max_frames", 2001),
        ("max_bytes", False),
        ("max_bytes", MAX_RECORDING_BYTES + 1),
    ],
)
def test_capacities_and_labels_are_explicit(field: str, value: object) -> None:
    kwargs = {"label": "Example", field: value}
    with pytest.raises(ValueError):
        ObserverStore().start_capture(**kwargs)


def test_limits_stop_capture_without_evicting_or_interrupting_observation() -> None:
    store = ObserverStore(max_events=1)
    info = store.start_capture("Small", max_frames=2)
    store.publish({"event_type": "callback.invoked"})
    store.publish({"event_type": "runtime.error"})
    assert store.capture_info().stop_reason == "frame-limit"
    doc = RecordingDocument.model_validate(store.capture_document(info.recording_id))
    assert len(doc.frames) == 2 and doc.frames[-1].state.retention.last_sequence == 1
    assert store.state()["retention"]["last_sequence"] == 2
    with pytest.raises(CaptureConflict):
        store.start_capture("Accidental overwrite")
    with pytest.raises(ValueError):
        store.start_capture("Too small", replace_id=info.recording_id, max_bytes=1)
    assert (
        store.capture_document(info.recording_id)["recording_id"] == info.recording_id
    )
    replacement = store.start_capture(
        "Explicit replacement", replace_id=info.recording_id
    )
    assert replacement.recording_id != info.recording_id
    with pytest.raises(CaptureConflict):
        store.stop_capture(info.recording_id)


def test_byte_limit_and_invalid_state_never_break_ingress() -> None:
    store = ObserverStore()
    info = store.start_capture("Bounded", max_bytes=65536)
    store.publish({"event_type": "callback.invoked", "details": {"large": "x" * 65536}})
    assert store.capture_info().stop_reason == "byte-limit"
    assert store.state()["retention"]["next_sequence"] == 2
    assert (
        len(
            RecordingDocument.model_validate(
                store.capture_document(info.recording_id)
            ).frames
        )
        == 1
    )


def test_concurrent_committed_batches_are_ordered_and_detached_from_mutable_inputs() -> (
    None
):
    store = ObserverStore(max_events=2)
    info = store.start_capture("Concurrent")

    def publish(index: int) -> None:
        store.publish_many(
            [
                {"event_type": "callback.invoked", "details": {"batch": index}}
                for _ in range(3)
            ]
        )

    with ThreadPoolExecutor(max_workers=4) as pool:
        list(pool.map(publish, range(12)))
    store.stop_capture(info.recording_id)
    doc = RecordingDocument.model_validate(store.capture_document(info.recording_id))
    assert len(doc.frames) == 14
    assert [row.sequence for frame in doc.frames for row in frame.arrivals] == list(
        range(1, 37)
    )


@pytest.mark.parametrize(
    "fault",
    [
        "version",
        "contract",
        "offset",
        "index",
        "duration",
        "missing_events",
        "cursor",
        "arrival",
        "duplicate",
        "changed_history",
        "bytes",
        "nan",
        "start_again",
    ],
)
def test_malformed_or_inconsistent_recordings_are_rejected(fault: str) -> None:
    _, source = recording()
    RecordingDocument.model_validate(source)
    data = copy.deepcopy(source)
    if fault == "version":
        data["recording_version"] = "future"
    if fault == "contract":
        data["contract"]["contract_version"] = "other"
    if fault == "offset":
        data["frames"][1]["offset_seconds"] = -1
    if fault == "index":
        data["frames"][1]["index"] = True
    if fault == "duration":
        data["duration_seconds"] = 1000
    if fault == "missing_events":
        data["frames"][0]["state"].pop("normalized_events")
    if fault == "cursor":
        data["frames"][1]["state"]["retention"]["last_sequence"] = 99
    if fault == "arrival":
        data["frames"][1]["arrivals"].pop(0)
    if fault == "duplicate":
        data["frames"][1]["arrivals"].append(data["frames"][1]["arrivals"][0])
    if fault == "changed_history":
        data["frames"][-1]["state"]["normalized_events"][0]["details"]["value"] = (
            "mutated"
        )
    if fault == "bytes":
        data["max_bytes"] = 1
    if fault == "nan":
        data["frames"][0]["state"]["context"]["invalid"] = float("nan")
    if fault == "start_again":
        data["frames"][1]["change"] = "start"
    with pytest.raises((ValidationError, ValueError)):
        RecordingDocument.model_validate(data)


def test_named_routes_validate_export_and_import_without_mutating_a_provider() -> None:
    store = ObserverStore()
    with TestClient(create_app(store)) as client:
        paths = client.get("/api/contract").json()["api_paths"]
        assert client.get(paths["recordings"]).json()["recording"] is None
        info = client.post(
            paths["recording_start"], json={"label": "API capture"}
        ).json()
        assert (
            client.post(paths["recording_start"], json={"label": "Second"}).status_code
            == 409
        )
        assert (
            client.get(
                paths["recording_export"], params={"recording_id": info["recording_id"]}
            ).status_code
            == 409
        )
        store.publish({"event_type": "runtime.error"})
        assert (
            client.post(
                paths["recording_stop"], json={"recording_id": "wrong"}
            ).status_code
            == 409
        )
        assert (
            client.post(
                paths["recording_stop"], json={"recording_id": info["recording_id"]}
            ).status_code
            == 200
        )
        response = client.get(
            paths["recording_export"], params={"recording_id": info["recording_id"]}
        )
        doc = RecordingDocument.model_validate(response.json())
        assert "attachment" in response.headers["content-disposition"]
        before = store.state()
        assert (
            client.post(paths["recording_validate"], json=response.json()).status_code
            == 200
        )
        analysis = client.post(
            paths["recording_analyze"],
            json={"frame": response.json()["frames"][-1], "window_seconds": 300},
        )
        assert (
            analysis.status_code == 200
            and analysis.json()["generated_at"] == doc.frames[-1].captured_at
        )
        assert store.state()["retention"] == before["retention"]
        assert (
            client.post(paths["recording_start"], json={"label": " "}).status_code
            == 422
        )


def test_disabled_recordings_omit_paths_and_resources() -> None:
    with TestClient(create_app(enable_recording=False)) as client:
        contract = client.get("/api/contract").json()
        assert contract["capabilities"]["recording"]["state"] == "disabled"
        assert not any(name.startswith("recording") for name in contract["api_paths"])
        assert client.get("/api/recordings").status_code == 404
        assert (
            "/api/recordings/validate"
            not in client.get("/openapi.json").json()["paths"]
        )


def test_import_size_is_bounded_for_declared_and_chunked_bodies() -> None:
    with TestClient(create_app()) as client:
        assert (
            client.post(
                "/api/recordings/validate",
                content=b"{}",
                headers={
                    "content-length": str(MAX_RECORDING_BYTES + 1),
                    "content-type": "application/json",
                },
            ).status_code
            == 413
        )

        def chunks():
            for _ in range(17):
                yield b"x" * 1024 * 1024

        assert (
            client.post(
                "/api/recordings/validate",
                content=chunks(),
                headers={"content-type": "application/json"},
            ).status_code
            == 413
        )


def test_invalid_import_errors_are_bounded_and_never_echo_federation_data() -> None:
    _, data = recording()
    data["frames"][0]["state"]["context"]["secret"] = "do-not-echo-this-value"
    data["recording_version"] = "bad"
    with TestClient(create_app()) as client:
        response = client.post("/api/recordings/validate", json=data)
        assert response.status_code == 422 and len(response.content) < 200
        assert "do-not-echo-this-value" not in response.text
        data["recording_version"] = "umbra-observer-recording-v1"
        data["frames"][0]["state"]["context"]["invalid"] = float("nan")
        assert (
            client.post(
                "/api/recordings/validate",
                content=json.dumps(data),
                headers={"content-type": "application/json"},
            ).status_code
            == 422
        )


def test_periodic_checkpoints_capture_idle_ages_without_inventing_envelopes() -> None:
    store = ObserverStore()
    with TestClient(create_app(store)) as client:
        info = client.post("/api/recordings/start", json={"label": "Idle clock"}).json()
        deadline = time.monotonic() + 4
        while store.capture_info().frame_count == 1 and time.monotonic() < deadline:
            time.sleep(0.05)
        client.post("/api/recordings/stop", json={"recording_id": info["recording_id"]})
        doc = RecordingDocument.model_validate(
            store.capture_document(info["recording_id"])
        )
        assert any(
            frame.change == "checkpoint" and frame.offset_seconds > 0
            for frame in doc.frames
        )
        assert all(
            not frame.arrivals and not frame.state.normalized_events
            for frame in doc.frames
        )


def test_unencodable_capture_stops_without_interrupting_collection() -> None:
    store = ObserverStore()
    info = store.start_capture("Invalid UTF-8")
    store.publish(
        {"event_type": "callback.invoked", "details": {"unencodable": "\ud800"}}
    )
    assert store.capture_info().stop_reason == "invalid-state"
    assert (
        RecordingDocument.model_validate(
            store.capture_document(info.recording_id)
        ).stop_reason
        == "invalid-state"
    )
    assert store.state()["retention"]["next_sequence"] == 2
    assert store.state()["retention"]["dropped_events"] == 1


@pytest.mark.parametrize(
    "fault",
    [
        None,
        "missing_path",
        "wrong_scope",
        "boolean_frames",
        "lying_active",
        "over_budget",
        "lying_window_cap",
        "lying_compact_cap",
    ],
)
def test_read_conformance_checks_advertised_recording_status_without_starting_or_stopping(
    fault: str | None,
) -> None:
    store = ObserverStore()
    with TestClient(create_app(store)) as client:
        info = client.post(
            "/api/recordings/start", json={"label": "Read-only checker"}
        ).json()

        def fetch(path: str) -> HttpReply:
            response = client.get(path)
            data = response.json()
            if path == "/api/contract" and fault == "missing_path":
                data["api_paths"].pop("recordings")
            if path == "/api/contract" and fault == "lying_window_cap":
                data["capabilities"]["recording"]["state"] = "disabled"
            if path == "/api/contract" and fault == "lying_compact_cap":
                data["api_paths"].pop("recording_validate")
            if path == "/api/recordings":
                if fault == "wrong_scope":
                    data["scope"] = "federation-control"
                if fault == "boolean_frames":
                    data["recording"]["frame_count"] = True
                if fault == "lying_active":
                    data["recording"]["stop_reason"] = "user"
                if fault == "over_budget":
                    data["recording"]["captured_bytes"] = MAX_RECORDING_BYTES + 1
            return HttpReply(
                status=response.status_code,
                headers=dict(response.headers),
                body=json.dumps(data),
            )

        report = ObserverConformance("http://fixture", fetch=fetch).run()
        assert report.passed is (fault is None), report.model_dump_json(indent=2)
        assert (
            store.capture_info().recording_id == info["recording_id"]
            and store.capture_info().active
        )


def test_recorded_projection_and_legacy_frame_requests_produce_identical_captured_time_analytics() -> (
    None
):
    store = ObserverStore(max_events=1)
    for index in range(100):
        store.publish(
            {
                "event_type": "object.updated",
                "federate": "A",
                "object_key": f"object-{index}",
            }
        )
    _, data = recording(store)
    frame = data["frames"][-1]
    state = frame["state"]
    window = {
        name: state[name]
        for name in (
            "contract_version",
            "retention",
            "federate_roster",
            "normalized_events",
            "storage",
        )
    }
    projected = {
        "window": window,
        "captured_at": frame["captured_at"],
        "window_seconds": 300,
    }
    with TestClient(create_app()) as client:
        legacy = client.post(
            "/api/recordings/analyze", json={"frame": frame, "window_seconds": 300}
        )
        current = client.post("/api/recordings/analyze", json=projected)
        assert legacy.status_code == current.status_code == 200
        assert current.json() == legacy.json()
        assert len(json.dumps(projected)) < len(json.dumps({"frame": frame}))
