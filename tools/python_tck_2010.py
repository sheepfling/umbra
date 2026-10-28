"""Export provider-neutral Python IEEE 1516e smoke results with Lab traceability."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Literal, TypeAlias, TypedDict, cast

from verify_1516e_python_tck_results import (
    JsonObject,
    JsonValue,
    load_json_object,
    verify,
)

ROOT = Path(__file__).resolve().parents[1]
CATALOG = ROOT / "compliance" / "catalogs" / "python-2010-tck-scenario-catalog.json"
DEFAULT_OUTPUT = ROOT / ".compliance" / "python-2010-tck-coverage.json"


_ScenarioStatus: TypeAlias = Literal["pass", "fail", "unsupported", "not applicable"]
_Transport: TypeAlias = Literal["python-jpype", "python-native"]
_EvidenceStatus: TypeAlias = Literal["passed", "failed", "blocked"]


class _PythonTckEvidence(TypedDict):
    id: str
    kind: Literal["integration-test"]
    status: _EvidenceStatus
    execution_mode: Literal["real"]
    provider: str
    transport: _Transport
    artifact: str
    scenario_id: str
    requirement_ids: list[str]
    mapping_ids: list[str]
    transition_ids: list[str]
    api_methods: list[str]
    notes: str


class _PythonTckArtifact(TypedDict):
    schema_version: Literal[1]
    kind: Literal["python-2010-rti-tck-compliance-export"]
    document_id: Literal["hla-1516.1-2010"]
    standard: Literal["IEEE 1516.1-2010"]
    transport: _Transport
    catalog: str
    result: str
    capability_profile: str
    provider: str
    scenarios: list[JsonObject]
    evidence: list[_PythonTckEvidence]
    summary: JsonObject


def _object_array(value: JsonValue, context: str) -> list[JsonObject]:
    if not isinstance(value, list):
        raise TypeError(f"{context} must be an array")
    records: list[JsonObject] = []
    for index, item in enumerate(value):
        if not isinstance(item, dict):
            raise TypeError(f"{context}[{index}] must be an object")
        records.append(item)
    return records


def _required_string(record: JsonObject, field: str, context: str) -> str:
    value = record.get(field)
    if not isinstance(value, str):
        raise TypeError(f"{context}.{field} must be a string")
    return value


def _string_array(record: JsonObject, field: str, context: str) -> list[str]:
    value = record.get(field, [])
    if not isinstance(value, list):
        raise TypeError(f"{context}.{field} must be a string array")
    strings: list[str] = []
    for index, item in enumerate(value):
        if not isinstance(item, str):
            raise TypeError(f"{context}.{field}[{index}] must be a string")
        strings.append(item)
    return strings


def _scenario_status(record: JsonObject, context: str) -> _ScenarioStatus:
    value = record.get("status")
    if not isinstance(value, str):
        raise TypeError(f"{context}.status must be a string")
    if value not in {"pass", "fail", "unsupported", "not applicable"}:
        raise ValueError(f"{context}.status has an unsupported value: {value!r}")
    return cast(_ScenarioStatus, value)


def _transport(record: JsonObject) -> _Transport:
    value = record.get("transport", "python-jpype")
    if not isinstance(value, str):
        raise TypeError("result.transport must be a string")
    if value not in {"python-jpype", "python-native"}:
        raise ValueError(f"result.transport has an unsupported value: {value!r}")
    return cast(_Transport, value)


def export(
    catalog_path: Path, result_path: Path, output: Path, provider: str
) -> _PythonTckArtifact:
    findings = verify(catalog_path, result_path)
    if findings:
        raise ValueError("; ".join(findings))
    catalog = load_json_object(catalog_path)
    result = load_json_object(result_path)
    transport = _transport(result)
    catalog_scenarios = _object_array(catalog.get("scenarios"), "catalog.scenarios")
    result_scenarios = _object_array(result.get("scenarios"), "result.scenarios")
    result_by_id: dict[str, JsonObject] = {
        _required_string(item, "id", f"result.scenarios[{index}]"): item
        for index, item in enumerate(result_scenarios)
    }
    evidence: list[_PythonTckEvidence] = []
    for index, scenario in enumerate(catalog_scenarios):
        context = f"catalog.scenarios[{index}]"
        scenario_id = _required_string(scenario, "id", context)
        item = result_by_id[scenario_id]
        item_context = f"result scenario {scenario_id}"
        status = _scenario_status(item, item_context)
        evidence_status: _EvidenceStatus = (
            "passed"
            if status == "pass"
            else "failed"
            if status == "fail"
            else "blocked"
        )
        message = item.get("message", "")
        if not isinstance(message, str):
            raise TypeError(f"{item_context}.message must be a string")
        evidence.append(
            {
                "id": f"python-2010-tck.{provider}.{scenario_id}",
                "kind": "integration-test",
                "status": evidence_status,
                "execution_mode": "real",
                "provider": provider,
                "transport": transport,
                "artifact": str(result_path),
                "scenario_id": scenario_id,
                "requirement_ids": _string_array(
                    scenario, "requirements_lab_requirement_ids", context
                ),
                "mapping_ids": _string_array(
                    scenario, "requirements_lab_mapping_ids", context
                ),
                "transition_ids": _string_array(scenario, "transition_ids", context),
                "api_methods": _string_array(scenario, "api_methods", context),
                "notes": message,
            }
        )
    capability_profile = result.get("capability_profile", "default")
    if not isinstance(capability_profile, str):
        raise TypeError("result.capability_profile must be a string")
    summary = result.get("summary")
    if not isinstance(summary, dict):
        raise TypeError("result.summary must be an object")
    artifact: _PythonTckArtifact = {
        "schema_version": 1,
        "kind": "python-2010-rti-tck-compliance-export",
        "document_id": "hla-1516.1-2010",
        "standard": "IEEE 1516.1-2010",
        "transport": transport,
        "catalog": str(catalog_path),
        "result": str(result_path),
        "capability_profile": capability_profile,
        "provider": provider,
        "scenarios": catalog_scenarios,
        "evidence": evidence,
        "summary": summary,
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
