"""Bounded observer alerts. Pure Python; the owning store supplies its lock.

Rules watch committed ingress, not the independently expiring event table.
No provider calls, payload copies, web models or filesystem writes happen here.
"""

# pyright: strict
from __future__ import annotations

import json
import math
import time
from collections import deque
from collections.abc import Callable, Sequence
from dataclasses import asdict, dataclass, field, replace
from time import monotonic
from typing import Literal
from uuid import uuid4

from .models import JsonObject, to_json_object

RuleKind = Literal["stale-observations", "errors", "dropped-history", "pending-commands"]
AlertState = Literal["disabled", "unknown", "clear", "firing", "ended"]
AlertCoverage = Literal["complete", "partial", "unavailable", "not-observed"]
Severity = Literal["info", "warning", "critical"]
AlertReason = Literal[
    "disabled", "not-observed", "observations-current", "observations-idle",
    "explicit-disconnect", "explicit-resignation", "errors-below-threshold",
    "errors-threshold", "error-window-incomplete", "history-below-threshold",
    "history-loss-threshold", "pending-below-threshold", "pending-threshold",
    "commands-unavailable", "target-unavailable", "clock-unusable",
]
CONFIG_VERSION = "umbra-observer-alerts-v1"
MAX_RULES, MAX_CONFIG_BYTES, ERROR_MARK_LIMIT, EVIDENCE_LIMIT = 64, 65536, 512, 500


def _text(value: object, limit: int = 512) -> None:
    if value is not None and (not isinstance(value, str) or not value.strip() or len(value) > limit):
        raise ValueError("alert text must be nonblank and bounded")
    if value is not None:
        value.encode("utf-8")


@dataclass(frozen=True, slots=True)
class AlertScope:
    source: str | None = None
    federation: str | None = None
    federate: str | None = None
    session_id: str | None = None

    def matches(self, other: AlertScope, *, session_end: bool = False) -> bool:
        return all(expected is None or expected == actual for expected, actual in (
            (self.source, other.source), (self.federation, other.federation),
            (None if session_end else self.federate, other.federate), (self.session_id, other.session_id)))


@dataclass(frozen=True, slots=True)
class AlertRuleSpec:
    rule_id: str
    name: str
    kind: RuleKind
    severity: Severity = "warning"
    enabled: bool = True
    scope: AlertScope = field(default_factory=AlertScope)
    threshold_seconds: float | None = None
    threshold_count: int | None = None
    window_seconds: float | None = None
    target_id: str | None = None

    def __post_init__(self) -> None:
        if type(self.rule_id) is not str or type(self.name) is not str or type(self.scope) is not AlertScope:
            raise TypeError("alert rules require named IDs and an explicit filter model")
        _text(self.rule_id, 128)
        _text(self.name, 128)
        _text(self.target_id, 128)
        for value in (self.scope.source, self.scope.federation, self.scope.federate, self.scope.session_id):
            _text(value)
        if self.kind not in {"stale-observations", "errors", "dropped-history", "pending-commands"} or self.severity not in {"info", "warning", "critical"} or type(self.enabled) is not bool:
            raise ValueError("invalid alert kind, severity or enabled flag")
        if self.kind in {"stale-observations", "pending-commands"}:
            if isinstance(self.threshold_seconds, bool) or not isinstance(self.threshold_seconds, (int, float)) or not math.isfinite(self.threshold_seconds) or not .1 <= self.threshold_seconds <= 86400:
                raise ValueError("age threshold must be 0.1..86400 seconds")
        elif type(self.threshold_count) is not int or not 1 <= self.threshold_count <= 1000000000:
            raise ValueError("count threshold must be 1..1000000000")
        if self.kind == "errors" and (isinstance(self.window_seconds, bool) or not isinstance(self.window_seconds, (int, float)) or not math.isfinite(self.window_seconds) or not 1 <= self.window_seconds <= 3600):
            raise ValueError("error window must be 1..3600 seconds")
        if (self.kind not in {"stale-observations", "pending-commands"} and self.threshold_seconds is not None
                or self.kind not in {"errors", "dropped-history"} and self.threshold_count is not None
                or self.kind != "errors" and self.window_seconds is not None
                or self.kind != "pending-commands" and self.target_id is not None
                or self.kind not in {"stale-observations", "errors"} and self.scope != AlertScope()):
            raise ValueError("alert parameters must apply to their rule kind")

    def to_json(self) -> JsonObject:
        result = to_json_object({"rule_id": self.rule_id, "name": self.name, "kind": self.kind,
                                "severity": self.severity, "enabled": self.enabled})
        if self.kind in {"stale-observations", "errors"}:
            result["scope"] = to_json_object(asdict(self.scope))
        if self.threshold_seconds is not None:
            result["threshold_seconds"] = float(self.threshold_seconds)
        if self.threshold_count is not None:
            result["threshold_count"] = self.threshold_count
        if self.window_seconds is not None:
            result["window_seconds"] = float(self.window_seconds)
        if self.kind == "pending-commands":
            result["target_id"] = self.target_id
        return result


@dataclass(frozen=True, slots=True)
class PendingCommandFact:
    command_id: str
    target_id: str
    age_seconds: float
    effect_unknown: bool = False


@dataclass(frozen=True, slots=True)
class CommandWatch:
    pending: tuple[PendingCommandFact, ...] = ()
    available: bool = False
    target_ids: tuple[str, ...] = ()


NO_COMMAND_WATCH = CommandWatch()


@dataclass(frozen=True, slots=True)
class HistoryLossFact:
    baseline: int
    current: int


@dataclass(frozen=True, slots=True)
class AlertResult:
    rule_id: str
    rule_generation: int
    state: AlertState
    reason: AlertReason
    coverage: AlertCoverage
    metric: int | float | None
    metric_upper: int | float | None
    threshold: int | float
    unit: Literal["seconds", "observations"]
    last_sequence: int | None
    captured_at: float
    command: PendingCommandFact | None = None
    pending_count: int | None = None
    history_loss: HistoryLossFact | None = None


@dataclass(frozen=True, slots=True)
class AlertEvidence:
    evidence_id: int
    rule: AlertRuleSpec
    result: AlertResult
    acknowledged: bool = False

    def to_json(self) -> JsonObject:
        return to_json_object({"evidence_id": self.evidence_id, "rule": self.rule.to_json(),
                               "result": asdict(self.result), "acknowledged": self.acknowledged})


@dataclass(slots=True)
class ErrorMark:
    at: float
    count: int
    last_sequence: int


@dataclass(slots=True)
class RuleRun:
    spec: AlertRuleSpec
    generation: int
    activated: float
    dropped_baseline: int
    last_at: float | None = None
    last_sequence: int | None = None
    ended: AlertReason | None = None
    marks: deque[ErrorMark] = field(default_factory=lambda: deque[ErrorMark]())
    retained_error_count: int = 0
    overflow_first: float | None = None
    overflow_last: float | None = None
    overflow_count: int = 0
    result: AlertResult | None = None


class AlertConflict(ValueError):
    """Configuration changed in another client; no mutation was applied."""


class AlertEngine:
    def __init__(self, *, clock: Callable[[], float] | None = None, wall: Callable[[], float] | None = None) -> None:
        self.clock = clock if clock is not None else monotonic
        self.wall = wall if wall is not None else time.time
        self.instance_id = str(uuid4())
        self.revision = 0
        self.runs: dict[str, RuleRun] = {}
        self.evidence: deque[AlertEvidence] = deque()
        self.evicted_evidence = 0
        self._next_evidence = 1

    def configure(self, specs: Sequence[AlertRuleSpec], expected_revision: int, dropped: int, *, reset_id: str | None = None) -> None:
        if type(expected_revision) is not int or expected_revision < 0:
            raise ValueError("expected alert revision must be a nonnegative integer")
        if expected_revision != self.revision:
            raise AlertConflict("Alert configuration changed. Refresh before applying changes.")
        if len(specs) > MAX_RULES or len({row.rule_id for row in specs}) != len(specs):
            raise ValueError("alert configuration needs at most 64 unique rule IDs")
        if len(json.dumps({"configuration_version": CONFIG_VERSION, "rules": [row.to_json() for row in specs]}, ensure_ascii=False).encode()) > MAX_CONFIG_BYTES:
            raise ValueError("alert configuration exceeds 64 KiB")
        if reset_id is not None and reset_id not in self.runs:
            raise KeyError(reset_id)
        revision, now = self.revision + 1, self.clock()
        updated: dict[str, RuleRun] = {}
        for spec in specs:
            previous = self.runs.get(spec.rule_id)
            if previous is not None and previous.spec == spec and spec.rule_id != reset_id:
                updated[spec.rule_id] = previous
                continue
            run = RuleRun(spec, revision, now, dropped)
            if previous is not None and spec.kind == previous.spec.kind == "stale-observations" and spec.scope == previous.spec.scope and previous.ended:
                # A threshold/name/reset edit cannot unsee an explicit end.
                run.ended, run.last_at, run.last_sequence = previous.ended, previous.last_at, previous.last_sequence
            updated[spec.rule_id] = run
        self.runs = updated
        self.revision = revision

    def acknowledge(self, evidence_id: int) -> None:
        for index, row in enumerate(self.evidence):
            if row.evidence_id == evidence_id:
                self.evidence[index] = replace(row, acknowledged=True)
                return
        raise KeyError(evidence_id)

    def observe(self, scope: AlertScope, event_type: str, sequence: int) -> None:
        if not self.runs:
            return
        now = self.clock()
        for run in self.runs.values():
            spec = run.spec
            if not spec.enabled or spec.kind not in {"stale-observations", "errors"}:
                continue
            exact_session = spec.scope.source is not None and spec.scope.session_id is not None
            ended = exact_session and ((event_type == "session.disconnected" and spec.scope.matches(scope, session_end=True))
                or (event_type == "federate.resigned" and spec.scope.federate is not None and spec.scope.matches(scope)))
            if spec.kind == "stale-observations" and ended:
                run.ended = "explicit-disconnect" if event_type == "session.disconnected" else "explicit-resignation"
                run.last_sequence, run.last_at = sequence, now
            elif spec.scope.matches(scope) and not run.ended:
                if spec.kind == "stale-observations":
                    run.last_at, run.last_sequence = now, sequence
                elif event_type == "runtime.error":
                    self._trim(run, now)
                    run.retained_error_count += 1
                    if run.marks and run.marks[-1].at == now:
                        run.marks[-1].count += 1
                        run.marks[-1].last_sequence = sequence
                    else:
                        run.marks.append(ErrorMark(now, 1, sequence))
                    run.last_sequence = sequence
                    if len(run.marks) > ERROR_MARK_LIMIT:
                        old = run.marks.popleft()
                        run.retained_error_count -= old.count
                        run.overflow_first = old.at if run.overflow_first is None else run.overflow_first
                        run.overflow_last = old.at
                        run.overflow_count += old.count
    def commit(self, dropped: int) -> None:
        self._evaluate(dropped, None)

    def _trim(self, run: RuleRun, now: float) -> None:
        cutoff = now - (run.spec.window_seconds or 0)
        while run.marks and run.marks[0].at < cutoff:
            run.retained_error_count -= run.marks.popleft().count
        if run.overflow_last is not None and run.overflow_last < cutoff:
            run.overflow_first = run.overflow_last = None
            run.overflow_count = 0

    def _result(self, run: RuleRun, dropped: int, commands: CommandWatch | None) -> AlertResult:
        spec, now = run.spec, self.clock()
        threshold = spec.threshold_seconds if spec.threshold_seconds is not None else spec.threshold_count or 1
        unit: Literal["seconds", "observations"] = "seconds" if spec.threshold_seconds is not None else "observations"
        state: AlertState = "unknown"
        reason: AlertReason = "not-observed"
        coverage: AlertCoverage = "not-observed"
        lower: int | float | None = None
        upper: int | float | None = None
        command: PendingCommandFact | None = None
        pending_count: int | None = None
        history_loss: HistoryLossFact | None = None
        if not spec.enabled:
            state, reason, coverage = "disabled", "disabled", "unavailable"
        elif now < run.activated or not math.isfinite(now) or run.last_at is not None and now < run.last_at or run.marks and now < run.marks[-1].at:
            reason, coverage = "clock-unusable", "unavailable"
        elif spec.kind == "stale-observations":
            if run.ended in {"explicit-disconnect", "explicit-resignation"}:
                state, reason, coverage = "ended", run.ended, "complete"
            elif run.last_at is not None:
                lower = upper = max(0., now - run.last_at)
                state, reason, coverage = ("firing", "observations-idle", "complete") if lower >= threshold else ("clear", "observations-current", "complete")
        elif spec.kind == "errors":
            self._trim(run, now)
            lower = run.retained_error_count
            upper = lower + run.overflow_count
            if run.overflow_first is not None and run.overflow_first >= now - (spec.window_seconds or 0):
                lower = upper
            coverage = "complete" if lower == upper else "partial"
            if lower >= threshold:
                state, reason = "firing", "errors-threshold"
            elif upper < threshold:
                state, reason = "clear", "errors-below-threshold"
            else:
                reason = "error-window-incomplete"
        elif spec.kind == "dropped-history":
            history_loss = HistoryLossFact(run.dropped_baseline, dropped)
            lower = upper = max(0, dropped - run.dropped_baseline)
            coverage = "complete"
            state, reason = ("firing", "history-loss-threshold") if lower >= threshold else ("clear", "history-below-threshold")
        elif commands is None or not commands.available:
            reason, coverage = "commands-unavailable", "unavailable"
        elif spec.target_id is not None and spec.target_id not in commands.target_ids:
            reason, coverage = "target-unavailable", "unavailable"
        else:
            pending = [row for row in commands.pending if spec.target_id is None or row.target_id == spec.target_id]
            pending_count = len(pending)
            command = max(pending, key=lambda row: row.age_seconds) if pending else None
            lower = upper = max((row.age_seconds for row in pending), default=0.)
            coverage = "partial" if any(row.effect_unknown for row in pending) else "complete"
            state, reason = ("firing", "pending-threshold") if lower >= threshold else ("clear", "pending-below-threshold")
        return AlertResult(spec.rule_id, run.generation, state, reason, coverage, lower, upper, threshold, unit,
                           run.last_sequence, self.wall(), command, pending_count, history_loss)

    def _evaluate(self, dropped: int, commands: CommandWatch | None) -> None:
        for run in self.runs.values():
            if commands is None and run.spec.kind == "pending-commands":
                continue
            result = self._result(run, dropped, commands)
            signature = (result.state, result.reason, result.coverage)
            if run.result is None or signature != (run.result.state, run.result.reason, run.result.coverage):
                if len(self.evidence) == EVIDENCE_LIMIT:
                    self.evidence.popleft()
                    self.evicted_evidence += 1
                self.evidence.append(AlertEvidence(self._next_evidence, run.spec, result))
                self._next_evidence += 1
            run.result = result

    def evaluate(self, dropped: int, commands: CommandWatch = NO_COMMAND_WATCH) -> None:
        self._evaluate(dropped, commands)

    def report(self, dropped: int, commands: CommandWatch = NO_COMMAND_WATCH) -> JsonObject:
        self.evaluate(dropped, commands)
        return to_json_object({"configuration_version": CONFIG_VERSION, "instance_id": self.instance_id, "revision": self.revision,
            "rules": [run.spec.to_json() for run in self.runs.values()],
            "results": [asdict(run.result) for run in self.runs.values() if run.result is not None],
            "history": [row.to_json() for row in reversed(self.evidence)], "evicted_evidence": self.evicted_evidence,
            "persistence": "server-process", "evaluation_basis": "observer-ingress-monotonic",
            "error_basis": "since-rule-activation-window", "max_rules": MAX_RULES, "max_configuration_bytes": MAX_CONFIG_BYTES,
            "evidence_capacity": EVIDENCE_LIMIT, "error_marks_per_rule": ERROR_MARK_LIMIT})
