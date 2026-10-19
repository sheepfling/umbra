from __future__ import annotations

import threading
import time
from collections.abc import Iterator

import pytest

pytest.importorskip("pydantic")

from umbra_rti_observer import ObserverStore
from umbra_rti_observer.command_models import (
    CommandRecord,
    ConnectCommand,
    DisconnectCommand,
    EmptyParameters,
    JoinCommand,
    JoinParameters,
    ResignCommand,
    ResignParameters,
)
from umbra_rti_observer.commands import CommandConflict, CommandDispatcher
from umbra_rti_observer.session_commands import SessionController
from umbra_rti_observer.showcase_commands import ShowcaseSessionDriver


def completed(dispatcher: CommandDispatcher, command_id: str) -> CommandRecord:
    deadline = time.monotonic() + 3
    while time.monotonic() < deadline:
        record = dispatcher.get(command_id)
        if record.status not in {"accepted", "running"}:
            return record
        threading.Event().wait(0.005)
    pytest.fail("command did not reach a terminal result")


@pytest.fixture
def dispatcher() -> Iterator[CommandDispatcher]:
    result = CommandDispatcher(timeout_seconds=2)
    result.register(
        SessionController(
            target_id="test",
            label="Test",
            adapter="fake",
            driver_factory=ShowcaseSessionDriver,
            federations=("Allowed",),
            synthetic=True,
        )
    )
    yield result
    result.close()


def connect(
    dispatcher: CommandDispatcher, request_id: str = "connect"
) -> CommandRecord:
    return dispatcher.submit(
        ConnectCommand(
            action="session.connect",
            target_id="test",
            request_id=request_id,
            parameters=EmptyParameters(),
        )
    )


def test_lifecycle_scope_confirmation_and_window_independent_history(
    dispatcher: CommandDispatcher,
) -> None:
    assert completed(dispatcher, connect(dispatcher).command_id).status == "succeeded"
    rejected = dispatcher.submit(
        JoinCommand(
            action="federate.join",
            target_id="test",
            request_id="outside",
            parameters=JoinParameters(federation="Other", federate_name="A"),
        )
    )
    assert completed(dispatcher, rejected.command_id).status == "unavailable"
    assert dispatcher.catalog().targets[0].state == "connected"
    joined = dispatcher.submit(
        JoinCommand(
            action="federate.join",
            target_id="test",
            request_id="join",
            parameters=JoinParameters(federation="Allowed", federate_name="A"),
        )
    )
    assert (
        completed(dispatcher, joined.command_id).outcome.federate_handle == "showcase:A"
    )
    assert dispatcher.catalog().targets[0].federate == "A"
    denied = dispatcher.submit(
        DisconnectCommand(
            action="session.disconnect",
            target_id="test",
            request_id="disconnect-joined",
            parameters=EmptyParameters(),
            confirmed=True,
        )
    )
    assert denied.status == "unavailable"
    unconfirmed = dispatcher.submit(
        ResignCommand(
            action="federate.resign",
            target_id="test",
            request_id="unconfirmed",
            parameters=ResignParameters(),
        )
    )
    assert unconfirmed.status == "unavailable"
    assert unconfirmed.error.effect == "none"
    resigned = dispatcher.submit(
        ResignCommand(
            action="federate.resign",
            target_id="test",
            request_id="resign",
            parameters=ResignParameters(),
            confirmed=True,
        )
    )
    assert completed(dispatcher, resigned.command_id).status == "succeeded"
    disconnected = dispatcher.submit(
        DisconnectCommand(
            action="session.disconnect",
            target_id="test",
            request_id="disconnect",
            parameters=EmptyParameters(),
            confirmed=True,
        )
    )
    assert completed(dispatcher, disconnected.command_id).status == "succeeded"
    assert dispatcher.catalog().targets[0].state == "disconnected"
    assert len(dispatcher.history().records) == 7


def test_idempotency_conflicts_and_unknown_targets(
    dispatcher: CommandDispatcher,
) -> None:
    first = connect(dispatcher)
    completed(dispatcher, first.command_id)
    assert connect(dispatcher).command_id == first.command_id
    with pytest.raises(CommandConflict, match="different command"):
        dispatcher.submit(
            ConnectCommand(
                action="session.connect",
                target_id="other",
                request_id="connect",
                parameters=EmptyParameters(),
            )
        )
    with pytest.raises(KeyError):
        dispatcher.submit(
            ConnectCommand(
                action="session.connect",
                target_id="other",
                request_id="new",
                parameters=EmptyParameters(),
            )
        )
    assert len(dispatcher.history().records) == 1


def test_timeout_keeps_session_busy_and_records_late_success() -> None:
    entered, release = threading.Event(), threading.Event()

    class BlockingDriver(ShowcaseSessionDriver):
        def connect(self) -> None:
            entered.set()
            release.wait(3)

    dispatcher = CommandDispatcher(timeout_seconds=0.05, history_limit=1)
    dispatcher.register(
        SessionController(
            target_id="test",
            label="Test",
            adapter="fake",
            driver_factory=BlockingDriver,
            federations=("Allowed",),
        )
    )
    try:
        record = connect(dispatcher)
        assert entered.wait(1)
        timed_out = completed(dispatcher, record.command_id)
        assert timed_out.status == "failed"
        assert timed_out.error.code == "timeout"
        assert timed_out.error.effect == "unknown"
        assert dispatcher.catalog().targets[0].busy
        assert dispatcher.catalog().targets[0].state == "unknown"
        with pytest.raises(CommandConflict, match="busy"):
            connect(dispatcher, "another")
        release.set()
        deadline = time.monotonic() + 2
        while (
            dispatcher.get(record.command_id).late_finished_at is None
            and time.monotonic() < deadline
        ):
            threading.Event().wait(0.005)
        late = dispatcher.get(record.command_id)
        assert late.status == "failed"  # A late return does not erase the timeout.
        assert late.late_outcome.summary == "The adapter session is connected."
        assert not dispatcher.catalog().targets[0].busy
        assert dispatcher.catalog().targets[0].state == "connected"
    finally:
        release.set()
        dispatcher.close()


def test_unsupported_actions_never_construct_a_provider() -> None:
    def forbidden_factory() -> ShowcaseSessionDriver:
        pytest.fail("unsupported command reached provider")

    dispatcher = CommandDispatcher()
    dispatcher.register(
        SessionController(
            target_id="test",
            label="Test",
            adapter="limited",
            driver_factory=forbidden_factory,
            federations=("Allowed",),
            supported_actions=(),
        )
    )
    try:
        assert connect(dispatcher).status == "unavailable"
        assert all(
            command.state == "unsupported"
            for command in dispatcher.catalog().targets[0].commands
        )
    finally:
        dispatcher.close()


def test_provider_failure_is_typed_and_does_not_guess_session_state() -> None:
    class FailingDriver(ShowcaseSessionDriver):
        def connect(self) -> None:
            raise RuntimeError("provider refused connection")

    dispatcher = CommandDispatcher()
    dispatcher.register(
        SessionController(
            target_id="test",
            label="Test",
            adapter="fake",
            driver_factory=FailingDriver,
            federations=("Allowed",),
        )
    )
    try:
        result = completed(dispatcher, connect(dispatcher).command_id)
        assert result.status == "failed"
        assert result.error.code == "provider_error"
        assert result.error.effect == "unknown"
        assert dispatcher.catalog().targets[0].state == "unknown"
        assert "refused connection" in dispatcher.catalog().targets[0].fault
    finally:
        dispatcher.close()


def test_command_history_survives_event_eviction_and_is_bounded() -> None:
    store = ObserverStore(max_events=1)
    dispatcher = CommandDispatcher(history_limit=2, sink=store.publish)
    dispatcher.register(
        SessionController(
            target_id="test",
            label="Test",
            adapter="fake",
            driver_factory=ShowcaseSessionDriver,
            federations=("Allowed",),
        )
    )
    try:
        completed(dispatcher, connect(dispatcher).command_id)
        for number in range(3):
            connect(dispatcher, f"disabled-{number}")
        store.clear()
        assert len(dispatcher.history().records) == 2
        assert dispatcher.history().evicted_records == 2
        assert dispatcher.history().persistence == "server-process"
    finally:
        dispatcher.close()


def test_closed_dispatcher_rejects_new_work(dispatcher: CommandDispatcher) -> None:
    dispatcher.close()
    with pytest.raises(CommandConflict, match="closed"):
        connect(dispatcher)


def test_blocked_session_does_not_block_other_targets_and_retains_late_failure() -> (
    None
):
    entered, release = threading.Event(), threading.Event()

    class LateFailingDriver(ShowcaseSessionDriver):
        def connect(self) -> None:
            entered.set()
            release.wait(3)
            raise RuntimeError("late provider failure")

    dispatcher = CommandDispatcher(timeout_seconds=0.2)
    for target, factory in (
        ("slow", LateFailingDriver),
        ("fast", ShowcaseSessionDriver),
    ):
        dispatcher.register(
            SessionController(
                target_id=target,
                label=target,
                adapter="fake",
                driver_factory=factory,
                federations=("Allowed",),
            )
        )
    try:
        slow = dispatcher.submit(
            ConnectCommand(
                action="session.connect",
                target_id="slow",
                request_id="slow",
                parameters=EmptyParameters(),
            )
        )
        assert entered.wait(1)
        fast = dispatcher.submit(
            ConnectCommand(
                action="session.connect",
                target_id="fast",
                request_id="fast",
                parameters=EmptyParameters(),
            )
        )
        assert completed(dispatcher, fast.command_id).status == "succeeded"
        assert completed(dispatcher, slow.command_id).error.code == "timeout"
        release.set()
        deadline = time.monotonic() + 2
        while (
            dispatcher.get(slow.command_id).late_error is None
            and time.monotonic() < deadline
        ):
            threading.Event().wait(0.005)
        result = dispatcher.get(slow.command_id)
        assert result.status == "failed"
        assert result.error.code == "timeout"
        assert "late provider failure" in result.late_error.message
        assert dispatcher.catalog().targets[1].state == "connected"
    finally:
        release.set()
        dispatcher.close()
