"""Read-only, strict-model HTTP interoperability checks; no RTI imports.

These are observer application checks, not IEEE RTI conformance or an audit.
Install the optional conformance extra before importing this module.
"""

# pyright: strict
from __future__ import annotations

import argparse
import json
import math
from collections.abc import Callable, Mapping
from typing import TypeVar
from urllib.error import HTTPError
from urllib.parse import urlencode, urlsplit
from urllib.request import HTTPRedirectHandler, ProxyHandler, Request, build_opener

from pydantic import Field, ValidationError, computed_field

from .command_models import CommandCatalog, CommandHistory
from .contract import OBSERVER_CONTRACT_VERSION, OBSERVER_JSON_MEDIA_TYPE
from .schemas import (
    AnalyticsResponse,
    ContractResponse,
    EventPageResponse,
    HealthResponse,
    MetricsResponse,
    ObserverModel,
    StateResponse,
)

Model = TypeVar("Model", bound=ObserverModel)
OptionalResponse = AnalyticsResponse | CommandCatalog | CommandHistory

# DTO defaults help Python callers construct models. They are not permission
# for a different HTTP implementation to omit collections the UI consumes.
WIRE_REQUIRED_FIELDS: dict[str, set[str]] = {
    "ContractResponse": {"capabilities"},
    "StateResponse": {"context", "adapters", "capabilities", "federate_roster", "inspectors"},
    "InspectorsResponse": {"objects", "interactions"},
    "HealthResponse": {"capabilities"}, "MetricsResponse": {"adapters"},
    "EventPageResponse": {"events"}, "CommandCatalog": {"targets"}, "CommandHistory": {"records"},
}


class HttpReply(ObserverModel):
    status: int
    headers: Mapping[str, str] = Field(default_factory=dict)
    body: str


class ConformanceCheck(ObserverModel):
    name: str
    passed: bool
    detail: str


class ConformanceReport(ObserverModel):
    base_url: str
    scope: str = "observer-http-read-v1"
    checks: tuple[ConformanceCheck, ...]
    omitted_features: tuple[str, ...]
    unchecked_features: tuple[str, ...] = ("stream-delivery", "command-execution", "rti-semantics")

    @computed_field
    @property
    def passed(self) -> bool:
        return bool(self.checks) and all(check.passed for check in self.checks)


Fetch = Callable[[str], HttpReply]


class _NoRedirect(HTTPRedirectHandler):
    def redirect_request(self, req: Request, fp: object, code: int, msg: str,
                         headers: object, newurl: str) -> None:
        return None


def local_path(value: str) -> str:
    parsed = urlsplit(value)
    if (not value.startswith("/") or value.startswith("//") or parsed.scheme
            or parsed.netloc or parsed.fragment or parsed.query or "\\" in value
            or any(ord(char) < 32 for char in value)):
        raise ValueError("discovered paths must be query-free same-origin absolute paths")
    return value


class ObserverConformance:
    """Check only GET resources; never mutate a federation or invoke commands.

    Optional resources are checked only when advertised as usable. Stream
    delivery, command execution and RTI semantics remain separate test lanes.
    """

    def __init__(self, base_url: str, *, timeout: float = 5, fetch: Fetch | None = None) -> None:
        parsed = urlsplit(base_url)
        if (parsed.scheme not in {"http", "https"} or not parsed.netloc or parsed.query
                or parsed.fragment or parsed.path not in {"", "/"} or parsed.username or parsed.password):
            raise ValueError("base_url must be an HTTP(S) origin without credentials or a path")
        if not math.isfinite(timeout) or timeout <= 0:
            raise ValueError("timeout must be positive and finite")
        self.base_url = base_url.rstrip("/")
        self.timeout = timeout
        self.fetch = fetch or self._fetch_http

    def _fetch_http(self, path: str) -> HttpReply:
        loopback = urlsplit(self.base_url).hostname in {"127.0.0.1", "localhost", "::1"}
        opener = build_opener(_NoRedirect(), *([ProxyHandler({})] if loopback else []))
        try:
            response = opener.open(Request(self.base_url + path, headers={"Accept": "application/json"}), timeout=self.timeout)
        except HTTPError as error:
            response = error
        with response:
            body = response.read(8 * 1024 * 1024 + 1)
            if len(body) > 8 * 1024 * 1024:
                raise ValueError("response exceeds the 8 MiB conformance limit")
            return HttpReply(status=response.code, headers=dict(response.headers), body=body.decode("utf-8"))

    def run(self) -> ConformanceReport:
        checks: list[ConformanceCheck] = []
        omitted: list[str] = []

        def check(name: str, passed: bool, detail: str) -> None:
            checks.append(ConformanceCheck(name=name, passed=passed, detail=detail))

        def read(name: str, path: str, model: type[Model]) -> Model | None:
            try:
                reply = self.fetch(path)
                headers = {key.lower(): value for key, value in reply.headers.items()}
                check(name + ".http", reply.status == 200, f"GET {path}: HTTP {reply.status}")
                if reply.status != 200:
                    return None
                check(name + ".headers", headers.get("x-umbra-observer-contract") == OBSERVER_CONTRACT_VERSION
                      and headers.get("x-umbra-observer-media-type") == OBSERVER_JSON_MEDIA_TYPE
                      and headers.get("content-type", "").split(";", 1)[0].strip().lower()
                      in {"application/json", OBSERVER_JSON_MEDIA_TYPE}, "version and JSON media headers")
                json.dumps(json.loads(reply.body), allow_nan=False)
                value = model.model_validate_json(reply.body)
                check(name + ".model", True, model.__name__ + " validated without coercion")
                missing = WIRE_REQUIRED_FIELDS.get(model.__name__, set()) - value.model_fields_set
                check(name + ".wire_fields", not missing, "missing wire collections: " + ", ".join(sorted(missing)))
                if isinstance(value, StateResponse):
                    check(name + ".inspectors", {"objects", "interactions"} <= value.inspectors.model_fields_set,
                          "inspector arrays must be present even when empty")
                    time_inspection = value.inspectors.time
                    time_capability = value.capabilities.get("time_inspection")
                    time_usable = time_capability is not None and time_capability.state in {"available", "partial"}
                    check(name + ".time.advertised", not time_usable or time_inspection is not None,
                          "usable time inspection includes its named model")
                    if time_inspection is not None:
                        check(name + ".time.cursors", all(row.last_sequence < value.retention.next_sequence
                              for row in time_inspection.snapshots), "persistent time snapshots use existing store sequences")
                    declarations = value.inspectors.declarations
                    declaration_capability = value.capabilities.get("declarations")
                    declarations_usable = declaration_capability is not None and declaration_capability.state in {"available", "partial"}
                    check(name + ".declarations.advertised", not declarations_usable or declarations is not None,
                          "usable declaration inspection includes its named snapshot model")
                    if declarations is not None:
                        check(name + ".declarations.cursors", all(row.sequence < value.retention.next_sequence
                              for row in declarations.snapshots), "declarations reference existing observation sequences")
                    observations = value.inspectors.object_observations
                    capability = value.capabilities.get("object_observations")
                    usable = capability is not None and capability.state in {"available", "partial"}
                    check(name + ".object_observations.advertised", not usable or observations is not None,
                          "usable object perspectives include their named snapshot/history model")
                    if observations is not None:
                        keys = [(row.federate, row.object.object_key) for row in observations.snapshots]
                        snapshot_keys = set(keys)
                        check(name + ".object_observations.snapshots", len(keys) == len(snapshot_keys)
                              and all(row.generation <= row.last_sequence < value.retention.next_sequence
                                      for row in observations.snapshots), "unique scopes and valid snapshot cursors")
                        changes = observations.changes
                        ids = [(row.sequence, row.federate, row.object_key, row.attribute) for row in changes]
                        check(name + ".object_observations.history", len(ids) == len(set(ids))
                              and [row.sequence for row in changes] == sorted(row.sequence for row in changes)
                              and observations.history_first_sequence == value.retention.first_retained_sequence
                              and observations.history_last_sequence == value.retention.last_sequence
                              and observations.history_truncated == bool(value.retention.dropped_events or value.retention.cleared_events)
                              and all(observations.history_first_sequence <= row.sequence <= observations.history_last_sequence
                                      and row.generation <= row.sequence
                                      and (row.previous_known or row.previous_value is None)
                                      and (row.federate, row.object_key) in snapshot_keys for row in changes),
                              "attribute history is bounded by the event window and references known scopes")
                return value
            except (ValidationError, ValueError, TypeError, OSError, RecursionError) as error:
                check(name + ".model", False, str(error)[:1600])
                return None

        contract = read("contract", "/api/contract", ContractResponse)
        if contract is None:
            return ConformanceReport(base_url=self.base_url, checks=tuple(checks), omitted_features=())
        check("contract.version", contract.contract_version == OBSERVER_CONTRACT_VERSION, contract.contract_version)
        check("contract.media", contract.media_type == OBSERVER_JSON_MEDIA_TYPE, contract.media_type)
        check("contract.capability_states", set(contract.capability_schema.states)
              == {"available", "partial", "disabled", "unsupported", "unknown"}, "all five capability meanings are declared")
        paths_valid = True
        for name, path in contract.api_paths.items():
            try:
                local_path(path)
            except ValueError as error:
                paths_valid = False
                check("discovery." + name, False, str(error))
        if not paths_valid:
            return ConformanceReport(base_url=self.base_url, checks=tuple(checks), omitted_features=())
        check("discovery.required", all(key in contract.api_paths for key in ("contract", "state", "events", "metrics")),
              "contract, state, events and metrics paths are required")
        check("discovery.bootstrap", contract.api_paths.get("contract") == "/api/contract", "contract bootstrap path is stable")
        check("discovery.descriptions", contract.snapshot.method == "GET" and contract.events.method == "GET"
              and contract.snapshot.path == contract.api_paths.get("state")
              and contract.events.path == contract.api_paths.get("events"), "resource descriptions agree with discovered paths")

        def version(name: str, actual: str, schema: str | None = None) -> None:
            check(name + ".version", actual == contract.contract_version, actual)
            if schema is not None:
                check(name + ".event_schema", schema == contract.event_schema_version, schema)

        health = read("health", "/api/health", HealthResponse)
        if health:
            version("health", health.contract_version, health.schema_version)
        state_path = contract.api_paths.get("state")
        if state_path:
            state = read("state", state_path + "?include_events=true", StateResponse)
            if state:
                version("state", state.contract_version, state.schema_version)
                check("state.required_collections", state.normalized_events is not None,
                      "include_events=true includes normalized_events, including an empty array")
                check("state.retention", state.retention.retained_events <= state.retention.max_events
                      and state.retention.next_sequence > state.retention.last_sequence
                      and state.live_metrics.event_count == state.retention.retained_events,
                      "retention capacity, next cursor and retained count agree")
                if state.normalized_events is not None:
                    sequences = [row.sequence for row in state.normalized_events]
                    check("state.sequences", sequences == sorted(set(sequences))
                          and len(sequences) == state.retention.retained_events
                          and all(state.retention.first_retained_sequence <= sequence <= state.retention.last_sequence
                                  for sequence in sequences), "snapshot rows cover retained history in strict sequence order")
            compact = read("state_compact", state_path + "?include_events=false", StateResponse)
            if compact:
                version("state_compact", compact.contract_version, compact.schema_version)
                check("state_compact.events", not compact.normalized_events, "include_events=false omits event history")
        metrics_path = contract.api_paths.get("metrics")
        if metrics_path:
            metrics = read("metrics", metrics_path, MetricsResponse)
            if metrics:
                version("metrics", metrics.contract_version, metrics.schema_version)
        events_path = contract.api_paths.get("events")
        if events_path:
            after = 0
            for page_number in range(2):
                name = f"events{page_number}"
                page = read(name, events_path + "?" + urlencode({"after": after, "limit": 2}), EventPageResponse)
                if page is None:
                    break
                version(name, page.contract_version)
                sequences = [row.sequence for row in page.events]
                check(name + ".cursor", sequences == sorted(set(sequences))
                      and all(after < sequence <= page.last_sequence for sequence in sequences)
                      and len(sequences) <= 2 and page.next_after == (sequences[-1] if sequences else after),
                      "unique ordered rows, limit, exclusive cursor and next_after")
                check(name + ".retention", len(sequences) <= page.retained_events
                      and (not sequences or min(sequences) >= page.first_retained_sequence)
                      and page.gap == bool(page.first_retained_sequence and after < page.first_retained_sequence - 1)
                      and (not page.has_more or bool(sequences)), "retention bounds and gap marker agree")
                if not page.has_more:
                    break
                after = page.next_after
            for query in ({"after": -1, "limit": 2}, {"after": 0, "limit": 0}):
                target = events_path + "?" + urlencode(query)
                try:
                    reply = self.fetch(target)
                    check("events.invalid_query", reply.status in {400, 422}, f"GET {target}: HTTP {reply.status}")
                except (OSError, ValueError, TypeError) as error:
                    check("events.invalid_query", False, str(error))
        optional_models: tuple[tuple[str, tuple[tuple[str, type[OptionalResponse]], ...]], ...] = (
            ("analytics", (("analytics", AnalyticsResponse),)),
            ("commands", (("command_catalog", CommandCatalog), ("commands", CommandHistory))),
        )
        for feature, resources in optional_models:
            capability = contract.capabilities.get(feature)
            if capability is None or capability.state not in {"available", "partial"}:
                omitted.append(feature)
                continue
            for resource, response_model in resources:
                path = contract.api_paths.get(resource)
                check(resource + ".advertised", bool(path), "usable feature advertises its endpoint")
                if path:
                    payload = read(resource, path, response_model)
                    if payload:
                        version(resource, payload.contract_version)
                        if isinstance(payload, CommandCatalog):
                            check("command_catalog.enabled", payload.enabled, "advertised command service is enabled")
                        if isinstance(payload, AnalyticsResponse):
                            check("analytics.counts", payload.window_start <= payload.generated_at
                                  and sum(row.events for row in payload.activity) == payload.events_in_window
                                  and sum(payload.event_types.values()) == payload.events_in_window
                                  and sum(payload.sources.values()) == payload.events_in_window
                                  and sum(payload.families.values()) == payload.events_in_window,
                                  "window and disjoint activity/type/source/family counts agree")
        return ConformanceReport(base_url=self.base_url, checks=tuple(checks), omitted_features=tuple(omitted))


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("base_url")
    parser.add_argument("--timeout", type=float, default=5)
    args = parser.parse_args(argv)
    try:
        report = ObserverConformance(args.base_url, timeout=args.timeout).run()
    except ValueError as error:
        parser.error(str(error))
    print(report.model_dump_json(indent=2))
    return 0 if report.passed else 1


if __name__ == "__main__":
    raise SystemExit(main())
