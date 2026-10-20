# pyright: strict
"""Explicitly owned Umbra session controls; never attach to an observed peer."""

from __future__ import annotations

from collections.abc import Callable
from typing import Protocol

from hla.rti1516_2025 import CallbackModel, FederateAmbassador, ResignAction
from hla.rti1516_2025.exceptions import RTIexception
from umbra_rti_observer import EventSink
from umbra_rti_observer.command_models import (
    CommandAction,
    JoinParameters,
    ResignParameters,
    SessionStateObservation,
)
from umbra_rti_observer.commands import CommandUnavailable
from umbra_rti_observer.session_commands import SessionController

from .adapter import UmbraFederateAmbassador


class LifecycleAmbassador(Protocol):
    """The exact subset used here, permitting strict fakes and other factories."""

    def connect(
        self,
        federateAmbassador: FederateAmbassador,
        callbackModel: CallbackModel,
        configuration: object | None = None,
        credentials: object | None = None,
    ) -> object: ...
    def disconnect(self) -> None: ...
    def joinFederationExecution(
        self,
        federateType: str,
        federationExecutionName: str,
        *,
        federateName: str | None = None,
    ) -> object: ...
    def resignFederationExecution(self, resignAction: ResignAction) -> None: ...
    def evokeMultipleCallbacks(
        self,
        approximateMinimumTimeInSeconds: float,
        approximateMaximumTimeInSeconds: float,
    ) -> bool: ...


class _UmbraSessionDriver:
    def __init__(
        self,
        factory: Callable[[], LifecycleAmbassador],
        *,
        target_id: str,
        sink: EventSink,
        configuration: object | None,
        credentials: object | None,
        inspection: Callable[[LifecycleAmbassador], SessionStateObservation] | None,
    ) -> None:
        # This constructor is called by SessionController on its owner thread.
        self._rti = factory()
        self._callback = UmbraFederateAmbassador(source=target_id, sink=sink)
        self._configuration = configuration
        self._credentials = credentials
        self._inspection = inspection

    def connect(self) -> None:
        # A single owner pumps evoked callbacks; no browser/HTTP thread calls RTI.
        self._rti.connect(
            self._callback,
            CallbackModel.HLA_EVOKED,
            configuration=self._configuration,
            credentials=self._credentials,
        )

    def join(self, parameters: JoinParameters) -> str:
        try:
            handle = self._rti.joinFederationExecution(
                parameters.federate_type,
                parameters.federation,
                federateName=parameters.federate_name,
            )
        except RTIexception as error:
            if type(error).__name__ in {
                "FederationExecutionDoesNotExist",
                "FederateNameAlreadyInUse",
                "InvalidFederateName",
                "InvalidFederateType",
            }:
                raise CommandUnavailable(f"{type(error).__name__}: {error}") from error
            raise
        self._callback.federation = parameters.federation
        self._callback.federate = parameters.federate_name
        return str(handle)

    def resign(self, parameters: ResignParameters) -> None:
        try:
            self._rti.resignFederationExecution(ResignAction[parameters.policy.upper()])
        except RTIexception as error:
            if type(error).__name__ in {
                "FederateOwnsAttributes",
                "OwnershipAcquisitionPending",
            }:
                raise CommandUnavailable(f"{type(error).__name__}: {error}") from error
            raise
        self._callback.federate = None

    def disconnect(self) -> None:
        self._rti.disconnect()
        self._callback.federation = None

    def poll(self) -> None:
        self._rti.evokeMultipleCallbacks(0.0, 0.01)

    def inspect(self) -> SessionStateObservation:
        if self._inspection is None:
            raise CommandUnavailable("No authoritative inspection is configured for this Umbra binding.")
        return SessionStateObservation.model_validate(self._inspection(self._rti).model_dump(mode="python"))


def create_umbra_command_session(
    factory: Callable[[], LifecycleAmbassador],
    *,
    target_id: str,
    federations: tuple[str, ...],
    sink: EventSink,
    label: str = "Umbra observer session",
    configuration: object | None = None,
    credentials: object | None = None,
    inspection: Callable[[LifecycleAmbassador], SessionStateObservation] | None = None,
    supported_actions: tuple[CommandAction, ...] = (
        "session.connect",
        "federate.join",
        "federate.resign",
        "session.disconnect",
        "session.inspect",
        "session.recover",
    ),
) -> SessionController:
    """Register only factories for sessions this application explicitly owns.

    The factory runs lazily on the execution thread. Configuration, credentials,
    and allowed federations are supplied by the application, never browser JSON.
    Other RTI shapes implement their own driver in a separate adapter package.
    """
    return SessionController(
        target_id=target_id,
        label=label,
        adapter="umbra-python-rti",
        federations=federations,
        driver_factory=lambda: _UmbraSessionDriver(
            factory,
            target_id=target_id,
            sink=sink,
            configuration=configuration,
            credentials=credentials,
            inspection=inspection,
        ),
        sink=sink,
        supported_actions=supported_actions,
        inspection_supported=inspection is not None,
    )
