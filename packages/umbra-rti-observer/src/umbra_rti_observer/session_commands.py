# pyright: strict
"""A provider-neutral lifecycle controller with explicit session ownership."""

from __future__ import annotations

import logging
import threading
from collections.abc import Callable
from typing import Literal, Protocol

from .command_models import (
    CommandAction,
    CommandDefinition,
    CommandField,
    CommandOutcome,
    CommandRequest,
    CommandTarget,
    EmptyParameters,
    JoinParameters,
    ResignParameters,
)
from .commands import CommandUnavailable
from .core import EventSink
from .schemas import JsonObjectResponse, ObserverModel

SessionState = Literal["disconnected", "connected", "joined", "unknown"]
_ACTIONS: tuple[CommandAction, ...] = (
    "session.connect",
    "federate.join",
    "federate.resign",
    "session.disconnect",
)


class SessionDriver(Protocol):
    """All methods, including factory construction, execute on one owner thread."""

    def connect(self) -> None: ...
    def join(self, parameters: JoinParameters) -> str: ...
    def resign(self, parameters: ResignParameters) -> None: ...
    def disconnect(self) -> None: ...
    def poll(self) -> None: ...


class SessionController:
    def __init__(
        self,
        *,
        target_id: str,
        label: str,
        adapter: str,
        driver_factory: Callable[[], SessionDriver],
        federations: tuple[str, ...],
        sink: EventSink | None = None,
        synthetic: bool = False,
        supported_actions: tuple[CommandAction, ...] = _ACTIONS,
    ) -> None:
        if not federations or any(not value.strip() for value in federations):
            raise ValueError("configure at least one allowed federation")
        self._target_id = target_id
        self._label = label
        self._adapter = adapter
        self._factory = driver_factory
        self._federations = tuple(dict.fromkeys(federations))
        self._sink = sink
        self._synthetic = synthetic
        self._supported = frozenset(supported_actions)
        self._driver: SessionDriver | None = None
        self._lock = threading.RLock()
        self._state: SessionState = "disconnected"
        self._federation: str | None = None
        self._federate: str | None = None
        self._fault: str | None = None
        self._definitions = self._build_definitions()
        self.describe()  # Validate the registration metadata before any thread starts.

    def _build_definitions(self) -> tuple[CommandDefinition, ...]:
        specs: tuple[
            tuple[
                CommandAction,
                str,
                str,
                type[ObserverModel],
                bool,
                tuple[CommandField, ...],
            ],
            ...,
        ] = (
            (
                "session.connect",
                "Connect",
                "Connect the configured adapter session.",
                EmptyParameters,
                False,
                (),
            ),
            (
                "federate.join",
                "Join federation",
                "Join an allowed federation using this session.",
                JoinParameters,
                False,
                (
                    CommandField(
                        name="federation",
                        label="Federation",
                        input="select",
                        required=True,
                        choices=self._federations,
                        default_value=self._federations[0],
                    ),
                    CommandField(
                        name="federate_name",
                        label="Federate name",
                        input="text",
                        required=True,
                        default_value="Observer",
                    ),
                    CommandField(
                        name="federate_type",
                        label="Federate type",
                        input="text",
                        required=True,
                        default_value="Observer",
                    ),
                ),
            ),
            (
                "federate.resign",
                "Resign",
                "Resign only the federate joined by this session.",
                ResignParameters,
                True,
                (
                    CommandField(
                        name="policy",
                        label="Resign policy",
                        input="select",
                        required=True,
                        choices=(
                            "no_action",
                            "unconditionally_divest_attributes",
                            "delete_objects",
                            "cancel_pending_ownership_acquisitions",
                            "delete_objects_then_divest",
                            "cancel_then_delete_then_divest",
                        ),
                        default_value="no_action",
                    ),
                ),
            ),
            (
                "session.disconnect",
                "Disconnect",
                "Disconnect this adapter after resigning its federate.",
                EmptyParameters,
                True,
                (),
            ),
        )
        return tuple(
            CommandDefinition(
                action=action,
                title=title,
                description=description,
                state="available",
                requires_confirmation=confirmation,
                parameter_model=model.__name__,
                parameter_schema=JsonObjectResponse.model_validate(
                    model.model_json_schema()
                ).root,
                fields=fields,
            )
            for action, title, description, model, confirmation, fields in specs
        )

    def describe(self) -> CommandTarget:
        with self._lock:
            allowed: dict[SessionState, set[CommandAction]] = {
                "disconnected": {"session.connect"},
                "connected": {"federate.join", "session.disconnect"},
                "joined": {"federate.resign"},
                "unknown": set(),
            }
            definitions: list[CommandDefinition] = []
            for definition in self._definitions:
                state = "available"
                reason = None
                if definition.action not in self._supported:
                    state, reason = (
                        "unsupported",
                        "This adapter does not implement this action.",
                    )
                elif definition.action not in allowed[self._state]:
                    state, reason = (
                        "disabled",
                        self._fault
                        or f"Action is unavailable while the session is {self._state}.",
                    )
                definitions.append(
                    CommandDefinition.model_validate(
                        {**dict(definition), "state": state, "reason": reason}
                    )
                )
            return CommandTarget(
                target_id=self._target_id,
                label=self._label,
                adapter=self._adapter,
                state=self._state,
                federation=self._federation,
                federate=self._federate,
                synthetic=self._synthetic,
                fault=self._fault,
                commands=tuple(definitions),
            )

    def execute(self, request: CommandRequest) -> CommandOutcome:
        descriptor = next(
            item for item in self.describe().commands if item.action == request.action
        )
        if request.target_id != self._target_id or descriptor.state != "available":
            raise CommandUnavailable(
                descriptor.reason or "Command target does not match this session."
            )
        if (
            request.action == "federate.join"
            and request.parameters.federation not in self._federations
        ):
            raise CommandUnavailable(
                "Federation is not in this session's configured allowlist."
            )
        try:
            if request.action == "session.connect":
                if self._driver is None:
                    self._driver = self._factory()
                self._driver.connect()
                with self._lock:
                    self._state = "connected"
                self._emit("session.connected")
                return CommandOutcome(summary="The adapter session is connected.")
            driver = self._driver
            if driver is None:
                raise CommandUnavailable("Session has no driver.")
            if request.action == "federate.join":
                handle = driver.join(request.parameters)
                with self._lock:
                    self._state = "joined"
                    self._federation = request.parameters.federation
                    self._federate = request.parameters.federate_name
                self._emit("federate.joined")
                return CommandOutcome(
                    summary=f"Joined {request.parameters.federation} as {request.parameters.federate_name}.",
                    federate_handle=handle,
                )
            if request.action == "federate.resign":
                name = self._federate
                driver.resign(request.parameters)
                with self._lock:
                    self._state, self._federate = "connected", None
                self._emit("federate.resigned", federate=name)
                return CommandOutcome(summary="The controlled federate resigned.")
            driver.disconnect()
            with self._lock:
                self._state, self._federation = "disconnected", None
            self._emit("session.disconnected")
            return CommandOutcome(summary="The adapter session disconnected.")
        except CommandUnavailable:
            raise
        except Exception as error:
            with self._lock:
                self._state = "unknown"
                self._fault = f"Provider call failed; session state is unknown and requires operator recovery: {type(error).__name__}: {error}"
            raise

    def _emit(self, event_type: str, *, federate: str | None = None) -> None:
        if self._sink is not None:
            try:
                self._sink(
                    {
                        "event_type": event_type,
                        "family": "lifecycle",
                        "source": self._target_id,
                        "federation": self._federation,
                        "federate": federate or self._federate,
                        "details": {
                            "synthetic": self._synthetic,
                            "target_id": self._target_id,
                        },
                    }
                )
            except Exception:
                logging.getLogger(__name__).warning(
                    "Session observation sink failed", exc_info=True
                )

    def poll(self) -> None:
        if self._driver is not None and self._state in {"connected", "joined"}:
            self._driver.poll()

    def close(self) -> None:
        # Never choose a destructive resign policy implicitly on shutdown.
        driver = self._driver
        if driver is not None:
            if self._state == "joined":
                driver.resign(ResignParameters(policy="no_action"))
                self._state = "connected"
            if self._state == "connected":
                driver.disconnect()
                self._state = "disconnected"
