"""Typed inputs shared by the bounded query-work regression suites."""

from __future__ import annotations

from dataclasses import dataclass

import query_rti_work


@dataclass(frozen=True, slots=True)
class LaneCatalogInputs:
    index: query_rti_work.JsonObject
    plan: query_rti_work.JsonObject
    process_item: query_rti_work.JsonObject | None
    process_lane: query_rti_work.JsonObject
    process_rows: list[query_rti_work.JsonObject]
    queue_by_id: dict[str, query_rti_work.JsonObject]
    source_health: query_rti_work.TestSourceHealth
    source_locations: dict[str, list[query_rti_work.TestSourceLocation]]
    standard_requirements: dict[str, query_rti_work.JsonObject]
    tests: list[query_rti_work.JsonObject]


@dataclass(frozen=True, slots=True)
class MappingRowInputs:
    contract_links: dict[str, list[query_rti_work.JsonObject]]
    declared_executables: set[str]
    index: query_rti_work.JsonObject
    ownership_transfer: query_rti_work.JsonObject | None
    standard_requirements: dict[str, query_rti_work.JsonObject]
    tests: list[query_rti_work.JsonObject]


@dataclass(frozen=True, slots=True)
class MappingLookupInputs:
    index: query_rti_work.JsonObject
    tests: list[query_rti_work.JsonObject]


@dataclass(frozen=True, slots=True)
class FamilyMatrixInputs:
    active_handoff: query_rti_work.JsonObject | None
    index: query_rti_work.JsonObject
    queue: query_rti_work.JsonObject
    restored_default_region_multi: query_rti_work.JsonObject
    source_locations: dict[str, list[query_rti_work.TestSourceLocation]]
    standard_requirements: dict[str, query_rti_work.JsonObject]
    status: query_rti_work.JsonObject
    tests: list[query_rti_work.JsonObject]


@dataclass(frozen=True, slots=True)
class LaneSearchInputs:
    index: query_rti_work.JsonObject
    pairs: list[query_rti_work.JsonValue]
    source_locations: dict[str, list[query_rti_work.TestSourceLocation]]
    tests: list[query_rti_work.JsonObject]
