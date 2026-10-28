"""Synthetic control driver for a repeatable, RTI-free management walkthrough."""

import time
from collections import OrderedDict, deque
from collections.abc import Callable
from dataclasses import dataclass
from typing import Literal
from uuid import uuid4

from .command_models import (
    CommandOutcome,
    CoordinationCommandReceipt,
    CoordinationCommandRequest,
    JoinParameters,
    ResignParameters,
    SessionStateObservation,
)
from .commands import CommandUnavailable
from .core import EventSink
from .models import JsonObject


@dataclass(slots=True)
class _Operation:
    operation_id: str
    label: str
    phase: str


class ShowcaseSessionDriver:
    def __init__(
        self,
        *,
        fault_once: Literal["connect", "join"] | None = None,
        inspection_complete: bool = True,
        sink: EventSink | None = None,
        clock: Callable[[], float] = time.monotonic,
    ) -> None:
        self._connected = False
        self._federation: str | None = None
        self._federate: str | None = None
        self._fault_once = fault_once
        self._inspection_complete = inspection_complete
        self._sink, self._clock = sink, clock
        self._session = uuid4().hex
        self._membership = uuid4().hex
        self._sync: dict[str, _Operation] = {}
        self._save: _Operation | None = None
        self._restore: _Operation | None = None
        self._saved: OrderedDict[tuple[str, str], None] = OrderedDict()
        self._scheduled: deque[tuple[float, Callable[[], None]]] = deque()

    def _fail_once(self, action: str) -> None:
        if self._fault_once == action:
            self._fault_once = None
            raise RuntimeError(
                f"Synthetic {action} failure after taking effect; inspect the owned session before recovery."
            )

    def connect(self) -> None:
        self._connected = True
        self._fail_once("connect")

    def join(self, parameters: JoinParameters) -> str:
        self._federation, self._federate = (
            parameters.federation,
            parameters.federate_name,
        )
        self._membership = uuid4().hex
        self._fail_once("join")
        return f"showcase:{parameters.federate_name}"

    def resign(self, parameters: ResignParameters) -> None:
        self._federation, self._federate = None, None
        self._sync.clear()
        self._save, self._restore = None, None
        self._scheduled.clear()

    def disconnect(self) -> None:
        self._connected = False
        self._federation, self._federate = None, None
        self._saved.clear()

    def _emit(
        self,
        kind: Literal["synchronization", "save", "restore"],
        operation: _Operation,
        phase: str,
        *,
        collective: bool = False,
        failed: bool = False,
    ) -> None:
        if self._sink is None:
            return
        identity: JsonObject = {
            "participant_id": "controlled",
            "membership_id": self._membership,
            "display_name": self._federate,
        }
        data: JsonObject = {
            "kind": kind,
            "operation_id": operation.operation_id,
            "label": operation.label,
            "session_id": self._session,
            "identity_namespace": f"showcase-control:{self._session}",
            "federation_id": self._federation,
            "phase": phase,
            "scope": "federation" if collective else "local",
        }
        if collective:
            data["participants"] = [
                {
                    "identity": identity,
                    "status": "failed"
                    if failed
                    else "achieved"
                    if kind == "synchronization" and phase == "completed"
                    else "completed"
                    if phase == "completed"
                    else "pending",
                }
            ]
        else:
            data["subject"] = identity
        if failed:
            data["reason"] = (
                "Explicit failed result in the synthetic owned-session walkthrough."
            )
        self._sink(
            {
                "event_type": "coordination.observed",
                "family": "management",
                "source": "showcase-session",
                "federation": self._federation,
                "federate": None if collective else self._federate,
                "details": {"synthetic": True},
                "coordination": data,
            }
        )

    def _later(self, callback: Callable[[], None]) -> None:
        self._scheduled.append((self._clock() + 0.3, callback))

    def coordinate(self, request: CoordinationCommandRequest) -> CommandOutcome:
        if self._federation is None:
            raise CommandUnavailable(
                "Join the owned demo session before using coordination services."
            )
        if request.action == "sync.register":
            label = request.parameters.label
            if label in self._sync or len(self._sync) >= 32:
                raise CommandUnavailable(
                    "This label is already pending, or the 32-point demo limit was reached."
                )
            operation = _Operation(request.request_id, label, "requested")
            self._sync[label] = operation
            self._emit("synchronization", operation, "requested")

            def announce() -> None:
                operation.phase = "announced"
                self._emit("synchronization", operation, "registered")
                self._emit("synchronization", operation, "announced", collective=True)

            self._later(announce)
            kind: Literal["synchronization", "save", "restore"] = "synchronization"
        elif request.action == "sync.achieve":
            existing = self._sync.get(request.parameters.label)
            if existing is None or existing.phase != "announced":
                raise CommandUnavailable(
                    "The owned demo driver has no announced synchronization point with this label."
                )
            operation, kind = existing, "synchronization"
            failed = request.parameters.result == "failed"
            operation.phase = "terminal-pending"
            self._emit(
                kind, operation, "failed" if failed else "achieved", failed=failed
            )

            def synchronize() -> None:
                self._emit(
                    "synchronization",
                    operation,
                    "failed" if failed else "completed",
                    collective=True,
                    failed=failed,
                )
                self._sync.pop(operation.label, None)

            self._later(synchronize)
        elif request.action == "save.request" or request.action == "restore.request":
            if self._save is not None or self._restore is not None:
                raise CommandUnavailable(
                    "A save or restore is already pending in this owned demo driver."
                )
            kind = "save" if request.action == "save.request" else "restore"
            label = request.parameters.label
            if kind == "restore" and (self._federation, label) not in self._saved:
                raise CommandUnavailable(
                    "The demo has no successfully saved checkpoint with this exact label."
                )
            operation = _Operation(request.request_id, label, "requested")
            if kind == "save":
                self._save = operation
            else:
                self._restore = operation
            self._emit(kind, operation, "requested")

            def begin() -> None:
                operation.phase = "ready" if kind == "save" else "begun"
                self._emit(
                    kind,
                    operation,
                    "requested" if kind == "save" else "begun",
                    collective=True,
                )
                if kind == "restore":
                    self._emit(kind, operation, "begun")

            self._later(begin)
        elif request.action == "save.begun":
            if self._save is None or self._save.phase != "ready":
                raise CommandUnavailable(
                    "Wait for a save request before reporting save begun."
                )
            operation, kind = self._save, "save"
            operation.phase = "begun"
            self._emit(kind, operation, "begun")
        else:
            kind = "save" if request.action == "save.complete" else "restore"
            existing = self._save if kind == "save" else self._restore
            if existing is None or existing.phase != "begun":
                raise CommandUnavailable(
                    f"The owned demo driver has no begun {kind} to complete."
                )
            operation = existing
            failed = request.parameters.result == "failed"
            operation.phase = "terminal-pending"
            self._emit(
                kind, operation, "failed" if failed else "completed", failed=failed
            )

            def complete() -> None:
                self._emit(
                    kind,
                    operation,
                    "failed" if failed else "completed",
                    collective=True,
                    failed=failed,
                )
                if kind == "save":
                    if not failed:
                        assert self._federation is not None
                        checkpoint = self._federation, operation.label
                        self._saved[checkpoint] = None
                        self._saved.move_to_end(checkpoint)
                        while len(self._saved) > 32:
                            self._saved.popitem(last=False)
                    self._save = None
                else:
                    self._restore = None

            self._later(complete)
        return CommandOutcome(
            summary="The owned demo service returned. Follow explicit coordination reports for the collective result.",
            coordination=CoordinationCommandReceipt(
                kind=kind, label=operation.label, operation_id=operation.operation_id
            ),
        )

    def inspect(self) -> SessionStateObservation:
        if not self._inspection_complete:
            return SessionStateObservation(
                connection="connected" if self._connected else "disconnected",
                membership="unknown",
                reason="Synthetic adapter cannot inspect membership; lifecycle controls remain unresolved.",
            )
        return SessionStateObservation(
            connection="connected" if self._connected else "disconnected",
            membership="joined" if self._federation is not None else "not-joined",
            federation=self._federation,
            federate=self._federate,
            federate_handle=f"showcase:{self._federate}" if self._federate else None,
            reason="Actual state of this synthetic owned driver, not an RTI report.",
        )

    def poll(self) -> None:
        while self._scheduled and self._scheduled[0][0] <= self._clock():
            _, callback = self._scheduled.popleft()
            callback()
