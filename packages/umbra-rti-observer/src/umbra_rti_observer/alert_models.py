"""Strict portable alert configuration, evaluation and retained-evidence models."""

# pyright: strict
from __future__ import annotations

import json
from collections.abc import Sequence
from typing import Annotated, Literal, TypeAlias

from pydantic import Field, StringConstraints, model_validator

from .alerting import (
    AlertCoverage,
    AlertReason,
    AlertRuleSpec,
    AlertScope,
    AlertState,
    Severity,
)
from .schemas import (
    JsonText,
    NonNegativeInt,
    ObserverModel,
    ParticipantPerspectiveResponse,
    PositiveInt,
)

AlertText = Annotated[str, StringConstraints(min_length=1, max_length=512, pattern=r"\S")]
RuleText = Annotated[str, StringConstraints(min_length=1, max_length=128, pattern=r"\S")]
AgeThreshold = Annotated[float, Field(ge=.1, le=86400, allow_inf_nan=False)]
CountThreshold = Annotated[int, Field(ge=1, le=1000000000)]
FiniteMetric: TypeAlias = Annotated[int, Field(ge=0)] | Annotated[float, Field(ge=0, allow_inf_nan=False)]


class AlertScopeModel(ObserverModel):
    source: AlertText | None = None
    federation: AlertText | None = None
    federate: AlertText | None = None
    session_id: Annotated[JsonText, StringConstraints(min_length=1, max_length=512, pattern=r"\S")] | None = None
    perspective: ParticipantPerspectiveResponse | None = None

    @model_validator(mode="after")
    def valid_scope(self) -> AlertScopeModel:
        # Use the same bounded runtime filter validation before accepting input.
        self.spec().validate_filter()
        return self

    def spec(self) -> AlertScope:
        return AlertScope(self.source, self.federation, self.federate, self.session_id, self.perspective.spec() if self.perspective else None)


class RuleBase(ObserverModel):
    rule_id: RuleText
    name: RuleText
    severity: Severity = "warning"
    enabled: bool = True


class StaleObservationRule(RuleBase):
    kind: Literal["stale-observations"]
    scope: AlertScopeModel = Field(default_factory=AlertScopeModel)
    threshold_seconds: AgeThreshold

    def spec(self) -> AlertRuleSpec:
        return AlertRuleSpec(self.rule_id, self.name, self.kind, self.severity, self.enabled, self.scope.spec(), threshold_seconds=self.threshold_seconds)


class ErrorCountRule(RuleBase):
    kind: Literal["errors"]
    scope: AlertScopeModel = Field(default_factory=AlertScopeModel)
    threshold_count: CountThreshold
    window_seconds: Annotated[float, Field(ge=1, le=3600, allow_inf_nan=False)] = 300.0

    def spec(self) -> AlertRuleSpec:
        return AlertRuleSpec(self.rule_id, self.name, self.kind, self.severity, self.enabled, self.scope.spec(), threshold_count=self.threshold_count, window_seconds=self.window_seconds)


class DroppedHistoryRule(RuleBase):
    kind: Literal["dropped-history"]
    threshold_count: CountThreshold

    def spec(self) -> AlertRuleSpec:
        return AlertRuleSpec(self.rule_id, self.name, self.kind, self.severity, self.enabled, threshold_count=self.threshold_count)


class PendingCommandRule(RuleBase):
    kind: Literal["pending-commands"]
    target_id: RuleText | None = None
    threshold_seconds: AgeThreshold

    def spec(self) -> AlertRuleSpec:
        return AlertRuleSpec(self.rule_id, self.name, self.kind, self.severity, self.enabled, threshold_seconds=self.threshold_seconds, target_id=self.target_id)


AlertRule: TypeAlias = Annotated[StaleObservationRule | ErrorCountRule | DroppedHistoryRule | PendingCommandRule, Field(discriminator="kind")]


class AlertConfiguration(ObserverModel):
    configuration_version: Literal["umbra-observer-alerts-v1"]
    rules: Annotated[Sequence[AlertRule], Field(max_length=64)]

    @model_validator(mode="after")
    def bounded_configuration(self) -> AlertConfiguration:
        if len({row.rule_id for row in self.rules}) != len(self.rules):
            raise ValueError("alert rule IDs must be unique")
        if len(json.dumps({"configuration_version": self.configuration_version, "rules": [row.model_dump(mode="json") for row in self.rules]}, ensure_ascii=False).encode()) > 65536:
            raise ValueError("alert configuration exceeds 64 KiB")
        return self


class AlertConfigurationUpdate(ObserverModel):
    expected_instance_id: RuleText
    expected_revision: NonNegativeInt
    configuration: AlertConfiguration


class AlertResetRequest(ObserverModel):
    expected_instance_id: RuleText
    expected_revision: NonNegativeInt
    rule_id: RuleText


class AlertAcknowledgeRequest(ObserverModel):
    expected_instance_id: RuleText
    evidence_id: PositiveInt


class AlertCommandProof(ObserverModel):
    command_id: RuleText
    target_id: RuleText
    age_seconds: Annotated[float, Field(ge=0, allow_inf_nan=False)]
    effect_unknown: bool


class HistoryLossProof(ObserverModel):
    baseline: NonNegativeInt
    current: NonNegativeInt


class AlertResultModel(ObserverModel):
    rule_id: RuleText
    rule_generation: PositiveInt
    state: AlertState
    reason: AlertReason
    coverage: AlertCoverage
    metric: FiniteMetric | None
    metric_upper: FiniteMetric | None
    threshold: FiniteMetric
    unit: Literal["seconds", "observations"]
    last_sequence: PositiveInt | None
    captured_at: Annotated[float, Field(allow_inf_nan=False)]
    command: AlertCommandProof | None
    pending_count: NonNegativeInt | None
    history_loss: HistoryLossProof | None

    @model_validator(mode="after")
    def consistent_result(self) -> AlertResultModel:
        if (self.metric is None) != (self.metric_upper is None) or self.metric is not None and self.metric_upper is not None and self.metric > self.metric_upper:
            raise ValueError("alert metric bounds must agree")
        if self.state == "firing" and (self.metric is None or self.metric < self.threshold):
            raise ValueError("a firing alert requires explicit threshold evidence")
        if self.state == "clear" and (self.metric_upper is None or self.metric_upper >= self.threshold):
            raise ValueError("a clear alert requires an upper bound below threshold")
        if self.state == "ended" and (self.reason not in {"explicit-disconnect", "explicit-resignation"} or self.last_sequence is None):
            raise ValueError("ended alerts need explicit lifecycle evidence")
        reasons = {"firing": {"observations-idle", "errors-threshold", "history-loss-threshold", "pending-threshold"},
                   "clear": {"observations-current", "errors-below-threshold", "history-below-threshold", "pending-below-threshold"},
                   "unknown": {"not-observed", "clock-unusable", "error-window-incomplete", "commands-unavailable", "target-unavailable"},
                   "ended": {"explicit-disconnect", "explicit-resignation"}, "disabled": {"disabled"}}
        if self.reason not in reasons[self.state] or self.coverage in {"unavailable", "not-observed"} and self.metric is not None:
            raise ValueError("alert status/reason/coverage must describe the same evidence")
        if self.history_loss is not None and (self.history_loss.current < self.history_loss.baseline or self.metric != self.history_loss.current - self.history_loss.baseline):
            raise ValueError("history-loss proof must match its exact baseline delta")
        if self.command is not None and (self.pending_count is None or self.pending_count < 1 or self.command.age_seconds != self.metric):
            raise ValueError("pending-command proof must identify the measured oldest cached request")
        if self.pending_count == 0 and (self.command is not None or self.metric != 0):
            raise ValueError("an empty pending cache must report an explicit zero")
        return self


def _check_rule_result(rule: AlertRule, row: AlertResultModel) -> None:
    age_rule = isinstance(rule, (StaleObservationRule, PendingCommandRule))
    threshold = rule.threshold_seconds if age_rule else rule.threshold_count
    if rule.rule_id != row.rule_id or row.threshold != threshold or row.unit != ("seconds" if age_rule else "observations") or (not rule.enabled) != (row.state == "disabled"):
        raise ValueError("alert evaluation must use its actual rule, threshold, unit and enabled state")
    if row.state == "ended" and (not isinstance(rule, StaleObservationRule) or rule.scope.perspective is None and (rule.scope.source is None or rule.scope.session_id is None)):
        raise ValueError("a reported end requires an exact source/session filter")
    measured = row.state in {"clear", "firing"}
    if measured and isinstance(rule, DroppedHistoryRule) and row.history_loss is None:
        raise ValueError("a history-loss evaluation needs its baseline counter proof")
    if measured and isinstance(rule, PendingCommandRule) and row.pending_count is None:
        raise ValueError("a pending-command evaluation needs explicit cached work coverage")
    if not isinstance(rule, DroppedHistoryRule) and row.history_loss is not None or not isinstance(rule, PendingCommandRule) and (row.command is not None or row.pending_count is not None):
        raise ValueError("alert evidence must apply to its rule kind")


class AlertEvidenceModel(ObserverModel):
    evidence_id: PositiveInt
    rule: AlertRule
    result: AlertResultModel
    acknowledged: bool

    @model_validator(mode="after")
    def matching_rule(self) -> AlertEvidenceModel:
        _check_rule_result(self.rule, self.result)
        return self


class AlertReport(AlertConfiguration):
    contract_version: Literal["umbra-observer-contract-v1"]
    instance_id: RuleText
    revision: NonNegativeInt
    results: Sequence[AlertResultModel]
    history: Annotated[Sequence[AlertEvidenceModel], Field(max_length=500)]
    evicted_evidence: NonNegativeInt
    persistence: Literal["server-process"]
    evaluation_basis: Literal["observer-ingress-monotonic"]
    error_basis: Literal["since-rule-activation-window"]
    max_rules: Literal[64]
    max_configuration_bytes: Literal[65536]
    evidence_capacity: Literal[500]
    error_marks_per_rule: Literal[512]

    @model_validator(mode="after")
    def exact_rule_results(self) -> AlertReport:
        ids = [row.rule_id for row in self.results]
        if len(set(ids)) != len(ids) or set(ids) != {row.rule_id for row in self.rules} or any(row.rule_generation > self.revision for row in self.results):
            raise ValueError("alert results must cover each configured rule exactly")
        by_id = {row.rule_id: row for row in self.results}
        for rule in self.rules:
            row = by_id[rule.rule_id]
            _check_rule_result(rule, row)
        if any(row.result.rule_generation > self.revision for row in self.history):
            raise ValueError("alert evidence cannot come from a future rule generation")
        evidence_ids = [row.evidence_id for row in self.history]
        if evidence_ids != sorted(set(evidence_ids), reverse=True):
            raise ValueError("alert evidence must be unique newest-first")
        return self
