from __future__ import annotations

import threading
from collections.abc import Sequence
from typing import Protocol

import pytest
from umbra_rti_observer import ShowcaseAdapter
from umbra_rti_observer.__main__ import main


class ShowcaseEvent(Protocol):
    def __getitem__(self, key: str) -> object: ...

    def get(self, key: str, default: object | None = None) -> object | None: ...


@pytest.mark.parametrize(
    "arguments",
    [
        ["--showcase-control-fault-once", "connect"],
        ["--showcase-control-inspection", "partial"],
    ],
)
def test_recovery_demo_options_require_explicit_synthetic_controls(
    arguments: Sequence[str],
) -> None:
    with pytest.raises(SystemExit, match="requires --showcase --enable-controls"):
        main(arguments)


def test_showcase_profiles_report_different_feature_surfaces() -> None:
    rich = ShowcaseAdapter(profile="umbra")
    limited = ShowcaseAdapter(profile="limited")

    assert rich.capabilities["mom"]["state"] == "available"
    assert rich.capabilities["ownership"]["state"] == "available"
    assert limited.capabilities["mom"]["state"] == "unsupported"
    assert limited.capabilities["persistence"]["state"] == "disabled"


def test_showcase_adapter_emits_live_events_and_stops() -> None:
    received: list[ShowcaseEvent] = []
    started = threading.Event()

    def sink(event: ShowcaseEvent) -> None:
        received.append(event)
        if event.get("event_type") == "runtime.started":
            started.set()

    adapter = ShowcaseAdapter(profile="limited", interval=0.05)
    adapter.start(sink)
    try:
        assert started.wait(1.0)
    finally:
        adapter.stop()

    assert received
    assert received[0]["source"] == "showcase-limited"
    assert any(event["event_type"] == "object.discovered" for event in received)
    assert not adapter.running


def test_rich_showcase_includes_management_callbacks() -> None:
    received: list[ShowcaseEvent] = []
    adapter = ShowcaseAdapter(profile="umbra", interval=0.05)
    adapter.start(received.append)
    try:
        assert any(
            event.get("operation") == "reportFederationExecutionMembers"
            for event in received
        ) or _wait_for_operation(received, "reportFederationExecutionMembers")
    finally:
        adapter.stop()

    assert any(event.get("operation") == "federationSynchronized" for event in received)


def _wait_for_operation(events: Sequence[ShowcaseEvent], operation: str) -> bool:
    for _ in range(40):
        if any(event.get("operation") == operation for event in events):
            return True
        threading.Event().wait(0.01)
    return False


def test_showcase_rejects_unknown_profile() -> None:
    with pytest.raises(ValueError, match="showcase profile"):
        ShowcaseAdapter(profile="unknown")
