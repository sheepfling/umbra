"""Verify that the 1516e Java and Python TCK catalogs remain transplantable.

The two transports intentionally have different prefixes and a small number
of Python-only probes. Shared provider scenarios must retain the same (or a
superset of) Requirements Lab references on the Python side so a Java test can
be transplanted without silently losing normative coverage.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Literal, TypedDict, cast

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_JAVA = ROOT / "compliance" / "catalogs" / "java-2010-tck-scenario-catalog.json"
DEFAULT_PYTHON = (
    ROOT / "compliance" / "catalogs" / "python-2010-tck-scenario-catalog.json"
)

_PYTHON_ONLY = frozenset(
    {
        "2010-tck.complex-encoders",
        "2010-tck.malformed-inputs",
        "2010-tck.logical-time-arithmetic",
    }
)
_ALIASES = (("2010-tck.ddm", "2010-tck.ddm-management"),)


class _CatalogInput(TypedDict, total=False):
    standard: object
    scenarios: object


class _ScenarioInput(TypedDict, total=False):
    id: object
    category: object
    requirements_lab_requirement_ids: object
    requirements_lab_mapping_ids: object
    transition_ids: object


class _Scenario(TypedDict):
    category: str
    requirements_lab_requirement_ids: list[str]
    requirements_lab_mapping_ids: list[str]
    transition_ids: list[str]


class _ParityResult(TypedDict):
    standard: Literal["IEEE 1516.1-2010"]
    java_scenario_count: int
    python_scenario_count: int
    python_only_scenarios: list[str]
    aliases: dict[str, str]
    findings: list[str]
    status: Literal["fail", "pass"]


def _required_string(value: object, *, context: str) -> str:
    if not isinstance(value, str) or not value:
        raise ValueError(f"{context} must be a non-empty string")
    return value


def _string_ids(value: object, *, context: str) -> list[str]:
    if not isinstance(value, list):
        raise TypeError(f"{context} must be an array of string IDs")
    result: list[str] = []
    for index, item in enumerate(value):
        if not isinstance(item, str) or not item:
            raise ValueError(f"{context}[{index}] must be a non-empty string ID")
        result.append(item)
    if len(result) != len(set(result)):
        raise ValueError(f"{context} contains duplicate IDs")
    return result


def _load(path: Path) -> _CatalogInput:
    try:
        value: object = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise ValueError(f"cannot read catalog {path}: {error}") from error
    if not isinstance(value, dict):
        raise TypeError(f"{path} must contain a JSON object")
    return cast(_CatalogInput, value)


def _scenario_map(catalog: _CatalogInput, prefix: str) -> dict[str, _Scenario]:
    scenarios = catalog.get("scenarios")
    if not isinstance(scenarios, list) or not scenarios:
        raise ValueError("catalog scenarios must be a non-empty array")
    result: dict[str, _Scenario] = {}
    for index, value in enumerate(scenarios):
        context = f"catalog.scenarios[{index}]"
        if not isinstance(value, dict):
            raise TypeError(f"{context} must be a JSON object")
        scenario = cast(_ScenarioInput, value)
        scenario_id = _required_string(scenario.get("id"), context=f"{context}.id")
        if not scenario_id.startswith(prefix):
            raise ValueError(f"scenario {scenario_id!r} does not use {prefix!r}")
        suffix = scenario_id[len(prefix) :]
        if not suffix:
            raise ValueError(f"scenario {scenario_id!r} has an empty normalized ID")
        if suffix in result:
            raise ValueError(f"duplicate normalized scenario id {suffix!r}")
        transition_ids = scenario.get("transition_ids")
        result[suffix] = {
            "category": _required_string(
                scenario.get("category"), context=f"{context}.category"
            ),
            "requirements_lab_requirement_ids": _string_ids(
                scenario.get("requirements_lab_requirement_ids"),
                context=f"{context}.requirements_lab_requirement_ids",
            ),
            "requirements_lab_mapping_ids": _string_ids(
                scenario.get("requirements_lab_mapping_ids"),
                context=f"{context}.requirements_lab_mapping_ids",
            ),
            # The Java catalog uses null for three scenarios without transitions.
            "transition_ids": []
            if transition_ids is None
            else _string_ids(transition_ids, context=f"{context}.transition_ids"),
        }
    return result


def verify(java_path: Path, python_path: Path) -> _ParityResult:
    java = _load(java_path)
    python = _load(python_path)
    java_standard = _required_string(
        java.get("standard"), context="Java catalog.standard"
    )
    python_standard = _required_string(
        python.get("standard"), context="Python catalog.standard"
    )
    if java_standard != "IEEE 1516.1-2010" or python_standard != "IEEE 1516.1-2010":
        raise ValueError("both catalogs must describe IEEE 1516.1-2010")
    java_scenarios = _scenario_map(java, "java-")
    python_scenarios = _scenario_map(python, "python-")
    findings: list[str] = []
    aliases = dict(_ALIASES)

    for java_id, java_scenario in java_scenarios.items():
        python_id = aliases.get(java_id, java_id)
        python_scenario = python_scenarios.get(python_id)
        if python_scenario is None:
            findings.append(f"Java scenario {java_id} has no Python transplant")
            continue
        if java_scenario.get("category") != python_scenario.get("category"):
            findings.append(f"{java_id}: Java/Python categories differ")
        for field, java_ids, python_ids in (
            (
                "requirements_lab_requirement_ids",
                java_scenario["requirements_lab_requirement_ids"],
                python_scenario["requirements_lab_requirement_ids"],
            ),
            (
                "requirements_lab_mapping_ids",
                java_scenario["requirements_lab_mapping_ids"],
                python_scenario["requirements_lab_mapping_ids"],
            ),
            (
                "transition_ids",
                java_scenario["transition_ids"],
                python_scenario["transition_ids"],
            ),
        ):
            java_values = set(java_ids)
            python_values = set(python_ids)
            missing = sorted(java_values - python_values)
            if missing:
                findings.append(f"{java_id}: Python drops {field} {missing}")

    actual_python_only = (
        set(python_scenarios) - set(aliases.values()) - set(java_scenarios)
    )
    unexpected = sorted(actual_python_only - _PYTHON_ONLY)
    if unexpected:
        findings.append(f"unexpected Python-only scenarios: {unexpected}")

    return {
        "standard": "IEEE 1516.1-2010",
        "java_scenario_count": len(java_scenarios),
        "python_scenario_count": len(python_scenarios),
        "python_only_scenarios": sorted(_PYTHON_ONLY),
        "aliases": aliases,
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
    except (OSError, TypeError, ValueError, json.JSONDecodeError) as error:
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
