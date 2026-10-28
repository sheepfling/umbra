"""Test inventory, mapping, and execution-evidence gate."""

from __future__ import annotations

import time
import xml.etree.ElementTree as ET
from collections import defaultdict
from pathlib import Path
from typing import cast

from .common import _finding, _read_json, _report
from .models import (
    ExecutionReport,
    GateReport,
    TestInventory,
    TestsPolicy,
)


def _junit_cases(path: Path) -> dict[str, list[ET.Element]]:
    document = ET.parse(path)
    cases: dict[str, list[ET.Element]] = defaultdict(list)
    for element in document.iter():
        if element.tag.rsplit("}", 1)[-1] == "testcase":
            cases[element.get("name", "")].append(element)
    return cases


def _case_has(case: ET.Element, tag: str) -> bool:
    return any(child.tag.rsplit("}", 1)[-1] == tag for child in case)


def _execution_cases(
    path: Path, *, normalized_json: bool, run_id: str | None = None
) -> dict[str, list[str]]:
    results: dict[str, list[str]] = defaultdict(list)
    if normalized_json:
        payload = cast(ExecutionReport, _read_json(path))
        if payload.get("schema") != 1 or not isinstance(payload.get("cases"), list):
            raise ValueError("execution report needs schema 1 and a cases array")
        if run_id is not None and payload.get("run_id") != run_id:
            raise ValueError("execution report has the wrong run_id")
        cases = payload.get("cases")
        if not isinstance(cases, list):
            raise ValueError("execution report needs a cases array")
        for case in cases:
            if (
                not isinstance(case, dict)
                or not isinstance(case.get("name"), str)
                or not case["name"]
                or case.get("status") not in {"passed", "failed", "error", "skipped"}
            ):
                raise ValueError("execution case needs name and valid status")
            results[case["name"]].append(case["status"])
    else:
        for name, occurrences in _junit_cases(path).items():
            for case in occurrences:
                status = next(
                    (
                        tag
                        for tag in ("failure", "error", "skipped")
                        if _case_has(case, tag)
                    ),
                    "passed",
                )
                results[name].append("failed" if status == "failure" else status)
    return results


def check_tests(
    root: Path,
    policy: TestsPolicy,
    inventory: Path | None,
    junit: Path | None,
    run_id: str | None,
    execution_json: Path | None = None,
) -> GateReport:
    if junit is not None and execution_json is not None:
        raise ValueError("choose either JUnit or normalized execution JSON")
    root = root.resolve()
    declarations = policy.get("cases")
    if not isinstance(declarations, list):
        raise TypeError("tests.cases must be an array")
    for key in (
        "require_mappings",
        "allow_extra_tests",
        "require_execution",
        "require_source_anchor",
    ):
        if key in policy and not isinstance(policy.get(key), bool):
            raise TypeError(f"tests.{key} must be boolean")
    if not isinstance(policy.get("known_mapping_ids", []), list):
        raise TypeError("tests.known_mapping_ids must be an array")
    findings: list[dict[str, str]] = []
    age_limit = int(policy.get("max_evidence_age_seconds", 3600))
    if age_limit < 0:
        raise ValueError("tests.max_evidence_age_seconds must be nonnegative")
    minimum = int(policy.get("min_cases", 1))
    if minimum < 1:
        raise ValueError("tests.min_cases must be at least 1")
    if len(declarations) < minimum:
        findings.append(
            _finding("too_few_declared_tests", f"{len(declarations)} < {minimum}")
        )
    ids: set[str] = set()
    selectors: set[str] = set()
    known_mappings = set(policy.get("known_mapping_ids", []))
    for row in declarations:
        if not isinstance(row, dict):
            raise TypeError("every tests.cases entry must be an object")
        if set(row) - {
            "id",
            "selector",
            "source",
            "source_anchor",
            "mappings",
            "classification",
        }:
            raise ValueError("unknown tests.cases key")
        case_id = row.get("id")
        selector = row.get("selector")
        source = row.get("source")
        if not all(
            isinstance(item, str) and item for item in (case_id, selector, source)
        ):
            raise ValueError("every test needs nonempty id, selector, and source")
        if case_id in ids:
            findings.append(_finding("duplicate_test_id", case_id))
        if selector in selectors:
            findings.append(_finding("duplicate_test_selector", selector))
        ids.add(case_id)
        selectors.add(selector)
        resolved = (root / source).resolve()
        if not resolved.is_relative_to(root) or not resolved.is_file():
            findings.append(
                _finding(
                    "missing_test_source",
                    "source is absent or outside root",
                    path=source,
                )
            )
        anchor = row.get("source_anchor")
        if policy.get("require_source_anchor", False) and not anchor:
            findings.append(_finding("missing_source_anchor", case_id))
        if anchor is not None:
            if not isinstance(anchor, str) or not anchor:
                raise ValueError(f"source_anchor must be nonempty text: {case_id}")
            if (
                resolved.is_relative_to(root)
                and resolved.is_file()
                and anchor not in resolved.read_text(encoding="utf-8", errors="replace")
            ):
                findings.append(
                    _finding("source_anchor_not_found", anchor, path=source)
                )
        mappings = row.get("mappings", [])
        if not isinstance(mappings, list) or any(
            not isinstance(item, str) for item in mappings
        ):
            raise ValueError(f"mappings must be a string array: {case_id}")
        if mappings:
            if row.get("classification") == "internal":
                findings.append(_finding("conflicting_test_classification", case_id))
            for mapping in mappings:
                if known_mappings and mapping not in known_mappings:
                    findings.append(_finding("unknown_mapping", mapping, path=case_id))
        elif policy.get("require_mappings", False):
            findings.append(_finding("unmapped_test", case_id))
        elif row.get("classification") != "internal":
            findings.append(_finding("unclassified_test", case_id))
    discovered: set[str] = set()
    if inventory is None:
        findings.append(_finding("missing_test_inventory", "pass --inventory-json"))
    else:
        if time.time() - inventory.stat().st_mtime > age_limit:
            findings.append(
                _finding(
                    "stale_test_inventory",
                    f"inventory is older than {age_limit} seconds",
                )
            )
        inventory_data = cast(TestInventory, _read_json(inventory))
        if (
            run_id is not None
            and "run_id" in inventory_data
            and inventory_data["run_id"] != run_id
        ):
            findings.append(_finding("wrong_inventory_run_id", str(inventory)))
        tests = inventory_data.get("tests")
        if not isinstance(tests, list):
            raise ValueError("test inventory needs a tests array")
        if any(
            not isinstance(item, dict)
            or not isinstance(item.get("name"), str)
            or not item["name"]
            for item in tests
        ):
            raise ValueError("every discovered test needs a nonempty name")
        names = [item["name"] for item in tests]
        discovered = set(names)
        for selector in selectors - discovered:
            findings.append(_finding("selector_not_discovered", selector))
        for selector in set(names) & selectors:
            if names.count(selector) != 1:
                findings.append(_finding("selector_not_unique", selector))
        if not policy.get("allow_extra_tests", True):
            for selector in discovered - selectors:
                findings.append(_finding("unexpected_discovered_test", selector))
    executed: set[str] = set()
    result_path = execution_json or junit
    if result_path is None:
        if policy.get("require_execution", False):
            findings.append(
                _finding(
                    "missing_execution_evidence", "pass --junit or --execution-json"
                )
            )
    else:
        if time.time() - result_path.stat().st_mtime > age_limit:
            findings.append(
                _finding("stale_execution", f"result is older than {age_limit} seconds")
            )
        if (
            inventory is not None
            and result_path.stat().st_mtime + 2 < inventory.stat().st_mtime
        ):
            findings.append(
                _finding(
                    "execution_predates_discovery",
                    "execution result predates test inventory",
                )
            )
        cases = _execution_cases(
            result_path, normalized_json=execution_json is not None, run_id=run_id
        )
        executed = set(cases)
        for selector in selectors - executed:
            findings.append(_finding("declared_test_not_executed", selector))
        if not policy.get("allow_extra_tests", True):
            for selector in executed - selectors:
                findings.append(_finding("unexpected_executed_test", selector))
        for selector, occurrences in cases.items():
            if len(occurrences) != 1:
                findings.append(_finding("duplicate_execution_case", selector))
            for status in occurrences:
                if status != "passed":
                    findings.append(_finding(f"execution_{status}", selector))
    return _report(
        "tests",
        findings,
        {
            "declared_count": len(declarations),
            "discovered_count": len(discovered),
            "executed_count": len(executed),
            "declared_selectors": sorted(selectors),
            "discovered_selectors": sorted(discovered),
            "executed_selectors": sorted(executed),
        },
        run_id,
    )
