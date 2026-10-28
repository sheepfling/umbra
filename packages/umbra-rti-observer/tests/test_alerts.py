"""Observer alert evidence; no normative RTI claims or provider imports."""

from __future__ import annotations

from dataclasses import replace

import pytest
from fastapi.testclient import TestClient
from pydantic import ValidationError
from umbra_rti_observer.alert_models import AlertConfiguration, AlertReport
from umbra_rti_observer.alerting import (
    AlertConflict,
    AlertEngine,
    AlertRuleSpec,
    AlertScope,
    CommandWatch,
    PendingCommandFact,
)
from umbra_rti_observer.core import ObserverStore
from umbra_rti_observer.server import create_app


class Clock:
    at = 0.0

    def __call__(self):
        return self.at


def result(engine, watch: CommandWatch | None = None):
    selected_watch = watch if watch is not None else CommandWatch()
    return engine.report(0, selected_watch)["results"][0]


def test_stale_observations_are_unknown_until_seen_then_idle_not_disconnected():
    clock = Clock()
    engine = AlertEngine(clock=clock)
    engine.configure(
        [
            AlertRuleSpec(
                "idle", "Idle observations", "stale-observations", threshold_seconds=2
            )
        ],
        0,
        0,
    )
    assert result(engine)["state"] == "unknown"
    engine.observe(AlertScope(source="provider", federate="A"), "object.updated", 1)
    assert result(engine)["state"] == "clear"
    clock.at = 3
    assert (
        result(engine)["state"] == "firing"
        and result(engine)["reason"] == "observations-idle"
    )
    before = len(engine.evidence)
    for _ in range(10):
        engine.report(0)
    assert len(engine.evidence) == before  # No repeated alert flood on polling.


@pytest.mark.parametrize("kind", ["session.disconnected", "federate.resigned"])
def test_explicit_end_requires_exact_source_session_and_applicable_subject(kind):
    clock = Clock()
    scope = AlertScope(source="provider", session_id="join", federate="A")
    engine = AlertEngine(clock=clock)
    engine.configure(
        [
            AlertRuleSpec(
                "idle",
                "Scoped idle",
                "stale-observations",
                scope=scope,
                threshold_seconds=2,
            )
        ],
        0,
        0,
    )
    engine.observe(scope, "object.updated", 1)
    engine.observe(replace(scope, source="other"), kind, 2)
    assert result(engine)["state"] == "clear"
    engine.observe(
        replace(scope, federate=None) if kind == "session.disconnected" else scope,
        kind,
        3,
    )
    assert result(engine)["state"] == "ended"
    engine.observe(scope, "object.updated", 4)
    clock.at = 10
    assert result(engine)["state"] == "ended" and result(engine)["last_sequence"] == 3
    engine.configure(
        [replace(engine.runs["idle"].spec, name="Renamed", threshold_seconds=20)],
        1,
        0,
        reset_id="idle",
    )
    assert result(engine)["state"] == "ended" and result(engine)["last_sequence"] == 3


def test_unscoped_resignation_does_not_claim_an_entire_session_or_federation_ended():
    engine = AlertEngine()
    engine.configure(
        [
            AlertRuleSpec(
                "all", "All observations", "stale-observations", threshold_seconds=2
            )
        ],
        0,
        0,
    )
    engine.observe(
        AlertScope(source="provider", session_id="join", federate="A"),
        "federate.resigned",
        1,
    )
    assert result(engine)["state"] == "clear"


def test_error_window_overflow_keeps_exact_burst_count_then_reports_uncertain_boundary():
    clock = Clock()
    engine = AlertEngine(clock=clock)
    engine.configure(
        [
            AlertRuleSpec(
                "errors", "Errors", "errors", threshold_count=513, window_seconds=1000
            )
        ],
        0,
        0,
    )
    for index in range(514):
        clock.at = float(index)
        engine.observe(AlertScope(), "runtime.error", index + 1)
    assert result(engine)["state"] == "firing" and result(engine)["metric"] == 514
    assert len(engine.runs["errors"].marks) == 512
    clock.at = 1000.5
    row = result(engine)
    assert row["state"] == "unknown" and row["coverage"] == "partial"
    assert row["metric"] == 512 and row["metric_upper"] == 514
    clock.at = 1002
    assert result(engine)["state"] == "clear" and result(engine)["metric"] == 512
    clock.at = 1514
    assert result(engine)["metric"] == 0


def test_same_clock_burst_uses_one_exact_count_not_one_sample_per_error():
    engine = AlertEngine(clock=lambda: 0.0)
    engine.configure(
        [
            AlertRuleSpec(
                "errors", "Errors", "errors", threshold_count=5000, window_seconds=30
            )
        ],
        0,
        0,
    )
    for index in range(10000):
        engine.observe(AlertScope(), "runtime.error", index + 1)
    assert len(engine.runs["errors"].marks) == 1
    assert result(engine)["metric"] == 10000 and result(engine)["state"] == "firing"


def test_committed_errors_and_loss_survive_event_expiry_and_clear_without_payload_copies():
    store = ObserverStore(max_events=1)
    rules = [
        AlertRuleSpec(
            "errors", "Errors", "errors", threshold_count=2, window_seconds=30
        ),
        AlertRuleSpec("loss", "History loss", "dropped-history", threshold_count=1),
    ]
    store.configure_alerts(rules, 0)
    store.publish_many(
        [
            {"event_type": "runtime.error", "details": {"secret": "must-not-copy"}}
            for _ in range(2)
        ]
    )
    state = AlertReport.model_validate(store.alert_report())
    assert all(row.state == "firing" for row in state.results)
    assert "must-not-copy" not in state.model_dump_json()
    store.clear()
    assert all(
        row.state == "firing"
        for row in AlertReport.model_validate(store.alert_report()).results
    )
    store.reset_alert("loss", 1)
    assert (
        next(
            row for row in store.alert_report()["results"] if row["rule_id"] == "loss"
        )["state"]
        == "clear"
    )


def test_configuration_conflict_is_atomic_unchanged_rules_keep_baselines_and_old_evidence_keeps_its_rule():
    store = ObserverStore()
    spec = AlertRuleSpec(
        "errors", "Original name", "errors", threshold_count=1, window_seconds=30
    )
    store.configure_alerts([spec], 0)
    store.publish({"event_type": "runtime.error"})
    with pytest.raises(AlertConflict):
        store.configure_alerts([], 0)
    store.configure_alerts([spec], 1)
    assert store.alert_report()["results"][0]["metric"] == 1
    store.configure_alerts(
        [replace(spec, name="Changed", scope=AlertScope(source="another"))], 2
    )
    report = AlertReport.model_validate(store.alert_report())
    assert report.results[0].metric == 0 and report.results[0].rule_generation == 3
    assert any(
        row.rule.name == "Original name" and row.result.state == "firing"
        for row in report.history
    )
    firing = next(row for row in report.history if row.result.state == "firing")
    ack = AlertReport.model_validate(store.acknowledge_alert(firing.evidence_id))
    assert next(
        row for row in ack.history if row.evidence_id == firing.evidence_id
    ).acknowledged


def test_pending_command_alerts_use_cached_age_unknown_targets_and_effect_coverage():
    engine = AlertEngine()
    engine.configure(
        [
            AlertRuleSpec(
                "pending",
                "Pending",
                "pending-commands",
                threshold_seconds=2,
                target_id="target",
            )
        ],
        0,
        0,
    )
    assert result(engine)["reason"] == "commands-unavailable"
    assert (
        result(engine, CommandWatch(available=True))["reason"] == "target-unavailable"
    )
    assert (
        result(engine, CommandWatch(available=True, target_ids=("target",)))["state"]
        == "clear"
    )
    watch = CommandWatch(
        (PendingCommandFact("one", "target", 3, True),), True, ("target",)
    )
    row = result(engine, watch)
    assert (
        row["state"] == "firing" and row["metric"] == 3 and row["coverage"] == "partial"
    )


def test_evidence_is_bounded_and_acknowledgement_does_not_resolve_a_firing_rule():
    clock = Clock()
    engine = AlertEngine(clock=clock)
    engine.configure(
        [AlertRuleSpec("idle", "Idle", "stale-observations", threshold_seconds=1)], 0, 0
    )
    for index in range(300):
        engine.observe(AlertScope(), "callback.invoked", index + 1)
        engine.report(0)
        clock.at += 2
        engine.report(0)
    assert len(engine.evidence) == 500 and engine.evicted_evidence == 100
    engine.acknowledge(engine.evidence[-1].evidence_id)
    assert engine.evidence[-1].acknowledged and result(engine)["state"] == "firing"
    with pytest.raises(KeyError):
        engine.acknowledge(1)


@pytest.mark.parametrize(
    "rule",
    [
        {"kind": "errors", "threshold_count": True},
        {"kind": "errors", "threshold_count": "2"},
        {"kind": "errors", "threshold_count": 0},
        {"kind": "stale-observations", "threshold_seconds": float("inf")},
        {"kind": "stale-observations", "threshold_seconds": 0},
        {"kind": "pending-commands", "threshold_seconds": 1, "scope": {}},
        {
            "kind": "dropped-history",
            "threshold_count": 1,
            "endpoint": "https://never-follow.invalid",
        },
        {"kind": "errors", "threshold_count": 1, "scope": {"source": " "}},
    ],
)
def test_configuration_models_are_strict_and_kind_specific(rule):
    with pytest.raises(ValidationError):
        AlertConfiguration.model_validate(
            {
                "configuration_version": "umbra-observer-alerts-v1",
                "rules": [{"rule_id": "one", "name": "Rule", **rule}],
            }
        )


@pytest.mark.parametrize(
    "corruption", ["threshold", "unit", "metric", "future-generation", "missing-proof"]
)
def test_named_report_models_reject_dishonest_evidence(corruption):
    store = ObserverStore(max_events=1)
    store.configure_alerts(
        [AlertRuleSpec("loss", "Loss", "dropped-history", threshold_count=1)], 0
    )
    store.publish_many([{"event_type": "callback.invoked"}] * 2)
    report = store.alert_report()
    row = report["results"][0]
    if corruption == "threshold":
        row["threshold"] = 100
    elif corruption == "unit":
        row["unit"] = "seconds"
    elif corruption == "metric":
        row["metric"] = row["metric_upper"] = 0
    elif corruption == "future-generation":
        report["history"][0]["result"]["rule_generation"] = 2
    else:
        row["history_loss"] = None
    with pytest.raises(ValidationError):
        AlertReport.model_validate(report)


def test_rule_limits_duplicate_ids_and_oversized_configuration_reject_before_mutation():
    store = ObserverStore()
    spec = AlertRuleSpec("one", "One", "errors", threshold_count=1, window_seconds=30)
    for rules in (
        [spec, spec],
        [replace(spec, rule_id=str(index)) for index in range(65)],
        [
            replace(spec, rule_id=str(index), scope=AlertScope(*(["x" * 512] * 4)))
            for index in range(64)
        ],
    ):
        with pytest.raises(ValueError):
            store.configure_alerts(rules, 0)
        assert store.alert_report()["revision"] == 0


def test_public_alert_report_is_detached_and_rejected_event_batches_do_not_change_alerts():
    store = ObserverStore()
    store.configure_alerts(
        [
            AlertRuleSpec(
                "errors", "Errors", "errors", threshold_count=1, window_seconds=30
            )
        ],
        0,
    )
    detached = store.alert_report()
    detached["rules"][0]["name"] = "external mutation"
    with pytest.raises((ValueError, TypeError)):
        store.publish_many(
            [
                {"event_type": "runtime.error"},
                {
                    "event_type": "time.observed",
                    "time_observation": {"kind": "impossible"},
                },
            ]
        )
    state = store.alert_report()
    assert state["rules"][0]["name"] == "Errors" and state["results"][0]["metric"] == 0


def test_http_round_trip_conflicts_read_only_and_disabled_capabilities():
    store = ObserverStore(max_events=1)
    app = create_app(store, enable_recording=False, enable_analytics=False)
    configuration = {
        "configuration_version": "umbra-observer-alerts-v1",
        "rules": [
            {
                "rule_id": "errors",
                "name": "Errors",
                "kind": "errors",
                "threshold_count": 1,
                "window_seconds": 30,
            }
        ],
    }
    with TestClient(app) as client:
        paths = client.get("/api/contract").json()["api_paths"]
        assert paths["alerts"] == "/api/alerts" and "commands" not in paths
        instance = client.get(paths["alerts"]).json()["instance_id"]
        response = client.put(
            paths["alert_configuration"],
            json={
                "expected_instance_id": instance,
                "expected_revision": 0,
                "configuration": configuration,
            },
        )
        assert response.status_code == 200
        store.publish({"event_type": "runtime.error"})
        report = AlertReport.model_validate(client.get(paths["alerts"]).json())
        assert (
            report.results[0].state == "firing"
            and report.persistence == "server-process"
        )
        assert (
            client.put(
                paths["alert_configuration"],
                json={
                    "expected_instance_id": instance,
                    "expected_revision": 0,
                    "configuration": configuration,
                },
            ).status_code
            == 409
        )
        assert (
            client.post(
                paths["alert_reset"],
                json={
                    "expected_instance_id": "previous-backend",
                    "expected_revision": 1,
                    "rule_id": "errors",
                },
            ).status_code
            == 409
        )
        assert (
            client.post(
                paths["alert_reset"],
                json={
                    "expected_instance_id": instance,
                    "expected_revision": 1,
                    "rule_id": "errors",
                },
            ).status_code
            == 200
        )
    with TestClient(create_app(store, enable_alert_controls=False)) as client:
        assert client.get("/api/alerts").status_code == 200
        assert (
            "alert_configuration" not in client.get("/api/contract").json()["api_paths"]
        )
        assert client.put("/api/alerts/configuration", json={}).status_code == 404
    with TestClient(create_app(store, enable_alerts=False)) as client:
        assert "alerts" not in client.get("/api/contract").json()["api_paths"]
        assert client.get("/api/alerts").status_code == 404
        assert client.get("/api/state").status_code == 200
