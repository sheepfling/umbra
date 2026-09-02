"""Export provider-neutral Python IEEE 1516e smoke results with Lab traceability."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any

from verify_1516e_python_tck_results import verify


ROOT = Path(__file__).resolve().parents[1]
CATALOG = ROOT / "compliance" / "catalogs" / "python-2010-tck-scenario-catalog.json"
DEFAULT_OUTPUT = ROOT / ".compliance" / "python-2010-tck-coverage.json"


def load(path: Path) -> dict[str, Any]:
    value = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise ValueError(f"{path} must contain a JSON object")
    return value


def export(catalog_path: Path, result_path: Path, output: Path, provider: str) -> dict[str, Any]:
    findings = verify(catalog_path, result_path)
    if findings:
        raise ValueError("; ".join(findings))
    catalog = load(catalog_path)
    result = load(result_path)
    transport = result.get("transport", "python-jpype")
    result_by_id = {item["id"]: item for item in result["scenarios"]}
    evidence = []
    for scenario in catalog["scenarios"]:
        item = result_by_id[scenario["id"]]
        status = item["status"]
        evidence.append(
            {
                "id": f"python-2010-tck.{provider}.{scenario['id']}",
                "kind": "integration-test",
                "status": "passed" if status == "pass" else "failed" if status == "fail" else "blocked",
                "execution_mode": "real",
                "provider": provider,
                "transport": transport,
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
        "kind": "python-2010-rti-tck-compliance-export",
        "document_id": "hla-1516.1-2010",
        "standard": "IEEE 1516.1-2010",
        "transport": transport,
        "catalog": str(catalog_path),
        "result": str(result_path),
        "capability_profile": result.get("capability_profile", "default"),
        "provider": provider,
        "scenarios": catalog["scenarios"],
        "evidence": evidence,
        "summary": result["summary"],
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
