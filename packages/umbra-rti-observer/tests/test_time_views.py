"""Internal observer evidence, not normative RTI time-service conformance."""
from __future__ import annotations

import pytest

from umbra_rti_observer.core import ObserverStore
from umbra_rti_observer.schemas import StateResponse, TimeInspectionResponse


def inspect(store: ObserverStore) -> TimeInspectionResponse:
    result = StateResponse.model_validate(store.state()).inspectors.time
    assert result is not None
    return result


def observe(store: ObserverStore, kind: str, *, source: str = "test", session: str | None = "session-1",
            federate: str | None = "Alice", federation: str = "Federation", request_id: str | None = "request-1",
            logical_time: float = 10, **state: object) -> None:
    payload: dict[str, object] = {"kind": kind, "session_id": session}
    if kind in {"request", "grant"}:
        payload["logical_time"] = logical_time
    if kind != "state":
        payload["request_id"] = request_id
    payload.update(state)
    store.publish({"event_type": "time.observed", "source": source, "federate": federate,
                   "federation": federation, "time_observation": payload})


def test_persistent_grant_and_pending_survive_event_eviction_with_monotonic_ages(monkeypatch: pytest.MonkeyPatch) -> None:
    clock = [100.0]
    monkeypatch.setattr("umbra_rti_observer.time_views.monotonic", lambda: clock[0])
    store = ObserverStore(max_events=1)
    observe(store, "grant", request_id=None, logical_time=3)
    observe(store, "request")
    clock[0] = 102.5
    store.publish({"event_type": "object.updated", "object_key": "timestamped", "logical_time": 999})
    result = inspect(store)
    assert result.snapshots[0].last_grant.logical_time == 3
    assert result.requests[0].status == "pending"
    assert result.requests[0].pending_seconds == 2.5
    clock[0] = 103.0
    observe(store, "grant", logical_time=8)  # A match need not grant the requested target.
    result = inspect(store)
    assert result.requests[0].status == "granted"
    assert result.requests[0].latency_seconds == 3
    assert result.requests[0].pending_seconds is None
    assert result.requests[0].granted_time == 8


@pytest.mark.parametrize("different", ["source", "session", "federate", "federation", "request_id"])
def test_request_ids_never_correlate_across_scope_or_ids(different: str) -> None:
    store = ObserverStore()
    observe(store, "request")
    observe(store, "grant", **{different: "other"})
    assert inspect(store).requests[0].status == "pending"
    observe(store, "grant")
    assert inspect(store).requests[0].status == "granted"


@pytest.mark.parametrize("missing", ["request_id", "session", "federate", "source"])
def test_missing_identity_is_not_an_implied_pending_completion(missing: str) -> None:
    store = ObserverStore()
    value = "unknown" if missing == "source" else None
    observe(store, "request", **{missing: value})
    observe(store, "grant", **{missing: value})
    result = inspect(store)
    assert not result.requests
    assert result.snapshots[0].uncorrelated_requests == result.snapshots[0].uncorrelated_grants == 1
    assert result.snapshots[0].last_grant.logical_time == 10


def test_repeated_ids_are_ambiguous_and_grants_are_not_retroactively_matched() -> None:
    store = ObserverStore()
    observe(store, "grant")
    observe(store, "request")
    assert inspect(store).requests[0].status == "pending"
    observe(store, "request")
    observe(store, "grant")
    result = inspect(store)
    assert result.requests[0].status == "ambiguous"
    assert result.requests[0].latency_seconds is None
    assert result.snapshots[0].uncorrelated_grants == 2


def test_state_patches_preserve_false_zero_and_do_not_invent_grants_or_bounds() -> None:
    store = ObserverStore()
    observe(store, "state", regulating=False, constrained=True, lookahead=0, galt=5)
    observe(store, "state", constrained=False)
    row = inspect(store).snapshots[0]
    assert row.reported_state.regulating is False and row.reported_state.constrained is False
    assert row.reported_state.lookahead == 0 and row.reported_state.galt == 5
    assert row.reported_state.lbts is None and row.last_grant is None


@pytest.mark.parametrize("kind", ["failed", "cancelled"])
def test_explicit_outcomes_close_only_matching_requests_without_grant_latency(kind: str) -> None:
    store = ObserverStore()
    observe(store, "request")
    observe(store, kind, reason="Explicit provider outcome")
    row = inspect(store).requests[0]
    assert row.status == kind and row.reason == "Explicit provider outcome"
    assert row.latency_seconds is None and row.granted_time is None


def test_explicit_session_end_cancels_pending_and_fresh_session_is_independent() -> None:
    store = ObserverStore()
    observe(store, "request")
    store.publish({"event_type": "session.disconnected", "source": "test", "session_id": "session-1", "federation": "Federation"})
    observe(store, "grant")  # A late callback cannot resurrect an ended scope.
    assert inspect(store).requests[0].status == "session-ended"
    assert inspect(store).snapshots[0].status == "ended"
    observe(store, "request", session="session-2")
    assert len(inspect(store).requests) == 2
    assert inspect(store).requests[1].status == "pending"


def test_bounded_request_ledger_reports_eviction_and_clear_resets_all_time_state() -> None:
    store = ObserverStore(max_time_requests=1)
    observe(store, "request", request_id="old")
    observe(store, "request", request_id="new")
    observe(store, "grant", request_id="old")
    result = inspect(store)
    assert result.dropped_requests == 1 and result.request_capacity == 1
    assert result.requests[0].request_id == "new" and result.requests[0].status == "pending"
    store.clear()
    result = inspect(store)
    assert not result.snapshots and not result.requests and result.dropped_requests == 0


def test_legacy_exact_grant_is_persistent_but_payload_timestamps_are_not_grants() -> None:
    store = ObserverStore()
    store.publish({"event_type": "object.updated", "federate": "Alice", "logical_time": 999, "operation": "timeAdvanceGrant"})
    assert not inspect(store).snapshots
    store.publish({"event_type": "callback.invoked", "federate": "Alice", "operation": "timeAdvanceGrant", "details": {"logical_time": 0}})
    row = inspect(store).snapshots[0]
    assert row.last_grant.logical_time == 0 and row.scope.session_id is None
    assert not inspect(store).requests
    assert StateResponse.model_validate_json(StateResponse.model_validate(store.state()).model_dump_json(exclude_none=True))


@pytest.mark.parametrize("payload", [
    {"kind": "grant", "logical_time": True}, {"kind": "grant", "logical_time": "2"},
    {"kind": "grant", "logical_time": float("inf")}, {"kind": "grant", "logical_time": {"opaque": "unsupported"}},
    {"kind": "state", "regulating": 1}, {"kind": "state", "galt": float("nan")},
    {"kind": "request", "logical_time": 2, "request_id": " "},
    {"kind": "state", "unknown": 3}, {"kind": "unknown"}, [],
])
def test_invalid_time_observations_reject_an_entire_core_batch(payload: object) -> None:
    store = ObserverStore()
    with pytest.raises((ValueError, TypeError)):
        store.publish_many([{"event_type": "runtime.started"}, {"time_observation": payload}])
    assert not store.events() and not inspect(store).snapshots


def test_opaque_or_unreported_numeric_time_does_not_prevent_explicit_id_correlation() -> None:
    store = ObserverStore()
    for kind in ("request", "grant"):
        store.publish({"event_type": "time.observed", "source": "test", "federate": "Alice",
                       "time_observation": {"kind": kind, "session_id": "opaque-time-session", "request_id": "id"}})
    result = inspect(store)
    assert result.requests[0].status == "granted"
    assert result.requests[0].requested_time is None and result.requests[0].granted_time is None
    assert result.snapshots[0].last_grant is not None
    assert result.snapshots[0].last_grant.logical_time is None


def test_wall_clock_changes_do_not_change_monotonic_delay_and_backward_duration_is_unknown(monkeypatch: pytest.MonkeyPatch) -> None:
    clock = [10.0]
    monkeypatch.setattr("umbra_rti_observer.time_views.monotonic", lambda: clock[0])
    monkeypatch.setattr("time.time", lambda: 100.0)
    store = ObserverStore()
    observe(store, "request", request_id="wall-change")
    monkeypatch.setattr("time.time", lambda: 5.0)
    clock[0] = 12.0
    observe(store, "grant", request_id="wall-change")
    assert inspect(store).requests[0].latency_seconds == 2
    observe(store, "request", request_id="bad-clock")
    clock[0] = 11.0
    assert inspect(store).requests[1].pending_seconds is None
    observe(store, "grant", request_id="bad-clock")
    assert inspect(store).requests[1].latency_seconds is None


@pytest.mark.parametrize("capacity", [0, -1, True, 1.5])
def test_request_capacity_rejects_invalid_values(capacity: object) -> None:
    with pytest.raises(ValueError):
        ObserverStore(max_time_requests=capacity)
