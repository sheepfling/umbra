#!/usr/bin/env python3
"""Verify the installed-profile process smoke names and labels.

The package consumer is configured in a clean downstream build tree, so its
CTest catalog is the authoritative place to verify that each process smoke
is still independently addressable.  The expected names and labels live in
the roadmap index; this check joins the two without running the consumer or
resyncing the Requirements Lab.
"""

from __future__ import annotations

import argparse
import json
import shutil
import subprocess
import sys
from collections.abc import Iterable
from pathlib import Path
from typing import Any


REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_INDEX = REPOSITORY_ROOT / "docs" / "planning" / "ROADMAP-INDEX.json"

# Keep this list deliberately explicit.  A new package projection must first
# acquire a roadmap handle; otherwise it cannot silently appear as an
# untraceable CTest lane.
PACKAGE_LANE_FIELDS = (
    ("next_process_package_test", "next_process_package_ctest_filter"),
    (
        "next_process_package_timestamped_test",
        "next_process_package_timestamped_ctest_filter",
    ),
    (
        "next_process_package_parameterized_test",
        "next_process_package_parameterized_ctest_filter",
    ),
    (
        "next_process_package_connection_loss_test",
        "next_process_package_connection_loss_ctest_filter",
    ),
    (
        "next_process_package_object_registration_test",
        "next_process_package_object_registration_ctest_filter",
    ),
    (
        "next_process_package_named_registration_test",
        "next_process_package_named_registration_ctest_filter",
    ),
    (
        "next_process_package_attribute_update_test",
        "next_process_package_attribute_update_ctest_filter",
    ),
    (
        "next_process_package_directed_retraction_test",
        "next_process_package_directed_retraction_ctest_filter",
    ),
)


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Verify installed-profile process package CTest names and labels "
            "against the roadmap index."
        )
    )
    parser.add_argument(
        "--ctest",
        required=True,
        help="CTest executable path or a command resolvable on PATH.",
    )
    parser.add_argument(
        "--test-dir",
        required=True,
        type=Path,
        help="Clean downstream package-consumer build directory.",
    )
    parser.add_argument(
        "--index",
        type=Path,
        default=DEFAULT_INDEX,
        help="Roadmap index containing the expected package handles.",
    )
    parser.add_argument(
        "--config",
        help="Optional CTest configuration for a multi-config consumer build.",
    )
    return parser.parse_args()


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


def labels_for(test: dict[str, Any]) -> frozenset[str]:
    for property_value in test.get("properties", ()):
        if not isinstance(property_value, dict) or property_value.get("name") != "LABELS":
            continue
        labels = property_value.get("value", ())
        if isinstance(labels, str):
            return frozenset(label for label in labels.split(";") if label)
        if isinstance(labels, Iterable):
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
        detail = result.stderr.strip() or result.stdout.strip()
        raise ValueError(f"CTest catalog listing failed: {detail}")
    try:
        catalog = json.loads(result.stdout)
        tests = catalog["tests"]
    except (json.JSONDecodeError, KeyError, TypeError) as error:
        raise ValueError(f"could not read the CTest JSON catalog: {error}") from error
    if not isinstance(tests, list):
        raise ValueError("CTest JSON catalog has no test list")
    return [test for test in tests if isinstance(test, dict)]


def expected_lanes(index: dict[str, Any]) -> tuple[list[tuple[str, str]], list[str]]:
    candidates = [
        item
        for item in index.get("items", [])
        if isinstance(item, dict)
        and any(item.get(name) is not None for name, _ in PACKAGE_LANE_FIELDS)
    ]
    candidates.sort(key=lambda item: item.get("priority", 999))
    if not candidates:
        return [], ["roadmap index has no process-package lane handles"]

    item = candidates[0]
    lanes: list[tuple[str, str]] = []
    errors: list[str] = []
    for test_field, label_field in PACKAGE_LANE_FIELDS:
        test_name = item.get(test_field)
        label = item.get(label_field)
        if not isinstance(test_name, str) or not test_name.strip():
            errors.append(f"{test_field} is missing or empty")
        if not isinstance(label, str) or not label.strip():
            errors.append(f"{label_field} is missing or empty")
        if isinstance(test_name, str) and isinstance(label, str):
            lanes.append((test_name, label))

    if len({test_name for test_name, _ in lanes}) != len(lanes):
        errors.append("roadmap index repeats a process-package test name")
    if len({label for _, label in lanes}) != len(lanes):
        errors.append("roadmap index repeats a process-package label")
    return lanes, errors


def main() -> int:
    arguments = parse_arguments()
    ctest_path = Path(arguments.ctest)
    if not ctest_path.is_file():
        resolved_ctest = shutil.which(arguments.ctest)
        if resolved_ctest is None:
            print(f"CTest executable does not exist or is not on PATH: {arguments.ctest}", file=sys.stderr)
            return 2
        ctest_path = Path(resolved_ctest)
    arguments.ctest = ctest_path
    if not arguments.ctest.is_file():
        print(f"CTest executable does not exist: {arguments.ctest}", file=sys.stderr)
        return 2
    if not arguments.test_dir.is_dir():
        print(f"CTest test directory does not exist: {arguments.test_dir}", file=sys.stderr)
        return 2

    try:
        index = load_json(arguments.index)
        catalog = load_catalog(arguments)
        lanes, violations = expected_lanes(index)
    except ValueError as error:
        print(f"verify_process_package_lanes: {error}", file=sys.stderr)
        return 2

    catalog_by_name = {
        test.get("name"): test
        for test in catalog
        if isinstance(test.get("name"), str)
    }
    package_tests = {
        name: test
        for name, test in catalog_by_name.items()
        if name.startswith("umbra_rti_package_process_")
    }
    expected_names = {name for name, _ in lanes}
    expected_labels = {label for _, label in lanes}
    actual_labels = {
        label
        for test in package_tests.values()
        for label in labels_for(test)
        if label.startswith("package-process")
    }
    if package_tests.keys() != expected_names:
        missing = sorted(expected_names - package_tests.keys())
        unexpected = sorted(package_tests.keys() - expected_names)
        if missing:
            violations.append(f"CTest catalog is missing package tests: {', '.join(missing)}")
        if unexpected:
            violations.append(f"CTest catalog has unindexed package tests: {', '.join(unexpected)}")
    if actual_labels != expected_labels:
        missing = sorted(expected_labels - actual_labels)
        unexpected = sorted(actual_labels - expected_labels)
        if missing:
            violations.append(f"CTest catalog is missing package labels: {', '.join(missing)}")
        if unexpected:
            violations.append(f"CTest catalog has unindexed package labels: {', '.join(unexpected)}")

    summaries: list[str] = []
    for test_name, label in lanes:
        test = catalog_by_name.get(test_name)
        if test is None:
            continue
        labels = labels_for(test)
        if label not in labels:
            violations.append(f"{test_name} is missing its indexed label {label}")
        selected = [candidate for candidate in catalog if label in labels_for(candidate)]
        if len(selected) != 1:
            violations.append(
                f"indexed package label {label} selects {len(selected)} tests; expected exactly one"
            )
        summaries.append(f"  {label}: {test_name}")

    if violations:
        print("Installed-profile process package catalog violations:", file=sys.stderr)
        print("\n".join(f"  {violation}" for violation in violations), file=sys.stderr)
        return 1

    print("Verified installed-profile process package catalog against ROADMAP-INDEX.json:")
    print("\n".join(summaries))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
