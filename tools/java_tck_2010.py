"""Validate and export IEEE 1516e (2010) Java TCK evidence.

The Java runner owns execution.  This small, edition-specific joiner keeps the
2010 result shape independent from the 2025 TCK exporter while linking every
scenario to the pinned 2010 catalog and Requirements Lab identifiers.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
CATALOG = ROOT / "compliance" / "catalogs" / "java-2010-tck-scenario-catalog.json"
DEFAULT_OUTPUT = ROOT / ".compliance" / "java-2010-tck-coverage.json"


def load(path: Path) -> dict[str, Any]:
    value = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise ValueError(f"{path} must contain a JSON object")
    return value


def validate(catalog_path: Path, result_path: Path) -> tuple[dict[str, Any], dict[str, Any]]:
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
        raise ValueError("catalog and result must contain scenario arrays")
    expected_ids = [item.get("id") for item in scenarios]
    result_ids = [item.get("id") for item in results]
    if result_ids != expected_ids:
        findings.append(f"scenario IDs/order differ: expected {expected_ids!r}, got {result_ids!r}")
    for item in results:
        if not isinstance(item, dict) or item.get("status") not in {
            "pass", "fail", "unsupported", "not applicable"
        }:
            findings.append(f"invalid scenario result: {item!r}")
    if findings:
        raise ValueError("; ".join(findings))
    return catalog, result


def export(catalog_path: Path, result_path: Path, output: Path, provider: str) -> dict[str, Any]:
    catalog, result = validate(catalog_path, result_path)
    result_by_id = {item["id"]: item for item in result["scenarios"]}
    evidence = []
    for scenario in catalog["scenarios"]:
        item = result_by_id[scenario["id"]]
        status = item["status"]
        evidence.append(
            {
                "id": f"java-2010-tck.{provider}.{scenario['id']}",
                "kind": "integration-test",
                "status": "passed" if status == "pass" else "failed" if status == "fail" else "blocked",
                "execution_mode": "real",
                "provider": provider,
                "artifact": str(result_path),
                "scenario_id": scenario["id"],
                "requirement_ids": scenario.get("requirements_lab_requirement_ids", []),
                "mapping_ids": scenario.get("requirements_lab_mapping_ids", []),
                "transition_ids": scenario.get("transition_ids", []),
                "api_methods": scenario.get("api_methods", []),
                "notes": item.get("message", ""),
            }
        )
    artifact = {
        "schema_version": 1,
        "kind": "java-2010-rti-tck-compliance-export",
        "document_id": "hla-1516.1-2010",
        "standard": "IEEE 1516.1-2010",
        "catalog": str(catalog_path),
        "result": str(result_path),
        "capability_profile": result.get("capability_profile", "default"),
        "provider": provider,
        "scenarios": catalog["scenarios"],
        "evidence": evidence,
        "summary": {
            "pass": sum(item["status"] == "pass" for item in result["scenarios"]),
            "fail": sum(item["status"] == "fail" for item in result["scenarios"]),
            "unsupported": sum(item["status"] == "unsupported" for item in result["scenarios"]),
            "not_applicable": sum(item["status"] == "not applicable" for item in result["scenarios"]),
        },
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(artifact, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    return artifact


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("result", type=Path)
    parser.add_argument("--catalog", type=Path, default=CATALOG)
    parser.add_argument("--provider", default="unknown-2010-provider")
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    args = parser.parse_args()
    artifact = export(args.catalog, args.result, args.output, args.provider)
    print(json.dumps({"output": str(args.output), "summary": artifact["summary"]}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
