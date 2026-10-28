from __future__ import annotations

from unittest.mock import patch

import pytest

pytest.importorskip("pydantic")

from umbra_rti_observer import ObserverStore
from umbra_rti_observer.analytics import build_analytics
from umbra_rti_observer.schemas import AnalyticsWindow, StateResponse


def test_window_scope_buckets_and_explicit_participant_attribution() -> None:
    store = ObserverStore()
    with patch("umbra_rti_observer.core.time.time", return_value=100):
        store.publish({"event_type": "federate.joined", "federate": "Alpha"})
        store.publish({"event_type": "federate.joined", "federate": "Idle"})
    with patch("umbra_rti_observer.core.time.time", return_value=175):
        store.publish(
            {
                "event_type": "object.updated",
                "federate": {"name": "Alpha"},
                "object_key": "o1",
            }
        )
    with patch("umbra_rti_observer.core.time.time", return_value=180):
        store.publish({"event_type": "interaction.received", "participant": "Alpha"})
        store.publish(
            {
                "event_type": "callback.invoked",
                "source": "Alpha",
                "details": {"logical_time": 4.5},
            }
        )
    with patch("umbra_rti_observer.core.time.time", return_value=200):
        store.publish({"event_type": "runtime.error", "federate": "Alpha"})
    with patch("umbra_rti_observer.core.time.time", return_value=201):
        store.publish(
            {
                "event_type": "object.updated",
                "federate": "Alpha",
                "object_key": "future",
            }
        )

    snapshot = StateResponse.model_validate(store.state())
    result = build_analytics(snapshot, now=200, window_seconds=30, bucket_count=10)
    assert result.events_in_window == 4
    assert (
        result.unattributed_events == 1
    )  # A source name is never participant identity.
    assert len(result.activity) == 10
    assert sum(row.events for row in result.activity) == 4
    assert (
        result.activity[-1].errors == 1
    )  # Inclusive right edge stays in final bucket.
    assert len(result.logical_times) == 1
    assert result.logical_times[0].federate is None
    alpha, idle = result.participants
    assert alpha.event_count == 3
    assert alpha.object_keys == ("o1",)
    assert alpha.latest_logical_time is None
    assert idle.event_count == 0  # Roster lives beyond the event window.
    scoped = build_analytics(snapshot, now=200, window_seconds=30, federate="Alpha")
    assert scoped.events_in_window == 3
    assert scoped.unattributed_events == 0
    assert [row.federate for row in scoped.participants] == ["Alpha"]


def test_retention_loss_and_clear_are_visible_in_analytics() -> None:
    store = ObserverStore(max_events=2)
    with patch("umbra_rti_observer.core.time.time", return_value=100):
        store.publish({"event_type": "federate.joined", "federate": "Old"})
        store.publish_many([{"event_type": "callback.invoked"}] * 3)
    result = build_analytics(StateResponse.model_validate(store.state()), now=100)
    assert result.history_truncated
    assert result.events_in_window == 2
    assert result.participants[0].federate == "Old"
    assert result.participants[0].event_count == 0
    store.clear()
    cleared = build_analytics(StateResponse.model_validate(store.state()), now=100)
    assert cleared.history_truncated
    assert cleared.events_in_window == 0
    assert not cleared.participants


def test_logical_timestamps_keep_zero_and_regressions_without_inferring_grants() -> (
    None
):
    store = ObserverStore()
    with patch("umbra_rti_observer.core.time.time", return_value=100):
        for value in (7.0, 0.0):
            store.publish(
                {
                    "event_type": "callback.invoked",
                    "federate": "A",
                    "logical_time": value,
                }
            )
        store.publish(
            {
                "event_type": "callback.invoked",
                "federate": "A",
                "details": {"logical_time": True},
            }
        )
        store.publish(
            {
                "event_type": "callback.invoked",
                "federate": "A",
                "details": {"logical_time": "42"},
            }
        )
    result = build_analytics(StateResponse.model_validate(store.state()), now=100)
    assert [row.logical_time for row in result.logical_times] == [7.0, 0.0]
    assert result.participants[0].latest_logical_time == 0.0
    assert all(row.operation is None for row in result.logical_times)


@pytest.mark.parametrize("window,buckets", [(0, 30), (3601, 30), (300, 9), (300, 121)])
def test_analytics_rejects_unbounded_queries(window: int, buckets: int) -> None:
    with pytest.raises(ValueError):
        build_analytics(
            StateResponse.model_validate(ObserverStore().state()),
            window_seconds=window,
            bucket_count=buckets,
        )


@pytest.mark.parametrize(
    "window,buckets,federate",
    [(30, 10, None), (300, 30, "Alpha"), (3600, 120, "Idle"), (300, 30, "Unknown")],
)
def test_projection_exactly_preserves_analytics_windows_attribution_and_idle_roster(
    window: int, buckets: int, federate: str | None
) -> None:
    store = ObserverStore(max_events=3)
    with patch("umbra_rti_observer.core.time.time", return_value=100):
        store.publish({"event_type": "federate.joined", "federate": "Idle"})
    with patch("umbra_rti_observer.core.time.time", return_value=200):
        store.publish_many(
            [
                {
                    "event_type": "object.updated",
                    "federate": "Alpha",
                    "object_key": "one",
                    "attributes": {"value": 0},
                },
                {
                    "event_type": "callback.invoked",
                    "source": "Alpha",
                    "logical_time": 0,
                },
                {"event_type": "runtime.error", "federate": "Alpha"},
            ]
        )
    full = build_analytics(
        StateResponse.model_validate(store.state()),
        now=200,
        window_seconds=window,
        bucket_count=buckets,
        federate=federate,
    )
    projected = build_analytics(
        AnalyticsWindow.model_validate(store.analytics_window()),
        now=200,
        window_seconds=window,
        bucket_count=buckets,
        federate=federate,
    )
    assert projected == full
    store.clear()
    assert build_analytics(
        AnalyticsWindow.model_validate(store.analytics_window()), now=200
    ) == build_analytics(StateResponse.model_validate(store.state()), now=200)


def test_projection_does_not_construct_or_expose_unrelated_inspectors_and_returns_detached_values() -> (
    None
):
    store = ObserverStore()
    store.publish(
        {
            "event_type": "object.updated",
            "object_key": "one",
            "attributes": {"nested": {"value": 7}},
        }
    )
    with patch.object(
        store,
        "_state_locked",
        side_effect=AssertionError(
            "full inspector walk is forbidden in the projection"
        ),
    ):
        window = store.analytics_window()
        assert "inspectors" not in window and "context" not in window
        AnalyticsWindow.model_validate(window)
    window["normalized_events"][0]["attributes"]["nested"]["value"] = "mutated"
    assert store.events()[0]["attributes"]["nested"]["value"] == 7


@pytest.mark.parametrize("fault", ["count", "duplicate", "next", "nan", "roster"])
def test_analytics_window_rejects_invalid_or_coerced_views(fault: str) -> None:
    store = ObserverStore()
    store.publish(
        {"event_type": "object.updated", "federate": "A", "attributes": {"value": 7}}
    )
    data = store.analytics_window()
    if fault == "count":
        data["retention"]["retained_events"] = True
    if fault == "duplicate":
        data["normalized_events"].append(data["normalized_events"][0])
    if fault == "next":
        data["retention"]["next_sequence"] = 1
    if fault == "nan":
        data["normalized_events"][0]["details"] = {"invalid": float("nan")}
    if fault == "roster":
        data["federate_roster"].append(data["federate_roster"][0])
    with pytest.raises(ValueError):
        AnalyticsWindow.model_validate(data)
