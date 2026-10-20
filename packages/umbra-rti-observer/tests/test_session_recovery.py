"""Internal observer control evidence; no normative HLA or native RTI claim."""
from __future__ import annotations

import threading
import time
from collections.abc import Callable

import pytest
from pydantic import ValidationError
from umbra_rti_observer import ObserverStore
from umbra_rti_observer.command_models import (
    CommandOutcome,
    CommandRecord,
    CommandRequest,
    CommandTarget,
    CommandTargetVersion,
    ConnectCommand,
    EmptyParameters,
    InspectSessionCommand,
    JoinCommand,
    JoinParameters,
    RecoverSessionCommand,
    SessionInspectionResult,
    SessionStateObservation,
)
from umbra_rti_observer.commands import CommandConflict, CommandDispatcher
from umbra_rti_observer.session_commands import SessionController, SessionDriver
from umbra_rti_observer.showcase_commands import ShowcaseSessionDriver


def finish(dispatcher: CommandDispatcher, record: CommandRecord) -> CommandRecord:
    deadline = time.monotonic() + 3
    while time.monotonic() < deadline:
        result = dispatcher.get(record.command_id)
        if result.status not in {"accepted", "running"} and not dispatcher.catalog().targets[0].busy:
            return result
        threading.Event().wait(0.005)
    pytest.fail("owned command did not finish")


def version(dispatcher: CommandDispatcher) -> CommandTargetVersion:
    current = dispatcher.catalog().targets[0].version
    assert current is not None
    return current


def inspect(dispatcher: CommandDispatcher, request_id: str, *, recover: bool = False,
            confirmed: bool = True, expected: CommandTargetVersion | None = None) -> CommandRecord:
    model = RecoverSessionCommand if recover else InspectSessionCommand
    return dispatcher.submit(model(action="session.recover" if recover else "session.inspect", target_id="owned",
        request_id=request_id, parameters=EmptyParameters(), confirmed=confirmed, expected_version=expected or version(dispatcher)))


def connect(dispatcher: CommandDispatcher, request_id: str = "connect") -> CommandRecord:
    return dispatcher.submit(ConnectCommand(action="session.connect", target_id="owned", request_id=request_id,
        parameters=EmptyParameters(), expected_version=version(dispatcher)))


def register(dispatcher: CommandDispatcher, factory: Callable[[], SessionDriver],
             *, inspection_supported: bool = True, store: ObserverStore | None = None) -> SessionController:
    session = SessionController(target_id="owned", label="Owned", adapter="test", federations=("Allowed",),
        driver_factory=factory, inspection_supported=inspection_supported, sink=store.publish if store else None)
    dispatcher.register(session)
    return session


def test_connect_effect_is_inspected_then_explicitly_reconciled_without_replaying_the_failed_operation() -> None:
    created: list[ShowcaseSessionDriver] = []
    calls: list[tuple[str, int]] = []
    class Driver(ShowcaseSessionDriver):
        def __init__(self) -> None:
            super().__init__(fault_once="connect")
            created.append(self)
        def connect(self) -> None:
            calls.append(("connect", threading.get_ident()))
            super().connect()
        def inspect(self) -> SessionStateObservation:
            calls.append(("inspect", threading.get_ident()))
            return super().inspect()
        def poll(self) -> None:
            calls.append(("poll", threading.get_ident()))
    dispatcher = CommandDispatcher()
    store = ObserverStore()
    register(dispatcher, Driver, store=store)
    try:
        failed = finish(dispatcher, connect(dispatcher))
        assert failed.error is not None and failed.error.effect == "unknown"
        assert dispatcher.catalog().targets[0].state == "unknown"
        preview = finish(dispatcher, inspect(dispatcher, "preview"))
        assert preview.outcome is not None and preview.outcome.inspection is not None
        assert preview.outcome.inspection.observation.connection == "connected"
        assert not preview.outcome.inspection.reconciled and dispatcher.catalog().targets[0].state == "unknown"
        refused = finish(dispatcher, inspect(dispatcher, "unconfirmed", recover=True, confirmed=False))
        assert refused.status == "unavailable"
        recovered = finish(dispatcher, inspect(dispatcher, "recover", recover=True))
        assert recovered.outcome is not None and recovered.outcome.inspection is not None and recovered.outcome.inspection.reconciled
        assert dispatcher.catalog().targets[0].state == "connected"
        assert dispatcher.get(failed.command_id) == failed
        assert len(created) == 1 and sum(name == "connect" for name, _ in calls) == 1
        assert len({owner for _, owner in calls}) == 1 and calls[0][1] != threading.get_ident()
        assert len([event for event in store.events() if event["event_type"] == "session.reconciled"]) == 1
        assert not any(event["event_type"] == "federate.joined" for event in store.events())
    finally:
        dispatcher.close()


@pytest.mark.parametrize("observation", [
    SessionStateObservation(connection="connected", membership="unknown", reason="No membership query"),
    SessionStateObservation(connection="unknown", membership="not-joined"),
    SessionStateObservation(connection="connected", membership="joined"),
    SessionStateObservation(connection="connected", membership="joined", federation="Other", federate="Not ours"),
    SessionStateObservation(connection="connected", membership="joined", federation=" Allowed ", federate="Not an exact allowlist match"),
])
def test_partial_or_out_of_allowlist_inspection_never_restores_actions(observation: SessionStateObservation) -> None:
    class Driver(ShowcaseSessionDriver):
        def __init__(self) -> None:
            super().__init__(fault_once="connect")
        def inspect(self) -> SessionStateObservation:
            return observation
    dispatcher = CommandDispatcher()
    register(dispatcher, Driver)
    try:
        finish(dispatcher, connect(dispatcher))
        result = finish(dispatcher, inspect(dispatcher, "partial", recover=True))
        assert result.outcome is not None and result.outcome.inspection is not None and not result.outcome.inspection.reconciled
        target = dispatcher.catalog().targets[0]
        assert target.state == "unknown"
        assert all(item.state != "available" for item in target.commands if item.action not in {"session.inspect", "session.recover"})
    finally:
        dispatcher.close()


def test_recovery_restores_known_owned_join_without_using_an_observed_peer_name() -> None:
    class Driver(ShowcaseSessionDriver):
        def __init__(self) -> None:
            super().__init__(fault_once="join")
        def inspect(self) -> SessionStateObservation:
            observation = super().inspect()
            return SessionStateObservation.model_validate({**dict(observation), "federate": None})
    dispatcher = CommandDispatcher()
    register(dispatcher, Driver)
    try:
        finish(dispatcher, connect(dispatcher))
        failed = finish(dispatcher, dispatcher.submit(JoinCommand(action="federate.join", target_id="owned", request_id="join",
            expected_version=version(dispatcher), parameters=JoinParameters(federation="Allowed", federate_name="PrivateOwned"))))
        assert failed.status == "failed"
        finish(dispatcher, inspect(dispatcher, "reconcile", recover=True))
        target = dispatcher.catalog().targets[0]
        assert target.state == "joined" and target.federation == "Allowed" and target.federate is None
        assert next(item for item in target.commands if item.action == "federate.resign").state == "available"
        assert dispatcher.get(failed.command_id) == failed
    finally:
        dispatcher.close()


def test_stale_foreign_missing_and_replayed_versions_have_no_new_provider_effect() -> None:
    created: list[ShowcaseSessionDriver] = []
    def factory() -> ShowcaseSessionDriver:
        result = ShowcaseSessionDriver()
        created.append(result)
        return result
    dispatcher = CommandDispatcher()
    register(dispatcher, factory)
    try:
        first_version = version(dispatcher)
        first_request = ConnectCommand(action="session.connect", target_id="owned", request_id="first",
            expected_version=first_version, parameters=EmptyParameters())
        first = finish(dispatcher, dispatcher.submit(first_request))
        assert dispatcher.submit(first_request) == first
        for identifier, expected in (("stale", first_version), ("foreign", CommandTargetVersion(instance_id="other", revision=0))):
            result = finish(dispatcher, inspect(dispatcher, identifier, recover=True, expected=expected))
            assert result.status == "unavailable" and result.error is not None and result.error.effect == "none"
        missing = dispatcher.submit(ConnectCommand(action="session.connect", target_id="owned", request_id="missing", parameters=EmptyParameters()))
        assert missing.status == "unavailable" and len(created) == 1
    finally:
        dispatcher.close()


def test_callback_fault_needs_a_healthy_owner_pump_before_recovery_clears_the_fault() -> None:
    fault = threading.Event()
    class Driver(ShowcaseSessionDriver):
        def poll(self) -> None:
            if fault.is_set():
                raise RuntimeError("callback pump fault")
    dispatcher = CommandDispatcher()
    register(dispatcher, Driver)
    try:
        finish(dispatcher, connect(dispatcher))
        fault.set()
        deadline = time.monotonic() + 2
        while dispatcher.catalog().targets[0].state != "unknown" and time.monotonic() < deadline:
            threading.Event().wait(0.005)
        preview = finish(dispatcher, inspect(dispatcher, "preview"))
        assert preview.status == "succeeded" and dispatcher.catalog().targets[0].state == "unknown"
        failure = finish(dispatcher, inspect(dispatcher, "unhealthy", recover=True))
        assert failure.status == "failed" and dispatcher.catalog().targets[0].state == "unknown"
        fault.clear()
        recovered = finish(dispatcher, inspect(dispatcher, "healthy", recover=True))
        assert recovered.status == "succeeded" and dispatcher.catalog().targets[0].state == "connected"
        assert dispatcher.catalog().targets[0].fault is None
    finally:
        fault.clear()
        dispatcher.close()


def test_timed_out_inspection_remains_busy_until_owner_returns_and_preserves_late_proof() -> None:
    entered, release = threading.Event(), threading.Event()
    class Driver(ShowcaseSessionDriver):
        def __init__(self) -> None:
            super().__init__(fault_once="connect")
        def inspect(self) -> SessionStateObservation:
            entered.set()
            release.wait(2)
            return super().inspect()
    dispatcher = CommandDispatcher(timeout_seconds=0.05)
    register(dispatcher, Driver)
    try:
        finish(dispatcher, connect(dispatcher))
        recovery = inspect(dispatcher, "slow", recover=True)
        assert entered.wait(1)
        deadline = time.monotonic() + 1
        while dispatcher.get(recovery.command_id).status != "failed" and time.monotonic() < deadline:
            threading.Event().wait(0.005)
        assert dispatcher.catalog().targets[0].busy and dispatcher.catalog().targets[0].state == "unknown"
        with pytest.raises(CommandConflict, match="busy"):
            inspect(dispatcher, "while-busy", recover=True)
        release.set()
        result = finish(dispatcher, recovery)
        assert result.status == "failed" and result.error is not None and result.error.code == "timeout"
        assert result.late_outcome is not None and result.late_outcome.inspection is not None and result.late_outcome.inspection.reconciled
        assert dispatcher.catalog().targets[0].state == "connected"
    finally:
        release.set()
        dispatcher.close()


def test_unavailable_recovery_and_failed_factory_do_not_create_or_replace_drivers() -> None:
    calls: list[int] = []
    def factory() -> ShowcaseSessionDriver:
        calls.append(threading.get_ident())
        raise RuntimeError("factory failed")
    dispatcher = CommandDispatcher()
    register(dispatcher, factory)
    try:
        finish(dispatcher, connect(dispatcher))
        result = inspect(dispatcher, "no-owned-driver", recover=True)
        assert result.status == "unavailable" and len(calls) == 1
        assert dispatcher.catalog().targets[0].state == "unknown"
    finally:
        dispatcher.close()
    dispatcher = CommandDispatcher()
    register(dispatcher, ShowcaseSessionDriver, inspection_supported=False)
    try:
        assert not any(item.action == "session.recover" for item in dispatcher.catalog().targets[0].commands)
        assert inspect(dispatcher, "unsupported", recover=True).status == "unavailable"
    finally:
        dispatcher.close()


def test_named_proof_rejects_contradictory_stale_or_incomplete_claims_of_reconciliation() -> None:
    with pytest.raises(ValidationError):
        SessionStateObservation(connection="disconnected", membership="joined")
    with pytest.raises(ValidationError):
        SessionStateObservation(connection="connected", membership="not-joined", federate="Old name")
    with pytest.raises(ValidationError):
        SessionInspectionResult(checked_at=1., observation=SessionStateObservation(connection="unknown", membership="unknown"),
            reconciled=True, resolution="Cannot prove this")


def test_provider_reported_names_and_handles_survive_the_named_wire_boundary_exactly() -> None:
    observation = SessionStateObservation(connection="connected", membership="joined", federation=" Allowed ",
        federate=" Owner ", federate_handle=" 7 ")
    inspection = SessionInspectionResult(checked_at=1., observation=observation, reconciled=False, resolution="Exact reported facts")
    target = CommandTarget(target_id="owned", label="Owned", adapter="test", state="unknown", commands=(),
        federation=" Previous ", federate=" Previous owner ", inspection=inspection)
    wire = target.model_dump(mode="json")
    assert wire["federation"] == " Previous " and wire["federate"] == " Previous owner "
    assert wire["inspection"]["observation"]["federation"] == " Allowed "
    assert wire["inspection"]["observation"]["federate"] == " Owner "
    assert wire["inspection"]["observation"]["federate_handle"] == " 7 "
    assert CommandTarget.model_validate(wire).model_dump(mode="json") == wire


def test_missing_named_proof_from_a_custom_session_is_typed_failure_not_an_owner_thread_crash() -> None:
    class Controller(SessionController):
        def execute(self, request: CommandRequest) -> CommandOutcome:
            if request.action == "session.inspect":
                return CommandOutcome(summary="No proof")
            return super().execute(request)
    dispatcher = CommandDispatcher()
    dispatcher.register(Controller(target_id="owned", label="Owned", adapter="test", federations=("Allowed",),
        driver_factory=lambda: ShowcaseSessionDriver(fault_once="connect"), inspection_supported=True))
    try:
        finish(dispatcher, connect(dispatcher))
        missing = finish(dispatcher, inspect(dispatcher, "missing-proof"))
        assert missing.status == "failed" and missing.error is not None and missing.error.effect == "unknown"
        recovered = finish(dispatcher, inspect(dispatcher, "owner-still-running", recover=True))
        assert recovered.status == "succeeded" and dispatcher.catalog().targets[0].state == "connected"
    finally:
        dispatcher.close()


def test_authoritatively_disconnected_inspection_restores_only_connect_without_pumping_or_retrying() -> None:
    calls: list[str] = []
    class Driver(ShowcaseSessionDriver):
        def connect(self) -> None:
            calls.append("connect")
            raise RuntimeError("unclassified error before connection took effect")
        def poll(self) -> None:
            pytest.fail("a disconnected driver must not be pumped during recovery")
    dispatcher = CommandDispatcher()
    register(dispatcher, Driver)
    try:
        failed = finish(dispatcher, connect(dispatcher))
        recovered = finish(dispatcher, inspect(dispatcher, "disconnected", recover=True))
        assert recovered.outcome is not None and recovered.outcome.inspection is not None and recovered.outcome.inspection.reconciled
        target = dispatcher.catalog().targets[0]
        assert target.state == "disconnected" and target.federation is None and target.federate is None
        assert {row.action for row in target.commands if row.state == "available"} == {"session.connect", "session.inspect", "session.recover"}
        assert calls == ["connect"] and dispatcher.get(failed.command_id) == failed
    finally:
        dispatcher.close()


def test_unsafe_model_copy_from_an_inspector_is_revalidated_and_does_not_restore_control() -> None:
    good = threading.Event()
    class Driver(ShowcaseSessionDriver):
        def __init__(self) -> None:
            super().__init__(fault_once="connect")
        def inspect(self) -> SessionStateObservation:
            report = super().inspect()
            return report if good.is_set() else report.model_copy(update={"connection": True})
    dispatcher = CommandDispatcher()
    register(dispatcher, Driver)
    try:
        finish(dispatcher, connect(dispatcher))
        invalid = finish(dispatcher, inspect(dispatcher, "bad-copy", recover=True))
        assert invalid.status == "failed" and dispatcher.catalog().targets[0].state == "unknown"
        good.set()
        recovered = finish(dispatcher, inspect(dispatcher, "valid-proof", recover=True))
        assert recovered.status == "succeeded" and dispatcher.catalog().targets[0].state == "connected"
    finally:
        dispatcher.close()
