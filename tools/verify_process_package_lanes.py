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
from collections.abc import Sequence
from dataclasses import dataclass
from pathlib import Path
from typing import Literal, TypedDict, cast
from xml.etree import ElementTree

REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_INDEX = REPOSITORY_ROOT / "docs" / "planning" / "ROADMAP-INDEX.json"

# Keep this list deliberately explicit.  A new package projection must first
# acquire a roadmap handle; otherwise it cannot silently appear as an
# untraceable CTest lane.
PackageLaneField = Literal[
    "next_process_package_test",
    "next_process_package_ctest_filter",
    "next_process_package_timestamped_test",
    "next_process_package_timestamped_ctest_filter",
    "next_process_package_parameterized_test",
    "next_process_package_parameterized_ctest_filter",
    "next_process_package_connection_loss_test",
    "next_process_package_connection_loss_ctest_filter",
    "next_process_package_connection_loss_delete_objects_test",
    "next_process_package_connection_loss_delete_objects_ctest_filter",
    "next_process_package_object_registration_test",
    "next_process_package_object_registration_ctest_filter",
    "next_process_package_named_registration_test",
    "next_process_package_named_registration_ctest_filter",
    "next_process_package_attribute_update_test",
    "next_process_package_attribute_update_ctest_filter",
    "next_process_package_directed_retraction_test",
    "next_process_package_directed_retraction_ctest_filter",
    "next_process_package_federation_save_restore_test",
    "next_process_package_federation_save_restore_ctest_filter",
    "next_process_package_federation_save_restore_failure_test",
    "next_process_package_federation_save_restore_failure_ctest_filter",
    "next_process_package_federation_save_restore_abort_test",
    "next_process_package_federation_save_restore_abort_ctest_filter",
    "next_process_package_federation_save_restore_status_test",
    "next_process_package_federation_save_restore_status_ctest_filter",
]

PACKAGE_LANE_FIELDS: tuple[tuple[PackageLaneField, PackageLaneField], ...] = (
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
        "next_process_package_connection_loss_delete_objects_test",
        "next_process_package_connection_loss_delete_objects_ctest_filter",
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
    (
        "next_process_package_federation_save_restore_test",
        "next_process_package_federation_save_restore_ctest_filter",
    ),
    (
        "next_process_package_federation_save_restore_failure_test",
        "next_process_package_federation_save_restore_failure_ctest_filter",
    ),
    (
        "next_process_package_federation_save_restore_abort_test",
        "next_process_package_federation_save_restore_abort_ctest_filter",
    ),
    (
        "next_process_package_federation_save_restore_status_test",
        "next_process_package_federation_save_restore_status_ctest_filter",
    ),
)


class _RoadmapItemInput(TypedDict, total=False):
    priority: object
    next_process_package_test: object
    next_process_package_ctest_filter: object
    next_process_package_timestamped_test: object
    next_process_package_timestamped_ctest_filter: object
    next_process_package_parameterized_test: object
    next_process_package_parameterized_ctest_filter: object
    next_process_package_connection_loss_test: object
    next_process_package_connection_loss_ctest_filter: object
    next_process_package_connection_loss_delete_objects_test: object
    next_process_package_connection_loss_delete_objects_ctest_filter: object
    next_process_package_object_registration_test: object
    next_process_package_object_registration_ctest_filter: object
    next_process_package_named_registration_test: object
    next_process_package_named_registration_ctest_filter: object
    next_process_package_attribute_update_test: object
    next_process_package_attribute_update_ctest_filter: object
    next_process_package_directed_retraction_test: object
    next_process_package_directed_retraction_ctest_filter: object
    next_process_package_federation_save_restore_test: object
    next_process_package_federation_save_restore_ctest_filter: object
    next_process_package_federation_save_restore_failure_test: object
    next_process_package_federation_save_restore_failure_ctest_filter: object
    next_process_package_federation_save_restore_abort_test: object
    next_process_package_federation_save_restore_abort_ctest_filter: object
    next_process_package_federation_save_restore_status_test: object
    next_process_package_federation_save_restore_status_ctest_filter: object


class _RoadmapLaneHandleInput(TypedDict, total=False):
    consumer_test: object
    ctest_label: object


class _RoadmapMappingInput(TypedDict, total=False):
    lane_handles: object


class _RoadmapIndexInput(TypedDict, total=False):
    items: object
    mapping: object


class _CTestPropertyInput(TypedDict, total=False):
    name: object
    value: object


class _CTestTestInput(TypedDict, total=False):
    name: object
    properties: object


class _CTestCatalogInput(TypedDict, total=False):
    tests: object


@dataclass(frozen=True, slots=True)
class VerifierOptions:
    ctest: str
    test_dir: Path
    index: Path
    config: str | None
    junit: Path | None


@dataclass(frozen=True, slots=True)
class PackageLane:
    test_name: str
    label: str


@dataclass(frozen=True, slots=True)
class RoadmapItem:
    priority: int
    lanes: tuple[PackageLane, ...]
    violations: tuple[str, ...]


@dataclass(frozen=True, slots=True)
class RoadmapLaneHandle:
    consumer_test: str
    ctest_label: str


@dataclass(frozen=True, slots=True)
class RoadmapIndex:
    items: tuple[RoadmapItem, ...]
    lane_handles: dict[str, RoadmapLaneHandle]


@dataclass(frozen=True, slots=True)
class CTestProperty:
    name: str
    value: object


@dataclass(frozen=True, slots=True)
class CTestTest:
    name: str
    properties: tuple[CTestProperty, ...]


def parse_arguments() -> VerifierOptions:
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
    parser.add_argument(
        "--junit",
        type=Path,
        help=(
            "Optional CTest JUnit artifact to validate against the indexed "
            "process-package lanes."
        ),
    )
    arguments = parser.parse_args()
    return VerifierOptions(
        ctest=arguments.ctest,
        test_dir=arguments.test_dir,
        index=arguments.index,
        config=arguments.config,
        junit=arguments.junit,
    )


def _required_string(value: object, context: str) -> str:
    if not isinstance(value, str) or not value.strip():
        raise ValueError(f"{context} must be a non-empty string")
    return value


def load_json(path: Path) -> RoadmapIndex:
    try:
        value: object = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise ValueError(f"cannot read JSON file {path}: {error}") from error
    if not isinstance(value, dict):
        raise TypeError(f"JSON root in {path} must be an object")
    root = cast(_RoadmapIndexInput, value)
    raw_items = root.get("items")
    if not isinstance(raw_items, list):
        raise TypeError(f"JSON items in {path} must be an array")

    items: list[RoadmapItem] = []
    for item_index, raw_item in enumerate(raw_items):
        context = f"JSON items[{item_index}] in {path}"
        if not isinstance(raw_item, dict):
            raise TypeError(f"{context} must be an object")
        fields = cast(_RoadmapItemInput, raw_item)
        raw_priority = fields.get("priority", 999)
        if not isinstance(raw_priority, int) or isinstance(raw_priority, bool):
            raise TypeError(f"{context}.priority must be an integer")

        lanes: list[PackageLane] = []
        violations: list[str] = []
        for test_field, label_field in PACKAGE_LANE_FIELDS:
            test_value = fields.get(test_field)
            label_value = fields.get(label_field)
            if test_value is None and label_value is None:
                continue
            if not isinstance(test_value, str) or not test_value.strip():
                violations.append(f"{test_field} is missing or empty")
            if not isinstance(label_value, str) or not label_value.strip():
                violations.append(f"{label_field} is missing or empty")
            if isinstance(test_value, str) and isinstance(label_value, str):
                lanes.append(PackageLane(test_name=test_value, label=label_value))
        items.append(
            RoadmapItem(
                priority=raw_priority,
                lanes=tuple(lanes),
                violations=tuple(violations),
            )
        )

    raw_mapping = root.get("mapping", {})
    if not isinstance(raw_mapping, dict):
        raise TypeError(f"JSON mapping in {path} must be an object")
    mapping = cast(_RoadmapMappingInput, raw_mapping)
    raw_handles = mapping.get("lane_handles", {})
    if not isinstance(raw_handles, dict):
        raise TypeError(f"JSON lane_handles in {path} must be an object")
    lane_handles: dict[str, RoadmapLaneHandle] = {}
    for label, raw_handle in raw_handles.items():
        if not isinstance(label, str):
            raise TypeError(f"JSON lane_handles keys in {path} must be strings")
        if not label.startswith("package-process"):
            continue
        context = f"JSON lane_handles[{label!r}] in {path}"
        if not isinstance(raw_handle, dict):
            raise TypeError(f"{context} must be an object")
        handle_fields = cast(_RoadmapLaneHandleInput, raw_handle)
        lane_handles[label] = RoadmapLaneHandle(
            consumer_test=_required_string(
                handle_fields.get("consumer_test"), f"{context}.consumer_test"
            ),
            ctest_label=_required_string(
                handle_fields.get("ctest_label"), f"{context}.ctest_label"
            ),
        )
    return RoadmapIndex(items=tuple(items), lane_handles=lane_handles)


def _string_values(value: object) -> list[str]:
    if isinstance(value, str):
        return [label for label in value.split(";") if label]
    if isinstance(value, list):
        return [item for item in value if isinstance(item, str) and item]
    return []


def labels_for(test: CTestTest) -> frozenset[str]:
    for property_value in test.properties:
        if property_value.name != "LABELS":
            continue
        return frozenset(_string_values(property_value.value))
    return frozenset()


def load_catalog(
    ctest_path: Path, test_dir: Path, config: str | None
) -> list[CTestTest]:
    command = [
        str(ctest_path),
        "--test-dir",
        str(test_dir),
        "--show-only=json-v1",
    ]
    if config:
        command.extend(("-C", config))
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
        catalog_value: object = json.loads(result.stdout)
    except (json.JSONDecodeError, KeyError, TypeError) as error:
        raise ValueError(f"could not read the CTest JSON catalog: {error}") from error
    if not isinstance(catalog_value, dict):
        raise TypeError("CTest JSON catalog root must be an object")
    catalog = cast(_CTestCatalogInput, catalog_value)
    tests = catalog.get("tests")
    if not isinstance(tests, list):
        raise TypeError("CTest JSON catalog tests must be an array")
    normalized: list[CTestTest] = []
    for index, value in enumerate(tests):
        context = f"CTest JSON catalog tests[{index}]"
        if not isinstance(value, dict):
            raise TypeError(f"{context} must be an object")
        raw_test = cast(_CTestTestInput, value)
        name = _required_string(raw_test.get("name"), f"{context}.name")
        raw_properties = raw_test.get("properties", [])
        if not isinstance(raw_properties, list):
            raise TypeError(f"{context}.properties must be an array")
        properties: list[CTestProperty] = []
        for property_index, property_value in enumerate(raw_properties):
            property_context = f"{context}.properties[{property_index}]"
            if not isinstance(property_value, dict):
                raise TypeError(f"{property_context} must be an object")
            raw_property = cast(_CTestPropertyInput, property_value)
            property_name = _required_string(
                raw_property.get("name"), f"{property_context}.name"
            )
            properties.append(CTestProperty(property_name, raw_property.get("value")))
        normalized.append(CTestTest(name=name, properties=tuple(properties)))
    return normalized


def expected_lanes(index: RoadmapIndex) -> tuple[list[PackageLane], list[str]]:
    candidates = [item for item in index.items if item.lanes or item.violations]
    candidates.sort(key=lambda item: item.priority)
    if not candidates:
        return [], ["roadmap index has no process-package lane handles"]

    item = candidates[0]
    lanes = list(item.lanes)
    errors = list(item.violations)
    if len({lane.test_name for lane in lanes}) != len(lanes):
        errors.append("roadmap index repeats a process-package test name")
    if len({lane.label for lane in lanes}) != len(lanes):
        errors.append("roadmap index repeats a process-package label")
    return lanes, errors


def validate_junit(path: Path, lanes: Sequence[PackageLane]) -> list[str]:
    """Validate the executed process-package names in a CTest JUnit file."""

    if not path.is_file():
        return [f"JUnit artifact does not exist: {path}"]
    if path.stat().st_size == 0:
        return [f"JUnit artifact is empty: {path}"]

    try:
        root = ElementTree.parse(path).getroot()
    except (ElementTree.ParseError, OSError) as error:
        return [f"JUnit artifact is not valid XML ({path}): {error}"]

    if root.tag not in {"testsuite", "testsuites"}:
        return [f"JUnit artifact has unexpected root element: {root.tag}"]

    cases = root.findall(".//testcase")
    if not cases:
        return [f"JUnit artifact contains no testcase elements: {path}"]

    violations: list[str] = []
    names = [case.get("name", "") for case in cases]
    name_counts = {name: names.count(name) for name in set(names) if name}
    expected_names = [lane.test_name for lane in lanes]
    for test_name in expected_names:
        count = name_counts.get(test_name, 0)
        if count != 1:
            violations.append(
                f"JUnit artifact contains {count} entries for indexed package test {test_name}"
            )

    root_failures = root.get("failures", "0")
    root_errors = root.get("errors", "0")
    if root_failures not in {"", "0"}:
        violations.append(f"JUnit artifact reports failures={root_failures}")
    if root_errors not in {"", "0"}:
        violations.append(f"JUnit artifact reports errors={root_errors}")
    for case in cases:
        name = case.get("name", "<unnamed>")
        if case.find("failure") is not None:
            violations.append(f"JUnit testcase failed: {name}")
        if case.find("error") is not None:
            violations.append(f"JUnit testcase errored: {name}")
    return violations


def main() -> int:
    arguments = parse_arguments()
    ctest_path = Path(arguments.ctest)
    if not ctest_path.is_file():
        resolved_ctest = shutil.which(arguments.ctest)
        if resolved_ctest is None:
            print(
                f"CTest executable does not exist or is not on PATH: {arguments.ctest}",
                file=sys.stderr,
            )
            return 2
        ctest_path = Path(resolved_ctest)
    if not arguments.test_dir.is_dir():
        print(
            f"CTest test directory does not exist: {arguments.test_dir}",
            file=sys.stderr,
        )
        return 2

    try:
        index = load_json(arguments.index)
        catalog = load_catalog(ctest_path, arguments.test_dir, arguments.config)
        lanes, violations = expected_lanes(index)
    except (TypeError, ValueError) as error:
        print(f"verify_process_package_lanes: {error}", file=sys.stderr)
        return 2

    catalog_by_name = {test.name: test for test in catalog}
    package_tests = {
        name: test
        for name, test in catalog_by_name.items()
        if name.startswith("umbra_rti_package_process_")
    }
    expected_names = {lane.test_name for lane in lanes}
    expected_labels = {lane.label for lane in lanes}
    lane_handles = index.lane_handles
    configured_package_labels = set(lane_handles)
    if configured_package_labels != expected_labels:
        missing = sorted(expected_labels - configured_package_labels)
        unexpected = sorted(configured_package_labels - expected_labels)
        if missing:
            violations.append(
                "ROADMAP-INDEX lane_handles is missing package labels: "
                + ", ".join(missing)
            )
        if unexpected:
            violations.append(
                "ROADMAP-INDEX lane_handles has unindexed package labels: "
                + ", ".join(unexpected)
            )
    for lane in lanes:
        handle = lane_handles.get(lane.label)
        if handle is None:
            violations.append(
                f"ROADMAP-INDEX lane_handles has no entry for package label {lane.label}"
            )
            continue
        if handle.consumer_test != lane.test_name:
            violations.append(
                f"ROADMAP-INDEX lane_handles[{lane.label}] consumer_test does not match {lane.test_name}"
            )
        if handle.ctest_label != lane.label:
            violations.append(
                f"ROADMAP-INDEX lane_handles[{lane.label}] ctest_label does not match {lane.label}"
            )
    actual_labels = {
        label
        for test in package_tests.values()
        for label in labels_for(test)
        if label.startswith("package-process")
    }
    if set(package_tests) != expected_names:
        missing = sorted(expected_names - package_tests.keys())
        unexpected = sorted(package_tests.keys() - expected_names)
        if missing:
            violations.append(
                f"CTest catalog is missing package tests: {', '.join(missing)}"
            )
        if unexpected:
            violations.append(
                f"CTest catalog has unindexed package tests: {', '.join(unexpected)}"
            )
    if actual_labels != expected_labels:
        missing = sorted(expected_labels - actual_labels)
        unexpected = sorted(actual_labels - expected_labels)
        if missing:
            violations.append(
                f"CTest catalog is missing package labels: {', '.join(missing)}"
            )
        if unexpected:
            violations.append(
                f"CTest catalog has unindexed package labels: {', '.join(unexpected)}"
            )

    if arguments.junit is not None:
        violations.extend(validate_junit(arguments.junit, lanes))

    summaries: list[str] = []
    for lane in lanes:
        test = catalog_by_name.get(lane.test_name)
        if test is None:
            continue
        labels = labels_for(test)
        if lane.label not in labels:
            violations.append(
                f"{lane.test_name} is missing its indexed label {lane.label}"
            )
        selected = [
            candidate for candidate in catalog if lane.label in labels_for(candidate)
        ]
        if len(selected) != 1:
            violations.append(
                f"indexed package label {lane.label} selects {len(selected)} tests; expected exactly one"
            )
        summaries.append(f"  {lane.label}: {lane.test_name}")

    if violations:
        print("Installed-profile process package catalog violations:", file=sys.stderr)
        print("\n".join(f"  {violation}" for violation in violations), file=sys.stderr)
        return 1

    print(
        "Verified installed-profile process package catalog against ROADMAP-INDEX.json:"
    )
    print("\n".join(summaries))
    if arguments.junit is not None:
        print(f"Verified process-package JUnit artifact: {arguments.junit}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
