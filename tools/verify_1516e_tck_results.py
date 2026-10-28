"""Verify a Java 1516e TCK result artifact against its portable catalog."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import TypeAlias, cast

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_CATALOG = (
    ROOT / "compliance" / "catalogs" / "java-2010-tck-scenario-catalog.json"
)

JsonScalar: TypeAlias = str | int | float | bool | None
JsonValue: TypeAlias = JsonScalar | list["JsonValue"] | dict[str, "JsonValue"]
JsonObject: TypeAlias = dict[str, JsonValue]


def _load(path: Path) -> JsonObject:
    with path.open(encoding="utf-8") as stream:
        decoded: object = json.load(stream)
    if not isinstance(decoded, dict) or not all(
        isinstance(key, str) for key in decoded
    ):
        raise ValueError(f"{path} must contain a JSON object")
    return cast(JsonObject, decoded)


def _scenario_ids(
    scenarios: list[JsonValue], *, label: str, findings: list[str]
) -> list[str]:
    identifiers: list[str] = []
    for index, value in enumerate(scenarios):
        if not isinstance(value, dict) or not all(
            isinstance(key, str) for key in value
        ):
            findings.append(f"{label} scenario {index} must be an object")
            continue
        scenario = cast(JsonObject, value)
        identifier = scenario.get("id")
        if not isinstance(identifier, str) or not identifier:
            findings.append(f"{label} scenario {index} must have a non-empty string id")
            continue
        identifiers.append(identifier)
    return identifiers


def verify(catalog_path: Path, result_path: Path) -> list[str]:
    catalog = _load(catalog_path)
    result = _load(result_path)
    findings: list[str] = []
    if catalog.get("standard") != "IEEE 1516.1-2010":
        findings.append("catalog standard must be IEEE 1516.1-2010")
    if result.get("standard") != "IEEE 1516.1-2010":
        findings.append("result standard must be IEEE 1516.1-2010")
    if "capability_profile" in result and not isinstance(
        result["capability_profile"], str
    ):
        findings.append("result capability_profile must be a string")
    expected = catalog.get("scenarios")
    actual = result.get("scenarios")
    if not isinstance(expected, list) or not isinstance(actual, list):
        return findings + ["catalog and result scenarios must be arrays"]
    expected_ids = _scenario_ids(expected, label="catalog", findings=findings)
    actual_ids = _scenario_ids(actual, label="result", findings=findings)
    if actual_ids != expected_ids:
        findings.append(
            "result scenario IDs/order differ from catalog: "
            f"expected {expected_ids!r}, got {actual_ids!r}"
        )
    for item in actual:
        if not isinstance(item, dict):
            findings.append("result contains a non-object scenario")
            continue
        scenario_record = cast(JsonObject, item)
        raw_scenario_id = scenario_record.get("id", "<unknown>")
        scenario_id = (
            raw_scenario_id if isinstance(raw_scenario_id, str) else "<unknown>"
        )
        status = scenario_record.get("status")
        if not isinstance(status, str) or status not in {
            "pass",
            "fail",
            "unsupported",
            "not applicable",
        }:
            findings.append(f"{scenario_id}: invalid status {status!r}")
        category = scenario_record.get("category")
        if not isinstance(category, str) or not category:
            findings.append(f"{scenario_id}: missing category")
        if not isinstance(scenario_record.get("message", ""), str):
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
