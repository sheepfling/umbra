#!/usr/bin/env python3
"""Verify Java TCK scenario IDs are represented by the portable C++ catalog."""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_CATALOG = ROOT / "compliance" / "catalogs" / "cpp-tck-scenario-catalog.json"
DEFAULT_JAVA_MAIN = (
    ROOT
    / "packages"
    / "hla-rti-java-tck"
    / "src"
    / "main"
    / "java"
    / "org"
    / "hla"
    / "rti"
    / "tck"
    / "RtiTckMain.java"
)
SCENARIO_ID = re.compile(r'new\s+Scenario\s*\(\s*"(java-tck\.[a-z0-9-]+)"')


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--catalog", type=Path, default=DEFAULT_CATALOG)
    parser.add_argument("--java-main", type=Path, default=DEFAULT_JAVA_MAIN)
    parser.add_argument(
        "--report-cpp-gaps",
        action="store_true",
        help="list unpaired C++ TCK scenarios and explicit not-applicable dispositions",
    )
    arguments = parser.parse_args()

    try:
        catalog = json.loads(arguments.catalog.read_text(encoding="utf-8"))
        rows = catalog["scenarios"]
        catalog_ids = {row["id"] for row in rows}
        java_source = arguments.java_main.read_text(encoding="utf-8")
    except (OSError, KeyError, TypeError, json.JSONDecodeError) as error:
        print(f"cpp/java parity audit: cannot read inputs: {error}", file=sys.stderr)
        return 2

    java_ids = set(SCENARIO_ID.findall(java_source))
    if arguments.report_cpp_gaps:
        candidates = [
            row
            for row in rows
            if isinstance(row.get("id"), str)
            and row["id"].startswith("cpp-tck.")
            and not row["id"].endswith("-contract")
        ]
        not_applicable = [
            row
            for row in candidates
            if isinstance(row.get("parity"), str)
            and row["parity"].startswith("not-applicable:")
        ]
        unpaired = [
            row for row in candidates
            if not (
                isinstance(row.get("parity"), str)
                and (
                    row["parity"].startswith("java-tck.")
                    or row["parity"].startswith("not-applicable:")
                )
            )
        ]
        print(
            f"cpp/java parity candidates: {len(unpaired)} unpaired; "
            f"{len(not_applicable)} explicitly not applicable"
        )
        for row in unpaired[:40]:
            print(
                f"  {row['id']} [{row.get('category', 'uncategorized')}]: "
                f"{row.get('title', '(untitled)')}"
            )
        if len(unpaired) > 40:
            print(f"  ... {len(unpaired) - 40} additional candidates")
        for row in not_applicable:
            print(f"  not applicable {row['id']}: {row['parity'].split(':', 1)[1].strip()}")

    parity_targets = {
        row["parity"]
        for row in rows
        if isinstance(row.get("parity"), str)
        and row["parity"].startswith("java-tck.")
    }
    missing = sorted(java_ids - catalog_ids - parity_targets)
    stale_catalog_ids = sorted(
        scenario_id
        for scenario_id in catalog_ids - java_ids
        if scenario_id.startswith("java-tck.")
    )
    unknown_parity_targets = sorted(
        {
            row["parity"]
            for row in rows
            if isinstance(row.get("parity"), str)
            and row["parity"].startswith("java-tck.")
            and row["parity"] not in java_ids
        }
    )

    if missing or stale_catalog_ids or unknown_parity_targets:
        print("cpp/java parity audit: failed", file=sys.stderr)
        for label, values in (
            ("Java IDs missing from C++ catalog", missing),
            ("stale Java IDs in C++ catalog", stale_catalog_ids),
            ("unknown Java parity targets", unknown_parity_targets),
        ):
            for value in values:
                print(f"  {label}: {value}", file=sys.stderr)
        return 1

    print(
        f"cpp/java parity audit: {len(java_ids)} Java scenario IDs are represented "
        "by catalog IDs or explicit parity links; all Java parity targets resolve."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
