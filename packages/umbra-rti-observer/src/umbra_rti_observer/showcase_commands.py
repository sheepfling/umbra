"""Synthetic control driver for a repeatable, RTI-free management walkthrough."""

from typing import Literal

from .command_models import JoinParameters, ResignParameters, SessionStateObservation


class ShowcaseSessionDriver:
    def __init__(self, *, fault_once: Literal["connect", "join"] | None = None, inspection_complete: bool = True) -> None:
        self._connected = False
        self._federation: str | None = None
        self._federate: str | None = None
        self._fault_once = fault_once
        self._inspection_complete = inspection_complete

    def _fail_once(self, action: str) -> None:
        if self._fault_once == action:
            self._fault_once = None
            raise RuntimeError(f"Synthetic {action} failure after taking effect; inspect the owned session before recovery.")

    def connect(self) -> None:
        self._connected = True
        self._fail_once("connect")

    def join(self, parameters: JoinParameters) -> str:
        self._federation, self._federate = parameters.federation, parameters.federate_name
        self._fail_once("join")
        return f"showcase:{parameters.federate_name}"

    def resign(self, parameters: ResignParameters) -> None:
        self._federation, self._federate = None, None

    def disconnect(self) -> None:
        self._connected = False
        self._federation, self._federate = None, None

    def inspect(self) -> SessionStateObservation:
        if not self._inspection_complete:
            return SessionStateObservation(connection="connected" if self._connected else "disconnected",
                membership="unknown", reason="Synthetic adapter cannot inspect membership; lifecycle controls remain unresolved.")
        return SessionStateObservation(connection="connected" if self._connected else "disconnected",
            membership="joined" if self._federation is not None else "not-joined", federation=self._federation,
            federate=self._federate, federate_handle=f"showcase:{self._federate}" if self._federate else None,
            reason="Actual state of this synthetic owned driver, not an RTI report.")

    def poll(self) -> None:
        return None
