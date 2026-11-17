"""Query saved portable C++ TCK executions or find the smallest incomplete sections."""

from __future__ import annotations

import argparse
import json
from collections import defaultdict
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_CATALOG = ROOT / "compliance" / "catalogs" / "cpp-tck-scenario-catalog.json"
EVIDENCE_KIND = "hla-rti-cpp-tck-evidence"
DEFAULT_CALLBACK_MODELS = ("evoked", "immediate")
CANONICAL_2025_SECTION_PREFIXES = (
    "hla-1516.1-2025:",
    "hla-1516.2-2025:",
)


def _result_files(roots: list[Path]) -> list[Path]:
    files: set[Path] = set()
    for root in roots:
        if root.is_file():
            files.add(root)
        elif root.is_dir():
            files.update(path for path in root.rglob("*.json") if path.is_file())
        else:
            raise FileNotFoundError(f"Results path does not exist: {root}")
    return sorted(files)


def _load_observations(roots: list[Path]) -> tuple[dict[tuple[str, str], set[str]], int, list[str]]:
    observations: dict[tuple[str, str], set[str]] = defaultdict(set)
    evidence_file_count = 0
    warnings: list[str] = []
    for path in _result_files(roots):
        try:
            payload = json.loads(path.read_text(encoding="utf-8-sig"))
        except (OSError, json.JSONDecodeError) as exc:
            warnings.append(f"Could not read {path}: {exc}")
            continue
        if not isinstance(payload, dict) or payload.get("kind") != EVIDENCE_KIND:
            continue
        results = payload.get("results")
        if not isinstance(results, list):
            warnings.append(f"Evidence file has no results list: {path}")
            continue
        evidence_file_count += 1
        for result in results:
            if not isinstance(result, dict):
                continue
            scenario_id = result.get("id")
            callback_model = result.get("callback_model")
            status = result.get("status")
            if all(isinstance(value, str) and value for value in (scenario_id, callback_model, status)):
                observations[(scenario_id, callback_model)].add(status)
    return observations, evidence_file_count, warnings


def _scenario_row(
    entry: dict[str, Any],
    mappings: list[dict[str, Any]],
    observations: dict[tuple[str, str], set[str]],
) -> dict[str, Any]:
    scenario_id = entry.get("id")
    declared_models = entry.get("callback_models")
    if declared_models is None:
        expected_models = list(DEFAULT_CALLBACK_MODELS)
    elif isinstance(declared_models, str):
        expected_models = [declared_models] if declared_models else []
    elif isinstance(declared_models, list):
        expected_models = [model for model in declared_models if isinstance(model, str) and model]
    else:
        expected_models = []
    model_status: dict[str, dict[str, Any]] = {}
    for model in expected_models:
        statuses = sorted(observations.get((scenario_id, model), set()))
        model_status[model] = {
            "passed": "passed" in statuses,
            "failed": "failed" in statuses,
            "observed_statuses": statuses,
        }
    passed_models = [model for model in expected_models if model_status[model]["passed"]]
    missing_models = [model for model in expected_models if not model_status[model]["passed"]]
    if not expected_models:
        state = "no-models-declared"
    elif not missing_models:
        state = "complete"
    elif passed_models:
        state = "partial"
    else:
        state = "unwitnessed"
    return {
        "id": scenario_id,
        "title": entry.get("title", ""),
        "promotion": entry.get("promotion", ""),
        "callback_models": expected_models,
        "passed_models": passed_models,
        "missing_models": missing_models,
        "model_status": model_status,
        "state": state,
        "requirement_mappings": mappings,
        "execution_blocker": entry.get("execution_blocker"),
    }


def build_report(
    catalog_path: Path,
    results_paths: list[Path],
    *,
    section: str | None = None,
    requirement: str | None = None,
    scenario: str | None = None,
    promotion: str = "promoted",
) -> dict[str, Any]:
    """Join catalog mappings with saved runs, without inferring conformance."""
    selectors = [section, requirement, scenario]
    if sum(value is not None for value in selectors) != 1:
        raise ValueError("Select exactly one of section, requirement, or scenario")

    catalog = json.loads(catalog_path.read_text(encoding="utf-8-sig"))
    observations, evidence_file_count, warnings = _load_observations(results_paths)
    selected: list[dict[str, Any]] = []
    for entry in catalog.get("scenarios", []):
        if promotion != "all" and entry.get("promotion") != promotion:
            continue
        mappings = entry.get("requirement_mappings", [])
        if section is not None:
            mappings = [mapping for mapping in mappings if mapping.get("standard_section") == section]
        elif requirement is not None:
            mappings = [mapping for mapping in mappings if mapping.get("requirement_id") == requirement]
        elif scenario is not None and entry.get("id") != scenario:
            continue
        if (section is not None or requirement is not None) and not mappings:
            continue

        selected.append(_scenario_row(entry, mappings, observations))

    selected.sort(key=lambda item: item["id"])
    requirement_ids = sorted(
        {
            mapping["requirement_id"]
            for item in selected
            for mapping in item["requirement_mappings"]
            if isinstance(mapping.get("requirement_id"), str)
        }
    )
    witnessed_requirements = sorted(
        {
            mapping["requirement_id"]
            for item in selected
            if item["state"] == "complete"
            for mapping in item["requirement_mappings"]
            if isinstance(mapping.get("requirement_id"), str)
        }
    )
    state_counts = {
        state: sum(item["state"] == state for item in selected)
        for state in ("complete", "partial", "unwitnessed", "no-models-declared")
    }
    return {
        "selector": {
            "section": section,
            "requirement": requirement,
            "scenario": scenario,
            "promotion": promotion,
        },
        "interpretation": (
            "A complete scenario has a passing result for every callback model declared in the catalog. "
            "This is local C++ TCK execution evidence, not interoperability or conformance evidence."
        ),
        "evidence_file_count": evidence_file_count,
        "direct_requirement_ids": requirement_ids,
        "requirements_with_complete_scenario_witness": witnessed_requirements,
        "scenario_state_counts": state_counts,
        "scenarios": selected,
        "warnings": warnings,
    }


def build_section_summary(
    catalog_path: Path,
    results_paths: list[Path],
    *,
    promotion: str = "promoted",
) -> dict[str, Any]:
    """Rank mapped C++ TCK sections by their remaining scenario evidence."""
    catalog = json.loads(catalog_path.read_text(encoding="utf-8-sig"))
    observations, evidence_file_count, warnings = _load_observations(results_paths)
    grouped: dict[str, dict[str, dict[str, Any]]] = defaultdict(dict)
    excluded_sections: dict[str, set[str]] = defaultdict(set)
    for entry in catalog.get("scenarios", []):
        if not isinstance(entry, dict):
            continue
        if promotion != "all" and entry.get("promotion") != promotion:
            continue
        scenario_id = entry.get("id")
        if not isinstance(scenario_id, str) or not scenario_id.startswith("cpp-tck."):
            continue
        mappings = entry.get("requirement_mappings", [])
        if not isinstance(mappings, list):
            continue
        for mapping in mappings:
            if not isinstance(mapping, dict):
                continue
            section = mapping.get("standard_section")
            if not isinstance(section, str) or not section:
                continue
            if not section.startswith(CANONICAL_2025_SECTION_PREFIXES):
                excluded_sections[section].add(scenario_id)
                continue
            scenario = grouped[section].setdefault(
                scenario_id,
                {"entry": entry, "mappings": []},
            )
            scenario["mappings"].append(mapping)

    sections: list[dict[str, Any]] = []
    blocked_by_scenario: dict[str, dict[str, Any]] = {}
    runnable_section_count = 0
    for section, scenarios in grouped.items():
        rows = [
            _scenario_row(item["entry"], item["mappings"], observations)
            for item in scenarios.values()
        ]
        rows.sort(key=lambda item: item["id"])
        for row in rows:
            blocker = row.get("execution_blocker")
            if row["state"] == "complete" or not isinstance(blocker, str) or not blocker:
                continue
            blocked = blocked_by_scenario.setdefault(
                row["id"],
                {
                    "id": row["id"],
                    "title": row["title"],
                    "execution_blocker": blocker,
                    "missing_models": row["missing_models"],
                    "standard_sections": [],
                    "requirement_ids": [],
                },
            )
            blocked["standard_sections"].append(section)
            blocked["requirement_ids"].extend(
                mapping["requirement_id"]
                for mapping in row["requirement_mappings"]
                if isinstance(mapping.get("requirement_id"), str)
            )
        runnable_rows = [
            row
            for row in rows
            if row["state"] == "complete" or not row.get("execution_blocker")
        ]
        if not runnable_rows:
            continue
        runnable_section_count += 1
        requirement_ids = sorted(
            {
                mapping["requirement_id"]
                for row in runnable_rows
                for mapping in row["requirement_mappings"]
                if isinstance(mapping.get("requirement_id"), str)
            }
        )
        witnessed_requirements = sorted(
            {
                mapping["requirement_id"]
                for row in runnable_rows
                if row["state"] == "complete"
                for mapping in row["requirement_mappings"]
                if isinstance(mapping.get("requirement_id"), str)
            }
        )
        incomplete_rows = [row for row in runnable_rows if row["state"] != "complete"]
        if not incomplete_rows:
            continue
        state_counts = {
            state: sum(row["state"] == state for row in runnable_rows)
            for state in ("complete", "partial", "unwitnessed", "no-models-declared")
        }
        sections.append(
            {
                "standard_section": section,
                "scenario_count": len(runnable_rows),
                "scenario_state_counts": state_counts,
                "incomplete_scenario_count": len(incomplete_rows),
                "direct_requirement_ids": requirement_ids,
                "requirements_with_complete_scenario_witness": witnessed_requirements,
                "unwitnessed_requirement_ids": sorted(set(requirement_ids) - set(witnessed_requirements)),
                "incomplete_scenarios": [
                    {
                        "id": row["id"],
                        "title": row["title"],
                        "state": row["state"],
                        "missing_models": row["missing_models"],
                    }
                    for row in incomplete_rows
                ],
            }
        )
    sections.sort(
        key=lambda item: (
            item["incomplete_scenario_count"],
            len(item["unwitnessed_requirement_ids"]),
            item["standard_section"],
        )
    )
    return {
        "mode": "next-incomplete-sections",
        "promotion": promotion,
        "interpretation": (
            "Canonical IEEE 1516.1-2025 and IEEE 1516.2-2025 sections are ranked by remaining promoted C++ TCK "
            "scenario count, then unwitnessed direct requirements. Other section identifiers are listed separately. "
            "This is local execution evidence, not interoperability or conformance evidence."
        ),
        "evidence_file_count": evidence_file_count,
        "mapped_section_count": runnable_section_count,
        "incomplete_section_count": len(sections),
        "sections": sections,
        "blocked_section_count": len(
            {
                section
                for blocked in blocked_by_scenario.values()
                for section in blocked["standard_sections"]
            }
        ),
        "blocked_scenarios": [
            {
                **blocked,
                "standard_sections": sorted(set(blocked["standard_sections"])),
                "requirement_ids": sorted(set(blocked["requirement_ids"])),
            }
            for _, blocked in sorted(blocked_by_scenario.items())
        ],
        "excluded_noncanonical_section_mappings": [
            {
                "standard_section": section,
                "scenario_ids": sorted(scenario_ids),
            }
            for section, scenario_ids in sorted(excluded_sections.items())
        ],
        "warnings": warnings,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--catalog", type=Path, default=DEFAULT_CATALOG)
    parser.add_argument(
        "--results",
        type=Path,
        action="append",
        required=True,
        metavar="FILE_OR_DIR",
        help="saved evidence JSON file or directory (repeatable)",
    )
    selector = parser.add_mutually_exclusive_group()
    selector.add_argument("--section", help="exact canonical section, e.g. hla-1516.1-2025:clause-8.6")
    selector.add_argument("--requirement", help="exact 2025 requirement id")
    selector.add_argument("--scenario", help="exact portable C++ TCK scenario id")
    parser.add_argument(
        "--next-sections",
        action="store_true",
        help="rank promoted C++ TCK sections by smallest remaining scenario-evidence slice",
    )
    parser.add_argument("--promotion", choices=("promoted", "candidate", "all"), default="promoted")
    parser.add_argument("--limit", type=int, default=12, help="incomplete scenario rows to show; 0 shows all")
    parser.add_argument("--json", action="store_true", help="emit machine-readable JSON")
    args = parser.parse_args()

    selected_modes = sum(value is not None for value in (args.section, args.requirement, args.scenario))
    if args.next_sections and selected_modes:
        parser.error("--next-sections cannot be combined with an exact section, requirement, or scenario selector")
    if not args.next_sections and selected_modes != 1:
        parser.error("select exactly one of --section, --requirement, or --scenario, or use --next-sections")

    try:
        if args.next_sections:
            report = build_section_summary(args.catalog, args.results, promotion=args.promotion)
        else:
            report = build_report(
                args.catalog,
                args.results,
                section=args.section,
                requirement=args.requirement,
                scenario=args.scenario,
                promotion=args.promotion,
            )
    except (OSError, json.JSONDecodeError, ValueError) as exc:
        parser.error(str(exc))

    if args.json:
        print(json.dumps(report, indent=2))
        return 0

    if args.next_sections:
        print(
            f"Incomplete promoted C++ TCK sections: {report['incomplete_section_count']} of "
            f"{report['mapped_section_count']} canonical 2025 sections; "
            f"adapter-blocked scenarios={len(report['blocked_scenarios'])}; "
            f"noncanonical section IDs={len(report['excluded_noncanonical_section_mappings'])}; "
            f"results files={report['evidence_file_count']}"
        )
        shown_count = len(report["sections"]) if args.limit == 0 else max(args.limit, 0)
        for section in report["sections"][:shown_count]:
            counts = section["scenario_state_counts"]
            print(
                f"- {section['standard_section']}: "
                f"scenarios={counts['complete']}/{section['scenario_count']} complete; "
                f"requirements={len(section['requirements_with_complete_scenario_witness'])}/"
                f"{len(section['direct_requirement_ids'])} witnessed; "
                f"remaining={section['incomplete_scenario_count']} scenarios"
            )
            for item in section["incomplete_scenarios"][:3]:
                missing = ",".join(item["missing_models"]) or item["state"]
                print(f"  * {item['id']}: missing pass for {missing}")
            remaining_rows = len(section["incomplete_scenarios"]) - 3
            if remaining_rows > 0:
                print(f"  * and {remaining_rows} more; query the exact section for details")
        if not report["sections"]:
            print("No runnable promoted, mapped C++ TCK section has an incomplete scenario witness.")
        if report["blocked_scenarios"]:
            blocked_limit = len(report["blocked_scenarios"]) if args.limit == 0 else max(args.limit, 0)
            for item in report["blocked_scenarios"][:blocked_limit]:
                sections = ",".join(item["standard_sections"])
                print(f"- blocked {item['id']} [{sections}]: {item['execution_blocker']}")
    else:
        counts = report["scenario_state_counts"]
        print(f"Saved C++ TCK evidence for {report['selector']}")
        print(
            f"Direct requirements: {len(report['direct_requirement_ids'])}; "
            f"with a complete mapped scenario: {len(report['requirements_with_complete_scenario_witness'])}; "
            f"scenarios complete/partial/unwitnessed: "
            f"{counts['complete']}/{counts['partial']}/{counts['unwitnessed']}"
        )
        incomplete = [item for item in report["scenarios"] if item["state"] != "complete"]
        shown_count = len(incomplete) if args.limit == 0 else max(args.limit, 0)
        for item in incomplete[:shown_count]:
            if item.get("execution_blocker"):
                print(f"- blocked {item['id']}: {item['execution_blocker']}")
                continue
            missing = ",".join(item["missing_models"]) or item["state"]
            print(f"- {item['id']}: missing pass for {missing}")
    if report["warnings"]:
        print(f"Warnings: {len(report['warnings'])} unreadable evidence file(s); use --json for details")
    print("Local test evidence only; not an interoperability or conformance result.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
