"""Verify a Python IEEE 1516e (JPype or native) smoke result against its catalog."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from types import NoneType
from typing import TypeAlias, cast

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_CATALOG = (
    ROOT / "compliance" / "catalogs" / "python-2010-tck-scenario-catalog.json"
)

JsonScalar: TypeAlias = NoneType | bool | int | float | str
JsonValue: TypeAlias = JsonScalar | list["JsonValue"] | dict[str, "JsonValue"]
JsonObject: TypeAlias = dict[str, JsonValue]


def load_json_object(path: Path) -> JsonObject:
    value: object = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict) or not all(isinstance(key, str) for key in value):
        raise ValueError(f"{path} must contain a JSON object")
    return cast(JsonObject, value)


def verify(catalog_path: Path, result_path: Path) -> list[str]:
    catalog = load_json_object(catalog_path)
    result = load_json_object(result_path)
    findings: list[str] = []
    if catalog.get("kind") != "portable-python-tck-catalog":
        findings.append("catalog kind must be portable-python-tck-catalog")
    if catalog.get("standard") != "IEEE 1516.1-2010":
        findings.append("catalog standard must be IEEE 1516.1-2010")
    if result.get("standard") != "IEEE 1516.1-2010":
        findings.append("result standard must be IEEE 1516.1-2010")
    transport = result.get("transport")
    if not isinstance(transport, str) or transport not in {
        "python-jpype",
        "python-native",
    }:
        findings.append("result transport must be python-jpype or python-native")
    if not isinstance(result.get("capability_profile", "default"), str):
        findings.append("result capability_profile must be a string")
    expected = catalog.get("scenarios")
    actual = result.get("scenarios")
    if not isinstance(expected, list) or not isinstance(actual, list):
        return findings + ["catalog and result scenarios must be arrays"]

    expected_ids: list[str] = []
    for index, item in enumerate(expected):
        if not isinstance(item, dict):
            findings.append(f"catalog contains a non-object scenario at index {index}")
            continue
        scenario_id = item.get("id")
        if not isinstance(scenario_id, str):
            findings.append(f"catalog scenario {index} must have a string id")
            continue
        expected_ids.append(scenario_id)
        for field in (
            "requirements_lab_requirement_ids",
            "requirements_lab_mapping_ids",
            "transition_ids",
            "api_methods",
        ):
            values = item.get(field, [])
            if not isinstance(values, list) or not all(
                isinstance(value, str) for value in values
            ):
                findings.append(f"{scenario_id}: {field} must be a string array")

    actual_ids: list[str] = []
    actual_rows: list[JsonObject] = []
    for index, item in enumerate(actual):
        if not isinstance(item, dict):
            findings.append(f"result contains a non-object scenario at index {index}")
            continue
        actual_rows.append(item)
        scenario_id = item.get("id")
        if not isinstance(scenario_id, str):
            findings.append(f"result scenario {index} must have a string id")
        else:
            actual_ids.append(scenario_id)
    if actual_ids != expected_ids:
        findings.append(
            f"scenario IDs/order differ: expected {expected_ids!r}, got {actual_ids!r}"
        )
    if len(expected_ids) != len(set(expected_ids)):
        findings.append("catalog scenario IDs must be unique")
    if len(actual_ids) != len(set(actual_ids)):
        findings.append("result scenario IDs must be unique")
    statuses = {"pass", "fail", "unsupported", "not applicable"}
    for item in actual_rows:
        scenario_id = item.get("id", "<unknown>")
        status = item.get("status")
        if not isinstance(status, str) or status not in statuses:
            findings.append(f"{scenario_id}: invalid status {item.get('status')!r}")
        category = item.get("category")
        if not isinstance(category, str) or not category:
            findings.append(f"{scenario_id}: missing category")
        if not isinstance(item.get("message", ""), str):
            findings.append(f"{scenario_id}: message must be a string")
    summary = result.get("summary")
    if not isinstance(summary, dict):
        findings.append("result summary must be an object")
    else:
        for status in ("pass", "fail", "unsupported"):
            expected_count = sum(item.get("status") == status for item in actual_rows)
            if summary.get(status) != expected_count:
                findings.append(f"summary.{status} does not match scenario statuses")
    overall_status = result.get("status")
    if not isinstance(overall_status, str) or overall_status not in {"pass", "fail"}:
        findings.append(
            f"result status must be pass or fail, got {result.get('status')!r}"
        )
    return findings


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("result", type=Path)
    parser.add_argument("--catalog", type=Path, default=DEFAULT_CATALOG)
    args = parser.parse_args()
    try:
        findings = verify(args.catalog, args.result)
    except (OSError, ValueError, json.JSONDecodeError) as error:
        print(
            f"IEEE 1516e Python TCK result verification: FAIL: {error}", file=sys.stderr
        )
        return 1
    if findings:
        print("IEEE 1516e Python TCK result verification: FAIL", file=sys.stderr)
        for finding in findings:
            print(f"  {finding}", file=sys.stderr)
        return 1
    print("IEEE 1516e Python TCK result verification: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
