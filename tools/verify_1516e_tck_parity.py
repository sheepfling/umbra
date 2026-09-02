"""Verify that the 1516e Java and Python TCK catalogs remain transplantable.

The two transports intentionally have different prefixes and a small number
of Python-only probes. Shared provider scenarios must retain the same (or a
superset of) Requirements Lab references on the Python side so a Java test can
be transplanted without silently losing normative coverage.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_JAVA = ROOT / "compliance" / "catalogs" / "java-2010-tck-scenario-catalog.json"
DEFAULT_PYTHON = ROOT / "compliance" / "catalogs" / "python-2010-tck-scenario-catalog.json"

_PYTHON_ONLY = {
    "2010-tck.complex-encoders",
    "2010-tck.malformed-inputs",
    "2010-tck.logical-time-arithmetic",
}
_ALIASES = {"2010-tck.ddm": "2010-tck.ddm-management"}


def _load(path: Path) -> dict[str, Any]:
    value = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise ValueError(f"{path} must contain a JSON object")
    return value


def _scenario_map(catalog: dict[str, Any], prefix: str) -> dict[str, dict[str, Any]]:
    scenarios = catalog.get("scenarios")
    if not isinstance(scenarios, list) or not scenarios:
        raise ValueError("catalog scenarios must be a non-empty array")
    result: dict[str, dict[str, Any]] = {}
    for scenario in scenarios:
        if not isinstance(scenario, dict) or not isinstance(scenario.get("id"), str):
            raise ValueError("catalog contains a scenario without a string id")
        scenario_id = scenario["id"]
        if not scenario_id.startswith(prefix):
            raise ValueError(f"scenario {scenario_id!r} does not use {prefix!r}")
        suffix = scenario_id[len(prefix) :]
        if suffix in result:
            raise ValueError(f"duplicate normalized scenario id {suffix!r}")
        result[suffix] = scenario
    return result


def verify(java_path: Path, python_path: Path) -> dict[str, Any]:
    java = _load(java_path)
    python = _load(python_path)
    if java.get("standard") != "IEEE 1516.1-2010" or python.get("standard") != "IEEE 1516.1-2010":
        raise ValueError("both catalogs must describe IEEE 1516.1-2010")
    java_scenarios = _scenario_map(java, "java-")
    python_scenarios = _scenario_map(python, "python-")
    findings: list[str] = []

    for java_id, java_scenario in java_scenarios.items():
        python_id = _ALIASES.get(java_id, java_id)
        python_scenario = python_scenarios.get(python_id)
        if python_scenario is None:
            findings.append(f"Java scenario {java_id} has no Python transplant")
            continue
        if java_scenario.get("category") != python_scenario.get("category"):
            findings.append(f"{java_id}: Java/Python categories differ")
        for field in (
            "requirements_lab_requirement_ids",
            "requirements_lab_mapping_ids",
            "transition_ids",
        ):
            java_values = set(java_scenario.get(field, []))
            python_values = set(python_scenario.get(field, []))
            missing = sorted(java_values - python_values)
            if missing:
                findings.append(f"{java_id}: Python drops {field} {missing}")

    actual_python_only = set(python_scenarios) - set(_ALIASES.values()) - set(java_scenarios)
    unexpected = sorted(actual_python_only - _PYTHON_ONLY)
    if unexpected:
        findings.append(f"unexpected Python-only scenarios: {unexpected}")

    return {
        "standard": "IEEE 1516.1-2010",
        "java_scenario_count": len(java_scenarios),
        "python_scenario_count": len(python_scenarios),
        "python_only_scenarios": sorted(_PYTHON_ONLY),
        "aliases": dict(_ALIASES),
        "findings": findings,
        "status": "fail" if findings else "pass",
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--java-catalog", type=Path, default=DEFAULT_JAVA)
    parser.add_argument("--python-catalog", type=Path, default=DEFAULT_PYTHON)
    args = parser.parse_args()
    try:
        result = verify(args.java_catalog, args.python_catalog)
    except (OSError, ValueError, json.JSONDecodeError) as error:
        print(f"IEEE 1516e TCK parity verification: FAIL: {error}", file=sys.stderr)
        return 1
    if result["findings"]:
        print("IEEE 1516e TCK parity verification: FAIL", file=sys.stderr)
        for finding in result["findings"]:
            print(f"  {finding}", file=sys.stderr)
        return 1
    print("IEEE 1516e TCK parity verification: PASS")
    print(
        f"  Java scenarios={result['java_scenario_count']}; "
        f"Python scenarios={result['python_scenario_count']}; "
        f"Python-only probes={len(result['python_only_scenarios'])}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
