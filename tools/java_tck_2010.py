"""Validate and export IEEE 1516e (2010) Java TCK evidence.

The Java runner owns execution.  This small, edition-specific joiner keeps the
2010 result shape independent of the 2025 TCK exporter while linking every
scenario to the pinned 2010 catalog and Requirements Lab identifiers.
"""

from __future__ import annotations

import argparse
import json
from dataclasses import dataclass
from pathlib import Path
from types import NoneType
from typing import Literal, TypeAlias, TypedDict, cast

ROOT = Path(__file__).resolve().parents[1]
CATALOG = ROOT / "compliance" / "catalogs" / "java-2010-tck-scenario-catalog.json"
DEFAULT_OUTPUT = ROOT / ".compliance" / "java-2010-tck-coverage.json"


JsonScalar: TypeAlias = NoneType | bool | int | float | str
JsonValue: TypeAlias = JsonScalar | list["JsonValue"] | dict[str, "JsonValue"]
JsonObject: TypeAlias = dict[str, JsonValue]
_ScenarioStatus: TypeAlias = Literal["pass", "fail", "unsupported", "not applicable"]
_EvidenceStatus: TypeAlias = Literal["passed", "failed", "blocked"]


@dataclass(frozen=True)
class _CatalogScenario:
    id: str
    requirement_ids: tuple[str, ...]
    mapping_ids: tuple[str, ...]
    transition_ids: tuple[str, ...]
    api_methods: tuple[str, ...]
    source: JsonObject


@dataclass(frozen=True)
class _Catalog:
    standard: str
    scenarios: tuple[_CatalogScenario, ...]


@dataclass(frozen=True)
class _ScenarioResult:
    id: str
    status: _ScenarioStatus
    message: str | None


@dataclass(frozen=True)
class _RunResult:
    standard: str
    scenarios: tuple[_ScenarioResult, ...]
    capability_profile: str | None


class _TckEvidence(TypedDict):
    id: str
    kind: Literal["integration-test"]
    status: _EvidenceStatus
    execution_mode: Literal["real"]
    provider: str
    artifact: str
    scenario_id: str
    requirement_ids: list[str]
    mapping_ids: list[str]
    transition_ids: list[str]
    api_methods: list[str]
    notes: str | None


_TckSummary = TypedDict(
    "_TckSummary",
    {
        "pass": int,
        "fail": int,
        "unsupported": int,
        "not_applicable": int,
    },
)


class _ComplianceArtifact(TypedDict):
    schema_version: Literal[1]
    kind: Literal["java-2010-rti-tck-compliance-export"]
    document_id: Literal["hla-1516.1-2010"]
    standard: Literal["IEEE 1516.1-2010"]
    catalog: str
    result: str
    capability_profile: str | None
    provider: str
    scenarios: list[JsonObject]
    evidence: list[_TckEvidence]
    summary: _TckSummary


def load(path: Path) -> JsonObject:
    value: object = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict) or not all(isinstance(key, str) for key in value):
        raise TypeError(f"{path} must contain a JSON object with string keys")
    return cast(JsonObject, value)


def _string_tuple(value: JsonValue, field: str, context: str) -> tuple[str, ...]:
    if not isinstance(value, list):
        raise TypeError(f"{context} field {field} must be a string array")
    strings: list[str] = []
    for item in value:
        if not isinstance(item, str):
            raise TypeError(f"{context} field {field} must be a string array")
        strings.append(item)
    return tuple(strings)


def _catalog_scenario(value: JsonValue, index: int) -> _CatalogScenario:
    if not isinstance(value, dict):
        raise TypeError(f"catalog scenario {index} must be a JSON object")
    scenario_id = value.get("id")
    if not isinstance(scenario_id, str):
        raise TypeError(f"catalog scenario {index} must have a string id")
    context = f"catalog scenario {index}"
    return _CatalogScenario(
        id=scenario_id,
        requirement_ids=_string_tuple(
            value.get("requirements_lab_requirement_ids", []),
            "requirements_lab_requirement_ids",
            context,
        ),
        mapping_ids=_string_tuple(
            value.get("requirements_lab_mapping_ids", []),
            "requirements_lab_mapping_ids",
            context,
        ),
        transition_ids=_string_tuple(
            value.get("transition_ids", []), "transition_ids", context
        ),
        api_methods=_string_tuple(value.get("api_methods", []), "api_methods", context),
        source=cast(JsonObject, value),
    )


def _result_scenario(value: JsonValue, index: int) -> _ScenarioResult:
    if not isinstance(value, dict):
        raise TypeError(f"result scenario {index} must be a JSON object")
    scenario_id = value.get("id")
    if not isinstance(scenario_id, str):
        raise TypeError(f"result scenario {index} must have a string id")
    status = value.get("status")
    if not isinstance(status, str):
        raise TypeError(f"result scenario {index} status must be a string")
    if status not in {
        "pass",
        "fail",
        "unsupported",
        "not applicable",
    }:
        raise ValueError(f"result scenario {index} has an invalid status")
    message = value.get("message", "")
    if message is not None and not isinstance(message, str):
        raise TypeError(f"result scenario {index} message must be a string or null")
    return _ScenarioResult(scenario_id, cast(_ScenarioStatus, status), message)


def validate(catalog_path: Path, result_path: Path) -> tuple[_Catalog, _RunResult]:
    catalog = load(catalog_path)
    result = load(result_path)
    findings: list[str] = []
    if catalog.get("standard") != "IEEE 1516.1-2010":
        findings.append("catalog standard is not IEEE 1516.1-2010")
    if result.get("standard") != "IEEE 1516.1-2010":
        findings.append("result standard is not IEEE 1516.1-2010")
    scenarios = catalog.get("scenarios")
    results = result.get("scenarios")
    if not isinstance(scenarios, list) or not isinstance(results, list):
        raise TypeError("catalog and result must contain scenario arrays")
    catalog_scenarios = tuple(
        _catalog_scenario(item, index) for index, item in enumerate(scenarios)
    )
    result_scenarios = tuple(
        _result_scenario(item, index) for index, item in enumerate(results)
    )
    capability_profile = result.get("capability_profile", "default")
    if capability_profile is not None and not isinstance(capability_profile, str):
        raise TypeError("result capability_profile must be a string or null")
    expected_ids = [item.id for item in catalog_scenarios]
    result_ids = [item.id for item in result_scenarios]
    if result_ids != expected_ids:
        findings.append(
            f"scenario IDs/order differ: expected {expected_ids!r}, got {result_ids!r}"
        )
    if findings:
        raise ValueError("; ".join(findings))
    return (
        _Catalog(cast(str, catalog.get("standard")), catalog_scenarios),
        _RunResult(
            cast(str, result.get("standard")),
            result_scenarios,
            cast(str | None, capability_profile),
        ),
    )


def export(
    catalog_path: Path, result_path: Path, output: Path, provider: str
) -> _ComplianceArtifact:
    catalog, result = validate(catalog_path, result_path)
    result_by_id = {item.id: item for item in result.scenarios}
    evidence: list[_TckEvidence] = []
    for scenario in catalog.scenarios:
        item = result_by_id[scenario.id]
        status: _EvidenceStatus = (
            "passed"
            if item.status == "pass"
            else "failed"
            if item.status == "fail"
            else "blocked"
        )
        evidence.append(
            {
                "id": f"java-2010-tck.{provider}.{scenario.id}",
                "kind": "integration-test",
                "status": status,
                "execution_mode": "real",
                "provider": provider,
                "artifact": str(result_path),
                "scenario_id": scenario.id,
                "requirement_ids": list(scenario.requirement_ids),
                "mapping_ids": list(scenario.mapping_ids),
                "transition_ids": list(scenario.transition_ids),
                "api_methods": list(scenario.api_methods),
                "notes": item.message,
            }
        )
    artifact: _ComplianceArtifact = {
        "schema_version": 1,
        "kind": "java-2010-rti-tck-compliance-export",
        "document_id": "hla-1516.1-2010",
        "standard": "IEEE 1516.1-2010",
        "catalog": str(catalog_path),
        "result": str(result_path),
        "capability_profile": result.capability_profile,
        "provider": provider,
        "scenarios": [scenario.source for scenario in catalog.scenarios],
        "evidence": evidence,
        "summary": {
            "pass": sum(item.status == "pass" for item in result.scenarios),
            "fail": sum(item.status == "fail" for item in result.scenarios),
            "unsupported": sum(
                item.status == "unsupported" for item in result.scenarios
            ),
            "not_applicable": sum(
                item.status == "not applicable" for item in result.scenarios
            ),
        },
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(
        json.dumps(artifact, indent=2, ensure_ascii=False) + "\n", encoding="utf-8"
    )
    return artifact


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("result", type=Path)
    parser.add_argument("--catalog", type=Path, default=CATALOG)
    parser.add_argument("--provider", default="unknown-2010-provider")
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    args = parser.parse_args()
    artifact = export(args.catalog, args.result, args.output, args.provider)
    print(
        json.dumps(
            {"output": str(args.output), "summary": artifact["summary"]}, indent=2
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
