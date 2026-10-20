# pyright: strict
"""Bounded command history and one execution owner per registered session.

The dispatcher never imports an RTI. A session's calls and optional callback
pump run on its dedicated thread. HTTP acceptance is distinct from execution;
timeout never pretends to cancel a provider call that may still change state.
"""

from __future__ import annotations

import logging
import queue
import threading
import time
import uuid
from collections import OrderedDict
from collections.abc import Callable
from typing import Protocol

from .alerting import CommandWatch, PendingCommandFact
from .command_models import (
    CommandCatalog,
    CommandError,
    CommandHistory,
    CommandOutcome,
    CommandRecord,
    CommandRequest,
    CommandTarget,
    CommandTargetVersion,
    validate_inspection_outcome,
)
from .contract import OBSERVER_CONTRACT_VERSION
from .core import EventSink

_TERMINAL = {"succeeded", "failed", "unavailable"}


class CommandSession(Protocol):
    """describe is a thread-safe cache; other methods run on the owner thread."""

    def describe(self) -> CommandTarget: ...
    def execute(self, request: CommandRequest) -> CommandOutcome: ...
    def poll(self) -> None: ...
    def close(self) -> None: ...


class CommandConflict(ValueError):
    """A request ID was reused with different content, or a session is busy."""


class CommandUnavailable(RuntimeError):
    """Raised before a provider call; the command had no effect."""


class _Worker:
    def __init__(self, session: CommandSession, run: Callable[[_Worker], None]) -> None:
        self.session = session
        self.queue: queue.Queue[str] = queue.Queue(maxsize=1)
        self.inflight: str | None = None
        self.fault: str | None = None
        self.instance_id = uuid.uuid4().hex
        self.revision = 0
        self.stop = threading.Event()
        self.thread = threading.Thread(
            target=run,
            args=(self,),
            daemon=True,
            name=f"observer-control-{session.describe().target_id}",
        )


class CommandDispatcher:
    def __init__(
        self,
        *,
        timeout_seconds: float = 30,
        history_limit: int = 200,
        sink: EventSink | None = None,
    ) -> None:
        if not 0 < timeout_seconds <= 300 or history_limit < 1:
            raise ValueError(
                "timeout must be 0..300 seconds and history_limit positive"
            )
        self.timeout_seconds = timeout_seconds
        self.history_limit = history_limit
        self._sink = sink
        self._lock = threading.RLock()
        self._workers: dict[str, _Worker] = {}
        self._records: OrderedDict[str, CommandRecord] = OrderedDict()
        self._accepted_clocks: dict[str, float] = {}
        self._requests: dict[str, str] = {}
        self._timers: dict[str, threading.Timer] = {}
        self._evicted = 0
        self._closed = False

    def register(self, session: CommandSession) -> None:
        target = session.describe()
        with self._lock:
            if self._closed or target.target_id in self._workers:
                raise ValueError("dispatcher closed or target already registered")
            worker = _Worker(session, self._run)
            self._workers[target.target_id] = worker
            worker.thread.start()

    def catalog(self) -> CommandCatalog:
        with self._lock:
            targets: list[CommandTarget] = []
            for worker in self._workers.values():
                target = worker.session.describe()
                blocked = (
                    self._closed
                    or worker.inflight is not None
                    or worker.fault is not None
                )
                inflight = (
                    self._records.get(worker.inflight)
                    if worker.inflight is not None
                    else None
                )
                uncertain = (
                    inflight is not None
                    and inflight.error is not None
                    and inflight.error.effect == "unknown"
                )
                fault = worker.fault or target.fault
                if uncertain:
                    fault = "A timed-out provider call is still running; its effect is unknown."
                commands = tuple(
                    command.model_copy(
                        update={
                            "state": "disabled",
                            "reason": worker.fault
                            or "A previous command is still running or this dispatcher is closed.",
                        }
                    )
                    if blocked and command.state == "available" and not (
                        worker.fault is not None and not self._closed and worker.inflight is None
                        and command.action in {"session.inspect", "session.recover"})
                    else command
                    for command in target.commands
                )
                targets.append(
                    CommandTarget.model_validate(
                        {
                            **dict(target),
                            "busy": worker.inflight is not None,
                            "fault": fault,
                            "state": "unknown"
                            if uncertain or worker.fault
                            else target.state,
                            "commands": commands,
                            "version": self._version(worker),
                        }
                    )
                )
            return CommandCatalog(
                contract_version=OBSERVER_CONTRACT_VERSION,
                enabled=not self._closed,
                timeout_seconds=self.timeout_seconds,
                history_limit=self.history_limit,
                targets=tuple(targets),
            )

    def submit(self, request: CommandRequest) -> CommandRecord:
        with self._lock:
            previous_id = self._requests.get(request.request_id)
            if previous_id is not None:
                previous = self._records[previous_id]
                if previous.request != request:
                    raise CommandConflict(
                        "request_id already belongs to a different command"
                    )
                return previous
            if self._closed:
                raise CommandConflict("command dispatcher is closed")
            worker = self._workers.get(request.target_id)
            if worker is None:
                raise KeyError(request.target_id)
            if worker.inflight is not None:
                raise CommandConflict(
                    "target is busy; its previous provider call has not returned"
                )
            while len(self._records) >= self.history_limit:
                oldest = next(
                    (
                        key
                        for key, record in self._records.items()
                        if record.status in _TERMINAL
                        and all(row.inflight != key for row in self._workers.values())
                    ),
                    None,
                )
                if oldest is None:
                    raise CommandConflict("command history is full of active work")
                removed = self._records.pop(oldest)
                self._accepted_clocks.pop(oldest, None)
                self._requests.pop(removed.request.request_id, None)
                self._evicted += 1
            record = CommandRecord(
                command_id=str(uuid.uuid4()),
                request=request,
                status="accepted",
                accepted_at=time.time(),
            )
            self._records[record.command_id] = record
            self._accepted_clocks[record.command_id] = time.monotonic()
            self._requests[request.request_id] = record.command_id
            reason = self._unavailable(worker, request)
            if reason is not None:
                return self._change(
                    record.command_id,
                    status="unavailable",
                    finished_at=time.time(),
                    error=CommandError(
                        code="unavailable", message=reason, effect="none"
                    ),
                )
            worker.inflight = record.command_id
            self._audit(record)
            timer = threading.Timer(
                self.timeout_seconds, self._expire, args=(record.command_id,)
            )
            timer.daemon = True
            self._timers[record.command_id] = timer
            timer.start()
            worker.queue.put_nowait(record.command_id)
            return record

    def get(self, command_id: str) -> CommandRecord:
        with self._lock:
            return self._records[command_id]

    def history(self) -> CommandHistory:
        with self._lock:
            return CommandHistory(
                contract_version=OBSERVER_CONTRACT_VERSION,
                records=tuple(reversed(self._records.values())),
                limit=self.history_limit,
                evicted_records=self._evicted,
            )

    def alert_watch(self) -> CommandWatch:
        """Read owner caches only, including timed-out calls still in flight."""
        with self._lock:
            now = time.monotonic()
            inflight = {row.inflight for row in self._workers.values() if row.inflight is not None}
            return CommandWatch(tuple(PendingCommandFact(row.command_id, row.request.target_id,
                max(0., now - self._accepted_clocks[row.command_id]), row.error is not None and row.error.effect == "unknown")
                for row in self._records.values() if row.status in {"accepted", "running"} or
                row.command_id in inflight and row.error is not None and row.error.effect == "unknown"),
                not self._closed, tuple(self._workers))

    def close(self) -> None:
        with self._lock:
            self._closed = True
            workers = tuple(self._workers.values())
            for timer in self._timers.values():
                timer.cancel()
            self._timers.clear()
            for record in tuple(self._records.values()):
                if record.status not in _TERMINAL:
                    self._change(
                        record.command_id,
                        status="failed",
                        finished_at=time.time(),
                        error=CommandError(
                            code="backend_shutdown",
                            message="Dispatcher closed before execution completed.",
                            effect="unknown" if record.status == "running" else "none",
                        ),
                    )
            for worker in workers:
                worker.stop.set()
        # No unbounded join: an RTI call cannot be forcibly interrupted safely.
        deadline = time.monotonic() + 1
        for worker in workers:
            worker.thread.join(timeout=max(0, deadline - time.monotonic()))

    def _unavailable(self, worker: _Worker, request: CommandRequest) -> str | None:
        if request.expected_version != self._version(worker):
            return "Owned target incarnation/revision changed or was not supplied. Refresh the catalog and review before submitting a new request."
        if worker.fault and request.action not in {"session.inspect", "session.recover"}:
            return worker.fault
        descriptor = next(
            (
                item
                for item in worker.session.describe().commands
                if item.action == request.action
            ),
            None,
        )
        if descriptor is None:
            return "The adapter does not offer this command."
        if descriptor.state != "available":
            return descriptor.reason or f"Command is {descriptor.state}."
        if descriptor.requires_confirmation and not request.confirmed:
            return "Confirm the selected action before submitting it."
        return None

    @staticmethod
    def _version(worker: _Worker) -> CommandTargetVersion:
        return CommandTargetVersion(instance_id=worker.instance_id, revision=worker.revision)

    def _change(self, command_id: str, **changes: object) -> CommandRecord:
        # Revalidate every state transition; model_copy(update=...) skips validation.
        previous = self._records[command_id]
        record = CommandRecord.model_validate({**dict(previous), **changes})
        self._records[command_id] = record
        self._audit(record)
        return record

    def _audit(self, record: CommandRecord) -> None:
        if self._sink is not None:
            try:
                phase = (
                    "late_result"
                    if record.late_finished_at is not None
                    else record.status
                )
                self._sink(
                    {
                        "event_type": f"command.{phase}",
                        "family": "management",
                        "source": "observer-control",
                        "operation": record.request.action,
                        "command_id": record.command_id,
                        "request_id": record.request.request_id,
                        "target_id": record.request.target_id,
                        "details": {
                            "status": record.status,
                            "error": record.error.message if record.error else None,
                        "outcome": record.outcome.summary if record.outcome else None,
                        "late_outcome": record.late_outcome.summary if record.late_outcome else None,
                        "late_error": record.late_error.message if record.late_error else None,
                        },
                    }
                )
            except Exception:
                # An unavailable telemetry sink cannot change a command's result.
                logging.getLogger(__name__).warning(
                    "Command audit sink failed", exc_info=True
                )

    def _expire(self, command_id: str) -> None:
        with self._lock:
            record = self._records.get(command_id)
            if record is not None and record.status not in _TERMINAL:
                self._change(
                    command_id,
                    status="failed",
                    finished_at=time.time(),
                    error=CommandError(
                        code="timeout",
                        message="Execution deadline exceeded. A running provider call has not been cancelled.",
                        effect="unknown" if record.status == "running" else "none",
                    ),
                )

    def _run(self, worker: _Worker) -> None:
        try:
            while not worker.stop.is_set():
                try:
                    command_id = worker.queue.get(timeout=0.05)
                except queue.Empty:
                    if worker.fault is not None:
                        continue
                    try:
                        worker.session.poll()
                    except Exception as error:  # noqa: BLE001 - isolate provider callback failures
                        with self._lock:
                            worker.fault = (
                                f"Callback pump failed: {type(error).__name__}: {error}"
                            )
                            worker.revision += 1
                    continue
                with self._lock:
                    record = self._records[command_id]
                    if record.status in _TERMINAL:
                        worker.inflight = None
                        timer = self._timers.pop(command_id, None)
                        if timer is not None:
                            timer.cancel()
                        continue
                    reason = self._unavailable(worker, record.request)
                    if reason is not None:
                        self._change(
                            command_id,
                            status="unavailable",
                            finished_at=time.time(),
                            error=CommandError(
                                code="unavailable", message=reason, effect="none"
                            ),
                        )
                        worker.inflight = None
                        self._timers.pop(command_id).cancel()
                        continue
                    worker.revision += 1
                    self._change(command_id, status="running", started_at=time.time())
                outcome: CommandOutcome | None = None
                failure: CommandError | None = None
                try:
                    outcome = CommandOutcome.model_validate(
                        worker.session.execute(record.request).model_dump(mode="python")
                    )
                    validate_inspection_outcome(record.request, outcome)
                except CommandUnavailable as error:
                    outcome = None
                    failure = CommandError(
                        code="unavailable", message=str(error), effect="none"
                    )
                except Exception as error:  # noqa: BLE001 - provider-defined failures become typed results
                    outcome = None
                    failure = CommandError(
                        code="provider_error",
                        message=f"{type(error).__name__}: {error}",
                        effect="unknown",
                    )
                with self._lock:
                    if failure is not None and failure.effect == "unknown":
                        worker.fault = f"Provider effect is unknown: {failure.message}"
                    if (failure is None and outcome is not None and outcome.inspection is not None
                            and outcome.inspection.reconciled and record.request.action == "session.recover"):
                        worker.fault = None
                    current = self._records[command_id]
                    if current.status in _TERMINAL:
                        self._change(
                            command_id,
                            late_outcome=outcome,
                            late_error=failure,
                            late_finished_at=time.time(),
                        )
                    else:
                        status = (
                            "unavailable"
                            if failure and failure.code == "unavailable"
                            else "failed"
                            if failure
                            else "succeeded"
                        )
                        self._change(
                            command_id,
                            status=status,
                            outcome=outcome,
                            error=failure,
                            finished_at=time.time(),
                        )
                    timer = self._timers.pop(command_id, None)
                    if timer is not None:
                        timer.cancel()
                    worker.inflight = None
                    worker.revision += 1
        finally:
            try:
                worker.session.close()
            except Exception as error:  # noqa: BLE001 - cleanup must not escape an owner thread
                with self._lock:
                    worker.fault = (
                        f"Session cleanup failed: {type(error).__name__}: {error}"
                    )
