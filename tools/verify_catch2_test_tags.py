#!/usr/bin/env python3
"""Verify that every Umbra Catch2 test belongs to focused test lanes.

Catch2 tags feed directly into CTest labels.  Keeping this check beside the
test executable makes a missing scope or domain tag a visible failure instead
of silently excluding a new scenario from focused development lanes.
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path
from typing import TypedDict

SCOPE_TAGS = frozenset(("unit", "integration"))
DOMAIN_TAGS = frozenset(
    (
        "callbacks",
        "foundation",
        "federation-management",
        "time-management",
        "ddm",
        "mom",
        "service-reporting",
        "fom",
        "object-management",
        "ownership-management",
        "declaration-management",
        "interaction-management",
        "save-restore",
    )
)


class _Catch2Test(TypedDict):
    name: str
    tags: list[str]
    filename: str
    line: int | str


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Verify the required execution-scope and domain tags on Catch2 tests."
    )
    parser.add_argument(
        "--catch2",
        required=True,
        type=Path,
        help="Path to the Umbra Catch2 test executable.",
    )
    return parser.parse_args()


def _test_rows(report: object) -> list[_Catch2Test]:
    if not isinstance(report, dict):
        raise TypeError("root must be an object")
    listings = report.get("listings")
    if not isinstance(listings, dict):
        raise TypeError("listings must be an object")
    tests = listings.get("tests")
    if not isinstance(tests, list):
        raise TypeError("listings.tests must be an array")

    rows: list[_Catch2Test] = []
    for index, value in enumerate(tests):
        if not isinstance(value, dict):
            raise TypeError(f"listings.tests[{index}] must be an object")
        name = value.get("name", "<unnamed test>")
        tags = value.get("tags", [])
        location = value.get("source-location", {})
        if not isinstance(name, str):
            raise TypeError(f"listings.tests[{index}].name must be a string")
        if not isinstance(tags, list) or any(not isinstance(tag, str) for tag in tags):
            raise TypeError(f"listings.tests[{index}].tags must be a string array")
        if not isinstance(location, dict):
            raise TypeError(
                f"listings.tests[{index}].source-location must be an object"
            )
        filename = location.get("filename", "<unknown source>")
        line = location.get("line", "?")
        if (
            not isinstance(filename, str)
            or not isinstance(line, (int, str))
            or isinstance(line, bool)
        ):
            raise TypeError(
                f"listings.tests[{index}].source-location has invalid filename or line"
            )
        rows.append(
            {
                "name": name,
                "tags": tags,
                "filename": filename,
                "line": line,
            }
        )
    return rows


def test_location(test: _Catch2Test) -> str:
    return f"{test['filename']}:{test['line']}"


def main() -> int:
    arguments = parse_arguments()
    executable = arguments.catch2
    if not executable.is_file():
        print(f"Catch2 executable does not exist: {executable}", file=sys.stderr)
        return 2

    result = subprocess.run(
        [str(executable), "--list-tests", "--reporter", "json"],
        check=False,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    if result.returncode != 0:
        print("Catch2 test listing failed:", file=sys.stderr)
        if result.stderr:
            print(result.stderr.rstrip(), file=sys.stderr)
        return result.returncode

    try:
        report: object = json.loads(result.stdout)
        tests = _test_rows(report)
    except (json.JSONDecodeError, TypeError) as error:
        print(f"Could not read the Catch2 JSON test listing: {error}", file=sys.stderr)
        return 2

    violations: list[str] = []
    for test in tests:
        tags = frozenset(test["tags"])
        scopes = tags & SCOPE_TAGS
        domains = tags & DOMAIN_TAGS
        problems: list[str] = []
        if len(scopes) != 1:
            problems.append(
                "expected exactly one execution-scope tag "
                f"from {sorted(SCOPE_TAGS)}, found {sorted(scopes)}"
            )
        if not domains:
            problems.append(
                f"expected at least one domain tag from {sorted(DOMAIN_TAGS)}"
            )
        if problems:
            violations.append(
                f"  {test_location(test)}: {test['name']}\n"
                f"    tags: {sorted(tags)}\n"
                f"    {'; '.join(problems)}"
            )

    if violations:
        print("Catch2 focused-lane tag taxonomy violations:", file=sys.stderr)
        print("\n".join(violations), file=sys.stderr)
        return 1

    print(
        f"Verified focused-lane tags for {len(tests)} Catch2 test cases "
        f"({len(SCOPE_TAGS)} execution scopes, {len(DOMAIN_TAGS)} domain lanes)."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
