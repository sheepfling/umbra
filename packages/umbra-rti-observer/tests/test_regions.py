"""Internal explicit-region observer contract, not normative RTI DDM evidence."""

from __future__ import annotations

import json
from typing import TypedDict, cast

import pytest
from fastapi.testclient import TestClient
from jsonschema import Draft202012Validator
from jsonschema.exceptions import ValidationError as SchemaError
from pydantic import ValidationError
from umbra_rti_observer.conformance_schema import wire_schema_text
from umbra_rti_observer.core import ObserverStore
from umbra_rti_observer.schemas import EventInput, RegionsResponse, StateResponse
from umbra_rti_observer.server import create_app


class DimensionRange(TypedDict):
    dimension_id: str
    lower_bound: str
    upper_bound: str
    dimension_upper_bound: str


class RegionObservation(TypedDict, total=False):
    kind: str
    session_id: str
    identity_namespace: str
    federation_id: str
    status: str
    evidence: str
    region_id: str
    extents: object
    association_id: str
    target_kind: str
    target_id: str
    direction: str
    region_ids: object
    attribute_keys: object
    routing_id: str
    arbitrary: object


class InvalidRegionFields(TypedDict, total=False):
    extents: object
    evidence: object
    status: object
    arbitrary: object


class RegionEvent(TypedDict):
    event_type: str
    source: str
    region_observation: RegionObservation


def dimension_range() -> DimensionRange:
    return {
        "dimension_id": "D",
        "lower_bound": "9007199254740993",
        "upper_bound": "9007199254741093",
        "dimension_upper_bound": "18446744073709551615",
    }


def event(kind: str = "region", **fields: object) -> RegionEvent:
    data: RegionObservation = {
        "kind": kind,
        "session_id": "s",
        "identity_namespace": "ns",
        "federation_id": "fed",
        "status": "active",
        "evidence": "declared",
    }
    if kind == "region":
        data.update(
            region_id="R",
            extents=[{"extent_id": "E", "dimensions": [dimension_range()]}],
        )
    elif kind == "association":
        data.update(
            association_id="A",
            target_kind="object",
            target_id="instance-1",
            direction="update",
            region_ids=["R"],
            attribute_keys=["value"],
        )
    else:
        data.update(routing_id="routing", region_ids=["R"], evidence="observed")
    observation = cast(RegionObservation, {**data, **fields})
    return {
        "event_type": "region.observed",
        "source": "adapter",
        "region_observation": observation,
    }


def inspect(store: ObserverStore) -> RegionsResponse:
    result = StateResponse.model_validate(store.state()).inspectors.regions
    assert result is not None
    return result


def test_exact_large_ranges_and_associations_do_not_infer_routing() -> None:
    store = ObserverStore(max_events=1)
    store.publish(event())
    store.publish(event("association"))
    assert len(inspect(store).snapshots) == 2
    assert all(row.kind != "routing" for row in inspect(store).snapshots)
    region = inspect(store).snapshots[0]
    assert (
        region.kind == "region"
        and region.extents[0].dimensions[0].lower_bound
        == dimension_range()["lower_bound"]
    )
    store.publish(event("routing"))
    assert inspect(store).snapshots[-1].evidence == "observed"


def test_replacement_deletion_and_unknown_empty_extents_and_associations_stay_distinct() -> (
    None
):
    store = ObserverStore()
    store.publish(event())
    store.publish(event(extents=[]))
    row = inspect(store).snapshots[0]
    assert row.extents is not None and not row.extents
    store.publish(event(status="deleted", extents=None))
    row = inspect(store).snapshots[0]
    assert row.extents is None and row.status == "deleted"
    store.publish(event("association", region_ids=[], attribute_keys=[]))
    row = inspect(store).snapshots[-1]
    assert row.region_ids is not None and not row.region_ids and not row.attribute_keys
    store.publish(event("association", region_ids=None, attribute_keys=None))
    row = inspect(store).snapshots[-1]
    assert row.region_ids is None and row.attribute_keys is None


@pytest.mark.parametrize(
    "field", ["session_id", "identity_namespace", "federation_id", "region_id"]
)
def test_reporter_scopes_and_instance_ids_keep_regions_separate(field: str) -> None:
    store = ObserverStore()
    store.publish(event())
    store.publish(event(**{field: "different"}))
    store.publish({**event(), "source": "other-reporter"})
    assert len(inspect(store).snapshots) == 3


def test_capacities_and_history_expire_independently_and_clear_resets_them() -> None:
    store = ObserverStore(max_regions=2, max_region_transitions=3)
    for identity in ("one", "two", "one", "three"):
        store.publish(event(region_id=identity))
    result = inspect(store)
    assert {row.region_id for row in result.snapshots} == {"one", "three"}
    assert result.dropped_snapshots == result.dropped_transitions == 1
    store.clear()
    result = inspect(store)
    assert (
        not result.snapshots and not result.transitions and not result.dropped_snapshots
    )


@pytest.mark.parametrize(
    "value",
    [1, True, 1.5, "-1", "01", "+1", "1e3", "١", "1\n", " ", "9" * 129],
)
def test_bounds_reject_coercion_noncanonical_or_oversized_values_atomically(
    value: object,
) -> None:
    store = ObserverStore()
    invalid = event(
        extents=[
            {
                "extent_id": "E",
                "dimensions": [{**dimension_range(), "lower_bound": value}],
            }
        ]
    )
    with pytest.raises(ValueError):
        store.publish_many([{"event_type": "runtime.started"}, invalid])
    with pytest.raises(ValidationError):
        EventInput.model_validate(invalid)
    assert not store.events()


@pytest.mark.parametrize(
    "invalid",
    [
        {
            "extents": [
                {
                    "extent_id": "E",
                    "dimensions": [
                        {**dimension_range(), "lower_bound": "5", "upper_bound": "4"}
                    ],
                }
            ]
        },
        {
            "extents": [
                {
                    "extent_id": "E",
                    "dimensions": [{**dimension_range(), "dimension_upper_bound": "4"}],
                }
            ]
        },
        {
            "extents": [
                {
                    "extent_id": "E",
                    "dimensions": [dimension_range(), dimension_range()],
                }
            ]
        },
        {"extents": [{"extent_id": "E", "dimensions": []}] * 2},
        {"evidence": None},
        {"status": "guessed"},
        {"arbitrary": True},
        {"extents": "unknown"},
    ],
)
def test_invalid_extent_structures_and_evidence_reject_atomic_batches(
    invalid: InvalidRegionFields,
) -> None:
    store = ObserverStore()
    with pytest.raises((ValueError, TypeError)):
        store.publish_many([{"event_type": "runtime.started"}, event(**invalid)])
    with pytest.raises(ValidationError):
        EventInput.model_validate(event(**invalid))
    assert not store.events()


def test_interaction_associations_cannot_claim_object_attribute_selections() -> None:
    with pytest.raises(ValueError):
        ObserverStore().publish(
            event("association", target_kind="interaction", attribute_keys=[])
        )
    with pytest.raises(ValueError):
        ObserverStore().publish(event("association", region_ids=["R", "R"]))


@pytest.mark.parametrize("field", ["max_regions", "max_region_transitions"])
@pytest.mark.parametrize("value", [0, -1, True, 1.5])
def test_capacities_are_positive_integers(field: str, value: object) -> None:
    with pytest.raises(ValueError):
        ObserverStore(**{field: value})


def test_http_stream_preserve_exact_bounds_and_named_region_reports() -> None:
    store = ObserverStore()
    with TestClient(create_app(store, enable_analytics=False)) as client:
        assert (
            client.post(
                "/api/events",
                json={"events": [event(), event("association"), event("routing")]},
            ).status_code
            == 200
        )
        payload = client.get("/api/state?include_events=false").json()
        assert (
            payload["inspectors"]["regions"]["snapshots"][0]["extents"][0][
                "dimensions"
            ][0]["lower_bound"]
            == "9007199254740993"
        )
        with client.websocket_connect("/ws/events") as socket:
            assert (
                socket.receive_json()["state"]["inspectors"]["regions"]
                == payload["inspectors"]["regions"]
            )


def test_portable_schema_rejects_noncanonical_bounds_including_a_final_newline() -> (
    None
):
    schema = json.loads(wire_schema_text())
    validator = Draft202012Validator(
        {**schema, "$ref": "#/$defs/DimensionRangeResponse"}
    )
    validator.validate(dimension_range())
    for value in ("1\n", "01", "١", 1):
        with pytest.raises(SchemaError):
            validator.validate({**dimension_range(), "lower_bound": value})
