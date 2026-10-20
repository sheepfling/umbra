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

from .alert_models import AlertReport
from .command_models import CommandCatalog, CommandHistory
from .contract import OBSERVER_CONTRACT_VERSION, OBSERVER_JSON_MEDIA_TYPE
from .recording_models import RecordingStatusResponse
from .schemas import (
    WIRE_REQUIRED_FIELDS,
    AnalyticsResponse,
    ContractResponse,
    EventPageResponse,
    HealthResponse,
    MetricsResponse,
    ObserverModel,
    StateResponse,
)
from .workspace_models import WorkspaceDefaultsResponse

Model = TypeVar("Model", bound=ObserverModel)
OptionalResponse = AnalyticsResponse | CommandCatalog | CommandHistory | RecordingStatusResponse | AlertReport | WorkspaceDefaultsResponse

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
    unchecked_features: tuple[str, ...] = ("stream-delivery", "command-execution", "owned-session-recovery", "recording-capture-import-replay", "storage-eviction", "alert-rule-evaluation", "alert-configuration-mutation", "workspace-import-validation", "rti-semantics")

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
                    relationships = value.inspectors.relationships
                    coordination = value.inspectors.coordination
                    ownership = value.inspectors.ownership
                    regions = value.inspectors.regions
                    mom = value.inspectors.mom
                    fom = value.inspectors.fom
                    fom_capability = value.capabilities.get("fom_inspection")
                    fom_usable = fom_capability is not None and fom_capability.state in {"available", "partial"}
                    check(name + ".fom.advertised", not fom_usable or fom is not None,
                          "usable FOM inspection includes named adapter metadata models")
                    if fom is not None:
                        check(name + ".fom.cursors", all(row.sequence < value.retention.next_sequence for row in (*fom.snapshots, *fom.transitions)),
                              "FOM metadata references existing observation sequences")
                    mom_capability = value.capabilities.get("mom_inspection")
                    mom_usable = mom_capability is not None and mom_capability.state in {"available", "partial"}
                    check(name + ".mom.advertised", not mom_usable or mom is not None,
                          "usable management inspection includes typed coverage, service and counter models")
                    if mom is not None:
                        check(name + ".mom.cursors", all(row.sequence < value.retention.next_sequence for row in (*mom.snapshots, *mom.transitions)),
                              "management reports reference existing observation sequences")
                    regions_capability = value.capabilities.get("regions_inspection")
                    regions_usable = regions_capability is not None and regions_capability.state in {"available", "partial"}
                    check(name + ".regions.advertised", not regions_usable or regions is not None,
                          "usable region inspection includes exact extents and explicit association/routing models")
                    if regions is not None:
                        check(name + ".regions.cursors", all(row.sequence < value.retention.next_sequence for row in (*regions.snapshots, *regions.transitions)),
                              "region reports reference existing observation sequences")
                    ownership_capability = value.capabilities.get("ownership_inspection")
                    ownership_usable = ownership_capability is not None and ownership_capability.state in {"available", "partial"}
                    check(name + ".ownership.advertised", not ownership_usable or ownership is not None,
                          "usable ownership inspection includes its named owner/request evidence model")
                    if ownership is not None:
                        cursors = [row.sequence for row in (*ownership.states, *ownership.transitions)]
                        cursors.extend(row.last_sequence for row in ownership.requests)
                        check(name + ".ownership.cursors", all(sequence < value.retention.next_sequence for sequence in cursors),
                              "ownership evidence references existing observation sequences")
                    flow = value.inspectors.interaction_flow
                    flow_capability = value.capabilities.get("interaction_flow")
                    flow_usable = flow_capability is not None and flow_capability.state in {"available", "partial"}
                    check(name + ".interaction_flow.advertised", not flow_usable or flow is not None,
                          "usable interaction flow includes its named evidence model")
                    if flow is not None:
                        check(name + ".interaction_flow.cursors", all(row.last_sequence < value.retention.next_sequence for row in flow.records),
                              "interaction records reference existing observation sequences")
                    coordination_usable = any(feature in value.capabilities and value.capabilities[feature].state in {"available", "partial"}
                                              for feature in ("synchronization", "persistence_inspection"))
                    check(name + ".coordination.advertised", not coordination_usable or coordination is not None,
                          "usable coordination inspection includes named local/federation phase models")
                    if coordination is not None:
                        check(name + ".coordination.cursors", all(row.sequence < value.retention.next_sequence
                              for row in (*coordination.snapshots, *coordination.transitions)), "coordination reports reference existing observation sequences")
                    relationship_capability = value.capabilities.get("relationships")
                    relationships_usable = relationship_capability is not None and relationship_capability.state in {"available", "partial"}
                    check(name + ".relationships.advertised", not relationships_usable or relationships is not None,
                          "usable relationship inspection includes explicit scoped endpoint models")
                    if relationships is not None:
                        check(name + ".relationships.cursors", all(row.sequence < value.retention.next_sequence
                              for row in relationships.snapshots), "relationships reference existing observation sequences")
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
                        keys = [row.cache_key for row in observations.snapshots]
                        snapshot_keys = set(keys)
                        check(name + ".object_observations.snapshots", len(keys) == len(snapshot_keys)
                              and all(row.generation <= row.last_sequence < value.retention.next_sequence
                                      for row in observations.snapshots), "unique scopes and valid snapshot cursors")
                        changes = observations.changes
                        ids = [(row.sequence, row.cache_key, row.attribute) for row in changes]
                        check(name + ".object_observations.history", len(ids) == len(set(ids))
                              and [row.sequence for row in changes] == sorted(row.sequence for row in changes)
                              and observations.history_first_sequence == value.retention.first_retained_sequence
                              and observations.history_last_sequence == value.retention.last_sequence
                              and observations.history_truncated == bool(value.retention.dropped_events or value.retention.cleared_events or observations.omitted_changes or observations.dropped_history_batches)
                              and all(observations.history_first_sequence <= row.sequence <= observations.history_last_sequence
                                      and row.generation <= row.sequence
                                      and (row.previous_known or row.previous_value is None)
                                      and row.cache_key in snapshot_keys for row in changes),
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
                        and len(sequences) <= 2 and page.next_after == (sequences[-1] if sequences else
                            (page.next_sequence - 1 if page.gap and not page.retained_events and page.next_sequence is not None else after)),
                      "unique ordered rows, limit, exclusive cursor and next_after")
                check(name + ".retention", len(sequences) <= page.retained_events
                      and (not sequences or min(sequences) >= page.first_retained_sequence)
                        and page.gap == bool(page.first_retained_sequence and after < page.first_retained_sequence - 1
                            or not page.retained_events and page.next_sequence is not None and after < page.next_sequence - 1)
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
            ("workspace_validation", (("workspace_defaults", WorkspaceDefaultsResponse),)),
            ("alerts", (("alerts", AlertReport),)),
            ("analytics", (("analytics", AnalyticsResponse),)),
            ("commands", (("command_catalog", CommandCatalog), ("commands", CommandHistory))),
            ("recording", (("recordings", RecordingStatusResponse),)),
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
                            conditional = contract.capabilities.get("command_versions")
                            if conditional is not None and conditional.state in {"available", "partial"}:
                                check("command_catalog.versions", all(target.version is not None for target in payload.targets),
                                      "conditional commands identify each owned target incarnation/revision")
                            recovery = contract.capabilities.get("session_recovery")
                            if recovery is not None and recovery.state in {"available", "partial"}:
                                check("command_catalog.recovery", any(target.version is not None
                                    and {"session.inspect", "session.recover"}.issubset({row.action for row in target.commands if row.state != "unsupported"})
                                    for target in payload.targets), "recovery advertises versioned owned inspection/reconciliation; no action is executed")
                        if isinstance(payload, AnalyticsResponse):
                            check("analytics.counts", payload.window_start <= payload.generated_at
                                  and sum(row.events for row in payload.activity) == payload.events_in_window
                                  and sum(payload.event_types.values()) == payload.events_in_window
                                  and sum(payload.sources.values()) == payload.events_in_window
                                  and sum(payload.families.values()) == payload.events_in_window,
                                  "window and disjoint activity/type/source/family counts agree")
        recovery_capability = contract.capabilities.get("session_recovery")
        if recovery_capability is not None and recovery_capability.state in {"available", "partial"}:
            check("session_recovery.dependencies", all(contract.capabilities.get(name) is not None
                and contract.capabilities[name].state in {"available", "partial"} for name in ("commands", "command_versions")),
                "owned recovery depends on command dispatch and explicit target versions, never observation identity")
        workspace_capability = contract.capabilities.get("workspace_validation")
        if workspace_capability is not None and workspace_capability.state in {"available", "partial"}:
            check("workspace_validation.discovery", bool(contract.api_paths.get("workspace_validate")),
                  "workspace validation advertises its named normalization endpoint; no file is applied")
        alert_controls = contract.capabilities.get("alert_controls")
        if alert_controls is not None and alert_controls.state in {"available", "partial"}:
            alerts = contract.capabilities.get("alerts")
            check("alert_controls.discovery", alerts is not None and alerts.state in {"available", "partial"}
                and all(contract.api_paths.get(path) for path in ("alert_configuration", "alert_reset", "alert_acknowledge")),
                "alert controls require usable alert reads and discovered mutation resources")
        window_capability = contract.capabilities.get("recording_analytics_window")
        if window_capability is not None and window_capability.state in {"available", "partial"}:
            recording_capability = contract.capabilities.get("recording")
            check("recording_analytics_window.discovery", recording_capability is not None
                  and recording_capability.state in {"available", "partial"}
                  and bool(contract.api_paths.get("recording_analyze")),
                  "lightweight recorded analytics requires usable recording and its discovered analysis resource")
        compact_capability = contract.capabilities.get("recording_compact")
        if compact_capability is not None and compact_capability.state in {"available", "partial"}:
            recording_capability = contract.capabilities.get("recording")
            check("recording_compact.discovery", recording_capability is not None
                  and recording_capability.state in {"available", "partial"}
                  and all(contract.api_paths.get(path) for path in ("recording_start", "recording_export", "recording_validate")),
                  "compact files require usable recording and discovered capture/export/import resources")
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
