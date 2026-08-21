#!/usr/bin/env python3
"""Verify that each named CTest service lane remains a complete development slice.

Catch2 tags are exposed as CTest labels and Requirements-Lab contracts receive
matching labels at CMake configure time.  This check inspects CTest's generated
test catalog, rather than a second hand-maintained list of test names, so it
detects a lost behavior case or traceability check before a focused lane becomes
misleadingly narrow.
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from collections.abc import Iterable
from pathlib import Path
from typing import Any


CATCH2_TEST_PREFIX = "umbra.ieee1516_2025.catch2."
REQUIRED_LABELS = ("requirements-lab", "api-contract")


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Verify behavioral and traceability membership of focused service lanes."
    )
    parser.add_argument(
        "--ctest",
        required=True,
        type=Path,
        help="Path to the CTest executable.",
    )
    parser.add_argument(
        "--test-dir",
        required=True,
        type=Path,
        help="Configured CMake build directory containing CTest metadata.",
    )
    parser.add_argument(
        "--config",
        help="Optional CTest configuration for multi-config build trees.",
    )
    parser.add_argument(
        "--lane",
        action="append",
        required=True,
        metavar="LABEL",
        help=(
            "Exact CTest label for a named service lane. Repeat for each lane "
            "that must retain Catch2, Requirements-Lab, and API-contract coverage."
        ),
    )
    return parser.parse_args()


def labels_for(test: dict[str, Any]) -> frozenset[str]:
    properties = test.get("properties", ())
    if not isinstance(properties, Iterable):
        return frozenset()
    for property_value in properties:
        if not isinstance(property_value, dict) or property_value.get("name") != "LABELS":
            continue
        labels = property_value.get("value", ())
        if isinstance(labels, list):
            return frozenset(label for label in labels if isinstance(label, str))
    return frozenset()


def load_catalog(arguments: argparse.Namespace) -> list[dict[str, Any]]:
    command = [
        str(arguments.ctest),
        "--test-dir",
        str(arguments.test_dir),
        "--show-only=json-v1",
    ]
    if arguments.config:
        command.extend(("-C", arguments.config))
    result = subprocess.run(
        command,
        check=False,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    if result.returncode != 0:
        print("CTest test-catalog listing failed:", file=sys.stderr)
        if result.stderr:
            print(result.stderr.rstrip(), file=sys.stderr)
        return []
    try:
        catalog = json.loads(result.stdout)
        tests = catalog["tests"]
    except (json.JSONDecodeError, KeyError, TypeError) as error:
        print(f"Could not read the CTest JSON test catalog: {error}", file=sys.stderr)
        return []
    if not isinstance(tests, list):
        print("CTest JSON test catalog has no test list.", file=sys.stderr)
        return []
    return [test for test in tests if isinstance(test, dict)]


def main() -> int:
    arguments = parse_arguments()
    if not arguments.ctest.is_file():
        print(f"CTest executable does not exist: {arguments.ctest}", file=sys.stderr)
        return 2
    if not arguments.test_dir.is_dir():
        print(f"CTest test directory does not exist: {arguments.test_dir}", file=sys.stderr)
        return 2

    tests = load_catalog(arguments)
    if not tests:
        return 2

    violations: list[str] = []
    summaries: list[str] = []
    for lane in arguments.lane:
        selected = [test for test in tests if lane in labels_for(test)]
        catch2_count = sum(
            1
            for test in selected
            if isinstance(test.get("name"), str)
            and test["name"].startswith(CATCH2_TEST_PREFIX)
        )
        requirements_count = sum(
            1 for test in selected if "requirements-lab" in labels_for(test)
        )
        api_count = sum(1 for test in selected if "api-contract" in labels_for(test))
        missing: list[str] = []
        if not catch2_count:
            missing.append("a Catch2 behavior case")
        if not requirements_count:
            missing.append("a Requirements-Lab traceability check")
        if not api_count:
            missing.append("an API-contract traceability check")
        if missing:
            violations.append(f"  {lane}: missing {', '.join(missing)}")
            continue
        summaries.append(
            f"  {lane}: {catch2_count} Catch2, {requirements_count} Requirements-Lab, "
            f"{api_count} API-contract"
        )

    if violations:
        print("Focused CTest service-lane audit violations:", file=sys.stderr)
        print("\n".join(violations), file=sys.stderr)
        return 1

    print("Verified focused CTest service-lane membership:")
    print("\n".join(summaries))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
