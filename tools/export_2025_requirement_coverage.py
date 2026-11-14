#!/usr/bin/env python3
"""Export the pinned 2025 requirement-to-test mappings as a row-per-requirement CSV.

This is a traceability export. Catch2 plan status and TCK catalog promotion are
reported separately; neither column is a claim that a test currently passes or
that Umbra is conformant.
"""

from __future__ import annotations

import argparse
import csv
import re
from collections import Counter, defaultdict
from pathlib import Path
from typing import Any

import query_rti_work as rtiq

DISCLAIMER = (
    "Traceability only: mappings and catalog status are not current test-run results "
    "or proof of implementation compliance."
)


def section_sort_key(section: str) -> tuple[tuple[int, int | str], ...]:
    parts = re.split(r"([0-9]+)", section.removeprefix("clause-"))
    return tuple(
        (0, int(part)) if part.isdigit() else (1, part.casefold())
        for part in parts
        if part
    )


def object_rows(value: Any) -> list[dict[str, Any]]:
    if not isinstance(value, list):
        return []
    return [row for row in value if isinstance(row, dict)]


def joined(values: set[str] | list[str]) -> str:
    return " | ".join(sorted(value for value in values if value))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--output",
        type=Path,
        default=rtiq.REPOSITORY_ROOT
        / "out"
        / "reports"
        / "ieee-1516-2025-requirement-coverage.csv",
        help="CSV destination (default: out/reports/ieee-1516-2025-requirement-coverage.csv)",
    )
    args = parser.parse_args()

    bundle = rtiq.load_json(rtiq.DEFAULT_BUNDLE)
    requirements = rtiq.load_standard_requirements(
        bundle, rtiq.DEFAULT_CONTRACT_DIRECTORY
    )
    plan = rtiq.load_json(rtiq.DEFAULT_PLAN)
    catalog = rtiq.load_json(rtiq.DEFAULT_CPP_TCK_CATALOG)

    catch2: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for test in object_rows(plan.get("tests")):
        for requirement_id in rtiq.test_lab_requirement_ids(test):
            if requirement_id in requirements:
                catch2[requirement_id].append(test)

    tck: dict[str, list[dict[str, str]]] = defaultdict(list)
    for scenario in object_rows(catalog.get("scenarios")):
        scenario_id = scenario.get("id")
        if not isinstance(scenario_id, str) or not scenario_id.startswith("cpp-tck."):
            continue
        promotion = str(scenario.get("promotion") or "unspecified")
        for mapping in object_rows(scenario.get("requirement_mappings")):
            requirement_id = mapping.get("requirement_id") or mapping.get(
                "lab_requirement_id"
            )
            if isinstance(requirement_id, str) and requirement_id in requirements:
                tck[requirement_id].append(
                    {
                        "id": scenario_id,
                        "title": str(scenario.get("title") or ""),
                        "promotion": promotion,
                    }
                )

    dispositions = {
        str(row.get("requirement_id")): row
        for row in object_rows(catalog.get("requirement_dispositions"))
        if isinstance(row.get("requirement_id"), str)
    }

    columns = [
        "document_id",
        "canonical_section",
        "exported_clause_id",
        "requirement_ordinal",
        "requirement_id",
        "title",
        "statement",
        "kind",
        "coverage_kind",
        "catch2_mapping_status",
        "catch2_test_ids",
        "catch2_test_cases",
        "catch2_plan_statuses",
        "cpp_tck_catalog_mapping_status",
        "cpp_tck_scenario_ids",
        "cpp_tck_scenario_titles",
        "cpp_tck_catalog_promotions",
        "tck_requirement_disposition",
        "tck_disposition_owner",
        "direct_test_mapping_status",
        "evidence_boundary",
        "source_path",
        "source_start_line",
    ]

    args.output.parent.mkdir(parents=True, exist_ok=True)
    section_counts: Counter[str] = Counter()
    section_stats: dict[str, Counter[str]] = defaultdict(Counter)
    catch2_mapped = 0
    tck_mapped = 0
    direct_mapped = 0
    with args.output.open("w", encoding="utf-8-sig", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=columns)
        writer.writeheader()
        ordered = sorted(
            requirements.values(),
            key=lambda row: (
                str(row.get("document_id") or ""),
                section_sort_key(str(row.get("clause_id") or "")),
                int(row.get("ordinal") or 0),
                str(row.get("id") or ""),
            ),
        )
        for requirement in ordered:
            requirement_id = str(requirement["id"])
            section = f"{requirement.get('document_id')}:{requirement.get('clause_id')}"
            section_counts[section] += 1
            catch_rows = catch2.get(requirement_id, [])
            tck_rows = tck.get(requirement_id, [])
            disposition = dispositions.get(requirement_id, {})
            catch2_mapped += bool(catch_rows)
            tck_mapped += bool(tck_rows)
            direct_mapped += bool(catch_rows or tck_rows)
            section_stats[section]["requirements"] += 1
            section_stats[section]["catch2_mapped"] += bool(catch_rows)
            section_stats[section]["tck_mapped"] += bool(tck_rows)
            section_stats[section]["either_mapped"] += bool(catch_rows or tck_rows)
            section_stats[section]["disposition_only"] += bool(
                disposition and not catch_rows and not tck_rows
            )

            source = requirement.get("source")
            source = source if isinstance(source, dict) else {}
            promotions = {row["promotion"] for row in tck_rows}
            if not tck_rows:
                tck_status = "unmapped"
            elif promotions == {"promoted"}:
                tck_status = "mapped-promoted-scenarios"
            elif promotions == {"candidate"}:
                tck_status = "mapped-candidate-only"
            elif "promoted" in promotions and "candidate" in promotions:
                tck_status = "mapped-mixed-promotions"
            else:
                tck_status = "mapped-other-catalog-status"

            writer.writerow(
                {
                    "document_id": requirement.get("document_id", ""),
                    "canonical_section": section,
                    "exported_clause_id": requirement.get(
                        "exported_clause_id", requirement.get("clause_id", "")
                    ),
                    "requirement_ordinal": requirement.get("ordinal", ""),
                    "requirement_id": requirement_id,
                    "title": requirement.get("title", ""),
                    "statement": requirement.get(
                        "statement", requirement.get("machine_statement", "")
                    ),
                    "kind": requirement.get("kind", ""),
                    "coverage_kind": requirement.get("coverage_kind", ""),
                    "catch2_mapping_status": "mapped" if catch_rows else "unmapped",
                    "catch2_test_ids": joined(
                        {str(row.get("id") or "") for row in catch_rows}
                    ),
                    "catch2_test_cases": joined(
                        {str(row.get("test_case") or "") for row in catch_rows}
                    ),
                    "catch2_plan_statuses": joined(
                        {str(row.get("status") or "unspecified") for row in catch_rows}
                    ),
                    "cpp_tck_catalog_mapping_status": tck_status,
                    "cpp_tck_scenario_ids": joined(
                        {row["id"] for row in tck_rows}
                    ),
                    "cpp_tck_scenario_titles": joined(
                        {row["title"] for row in tck_rows}
                    ),
                    "cpp_tck_catalog_promotions": joined(promotions),
                    "tck_requirement_disposition": disposition.get("status", ""),
                    "tck_disposition_owner": disposition.get("owner_role", ""),
                    "direct_test_mapping_status": (
                        "mapped" if catch_rows or tck_rows else "unmapped"
                    ),
                    "evidence_boundary": DISCLAIMER,
                    "source_path": source.get("path", ""),
                    "source_start_line": source.get("line", ""),
                }
            )

    summary_path = args.output.with_name(f"{args.output.stem}-by-section.csv")
    with summary_path.open("w", encoding="utf-8-sig", newline="") as stream:
        fields = [
            "document_id",
            "canonical_section",
            "requirement_count",
            "catch2_mapped_count",
            "catch2_mapping_percent",
            "cpp_tck_mapped_count",
            "cpp_tck_mapping_percent",
            "mapped_by_either_count",
            "mapped_by_either_percent",
            "unmapped_count",
            "disposition_only_count",
        ]
        writer = csv.DictWriter(stream, fieldnames=fields)
        writer.writeheader()
        for section in sorted(
            section_counts,
            key=lambda value: (value.split(":", 1)[0], section_sort_key(value.split(":", 1)[-1])),
        ):
            stats = section_stats[section]
            count = stats["requirements"]
            writer.writerow(
                {
                    "document_id": section.split(":", 1)[0],
                    "canonical_section": section,
                    "requirement_count": count,
                    "catch2_mapped_count": stats["catch2_mapped"],
                    "catch2_mapping_percent": f"{100 * stats['catch2_mapped'] / count:.2f}",
                    "cpp_tck_mapped_count": stats["tck_mapped"],
                    "cpp_tck_mapping_percent": f"{100 * stats['tck_mapped'] / count:.2f}",
                    "mapped_by_either_count": stats["either_mapped"],
                    "mapped_by_either_percent": f"{100 * stats['either_mapped'] / count:.2f}",
                    "unmapped_count": count - stats["either_mapped"],
                    "disposition_only_count": stats["disposition_only"],
                }
            )

    print(f"CSV: {args.output}")
    print(f"Section summary CSV: {summary_path}")
    print(f"Requirements: {len(requirements)}")
    print(f"Sections: {len(section_counts)} (see section summary CSV)")
    print(f"Catch2-mapped requirements: {catch2_mapped}")
    print(f"Portable C++ TCK-mapped requirements: {tck_mapped}")
    print(f"Mapped by either test inventory: {direct_mapped}")
    print(DISCLAIMER)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
