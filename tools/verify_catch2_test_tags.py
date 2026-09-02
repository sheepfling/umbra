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
from typing import Any


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


def test_location(test: dict[str, Any]) -> str:
    location = test.get("source-location", {})
    filename = location.get("filename", "<unknown source>")
    line = location.get("line", "?")
    return f"{filename}:{line}"


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
        report = json.loads(result.stdout)
        tests = report["listings"]["tests"]
    except (json.JSONDecodeError, KeyError, TypeError) as error:
        print(f"Could not read the Catch2 JSON test listing: {error}", file=sys.stderr)
        return 2

    violations: list[str] = []
    for test in tests:
        tags = frozenset(test.get("tags", ()))
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
                "expected at least one domain tag from " f"{sorted(DOMAIN_TAGS)}"
            )
        if problems:
            violations.append(
                f"  {test_location(test)}: {test.get('name', '<unnamed test>')}\n"
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
