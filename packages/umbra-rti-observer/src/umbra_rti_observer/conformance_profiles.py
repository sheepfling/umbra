"""Deterministic synthetic HTTP fixtures shared with the Java fixture server.

Development/test-only helpers: importing the ordinary observer core never
imports this module, FastAPI, or Pydantic. No provider APIs are involved.
"""

# pyright: strict

from __future__ import annotations

import argparse
import json
from collections.abc import Awaitable, Callable, Generator
from contextlib import contextmanager
from pathlib import Path
from unittest.mock import patch

from fastapi import FastAPI, Request
from fastapi.responses import JSONResponse
from fastapi.testclient import TestClient
from starlette.responses import Response

from .conformance import Fetch, HttpReply
from .core import ObserverStore
from .server import create_app
from .showcase import ShowcaseAdapter

PROFILES = ("full", "limited", "partial", "empty", "retention-loss", "no-analytics", "reconnect")
FIXTURE_ROOT = Path(__file__).resolve().parents[2] / "conformance" / "fixtures"


@contextmanager
def scenario(profile: str) -> Generator[FastAPI, None, None]:
    if profile not in PROFILES:
        raise ValueError(f"unknown conformance profile: {profile}")
    store = ObserverStore(max_events=3 if profile == "retention-loss" else 20)
    if profile != "empty":
        store.set_capabilities(ShowcaseAdapter(profile="umbra" if profile == "full" else "limited").capabilities,
                               owner="adapter", source="contract-fixture")
    if profile == "partial":
        store.set_capabilities({"logical_time": {"state": "partial", "reason": "Only grants are reported."}},
                               owner="adapter", source="contract-fixture")
    # The Java fixture is explicitly HTTP-only, so both sides of the fixture
    # matrix advertise the same transport boundary instead of promising WS.
    app = create_app(store, enable_analytics=profile != "no-analytics")
    store.set_capabilities({name: {"state": "unsupported", "reason": "This fixture uses HTTP polling."}
                            for name in ("websocket", "sse")}, owner="backend", source="fastapi")
    if profile != "empty":
        store.set_context(federation="ContractFixture", source="contract-fixture")
        if profile == "retention-loss":
            store.publish({"event_type": "time.observed", "source": "contract-fixture", "federate": "FixtureFederate",
                           "time_observation": {"kind": "request", "session_id": "fixture-session", "request_id": "retained-pending", "logical_time": 9}})
        for index in range(6 if profile == "retention-loss" else 4):
            store.publish({"event_type": "object.updated", "family": "generic", "source": "contract-fixture",
                           "federate": "FixtureFederate", "object_key": "fixture-object",
                           "object_name": "FixtureObject", "attributes": {"value": index},
                           "vendor_extension": {"nested": [True, None, "retained"]}})
        store.publish({"event_type": "callback.invoked", "family": "generic", "source": "contract-fixture",
                       "federate": "FixtureFederate", "logical_time": 4, "operation": "timeAdvanceGrant"})
        if profile == "full":
            for observation in (
                {"kind": "state", "lookahead": 0.5, "regulating": True, "constrained": False, "galt": 3},
                {"kind": "request", "request_id": "advance-1", "logical_time": 5},
                {"kind": "grant", "request_id": "advance-1", "logical_time": 5},
                {"kind": "request", "request_id": "advance-2", "logical_time": 6},
            ):
                store.publish({"event_type": "time.observed", "source": "contract-fixture", "federate": "FixtureFederate",
                               "federation": "ContractFixture", "time_observation": {"session_id": "fixture-session", **observation}})
            for declaration in (
                {"kind": "object", "direction": "publication", "class_key": "FixtureClass", "attribute_scope": "selected", "attributes": ["value"]},
                {"kind": "object", "direction": "subscription", "class_key": "FixtureClass", "attribute_scope": "unknown"},
                {"kind": "interaction", "direction": "subscription", "class_key": "FixtureInteraction"},
            ):
                store.publish({"event_type": "declaration.observed", "source": "contract-fixture", "federate": "FixtureFederate",
                               "federation": "ContractFixture", "declaration": {"status": "declared", "session_id": "fixture-session", **declaration}})
    if profile == "reconnect":
        unavailable = True

        async def transient_outage(request: Request, call_next: Callable[[Request], Awaitable[Response]]) -> Response:
            nonlocal unavailable
            if request.url.path == "/api/contract" and unavailable:
                unavailable = False
                return JSONResponse({"detail": "Synthetic transient outage"}, status_code=503)
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
    with (patch("time.time", return_value=1_760_000_000.0),
          patch("umbra_rti_observer.time_views.monotonic", return_value=100.0),
          scenario(profile) as app, TestClient(app) as client):
        targets = ["/api/contract", "/api/health", "/api/state?include_events=true",
                   "/api/state?include_events=false", "/api/metrics", "/api/events?after=0&limit=2",
                   "/api/events?after=0&limit=500", "/api/events?after=-1&limit=2", "/api/events?after=0&limit=0"]
        if profile == "reconnect":
            targets.insert(1, "/api/contract")
        if profile != "no-analytics":
            targets.append("/api/analytics")
            targets.extend(f"/api/analytics?window_seconds={seconds}&bucket_count=30" for seconds in (30, 300, 900, 3600))
        targets.extend(f"/api/inspectors/{kind}" for kind in ("objects", "interactions", "federates"))
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
                files[next_name] = json.dumps(next_response.json(), indent=2, sort_keys=True) + "\n"
                manifest.append(f"{follow}\t{next_response.status_code}\t{next_name}")
        files["routes.tsv"] = "\n".join(manifest) + "\n"
    return files


def fixture_fetch(profile: str) -> Fetch:
    """Replay immutable checked-in HTTP responses, including one-shot outages."""
    directory = FIXTURE_ROOT / profile
    routes: dict[str, list[HttpReply]] = {}
    for line in (directory / "routes.tsv").read_text(encoding="utf-8").splitlines():
        target, status, name = line.split("\t")
        routes.setdefault(target, []).append(HttpReply(status=int(status), body=(directory / name).read_text(encoding="utf-8"),
            headers={"Content-Type": "application/json", "X-Umbra-Observer-Contract": "umbra-observer-contract-v1",
                     "X-Umbra-Observer-Media-Type": "application/vnd.umbra.observer+json"}))

    def fetch(target: str) -> HttpReply:
        replies = routes.get(target)
        if not replies:
            return HttpReply(status=404, body='{"detail":"fixture route not configured"}')
        return replies.pop(0) if len(replies) > 1 else replies[0]
    return fetch


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--write", action="store_true", help="explicitly update checked-in fixture files")
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
    print("Shared conformance fixtures " + ("updated." if args.write else "are current."))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
