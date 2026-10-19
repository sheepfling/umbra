"""Internal observer application behavior; not normative RTI validation."""
from __future__ import annotations

import pytest
from umbra_rti_observer.core import ObserverStore
from umbra_rti_observer.models import JsonObject
from umbra_rti_observer.schemas import ObjectObservationsResponse, StateResponse


def observations(store: ObserverStore) -> ObjectObservationsResponse:
    result = StateResponse.model_validate(store.state()).inspectors.object_observations
    assert result is not None
    return result


def update(store: ObserverStore, federate: str | None, **attributes: object) -> JsonObject:
    return store.publish({"event_type": "object.updated", "source": "test", "object_key": "shared",
                          "federate": federate, "attributes": attributes})


def test_independent_snapshot_budget_uses_recently_observed_scopes_and_discloses_omissions() -> None:
    store = ObserverStore(max_events=10, max_object_perspectives=2)
    for key in ("a", "b", "a", "c"):
        store.publish({"event_type": "object.updated", "object_key": key, "attributes": {"value": key}})
    data = observations(store)
    assert {row.object.object_key for row in data.snapshots} == {"a", "c"}
    assert data.snapshot_capacity == 2 and data.dropped_snapshots == 1 and data.omitted_changes == 1
    assert data.history_truncated and store.state()["retention"]["dropped_events"] == 0
    assert {row.object_key for row in data.changes} == {"a", "c"}
    assert len(store.events()) == 4 and len(store.state()["inspectors"]["objects"]) == 3


def test_reobserved_perspective_gets_a_fresh_comparison_not_another_federates_values() -> None:
    store = ObserverStore(max_object_perspectives=2)
    update(store, "A", a_only=1)
    update(store, "B", b_only=7)
    update(store, "A", next_value=9)
    data = observations(store)
    a = next(row for row in data.snapshots if row.federate == "A")
    assert a.generation == 3 and a.object.attributes == {"next_value": 9}
    current_changes = [row for row in data.changes if row.federate == "A" and row.generation == a.generation]
    assert len(current_changes) == 1 and not current_changes[0].previous_known and current_changes[0].previous_value is None
    store.clear()
    assert observations(store).dropped_snapshots == observations(store).omitted_changes == 0


@pytest.mark.parametrize("value", [0, -1, True, 1.5, "2"])
def test_snapshot_capacity_is_a_positive_integer(value: object) -> None:
    with pytest.raises(ValueError): ObserverStore(max_object_perspectives=value)


def test_older_object_budget_is_unknown_not_reported_as_zero_loss() -> None:
    raw = observations(ObserverStore()).model_dump(mode="json")
    for name in ("snapshot_capacity", "dropped_snapshots", "omitted_changes"): raw.pop(name)
    older = ObjectObservationsResponse.model_validate(raw)
    assert older.snapshot_capacity is older.dropped_snapshots is older.omitted_changes is None


@pytest.mark.parametrize("fields", [{"snapshot_capacity": None}, {"snapshot_capacity": 0}, {"dropped_snapshots": True}, {"omitted_changes": -1}, {"omitted_changes": 1, "history_truncated": False}])
def test_snapshot_budget_metadata_cannot_be_incomplete_or_coerced(fields: dict[str, object]) -> None:
    raw = observations(ObserverStore()).model_dump(mode="json")
    with pytest.raises(ValueError): ObjectObservationsResponse.model_validate({**raw, **fields})


def test_perspectives_survive_eviction_without_mixing_global_or_other_federate():
    store = ObserverStore(max_events=2)
    update(store, "Alice", value=10, only_alice=True)
    update(store, "Bob", value=20, only_bob=True)
    update(store, None, value=30)
    store.publish({"event_type": "callback.invoked"})
    result = observations(store)
    rows = {row.federate: row.object for row in result.snapshots}
    assert rows[None].attributes == {"value": 30, "only_alice": True, "only_bob": True}
    assert rows["Alice"].attributes == {"value": 10, "only_alice": True}
    assert rows["Bob"].attributes == {"value": 20, "only_bob": True}
    assert result.history_truncated
    assert result.history_first_sequence == 3
    assert [(row.sequence, row.federate) for row in result.changes] == [(3, None)]
    assert result.changes[0].previous_value == 20
    assert result.changes[0].previous_known


def test_comparisons_are_scoped_and_distinguish_null_from_missing():
    store = ObserverStore()
    update(store, "Alice", value=None)
    update(store, "Bob", value=9)
    update(store, "Alice", value=2)
    result = observations(store)
    latest = [row for row in result.changes if row.sequence == 3]
    assert [(row.federate, row.previous_known, row.previous_value) for row in latest] == [
        (None, True, 9), ("Alice", True, None)]
    assert not result.changes[0].previous_known
    assert not next(row for row in result.changes if row.federate == "Bob").previous_known


def test_partial_updates_copy_values_and_keep_discovery_metadata():
    store = ObserverStore()
    original = {"nested": [1, 2]}
    store.publish({"event_type": "object.discovered", "source": "test", "object_key": "shared", "federate": {"name": "Alice"},
                   "object_name": "Shared sensor", "class_name": "Sensor", "attributes": {"data": original}})
    original["nested"].append(3)
    returned = update(store, "Alice", raw=b"\x00\xff", temperature=2.5)
    returned["attributes"]["raw"]["value"] = "bad"
    result = observations(store)
    alice = next(row for row in result.snapshots if row.federate == "Alice")
    assert alice.object.object_name == "Shared sensor"
    assert alice.object.class_name == "Sensor"
    assert alice.object.attributes == {"data": {"nested": [1, 2]}, "raw": {"encoding": "hex", "value": "00ff"}, "temperature": 2.5}
    assert alice.object.discovery_count == 1
    assert alice.object.update_count == 1
    # Mutating a returned snapshot must not mutate the store or comparison baseline.
    raw = store.state()
    raw["inspectors"]["object_observations"]["snapshots"].clear()
    assert len(observations(store).snapshots) == 2


def test_removal_is_attributed_and_rediscovery_starts_a_new_generation():
    store = ObserverStore()
    update(store, "Alice", old_value=1)
    update(store, "Bob", old_value=2)
    store.publish({"event_type": "object.removed", "source": "test", "object_key": "shared", "federate": "Alice"})
    rows = {row.federate: row for row in observations(store).snapshots}
    assert rows["Alice"].object.status == "removed"
    assert rows["Bob"].object.status == "active"
    store.publish({"event_type": "object.discovered", "source": "test", "object_key": "shared", "federate": "Alice", "attributes": {"new_value": 3}})
    rows = {row.federate: row for row in observations(store).snapshots}
    assert rows["Alice"].generation == 4
    assert rows[None].generation == 4
    assert rows["Alice"].object.attributes == {"new_value": 3}
    assert rows["Bob"].object.attributes == {"old_value": 2}
    assert not observations(store).changes[-1].previous_known
    assert StateResponse.model_validate(store.state()).inspectors.objects[0].attributes == {"new_value": 3}


def test_atomic_batch_clear_and_empty_history():
    store = ObserverStore(max_events=1)
    with pytest.raises(ValueError):
        store.publish_many([{"event_type": "object.updated", "object_key": "shared", "attributes": {"x": 1}},
                            {"event_type": "x" * 257}])
    assert not observations(store).snapshots
    update(store, "Alice", value=1)
    store.clear()
    result = observations(store)
    assert not result.snapshots and not result.changes
    assert result.history_first_sequence == result.history_last_sequence == 0
    assert result.history_truncated
    update(store, "Alice", value=2)
    result = observations(store)
    assert result.snapshots[0].generation == 2
    assert not result.changes[0].previous_known


def test_legacy_snapshot_can_omit_optional_perspectives():
    raw = ObserverStore().state()
    del raw["inspectors"]["object_observations"]
    del raw["capabilities"]["object_observations"]
    assert StateResponse.model_validate(raw).inspectors.object_observations is None


def test_exclude_none_preserves_required_global_scope_and_reported_nulls():
    store = ObserverStore()
    update(store, None, value=None)
    state = StateResponse.model_validate(store.state())
    payload = state.model_dump(mode="json", exclude_none=True)
    change = payload["inspectors"]["object_observations"]["changes"][0]
    assert change["federate"] is None
    assert change["value"] is None
    assert change["previous_value"] is None
    assert StateResponse.model_validate(payload).inspectors.object_observations is not None
    schema = StateResponse.model_json_schema(mode="serialization")
    assert "previous_value" in schema["$defs"]["ObjectAttributeChangeResponse"]["required"]
    assert "federate" in schema["$defs"]["ObjectPerspectiveResponse"]["required"]
