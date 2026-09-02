"""Verify a Java 1516e TCK result artifact against its portable catalog."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_CATALOG = ROOT / "compliance" / "catalogs" / "java-2010-tck-scenario-catalog.json"


def _load(path: Path) -> dict[str, Any]:
    with path.open(encoding="utf-8") as stream:
        value = json.load(stream)
    if not isinstance(value, dict):
        raise ValueError(f"{path} must contain a JSON object")
    return value


def verify(catalog_path: Path, result_path: Path) -> list[str]:
    catalog = _load(catalog_path)
    result = _load(result_path)
    findings: list[str] = []
    if catalog.get("standard") != "IEEE 1516.1-2010":
        findings.append("catalog standard must be IEEE 1516.1-2010")
    if result.get("standard") != "IEEE 1516.1-2010":
        findings.append("result standard must be IEEE 1516.1-2010")
    if "capability_profile" in result and not isinstance(result["capability_profile"], str):
        findings.append("result capability_profile must be a string")
    expected = catalog.get("scenarios")
    actual = result.get("scenarios")
    if not isinstance(expected, list) or not isinstance(actual, list):
        return findings + ["catalog and result scenarios must be arrays"]
    expected_ids = [item.get("id") for item in expected if isinstance(item, dict)]
    actual_ids = [item.get("id") for item in actual if isinstance(item, dict)]
    if actual_ids != expected_ids:
        findings.append(
            "result scenario IDs/order differ from catalog: "
            f"expected {expected_ids!r}, got {actual_ids!r}"
        )
    for item in actual:
        if not isinstance(item, dict):
            findings.append("result contains a non-object scenario")
            continue
        scenario_id = item.get("id", "<unknown>")
        if item.get("status") not in {"pass", "fail", "unsupported", "not applicable"}:
            findings.append(f"{scenario_id}: invalid status {item.get('status')!r}")
        if not isinstance(item.get("category"), str) or not item["category"]:
            findings.append(f"{scenario_id}: missing category")
        if not isinstance(item.get("message", ""), str):
            findings.append(f"{scenario_id}: message must be a string")
    return findings


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("result", type=Path)
    parser.add_argument("--catalog", type=Path, default=DEFAULT_CATALOG)
    args = parser.parse_args()
    try:
        findings = verify(args.catalog, args.result)
    except (OSError, ValueError, json.JSONDecodeError) as error:
        print(f"IEEE 1516e TCK result verification: FAIL: {error}", file=sys.stderr)
        return 1
    if findings:
        print("IEEE 1516e TCK result verification: FAIL", file=sys.stderr)
        for finding in findings:
            print(f"  {finding}", file=sys.stderr)
        return 1
    print("IEEE 1516e TCK result verification: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
