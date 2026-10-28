"""Deterministic synthetic HTTP fixtures shared with the Java fixture server.

Development/test-only helpers: importing the ordinary observer core never
imports this module, FastAPI, or Pydantic. No provider APIs are involved.
"""

# pyright: strict

from __future__ import annotations

import argparse
import json
from collections.abc import Awaitable, Callable, Generator
from contextlib import AbstractContextManager, contextmanager
from pathlib import Path
from typing import Protocol, cast
from unittest.mock import patch
from uuid import UUID

from fastapi import FastAPI, Request
from fastapi.responses import JSONResponse
from fastapi.testclient import TestClient
from httpx import Response as HttpxResponse
from starlette.responses import Response

from .conformance import Fetch, HttpReply
from .core import ObserverStore
from .models import JsonObject
from .server import create_app
from .showcase import ShowcaseAdapter


class _ProfileTestClient(Protocol):
    def get(self, url: str) -> HttpxResponse: ...


@contextmanager
def _open_profile_test_client(
    app: FastAPI,
) -> Generator[_ProfileTestClient, None, None]:
    client = TestClient(app)
    with cast(AbstractContextManager[object], client) as active_client:
        yield cast(_ProfileTestClient, active_client)


PROFILES = (
    "full",
    "limited",
    "partial",
    "empty",
    "retention-loss",
    "no-analytics",
    "reconnect",
    "identity",
)
FIXTURE_ROOT = Path(__file__).resolve().parents[2] / "conformance" / "fixtures"


def _seed_identity(store: ObserverStore) -> None:
    """Same-name joins across independent domains, with no MOM dependency."""
    store.set_context(federation="Shared display name", source="identity-reporter")
    store.set_capabilities(
        {
            "logical_time": {
                "state": "partial",
                "reason": "Only explicit fixture time observations.",
            },
            "time_inspection": {
                "state": "partial",
                "reason": "Explicit synthetic requests/grants are reported; unobserved roles and bounds remain unknown.",
            },
            "declarations": {
                "state": "partial",
                "reason": "Only selected fixture publications are reported; federation-wide declaration coverage is unknown.",
            },
        },
        owner="adapter",
        source="identity-reporter",
    )
    first: JsonObject = {}
    for index, (namespace, federation, membership, source) in enumerate(
        (
            ("namespace-a", "federation-a", "join-1", "identity-reporter"),
            ("namespace-a", "federation-a", "join-2", "identity-reporter"),
            ("namespace-b", "federation-a", "join-1", "identity-reporter"),
            ("namespace-a", "federation-b", "join-1", "identity-reporter"),
            ("namespace-a", "federation-a", "join-1", "other-reporter"),
        )
    ):
        attribution: JsonObject = {
            "session_id": "reporting-session",
            "identity_namespace": namespace,
            "federation_id": federation,
            "participant": {
                "participant_id": "same-participant",
                "membership_id": membership,
                "display_name": "Twin",
                "handle": "7",
            },
        }
        actor: JsonObject = {
            "source": source,
            "federate": "Twin",
            "federation": "Shared display name",
            "participant_attribution": attribution,
        }
        if index == 0:
            first = actor
        store.publish({**actor, "event_type": "federate.joined"})
        for value in (index * 100 + 10, index * 100 + 20):
            store.publish(
                {
                    **actor,
                    "event_type": "object.updated",
                    "object_key": "shared-object",
                    "object_name": "Shared sensor",
                    "object_context": {
                        "identity_namespace": namespace,
                        "federation_id": federation,
                    },
                    "attributes": {"value": value},
                }
            )
        store.publish(
            {
                **actor,
                "event_type": "time.observed",
                "time_observation": {
                    "kind": "request",
                    "request_id": "shared-request",
                    "logical_time": index + 5,
                },
            }
        )
        if index in {1, 2, 3}:
            store.publish(
                {
                    **actor,
                    "event_type": "time.observed",
                    "time_observation": {
                        "kind": "grant",
                        "request_id": "shared-request",
                        "logical_time": index + 5,
                    },
                }
            )
        store.publish(
            {
                **actor,
                "event_type": "declaration.observed",
                "declaration": {
                    "kind": "object",
                    "direction": "publication",
                    "status": "declared",
                    "class_key": f"Class-{index}",
                    "attribute_scope": "selected",
                    "attributes": ["value"],
                },
            }
        )
        store.publish(
            {
                **actor,
                "event_type": "interaction.received",
                "interaction_class": "SharedInteraction",
            }
        )
    original = first["participant_attribution"]
    assert isinstance(original, dict)
    participant = original["participant"]
    assert isinstance(participant, dict)
    renamed: JsonObject = {
        **first,
        "federate": "Renamed Twin",
        "participant_attribution": {
            **original,
            "participant": {
                **participant,
                "display_name": "Renamed Twin",
                "handle": "99",
            },
        },
    }
    store.publish(
        {
            **renamed,
            "event_type": "time.observed",
            "time_observation": {"kind": "state", "logical_time": 25},
        }
    )
    store.publish(
        {
            **renamed,
            "event_type": "object.updated",
            "object_key": "shared-object",
            "object_name": "Shared sensor",
            "object_context": {
                "identity_namespace": "namespace-a",
                "federation_id": "federation-a",
            },
            "attributes": {"value": 25},
        }
    )
    store.publish(
        {
            "event_type": "object.updated",
            "source": "legacy-reporter",
            "federate": "Twin",
            "object_key": "shared-object",
            "attributes": {"value": 999},
        }
    )


@contextmanager
def scenario(profile: str) -> Generator[FastAPI, None, None]:
    if profile not in PROFILES:
        raise ValueError(f"unknown conformance profile: {profile}")
    store = ObserverStore(max_events=3 if profile == "retention-loss" else 64)
    if profile != "empty":
        store.set_capabilities(
            ShowcaseAdapter(
                profile="umbra" if profile == "full" else "limited"
            ).capabilities,
            owner="adapter",
            source="contract-fixture",
        )
    if profile == "partial":
        store.set_capabilities(
            {
                "logical_time": {
                    "state": "partial",
                    "reason": "Only grants are reported.",
                }
            },
            owner="adapter",
            source="contract-fixture",
        )
    # The Java fixture is explicitly HTTP-only, so both sides of the fixture
    # matrix advertise the same transport boundary instead of promising WS.
    app = create_app(
        store,
        enable_analytics=profile != "no-analytics",
        enable_recording=False,
        enable_alert_controls=False,
        enable_workspace_validation=False,
    )
    if profile != "empty":
        from .alerting import AlertRuleSpec

        store.configure_alerts(
            (
                AlertRuleSpec(
                    "fixture-loss",
                    "Fixture history loss",
                    "dropped-history",
                    threshold_count=1,
                ),
                AlertRuleSpec(
                    "fixture-pending",
                    "Fixture pending work",
                    "pending-commands",
                    threshold_seconds=5,
                ),
            ),
            0,
        )
    store.set_capabilities(
        {
            name: {"state": "unsupported", "reason": "This fixture uses HTTP polling."}
            for name in ("websocket", "sse")
        },
        owner="backend",
        source="fastapi",
    )
    if profile not in {"empty", "identity"}:
        store.set_context(federation="ContractFixture", source="contract-fixture")
        if profile == "retention-loss":
            store.publish(
                {
                    "event_type": "time.observed",
                    "source": "contract-fixture",
                    "federate": "FixtureFederate",
                    "time_observation": {
                        "kind": "request",
                        "session_id": "fixture-session",
                        "request_id": "retained-pending",
                        "logical_time": 9,
                    },
                }
            )
        for index in range(6 if profile == "retention-loss" else 4):
            store.publish(
                {
                    "event_type": "object.updated",
                    "family": "generic",
                    "source": "contract-fixture",
                    "federate": "FixtureFederate",
                    "object_key": "fixture-object",
                    "object_name": "FixtureObject",
                    "attributes": {"value": index},
                    "vendor_extension": {"nested": [True, None, "retained"]},
                }
            )
        store.publish(
            {
                "event_type": "callback.invoked",
                "family": "generic",
                "source": "contract-fixture",
                "federate": "FixtureFederate",
                "logical_time": 4,
                "operation": "timeAdvanceGrant",
            }
        )
        if profile != "full":
            store.publish(
                {
                    "event_type": "interaction.received",
                    "source": "contract-fixture",
                    "federate": "FixtureFederate",
                    "interaction_class": "FixtureInteraction",
                }
            )
        if profile == "full":
            for observation in (
                {
                    "kind": "state",
                    "lookahead": 0.5,
                    "regulating": True,
                    "constrained": False,
                    "galt": 3,
                },
                {"kind": "request", "request_id": "advance-1", "logical_time": 5},
                {"kind": "grant", "request_id": "advance-1", "logical_time": 5},
                {"kind": "request", "request_id": "advance-2", "logical_time": 6},
            ):
                store.publish(
                    {
                        "event_type": "time.observed",
                        "source": "contract-fixture",
                        "federate": "FixtureFederate",
                        "federation": "ContractFixture",
                        "time_observation": {
                            "session_id": "fixture-session",
                            **observation,
                        },
                    }
                )
            for declaration in (
                {
                    "kind": "object",
                    "direction": "publication",
                    "class_key": "FixtureClass",
                    "attribute_scope": "selected",
                    "attributes": ["value"],
                },
                {
                    "kind": "object",
                    "direction": "subscription",
                    "class_key": "FixtureClass",
                    "attribute_scope": "unknown",
                },
                {
                    "kind": "interaction",
                    "direction": "subscription",
                    "class_key": "FixtureInteraction",
                },
            ):
                store.publish(
                    {
                        "event_type": "declaration.observed",
                        "source": "contract-fixture",
                        "federate": "FixtureFederate",
                        "federation": "ContractFixture",
                        "declaration": {
                            "status": "declared",
                            "session_id": "fixture-session",
                            **declaration,
                        },
                    }
                )
            store.publish(
                {
                    "event_type": "relationship.observed",
                    "source": "contract-fixture",
                    "relationship": {
                        "relationship_id": "fixture-route",
                        "session_id": "fixture-session",
                        "identity_namespace": "fixture-provider",
                        "federation_id": "fixture-federation",
                        "kind": "object",
                        "evidence": "declared",
                        "status": "active",
                        "viewpoint": "observer",
                        "producer": {
                            "participant_id": "producer",
                            "membership_id": "join-1",
                            "display_name": "FixtureFederate",
                        },
                        "recipient": {
                            "participant_id": "recipient",
                            "membership_id": "join-2",
                            "display_name": "FixtureRecipient",
                        },
                        "class_key": "FixtureClass",
                        "reason": "Explicit fixture edge; not proof of delivery.",
                    },
                }
            )
            for kind, phase, scope, subject in (
                (
                    "synchronization",
                    "registered",
                    "local",
                    {
                        "participant_id": "producer",
                        "membership_id": "join-1",
                        "display_name": "FixtureFederate",
                    },
                ),
                ("synchronization", "completed", "federation", None),
                (
                    "save",
                    "begun",
                    "local",
                    {
                        "participant_id": "producer",
                        "membership_id": "join-1",
                        "display_name": "FixtureFederate",
                    },
                ),
                (
                    "save",
                    "completed",
                    "local",
                    {
                        "participant_id": "producer",
                        "membership_id": "join-1",
                        "display_name": "FixtureFederate",
                    },
                ),
                ("save", "failed", "federation", None),
            ):
                store.publish(
                    {
                        "event_type": "coordination.observed",
                        "source": "contract-fixture",
                        "coordination": {
                            "kind": kind,
                            "operation_id": f"fixture-{kind}",
                            "label": f"Fixture {kind}",
                            "phase": phase,
                            "scope": scope,
                            "session_id": "fixture-session",
                            "identity_namespace": "fixture-provider",
                            "federation_id": "fixture-federation",
                            "subject": subject,
                            "reason": "Explicit fixture phase; local completion is not federation completion.",
                        },
                    }
                )
            for kind in ("send", "receive"):
                store.publish(
                    {
                        "event_type": "interaction.sent"
                        if kind == "send"
                        else "interaction.received",
                        "source": "contract-fixture",
                        "interaction_class": "FixtureInteraction",
                        "interaction_flow": {
                            "kind": kind,
                            "order": "timestamped",
                            "logical_time": 5,
                            "message_id": "fixture-message",
                            "identity_namespace": "fixture-provider",
                            "federation_id": "fixture-federation",
                            "session_id": "fixture-session",
                            "producer": {
                                "participant_id": "producer",
                                "membership_id": "join-1",
                                "display_name": "FixtureFederate",
                            },
                            "recipient": {
                                "participant_id": "recipient",
                                "membership_id": "join-2",
                                "display_name": "FixtureRecipient",
                            },
                            "class_key": "FixtureInteraction",
                        },
                    }
                )
            ownership_subject = {
                "participant_id": "recipient",
                "membership_id": "join-2",
                "display_name": "FixtureRecipient",
            }
            fom_observations: tuple[JsonObject, ...] = (
                {
                    "kind": "object-class",
                    "element_id": "FixtureBase",
                    "parent_state": "root",
                    "field_scope": "declared",
                    "attributes": [],
                },
                {
                    "kind": "object-class",
                    "element_id": "FixtureClass",
                    "parent_state": "parent",
                    "parent_id": "FixtureBase",
                    "field_scope": "declared",
                    "attributes": [
                        {
                            "field_id": "value",
                            "name": "value",
                            "datatype_id": "FixtureNumber",
                        }
                    ],
                },
                {
                    "kind": "interaction-class",
                    "element_id": "FixtureInteraction",
                    "parent_state": "root",
                    "field_scope": "declared",
                    "parameters": [
                        {
                            "field_id": "sample",
                            "name": "sample",
                            "datatype_id": "FixtureNumber",
                        }
                    ],
                },
                {
                    "kind": "datatype",
                    "element_id": "FixtureNumber",
                    "encoding": "primitive",
                    "representation": "FixtureNumeric",
                },
                {
                    "kind": "datatype",
                    "element_id": "FixtureArray",
                    "encoding": "array",
                    "element_type_id": "FixtureNumber",
                    "cardinality": "4",
                },
                {
                    "kind": "datatype",
                    "element_id": "FixtureEnum",
                    "encoding": "enumeration",
                    "enumerators": [
                        {"name": "Off", "value": "0"},
                        {"name": "On", "value": "1"},
                    ],
                },
                {
                    "kind": "dimension",
                    "element_id": "dimension-1",
                    "upper_bound": "18446744073709551615",
                },
            )
            for observation in fom_observations:
                store.publish(
                    {
                        "event_type": "fom.observed",
                        "source": "contract-fixture",
                        "fom_observation": {
                            "session_id": "fixture-session",
                            "identity_namespace": "fixture-provider",
                            "federation_id": "fixture-federation",
                            "model_id": "fixture-model",
                            "revision": "1",
                            "name": observation["element_id"],
                            "status": "present",
                            **observation,
                        },
                    }
                )
            for observation in (
                {
                    "kind": "members",
                    "report_id": "fixture-members",
                    "coverage": "partial",
                    "members": [
                        {
                            "participant": {"identity": ownership_subject},
                            "status": "joined",
                        },
                        {
                            "participant": {
                                "display_name": "Unresolved member",
                                "handle": "0x45",
                            },
                            "status": "unknown",
                        },
                    ],
                },
                {
                    "kind": "service",
                    "report_id": "fixture-service",
                    "service_name": "FixtureService",
                    "status": "failed",
                    "error_name": "FixtureError",
                    "invocation_id": "invoke-1",
                },
                {
                    "kind": "counter",
                    "report_id": "fixture-counter",
                    "counter_key": "reports",
                    "semantics": "cumulative",
                    "value": 0,
                    "unit": "reports",
                },
                {
                    "kind": "counter",
                    "report_id": "fixture-counter",
                    "counter_key": "reports",
                    "semantics": "cumulative",
                    "value": 7,
                    "unit": "reports",
                },
                {
                    "kind": "counter",
                    "report_id": "fixture-opaque",
                    "counter_key": "wide-integer",
                    "semantics": "unknown",
                    "value_text": "18446744073709551615",
                },
            ):
                store.publish(
                    {
                        "event_type": "mom.observed",
                        "source": "contract-fixture",
                        "mom_observation": {
                            "session_id": "fixture-session",
                            "identity_namespace": "fixture-provider",
                            "federation_id": "fixture-federation",
                            **observation,
                        },
                    }
                )
            for observation in (
                {
                    "kind": "region",
                    "region_id": "fixture-region",
                    "extents": [
                        {
                            "extent_id": "e1",
                            "dimensions": [
                                {
                                    "dimension_id": "dimension-1",
                                    "lower_bound": "9007199254740993",
                                    "upper_bound": "9007199254741093",
                                    "dimension_upper_bound": "18446744073709551615",
                                }
                            ],
                        }
                    ],
                },
                {
                    "kind": "association",
                    "association_id": "fixture-association",
                    "target_kind": "object",
                    "target_id": "fixture-object-instance-1",
                    "direction": "update",
                    "region_ids": ["fixture-region"],
                    "attribute_keys": ["value"],
                },
                {
                    "kind": "routing",
                    "routing_id": "fixture-routing",
                    "region_ids": ["fixture-region"],
                    "evidence": "observed",
                    "recipient": ownership_subject,
                },
            ):
                store.publish(
                    {
                        "event_type": "region.observed",
                        "source": "contract-fixture",
                        "region_observation": {
                            "session_id": "fixture-session",
                            "identity_namespace": "fixture-provider",
                            "federation_id": "fixture-federation",
                            "evidence": "declared",
                            "status": "active",
                            **observation,
                        },
                    }
                )
            for observation in (
                {"kind": "state", "owner_state": "unknown", "transition": "reported"},
                {
                    "kind": "request",
                    "request_id": "acquire-1",
                    "action": "acquire",
                    "requester": ownership_subject,
                },
                {
                    "kind": "outcome",
                    "request_id": "acquire-1",
                    "action": "acquire",
                    "requester": ownership_subject,
                    "outcome": "completed",
                },
                {
                    "kind": "state",
                    "owner_state": "known",
                    "owner": ownership_subject,
                    "transition": "acquired",
                },
                {
                    "kind": "request",
                    "request_id": "divest-1",
                    "action": "divest",
                    "requester": ownership_subject,
                },
                {
                    "kind": "state",
                    "attribute_key": "unresolved-value",
                    "owner_state": "unknown",
                    "transition": "reported",
                    "reported_owner_name": "Unresolved owner",
                    "reported_owner_handle": "0x45",
                },
            ):
                store.publish(
                    {
                        "event_type": "ownership.observed",
                        "source": "contract-fixture",
                        "ownership": {
                            "session_id": "fixture-session",
                            "identity_namespace": "fixture-provider",
                            "federation_id": "fixture-federation",
                            "object_id": "fixture-object-instance-1",
                            "attribute_key": "value",
                            **observation,
                        },
                    }
                )
    if profile == "identity":
        _seed_identity(store)
    if profile == "reconnect":
        unavailable = True

        async def transient_outage(
            request: Request, call_next: Callable[[Request], Awaitable[Response]]
        ) -> Response:
            nonlocal unavailable
            if request.url.path == "/api/contract" and unavailable:
                unavailable = False
                return JSONResponse(
                    {"detail": "Synthetic transient outage"}, status_code=503
                )
            return await call_next(request)

        app.middleware("http")(transient_outage)
    try:
        yield app
    finally:
        store.close()


def capture(profile: str) -> dict[str, str]:
    files: dict[str, str] = {}
    # Freeze wall time, not monotonic clocks or async scheduling. JSON fixtures
    # and their digests can then be regenerated and compared without churn.
    with (
        patch("time.time", return_value=1_760_000_000.0),
        patch("umbra_rti_observer.time_views.monotonic", return_value=100.0),
        patch("umbra_rti_observer.interaction_flow.monotonic", return_value=100.0),
        patch("umbra_rti_observer.alerting.monotonic", return_value=100.0),
        patch(
            "umbra_rti_observer.alerting.uuid4", return_value="fixture-alert-instance"
        ),
        patch("umbra_rti_observer.core.uuid4", return_value=UUID(int=1)),
        scenario(profile) as app,
        _open_profile_test_client(app) as client,
    ):
        targets = [
            "/api/contract",
            "/api/health",
            "/api/state?include_events=true",
            "/api/state?include_events=false",
            "/api/metrics",
            "/api/events?after=0&limit=2",
            "/api/events?after=0&limit=500",
            "/api/events?after=-1&limit=2",
            "/api/events?after=0&limit=0",
        ]
        if profile == "reconnect":
            targets.insert(1, "/api/contract")
        if profile != "no-analytics":
            targets.append("/api/analytics")
            targets.extend(
                f"/api/analytics?window_seconds={seconds}&bucket_count=30"
                for seconds in (30, 300, 900, 3600)
            )
        targets.extend(
            f"/api/inspectors/{kind}"
            for kind in ("objects", "interactions", "federates")
        )
        targets.append("/api/alerts")
        manifest: list[str] = []
        for index, target in enumerate(targets):
            response = client.get(target)
            name = f"response-{index:02}.json"
            files[name] = json.dumps(response.json(), indent=2, sort_keys=True) + "\n"
            manifest.append(f"{target}\t{response.status_code}\t{name}")
            if target == "/api/events?after=0&limit=2" and response.json()["has_more"]:
                after = response.json()["next_after"]
                follow = f"/api/events?after={after}&limit=2"
                next_response = client.get(follow)
                next_name = "next-page.json"
                files[next_name] = (
                    json.dumps(next_response.json(), indent=2, sort_keys=True) + "\n"
                )
                manifest.append(f"{follow}\t{next_response.status_code}\t{next_name}")
        files["routes.tsv"] = "\n".join(manifest) + "\n"
    return files


def fixture_fetch(profile: str) -> Fetch:
    """Replay immutable checked-in HTTP responses, including one-shot outages."""
    directory = FIXTURE_ROOT / profile
    routes: dict[str, list[HttpReply]] = {}
    for line in (directory / "routes.tsv").read_text(encoding="utf-8").splitlines():
        target, status, name = line.split("\t")
        routes.setdefault(target, []).append(
            HttpReply(
                status=int(status),
                body=(directory / name).read_text(encoding="utf-8"),
                headers={
                    "Content-Type": "application/json",
                    "X-Umbra-Observer-Contract": "umbra-observer-contract-v1",
                    "X-Umbra-Observer-Media-Type": "application/vnd.umbra.observer+json",
                },
            )
        )

    def fetch(target: str) -> HttpReply:
        replies = routes.get(target)
        if not replies:
            return HttpReply(
                status=404, body='{"detail":"fixture route not configured"}'
            )
        return replies.pop(0) if len(replies) > 1 else replies[0]

    return fetch


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--write",
        action="store_true",
        help="explicitly update checked-in fixture files",
    )
    args = parser.parse_args(argv)
    drift: list[str] = []
    for profile in PROFILES:
        for name, content in capture(profile).items():
            path = FIXTURE_ROOT / profile / name
            if args.write:
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(content, encoding="utf-8")
            elif not path.is_file() or path.read_text(encoding="utf-8") != content:
                drift.append(f"{profile}/{name}")
    if drift:
        print("Conformance fixture drift: " + ", ".join(drift))
        return 1
    print(
        "Shared conformance fixtures " + ("updated." if args.write else "are current.")
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
