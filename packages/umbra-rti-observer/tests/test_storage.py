"""Internal observer storage/coverage evidence, not normative RTI behavior."""
from __future__ import annotations

import json
from concurrent.futures import ThreadPoolExecutor

import pytest
from pydantic import ValidationError

from umbra_rti_observer.analytics import build_analytics
from umbra_rti_observer.core import ObserverStore
from umbra_rti_observer.recording_models import CompactRecordingDocument, RecordingDocument
from umbra_rti_observer.scale_workload import seed_inspectors
from umbra_rti_observer.schemas import AnalyticsWindow, StateResponse, StorageCoverageResponse
from umbra_rti_observer.storage import EvidenceBudget, EvidenceIndex, EvidenceQueue


def coverage(store: ObserverStore, name: str):
    return next(row for row in store.state()["storage"]["evidence"]["collections"] if row["collection"] == name)


def test_shared_byte_budget_expires_lru_whole_records_and_replacement_reaccounts():
    budget = EvidenceBudget(300)
    one = EvidenceIndex("one", 4, budget, lambda _, row: row)
    two = EvidenceIndex("two", 4, budget, lambda _, row: row)
    one["a"] = "x" * 50
    two["b"] = "y" * 50
    one["a"] = "z" * 50  # A new fact about a is the newest LRU entry.
    two["c"] = "w" * 50
    assert list(one) == ["a"] and list(two) == ["c"]
    assert two.coverage()["byte_limit_drops"] == 1
    assert budget.used_bytes == sum(row["accounted_bytes"] for row in budget.coverage()["collections"])
    before = budget.used_bytes
    one["a"] = "short"
    assert budget.used_bytes < before
    one.clear(); two.clear()
    assert budget.used_bytes == 0 and one.dropped == two.dropped == 0


def test_count_and_oversize_limits_do_not_evict_unrelated_records_to_admit_unstorable_data():
    budget = EvidenceBudget(1000, 200)
    index = EvidenceIndex("facts", 2, budget, lambda _, row: row)
    for key in range(3): index[key] = {"value": key}
    assert list(index) == [1, 2] and index.coverage()["count_limit_drops"] == 1
    index[3] = {"value": "x" * 500}
    assert list(index) == [1, 2] and index.coverage()["oversized_drops"] == 1


def test_soft_reservations_keep_static_metadata_without_holding_unused_space():
    budget = EvidenceBudget(1000, 400)
    static = EvidenceIndex("fom_snapshots", 100, budget, lambda _, row: row)
    traffic = EvidenceIndex("events", 100, budget, lambda _, row: row)
    static[1] = "static metadata"
    for key in range(20): traffic[key] = "x" * 100
    assert 1 in static and traffic.dropped > 0 and budget.used_bytes <= 1000
    static.clear()
    for key in range(20, 40): traffic[key] = "x" * 100
    assert sum(row["accounted_bytes"] for row in budget.coverage()["collections"]) == budget.used_bytes
    assert budget.used_bytes > 750  # An absent FOM does not reserve a quarter.


def test_contiguous_envelope_queue_discards_earlier_prefix_when_last_record_cannot_fit():
    budget = EvidenceBudget(1000, 200)
    queue = EvidenceQueue("events", 10, budget, lambda row: row, contiguous=True)
    for key in range(3): queue.append({"sequence": key})
    queue.append({"sequence": 3, "too_big": "x" * 500})
    assert not list(queue) and queue.dropped == 4 and budget.used_bytes == 0
    queue.append({"sequence": 4})
    assert list(queue) == [{"sequence": 4}]


@pytest.mark.parametrize("value", [float("nan"), float("inf"), "\ud800"])
def test_non_wire_values_expire_explicitly_without_leaving_stale_preceding_values(value):
    budget = EvidenceBudget()
    index = EvidenceIndex("facts", 5, budget, lambda _, row: row)
    index[1] = "preceding"
    index[1] = value
    assert not index and index.coverage()["encoding_drops"] == 1 and budget.used_bytes == 0


@pytest.mark.parametrize("field,value", [("max_evidence_bytes", True), ("max_evidence_bytes", 0), ("max_evidence_bytes", 6291457), ("max_evidence_record_bytes", False), ("max_objects", 0), ("max_federates", True), ("max_time_snapshots", -1)])
def test_store_budget_configuration_is_strict(field, value):
    with pytest.raises(ValueError): ObserverStore(**{field: value})


def test_classic_roster_and_objects_expire_without_lifecycle_inference_and_attribution_survives():
    store = ObserverStore(max_events=10, max_objects=2, max_federates=2)
    for index in range(4):
        store.publish({"event_type": "object.updated", "federate": f"Name-{index}", "object_key": f"object-{index}", "attributes": {"v": index}})
    state = StateResponse.model_validate(store.state())
    assert [row.federate for row in state.federate_roster] == ["Name-2", "Name-3"]
    assert len(state.inspectors.objects) == 2 and coverage(store, "federates")["count_limit_drops"] == 2
    analytics = build_analytics(AnalyticsWindow.model_validate(store.analytics_window()))
    assert len(analytics.participants) == 4 and sum(row.event_count for row in analytics.participants) == 4
    assert analytics.participants[0].roster_retained is False and analytics.participants[0].status == "observed"
    assert analytics.roster_storage.dropped_records == 2
    assert all(row.status != "resigned" for row in state.federate_roster)


def test_time_request_survives_snapshot_expiry_and_late_exact_grant_does_not_borrow_state():
    store = ObserverStore(max_time_snapshots=1)
    for name in ("A", "B"):
        store.publish({"event_type": "time.advance_requested", "source": "provider", "federate": name, "federation": "F",
                       "time_observation": {"kind": "request", "session_id": name + "-join", "request_id": "r", "logical_time": 5}})
    state = StateResponse.model_validate(store.state())
    request = next(row for row in state.inspectors.time.requests if row.scope.federate == "A")
    assert request.snapshot_retained is False and request.status == "pending"
    assert state.inspectors.time.dropped_snapshots == 1
    store.publish({"event_type": "time.advance_granted", "source": "provider", "federate": "A", "federation": "F",
                   "time_observation": {"kind": "grant", "session_id": "A-join", "request_id": "r", "logical_time": 5}})
    current = StateResponse.model_validate(store.state()).inspectors.time
    assert next(row for row in current.requests if row.scope.federate == "A").status == "granted"
    assert current.snapshots[0].reported_state is None


@pytest.mark.parametrize("storage_format", ["full-v1", "compact-v2"])
def test_recordings_preserve_arrivals_and_exact_prefix_expiry_even_when_no_envelope_fits(storage_format):
    store = ObserverStore(max_evidence_bytes=30000, max_evidence_record_bytes=3000)
    store.publish({"event_type": "callback.invoked", "details": {"earlier": 1}})
    info = store.start_capture("Storage expiry", storage_format=storage_format)
    store.publish({"event_type": "callback.invoked", "details": {"too_big": "x" * 4000}})
    page = store.event_page(after_sequence=0)
    assert page["gap"] and page["next_after"] == 2 and page["next_sequence"] == 3 and page["retained_events"] == 0
    assert not store.event_page(after_sequence=2)["gap"]
    store.stop_capture(info.recording_id)
    source = store.capture_document(info.recording_id)
    model = CompactRecordingDocument if storage_format == "compact-v2" else RecordingDocument
    recording = model.model_validate(source)
    assert sum(len(row.arrivals) for row in recording.frames) == 1
    assert source["frames"][1]["arrivals"][0]["details"]["too_big"] == "x" * 4000


def test_all_inspector_families_share_accounted_budget_and_reject_corrupt_coverage():
    store = ObserverStore(max_evidence_bytes=40000, max_evidence_record_bytes=10000)
    seed_inspectors(store, 10, 3)
    source = store.state()
    state = StateResponse.model_validate(source)
    assert state.storage.evidence.accounted_bytes <= 40000
    assert not state.storage.evidence.unbudgeted_collections
    assert {row.collection for row in state.storage.evidence.collections} >= {"regions_snapshots", "mom_history", "fom_snapshots", "interaction_flow", "ownership_requests", "declarations", "coordination_history"}
    assert any(row.byte_limit_drops > 0 for row in state.storage.evidence.collections)
    source["storage"]["evidence"]["accounted_bytes"] += 1
    with pytest.raises(ValidationError): StorageCoverageResponse.model_validate(source["storage"])


def test_clear_releases_evidence_but_keeps_bounded_context_and_lifetime_loss_counts():
    store = ObserverStore(max_events=1)
    store.set_context(federation="F")
    for _ in range(4): store.publish({"event_type": "callback.invoked"})
    store.clear()
    state = StateResponse.model_validate(store.state())
    assert state.retention.dropped_events == 3 and state.retention.next_sequence == 5
    assert state.storage.evidence.accounted_bytes == 0 and state.storage.context.accounted_bytes > 0
    assert all(row.dropped_records == 0 for row in state.storage.evidence.collections)


def test_metadata_admission_is_atomic_and_context_loss_is_explicit():
    store = ObserverStore()
    before = store.capabilities()
    with pytest.raises(ValueError, match="128 KiB"):
        store.set_capabilities({"objects": {"state": "available", "description": "x" * (128 * 1024)}})
    assert store.capabilities() == before
    with pytest.raises(ValueError): store.set_status("x" * 257)
    store.set_context(federation="earlier")
    with pytest.raises(ValueError): store.set_context(federation="x" * (128 * 1024))
    assert store.state()["context"]["federation"] == "earlier"
    store.publish({"event_type": "callback.invoked", "federation": "x" * (128 * 1024)})
    state = StateResponse.model_validate(store.state())
    assert "federation" not in state.context and state.storage.context.collections[0].oversized_drops == 1
    assert len(json.dumps(store.state()).encode()) < 8 * 1024 * 1024


def test_public_json_results_cannot_mutate_retained_evidence_or_bypass_accounting():
    store = ObserverStore()
    context = store.set_context(notes={"items": ["original"]})
    returned = store.publish({"event_type": "callback.invoked", "details": {"items": ["original"]}})
    returned["details"]["items"].append("modified")
    context["notes"]["items"].append("modified")
    for result in (store.events()[0], store.event_page()["events"][0], store.wait_for_events()[0]):
        result["details"]["items"].append("modified")
    state = StateResponse.model_validate(store.state())
    assert state.context["notes"] == {"items": ["original"]}
    assert state.normalized_events[0].details == {"items": ["original"]}


def test_waiter_wakes_on_empty_lost_window_without_waiting_for_a_retained_event():
    store = ObserverStore(max_evidence_bytes=10000, max_evidence_record_bytes=1000)
    with ThreadPoolExecutor(max_workers=1) as executor:
        waiter = executor.submit(store.wait_for_events, timeout=5)
        store.publish({"event_type": "callback.invoked", "details": {"oversize": "x" * 2000}})
        assert waiter.result(timeout=1) == []


def test_default_snapshot_remains_bounded_with_large_values_context_and_adapter_metadata():
    store = ObserverStore()
    for index in range(10):
        store.publish({"event_type": "object.updated", "object_key": str(index), "federate": "A",
                       "attributes": {"value": "x" * 400000}})
    store.set_context(one="x" * 100000, two="y" * 100000)

    class Adapter:
        capabilities = {"probe-" + "x" * 32000: "available"}
        started = False
        def __init__(self, index): self.name = f"adapter-{index}"
        def start(self, sink): self.started = True
        def stop(self): pass

    attached = []
    for index in range(6):
        adapter = Adapter(index)
        try: store.attach_adapter(adapter)
        except ValueError as error:
            assert "adapter metadata" in str(error) and not adapter.started
            break
        attached.append(adapter)
    else: pytest.fail("adapter metadata admission did not bound repeated feature names")
    assert attached
    state = StateResponse.model_validate(store.state())
    assert state.storage.evidence.accounted_bytes <= 6 * 1024 * 1024
    assert len(state.model_dump_json(exclude_unset=True).encode()) < 8 * 1024 * 1024
    store.close()
