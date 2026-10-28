"""Internal identity-scoped alert evidence; no provider calls or RTI claims."""

from __future__ import annotations

from dataclasses import replace

import pytest
from pydantic import ValidationError
from umbra_rti_observer.alert_models import AlertConfiguration, AlertReport
from umbra_rti_observer.alerting import AlertRuleSpec, AlertScope
from umbra_rti_observer.attribution import event_perspective
from umbra_rti_observer.core import ObserverStore


def event(
    kind="callback.invoked",
    *,
    namespace="ns",
    membership="join",
    name="Twin",
    source="reporter",
):
    return {
        "event_type": kind,
        "source": source,
        "federate": name,
        "session_id": " session ",
        "participant_attribution": {
            "session_id": " session ",
            "identity_namespace": namespace,
            "federation_id": "fed",
            "participant": {
                "participant_id": "p",
                "membership_id": membership,
                "display_name": name,
            },
        },
    }


def exact_scope():
    perspective = event_perspective(event())
    assert perspective is not None
    return AlertScope(perspective=perspective)


@pytest.mark.parametrize(
    "changed", [{"namespace": "other"}, {"membership": "rejoin"}, {"source": "other"}]
)
def test_error_rules_match_only_exact_perspectives_across_aliases_and_event_expiry(
    changed,
):
    store = ObserverStore(max_events=1)
    spec = AlertRuleSpec(
        "exact",
        "Exact errors",
        "errors",
        scope=exact_scope(),
        threshold_count=2,
        window_seconds=30,
    )
    store.configure_alerts([spec], 0)
    store.publish(event("runtime.error", **changed))
    assert store.alert_report()["results"][0]["metric"] == 0
    store.publish(event("runtime.error"))
    store.publish(event("runtime.error", name="Renamed"))
    report = AlertReport.model_validate(store.alert_report())
    assert report.results[0].state == "firing" and report.results[0].metric == 2
    assert report.rules[0].scope.perspective.key == exact_scope().perspective.key
    store.clear()
    assert AlertReport.model_validate(store.alert_report()).results[0].metric == 2


@pytest.mark.parametrize("kind", ["session.disconnected", "federate.resigned"])
def test_explicit_join_alerts_require_matching_domain_end_and_do_not_reopen(kind):
    store = ObserverStore()
    spec = AlertRuleSpec(
        "exact",
        "Exact freshness",
        "stale-observations",
        scope=exact_scope(),
        threshold_seconds=60,
    )
    store.configure_alerts([spec], 0)
    store.publish(event())
    store.publish(event(kind, namespace="other"))
    store.publish(
        {
            "event_type": "session.disconnected",
            "source": "reporter",
            "session_id": " session ",
        }
    )
    assert store.alert_report()["results"][0]["state"] == "clear"
    store.publish(event(kind, name="Alias"))
    report = AlertReport.model_validate(store.alert_report())
    assert report.results[0].state == "ended"
    store.publish(event())
    assert store.alert_report()["results"][0]["state"] == "ended"
    store.configure_alerts([replace(spec, name="Edited", threshold_seconds=120)], 1)
    assert store.alert_report()["results"][0]["state"] == "ended"


def test_a_name_filter_aggregating_identified_joins_cannot_end_when_one_join_ends_even_after_reset():
    store = ObserverStore()
    spec = AlertRuleSpec(
        "legacy",
        "Shared name",
        "stale-observations",
        scope=AlertScope(source="reporter", federate="Twin", session_id=" session "),
        threshold_seconds=60,
    )
    store.configure_alerts([spec], 0)
    store.publish(event())
    store.publish(event(membership="rejoin"))
    store.publish(event("federate.resigned"))
    store.publish(
        {
            "event_type": "session.disconnected",
            "source": "reporter",
            "session_id": " session ",
        }
    )
    assert store.alert_report()["results"][0]["state"] != "ended"
    store.reset_alert("legacy", 1)
    store.publish(
        {
            "event_type": "session.disconnected",
            "source": "reporter",
            "session_id": " session ",
        }
    )
    assert store.alert_report()["results"][0]["state"] != "ended"


def test_named_alert_filters_round_trip_and_reject_forged_mixed_or_unresolved_identity():
    scope = exact_scope()
    spec = AlertRuleSpec(
        "exact", "Exact", "errors", scope=scope, threshold_count=1, window_seconds=30
    )
    value = {
        "configuration_version": "umbra-observer-alerts-v1",
        "rules": [spec.to_json()],
    }
    assert AlertConfiguration.model_validate(value).rules[0].spec() == spec
    for update in ({"source": "reporter"}, {"session_id": " session "}):
        invalid = {
            **value,
            "rules": [{**spec.to_json(), "scope": {**scope.to_json(), **update}}],
        }
        with pytest.raises(ValidationError):
            AlertConfiguration.model_validate(invalid)
    legacy = event_perspective({"source": "reporter", "federate": "Twin"})
    with pytest.raises(ValueError, match="explicit join"):
        replace(spec, scope=AlertScope(perspective=legacy))
    invalid = {
        **value,
        "rules": [
            {
                **spec.to_json(),
                "scope": {
                    "perspective": {**scope.perspective.to_json(), "key": "forged"}
                },
            }
        ],
    }
    with pytest.raises(ValidationError):
        AlertConfiguration.model_validate(invalid)


def test_legacy_alert_session_filters_preserve_opaque_whitespace_at_the_model_boundary():
    value = {
        "configuration_version": "umbra-observer-alerts-v1",
        "rules": [
            {
                "rule_id": "legacy",
                "name": "Legacy",
                "kind": "errors",
                "scope": {"source": "reporter", "session_id": " session "},
                "threshold_count": 1,
                "window_seconds": 30,
            }
        ],
    }
    configuration = AlertConfiguration.model_validate(value)
    spec = configuration.rules[0].spec()
    assert spec.scope.session_id == " session "
    store = ObserverStore()
    store.configure_alerts([spec], 0)
    store.publish(
        {"event_type": "runtime.error", "source": "reporter", "session_id": " session "}
    )
    report = AlertReport.model_validate(store.alert_report())
    assert report.results[0].state == "firing"
    assert report.rules[0].scope.session_id == " session "
