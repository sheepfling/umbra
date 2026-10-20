"""Opt-in internal adapter smoke; this is not IEEE conformance evidence."""

from __future__ import annotations

import os
import threading
import time
from pathlib import Path
from uuid import uuid4

import pytest

if os.environ.get("UMBRA_OBSERVER_NATIVE_SMOKE") != "1":
    pytest.skip(
        "Set UMBRA_OBSERVER_NATIVE_SMOKE=1 with the built 2025 native provider on PYTHONPATH",
        allow_module_level=True,
    )

from hla.rti1516_2025 import CallbackModel, FederateAmbassador, ResignAction
from umbra._native.rti1516_2025 import UmbraRtiFactory
from umbra_rti_observer import ObserverStore
from umbra_rti_observer.command_models import (
    CommandRecord,
    CommandRequest,
    CommandStatus,
    ConnectCommand,
    DisconnectCommand,
    EmptyParameters,
    JoinCommand,
    JoinParameters,
    ResignCommand,
    ResignParameters,
)
from umbra_rti_observer.commands import CommandDispatcher
from umbra_rti_observer_umbra import UmbraFederateAmbassador
from umbra_rti_observer_umbra.commands import (
    LifecycleAmbassador,
    create_umbra_command_session,
)


class _TracedNativeLifecycle:
    """Trace Python entry threads while every operation reaches the real RTI."""

    def __init__(
        self, native: LifecycleAmbassador, calls: list[tuple[str, int]],
    ) -> None:
        self._native = native
        self._calls = calls

    def _record(self, name: str) -> None:
        self._calls.append((name, threading.get_ident()))

    def connect(
        self, federateAmbassador: FederateAmbassador, callbackModel: CallbackModel,
        configuration: object | None = None, credentials: object | None = None,
    ) -> object:
        self._record("connect")
        return self._native.connect(
            federateAmbassador, callbackModel,
            configuration=configuration, credentials=credentials,
        )

    def joinFederationExecution(
        self, federateType: str, federationExecutionName: str, *,
        federateName: str | None = None,
    ) -> object:
        self._record("join")
        return self._native.joinFederationExecution(
            federateType, federationExecutionName, federateName=federateName,
        )

    def resignFederationExecution(self, resignAction: ResignAction) -> None:
        self._record("resign")
        self._native.resignFederationExecution(resignAction)

    def disconnect(self) -> None:
        self._record("disconnect")
        self._native.disconnect()

    def evokeMultipleCallbacks(
        self, approximateMinimumTimeInSeconds: float,
        approximateMaximumTimeInSeconds: float,
    ) -> bool:
        self._record("poll")
        return self._native.evokeMultipleCallbacks(
            approximateMinimumTimeInSeconds, approximateMaximumTimeInSeconds,
        )


def test_native_controlled_session_connect_join_resign_disconnect() -> None:
    name = f"observer-control-smoke-{uuid4()}"
    fom = (
        Path(__file__).resolve().parents[3]
        / "cpp/tests/data/switch-support-enabled-fom.xml"
    )
    factory = UmbraRtiFactory()
    admin = factory.getRtiAmbassador()
    admin.connect(UmbraFederateAmbassador(), CallbackModel.HLA_EVOKED)
    created = False
    store = ObserverStore()
    calls: list[tuple[str, int]] = []
    commands = CommandDispatcher(timeout_seconds=5)

    def owned_factory() -> LifecycleAmbassador:
        calls.append(("factory", threading.get_ident()))
        return _TracedNativeLifecycle(factory.getRtiAmbassador(), calls)

    def submit(request: CommandRequest, status: CommandStatus = "succeeded") -> CommandRecord:
        current = type(request).model_validate({
            **request.model_dump(mode="python"),
            "expected_version": commands.catalog().targets[0].version,
        })
        result = commands.submit(current)
        deadline = time.monotonic() + 6
        while result.status in {"accepted", "running"} and time.monotonic() < deadline:
            threading.Event().wait(0.01)
            result = commands.get(result.command_id)
        assert result.status == status, result.model_dump_json(indent=2)
        return result

    try:
        admin.createFederationExecution(name, str(fom), "HLAinteger64Time")
        created = True
        commands.register(
            create_umbra_command_session(
                owned_factory,
                target_id="native",
                federations=(name, name + "-missing"),
                sink=store.publish,
            )
        )
        initial = commands.catalog().targets[0]
        assert initial.state == "disconnected" and initial.version is not None
        assert {row.action for row in initial.commands} == {
            "session.connect", "federate.join", "federate.resign", "session.disconnect",
        }
        requests = (
            ConnectCommand(
                action="session.connect",
                request_id="connect",
                target_id="native",
                parameters=EmptyParameters(),
            ),
            JoinCommand(
                action="federate.join",
                request_id="join",
                target_id="native",
                parameters=JoinParameters(
                    federation=name, federate_name="ManagedObserver"
                ),
            ),
            ResignCommand(
                action="federate.resign",
                request_id="resign",
                target_id="native",
                confirmed=True,
                parameters=ResignParameters(),
            ),
            DisconnectCommand(
                action="session.disconnect",
                request_id="disconnect",
                target_id="native",
                confirmed=True,
                parameters=EmptyParameters(),
            ),
        )
        for request, state in zip(
            requests, ("connected", "joined", "connected", "disconnected"), strict=True,
        ):
            result = submit(request)
            current = commands.catalog().targets[0]
            assert current.state == state and current.fault is None
            assert current.version is not None
            assert current.version.instance_id == initial.version.instance_id
            assert current.version.revision > initial.version.revision
            if request.action == "federate.join":
                assert result.outcome is not None and result.outcome.federate_handle
                assert current.federation == name and current.federate == "ManagedObserver"
            if request.action == "session.connect":
                # Wait while connected, when an idle evoked pump is meaningful.
                deadline = time.monotonic() + 1
                while not any(action == "poll" for action, _ in calls) and time.monotonic() < deadline:
                    threading.Event().wait(0.01)
                assert any(action == "poll" for action, _ in calls)
                # The federation is allowlisted but does not exist in this real RTI.
                # Its typed rejection must not poison the connected owned session.
                rejected = submit(JoinCommand(
                    action="federate.join", request_id="missing", target_id="native",
                    parameters=JoinParameters(
                        federation=name + "-missing", federate_name="ManagedObserver",
                    ),
                ), "unavailable")
                assert rejected.error is not None and rejected.error.effect == "none"
                assert "FederationExecutionDoesNotExist" in rejected.error.message
                assert commands.catalog().targets[0].state == "connected"
        assert {action for action, _ in calls} == {
            "factory", "connect", "join", "resign", "disconnect", "poll",
        }
        assert sum(action == "factory" for action, _ in calls) == 1
        assert len({thread for _, thread in calls}) == 1
        assert calls[0][1] != threading.get_ident()
        assert [row["event_type"] for row in store.events() if row["family"] == "lifecycle"] == [
            "session.connected", "federate.joined", "federate.resigned", "session.disconnected",
        ]
    finally:
        commands.close()
        try:
            if created:
                admin.destroyFederationExecution(name)
        finally:
            try:
                admin.disconnect()
            finally:
                store.close()
