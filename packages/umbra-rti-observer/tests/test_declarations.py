"""Internal observer contract behavior, not normative HLA declaration evidence."""

from __future__ import annotations

import pytest
from umbra_rti_observer.core import ObserverStore
from umbra_rti_observer.models import JsonObject, JsonValue
from umbra_rti_observer.schemas import DeclarationsResponse, StateResponse


def inspect(store: ObserverStore) -> DeclarationsResponse:
    result = StateResponse.model_validate(store.state()).inspectors.declarations
    assert result is not None
    return result


def declare(
    store: ObserverStore,
    *,
    source: str = "test",
    federate: str | None = "Alice",
    federation: str | None = "Federation",
    **declaration: JsonValue,
) -> None:
    fields: JsonObject = {
        "kind": "object",
        "class_key": "Class",
        "status": "declared",
        "direction": "publication",
        "session_id": "session-1",
        **declaration,
    }
    store.publish(
        {
            "event_type": "declaration.observed",
            "source": source,
            "federate": federate,
            "federation": federation,
            "declaration": fields,
        }
    )


def test_persistent_snapshots_replace_attribute_sets_without_inferred_declarations() -> (
    None
):
    store = ObserverStore(max_events=1)
    store.publish(
        {
            "event_type": "object.updated",
            "object_key": "one",
            "class_name": "Class",
            "federate": "Alice",
        }
    )
    assert not inspect(store).snapshots
    declare(store, attribute_scope="selected", attributes=["A", "B"])
    declare(store, attribute_scope="selected", attributes=["B"])
    store.publish({"event_type": "callback.invoked"})
    rows = inspect(store).snapshots
    assert len(rows) == 1 and list(rows[0].attributes) == ["B"]
    assert (
        rows[0].sequence == 3
        and store.state()["retention"]["first_retained_sequence"] == 4
    )
    declare(store, status="unknown")
    assert inspect(store).snapshots[0].attributes is None
    assert inspect(store).snapshots[0].status == "unknown"


@pytest.mark.parametrize(
    "identity",
    [
        "source",
        "federate",
        "federation",
        "session_id",
        "kind",
        "direction",
        "class_key",
    ],
)
def test_scope_kind_and_direction_do_not_merge_unrelated_declarations(
    identity: str,
) -> None:
    store = ObserverStore()
    declare(store)
    other = (
        "interaction"
        if identity == "kind"
        else "subscription"
        if identity == "direction"
        else "different"
    )
    declare(store, **{identity: other})
    assert len(inspect(store).snapshots) == 2


def test_empty_unknown_all_and_interaction_attributes_have_distinct_meanings() -> None:
    store = ObserverStore()
    declare(store, class_key="Empty", attribute_scope="selected", attributes=[])
    declare(store, class_key="Unknown")
    declare(store, class_key="All", attribute_scope="all")
    declare(store, class_key="Interaction", kind="interaction")
    rows = {row.class_key: row for row in inspect(store).snapshots}
    assert rows["Empty"].attributes is not None and not rows["Empty"].attributes
    assert (
        rows["Unknown"].attributes is None
        and rows["Unknown"].attribute_scope == "unknown"
    )
    assert rows["All"].attribute_scope == "all" and rows["All"].attributes is None
    assert rows["Interaction"].attribute_scope == "not-applicable"


def test_capacity_evicts_least_recently_updated_record_and_clear_resets_coverage() -> (
    None
):
    store = ObserverStore(max_declarations=2)
    declare(store, class_key="A")
    declare(store, class_key="B")
    declare(store, class_key="A", status="withdrawn")
    declare(store, class_key="C")
    result = inspect(store)
    assert {row.class_key for row in result.snapshots} == {"A", "C"}
    assert result.dropped_declarations == 1
    assert (
        next(row for row in result.snapshots if row.class_key == "A").status
        == "withdrawn"
    )
    store.clear()
    assert not inspect(store).snapshots and inspect(store).dropped_declarations == 0


def test_opaque_identifier_text_is_not_trimmed_and_returned_state_does_not_mutate_store() -> (
    None
):
    store = ObserverStore()
    declare(
        store,
        class_key=" Class ",
        session_id=" opaque ",
        attribute_scope="selected",
        attributes=[" A "],
    )
    row = inspect(store).snapshots[0]
    assert (
        row.class_key == " Class "
        and row.scope.session_id == " opaque "
        and list(row.attributes) == [" A "]
    )
    state = store.state()
    state["inspectors"]["declarations"]["snapshots"].clear()
    assert len(inspect(store).snapshots) == 1


@pytest.mark.parametrize(
    "invalid",
    [
        {"attribute_scope": "selected"},
        {"attribute_scope": "selected", "attributes": ["A", "A"]},
        {"attribute_scope": "all", "attributes": []},
        {"attribute_scope": "not-applicable"},
        {"attributes": ["A"]},
        {"attribute_scope": "selected", "attributes": [1]},
        {"attribute_scope": "selected", "attributes": [" "]},
        {"class_key": True},
        {"kind": "interaction", "attributes": []},
        {"status": None},
        {"direction": "guess"},
        {"vendor_guess": True},
        {
            "attribute_scope": "selected",
            "attributes": [str(index) for index in range(4097)],
        },
    ],
)
def test_invalid_declarations_reject_atomic_batches(invalid: JsonObject) -> None:
    store = ObserverStore()
    with pytest.raises(ValueError):
        store.publish_many(
            [
                {"event_type": "runtime.started"},
                {
                    "declaration": {
                        "kind": "object",
                        "direction": "publication",
                        "class_key": "Class",
                        "status": "declared",
                        **invalid,
                    }
                },
            ]
        )
    assert not store.events() and not inspect(store).snapshots


@pytest.mark.parametrize("capacity", [0, -1, True, 1.5])
def test_capacity_is_a_positive_integer(capacity: object) -> None:
    with pytest.raises(ValueError):
        ObserverStore(max_declarations=capacity)
