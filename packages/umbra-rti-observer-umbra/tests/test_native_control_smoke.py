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

from hla.rti1516_2025 import CallbackModel
from umbra._native.rti1516_2025 import UmbraRtiFactory
from umbra_rti_observer import ObserverStore
from umbra_rti_observer.command_models import (
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
from umbra_rti_observer_umbra.commands import create_umbra_command_session


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
    commands = CommandDispatcher(timeout_seconds=5)
    try:
        admin.createFederationExecution(name, str(fom), "HLAinteger64Time")
        created = True
        commands.register(
            create_umbra_command_session(
                factory.getRtiAmbassador,
                target_id="native",
                federations=(name,),
                sink=ObserverStore().publish,
            )
        )
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
        for request in requests:
            result = commands.submit(request)
            deadline = time.monotonic() + 6
            while (
                result.status in {"accepted", "running"} and time.monotonic() < deadline
            ):
                threading.Event().wait(0.01)
                result = commands.get(result.command_id)
            assert result.status == "succeeded", result.model_dump()
    finally:
        commands.close()
        try:
            if created:
                admin.destroyFederationExecution(name)
        finally:
            admin.disconnect()
