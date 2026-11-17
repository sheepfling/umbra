#!/usr/bin/env python3
"""Export IEEE 1516.1/1516.2-2025 requirement-to-test mappings as CSV.

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

if __package__:
    from . import java_tck, query_rti_work as rtiq
else:
    import java_tck
    import query_rti_work as rtiq

DISCLAIMER = (
    "Traceability only: mappings and catalog status are not current test-run results "
    "or proof of implementation compliance."
)
TARGET_DOCUMENT_IDS = frozenset({"hla-1516.1-2025", "hla-1516.2-2025"})


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


def java_requirement_ids_for_scenario(
    scenario: dict[str, Any],
    known_lab_requirement_ids: set[str],
    linked_lab_requirement_ids: set[str],
) -> set[str]:
    """Mirror java_tck.export's stable-ID to pinned-Lab requirement join."""
    if scenario.get("mapping_classification") == (
        "api-contract-only-not-normative-conformance"
    ):
        return set()

    stable_ids = scenario.get("requirement_ids")
    if not isinstance(stable_ids, list):
        return set()

    requirement_ids: set[str] = set()
    for stable_id in stable_ids:
        if not isinstance(stable_id, str):
            continue
        if stable_id in known_lab_requirement_ids:
            requirement_ids.add(stable_id)
        else:
            requirement_ids.update(linked_lab_requirement_ids)
    return requirement_ids


def aggregate_section_stats_by_document(
    section_stats: dict[str, Counter[str]],
) -> dict[str, Counter[str]]:
    document_stats: dict[str, Counter[str]] = defaultdict(Counter)
    for section, stats in section_stats.items():
        document_id = section.split(":", 1)[0]
        for metric, count in stats.items():
            document_stats[document_id][metric] += count
    return document_stats


def target_standard_requirements(
    requirements: dict[str, dict[str, Any]],
) -> dict[str, dict[str, Any]]:
    return {
        requirement_id: requirement
        for requirement_id, requirement in requirements.items()
        if requirement.get("document_id") in TARGET_DOCUMENT_IDS
    }


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
    loaded_requirements = rtiq.load_standard_requirements(
        bundle, rtiq.DEFAULT_CONTRACT_DIRECTORY
    )
    requirements = target_standard_requirements(loaded_requirements)
    plan = rtiq.load_json(rtiq.DEFAULT_PLAN)
    catalog = rtiq.load_json(rtiq.DEFAULT_CPP_TCK_CATALOG)
    java_catalog = java_tck.load_catalog()

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

    java_tck_mappings: dict[str, list[dict[str, str]]] = defaultdict(list)
    java_known_lab_requirement_ids = java_tck.lab_requirement_ids(
        java_tck.DEFAULT_BUNDLE
    )
    java_contract_requirement_cache: dict[tuple[str, ...], set[str]] = {}
    for scenario in object_rows(java_catalog.get("scenarios")):
        contract_refs_value = scenario.get("contract_refs")
        contract_refs = (
            tuple(ref for ref in contract_refs_value if isinstance(ref, str))
            if isinstance(contract_refs_value, list)
            else ()
        )
        if contract_refs not in java_contract_requirement_cache:
            java_contract_requirement_cache[contract_refs] = {
                record["requirements_lab_requirement_id"]
                for record in java_tck.contract_requirements(list(contract_refs))
                if isinstance(record.get("requirements_lab_requirement_id"), str)
            }
        linked_lab_ids = java_contract_requirement_cache[contract_refs]
        scenario_id = scenario.get("id")
        if not isinstance(scenario_id, str) or not scenario_id.startswith("java-tck."):
            continue
        for requirement_id in java_requirement_ids_for_scenario(
            scenario, java_known_lab_requirement_ids, linked_lab_ids
        ):
            if requirement_id in requirements:
                java_tck_mappings[requirement_id].append(
                    {
                        "id": scenario_id,
                        "title": str(scenario.get("title") or ""),
                        "status": str(scenario.get("default_status") or "unspecified"),
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
        "java_tck_catalog_mapping_status",
        "java_tck_scenario_ids",
        "java_tck_scenario_titles",
        "java_tck_catalog_statuses",
        "tck_requirement_disposition",
        "tck_disposition_owner",
        "direct_test_mapping_status",
        "any_language_test_mapping_status",
        "evidence_boundary",
        "source_path",
        "source_start_line",
    ]

    args.output.parent.mkdir(parents=True, exist_ok=True)
    section_counts: Counter[str] = Counter()
    section_stats: dict[str, Counter[str]] = defaultdict(Counter)
    catch2_mapped = 0
    tck_mapped = 0
    cpp_mapped = 0
    java_mapped = 0
    any_language_mapped = 0
    cpp_mapped_requirement_ids: set[str] = set()
    java_mapped_requirement_ids: set[str] = set()
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
            java_rows = java_tck_mappings.get(requirement_id, [])
            disposition = dispositions.get(requirement_id, {})
            catch2_mapped += bool(catch_rows)
            tck_mapped += bool(tck_rows)
            if catch_rows or tck_rows:
                cpp_mapped += 1
                cpp_mapped_requirement_ids.add(requirement_id)
            if java_rows:
                java_mapped += 1
                java_mapped_requirement_ids.add(requirement_id)
            any_language_mapped += bool(catch_rows or tck_rows or java_rows)
            section_stats[section]["requirements"] += 1
            section_stats[section]["catch2_mapped"] += bool(catch_rows)
            section_stats[section]["tck_mapped"] += bool(tck_rows)
            section_stats[section]["either_mapped"] += bool(catch_rows or tck_rows)
            section_stats[section]["java_tck_mapped"] += bool(java_rows)
            section_stats[section]["any_language_mapped"] += bool(
                catch_rows or tck_rows or java_rows
            )
            section_stats[section]["disposition_only"] += bool(
                disposition and not catch_rows and not tck_rows and not java_rows
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
            java_statuses = {row["status"] for row in java_rows}

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
                    "java_tck_catalog_mapping_status": (
                        "mapped" if java_rows else "unmapped"
                    ),
                    "java_tck_scenario_ids": joined(
                        {row["id"] for row in java_rows}
                    ),
                    "java_tck_scenario_titles": joined(
                        {row["title"] for row in java_rows}
                    ),
                    "java_tck_catalog_statuses": joined(java_statuses),
                    "tck_requirement_disposition": disposition.get("status", ""),
                    "tck_disposition_owner": disposition.get("owner_role", ""),
                    "direct_test_mapping_status": (
                        "mapped" if catch_rows or tck_rows else "unmapped"
                    ),
                    "any_language_test_mapping_status": (
                        "mapped" if catch_rows or tck_rows or java_rows else "unmapped"
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
            "java_tck_mapped_count",
            "java_tck_mapping_percent",
            "mapped_by_any_language_count",
            "mapped_by_any_language_percent",
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
                    "java_tck_mapped_count": stats["java_tck_mapped"],
                    "java_tck_mapping_percent": f"{100 * stats['java_tck_mapped'] / count:.2f}",
                    "mapped_by_any_language_count": stats["any_language_mapped"],
                    "mapped_by_any_language_percent": f"{100 * stats['any_language_mapped'] / count:.2f}",
                    "unmapped_count": count - stats["any_language_mapped"],
                    "disposition_only_count": stats["disposition_only"],
                }
            )

    document_summary_path = args.output.with_name(f"{args.output.stem}-by-document.csv")
    document_stats = aggregate_section_stats_by_document(section_stats)
    with document_summary_path.open("w", encoding="utf-8-sig", newline="") as stream:
        fields = [
            "document_id",
            "requirement_count",
            "catch2_mapped_count",
            "catch2_mapping_percent",
            "cpp_tck_mapped_count",
            "cpp_tck_mapping_percent",
            "cpp_mapped_by_either_count",
            "cpp_mapping_percent",
            "java_tck_mapped_count",
            "java_tck_mapping_percent",
            "mapped_by_any_language_count",
            "mapped_by_any_language_percent",
            "unmapped_count",
            "disposition_only_count",
        ]
        writer = csv.DictWriter(stream, fieldnames=fields)
        writer.writeheader()
        for document_id, stats in sorted(document_stats.items()):
            count = stats["requirements"]
            writer.writerow(
                {
                    "document_id": document_id,
                    "requirement_count": count,
                    "catch2_mapped_count": stats["catch2_mapped"],
                    "catch2_mapping_percent": f"{100 * stats['catch2_mapped'] / count:.2f}",
                    "cpp_tck_mapped_count": stats["tck_mapped"],
                    "cpp_tck_mapping_percent": f"{100 * stats['tck_mapped'] / count:.2f}",
                    "cpp_mapped_by_either_count": stats["either_mapped"],
                    "cpp_mapping_percent": f"{100 * stats['either_mapped'] / count:.2f}",
                    "java_tck_mapped_count": stats["java_tck_mapped"],
                    "java_tck_mapping_percent": f"{100 * stats['java_tck_mapped'] / count:.2f}",
                    "mapped_by_any_language_count": stats["any_language_mapped"],
                    "mapped_by_any_language_percent": f"{100 * stats['any_language_mapped'] / count:.2f}",
                    "unmapped_count": count - stats["any_language_mapped"],
                    "disposition_only_count": stats["disposition_only"],
                }
            )

    print(f"CSV: {args.output}")
    print(f"Section summary CSV: {summary_path}")
    print(f"Document summary CSV: {document_summary_path}")
    print(f"Requirements: {len(requirements)}")
    print(
        f"Target documents: {', '.join(sorted(TARGET_DOCUMENT_IDS))}; "
        f"other pinned 2025 bundle rows excluded from totals: "
        f"{len(loaded_requirements) - len(requirements)}"
    )
    print(f"Sections: {len(section_counts)} (see section summary CSV)")
    print(
        f"C++ Catch2-mapped requirements: {catch2_mapped} "
        f"({100 * catch2_mapped / len(requirements):.1f}%)"
    )
    print(
        f"Portable C++ TCK-mapped requirements: {tck_mapped} "
        f"({100 * tck_mapped / len(requirements):.1f}%)"
    )
    print(
        f"C++ mapped by Catch2 or portable TCK: {cpp_mapped} "
        f"({100 * cpp_mapped / len(requirements):.1f}%)"
    )
    print(
        f"Portable Java TCK-mapped requirements: {java_mapped} "
        f"({100 * java_mapped / len(requirements):.1f}%)"
    )
    print(
        f"Mapped by both C++ and Java inventories: "
        f"{len(cpp_mapped_requirement_ids & java_mapped_requirement_ids)}"
    )
    print(
        f"Mapped by either language inventory: {any_language_mapped} "
        f"({100 * any_language_mapped / len(requirements):.1f}%)"
    )
    for document_id, stats in sorted(document_stats.items()):
        count = stats["requirements"]
        print(
            f"{document_id}: {count} requirements; C++ "
            f"{stats['either_mapped']} ({100 * stats['either_mapped'] / count:.1f}%); "
            f"Java {stats['java_tck_mapped']} "
            f"({100 * stats['java_tck_mapped'] / count:.1f}%); either language "
            f"{stats['any_language_mapped']} "
            f"({100 * stats['any_language_mapped'] / count:.1f}%)"
        )
    print(DISCLAIMER)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
