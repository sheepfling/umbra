from __future__ import annotations

from umbra_rti_observer import OBSERVER_CONTRACT_VERSION
from umbra_rti_observer.contract import build_contract_document
from umbra_rti_observer.core import EVENT_TYPES, OBSERVER_SCHEMA_VERSION
from umbra_rti_observer.models import JsonObject
from umbra_rti_observer.probe import ObserverContractProbe


def _responses() -> dict[str, JsonObject]:
    contract = build_contract_document(
        EVENT_TYPES, event_schema_version=OBSERVER_SCHEMA_VERSION
    )
    return {
        "/api/contract": contract,
        "/api/health": {
            "contract_version": OBSERVER_CONTRACT_VERSION,
            "status": "running",
            "event_count": 1,
            "capabilities": {},
        },
        "/api/state?include_events=false": {
            "contract_version": OBSERVER_CONTRACT_VERSION,
            "schema_version": OBSERVER_SCHEMA_VERSION,
            "status": "running",
            "context": {},
            "live_metrics": {},
            "retention": {},
            "adapters": [],
            "capabilities": {"lifecycle": {"state": "available"}},
            "federate_roster": [],
            "inspectors": {"objects": [], "interactions": []},
        },
        "/api/metrics": {
            "contract_version": OBSERVER_CONTRACT_VERSION,
            "schema_version": OBSERVER_SCHEMA_VERSION,
            "status": "running",
            "live_metrics": {},
            "retention": {},
            "adapters": [],
        },
        "/api/events?after=0&limit=20": {
            "contract_version": OBSERVER_CONTRACT_VERSION,
            "events": [
                {
                    "sequence": 1,
                    "event_type": "runtime.started",
                    "family": "generic",
                    "source": "test",
                    "observed_at": 1.0,
                }
            ],
            "next_after": 1,
            "has_more": False,
            "gap": False,
            "first_retained_sequence": 1,
            "last_sequence": 1,
            "retained_events": 1,
            "dropped_events": 0,
        },
    }


def test_probe_accepts_a_contract_compatible_backend() -> None:
    responses = _responses()
    result = ObserverContractProbe(
        "http://backend", fetch=lambda path: (200, responses[path])
    ).run()

    assert result.passed is True
    assert all(check.passed for check in result.checks)


def test_probe_reports_missing_backend_fields() -> None:
    responses = _responses()
    responses["/api/metrics"] = {"status": "running"}
    result = ObserverContractProbe(
        "http://backend", fetch=lambda path: (200, responses[path])
    ).run()

    assert result.passed is False
    assert any(
        check.name == "metrics_fields" and not check.passed for check in result.checks
    )


def test_probe_rejects_malformed_event_envelopes() -> None:
    responses = _responses()
    responses["/api/events?after=0&limit=20"] = {
        **responses["/api/events?after=0&limit=20"],
        "events": [{"sequence": "one"}],
    }
    result = ObserverContractProbe(
        "http://backend", fetch=lambda path: (200, responses[path])
    ).run()

    assert result.passed is False
    assert any(
        check.name == "event_envelopes" and not check.passed for check in result.checks
    )
