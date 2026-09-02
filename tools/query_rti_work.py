#!/usr/bin/env python3
"""Query the Umbra roadmap and native C++ test-to-standard mappings.

The Requirements Lab export and the Catch2 plan remain the sources of truth.
This tool only joins them for fast, read-only work selection; ``work`` resolves
one bounded implementation slice, ``focus`` resolves one exact lane's
candidate/status card, and ``trace`` resolves one direct
test-to-requirement-to-2025-subsection mapping.  It never edits the Lab, the
plan, or generated evidence.  ``unplanned`` is a one-way source-to-plan
reconciliation view; it never invents a requirement or implementation status.
Roadmap items may carry an explicit ``planned`` source-slice pointer; that
pointer is a bounded future test contract and is not counted as executable
evidence until its Catch2 declaration is added.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from collections import Counter, defaultdict
from pathlib import Path
from typing import Any, Iterable


REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_INDEX = REPOSITORY_ROOT / "docs" / "planning" / "ROADMAP-INDEX.json"
DEFAULT_PLAN = (
    REPOSITORY_ROOT
    / "compliance"
    / "requirements-lab"
    / "catch2-test-plan.json"
)
DEFAULT_BUNDLE = REPOSITORY_ROOT / ".compliance" / "corpus-bundle.json"
DEFAULT_CONTRACT_DIRECTORY = REPOSITORY_ROOT / "compliance" / "requirements-lab"
DEFAULT_ROADMAP = REPOSITORY_ROOT / "docs" / "planning" / "ROADMAP.md"
DEFAULT_IMPLEMENTATION_PLAN = REPOSITORY_ROOT / "docs" / "planning" / "IMPLEMENTATION-PLAN.md"
DEFAULT_TEST_ROOT = REPOSITORY_ROOT / "cpp" / "tests"


_CATCH2_TEST_CASE = re.compile(
    r'\bTEST_CASE(?:_[A-Za-z0-9_]+)?\s*\(\s*"((?:\\.|[^"\\])*)"',
    re.MULTILINE,
)


def load_json(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise ValueError(f"cannot read JSON file {path}: {error}") from error
    if not isinstance(value, dict):
        raise ValueError(f"JSON root must be an object: {path}")
    return value


def strings(value: Any) -> list[str]:
    if isinstance(value, str):
        return [value]
    if isinstance(value, Iterable) and not isinstance(value, (bytes, dict)):
        return [item for item in value if isinstance(item, str)]
    return []


def compact_prose(value: Any, limit: int = 240) -> str:
    """Collapse roadmap prose so compact reports stay bounded."""

    if not isinstance(value, str):
        return ""
    collapsed = " ".join(value.split())
    if len(collapsed) <= limit:
        return collapsed
    return f"{collapsed[: max(limit - 3, 1)].rstrip()}..."


def item_handles(item: dict[str, Any], compact: bool = False) -> list[str]:
    """Return explicit next-step handles recorded in the roadmap index.

    Compact reports keep the actionable lane/test/ctest handles but replace
    potentially large standard-section and plan-id lists with bounded counts.
    The full lists remain available through the default text and JSON views.
    """

    handles: list[str] = []
    if item.get("next_work_id"):
        handles.append(f"work={item['next_work_id']}")
    if item.get("next_work_status"):
        handles.append(f"work_status={item['next_work_status']}")
    if item.get("next_work_query"):
        handles.append(
            f"work_query={compact_prose(item['next_work_query'], 180)}"
        )
    if item.get("next_lane"):
        handles.append(f"lane={item['next_lane']}")
        handles.append(
            "focus_query="
            f"python tools/query_rti_work.py focus {item['next_lane']} --summary --compact"
        )
    if item.get("next_test_query"):
        role = item.get("next_test_role", "planned")
        label = "baseline_test" if role == "baseline" else "test"
        handles.append(f"{label}={item['next_test_query']}")
    if item.get("next_source_state"):
        handles.append(f"source_state={item['next_source_state']}")
    if item.get("next_source_test_query"):
        handles.append(f"source_test={item['next_source_test_query']}")
    if item.get("next_source_location"):
        handles.append(f"source_location={item['next_source_location']}")
    if item.get("next_source_lane"):
        handles.append(f"source_lane={item['next_source_lane']}")
    source_requirement_ids = strings(item.get("next_source_requirement_ids"))
    if source_requirement_ids:
        handles.append(
            f"source_requirement_count={len(source_requirement_ids)}"
            if compact
            else f"source_requirements={','.join(source_requirement_ids)}"
        )
    source_standard_sections = strings(item.get("next_source_standard_sections"))
    if source_standard_sections:
        handles.append(
            f"source_standard_count={len(source_standard_sections)}"
            if compact
            else f"source_standard={','.join(source_standard_sections)}"
        )
    source_api_surfaces = strings(item.get("next_source_api_surfaces"))
    if source_api_surfaces:
        handles.append(
            f"source_api_count={len(source_api_surfaces)}"
            if compact
            else f"source_api={','.join(source_api_surfaces)}"
        )
    if item.get("next_source_ctest_filter"):
        handles.append(f"source_ctest={item['next_source_ctest_filter']}")
    if item.get("next_ctest_filter"):
        handles.append(f"ctest={item['next_ctest_filter']}")
    if item.get("next_package_target"):
        handles.append(f"package_target={item['next_package_target']}")
    if item.get("next_package_ctest_filter"):
        handles.append(f"package_ctest={item['next_package_ctest_filter']}")
    if item.get("next_package_manifest"):
        handles.append(f"package_manifest={item['next_package_manifest']}")
    if item.get("next_junit_target"):
        handles.append(f"junit_target={item['next_junit_target']}")
    if item.get("next_junit_artifact"):
        handles.append(f"junit_artifact={item['next_junit_artifact']}")
    if item.get("next_process_probe_target"):
        handles.append(f"process_probe_target={item['next_process_probe_target']}")
    if item.get("next_process_package_target"):
        handles.append(f"process_package_target={item['next_process_package_target']}")
    if item.get("next_process_package_test"):
        handles.append(f"process_package_test={item['next_process_package_test']}")
    if item.get("next_process_package_ctest_filter"):
        handles.append(
            f"process_package_ctest={item['next_process_package_ctest_filter']}"
        )
    if item.get("next_process_package_timestamped_test"):
        handles.append(
            "process_package_timestamped_test="
            f"{item['next_process_package_timestamped_test']}"
        )
    if item.get("next_process_package_timestamped_ctest_filter"):
        handles.append(
            "process_package_timestamped_ctest="
            f"{item['next_process_package_timestamped_ctest_filter']}"
        )
    if item.get("next_process_package_parameterized_test"):
        handles.append(
            "process_package_parameterized_test="
            f"{item['next_process_package_parameterized_test']}"
        )
    if item.get("next_process_package_parameterized_ctest_filter"):
        handles.append(
            "process_package_parameterized_ctest="
            f"{item['next_process_package_parameterized_ctest_filter']}"
        )
    if item.get("next_process_package_connection_loss_test"):
        handles.append(
            "process_package_connection_loss_test="
            f"{item['next_process_package_connection_loss_test']}"
        )
    if item.get("next_process_package_connection_loss_ctest_filter"):
        handles.append(
            "process_package_connection_loss_ctest="
            f"{item['next_process_package_connection_loss_ctest_filter']}"
        )
    if item.get("next_process_package_object_registration_test"):
        handles.append(
            "process_package_object_registration_test="
            f"{item['next_process_package_object_registration_test']}"
        )
    if item.get("next_process_package_object_registration_ctest_filter"):
        handles.append(
            "process_package_object_registration_ctest="
            f"{item['next_process_package_object_registration_ctest_filter']}"
        )
    if item.get("next_process_package_named_registration_test"):
        handles.append(
            "process_package_named_registration_test="
            f"{item['next_process_package_named_registration_test']}"
        )
    if item.get("next_process_package_named_registration_ctest_filter"):
        handles.append(
            "process_package_named_registration_ctest="
            f"{item['next_process_package_named_registration_ctest_filter']}"
        )
    if item.get("next_process_package_attribute_update_test"):
        handles.append(
            "process_package_attribute_update_test="
            f"{item['next_process_package_attribute_update_test']}"
        )
    if item.get("next_process_package_attribute_update_ctest_filter"):
        handles.append(
            "process_package_attribute_update_ctest="
            f"{item['next_process_package_attribute_update_ctest_filter']}"
        )
    if item.get("next_process_package_directed_retraction_test"):
        handles.append(
            "process_package_directed_retraction_test="
            f"{item['next_process_package_directed_retraction_test']}"
        )
    if item.get("next_process_package_directed_retraction_ctest_filter"):
        handles.append(
            "process_package_directed_retraction_ctest="
            f"{item['next_process_package_directed_retraction_ctest_filter']}"
        )
    if item.get("next_process_package_catalog_verifier"):
        handles.append(
            "process_package_catalog_verifier="
            f"{item['next_process_package_catalog_verifier']}"
        )
    sections = strings(item.get("next_standard_sections"))
    if sections:
        handles.append(
            f"standard_count={len(sections)}"
            if compact
            else f"standard={','.join(sections)}"
        )
    plan_ids = strings(item.get("next_plan_ids"))
    if plan_ids:
        handles.append(
            f"plan_id_count={len(plan_ids)}"
            if compact
            else f"plan={','.join(plan_ids)}"
        )
    catalog_gap_ids = strings(item.get("catalog_gap_plan_ids"))
    if catalog_gap_ids:
        handles.append(
            f"catalog_gap_count={len(catalog_gap_ids)}"
            if compact
            else f"catalog_gaps={','.join(catalog_gap_ids)}"
        )
    return handles


def search_key(value: Any) -> str:
    """Fold section punctuation and separators for forgiving human queries."""

    return re.sub(r"[^a-z0-9]+", "", str(value).casefold())


def relative_path(path: Path) -> str:
    try:
        return path.resolve().relative_to(REPOSITORY_ROOT.resolve()).as_posix()
    except ValueError:
        return str(path)


def decode_cpp_string(value: str) -> str:
    """Decode the small escape subset used in Catch2 test names."""

    return re.sub(r'\\(["\\])', r"\1", value)


def load_test_source_locations(root: Path) -> dict[str, list[dict[str, Any]]]:
    """Index Catch2 test names to their C++ source locations.

    The test plan remains the traceability source of truth.  This bounded
    derived index only makes the plan actionable by locating the corresponding
    ``TEST_CASE`` declaration, and is intentionally rebuilt per query so it
    cannot become a second, stale plan.
    """

    if not root.is_dir():
        raise ValueError(f"test source root is not a directory: {root}")
    locations: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for path in sorted(root.rglob("*.cpp")):
        try:
            source = path.read_text(encoding="utf-8")
        except OSError as error:
            raise ValueError(f"cannot read test source {path}: {error}") from error
        for match in _CATCH2_TEST_CASE.finditer(source):
            name = decode_cpp_string(match.group(1))
            line = source.count("\n", 0, match.start()) + 1
            locations[name].append(
                {
                    "path": relative_path(path),
                    "line": line,
                }
            )
    return dict(locations)


def source_location_text(test: dict[str, Any]) -> str:
    locations = test.get("source_locations")
    if not isinstance(locations, list) or not locations:
        return "<not found>"
    values = []
    for location in locations:
        if not isinstance(location, dict):
            continue
        path = location.get("path")
        line = location.get("line")
        if isinstance(path, str) and line is not None:
            values.append(f"{path}:{line}")
        elif isinstance(path, str):
            values.append(path)
    return ", ".join(values) or "<not found>"


def source_locations_for_test(
    test_case: Any,
    source_locations: dict[str, list[dict[str, Any]]],
) -> list[dict[str, Any]]:
    """Resolve a plan entry, including entries grouping cases with semicolons."""

    if not isinstance(test_case, str):
        return []
    candidates = [test_case]
    candidates.extend(part.strip() for part in test_case.split(";") if part.strip())
    resolved: list[dict[str, Any]] = []
    seen: set[tuple[Any, Any]] = set()
    for candidate in candidates:
        for location in source_locations.get(candidate, []):
            if not isinstance(location, dict):
                continue
            key = (location.get("path"), location.get("line"))
            if key in seen:
                continue
            seen.add(key)
            resolved.append(location)
    return resolved


def unplanned_source_cases(
    tests: list[dict[str, Any]],
    source_locations: dict[str, list[dict[str, Any]]],
    source_path_query: str | None = None,
) -> list[dict[str, Any]]:
    """Return source TEST_CASEs that have no exact Catch2 plan row.

    The Catch2 plan remains the traceability source of truth.  This derived
    view is intentionally one-way: it does not infer requirements or status
    for a source declaration.  It only exposes a bounded reconciliation queue
    so a new case can be mapped deliberately instead of found by a repository-
    wide text search.
    """

    planned_names: set[str] = set()
    for test in tests:
        test_case = test.get("test_case")
        if not isinstance(test_case, str):
            continue
        planned_names.add(test_case)
        planned_names.update(
            part.strip() for part in test_case.split(";") if part.strip()
        )
    folded_path = source_path_query.casefold() if source_path_query else None
    records: list[dict[str, Any]] = []
    for test_case, locations in source_locations.items():
        if test_case in planned_names:
            continue
        if folded_path is not None and not any(
            folded_path in str(location.get("path", "")).casefold()
            for location in locations
            if isinstance(location, dict)
        ):
            continue
        records.append(
            {
                "test_case": test_case,
                "source_locations": locations,
                "source_state": "unplanned-source",
            }
        )
    records.sort(
        key=lambda record: (
            source_location_text(record),
            str(record.get("test_case") or ""),
        )
    )
    return records


def load_standard_requirements(bundle: dict[str, Any]) -> dict[str, dict[str, Any]]:
    requirements: dict[str, dict[str, Any]] = {}
    for document in bundle.get("documents", []):
        if not isinstance(document, dict):
            continue
        document_id = document.get("document_id")
        if not isinstance(document_id, str) or not document_id.endswith("-2025"):
            continue
        for requirement in document.get("requirements", []):
            if not isinstance(requirement, dict):
                continue
            requirement_id = requirement.get("id")
            if not isinstance(requirement_id, str):
                continue
            requirements[requirement_id] = {
                "id": requirement_id,
                "document_id": document_id,
                "ordinal": requirement.get("ordinal"),
                "clause": requirement.get("clause"),
                "clause_id": requirement.get("clause_id"),
                "title": requirement.get("title"),
                "source": requirement.get("source"),
            }
    return requirements


def load_contract_links(directory: Path) -> dict[str, list[dict[str, Any]]]:
    links: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for path in sorted(directory.glob("*-requirements-contract.json")):
        try:
            contract = load_json(path)
        except ValueError:
            continue
        for requirement in contract.get("requirements", []):
            if not isinstance(requirement, dict):
                continue
            lab_id = requirement.get("requirements_lab_requirement_id")
            if not isinstance(lab_id, str):
                continue
            links[lab_id].append(
                {
                    "contract": relative_path(path),
                    "contract_requirement_id": requirement.get("id"),
                    "source": requirement.get("source"),
                    "source_symbol": requirement.get("source_symbol"),
                }
            )
    return dict(links)


def requirement_view(
    lab_id: str,
    standard_requirements: dict[str, dict[str, Any]],
    contract_links: dict[str, list[dict[str, Any]]],
) -> dict[str, Any]:
    standard = standard_requirements.get(lab_id)
    value: dict[str, Any] = {
        "lab_requirement_id": lab_id,
        "standard": standard,
        "contracts": contract_links.get(lab_id, []),
    }
    if standard is None:
        value["unresolved"] = True
    return value


def mapped_test(
    test: dict[str, Any],
    standard_requirements: dict[str, dict[str, Any]],
    contract_links: dict[str, list[dict[str, Any]]],
    source_locations: dict[str, list[dict[str, Any]]],
) -> dict[str, Any]:
    requirement_ids = strings(test.get("selected_requirements_lab_requirement_ids"))
    requirements = [
        requirement_view(identifier, standard_requirements, contract_links)
        for identifier in requirement_ids
    ]
    standards = [
        requirement["standard"]
        for requirement in requirements
        if requirement.get("standard") is not None
    ]
    clauses = sorted(
        {
            f"{standard.get('document_id')}:{standard.get('clause_id')}"
            for standard in standards
            if standard.get("document_id") and standard.get("clause_id")
        }
    )
    locations = source_locations_for_test(
        test.get("test_case"), source_locations
    )
    return {
        "id": test.get("id"),
        "test_case": test.get("test_case"),
        "requirements_lab_mapping_id": test.get("requirements_lab_mapping_id"),
        "tags": strings(test.get("tags")),
        "selected_cpp_api_surface_ids": strings(
            test.get("selected_cpp_api_surface_ids")
        ),
        "assertions": test.get("assertions"),
        "callback_models": strings(test.get("callback_models")),
        "delivery_modes": strings(test.get("delivery_modes")),
        "callback_gate_modes": strings(test.get("callback_gate_modes")),
        "status": test.get("status"),
        "source_missing_reason": test.get("source_missing_reason"),
        "next_action": test.get("next_action"),
        "requirements": requirements,
        "lab_requirement_ids": requirement_ids,
        "standard_clauses": clauses,
        "standard_sections": clauses,
        "source_locations": locations,
        "source_state": "located" if locations else "unlocated",
        "unresolved_requirement_ids": [
            requirement["lab_requirement_id"]
            for requirement in requirements
            if requirement.get("unresolved")
        ],
    }


def all_mapped_tests(
    plan: dict[str, Any],
    standard_requirements: dict[str, dict[str, Any]],
    contract_links: dict[str, list[dict[str, Any]]],
    source_locations: dict[str, list[dict[str, Any]]],
) -> list[dict[str, Any]]:
    return [
        mapped_test(test, standard_requirements, contract_links, source_locations)
        for test in plan.get("tests", [])
        if isinstance(test, dict)
    ]


def roadmap_checklist(path: Path) -> list[dict[str, Any]]:
    entries: list[dict[str, Any]] = []
    section = ""
    heading = re.compile(r"^#{1,6}\s+(.+?)\s*$")
    checkbox = re.compile(r"^-\s+\[([ xX])\]\s+(.+?)\s*$")
    for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        heading_match = heading.match(line)
        if heading_match:
            section = heading_match.group(1)
            continue
        checkbox_match = checkbox.match(line)
        if checkbox_match:
            entries.append(
                {
                    "line": line_number,
                    "section": section,
                    "status": "complete" if checkbox_match.group(1).lower() == "x" else "open",
                    "title": checkbox_match.group(2),
                }
            )
    return entries


def implementation_plan_sections(path: Path) -> list[dict[str, Any]]:
    """Return the stable section outline without loading the plan prose."""

    sections: list[dict[str, Any]] = []
    heading = re.compile(r"^(#{1,6})\s+(.+?)\s*$")
    for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        match = heading.match(line)
        if match is None:
            continue
        sections.append(
            {
                "line": line_number,
                "level": len(match.group(1)),
                "title": match.group(2),
            }
        )
    return sections


def index_status(
    index: dict[str, Any],
    tests: list[dict[str, Any]],
    roadmap_entries: list[dict[str, Any]],
    source_locations: dict[str, list[dict[str, Any]]] | None = None,
) -> dict[str, Any]:
    items = [item for item in index.get("items", []) if isinstance(item, dict)]
    tag_counts = Counter(tag for test in tests for tag in test.get("tags", []))
    enriched_items = []
    for item in items:
        tags = strings(item.get("query_tags"))
        pointer = next_test_pointer(item, tests)
        matching_tests = item_tests(item, tests)
        enriched_items.append(
            {
                **item,
                "matching_test_counts": {tag: tag_counts.get(tag, 0) for tag in tags},
                # A roadmap item usually carries several overlapping tags.
                # Keep the per-tag counts for discovery, but also publish the
                # number of distinct Catch2 cases so compact status output
                # cannot look larger merely because a case has many tags.
                "matching_test_count": len(matching_tests),
                "next_test_pointer": pointer,
            }
        )
    mapping = index.get("mapping", {})
    current_counts = (
        mapping.get("current_counts", {})
        if isinstance(mapping, dict)
        else {}
    )
    derived_counts = {
        "catch2_plan_cases": len(tests),
        "catch2_cases_without_lab_requirement_mapping": sum(
            not test.get("requirements") for test in tests
        ),
        "catch2_cases_without_cpp_source_location": sum(
            not test.get("source_locations") for test in tests
        ),
    }
    if source_locations is not None:
        derived_counts["cpp_source_cases_without_plan_row"] = len(
            unplanned_source_cases(tests, source_locations)
        )
    return {
        "roadmap_items": enriched_items,
        "roadmap_checklist": roadmap_entries,
        "roadmap_counts": dict(Counter(entry["status"] for entry in roadmap_entries)),
        "test_count": len(tests),
        "mapping_counts": current_counts if isinstance(current_counts, dict) else {},
        "derived_mapping_counts": derived_counts,
    }


def item_tests(item: dict[str, Any], tests: list[dict[str, Any]]) -> list[dict[str, Any]]:
    """Return tests belonging to an indexed roadmap item through its query tags."""

    tags = set(strings(item.get("query_tags")))
    if not tags:
        return []
    return [test for test in tests if tags.intersection(test.get("tags", []))]


def item_matches_lane(item: dict[str, Any], lane: str) -> bool:
    """Match a lane against both broad family tags and its exact next pointer.

    A roadmap family may intentionally keep a broad ``query_tags`` set while
    pointing at a narrower ``next_lane``.  Treating only the broad tags as a
    lane relationship made ``check --lane`` report zero roadmap owners for
    otherwise valid focused slices.
    """

    return lane in strings(item.get("query_tags")) or item.get("next_lane") == lane


def roadmap_links_for_test(
    index: dict[str, Any],
    test: dict[str, Any],
    limit: int = 8,
) -> list[dict[str, Any]]:
    """Return bounded roadmap-family links for one mapped Catch2 case.

    Exact ``next_lane`` ownership is ranked ahead of broad tag overlap so a
    trace result points first at the family that owns the focused slice while
    retaining the broader roadmap relationships for context.
    """

    test_tags = set(strings(test.get("tags")))
    candidates: list[tuple[tuple[int, int, int, str], dict[str, Any]]] = []
    for item in index.get("items", []):
        if not isinstance(item, dict):
            continue
        query_tags = set(strings(item.get("query_tags")))
        next_lane = item.get("next_lane")
        exact_next_lane = isinstance(next_lane, str) and next_lane in test_tags
        overlap = len(query_tags.intersection(test_tags))
        if not exact_next_lane and overlap == 0:
            continue
        candidates.append(
            (
                (
                    0 if exact_next_lane else 1,
                    -overlap,
                    item.get("priority", 999)
                    if isinstance(item.get("priority"), int)
                    else 999,
                    str(item.get("id") or ""),
                ),
                {
                    "id": item.get("id"),
                    "title": item.get("title"),
                    "status": item.get("status"),
                    "priority": item.get("priority"),
                    "next_lane": next_lane,
                    "match": "next_lane" if exact_next_lane else "query_tag",
                },
            )
        )
    candidates.sort(key=lambda value: value[0])
    values = [value for _, value in candidates]
    return values if limit == 0 else values[: max(limit, 0)]


def next_test_pointer(item: dict[str, Any], tests: list[dict[str, Any]]) -> dict[str, Any]:
    """Describe whether an indexed test handle is a TODO or a completed baseline.

    Roadmap items often name an existing regression as the baseline for a new
    slice. Treating that handle as an unqualified ``next`` test made the work
    queue look stale and encouraged rerunning completed work. Keep the handle,
    but derive a small status record from the mapped plan so callers can
    distinguish a planned test from an implemented baseline.
    """

    query = item.get("next_test_query")
    if not isinstance(query, str) or not query.strip():
        return {"state": "not-specified", "match_count": 0, "statuses": {}}

    matches = [test for test in tests if test.get("test_case") == query]
    if not matches:
        return {
            "query": query,
            "state": "missing",
            "match_count": 0,
            "statuses": {},
        }

    status_counts = Counter(
        str(test.get("status") or "<unspecified>") for test in matches
    )
    implemented = all(
        status.startswith(("implemented", "verified"))
        for status in status_counts
    )
    located = all(bool(test.get("source_locations")) for test in matches)
    if implemented and located:
        state = "complete"
    elif implemented:
        # A plan entry may describe an implemented historical expectation
        # whose exact declaration is absent from the current checkout. Keep
        # that distinction visible so ``next`` cannot mistake catalog prose
        # for executable evidence.
        state = "source-missing"
    elif any(status.startswith(("implemented", "verified")) for status in status_counts):
        state = "partial"
    else:
        state = "planned"
    return {
        "query": query,
        "state": state,
        "match_count": len(matches),
        "statuses": dict(sorted(status_counts.items())),
    }


def index_warnings(
    index_result: dict[str, Any],
    lane: str | None = None,
) -> list[str]:
    """Return bounded warnings for open items whose test handle is complete."""

    warnings: list[str] = []
    for item in index_result.get("roadmap_items", []):
        if not isinstance(item, dict) or item.get("status") != "open":
            continue
        if lane and not item_matches_lane(item, lane):
            continue
        pointer = item.get("next_test_pointer")
        if not isinstance(pointer, dict) or pointer.get("state") != "complete":
            continue
        # A baseline is intentional: the accompanying next_work_query remains
        # the actionable slice. Any unlabelled completed handle is likely a
        # stale pointer and should be corrected in the index.
        if item.get("next_test_role") == "baseline":
            continue
        warnings.append(
            f"{item.get('id', '<unnamed>')} next_test_query is already implemented; "
            "label it as next_test_role=baseline or replace the handle"
        )
    return warnings


def roadmap_item_summary(item: dict[str, Any]) -> dict[str, Any]:
    """Keep the active-work record small while retaining its stable handles."""

    return {
        "id": item.get("id"),
        "title": item.get("title"),
        "status": item.get("status"),
        "kind": item.get("kind"),
        "priority": item.get("priority"),
        "roadmap_anchor": item.get("roadmap_anchor"),
    }


def bounded_requirement_ids(test: dict[str, Any], limit: int = 12) -> dict[str, Any]:
    """Return exact requirement handles without expanding a cross-cutting case."""

    values = strings(test.get("lab_requirement_ids"))
    result: dict[str, Any] = {
        "requirement_count": len(values),
        "requirement_ids": values[:limit],
    }
    if len(values) > limit:
        result["requirement_ids_remaining"] = len(values) - limit
    return result


def indexed_work_slice(
    index: dict[str, Any],
    tests: list[dict[str, Any]],
    requested_id: str | None = None,
) -> dict[str, Any]:
    """Resolve one actionable slice without scanning roadmap prose.

    The human roadmap item and the active implementation item are deliberately
    separate in the index: a broad parent can point at a narrower
    ``next_work_id``.  This resolver makes that relationship explicit and joins
    only the exact baseline test, lane counts, and canonical 2025 sections.
    """

    items = [item for item in index.get("items", []) if isinstance(item, dict)]
    by_id = {
        item.get("id"): item
        for item in items
        if isinstance(item.get("id"), str) and item.get("id")
    }
    if requested_id is None:
        open_items = sorted(
            (item for item in items if item.get("status") == "open"),
            key=lambda item: item.get("priority", 999),
        )
        pointer_owner = open_items[0] if open_items else None
    else:
        pointer_owner = by_id.get(requested_id)

    if pointer_owner is None:
        return {
            "found": False,
            "requested_id": requested_id,
            "reason": "no indexed open roadmap item matched" if requested_id is None
            else "no indexed roadmap item matched",
        }

    # A narrow work item is often referenced by a broader priority owner and
    # intentionally carries no duplicate pointer fields of its own.  Resolve
    # that reverse edge so ``work <next_work_id>`` is just as useful as the
    # default ``work`` query.
    if requested_id is not None and not any(
        pointer_owner.get(field)
        for field in ("next_lane", "next_ctest_filter", "next_test_query", "next_work_query")
    ):
        owners = sorted(
            (
                item
                for item in items
                if item.get("next_work_id") == requested_id
            ),
            key=lambda item: item.get("priority", 999),
        )
        if owners:
            pointer_owner = owners[0]

    target_id = pointer_owner.get("next_work_id") or pointer_owner.get("id")
    target = by_id.get(target_id, pointer_owner)
    lane = pointer_owner.get("next_lane") or target.get("next_lane")
    ctest_filter = pointer_owner.get("next_ctest_filter") or target.get("next_ctest_filter")
    package_target = pointer_owner.get("next_package_target") or target.get("next_package_target")
    package_ctest_filter = pointer_owner.get("next_package_ctest_filter") or target.get("next_package_ctest_filter")
    package_manifest = pointer_owner.get("next_package_manifest") or target.get("next_package_manifest")
    junit_target = pointer_owner.get("next_junit_target") or target.get("next_junit_target")
    junit_artifact = pointer_owner.get("next_junit_artifact") or target.get("next_junit_artifact")
    process_probe_target = pointer_owner.get("next_process_probe_target") or target.get("next_process_probe_target")
    process_package_target = pointer_owner.get("next_process_package_target") or target.get("next_process_package_target")
    process_package_test = pointer_owner.get("next_process_package_test") or target.get("next_process_package_test")
    process_package_ctest_filter = pointer_owner.get("next_process_package_ctest_filter") or target.get("next_process_package_ctest_filter")
    process_package_timestamped_test = pointer_owner.get("next_process_package_timestamped_test") or target.get("next_process_package_timestamped_test")
    process_package_timestamped_ctest_filter = pointer_owner.get("next_process_package_timestamped_ctest_filter") or target.get("next_process_package_timestamped_ctest_filter")
    process_package_parameterized_test = pointer_owner.get("next_process_package_parameterized_test") or target.get("next_process_package_parameterized_test")
    process_package_parameterized_ctest_filter = pointer_owner.get("next_process_package_parameterized_ctest_filter") or target.get("next_process_package_parameterized_ctest_filter")
    process_package_connection_loss_test = pointer_owner.get("next_process_package_connection_loss_test") or target.get("next_process_package_connection_loss_test")
    process_package_connection_loss_ctest_filter = pointer_owner.get("next_process_package_connection_loss_ctest_filter") or target.get("next_process_package_connection_loss_ctest_filter")
    process_package_object_registration_test = pointer_owner.get("next_process_package_object_registration_test") or target.get("next_process_package_object_registration_test")
    process_package_object_registration_ctest_filter = pointer_owner.get("next_process_package_object_registration_ctest_filter") or target.get("next_process_package_object_registration_ctest_filter")
    process_package_named_registration_test = pointer_owner.get("next_process_package_named_registration_test") or target.get("next_process_package_named_registration_test")
    process_package_named_registration_ctest_filter = pointer_owner.get("next_process_package_named_registration_ctest_filter") or target.get("next_process_package_named_registration_ctest_filter")
    process_package_attribute_update_test = pointer_owner.get("next_process_package_attribute_update_test") or target.get("next_process_package_attribute_update_test")
    process_package_attribute_update_ctest_filter = pointer_owner.get("next_process_package_attribute_update_ctest_filter") or target.get("next_process_package_attribute_update_ctest_filter")
    process_package_directed_retraction_test = pointer_owner.get("next_process_package_directed_retraction_test") or target.get("next_process_package_directed_retraction_test")
    process_package_directed_retraction_ctest_filter = pointer_owner.get("next_process_package_directed_retraction_ctest_filter") or target.get("next_process_package_directed_retraction_ctest_filter")
    process_package_catalog_verifier = pointer_owner.get("next_process_package_catalog_verifier") or target.get("next_process_package_catalog_verifier")
    baseline_query = pointer_owner.get("next_test_query") or target.get("next_test_query")
    source_state = pointer_owner.get("next_source_state") or target.get("next_source_state")
    source_test_query = pointer_owner.get("next_source_test_query") or target.get("next_source_test_query")
    source_location = pointer_owner.get("next_source_location") or target.get("next_source_location")
    source_lane = pointer_owner.get("next_source_lane") or target.get("next_source_lane")
    source_requirement_ids = strings(
        pointer_owner.get("next_source_requirement_ids")
        or target.get("next_source_requirement_ids")
    )
    source_standard_sections = strings(
        pointer_owner.get("next_source_standard_sections")
        or target.get("next_source_standard_sections")
    )
    source_api_surfaces = strings(
        pointer_owner.get("next_source_api_surfaces")
        or target.get("next_source_api_surfaces")
    )
    source_ctest_filter = pointer_owner.get("next_source_ctest_filter") or target.get("next_source_ctest_filter")
    baseline_matches = [
        test
        for test in tests
        if isinstance(baseline_query, str) and test.get("test_case") == baseline_query
    ]
    baseline = None
    if baseline_matches:
        baseline_test = baseline_matches[0]
        baseline = {
            **test_summary_data(baseline_test),
            **bounded_requirement_ids(baseline_test),
        }
    elif isinstance(baseline_query, str):
        baseline = {
            "test_case": baseline_query,
            "state": "missing",
            "requirement_count": 0,
            "requirement_ids": [],
            "standard_sections": [],
        }

    lane_case_count = None
    lane_snapshot: dict[str, Any] | None = None
    if isinstance(lane, str) and lane:
        lane_case_count = sum(lane in strings(test.get("tags")) for test in tests)
        # Keep the active work card honest when its baseline lane has just
        # become complete.  This is a bounded exact-tag lookup; it does not
        # reopen the whole plan or perform a fuzzy search for replacement work.
        lane_snapshot = focused_lane_result(index, tests, lane, limit=1)

    commands: list[str] = []
    if isinstance(baseline_query, str) and baseline_query:
        commands.append(
            f'python tools/query_rti_work.py trace "{baseline_query}" --summary'
        )
    if (
        isinstance(source_test_query, str)
        and source_test_query
        and source_state != "planned"
    ):
        source_path = None
        if isinstance(source_location, str) and source_location:
            source_path = (
                source_location.rsplit(":", 1)[0]
                if ":" in source_location
                else source_location
            )
        source_command = "python tools/query_rti_work.py unplanned"
        if source_path:
            source_command += f" --path {source_path}"
        commands.append(f"{source_command} --limit 1 --summary")
    if isinstance(lane, str) and lane:
        commands.append(
            f"python tools/query_rti_work.py focus {lane} --summary --compact"
        )
        commands.append(
            f"python tools/query_rti_work.py lane {lane} --summary --compact"
        )
        commands.append(
            f"python tools/query_rti_work.py check --lane {lane} --summary --compact"
        )
    if isinstance(package_target, str) and package_target:
        commands.append(
            f"cmake --build <build-dir> --config Debug --target {package_target}"
        )
    if isinstance(package_ctest_filter, str) and package_ctest_filter:
        commands.append(
            "ctest --test-dir <build-dir> -C Debug "
            f"-L {package_ctest_filter} --output-on-failure"
        )
    if isinstance(junit_target, str) and junit_target:
        commands.append(
            f"cmake --build <build-dir> --config Debug --target {junit_target}"
        )
    if isinstance(process_probe_target, str) and process_probe_target:
        commands.append(
            f"cmake --build <build-dir> --config Debug --target {process_probe_target}"
        )
    if isinstance(process_package_target, str) and process_package_target:
        commands.append(
            "cmake --build <build-dir>/package-smoke-consumer --config Debug "
            f"--target {process_package_target}"
        )
    if isinstance(process_package_test, str) and process_package_test:
        commands.append(
            "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
            f'-R "^{process_package_test}$" --output-on-failure'
        )
    if isinstance(process_package_timestamped_test, str) and process_package_timestamped_test:
        commands.append(
            "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
            f'-R "^{process_package_timestamped_test}$" --output-on-failure'
        )
    if isinstance(process_package_timestamped_ctest_filter, str) and process_package_timestamped_ctest_filter:
        commands.append(
            "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
            f"-L {process_package_timestamped_ctest_filter} --output-on-failure"
        )
    if isinstance(process_package_parameterized_test, str) and process_package_parameterized_test:
        commands.append(
            "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
            f'-R "^{process_package_parameterized_test}$" --output-on-failure'
        )
    if isinstance(process_package_parameterized_ctest_filter, str) and process_package_parameterized_ctest_filter:
        commands.append(
            "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
            f"-L {process_package_parameterized_ctest_filter} --output-on-failure"
        )
    if isinstance(process_package_connection_loss_test, str) and process_package_connection_loss_test:
        commands.append(
            "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
            f'-R "^{process_package_connection_loss_test}$" --output-on-failure'
        )
    if isinstance(process_package_connection_loss_ctest_filter, str) and process_package_connection_loss_ctest_filter:
        commands.append(
            "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
            f"-L {process_package_connection_loss_ctest_filter} --output-on-failure"
        )
    if isinstance(process_package_object_registration_test, str) and process_package_object_registration_test:
        commands.append(
            "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
            f'-R "^{process_package_object_registration_test}$" --output-on-failure'
        )
    if isinstance(process_package_object_registration_ctest_filter, str) and process_package_object_registration_ctest_filter:
        commands.append(
            "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
            f"-L {process_package_object_registration_ctest_filter} --output-on-failure"
        )
    if isinstance(process_package_named_registration_test, str) and process_package_named_registration_test:
        commands.append(
            "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
            f'-R "^{process_package_named_registration_test}$" --output-on-failure'
        )
    if isinstance(process_package_named_registration_ctest_filter, str) and process_package_named_registration_ctest_filter:
        commands.append(
            "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
            f"-L {process_package_named_registration_ctest_filter} --output-on-failure"
        )
    if isinstance(process_package_attribute_update_test, str) and process_package_attribute_update_test:
        commands.append(
            "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
            f'-R "^{process_package_attribute_update_test}$" --output-on-failure'
        )
    if isinstance(process_package_attribute_update_ctest_filter, str) and process_package_attribute_update_ctest_filter:
        commands.append(
            "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
            f"-L {process_package_attribute_update_ctest_filter} --output-on-failure"
        )
    if isinstance(process_package_directed_retraction_test, str) and process_package_directed_retraction_test:
        commands.append(
            "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
            f'-R "^{process_package_directed_retraction_test}$" --output-on-failure'
        )
    if isinstance(process_package_directed_retraction_ctest_filter, str) and process_package_directed_retraction_ctest_filter:
        commands.append(
            "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
            f"-L {process_package_directed_retraction_ctest_filter} --output-on-failure"
        )
    if isinstance(process_package_catalog_verifier, str) and process_package_catalog_verifier:
        commands.append(
            "python tools/verify_process_package_lanes.py --ctest ctest "
            "--test-dir <build-dir>/package-smoke-consumer --config Debug"
        )
    standard_sections = strings(
        pointer_owner.get("next_standard_sections")
        or target.get("next_standard_sections")
    )
    plan_ids = strings(
        pointer_owner.get("next_plan_ids") or target.get("next_plan_ids")
    )
    catalog_gap_plan_ids = strings(
        pointer_owner.get("catalog_gap_plan_ids")
        or target.get("catalog_gap_plan_ids")
    )
    return {
        "found": True,
        "requested_id": requested_id,
        "parent_item": roadmap_item_summary(pointer_owner),
        "work_item": roadmap_item_summary(target),
        "work_id": target.get("id"),
        "work_status": pointer_owner.get("next_work_status") or target.get("status"),
        "task": pointer_owner.get("next_task") or pointer_owner.get("next_work_query") or target.get("next_action"),
        "work_query": pointer_owner.get("next_work_query"),
        "next_action": pointer_owner.get("next_action") or target.get("next_action"),
        "lane": lane,
        "lane_case_count": lane_case_count,
        "lane_state": lane_snapshot.get("lane_state") if lane_snapshot else None,
        "lane_mapped_test_count": (
            lane_snapshot.get("mapped_test_count") if lane_snapshot else None
        ),
        "lane_candidate_count": (
            lane_snapshot.get("candidate_count") if lane_snapshot else None
        ),
        "lane_source_drift_count": (
            lane_snapshot.get("source_drift_count") if lane_snapshot else None
        ),
        "lane_assertion_count": (
            lane_snapshot.get("assertion_count") if lane_snapshot else None
        ),
        "ctest_filter": ctest_filter,
        "package_target": package_target,
        "package_ctest_filter": package_ctest_filter,
        "package_manifest": package_manifest,
        "junit_target": junit_target,
        "junit_artifact": junit_artifact,
        "process_probe_target": process_probe_target,
        "process_package_target": process_package_target,
        "process_package_test": process_package_test,
        "process_package_ctest_filter": process_package_ctest_filter,
        "process_package_timestamped_test": process_package_timestamped_test,
        "process_package_timestamped_ctest_filter": process_package_timestamped_ctest_filter,
        "process_package_parameterized_test": process_package_parameterized_test,
        "process_package_parameterized_ctest_filter": process_package_parameterized_ctest_filter,
        "process_package_connection_loss_test": process_package_connection_loss_test,
        "process_package_connection_loss_ctest_filter": process_package_connection_loss_ctest_filter,
        "process_package_object_registration_test": process_package_object_registration_test,
        "process_package_object_registration_ctest_filter": process_package_object_registration_ctest_filter,
        "process_package_named_registration_test": process_package_named_registration_test,
        "process_package_named_registration_ctest_filter": process_package_named_registration_ctest_filter,
        "process_package_attribute_update_test": process_package_attribute_update_test,
        "process_package_attribute_update_ctest_filter": process_package_attribute_update_ctest_filter,
        "process_package_directed_retraction_test": process_package_directed_retraction_test,
        "process_package_directed_retraction_ctest_filter": process_package_directed_retraction_ctest_filter,
        "process_package_catalog_verifier": process_package_catalog_verifier,
        "baseline_test_role": pointer_owner.get("next_test_role") or target.get("next_test_role"),
        "baseline": baseline,
        "next_source_state": source_state,
        "next_source_test_query": source_test_query,
        "next_source_location": source_location,
        "next_source_lane": source_lane,
        "next_source_requirement_ids": source_requirement_ids,
        "next_source_standard_sections": source_standard_sections,
        "next_source_api_surfaces": source_api_surfaces,
        "next_source_ctest_filter": source_ctest_filter,
        "standard_sections": standard_sections,
        "plan_ids": plan_ids,
        "catalog_gap_plan_ids": catalog_gap_plan_ids,
        "commands": commands,
    }


def indexed_item(
    index: dict[str, Any],
    tests: list[dict[str, Any]],
    item_id: str,
) -> tuple[dict[str, Any] | None, list[dict[str, Any]]]:
    """Resolve one roadmap item and its tagged tests without scanning prose."""

    for item in index.get("items", []):
        if isinstance(item, dict) and item.get("id") == item_id:
            return item, item_tests(item, tests)
    return None, []


def recent_completed_views(
    index: dict[str, Any],
    tests: list[dict[str, Any]],
    lane: str | None = None,
) -> list[dict[str, Any]]:
    """Join the bounded recent-slice ledger to exact Catch2 test records.

    ``ROADMAP-INDEX.json`` keeps a small, human-curated ledger of the latest
    completed slices so work can resume from evidence without reopening the
    full plan.  The test record remains authoritative for source locations and
    requirement/section mappings; this helper only joins the two views.
    """

    views: list[dict[str, Any]] = []
    # The ledger is append-only so that each completion remains easy to audit.
    # Present it newest-first for the bounded resumption query; otherwise a
    # small ``recent --limit`` request misleadingly returned the oldest rows.
    for entry in reversed(index.get("recent_completed_slices", [])):
        if not isinstance(entry, dict):
            continue
        if lane and lane not in strings(entry.get("lane_queries")):
            continue
        view = dict(entry)
        query = entry.get("test_query")
        matches = [
            test
            for test in tests
            if isinstance(query, str) and test.get("test_case") == query
        ]
        view["matched_test_count"] = len(matches)
        view["tests"] = [test_summary_data(test) for test in matches]
        if matches:
            view["source_locations"] = [
                location
                for test in matches
                for location in test.get("source_locations", [])
            ]
            view["resolved_standard_sections"] = sorted(
                {
                    section
                    for test in matches
                    for section in test.get("standard_sections", [])
                }
            )
            view["resolved_requirement_count"] = sum(
                len(strings(test.get("lab_requirement_ids")))
                for test in matches
            )
        else:
            view.setdefault("source_locations", [])
            view.setdefault("resolved_standard_sections", [])
            view.setdefault("resolved_requirement_count", 0)
        views.append(view)
    return views


def validate_index(
    index: dict[str, Any],
    roadmap_path: Path,
    plan: dict[str, Any],
    standard_requirements: dict[str, dict[str, Any]],
    tests: list[dict[str, Any]],
    source_locations: dict[str, list[dict[str, Any]]] | None = None,
    focus_lane: str | None = None,
) -> list[str]:
    errors: list[str] = []
    if index.get("schema_version") != 1:
        errors.append("roadmap index schema_version must be 1")
    items = index.get("items")
    if not isinstance(items, list) or not items:
        errors.append("roadmap index must contain at least one item")
        items = []
    try:
        roadmap_text = roadmap_path.read_text(encoding="utf-8")
    except OSError as error:
        errors.append(f"cannot read roadmap {roadmap_path}: {error}")
        roadmap_text = ""
    seen_ids: set[str] = set()
    seen_anchors: set[str] = set()
    known_tags = {
        tag
        for test in plan.get("tests", [])
        if isinstance(test, dict)
        for tag in strings(test.get("tags"))
    }
    known_test_ids = {
        test.get("id")
        for test in plan.get("tests", [])
        if isinstance(test, dict) and isinstance(test.get("id"), str)
    }
    known_test_cases = {
        test.get("test_case")
        for test in plan.get("tests", [])
        if isinstance(test, dict) and isinstance(test.get("test_case"), str)
    }
    known_standard_sections = {
        f"{requirement.get('document_id')}:{requirement.get('clause_id')}"
        for requirement in standard_requirements.values()
        if requirement.get("document_id") and requirement.get("clause_id")
    }
    if focus_lane is not None and focus_lane not in known_tags:
        errors.append(f"check lane is absent from the Catch2 plan: {focus_lane}")
    scoped_tests = (
        tests
        if focus_lane is None
        else [test for test in tests if focus_lane in strings(test.get("tags"))]
    )
    for item in items:
        if not isinstance(item, dict):
            errors.append("roadmap index item must be an object")
            continue
        item_id = item.get("id")
        if not isinstance(item_id, str) or not item_id:
            errors.append("roadmap index item is missing a non-empty id")
        elif item_id in seen_ids:
            errors.append(f"duplicate roadmap index id: {item_id}")
        else:
            seen_ids.add(item_id)
        anchor = item.get("roadmap_anchor")
        if not isinstance(anchor, str) or not anchor:
            errors.append(f"{item_id or '<unnamed>'} is missing roadmap_anchor")
        elif anchor in seen_anchors:
            errors.append(f"duplicate roadmap anchor: {anchor}")
        else:
            seen_anchors.add(anchor)
            if anchor not in roadmap_text:
                errors.append(f"{item_id or '<unnamed>'} anchor is absent from {roadmap_path}")
        if item.get("status") not in {"open", "complete", "blocked"}:
            errors.append(f"{item_id or '<unnamed>'} has an invalid status")
        for field in (
            "next_work_id",
            "next_work_status",
            "next_work_query",
            "next_lane",
            "next_test_query",
            "next_source_state",
            "next_source_test_query",
            "next_source_location",
            "next_source_lane",
            "next_source_ctest_filter",
            "next_ctest_filter",
            "next_package_target",
            "next_package_ctest_filter",
            "next_package_manifest",
            "next_junit_target",
            "next_junit_artifact",
            "next_process_probe_target",
            "next_process_package_target",
            "next_process_package_test",
            "next_process_package_ctest_filter",
            "next_process_package_timestamped_test",
            "next_process_package_timestamped_ctest_filter",
            "next_process_package_parameterized_test",
            "next_process_package_parameterized_ctest_filter",
            "next_process_package_connection_loss_test",
            "next_process_package_connection_loss_ctest_filter",
            "next_process_package_object_registration_test",
            "next_process_package_object_registration_ctest_filter",
            "next_process_package_named_registration_test",
            "next_process_package_named_registration_ctest_filter",
            "next_process_package_attribute_update_test",
            "next_process_package_attribute_update_ctest_filter",
            "next_process_package_directed_retraction_test",
            "next_process_package_directed_retraction_ctest_filter",
            "next_process_package_catalog_verifier",
        ):
            value = item.get(field)
            if value is not None and (not isinstance(value, str) or not value.strip()):
                errors.append(f"{item_id or '<unnamed>'} {field} must be a non-empty string")
        next_work_status = item.get("next_work_status")
        if next_work_status is not None and next_work_status not in {
            "planned",
            "in-progress",
            "blocked",
            "complete",
        }:
            errors.append(
                f"{item_id or '<unnamed>'} next_work_status must be planned, in-progress, blocked, or complete"
            )
        if next_work_status is not None and item.get("next_work_id") is None:
            errors.append(
                f"{item_id or '<unnamed>'} next_work_status requires next_work_id"
            )
        next_task = item.get("next_task")
        if next_task is not None and (not isinstance(next_task, str) or not next_task.strip()):
            errors.append(f"{item_id or '<unnamed>'} next_task must be a non-empty string")
        next_lane = item.get("next_lane")
        if isinstance(next_lane, str) and next_lane not in known_tags:
            errors.append(f"{item_id or '<unnamed>'} next_lane is absent from the Catch2 plan: {next_lane}")
        next_source_lane = item.get("next_source_lane")
        if isinstance(next_source_lane, str) and next_source_lane not in known_tags:
            errors.append(
                f"{item_id or '<unnamed>'} next_source_lane is absent from the Catch2 plan: {next_source_lane}"
            )
        next_test_query = item.get("next_test_query")
        if isinstance(next_test_query, str) and next_test_query not in known_test_cases:
            errors.append(f"{item_id or '<unnamed>'} next_test_query is absent from the Catch2 plan: {next_test_query}")
        next_source_test_query = item.get("next_source_test_query")
        next_source_location = item.get("next_source_location")
        next_source_state = item.get("next_source_state")
        if next_source_test_query is not None and not isinstance(next_source_test_query, str):
            errors.append(f"{item_id or '<unnamed>'} next_source_test_query must be a non-empty string")
        if next_source_location is not None and not isinstance(next_source_location, str):
            errors.append(f"{item_id or '<unnamed>'} next_source_location must be a non-empty string")
        if next_source_state is not None and next_source_state not in {
            "unplanned-source",
            "planned",
        }:
            errors.append(
                f"{item_id or '<unnamed>'} next_source_state must be unplanned-source or planned"
            )
        if isinstance(next_source_test_query, str) and next_source_test_query.strip():
            if not isinstance(next_source_location, str) or not next_source_location.strip():
                errors.append(
                    f"{item_id or '<unnamed>'} next_source_test_query requires next_source_location"
                )
            elif source_locations is not None:
                expected_source_locations = {
                    f"{location.get('path')}:{location.get('line')}"
                    for location in source_locations.get(next_source_test_query, [])
                    if isinstance(location, dict)
                    and isinstance(location.get("path"), str)
                    and location.get("line") is not None
                }
                # ``planned`` is an explicit future-test pointer.  Its file
                # location is a source target, not a claim that the TEST_CASE
                # already exists.  Existing unplanned-source pointers retain
                # the strict derived-location check.
                if (
                    next_source_state != "planned"
                    and next_source_location not in expected_source_locations
                ):
                    errors.append(
                        f"{item_id or '<unnamed>'} next_source_location does not match "
                        f"the derived TEST_CASE location: {next_source_location}"
                    )
        elif next_source_location is not None:
            errors.append(
                f"{item_id or '<unnamed>'} next_source_location requires next_source_test_query"
            )
        next_test_role = item.get("next_test_role")
        if next_test_role is not None and next_test_role not in {"planned", "baseline"}:
            errors.append(
                f"{item_id or '<unnamed>'} next_test_role must be planned or baseline"
            )
        if next_test_role == "baseline" and not isinstance(next_test_query, str):
            errors.append(
                f"{item_id or '<unnamed>'} next_test_role=baseline requires next_test_query"
            )
        next_ctest_filter = item.get("next_ctest_filter")
        if isinstance(next_ctest_filter, str):
            if not (next_ctest_filter.startswith("[") and next_ctest_filter.endswith("]")):
                errors.append(f"{item_id or '<unnamed>'} next_ctest_filter must be a Catch2 tag filter: {next_ctest_filter}")
            elif next_ctest_filter[1:-1] not in known_tags:
                errors.append(f"{item_id or '<unnamed>'} next_ctest_filter is absent from the Catch2 plan: {next_ctest_filter}")
        next_source_ctest_filter = item.get("next_source_ctest_filter")
        if isinstance(next_source_ctest_filter, str):
            if not (
                next_source_ctest_filter.startswith("[")
                and next_source_ctest_filter.endswith("]")
            ):
                errors.append(
                    f"{item_id or '<unnamed>'} next_source_ctest_filter must be a Catch2 tag filter: {next_source_ctest_filter}"
                )
            elif next_source_ctest_filter[1:-1] not in known_tags:
                errors.append(
                    f"{item_id or '<unnamed>'} next_source_ctest_filter is absent from the Catch2 plan: {next_source_ctest_filter}"
                )
        for field in (
            "next_standard_sections",
            "next_plan_ids",
            "next_source_requirement_ids",
            "next_source_standard_sections",
            "next_source_api_surfaces",
        ):
            value = item.get(field)
            if value is not None and not isinstance(value, list):
                errors.append(f"{item_id or '<unnamed>'} {field} must be an array")
            elif isinstance(value, list) and any(
                not isinstance(entry, str) or not entry.strip() for entry in value
            ):
                errors.append(f"{item_id or '<unnamed>'} {field} must contain only non-empty strings")
        next_standard_sections = item.get("next_standard_sections")
        if isinstance(next_standard_sections, list):
            for section in next_standard_sections:
                if isinstance(section, str) and section not in known_standard_sections:
                    errors.append(
                        f"{item_id or '<unnamed>'} next_standard_sections references unknown 2025 section: {section}"
                    )
        next_plan_ids = item.get("next_plan_ids")
        if isinstance(next_plan_ids, list):
            for plan_id in next_plan_ids:
                if isinstance(plan_id, str) and plan_id not in known_test_ids:
                    errors.append(f"{item_id or '<unnamed>'} next_plan_ids references unknown Catch2 plan id: {plan_id}")
        next_source_requirement_ids = item.get("next_source_requirement_ids")
        if isinstance(next_source_requirement_ids, list):
            for requirement_id in next_source_requirement_ids:
                if isinstance(requirement_id, str) and requirement_id not in standard_requirements:
                    errors.append(
                        f"{item_id or '<unnamed>'} next_source_requirement_ids references unknown 2025 requirement: {requirement_id}"
                    )
        next_source_standard_sections = item.get("next_source_standard_sections")
        if isinstance(next_source_standard_sections, list):
            for section in next_source_standard_sections:
                if isinstance(section, str) and section not in known_standard_sections:
                    errors.append(
                        f"{item_id or '<unnamed>'} next_source_standard_sections references unknown 2025 section: {section}"
                    )
        catalog_gap_plan_ids = item.get("catalog_gap_plan_ids")
        if catalog_gap_plan_ids is not None and not isinstance(catalog_gap_plan_ids, list):
            errors.append(f"{item_id or '<unnamed>'} catalog_gap_plan_ids must be an array")
        elif isinstance(catalog_gap_plan_ids, list):
            for plan_id in catalog_gap_plan_ids:
                if not isinstance(plan_id, str) or not plan_id.strip():
                    errors.append(
                        f"{item_id or '<unnamed>'} catalog_gap_plan_ids must contain only non-empty strings"
                    )
                elif plan_id not in known_test_ids:
                    errors.append(
                        f"{item_id or '<unnamed>'} catalog_gap_plan_ids references unknown Catch2 plan id: {plan_id}"
                    )
        for tag in strings(item.get("query_tags")):
            if tag not in known_tags:
                errors.append(f"{item_id or '<unnamed>'} query tag is absent from the Catch2 plan: {tag}")
    for test in plan.get("tests", []):
        if not isinstance(test, dict):
            continue
        for requirement_id in strings(test.get("selected_requirements_lab_requirement_ids")):
            if requirement_id not in standard_requirements:
                errors.append(
                    f"Catch2 test {test.get('id', '<unnamed>')} references unknown 2025 requirement {requirement_id}"
                )
    recent_slices = index.get("recent_completed_slices", [])
    if recent_slices is not None and not isinstance(recent_slices, list):
        errors.append("roadmap index recent_completed_slices must be an array")
        recent_slices = []
    known_test_ids = {
        test.get("id")
        for test in tests
        if isinstance(test.get("id"), str)
    }
    seen_recent_plan_ids: set[str] = set()
    seen_recent_queries: set[str] = set()
    for position, slice_value in enumerate(recent_slices):
        label = f"recent_completed_slices[{position}]"
        if not isinstance(slice_value, dict):
            errors.append(f"{label} must be an object")
            continue
        if focus_lane is not None and focus_lane not in strings(slice_value.get("lane_queries")):
            continue
        plan_id = slice_value.get("plan_id")
        if not isinstance(plan_id, str) or not plan_id.strip():
            errors.append(f"{label} is missing plan_id")
        elif plan_id not in known_test_ids:
            errors.append(f"{label} references unknown Catch2 plan id: {plan_id}")
        elif plan_id in seen_recent_plan_ids:
            errors.append(f"{label} duplicates recent completion plan_id: {plan_id}")
        else:
            seen_recent_plan_ids.add(plan_id)
        test_query = slice_value.get("test_query")
        if not isinstance(test_query, str) or not test_query.strip():
            errors.append(f"{label} is missing test_query")
            matches: list[dict[str, Any]] = []
        else:
            matches = [
                test
                for test in tests
                if test.get("test_case") == test_query
            ]
            if len(matches) != 1:
                errors.append(
                    f"{label} test_query must resolve to exactly one Catch2 case; "
                    f"found {len(matches)}"
                )
            elif isinstance(plan_id, str) and matches[0].get("id") != plan_id:
                errors.append(
                    f"{label} plan_id does not match its exact test_query: {plan_id}"
                )
        if isinstance(test_query, str) and test_query.strip():
            if test_query in seen_recent_queries:
                errors.append(f"{label} duplicates recent completion test_query: {test_query}")
            else:
                seen_recent_queries.add(test_query)
        source_location = slice_value.get("source_location")
        expected_locations = {
            f"{location.get('path')}:{location.get('line')}"
            for test in matches
            for location in test.get("source_locations", [])
            if isinstance(location, dict)
            and isinstance(location.get("path"), str)
            and location.get("line") is not None
        }
        if not isinstance(source_location, str) or not source_location.strip():
            errors.append(f"{label} is missing source_location")
        elif not expected_locations:
            # A lane-scoped check is the focused executable gate.  Historical
            # plan rows whose declaration is absent remain visible in the
            # result's source-drift count, but they must not block validation
            # of the source-backed cases in the selected lane.  The unscoped
            # check remains strict and is the reconciliation gate.
            if focus_lane is None:
                errors.append(f"{label} has no derived C++ source location")
        elif source_location not in expected_locations:
            errors.append(
                f"{label} source_location does not match the derived TEST_CASE location: "
                f"{source_location}"
            )
        assertions = slice_value.get("assertions")
        if not isinstance(assertions, int) or assertions <= 0:
            errors.append(f"{label} assertions must be a positive integer")
        status = slice_value.get("status")
        if not isinstance(status, str) or not status.strip():
            errors.append(f"{label} is missing status")
        sections = slice_value.get("standard_sections")
        if not isinstance(sections, list) or any(
            not isinstance(section, str) or not section.strip()
            for section in sections
        ):
            errors.append(f"{label} standard_sections must contain non-empty strings")
        elif any(section not in known_standard_sections for section in sections):
            errors.append(f"{label} standard_sections references an unknown 2025 section")
        lanes = slice_value.get("lane_queries")
        if not isinstance(lanes, list) or any(
            not isinstance(lane, str) or not lane.strip() for lane in lanes
        ):
            errors.append(f"{label} lane_queries must contain non-empty strings")
        else:
            for lane in lanes:
                if lane not in known_tags:
                    errors.append(
                        f"{label} lane_queries references an unknown Catch2 tag: {lane}"
                    )
    for test in scoped_tests:
        source_missing = test.get("status") == "source-missing-needs-reconciliation"
        if source_missing and test.get("source_locations"):
            errors.append(
                f"Catch2 test {test.get('id', '<unnamed>')} is marked source-missing but has a TEST_CASE source location"
            )
        elif not source_missing and not test.get("source_locations"):
            if focus_lane is None:
                errors.append(
                    f"Catch2 test {test.get('id', '<unnamed>')} has no matching TEST_CASE source location"
                )
    for document in index.get("documents", []):
        if not isinstance(document, dict):
            errors.append("roadmap index document entry must be an object")
            continue
        document_path = document.get("path")
        if not isinstance(document_path, str) or not (REPOSITORY_ROOT / document_path).is_file():
            errors.append(f"roadmap index document is missing: {document_path}")
    return errors


def text_test(test: dict[str, Any], verbose: bool, compact: bool = False) -> str:
    lines = [
        f"- {test.get('id', '<unnamed>')}: {test.get('test_case', '<unnamed test>')}",
        f"  status: {test.get('status', '<unspecified>')}",
        f"  mapping id: {test.get('requirements_lab_mapping_id') or '<none>'}",
        f"  tags: {', '.join(test.get('tags', [])) or '<none>'}",
        f"  source: {source_location_text(test)}",
        f"  standard: {', '.join(test.get('standard_clauses', [])) or '<unmapped>'}",
    ]
    reason = test.get("source_missing_reason")
    if isinstance(reason, str) and reason.strip():
        lines.append(f"  source-missing reason: {compact_prose(reason, 180)}")
    api_surface_ids = strings(test.get("selected_cpp_api_surface_ids"))
    if api_surface_ids:
        lines.append(f"  cpp api surfaces: {', '.join(api_surface_ids)}")
    if compact:
        if test.get("requirements"):
            lines.append("  requirements:")
            for requirement in test.get("requirements", []):
                standard = requirement.get("standard")
                if standard is None:
                    lines.append(f"    - {requirement['lab_requirement_id']} (unresolved)")
                    continue
                section = f"{standard.get('document_id')} {standard.get('clause_id')}"
                lines.append(f"    - {requirement['lab_requirement_id']} -> {section}")
        else:
            lines.append("  requirements: <unmapped>")
    else:
        for requirement in test.get("requirements", []):
            standard = requirement.get("standard")
            if standard is None:
                lines.append(f"  requirement: {requirement['lab_requirement_id']} (unresolved)")
                continue
            section = f"{standard.get('document_id')} {standard.get('clause_id')}"
            source = standard.get("source") or {}
            source_text = source.get("path") if isinstance(source, dict) else None
            if isinstance(source, dict) and source.get("line") is not None:
                source_text = f"{source_text}:{source['line']}"
            lines.append(
                f"  requirement: {requirement['lab_requirement_id']} -> {section}"
                f"; {standard.get('title', '')}"
            )
            if source_text:
                lines.append(f"    standard source: {source_text}")
            for contract in requirement.get("contracts", []):
                symbol = contract.get("source_symbol")
                if symbol:
                    lines.append(f"    contract: {contract.get('contract')} ({symbol})")
    if verbose and test.get("next_action"):
        lines.append(f"  next: {test['next_action']}")
    return "\n".join(lines)


def text_test_summary(test: dict[str, Any]) -> str:
    """Render one bounded line-oriented record for fast work selection."""

    requirements = strings(test.get("lab_requirement_ids"))
    sections = strings(test.get("standard_sections"))
    api_surfaces = strings(test.get("selected_cpp_api_surface_ids"))
    service_tags = [
        tag for tag in strings(test.get("tags")) if tag.startswith("rti.service.")
    ]
    callback_tags = [
        tag
        for tag in strings(test.get("tags"))
        if tag.startswith("federate.callback.")
    ]
    mapping = test.get("requirements_lab_mapping_id") or "<none>"
    # Keep the summary useful for traceability without allowing a highly
    # cross-cutting case to expand into a context-sized report.  The complete
    # values remain available from ``--json`` or the non-summary ``--compact``
    # view.
    preview_limit = 8

    def preview(values: list[str]) -> str:
        if not values:
            return "<none>"
        shown = ", ".join(values[:preview_limit])
        remaining = len(values) - preview_limit
        return f"{shown}, ... (+{remaining})" if remaining > 0 else shown

    focus_tags = [
        tag
        for tag in test.get("tags", [])
        if not tag.startswith("rti.service.")
        and not tag.startswith("federate.callback.")
    ]
    lines = [
        f"- {test.get('id', '<unnamed>')}: {test.get('test_case', '<unnamed test>')}",
        f"  status: {test.get('status', '<unspecified>')}",
        f"  source: {source_location_text(test)}",
        f"  assertions: {test.get('assertions', '<unspecified>')}; "
        f"callback_models={', '.join(strings(test.get('callback_models'))) or '<unspecified>'}; "
        f"delivery_modes={', '.join(strings(test.get('delivery_modes'))) or '<unspecified>'}; "
        f"callback_gate_modes={', '.join(strings(test.get('callback_gate_modes'))) or '<unspecified>'}",
        f"  mapping id: {mapping}; requirements={len(requirements)}; "
        f"sections={len(sections)}; cpp_api_surfaces={len(api_surfaces)}",
        f"  requirements: {preview(requirements)}",
        f"  standard sections: {preview(sections)}",
        f"  cpp api surfaces: {preview(api_surfaces)}",
        f"  service tags: {', '.join(service_tags) or '<none>'}",
        f"  callback tags: {', '.join(callback_tags) or '<none>'}",
        f"  focus tags: {', '.join(focus_tags) or '<none>'}",
    ]
    reason = test.get("source_missing_reason")
    if isinstance(reason, str) and reason.strip():
        lines.append(f"  source-missing reason: {compact_prose(reason, 180)}")
    return "\n".join(lines)


def test_summary_data(test: dict[str, Any]) -> dict[str, Any]:
    """Return the bounded machine-readable shape used by ``--summary``."""

    result = {
        "id": test.get("id"),
        "test_case": test.get("test_case"),
        "status": test.get("status"),
        "source_locations": test.get("source_locations", []),
        "source_state": test.get("source_state"),
        "assertions": test.get("assertions"),
        "callback_models": strings(test.get("callback_models")),
        "delivery_modes": strings(test.get("delivery_modes")),
        "callback_gate_modes": strings(test.get("callback_gate_modes")),
        "requirements_lab_mapping_id": test.get("requirements_lab_mapping_id"),
        "requirement_count": len(strings(test.get("lab_requirement_ids"))),
        "standard_sections": strings(test.get("standard_sections")),
        "cpp_api_surface_count": len(
            strings(test.get("selected_cpp_api_surface_ids"))
        ),
        "cpp_api_surfaces": strings(test.get("selected_cpp_api_surface_ids")),
        "service_tags": [
            tag
            for tag in strings(test.get("tags"))
            if tag.startswith("rti.service.")
        ],
        "callback_tags": [
            tag
            for tag in strings(test.get("tags"))
            if tag.startswith("federate.callback.")
        ],
        "focus_tags": [
            tag
            for tag in strings(test.get("tags"))
            if not tag.startswith("rti.service.")
            and not tag.startswith("federate.callback.")
        ],
    }
    reason = test.get("source_missing_reason")
    if isinstance(reason, str) and reason.strip():
        result["source_missing_reason"] = reason
    return result


def focused_lane_result(
    index: dict[str, Any],
    tests: list[dict[str, Any]],
    lane: str,
    limit: int = 8,
) -> dict[str, Any]:
    """Resolve one lane into a bounded, actionable work card.

    ``lane`` is deliberately an exact Catch2 tag.  This view is different
    from ``coverage``: it separates executable work from historical rows and
    tells a caller when every source-located case in the lane is already
    implemented.  That prevents a completed baseline from being mistaken for
    the next task and avoids reopening the full plan just to choose a focus
    slice.
    """

    selected = [test for test in tests if lane in strings(test.get("tags"))]
    status_counts = Counter(
        str(test.get("status") or "<unspecified>") for test in selected
    )
    implemented = [
        test
        for test in selected
        if str(test.get("status") or "").startswith(("implemented", "verified"))
    ]
    executable_candidates = [
        test
        for test in selected
        if not str(test.get("status") or "").startswith(("implemented", "verified"))
        and test.get("source_locations")
    ]
    source_drift = [test for test in selected if not test.get("source_locations")]
    mapped = [test for test in selected if test.get("requirements")]
    standard_sections = sorted(
        {
            section
            for test in selected
            for section in strings(test.get("standard_sections"))
        }
    )
    recorded_assertion_count = sum(
        value
        for test in selected
        for value in [test.get("assertions")]
        if isinstance(value, int)
    )
    executable_assertion_count = sum(
        value
        for test in executable_candidates
        for value in [test.get("assertions")]
        if isinstance(value, int)
    )
    requirement_ids = sorted(
        {
            requirement_id
            for test in selected
            for requirement_id in strings(test.get("lab_requirement_ids"))
        }
    )
    mapping = index.get("mapping", {})
    if not isinstance(mapping, dict):
        mapping = {}
    lane_assertion_counts = mapping.get("lane_assertion_counts", {})
    if not isinstance(lane_assertion_counts, dict):
        lane_assertion_counts = {}
    configured_assertion_count = lane_assertion_counts.get(lane)
    assertion_count = (
        configured_assertion_count
        if isinstance(configured_assertion_count, int)
        else recorded_assertion_count
    )
    assertion_count_source = (
        "indexed-lane-total"
        if isinstance(configured_assertion_count, int)
        else "plan-entry-records"
    )
    lane_owners = mapping.get("lane_owners", {})
    if not isinstance(lane_owners, dict):
        lane_owners = {}
    configured_handles = mapping.get("lane_handles", {})
    if not isinstance(configured_handles, dict):
        configured_handles = {}
    lane_handles = configured_handles.get(lane, {})
    if not isinstance(lane_handles, dict):
        lane_handles = {}
    candidate_limit = max(limit, 0)
    candidates = executable_candidates if candidate_limit == 0 else executable_candidates[:candidate_limit]
    return {
        "lane": lane,
        "roadmap_owner": lane_owners.get(lane),
        "lane_handles": dict(lane_handles),
        "lane_state": (
            "missing"
            if not selected
            else "complete"
            if not executable_candidates
            else "has-executable-candidates"
        ),
        "test_count": len(selected),
        "mapped_test_count": len(mapped),
        "source_located_test_count": len(selected) - len(source_drift),
        "source_drift_count": len(source_drift),
        "implemented_test_count": len(implemented),
        "candidate_count": len(executable_candidates),
        "shown_candidate_count": len(candidates),
        "assertion_count": assertion_count,
        "recorded_assertion_count": recorded_assertion_count,
        "assertion_count_source": assertion_count_source,
        "executable_assertion_count": executable_assertion_count,
        "status_counts": dict(sorted(status_counts.items())),
        "requirement_count": len(requirement_ids),
        "requirement_ids": requirement_ids,
        "standard_section_count": len(standard_sections),
        "standard_sections": standard_sections,
        "candidates": [test_summary_data(test) for test in candidates],
        "source_drift_test_ids": [test.get("id") for test in source_drift],
    }


def indexed_work_queue(
    index: dict[str, Any],
    tests: list[dict[str, Any]],
    limit: int = 8,
) -> dict[str, Any]:
    """Return a bounded queue of open roadmap families.

    The roadmap contains broad, overlapping query tags, so printing every tag
    is a poor work-selection interface.  This view keeps one row per open
    indexed family and joins only its exact next lane (when present).  It
    classifies a row as ready, complete-pointer, source-drift-only, or
    new-case-needed without changing the plan or re-reading the Lab.
    """

    items = sorted(
        (
            item
            for item in index.get("items", [])
            if isinstance(item, dict) and item.get("status") == "open"
        ),
        key=lambda item: item.get("priority", 999),
    )
    rows: list[dict[str, Any]] = []
    for item in items:
        selected = item_tests(item, tests)
        mapped_count = sum(bool(test.get("requirements")) for test in selected)
        source_drift_count = sum(not test.get("source_locations") for test in selected)
        candidate_count = sum(
            bool(test.get("source_locations"))
            and not str(test.get("status") or "").startswith(("implemented", "verified"))
            for test in selected
        )
        lane = item.get("next_lane")
        lane_snapshot: dict[str, Any] | None = None
        if isinstance(lane, str) and lane:
            lane_snapshot = focused_lane_result(index, tests, lane, limit=1)
        if lane_snapshot and lane_snapshot.get("lane_state") != "missing":
            case_count = lane_snapshot.get("test_count", 0)
            mapped_count = lane_snapshot.get("mapped_test_count", mapped_count)
            source_drift_count = lane_snapshot.get("source_drift_count", source_drift_count)
            candidate_count = lane_snapshot.get("candidate_count", candidate_count)
            if candidate_count:
                state = "ready"
            elif source_drift_count:
                state = "source-drift-only"
            else:
                state = "complete-pointer"
        elif candidate_count:
            case_count = len(selected)
            state = "ready"
        elif selected and source_drift_count:
            case_count = len(selected)
            state = "source-drift-only"
        else:
            case_count = len(selected)
            state = "new-case-needed"
        pointer = item.get("next_test_pointer")
        if not isinstance(pointer, dict):
            pointer = {}
        rows.append(
            {
                "id": item.get("id"),
                "title": item.get("title"),
                "priority": item.get("priority"),
                "kind": item.get("kind"),
                "next_work_id": item.get("next_work_id"),
                "next_work_status": item.get("next_work_status"),
                "next_lane": lane,
                "state": state,
                "case_count": case_count,
                "mapped_case_count": mapped_count,
                "source_drift_count": source_drift_count,
                "candidate_count": candidate_count,
                "tag_count": len(strings(item.get("query_tags"))),
                "next_test_query": item.get("next_test_query"),
                "next_test_state": pointer.get("state"),
                "next_action": item.get("next_action"),
                "work_query": item.get("next_work_query"),
            }
        )
    requested_limit = max(limit, 0)
    shown = rows if requested_limit == 0 else rows[:requested_limit]
    return {
        "items": shown,
        "shown_count": len(shown),
        "open_count": len(rows),
        "executable_candidate_count": sum(row["candidate_count"] for row in rows),
        "source_drift_count": sum(row["source_drift_count"] for row in rows),
        "mapped_case_count": sum(row["mapped_case_count"] for row in rows),
    }


def trace_requirement_row(requirement: dict[str, Any]) -> dict[str, Any]:
    """Return the direct Lab-requirement to 2025-subsection relationship."""

    standard = requirement.get("standard")
    if not isinstance(standard, dict):
        return {
            "lab_requirement_id": requirement.get("lab_requirement_id"),
            "unresolved": True,
        }
    source = standard.get("source")
    source_value = source if isinstance(source, dict) else None
    return {
        "lab_requirement_id": requirement.get("lab_requirement_id"),
        "document_id": standard.get("document_id"),
        "clause": standard.get("clause"),
        "clause_id": standard.get("clause_id"),
        "title": standard.get("title"),
        "source": source_value,
        "unresolved": False,
    }


def exact_requirement_match(requirement: dict[str, Any], query: str) -> bool:
    """Match only stable requirement/contract identifiers for ``trace``."""

    query_folded = query.casefold()
    values = strings(requirement.get("lab_requirement_id"))
    for contract in requirement.get("contracts", []):
        if isinstance(contract, dict):
            values.extend(strings(contract.get("contract_requirement_id")))
    return any(query_folded == value.casefold() for value in values)


def trace_section_match(section: str, query: str) -> bool:
    """Match one canonical document:clause key or its clause-only suffix."""

    folded = query.casefold()
    normalized = search_key(query)
    values = (section, section.replace(":", " "), section.rsplit(":", 1)[-1])
    return any(
        folded == value.casefold()
        or normalized == search_key(value)
        or (
            normalized
            and search_key(value).endswith(normalized)
            and normalized.startswith("clause")
        )
        for value in values
    )


def resolve_trace_query(
    tests: list[dict[str, Any]],
    query: str,
) -> tuple[str | None, list[dict[str, Any]]]:
    """Resolve one exact trace handle without falling back to broad search.

    The precedence is deliberate: exact plan/test handles, exact requirement
    identifiers, exact contract mapping identifiers, exact standard subsection
    keys, then exact Catch2 lane tags.
    A fuzzy repository-wide search remains available through ``search`` and is
    never implicit in this bounded trace command.
    """

    folded = query.casefold()
    exact_tests = [
        test
        for test in tests
        if str(test.get("id", "")).casefold() == folded
        or str(test.get("test_case", "")).casefold() == folded
    ]
    if exact_tests:
        return "test", exact_tests

    exact_requirements = [
        test
        for test in tests
        if any(
            isinstance(requirement, dict)
            and exact_requirement_match(requirement, query)
            for requirement in test.get("requirements", [])
        )
    ]
    if exact_requirements:
        return "requirement", exact_requirements

    exact_contract_mappings = [
        test
        for test in tests
        if str(test.get("requirements_lab_mapping_id", "")).casefold() == folded
    ]
    if exact_contract_mappings:
        return "contract", exact_contract_mappings

    exact_sections = [
        test
        for test in tests
        if any(
            trace_section_match(section, query)
            for section in strings(test.get("standard_sections"))
        )
    ]
    if exact_sections:
        return "section", exact_sections

    exact_lanes = [test for test in tests if query in strings(test.get("tags"))]
    if exact_lanes:
        return "lane", exact_lanes
    return None, []


def trace_record(
    test: dict[str, Any],
    match_kind: str,
    query: str,
    index: dict[str, Any] | None = None,
) -> dict[str, Any]:
    """Build a bounded, direct test-to-requirement trace record."""

    requirements = [
        requirement
        for requirement in test.get("requirements", [])
        if isinstance(requirement, dict)
    ]
    if match_kind == "requirement":
        requirements = [
            requirement
            for requirement in requirements
            if exact_requirement_match(requirement, query)
        ]
    elif match_kind == "section":
        requirements = [
            requirement
                for requirement in requirements
            if isinstance(requirement.get("standard"), dict)
            and trace_section_match(
                ":".join(
                    str(value)
                    for value in (
                        requirement["standard"].get("document_id"),
                        requirement["standard"].get("clause_id"),
                    )
                    if value
                ),
                query,
            )
        ]
    return {
        "id": test.get("id"),
        "test_case": test.get("test_case"),
        "status": test.get("status"),
        "source_locations": test.get("source_locations", []),
        "source_state": test.get("source_state"),
        "requirements_lab_mapping_id": test.get("requirements_lab_mapping_id"),
        "tags": strings(test.get("tags")),
        "cpp_api_surfaces": strings(test.get("selected_cpp_api_surface_ids")),
        "standard_sections": strings(test.get("standard_sections")),
        "roadmap_items": roadmap_links_for_test(index, test)
        if index is not None
        else [],
        "requirement_mapping_count": len(
            strings(test.get("lab_requirement_ids"))
        ),
        "matched_requirement_count": len(requirements),
        "requirement_mappings": [
            trace_requirement_row(requirement) for requirement in requirements
        ],
    }


def trace_summary_record(record: dict[str, Any], limit: int = 8) -> dict[str, Any]:
    """Bound the mapping rows while retaining exact counts for quick queries."""

    result = dict(record)
    for field in ("tags", "cpp_api_surfaces", "standard_sections"):
        values = strings(record.get(field))
        result[field] = values[:limit]
        if len(values) > limit:
            result[f"{field}_remaining"] = len(values) - limit
    mappings = record.get("requirement_mappings", [])
    if isinstance(mappings, list):
        result["requirement_mappings"] = mappings[:limit]
        if len(mappings) > limit:
            result["requirement_mappings_remaining"] = len(mappings) - limit
    roadmap_items = record.get("roadmap_items", [])
    if isinstance(roadmap_items, list):
        result["roadmap_items"] = roadmap_items[:limit]
        if len(roadmap_items) > limit:
            result["roadmap_items_remaining"] = len(roadmap_items) - limit
    return result


def text_trace(record: dict[str, Any], summary: bool = False) -> str:
    """Render one direct test → requirement → subsection trace."""

    def preview(values: list[str], limit: int = 8) -> str:
        if not values:
            return "<none>"
        shown = ", ".join(values[:limit])
        remaining = len(values) - limit
        return f"{shown}, ... (+{remaining})" if remaining > 0 else shown

    tags = strings(record.get("tags"))
    api_surfaces = strings(record.get("cpp_api_surfaces"))
    lines = [
        f"- {record.get('id', '<unnamed>')}: {record.get('test_case', '<unnamed test>')}",
        f"  status: {record.get('status', '<unspecified>')}",
        f"  source: {source_location_text(record)}",
        f"  mapping id: {record.get('requirements_lab_mapping_id') or '<none>'}",
        f"  tags: {preview(tags) if summary else ', '.join(tags) or '<none>'}",
        f"  cpp api surfaces: {preview(api_surfaces) if summary else ', '.join(api_surfaces) or '<none>'}",
        "  roadmap families: "
        + (
            ", ".join(
                f"{item.get('id')} ({item.get('match')})"
                for item in record.get("roadmap_items", [])
                if isinstance(item, dict)
            )
            or "<none>"
        ),
        "  requirement mappings:",
    ]
    mappings = record.get("requirement_mappings", [])
    if not isinstance(mappings, list) or not mappings:
        lines.append("    - <none>")
        return "\n".join(lines)
    shown = mappings[:8] if summary else mappings
    for mapping in shown:
        if not isinstance(mapping, dict):
            continue
        requirement_id = mapping.get("lab_requirement_id", "<unnamed requirement>")
        if mapping.get("unresolved"):
            lines.append(f"    - {requirement_id} (unresolved)")
            continue
        section = ":".join(
            str(value)
            for value in (mapping.get("document_id"), mapping.get("clause_id"))
            if value
        ) or "<unmapped section>"
        title = mapping.get("title") or ""
        lines.append(f"    - {requirement_id} -> {section}; {title}")
    remaining = len(mappings) - len(shown)
    if remaining > 0:
        lines.append(f"    - ... (+{remaining}); use trace --json for all rows")
    return "\n".join(lines)


def test_search_values(test: dict[str, Any]) -> list[str]:
    """Return all human-queryable identifiers carried by a mapped test."""

    values: list[str] = []
    for field in (
        "id",
        "test_case",
        "requirements_lab_mapping_id",
        "status",
        "tags",
        "selected_cpp_api_surface_ids",
        "standard_clauses",
        "standard_sections",
        "source_state",
        "lab_requirement_ids",
    ):
        values.extend(strings(test.get(field)))
    for requirement in test.get("requirements", []):
        if not isinstance(requirement, dict):
            continue
        values.extend(strings(requirement.get("lab_requirement_id")))
        standard = requirement.get("standard")
        if isinstance(standard, dict):
            for field in ("document_id", "clause", "clause_id", "title"):
                values.extend(strings(standard.get(field)))
            source = standard.get("source")
            if isinstance(source, dict):
                values.extend(strings(source.get("path")))
                if source.get("line") is not None:
                    values.append(str(source["line"]))
        for contract in requirement.get("contracts", []):
            if isinstance(contract, dict):
                values.extend(strings(contract.get("contract_requirement_id")))
                values.extend(strings(contract.get("source_symbol")))
    for location in test.get("source_locations", []):
        if isinstance(location, dict):
            values.extend(strings(location.get("path")))
            if location.get("line") is not None:
                values.append(str(location["line"]))
    return values


def select_tests(tests: list[dict[str, Any]], query: str, mode: str) -> list[dict[str, Any]]:
    folded = query.casefold()
    if mode == "lane":
        return [test for test in tests if query in test.get("tags", [])]
    if mode == "section":
        # Standard mappings are published as canonical
        # ``document_id:clause_id`` keys.  Accept the canonical key, the
        # space-separated form shown by the text renderer, or a clause-only
        # suffix (for example ``clause-9.13.1``) without doing a broad
        # free-text search across every test field.
        normalized = search_key(query)
        selected: list[dict[str, Any]] = []
        for test in tests:
            for section in strings(test.get("standard_sections")):
                section_values = (
                    section,
                    section.replace(":", " "),
                    section.rsplit(":", 1)[-1],
                )
                if any(
                    folded == value.casefold()
                    or normalized == search_key(value)
                    or (
                        normalized
                        and search_key(value).endswith(normalized)
                        and normalized.startswith("clause")
                    )
                    for value in section_values
                ):
                    selected.append(test)
                    break
        return selected
    if mode == "requirement":
        selected: list[dict[str, Any]] = []
        for test in tests:
            for requirement in test.get("requirements", []):
                standard = requirement.get("standard") or {}
                contract_ids = {
                    str(contract.get("contract_requirement_id"))
                    for contract in requirement.get("contracts", [])
                }
                values = {
                    requirement.get("lab_requirement_id"),
                    standard.get("clause"),
                    standard.get("clause_id"),
                    standard.get("document_id"),
                    standard.get("title"),
                    *contract_ids,
                }
                document_id = standard.get("document_id")
                clause_id = standard.get("clause_id")
                if document_id and clause_id:
                    # The compact test output publishes this canonical
                    # document:clause key. Accept it as a direct query too,
                    # so selecting a standard subsection does not require a
                    # second lookup to discover the Lab requirement id.
                    values.add(f"{document_id}:{clause_id}")
                    values.add(f"{document_id} {clause_id}")
                source = standard.get("source")
                if isinstance(source, dict):
                    values.update(
                        value
                        for value in (source.get("path"), source.get("line"))
                        if value is not None
                    )
                normalized = search_key(query)
                if query in values or any(
                    isinstance(value, str)
                    and (
                        folded in value.casefold()
                        or (normalized and normalized in search_key(value))
                    )
                    for value in values
                ):
                    selected.append(test)
                    break
        return selected
    if mode == "search":
        normalized = search_key(query)
        return [
            test
            for test in tests
            if any(
                folded in value.casefold()
                or (normalized and normalized in search_key(value))
                for value in test_search_values(test)
            )
        ]
    return [
        test
        for test in tests
        if folded in str(test.get("id", "")).casefold()
        or folded in str(test.get("test_case", "")).casefold()
    ]


def build_parser() -> argparse.ArgumentParser:
    value = argparse.ArgumentParser(description=__doc__)
    value.add_argument("--index", type=Path, default=DEFAULT_INDEX)
    value.add_argument("--plan", type=Path, default=DEFAULT_PLAN)
    value.add_argument("--bundle", type=Path, default=DEFAULT_BUNDLE)
    value.add_argument("--contract-directory", type=Path, default=DEFAULT_CONTRACT_DIRECTORY)
    value.add_argument("--roadmap", type=Path, default=DEFAULT_ROADMAP)
    value.add_argument(
        "--implementation-plan",
        type=Path,
        default=DEFAULT_IMPLEMENTATION_PLAN,
        help="implementation plan to outline (used by the plan command)",
    )
    value.add_argument(
        "--test-root",
        type=Path,
        default=DEFAULT_TEST_ROOT,
        help="C++ Catch2 source root used for derived test locations",
    )
    value.add_argument("--json", action="store_true", help="emit machine-readable JSON")
    value.add_argument("--verbose", action="store_true", help="include per-test next actions")
    value.add_argument(
        "--compact",
        action="store_true",
        help="omit contract provenance while retaining requirement and clause mappings",
    )
    value.add_argument(
        "--summary",
        action="store_true",
        help="emit one bounded record per test without individual requirement rows",
    )
    subcommands = value.add_subparsers(dest="command", required=True)

    def add_output_flags(command: argparse.ArgumentParser) -> None:
        # Suppress the subparser defaults so flags work both before and after
        # the subcommand without a post-parse argv rewrite.
        command.add_argument(
            "--json",
            action="store_true",
            default=argparse.SUPPRESS,
            help="emit machine-readable JSON",
        )
        command.add_argument(
            "--verbose",
            action="store_true",
            default=argparse.SUPPRESS,
            help="include per-test next actions",
        )
        command.add_argument(
            "--compact",
            action="store_true",
            default=argparse.SUPPRESS,
            help="omit contract provenance while retaining requirement and clause mappings",
        )
        command.add_argument(
            "--summary",
            action="store_true",
            default=argparse.SUPPRESS,
            help="emit one bounded record per test without individual requirement rows",
        )
        command.add_argument(
            "--test-root",
            type=Path,
            default=argparse.SUPPRESS,
            help="C++ Catch2 source root used for derived test locations",
        )

    status = subcommands.add_parser("status", help="show open roadmap items and checklist counts")
    add_output_flags(status)
    queue = subcommands.add_parser(
        "queue",
        help=(
            "show a bounded queue of open roadmap families and exact lane states"
        ),
    )
    queue.add_argument(
        "--limit",
        type=int,
        default=8,
        help="maximum open roadmap families to print (default: 8; use 0 for all)",
    )
    add_output_flags(queue)
    next_item = subcommands.add_parser(
        "next",
        help="show the highest-priority open roadmap item and its query handles",
    )
    next_item.add_argument(
        "--all",
        action="store_true",
        help="show every open indexed item instead of only the highest-priority item",
    )
    add_output_flags(next_item)
    focus = subcommands.add_parser(
        "focus",
        help=(
            "show one exact lane's executable candidates, mapping counts, "
            "and copyable execution handles"
        ),
    )
    focus.add_argument(
        "lane",
        nargs="?",
        help=(
            "exact Catch2 lane tag; omit to use the active indexed work lane"
        ),
    )
    focus.add_argument(
        "--limit",
        type=int,
        default=8,
        help="maximum candidate tests to print (default: 8; use 0 for all)",
    )
    add_output_flags(focus)
    work = subcommands.add_parser(
        "work",
        help=(
            "resolve one actionable implementation slice, its exact baseline "
            "test, lane, and 2025 mappings"
        ),
    )
    work.add_argument(
        "id",
        nargs="?",
        help=(
            "indexed roadmap/work item id; omit to follow the highest-priority "
            "open item's next_work_id"
        ),
    )
    add_output_flags(work)
    recent = subcommands.add_parser(
        "recent",
        help="show the bounded ledger of recently completed slices",
    )
    recent.add_argument(
        "--limit",
        type=int,
        default=10,
        help="maximum completed slices to print (default: 10; use 0 for all)",
    )
    recent.add_argument(
        "--lane",
        help="show only indexed slices carrying this exact Catch2 lane tag",
    )
    add_output_flags(recent)
    item = subcommands.add_parser(
        "item",
        help="show one indexed roadmap item and its tagged C++ cases",
    )
    item.add_argument("id")
    item.add_argument(
        "--limit",
        type=int,
        default=20,
        help="maximum tests to print (default: 20; use 0 for all)",
    )
    add_output_flags(item)
    plan_outline = subcommands.add_parser(
        "plan",
        help="show the implementation-plan section outline",
    )
    add_output_flags(plan_outline)
    coverage = subcommands.add_parser("coverage", help="summarize C++ test and clause mapping coverage")
    coverage.add_argument(
        "--lane",
        help="limit coverage to one exact Catch2 lane tag",
    )
    add_output_flags(coverage)
    check = subcommands.add_parser(
        "check",
        help="validate roadmap anchors and requirement references",
    )
    check.add_argument(
        "--lane",
        help=(
            "limit test/source/recent-slice validation to one exact Catch2 lane; "
            "roadmap structure remains checked globally"
        ),
    )
    add_output_flags(check)
    lane = subcommands.add_parser("lane", help="show tests carrying one exact Catch2 tag")
    lane.add_argument("tag")
    lane.add_argument(
        "--limit",
        type=int,
        default=20,
        help="maximum tests to print (default: 20; use 0 for all)",
    )
    add_output_flags(lane)
    test = subcommands.add_parser("test", help="find tests by id or case-name substring")
    test.add_argument("query")
    test.add_argument(
        "--limit",
        type=int,
        default=20,
        help="maximum tests to print (default: 20; use 0 for all)",
    )
    add_output_flags(test)
    requirement = subcommands.add_parser(
        "requirement",
        help="find tests by Lab id, contract id, clause, or document",
    )
    requirement.add_argument("query")
    requirement.add_argument(
        "--limit",
        type=int,
        default=20,
        help="maximum tests to print (default: 20; use 0 for all)",
    )
    add_output_flags(requirement)
    section = subcommands.add_parser(
        "section",
        help="find tests mapped to one exact 2025 standard clause/subsection",
    )
    section.add_argument("query")
    section.add_argument(
        "--limit",
        type=int,
        default=20,
        help="maximum tests to print (default: 20; use 0 for all)",
    )
    add_output_flags(section)
    trace = subcommands.add_parser(
        "trace",
        help=(
            "show a bounded direct test-to-Requirements-Lab-to-2025-section "
            "mapping for one exact handle"
        ),
    )
    trace.add_argument(
        "query",
        help=(
            "exact Catch2 plan id/title, Lab requirement id, canonical "
            "2025 document:clause key, or exact Catch2 lane tag"
        ),
    )
    trace.add_argument(
        "--limit",
        type=int,
        default=5,
        help="maximum test mappings to print (default: 5; use 0 for all)",
    )
    add_output_flags(trace)
    search = subcommands.add_parser(
        "search",
        help="find tests by any id, tag, API surface, requirement, or standard section",
    )
    search.add_argument("query")
    search.add_argument(
        "--limit",
        type=int,
        default=20,
        help="maximum tests to print (default: 20; use 0 for all)",
    )
    add_output_flags(search)
    unplanned = subcommands.add_parser(
        "unplanned",
        help="show C++ TEST_CASE declarations that have no exact Catch2 plan row",
    )
    unplanned.add_argument(
        "--limit",
        type=int,
        default=20,
        help="maximum source cases to print (default: 20; use 0 for all)",
    )
    unplanned.add_argument(
        "--path",
        help="limit the queue to source paths containing this case-insensitive substring",
    )
    add_output_flags(unplanned)
    unmapped = subcommands.add_parser(
        "unmapped",
        help="show Catch2 cases that do not yet select a Requirements-Lab requirement",
    )
    unmapped.add_argument(
        "--limit",
        type=int,
        default=20,
        help="maximum tests to print (default: 20; use 0 for all)",
    )
    unmapped.add_argument(
        "--lane",
        help="limit the queue to one exact Catch2 lane tag",
    )
    add_output_flags(unmapped)
    unlocated = subcommands.add_parser(
        "unlocated",
        help="show plan cases whose TEST_CASE declaration is absent from the current C++ source",
    )
    unlocated.add_argument(
        "--limit",
        type=int,
        default=20,
        help="maximum tests to print (default: 20; use 0 for all)",
    )
    unlocated.add_argument(
        "--lane",
        help="limit the queue to one exact Catch2 lane tag",
    )
    add_output_flags(unlocated)
    lanes = subcommands.add_parser(
        "lanes",
        help="list exact Catch2 tags with their case counts for lane discovery",
    )
    lanes.add_argument(
        "--limit",
        type=int,
        default=None,
        help="maximum lane rows to print (default: 40 with --compact/--summary; 0 for all)",
    )
    add_output_flags(lanes)
    return value


def main() -> int:
    arguments = build_parser().parse_args()
    try:
        index = load_json(arguments.index)
        plan = load_json(arguments.plan)
        bundle = load_json(arguments.bundle)
        standard_requirements = load_standard_requirements(bundle)
        contract_links = load_contract_links(arguments.contract_directory)
        source_locations = load_test_source_locations(arguments.test_root)
        tests = all_mapped_tests(
            plan,
            standard_requirements,
            contract_links,
            source_locations,
        )
        roadmap_entries = roadmap_checklist(arguments.roadmap)
    except ValueError as error:
        print(f"query_rti_work: {error}", file=sys.stderr)
        return 2
    except OSError as error:
        print(f"query_rti_work: {error}", file=sys.stderr)
        return 2

    if arguments.command == "check":
        scoped_tests = (
            tests
            if arguments.lane is None
            else [
                test
                for test in tests
                if arguments.lane in strings(test.get("tags"))
            ]
        )
        errors = validate_index(
            index,
            arguments.roadmap,
            plan,
            standard_requirements,
            tests,
            source_locations=source_locations,
            focus_lane=arguments.lane,
        )
        index_result = index_status(index, tests, roadmap_entries, source_locations)
        warnings = index_warnings(index_result, arguments.lane)
        mapped_test_count = sum(bool(test.get("requirements")) for test in scoped_tests)
        scoped_roadmap_item_count = sum(
            arguments.lane is None or item_matches_lane(item, arguments.lane)
            for item in index.get("items", [])
            if isinstance(item, dict)
        )
        result = {
            "ok": not errors,
            "errors": errors,
            "warnings": warnings,
            "scope_lane": arguments.lane,
            "roadmap_item_count": scoped_roadmap_item_count,
            "test_count": len(scoped_tests),
            "repository_test_count": len(tests),
            "mapped_test_count": mapped_test_count,
            "unmapped_test_count": len(scoped_tests) - mapped_test_count,
            "unique_standard_clause_count": len(
                {
                    clause
                    for test in scoped_tests
                    for clause in test.get("standard_clauses", [])
                }
            ),
            "standard_requirement_count": len(standard_requirements),
            "tests_without_source_location": sum(
                not test.get("source_locations") for test in scoped_tests
            ),
            "unlocated_test_ids": [
                test.get("id")
                for test in scoped_tests
                if not test.get("source_locations")
            ],
        }
        if arguments.json:
            print(json.dumps(result, indent=2, sort_keys=True))
        elif errors:
            if arguments.compact or arguments.summary:
                # Keep the normal integrity gate bounded even when the dirty
                # checkout contains a large source-drift queue.  The complete
                # diagnostic remains available through ``--json`` (or the
                # unqualified text form when a human needs every row).
                sample_limit = 12
                scope = (
                    f" for lane {arguments.lane!r}"
                    if arguments.lane
                    else ""
                )
                print(
                    "Roadmap/test mapping check failed: "
                    f"{len(errors)} errors{scope}; {len(scoped_tests)} cases, "
                    f"{mapped_test_count} mapped, "
                    f"{len(scoped_tests) - mapped_test_count} without a requirement mapping, "
                    f"{result['tests_without_source_location']} without a C++ source location."
                )
                print("Sample errors:")
                print("\n".join(f"- {error}" for error in errors[:sample_limit]))
                if len(errors) > sample_limit:
                    print(
                        f"- ... {len(errors) - sample_limit} more; "
                        "use check --json for the complete diagnostic"
                    )
            else:
                print("Roadmap/test mapping check failed:")
                print("\n".join(f"- {error}" for error in errors))
        else:
            scope = (
                f" for lane {arguments.lane!r}"
                if arguments.lane
                else ""
            )
            print(
                f"Roadmap/test mapping check passed{scope}: "
                f"{result['roadmap_item_count']} roadmap items, "
                f"{len(scoped_tests)} Catch2 cases ({mapped_test_count} mapped, "
                f"{len(scoped_tests) - mapped_test_count} explicitly unmapped), "
                f"{result['unique_standard_clause_count']} standard clauses, "
                f"{len(standard_requirements)} 2025 standard requirements; "
                f"{result['tests_without_source_location']} tests without a C++ source location."
            )
            if result["unlocated_test_ids"]:
                if arguments.compact or arguments.summary:
                    sample_limit = 12
                    print(
                        f"Unlocated Catch2 plan entries: {len(result['unlocated_test_ids'])}"
                    )
                    print(
                        "\n".join(
                            f"- {identifier}"
                            for identifier in result["unlocated_test_ids"][:sample_limit]
                        )
                    )
                    if len(result["unlocated_test_ids"]) > sample_limit:
                        print(
                            f"- ... {len(result['unlocated_test_ids']) - sample_limit} more; "
                            "use check --json for the complete list"
                        )
                else:
                    print("Unlocated Catch2 plan entries:")
                    print("\n".join(f"- {identifier}" for identifier in result["unlocated_test_ids"]))
            if warnings:
                print("Roadmap index warnings:")
                print("\n".join(f"- {warning}" for warning in warnings))
        return 0 if not errors else 1

    if arguments.command == "queue":
        result = indexed_work_queue(index, tests, arguments.limit)
        if arguments.json:
            print(json.dumps(result, indent=2, sort_keys=True))
            return 0
        print(
            "Indexed roadmap queue: "
            f"{result['open_count']} open families; "
            f"{result['executable_candidate_count']} executable candidates; "
            f"{result['source_drift_count']} source-drift rows."
        )
        if not result["items"]:
            print("No open indexed roadmap families.")
            return 0
        if result["shown_count"] < result["open_count"]:
            print(
                f"Showing {result['shown_count']}; use --limit 0 for all indexed families."
            )
        for row in result["items"]:
            print(
                f"{row.get('priority', '?')}. {row.get('id')}: {row.get('state')} "
                f"cases={row.get('case_count', 0)} "
                f"mapped={row.get('mapped_case_count', 0)} "
                f"candidates={row.get('candidate_count', 0)} "
                f"source_drift={row.get('source_drift_count', 0)}"
            )
            if row.get("next_work_id"):
                print(
                    f"   work={row['next_work_id']}"
                    + (
                        f" status={row['next_work_status']}"
                        if row.get("next_work_status")
                        else ""
                    )
                )
            if row.get("next_lane"):
                print(
                    "   focus_query="
                    "python tools/query_rti_work.py focus "
                    f"{row['next_lane']} --summary --compact"
                )
            print(
                "   work_query="
                f"python tools/query_rti_work.py work {row.get('id')} --summary --compact"
            )
            if row.get("next_action"):
                next_action = (
                    compact_prose(row["next_action"], 180)
                    if arguments.compact or arguments.summary
                    else row["next_action"]
                )
                print(f"   next={next_action}")
        return 0

    if arguments.command == "status":
        result = index_status(index, tests, roadmap_entries, source_locations)
        if arguments.json:
            print(json.dumps(result, indent=2, sort_keys=True))
            return 0
        print(
            f"Roadmap checklist: {result['roadmap_counts'].get('complete', 0)} complete, "
            f"{result['roadmap_counts'].get('open', 0)} open"
        )
        print(f"C++ Catch2 plan: {result['test_count']} cases")
        mapping_counts = result.get("derived_mapping_counts", {})
        if isinstance(mapping_counts, dict):
            print(
                "Traceability queues: "
                f"{mapping_counts.get('catch2_cases_without_lab_requirement_mapping', '?')} unmapped, "
                f"{mapping_counts.get('catch2_cases_without_cpp_source_location', '?')} source-unlocated, "
                f"{mapping_counts.get('cpp_source_cases_without_plan_row', '?')} source-only"
            )
        indexed_counts = result.get("mapping_counts", {})
        indexed_queue_counts = (
            {key: indexed_counts.get(key) for key in mapping_counts}
            if isinstance(indexed_counts, dict)
            else {}
        )
        if isinstance(indexed_counts, dict) and indexed_queue_counts != mapping_counts:
            print("Traceability index snapshot differs from the live local counts; run the bounded queries before editing the index.")
        print("Open indexed work:")
        for item in sorted(result["roadmap_items"], key=lambda value: value.get("priority", 999)):
            if item.get("status") != "open":
                continue
            print(f"{item.get('priority', '?')}. {item.get('id')}: {item.get('title')}")
            if arguments.compact:
                print(
                    f"   kind={item.get('kind')} tag_count={len(item.get('query_tags', []))} "
                    f"tagged_cases={item.get('matching_test_count', 0)}"
                )
            else:
                counts = ", ".join(
                    f"{tag}={count}"
                    for tag, count in item["matching_test_counts"].items()
                ) or "no tagged cases"
                print(f"   kind={item.get('kind')} tags=({counts})")
            if item.get("current_focus"):
                focus = compact_prose(item.get("current_focus")) if arguments.compact else item.get("current_focus")
                print(f"   focus={focus}")
            if item.get("next_task"):
                task = compact_prose(item.get("next_task")) if arguments.compact else item.get("next_task")
                print(f"   task={task}")
            handles = item_handles(item, arguments.compact)
            if handles:
                print(f"   handles={' '.join(handles)}")
            next_action = compact_prose(item.get("next_action")) if arguments.compact else item.get("next_action")
            print(f"   next={next_action}")
        return 0

    if arguments.command == "next":
        result = index_status(index, tests, roadmap_entries, source_locations)
        open_items = [
            item
            for item in sorted(
                result["roadmap_items"], key=lambda value: value.get("priority", 999)
            )
            if item.get("status") == "open"
        ]
        selected_items = open_items if arguments.all else open_items[:1]
        if arguments.json:
            print(json.dumps({"items": selected_items}, indent=2, sort_keys=True))
            return 0 if selected_items else 1
        if not selected_items:
            print("No open indexed roadmap items.")
            return 0
        print("Next indexed roadmap work:")
        for item in selected_items:
            print(f"{item.get('priority', '?')}. {item.get('id')}: {item.get('title')}")
            if arguments.summary:
                if item.get("next_task"):
                    print(f"   task={compact_prose(item.get('next_task'), 180)}")
                if item.get("next_work_id"):
                    print(f"   work={item.get('next_work_id')}")
                    if item.get("next_work_id") != item.get("id"):
                        print(f"   work_item={item.get('next_work_id')}")
                if item.get("next_work_status"):
                    print(f"   work_status={item.get('next_work_status')}")
                if item.get("next_work_query"):
                    print(
                        f"   work_query={compact_prose(item.get('next_work_query'), 180)}"
                    )
                if item.get("next_lane"):
                    print(f"   lane={item.get('next_lane')}")
                    lane_snapshot = focused_lane_result(
                        index, tests, item.get("next_lane"), limit=1
                    )
                    print(
                        "   lane_state="
                        f"{lane_snapshot.get('lane_state', 'missing')} "
                        f"cases={lane_snapshot.get('test_count', 0)} "
                        f"mapped={lane_snapshot.get('mapped_test_count', 0)} "
                        f"candidates={lane_snapshot.get('candidate_count', 0)} "
                        f"source_drift={lane_snapshot.get('source_drift_count', 0)}"
                    )
                    print(
                        "   focus_query="
                        "python tools/query_rti_work.py focus "
                        f"{item.get('next_lane')} --summary --compact"
                    )
                if item.get("next_ctest_filter"):
                    ctest_label = (
                        "baseline_ctest"
                        if item.get("next_test_role") == "baseline"
                        else "ctest"
                    )
                    print(f"   {ctest_label}={item.get('next_ctest_filter')}")
                for field, label in (
                    ("next_package_target", "package_target"),
                    ("next_package_ctest_filter", "package_ctest"),
                    ("next_process_probe_target", "process_probe_target"),
                    ("next_process_package_target", "process_package_target"),
                    ("next_process_package_test", "process_package_test"),
                    ("next_process_package_ctest_filter", "process_package_ctest"),
                    ("next_process_package_timestamped_test", "process_package_timestamped_test"),
                    ("next_process_package_timestamped_ctest_filter", "process_package_timestamped_ctest"),
                    ("next_process_package_parameterized_test", "process_package_parameterized_test"),
                    ("next_process_package_parameterized_ctest_filter", "process_package_parameterized_ctest"),
                    ("next_process_package_connection_loss_test", "process_package_connection_loss_test"),
                    ("next_process_package_connection_loss_ctest_filter", "process_package_connection_loss_ctest"),
                    ("next_process_package_object_registration_test", "process_package_object_registration_test"),
                    ("next_process_package_object_registration_ctest_filter", "process_package_object_registration_ctest"),
                    ("next_process_package_named_registration_test", "process_package_named_registration_test"),
                    ("next_process_package_named_registration_ctest_filter", "process_package_named_registration_ctest"),
                    ("next_process_package_attribute_update_test", "process_package_attribute_update_test"),
                    ("next_process_package_attribute_update_ctest_filter", "process_package_attribute_update_ctest"),
                    ("next_process_package_directed_retraction_test", "process_package_directed_retraction_test"),
                    ("next_process_package_directed_retraction_ctest_filter", "process_package_directed_retraction_ctest"),
                    ("next_process_package_catalog_verifier", "process_package_catalog_verifier"),
                ):
                    if item.get(field):
                        print(f"   {label}={item.get(field)}")
                if item.get("next_test_query"):
                    test_label = (
                        "baseline_test"
                        if item.get("next_test_role") == "baseline"
                        else "test"
                    )
                    print(f"   {test_label}={compact_prose(item.get('next_test_query'), 180)}")
                    pointer = item.get("next_test_pointer", {})
                    if isinstance(pointer, dict):
                        print(
                            f"   test_pointer={pointer.get('state', 'unknown')} "
                            f"matches={pointer.get('match_count', 0)} "
                            f"statuses={','.join(pointer.get('statuses', {}).keys()) or '<none>'}"
                        )
                if item.get("next_source_test_query"):
                    print(
                        f"   next_source_state={item.get('next_source_state', 'unplanned-source')}"
                    )
                    print(f"   next_source_test={item.get('next_source_test_query')}")
                    if item.get("next_source_location"):
                        print(f"   next_source_location={item.get('next_source_location')}")
                    if item.get("next_source_lane"):
                        print(f"   next_source_lane={item.get('next_source_lane')}")
                    source_requirement_ids = strings(item.get("next_source_requirement_ids"))
                    source_standard_sections = strings(item.get("next_source_standard_sections"))
                    source_api_surfaces = strings(item.get("next_source_api_surfaces"))
                    print(
                        "   next_source_mapping: "
                        f"requirements={len(source_requirement_ids)} "
                        f"standard_sections={len(source_standard_sections)} "
                        f"api_surfaces={len(source_api_surfaces)}"
                    )
                    if source_requirement_ids:
                        print(
                            "   next_source_requirement_ids="
                            f"{','.join(source_requirement_ids)}"
                        )
                    if source_standard_sections:
                        print(
                            "   next_source_standard_sections="
                            f"{','.join(source_standard_sections)}"
                        )
                    if source_api_surfaces:
                        print(
                            "   next_source_api_surfaces="
                            f"{','.join(source_api_surfaces)}"
                        )
                    if item.get("next_source_ctest_filter"):
                        print(f"   next_source_ctest={item.get('next_source_ctest_filter')}")
                sections = strings(item.get("next_standard_sections"))
                if sections:
                    print(f"   standard_sections={','.join(sections)}")
                print(
                    f"   mapped_handles: standards={len(strings(item.get('next_standard_sections')))} "
                    f"plan_ids={len(strings(item.get('next_plan_ids')))} "
                    f"catalog_gaps={len(strings(item.get('catalog_gap_plan_ids')))}"
                )
                continue
            tags = strings(item.get("query_tags"))
            if arguments.compact:
                print(f"   tag_count={len(tags)}")
            else:
                print(f"   tags={', '.join(tags) or '<none>'}")
            print(f"   anchor={item.get('roadmap_anchor')}")
            if item.get("current_focus"):
                focus = compact_prose(item.get("current_focus")) if arguments.compact else item.get("current_focus")
                print(f"   focus={focus}")
            if item.get("next_task"):
                task = compact_prose(item.get("next_task")) if arguments.compact else item.get("next_task")
                print(f"   task={task}")
            handles = item_handles(item, arguments.compact)
            if handles:
                print(f"   handles={' '.join(handles)}")
            next_action = compact_prose(item.get("next_action")) if arguments.compact else item.get("next_action")
            print(f"   next={next_action}")
        return 0

    if arguments.command == "focus":
        active_work = None
        lane = arguments.lane
        if not lane:
            active_work = indexed_work_slice(index, tests, None)
            if active_work.get("found"):
                lane = active_work.get("lane")
        if not isinstance(lane, str) or not lane.strip():
            result = {
                "found": False,
                "reason": (
                    "no lane was supplied and the active indexed work item "
                    "has no exact lane handle"
                ),
            }
            if arguments.json:
                print(json.dumps(result, indent=2, sort_keys=True))
            else:
                print(result["reason"], file=sys.stderr)
            return 1

        result = focused_lane_result(index, tests, lane.strip(), arguments.limit)
        result["found"] = result.get("lane_state") != "missing"
        if active_work is not None and active_work.get("found"):
            result["active_work_id"] = active_work.get("work_id")
            result["active_work_status"] = active_work.get("work_status")
            result["active_work_query"] = active_work.get("work_query")
            result["active_work_next_action"] = active_work.get("next_action")
        if arguments.json:
            print(json.dumps(result, indent=2, sort_keys=True))
            return 0 if result["found"] else 1
        if not result["found"]:
            print(f"No indexed Catch2 lane matched: {lane}", file=sys.stderr)
            return 1

        print("Focused Catch2 lane:")
        print(f"lane: {result['lane']} ({result['lane_state']})")
        if result.get("roadmap_owner"):
            print(f"roadmap_owner: {result['roadmap_owner']}")
        print(
            "cases: "
            f"{result['test_count']} "
            f"({result['implemented_test_count']} implemented, "
            f"{result['candidate_count']} executable candidates, "
            f"{result['source_drift_count']} source drift)"
        )
        print(
            "mapping: "
            f"{result['mapped_test_count']}/{result['test_count']} mapped; "
            f"requirements={result['requirement_count']}; "
            f"standard_sections={result['standard_section_count']}"
        )
        print(
            "assertions: "
            f"{result['assertion_count']} total; "
            f"{result['executable_assertion_count']} executable candidates"
            + (
                f" (indexed lane total; plan rows record {result['recorded_assertion_count']})"
                if result.get("assertion_count_source") == "indexed-lane-total"
                else ""
            )
        )
        if result.get("lane_handles"):
            print("handles:")
            for name, value in result["lane_handles"].items():
                print(f"  {name}={value}")
        if result["candidates"]:
            print(
                f"candidates (showing {result['shown_candidate_count']} "
                f"of {result['candidate_count']}):"
            )
            for candidate in result["candidates"]:
                print(text_test_summary(candidate))
        else:
            print("candidates: none; no source-located unimplemented cases remain.")
            if result["source_drift_count"]:
                print("note: source-drift rows remain historical and are not executable candidates.")
        return 0

    if arguments.command == "work":
        result = indexed_work_slice(index, tests, arguments.id)
        if arguments.json:
            print(json.dumps(result, indent=2, sort_keys=True))
            return 0 if result.get("found") else 1
        if not result.get("found"):
            print(
                f"No indexed work item matched: {arguments.id or '<next>'}",
                file=sys.stderr,
            )
            return 1
        if arguments.summary:
            parent = result["parent_item"]
            work_item = result["work_item"]
            print("Active indexed work slice (summary):")
            print(
                f"parent: {parent.get('id')}: {parent.get('title')} "
                f"(priority {parent.get('priority')})"
            )
            print(
                f"work: {work_item.get('id')}: {work_item.get('title')} "
                f"(status {result.get('work_status')})"
            )
            if result.get("task"):
                print(f"task: {compact_prose(result['task'], 240)}")
            if result.get("lane"):
                lane_count = result.get("lane_case_count")
                suffix = f" ({lane_count} cases)" if lane_count is not None else ""
                print(f"lane: {result['lane']}{suffix}")
                if result.get("lane_state"):
                    print(
                        "lane_state: "
                        f"{result['lane_state']}; "
                        f"mapped={result.get('lane_mapped_test_count', 0)}/"
                        f"{lane_count if lane_count is not None else '?'}; "
                        f"executable_candidates={result.get('lane_candidate_count', 0)}; "
                        f"source_drift={result.get('lane_source_drift_count', 0)}; "
                        f"assertions={result.get('lane_assertion_count', 0)}"
                    )
            if result.get("ctest_filter"):
                print(f"ctest: {result['ctest_filter']}")
            baseline = result.get("baseline")
            if isinstance(baseline, dict):
                baseline_state = baseline.get("state") or baseline.get("status", "<unspecified>")
                print(
                    f"baseline: {baseline.get('test_case', '<unnamed test>')} "
                    f"({baseline_state}; source={source_location_text(baseline)}; "
                    f"requirements={baseline.get('requirement_count', 0)}; "
                    f"sections={len(strings(baseline.get('standard_sections')))}"
                    ")"
                )
            if result.get("next_source_test_query"):
                print(
                    "next_slice: "
                    f"{result['next_source_test_query']} "
                    f"(state={result.get('next_source_state') or 'unplanned-source'}; "
                    f"source={result.get('next_source_location') or '<unlocated>'})"
                )
                if result.get("next_source_lane"):
                    print(f"next_slice_lane: {result['next_source_lane']}")
                print(
                    "next_slice_mapping: "
                    f"requirements={len(strings(result.get('next_source_requirement_ids')))}; "
                    f"standard_sections={len(strings(result.get('next_source_standard_sections')))}; "
                    f"api_surfaces={len(strings(result.get('next_source_api_surfaces')))}"
                )
                source_sections = strings(result.get("next_source_standard_sections"))
                if source_sections:
                    print(f"next_slice_standard_sections: {', '.join(source_sections)}")
                source_requirement_ids = strings(result.get("next_source_requirement_ids"))
                if source_requirement_ids:
                    print(f"next_slice_requirements: {', '.join(source_requirement_ids)}")
                source_api_surfaces = strings(result.get("next_source_api_surfaces"))
                if source_api_surfaces:
                    print(f"next_slice_api_surfaces: {', '.join(source_api_surfaces)}")
                if result.get("next_source_ctest_filter"):
                    print(f"next_slice_ctest: {result['next_source_ctest_filter']}")
            sections = strings(result.get("standard_sections"))
            plan_ids = strings(result.get("plan_ids"))
            print(
                f"mapped work handles: standard_sections={len(sections)}; "
                f"plan_ids={len(plan_ids)}; "
                f"catalog_gaps={len(strings(result.get('catalog_gap_plan_ids')))}"
            )
            if result.get("next_action"):
                print(f"next_action: {compact_prose(result['next_action'], 240)}")
            print("commands:")
            for command in result.get("commands", [])[:5]:
                print(f"  {command}")
            if len(result.get("commands", [])) > 5:
                print("  (additional package/JUnit commands available with work --compact or --json)")
            return 0
        parent = result["parent_item"]
        work_item = result["work_item"]
        print("Active indexed work slice:")
        print(
            f"parent: {parent.get('id')}: {parent.get('title')} "
            f"(priority {parent.get('priority')})"
        )
        print(
            f"work: {work_item.get('id')}: {work_item.get('title')} "
            f"(status {result.get('work_status')})"
        )
        if result.get("task"):
            print(f"task: {compact_prose(result['task'], 240)}")
        if result.get("work_query"):
            print(f"work_query: {compact_prose(result['work_query'], 240)}")
        if result.get("lane"):
            lane_count = result.get("lane_case_count")
            suffix = f" ({lane_count} cases)" if lane_count is not None else ""
            print(f"lane: {result['lane']}{suffix}")
        if result.get("ctest_filter"):
            print(f"ctest: {result['ctest_filter']}")
        if result.get("package_target"):
            print(f"package_target: {result['package_target']}")
        if result.get("package_ctest_filter"):
            print(f"package_ctest: {result['package_ctest_filter']}")
        if result.get("package_manifest"):
            print(f"package_manifest: {result['package_manifest']}")
        if result.get("junit_target"):
            print(f"junit_target: {result['junit_target']}")
        if result.get("junit_artifact"):
            print(f"junit_artifact: {result['junit_artifact']}")
        if result.get("process_probe_target"):
            print(f"process_probe_target: {result['process_probe_target']}")
        if result.get("process_package_target"):
            print(f"process_package_target: {result['process_package_target']}")
        if result.get("process_package_test"):
            print(f"process_package_test: {result['process_package_test']}")
        if result.get("process_package_ctest_filter"):
            print(
                "process_package_ctest: "
                f"{result['process_package_ctest_filter']}"
            )
        if result.get("process_package_timestamped_test"):
            print(
                "process_package_timestamped_test: "
                f"{result['process_package_timestamped_test']}"
            )
        if result.get("process_package_timestamped_ctest_filter"):
            print(
                "process_package_timestamped_ctest: "
                f"{result['process_package_timestamped_ctest_filter']}"
            )
        if result.get("process_package_parameterized_test"):
            print(
                "process_package_parameterized_test: "
                f"{result['process_package_parameterized_test']}"
            )
        if result.get("process_package_parameterized_ctest_filter"):
            print(
                "process_package_parameterized_ctest: "
                f"{result['process_package_parameterized_ctest_filter']}"
            )
        if result.get("process_package_connection_loss_test"):
            print(
                "process_package_connection_loss_test: "
                f"{result['process_package_connection_loss_test']}"
            )
        if result.get("process_package_connection_loss_ctest_filter"):
            print(
                "process_package_connection_loss_ctest: "
                f"{result['process_package_connection_loss_ctest_filter']}"
            )
        if result.get("process_package_object_registration_test"):
            print(
                "process_package_object_registration_test: "
                f"{result['process_package_object_registration_test']}"
            )
        if result.get("process_package_object_registration_ctest_filter"):
            print(
                "process_package_object_registration_ctest: "
                f"{result['process_package_object_registration_ctest_filter']}"
            )
        if result.get("process_package_named_registration_test"):
            print(
                "process_package_named_registration_test: "
                f"{result['process_package_named_registration_test']}"
            )
        if result.get("process_package_named_registration_ctest_filter"):
            print(
                "process_package_named_registration_ctest: "
                f"{result['process_package_named_registration_ctest_filter']}"
            )
        if result.get("process_package_attribute_update_test"):
            print(
                "process_package_attribute_update_test: "
                f"{result['process_package_attribute_update_test']}"
            )
        if result.get("process_package_attribute_update_ctest_filter"):
            print(
                "process_package_attribute_update_ctest: "
                f"{result['process_package_attribute_update_ctest_filter']}"
            )
        if result.get("process_package_directed_retraction_test"):
            print(
                "process_package_directed_retraction_test: "
                f"{result['process_package_directed_retraction_test']}"
            )
        if result.get("process_package_directed_retraction_ctest_filter"):
            print(
                "process_package_directed_retraction_ctest: "
                f"{result['process_package_directed_retraction_ctest_filter']}"
            )
        if result.get("process_package_catalog_verifier"):
            print(
                "process_package_catalog_verifier: "
                f"{result['process_package_catalog_verifier']}"
            )
        baseline = result.get("baseline")
        if isinstance(baseline, dict):
            print(f"baseline: {baseline.get('test_case', '<unnamed test>')}")
            if baseline.get("state") == "missing":
                print("  baseline_status: missing from the Catch2 plan")
            else:
                print(
                    f"  baseline_status: {baseline.get('status', '<unspecified>')}; "
                    f"source: {source_location_text(baseline)}"
                )
                print(
                    f"  baseline_mapping: requirements={baseline.get('requirement_count', 0)}; "
                    f"standard_sections={len(strings(baseline.get('standard_sections')))}"
                )
                requirement_ids = strings(baseline.get("requirement_ids"))
                if requirement_ids:
                    print(f"  baseline_requirements: {', '.join(requirement_ids)}")
                remaining = baseline.get("requirement_ids_remaining")
                if remaining:
                    print(f"  baseline_requirements_remaining: {remaining}")
                sections = strings(baseline.get("standard_sections"))
                if sections:
                    print(f"  baseline_standard_sections: {', '.join(sections)}")
        if result.get("next_source_test_query"):
            print(
                "next_slice: "
                f"{result['next_source_test_query']} "
                f"(state={result.get('next_source_state') or 'unplanned-source'}; "
                f"source={result.get('next_source_location') or '<unlocated>'})"
            )
            if result.get("next_source_lane"):
                print(f"next_slice_lane: {result['next_source_lane']}")
            source_requirement_ids = strings(result.get("next_source_requirement_ids"))
            source_sections = strings(result.get("next_source_standard_sections"))
            source_api_surfaces = strings(result.get("next_source_api_surfaces"))
            print(
                "next_slice_mapping: "
                f"requirements={len(source_requirement_ids)}; "
                f"standard_sections={len(source_sections)}; "
                f"api_surfaces={len(source_api_surfaces)}"
            )
            if source_requirement_ids:
                print(f"next_slice_requirements: {', '.join(source_requirement_ids)}")
            if source_sections:
                print(f"next_slice_standard_sections: {', '.join(source_sections)}")
            if source_api_surfaces:
                print(f"next_slice_api_surfaces: {', '.join(source_api_surfaces)}")
            if result.get("next_source_ctest_filter"):
                print(f"next_slice_ctest: {result['next_source_ctest_filter']}")
        sections = strings(result.get("standard_sections"))
        if sections:
            print(f"work_standard_sections: {', '.join(sections)}")
        plan_ids = strings(result.get("plan_ids"))
        if plan_ids:
            print(f"work_plan_ids: {', '.join(plan_ids)}")
        catalog_gaps = strings(result.get("catalog_gap_plan_ids"))
        if catalog_gaps:
            print(f"catalog_gaps: {', '.join(catalog_gaps)}")
        if result.get("next_action"):
            print(f"next_action: {compact_prose(result['next_action'], 240)}")
        if result.get("commands"):
            print("commands:")
            for command in result["commands"]:
                print(f"  {command}")
        return 0

    if arguments.command == "recent":
        all_slices = recent_completed_views(index, tests, arguments.lane)
        shown_slices = (
            all_slices
            if not arguments.limit
            else all_slices[: arguments.limit]
        )
        result = {
            "count": len(all_slices),
            "shown_count": len(shown_slices),
            "slices": shown_slices,
        }
        if arguments.json:
            print(json.dumps(result, indent=2, sort_keys=True))
            return 0
        if not all_slices:
            print("No recent completed slices are indexed.", file=sys.stderr)
            return 1
        lane_suffix = f" for lane {arguments.lane!r}" if arguments.lane else ""
        if len(shown_slices) == len(all_slices):
            print(f"{len(all_slices)} recent completed slices{lane_suffix}.")
        else:
            print(
                f"{len(all_slices)} recent completed slices; showing "
                f"{len(shown_slices)}{lane_suffix}. Use --limit 0 for all."
            )
        for slice_value in shown_slices:
            print(
                f"- {slice_value.get('plan_id', '<unnamed>')}: "
                f"{slice_value.get('test_query', '<unnamed test>')}"
            )
            print(f"  source: {source_location_text({'source_locations': slice_value.get('source_locations', [])})}")
            print(
                f"  assertions: {slice_value.get('assertions', '<unspecified>')}; "
                f"status: {slice_value.get('status', '<unspecified>')}; "
                f"matched_tests={slice_value.get('matched_test_count', 0)}"
            )
            callback_models = strings(slice_value.get("callback_models"))
            if callback_models:
                print(f"  callback_models: {', '.join(callback_models)}")
            lanes = strings(slice_value.get("lane_queries"))
            if lanes:
                print(f"  lanes: {', '.join(lanes)}")
            sections = strings(
                slice_value.get("resolved_standard_sections")
                or slice_value.get("standard_sections")
            )
            if arguments.compact:
                print(
                    f"  mapped: requirements={slice_value.get('resolved_requirement_count', 0)}; "
                    f"standard_sections={len(sections)}"
                )
            elif sections:
                print(f"  standard_sections: {', '.join(sections)}")
        return 0

    if arguments.command == "item":
        item_value, matching_tests = indexed_item(index, tests, arguments.id)
        if item_value is None:
            if arguments.json:
                print(json.dumps({"id": arguments.id, "found": False}, indent=2))
            else:
                print(f"No indexed roadmap item matched: {arguments.id}", file=sys.stderr)
            return 1
        shown_tests = matching_tests if not arguments.limit else matching_tests[: arguments.limit]
        result = {
            "item": item_value,
            "test_count": len(matching_tests),
            "shown_test_count": len(shown_tests),
            "tests": shown_tests,
        }
        if arguments.json:
            if arguments.summary:
                result["tests"] = [test_summary_data(test) for test in shown_tests]
            print(json.dumps(result, indent=2, sort_keys=True))
            return 0
        print(f"{item_value.get('id')}: {item_value.get('title')}")
        print(f"status: {item_value.get('status')}; priority: {item_value.get('priority')}")
        print(f"anchor: {item_value.get('roadmap_anchor')}")
        query_tags = strings(item_value.get("query_tags"))
        if arguments.compact:
            print(f"query tag count: {len(query_tags)}")
        else:
            print(f"query tags: {', '.join(query_tags) or '<none>'}")
        if item_value.get("current_focus"):
            focus = compact_prose(item_value.get("current_focus")) if arguments.compact else item_value.get("current_focus")
            print(f"current focus: {focus}")
        if item_value.get("next_task"):
            task = compact_prose(item_value.get("next_task")) if arguments.compact else item_value.get("next_task")
            print(f"next task: {task}")
        handles = item_handles(item_value, arguments.compact)
        if handles:
            print(f"handles: {' '.join(handles)}")
        next_action = compact_prose(item_value.get("next_action")) if arguments.compact else item_value.get("next_action")
        print(f"next: {next_action}")
        if len(shown_tests) == len(matching_tests):
            print(f"{len(matching_tests)} tagged Catch2 cases.")
        else:
            print(f"{len(matching_tests)} tagged Catch2 cases; showing {len(shown_tests)}.")
        if shown_tests:
            renderer = (
                text_test_summary
                if arguments.summary
                else lambda test: text_test(test, arguments.verbose, arguments.compact)
            )
            print(
                "\n".join(
                    renderer(test)
                    for test in shown_tests
                )
            )
        return 0

    if arguments.command == "plan":
        try:
            sections = implementation_plan_sections(arguments.implementation_plan)
        except OSError as error:
            print(f"query_rti_work: {error}", file=sys.stderr)
            return 2
        result = {
            "path": relative_path(arguments.implementation_plan),
            "sections": sections,
        }
        if arguments.json:
            print(json.dumps(result, indent=2, sort_keys=True))
            return 0
        print(f"Implementation plan: {result['path']}")
        for section in sections:
            indent = "  " * max(section["level"] - 1, 0)
            print(f"{indent}- line {section['line']}: {section['title']}")
        return 0

    if arguments.command == "coverage":
        scope_lane = arguments.lane
        scoped_tests = (
            tests
            if scope_lane is None
            else [
                test
                for test in tests
                if scope_lane in strings(test.get("tags"))
            ]
        )
        references = [
            requirement
            for test in scoped_tests
            for requirement in test.get("requirements", [])
        ]
        mapped_tests = sum(bool(test.get("requirements")) for test in scoped_tests)
        unresolved = sorted(
            {
                identifier
                for test in scoped_tests
                for identifier in test.get("unresolved_requirement_ids", [])
            }
        )
        recorded_assertion_count = sum(
            value
            for test in scoped_tests
            for value in [test.get("assertions")]
            if isinstance(value, int)
        )
        mapping = index.get("mapping", {})
        if not isinstance(mapping, dict):
            mapping = {}
        lane_assertion_counts = mapping.get("lane_assertion_counts", {})
        if not isinstance(lane_assertion_counts, dict):
            lane_assertion_counts = {}
        configured_assertion_count = lane_assertion_counts.get(scope_lane)
        assertion_count = (
            configured_assertion_count
            if isinstance(configured_assertion_count, int)
            else recorded_assertion_count
        )
        result = {
            "scope_lane": scope_lane,
            "test_count": len(scoped_tests),
            "assertion_count": assertion_count,
            "recorded_assertion_count": recorded_assertion_count,
            "assertion_count_source": (
                "indexed-lane-total"
                if isinstance(configured_assertion_count, int)
                else "plan-entry-records"
            ),
            "tests_with_source_location": sum(
                bool(test.get("source_locations")) for test in scoped_tests
            ),
            "tests_without_source_location": sum(
                not test.get("source_locations") for test in scoped_tests
            ),
            "tests_with_requirement_mapping": mapped_tests,
            "tests_without_requirement_mapping": len(scoped_tests) - mapped_tests,
            "requirement_reference_count": len(references),
            "unique_requirement_count": len(
                {requirement["lab_requirement_id"] for requirement in references}
            ),
            "resolved_requirement_reference_count": sum(
                requirement.get("standard") is not None for requirement in references
            ),
            "unique_standard_clause_count": len(
                {
                    clause
                    for test in scoped_tests
                    for clause in test.get("standard_clauses", [])
                }
            ),
            "unresolved_requirement_ids": unresolved,
        }
        if arguments.json:
            print(json.dumps(result, indent=2, sort_keys=True))
        else:
            scope_text = f" in lane {scope_lane!r}" if scope_lane else ""
            print(f"C++ Catch2 cases{scope_text}: {result['test_count']}")
            assertion_note = (
                f" (indexed lane total; plan rows record {result['recorded_assertion_count']})"
                if result.get("assertion_count_source") == "indexed-lane-total"
                else ""
            )
            print(f"Assertions: {result['assertion_count']}{assertion_note}")
            print(
                f"Mapped to Lab requirements: {result['tests_with_requirement_mapping']} "
                f"({result['tests_without_requirement_mapping']} without a requirement mapping)"
            )
            print(
                f"C++ source locations: {result['tests_with_source_location']} "
                f"({result['tests_without_source_location']} unlocated)"
            )
            print(
                f"Requirement references: {result['requirement_reference_count']} "
                f"across {result['unique_requirement_count']} unique IDs"
            )
            print(
                f"Resolved 2025 references: {result['resolved_requirement_reference_count']}; "
                f"standard clauses/subsections: {result['unique_standard_clause_count']}"
            )
            if unresolved:
                print("Unresolved requirement IDs:")
                print("\n".join(f"- {identifier}" for identifier in unresolved[:20]))
        return 0

    if arguments.command == "lanes":
        tag_counts = Counter(tag for test in tests for tag in test.get("tags", []))
        lane_values = [
            {"tag": tag, "test_count": count}
            for tag, count in sorted(tag_counts.items(), key=lambda value: (-value[1], value[0]))
        ]
        total_count = len(lane_values)
        requested_limit = arguments.limit
        if requested_limit is None:
            # ``lanes --compact``/``--summary`` are intended for work selection;
            # keep those views bounded so a tag inventory cannot fill a context
            # window.  The unqualified command retains its complete inventory.
            requested_limit = 40 if arguments.compact or arguments.summary else 0
        shown_lanes = lane_values if requested_limit == 0 else lane_values[:requested_limit]
        result = {
            "count": total_count,
            "shown_count": len(shown_lanes),
            "lanes": shown_lanes,
        }
        if arguments.json:
            print(json.dumps(result, indent=2, sort_keys=True))
            return 0
        if len(shown_lanes) == total_count:
            print(f"{total_count} exact Catch2 lane tags.")
        else:
            print(
                f"{total_count} exact Catch2 lane tags; showing {len(shown_lanes)}. "
                "Use --limit 0 for all."
            )
        for lane_value in shown_lanes:
            print(f"- {lane_value['tag']}: {lane_value['test_count']} cases")
        return 0
    if arguments.command == "unmapped":
        scoped_tests = (
            tests
            if arguments.lane is None
            else [test for test in tests if arguments.lane in strings(test.get("tags"))]
        )
        selected = [test for test in scoped_tests if not test.get("requirements")]
        matched_count = len(selected)
        if arguments.limit:
            selected = selected[: arguments.limit]
        result = {
            "scope_lane": arguments.lane,
            "count": matched_count,
            "shown_count": len(selected),
            "tests": selected,
        }
    elif arguments.command == "unlocated":
        scoped_tests = (
            tests
            if arguments.lane is None
            else [test for test in tests if arguments.lane in strings(test.get("tags"))]
        )
        selected = [test for test in scoped_tests if not test.get("source_locations")]
        matched_count = len(selected)
        if arguments.limit:
            selected = selected[: arguments.limit]
        result = {
            "scope_lane": arguments.lane,
            "count": matched_count,
            "shown_count": len(selected),
            "tests": selected,
        }
    elif arguments.command == "unplanned":
        selected = unplanned_source_cases(
            tests,
            source_locations,
            arguments.path,
        )
        matched_count = len(selected)
        if arguments.limit:
            selected = selected[: arguments.limit]
        result = {
            "source_path_query": arguments.path,
            "count": matched_count,
            "shown_count": len(selected),
            "tests": selected,
        }
    elif arguments.command == "lane":
        selected = select_tests(tests, arguments.tag, "lane")
        matched_count = len(selected)
        if arguments.limit:
            selected = selected[: arguments.limit]
        mapping = index.get("mapping", {})
        lane_owners = (
            mapping.get("lane_owners", {}) if isinstance(mapping, dict) else {}
        )
        roadmap_owner = (
            lane_owners.get(arguments.tag)
            if isinstance(lane_owners, dict)
            else None
        )
        lane_handles = {}
        configured_lane_handles = (
            mapping.get("lane_handles", {}) if isinstance(mapping, dict) else {}
        )
        if isinstance(configured_lane_handles, dict):
            configured = configured_lane_handles.get(arguments.tag)
            if isinstance(configured, dict):
                lane_handles = dict(configured)
        result: dict[str, Any] = {
            "tag": arguments.tag,
            "roadmap_owner": roadmap_owner,
            "lane_handles": lane_handles,
            "count": matched_count,
            "shown_count": len(selected),
            "tests": selected,
        }
    elif arguments.command == "test":
        selected = select_tests(tests, arguments.query, "test")
        matched_count = len(selected)
        if arguments.limit:
            selected = selected[: arguments.limit]
        result = {
            "query": arguments.query,
            "count": matched_count,
            "shown_count": len(selected),
            "tests": selected,
        }
    elif arguments.command == "requirement":
        selected = select_tests(tests, arguments.query, "requirement")
        matched_count = len(selected)
        if arguments.limit:
            selected = selected[: arguments.limit]
        result = {
            "query": arguments.query,
            "count": matched_count,
            "shown_count": len(selected),
            "tests": selected,
        }
    elif arguments.command == "section":
        selected = select_tests(tests, arguments.query, "section")
        matched_count = len(selected)
        if arguments.limit:
            selected = selected[: arguments.limit]
        result = {
            "query": arguments.query,
            "count": matched_count,
            "shown_count": len(selected),
            "tests": selected,
        }
    elif arguments.command == "trace":
        match_kind, matches = resolve_trace_query(tests, arguments.query)
        if match_kind is None:
            result = {
                "query": arguments.query,
                "found": False,
                "match_kind": None,
                "count": 0,
                "shown_count": 0,
                "tests": [],
            }
        else:
            selected = matches if not arguments.limit else matches[: arguments.limit]
            result = {
                "query": arguments.query,
                "found": True,
                "match_kind": match_kind,
                "count": len(matches),
                "shown_count": len(selected),
                "tests": [
                    trace_record(test, match_kind, arguments.query, index)
                    for test in selected
                ],
            }
    elif arguments.command == "search":
        selected = select_tests(tests, arguments.query, "search")
        matched_count = len(selected)
        if arguments.limit:
            selected = selected[: arguments.limit]
        result = {
            "query": arguments.query,
            "count": matched_count,
            "shown_count": len(selected),
            "tests": selected,
        }
    else:
        print(f"query_rti_work: unsupported command {arguments.command}", file=sys.stderr)
        return 2

    if arguments.command == "trace":
        if not result.get("found"):
            if arguments.json:
                print(json.dumps(result, indent=2, sort_keys=True))
            else:
                print(
                    "No exact trace handle matched. Use test, requirement, "
                    "section, lane, or search for a broader query.",
                    file=sys.stderr,
                )
            return 1
        if arguments.json:
            if arguments.summary:
                result["tests"] = [
                    trace_summary_record(record) for record in result["tests"]
                ]
            print(json.dumps(result, indent=2, sort_keys=True))
            return 0
        shown_count = result.get("shown_count", result["count"])
        kind = result.get("match_kind") or "handle"
        if shown_count == result["count"]:
            print(f"Trace ({kind}): {result['count']} matched.")
        else:
            print(
                f"Trace ({kind}): {result['count']} matched; "
                f"showing {shown_count}."
            )
        print(
            "\n".join(
                text_trace(record, summary=arguments.summary)
                for record in result["tests"]
            )
        )
        return 0

    if arguments.command == "unplanned":
        if arguments.json:
            print(json.dumps(result, indent=2, sort_keys=True))
            return 0 if result["tests"] else 1
        path_suffix = (
            f" under source paths containing {arguments.path!r}"
            if arguments.path
            else ""
        )
        if not result["tests"]:
            print(f"No unplanned C++ TEST_CASE declarations{path_suffix}.", file=sys.stderr)
            return 1
        shown_count = result["shown_count"]
        if shown_count == result["count"]:
            print(f"{result['count']} unplanned C++ TEST_CASE declarations{path_suffix}.")
        else:
            print(
                f"{result['count']} unplanned C++ TEST_CASE declarations{path_suffix}; "
                f"showing {shown_count}."
            )
        for record in result["tests"]:
            print(
                f"- {record.get('test_case', '<unnamed test>')}: "
                f"{source_location_text(record)}"
            )
        return 0

    if arguments.json:
        if arguments.summary:
            result["tests"] = [test_summary_data(test) for test in result["tests"]]
        print(json.dumps(result, indent=2, sort_keys=True))
        return 0
    if not result["tests"]:
        if arguments.command == "unlocated":
            scope_text = f" in lane {arguments.lane!r}" if getattr(arguments, "lane", None) else ""
            print(f"No unlocated Catch2 plan cases{scope_text}.", file=sys.stderr)
        elif arguments.command == "unmapped":
            scope_text = f" in lane {arguments.lane!r}" if getattr(arguments, "lane", None) else ""
            print(f"No unmapped Catch2 cases{scope_text}.", file=sys.stderr)
        else:
            print(f"No Catch2 cases matched {arguments.command} query.", file=sys.stderr)
        return 1
    shown_count = result.get("shown_count", result["count"])
    if arguments.command == "lane" and result.get("roadmap_owner"):
        print(f"Roadmap owner: {result['roadmap_owner']}")
    if arguments.command == "lane" and result.get("lane_handles"):
        for name, value in result["lane_handles"].items():
            print(f"{name}: {value}")
    if arguments.command == "unlocated":
        scope_text = f" in lane {arguments.lane!r}" if getattr(arguments, "lane", None) else ""
        if shown_count == result["count"]:
            print(f"{result['count']} Catch2 plan cases{scope_text} have no matching TEST_CASE source declaration.")
        else:
            print(
                f"{result['count']} Catch2 plan cases{scope_text} have no matching TEST_CASE source declaration; "
                f"showing {shown_count}."
            )
    elif arguments.command == "unmapped":
        scope_text = f" in lane {arguments.lane!r}" if getattr(arguments, "lane", None) else ""
        if shown_count == result["count"]:
            print(f"{result['count']} unmapped Catch2 cases{scope_text}.")
        else:
            print(f"{result['count']} unmapped Catch2 cases{scope_text}; showing {shown_count}.")
    elif shown_count == result["count"]:
        print(f"{result['count']} Catch2 cases matched.")
    else:
        print(f"{result['count']} Catch2 cases matched; showing {shown_count}.")
    renderer = (
        text_test_summary
        if arguments.summary
        else lambda test: text_test(test, arguments.verbose, arguments.compact)
    )
    print("\n".join(renderer(test) for test in result["tests"]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
