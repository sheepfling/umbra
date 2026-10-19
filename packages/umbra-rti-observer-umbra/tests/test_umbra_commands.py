"""Integration tests for the provider-specific lifecycle driver."""

from __future__ import annotations

import threading
import time

import pytest

pytest.importorskip("pydantic")

from hla.rti1516_2025 import CallbackModel, FederateAmbassador, ResignAction
from hla.rti1516_2025.exceptions import exceptionForName
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
from umbra_rti_observer.commands import CommandDispatcher
from umbra_rti_observer_umbra.commands import create_umbra_command_session


class StrictAmbassador:
    def __init__(self) -> None:
        self.owner = threading.get_ident()
        self.calls: list[str] = []
        self.pumped = threading.Event()
        self.join_error: Exception | None = None
        self.connected = False
        self.joined = False
        self.last_policy: ResignAction | None = None

    def _call(self, name: str) -> None:
        assert threading.get_ident() == self.owner
        self.calls.append(name)

    def connect(
        self,
        federateAmbassador: FederateAmbassador,
        callbackModel: CallbackModel,
        configuration: object | None = None,
        credentials: object | None = None,
    ) -> object:
        self._call("connect")
        assert callbackModel == CallbackModel.HLA_EVOKED
        self.connected = True
        return None

    def joinFederationExecution(
        self,
        federateType: str,
        federationExecutionName: str,
        *,
        federateName: str | None = None,
    ) -> object:
        self._call("join")
        assert (federateType, federationExecutionName, federateName) == (
            "Observer",
            "Allowed",
            "MyObserver",
        )
        if self.join_error is not None:
            raise self.join_error
        self.joined = True
        return "handle-101"

    def resignFederationExecution(self, resignAction: ResignAction) -> None:
        self._call("resign")
        self.last_policy = resignAction
        self.joined = False

    def disconnect(self) -> None:
        self._call("disconnect")
        assert not self.joined
        self.connected = False

    def evokeMultipleCallbacks(
        self,
        approximateMinimumTimeInSeconds: float,
        approximateMaximumTimeInSeconds: float,
    ) -> bool:
        self._call("pump")
        self.pumped.set()
        return False


def wait_for(dispatcher: CommandDispatcher, record: CommandRecord) -> CommandRecord:
    deadline = time.monotonic() + 2
    while time.monotonic() < deadline:
        result = dispatcher.get(record.command_id)
        if result.status not in {"accepted", "running"}:
            return result
        threading.Event().wait(0.005)
    pytest.fail("provider command did not complete")


def test_umbra_lifecycle_factory_calls_and_pump_share_one_owner_thread() -> None:
    created: list[StrictAmbassador] = []

    def factory() -> StrictAmbassador:
        value = StrictAmbassador()
        created.append(value)
        return value

    store = ObserverStore()
    commands = CommandDispatcher(sink=store.publish)
    session = create_umbra_command_session(
        factory, target_id="umbra", federations=("Allowed",), sink=store.publish
    )
    commands.register(session)
    try:
        assert not created
        result = wait_for(
            commands,
            commands.submit(
                ConnectCommand(
                    action="session.connect",
                    request_id="connect",
                    target_id="umbra",
                    parameters=EmptyParameters(),
                )
            ),
        )
        assert result.status == "succeeded"
        rti = created[0]
        assert rti.owner != threading.get_ident()
        assert rti.pumped.wait(1)
        joined = wait_for(
            commands,
            commands.submit(
                JoinCommand(
                    action="federate.join",
                    request_id="join",
                    target_id="umbra",
                    parameters=JoinParameters(
                        federation="Allowed", federate_name="MyObserver"
                    ),
                )
            ),
        )
        assert joined.outcome.federate_handle == "handle-101"
        assert session.describe().federate == "MyObserver"
        resigned = wait_for(
            commands,
            commands.submit(
                ResignCommand(
                    action="federate.resign",
                    request_id="resign",
                    target_id="umbra",
                    confirmed=True,
                    parameters=ResignParameters(
                        policy="cancel_then_delete_then_divest"
                    ),
                )
            ),
        )
        assert resigned.status == "succeeded"
        assert rti.last_policy == ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST
        disconnected = wait_for(
            commands,
            commands.submit(
                DisconnectCommand(
                    action="session.disconnect",
                    request_id="disconnect",
                    target_id="umbra",
                    confirmed=True,
                    parameters=EmptyParameters(),
                )
            ),
        )
        assert disconnected.status == "succeeded"
        assert not rti.connected
        assert any(
            event["event_type"] == "federate.joined"
            and event["federate"] == "MyObserver"
            for event in store.events()
        )
    finally:
        commands.close()


def test_known_membership_failure_preserves_connected_state_and_can_retry() -> None:
    created: list[StrictAmbassador] = []

    def factory() -> StrictAmbassador:
        value = StrictAmbassador()
        value.join_error = exceptionForName(
            "FederationExecutionDoesNotExist", "not present"
        )
        created.append(value)
        return value

    commands = CommandDispatcher()
    session = create_umbra_command_session(
        factory, target_id="umbra", federations=("Allowed",), sink=lambda _: None
    )
    commands.register(session)
    try:
        connected = wait_for(
            commands,
            commands.submit(
                ConnectCommand(
                    action="session.connect",
                    request_id="c",
                    target_id="umbra",
                    parameters=EmptyParameters(),
                )
            ),
        )
        assert connected.status == "succeeded"
        request = JoinCommand(
            action="federate.join",
            request_id="j",
            target_id="umbra",
            parameters=JoinParameters(federation="Allowed", federate_name="MyObserver"),
        )
        result = wait_for(commands, commands.submit(request))
        assert result.status == "unavailable"
        assert result.error.effect == "none"
        assert session.describe().state == "connected"
        created[0].join_error = None
        retry = wait_for(
            commands,
            commands.submit(request.model_copy(update={"request_id": "retry"})),
        )
        assert retry.status == "succeeded"
    finally:
        commands.close()
