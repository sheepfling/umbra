from __future__ import annotations

import json
from collections import Counter
from pathlib import Path

import query_rti_work

TARGET_TEST = (
    "Embedded timestamped Delete Object Instance reconstitutes on retraction "
    "and removes before grant"
)


def assert_no_duplicate_json_keys(path: Path) -> None:
    """Reject ambiguous plan/index objects before query assertions run."""

    duplicates: list[str] = []

    def object_pairs_hook(
        pairs: list[tuple[str, query_rti_work.JsonValue]],
    ) -> query_rti_work.JsonObject:
        seen: set[str] = set()
        result: query_rti_work.JsonObject = {}
        for key, value in pairs:
            if key in seen:
                duplicates.append(key)
            seen.add(key)
            result[key] = value
        return result

    json.loads(
        path.read_text(encoding="utf-8"),
        object_pairs_hook=object_pairs_hook,
    )
    if duplicates:
        names = ", ".join(sorted(set(duplicates)))
        raise AssertionError(f"{path} contains duplicate JSON keys: {names}")


def expected_family_gap_head(
    index: query_rti_work.JsonObject,
    tests: list[query_rti_work.JsonObject],
    standard_requirements: dict[str, query_rti_work.JsonObject],
    family: str,
) -> query_rti_work.JsonObject:
    family_item = next(
        (
            item
            for item in query_rti_work.json_object_rows(index.get("items"))
            if item.get("id") == family
        ),
        None,
    )
    family_tests = (
        query_rti_work.item_tests(family_item, tests, index=index)
        if isinstance(family_item, dict)
        else []
    )
    family_pairs = {
        (row.get("lab_requirement_id"), row.get("standard_section"))
        for test in family_tests
        for row in query_rti_work.requirement_section_mapping_rows(test)
        if isinstance(row, dict)
        and row.get("lab_requirement_id")
        and row.get("standard_section")
    }
    family_section_counts = Counter(
        str(section) for _, section in family_pairs if section
    )
    family_sections = set(family_section_counts)
    family_gaps = query_rti_work.requirement_gap_inventory(
        standard_requirements,
        tests,
        index=index,
        family=family,
        limit=0,
    )
    globally_covered = {
        requirement_id
        for test in tests
        for requirement_id in query_rti_work.strings(test.get("lab_requirement_ids"))
    }
    candidates = [
        record
        for record in family_gaps.get("requirements", [])
        if isinstance(record, dict)
        and record.get("id") not in globally_covered
        and record.get("standard_section") in family_sections
    ]
    out_of_scope = [
        record
        for record in candidates
        if str(record.get("document_id") or "").casefold() != "hla-1516.1-2025"
    ]
    implementation_candidates = [
        record
        for record in candidates
        if str(record.get("document_id") or "").casefold() == "hla-1516.1-2025"
    ]
    inventory_only = [
        record
        for record in implementation_candidates
        if str(record.get("api_surface_scope") or "").casefold() == "cross-cutting"
        and str(record.get("api_surface_note") or "").strip().casefold()
        == "table structure is not itself a callable api mapping."
    ]
    preview_candidates = [
        record for record in implementation_candidates if record not in inventory_only
    ]
    candidate_counts = Counter(
        record.get("standard_section")
        for record in preview_candidates
        if record.get("standard_section")
    )
    sections = set(candidate_counts)
    selected_section = min(
        sections,
        key=lambda section: (
            -candidate_counts.get(section, 0),
            -family_section_counts.get(section, 0),
            section,
        ),
        default=None,
    )
    selected_requirement = next(
        (
            record
            for record in preview_candidates
            if record.get("standard_section") == selected_section
        ),
        None,
    )
    return {
        "requirement": selected_requirement or {},
        "section": selected_section,
        "candidate_count": len(candidates),
        "out_of_scope_candidate_count": len(out_of_scope),
        "preview_candidate_count": len(preview_candidates),
        "inventory_only_count": len(inventory_only),
        "family_mapped_requirement_count": len(
            {requirement_id for requirement_id, _ in family_pairs}
        ),
        "family_section_count": len(family_sections),
    }
