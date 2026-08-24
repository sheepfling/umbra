"""Validate and export the provider-neutral Java RTI TCK evidence.

The Java runner owns behavioral execution.  This tool owns the stable
traceability join: local TCK scenario IDs -> standard Java API signatures ->
Requirements Lab IDs from Umbra's pinned compliance contracts -> provider
result/evidence artifacts.  It does not import or execute a provider.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
CATALOG = ROOT / "compliance" / "catalogs" / "java-tck-scenario-catalog.json"
DEFAULT_BUNDLE = ROOT / ".compliance" / "corpus-bundle.json"
DEFAULT_OUTPUT = ROOT / ".compliance" / "java-tck-coverage.json"


def load_json(path: Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8"))


def load_catalog(path: Path = CATALOG) -> dict[str, Any]:
    catalog = load_json(path)
    if catalog.get("schema_version") != 1 or catalog.get("kind") != "portable-java-tck-catalog":
        raise ValueError(f"unsupported Java TCK catalog: {path}")
    scenarios = catalog.get("scenarios")
    if not isinstance(scenarios, list) or not scenarios:
        raise ValueError("Java TCK catalog must contain scenarios")
    ids = [item.get("id") for item in scenarios]
    if any(not isinstance(item, str) or not item for item in ids) or len(ids) != len(set(ids)):
        raise ValueError("Java TCK scenario IDs must be unique non-empty strings")
    for scenario in scenarios:
        for field in ("category", "title", "java_test", "default_status"):
            if not isinstance(scenario.get(field), str) or not scenario[field]:
                raise ValueError(f"{scenario.get('id', '<unknown>')} missing {field}")
        if not scenario.get("api_methods"):
            raise ValueError(f"{scenario['id']} must link standard Java API methods")
        if not scenario.get("requirement_ids"):
            raise ValueError(f"{scenario['id']} must have stable requirement IDs")
        if not scenario.get("contract_refs"):
            raise ValueError(f"{scenario['id']} must reference compliance contracts")
        if scenario["default_status"] not in {"run", "unsupported", "not applicable"}:
            raise ValueError(f"{scenario['id']} has invalid default_status")
        for ref in scenario["contract_refs"]:
            if not (ROOT / "compliance" / "requirements-lab" / ref).is_file():
                raise ValueError(f"{scenario['id']} references absent contract {ref}")
    return catalog


def contract_requirements(refs: list[str]) -> list[dict[str, Any]]:
    records: list[dict[str, Any]] = []
    for ref in refs:
        data = load_json(ROOT / "compliance" / "requirements-lab" / ref)
        if ref.endswith("-requirements-contract.json"):
            for requirement in data.get("requirements", []):
                records.append(
                    {
                        "id": requirement.get("id"),
                        "requirements_lab_requirement_id": requirement.get(
                            "requirements_lab_requirement_id"
                        ),
                        "clause_id": requirement.get("clause_id"),
                        "contract": f"compliance/requirements-lab/{ref}",
                    }
                )
        else:
            for mapping in data.get("mappings", []):
                records.append(
                    {
                        "id": mapping.get("id"),
                        "requirements_lab_requirement_id": None,
                        "clause_id": None,
                        "contract": f"compliance/requirements-lab/{ref}",
                    }
                )
    unique: dict[str, dict[str, Any]] = {}
    for record in records:
        if record["id"]:
            unique.setdefault(record["id"], record)
    return sorted(unique.values(), key=lambda item: item["id"])


def java_surfaces(bundle_path: Path) -> list[dict[str, Any]]:
    if not bundle_path.is_file():
        return []
    bundle = load_json(bundle_path)
    document = next(
        (item for item in bundle.get("documents", []) if item.get("document_id") == "hla-1516.1-2025"),
        None,
    )
    if document is None:
        raise ValueError("Requirements Lab bundle has no hla-1516.1-2025 document")
    return [item for item in document.get("api_surfaces", []) if item.get("language") == "java"]


def lab_requirement_ids(bundle_path: Path) -> set[str]:
    if not bundle_path.is_file():
        return set()
    bundle = load_json(bundle_path)
    document = next(
        (item for item in bundle.get("documents", []) if item.get("document_id") == "hla-1516.1-2025"),
        None,
    )
    if document is None:
        return set()
    return {
        item.get("id")
        for item in document.get("requirements", [])
        if isinstance(item, dict) and isinstance(item.get("id"), str)
    }


def scenario_for_surface(surface: dict[str, Any], scenarios: list[dict[str, Any]]) -> dict[str, Any]:
    name = surface.get("name", "")
    owner = surface.get("owner", "")
    candidates = [f"{owner}.{name}", name]
    for scenario in scenarios:
        methods = scenario.get("api_methods", [])
        if "*" in methods or any(candidate in methods for candidate in candidates):
            return scenario
    # The inventory scenario is an explicit catch-all for standard declarations
    # not yet assigned to a behavioral family.
    return next(item for item in scenarios if item["id"] == "java-tck.api-surface-inventory")


def load_results(paths: list[Path]) -> list[dict[str, Any]]:
    results: list[dict[str, Any]] = []
    for path in paths:
        data = load_json(path)
        if data.get("kind") != "java-rti-tck-evidence":
            raise ValueError(f"not Java TCK evidence: {path}")
        results.append({"path": str(path), "data": data})
    return results


def validate_sources() -> list[str]:
    findings: list[str] = []
    source_root = ROOT / "packages" / "umbra-rti-java-tck" / "src" / "main" / "java"
    for path in source_root.rglob("*.java"):
        text = path.read_text(encoding="utf-8")
        if "import org.umbra" in text or "import umbra." in text:
            findings.append(f"portable Java TCK imports provider implementation: {path}")
        if "NativeBridge" in text or "JNIEnv" in text or "#include" in text:
            findings.append(f"portable Java TCK contains native implementation coupling: {path}")
    return findings


def validate(catalog_path: Path, bundle_path: Path) -> dict[str, Any]:
    catalog = load_catalog(catalog_path)
    findings = validate_sources()
    scenarios = catalog["scenarios"]
    surfaces = java_surfaces(bundle_path)
    unmapped: list[str] = []
    for surface in surfaces:
        selected = scenario_for_surface(surface, scenarios)
        if not selected:
            unmapped.append(surface.get("id", "<unknown>"))
    if unmapped:
        findings.append("unmapped Java API surfaces: " + ", ".join(unmapped))
    return {
        "valid": not findings,
        "findings": findings,
        "scenario_count": len(scenarios),
        "requirements_lab_java_api_surface_count": len(surfaces),
        "portable_exclusions": catalog.get("exclusions", []),
    }


def export(
    catalog_path: Path,
    bundle_path: Path,
    results_paths: list[Path],
    output: Path,
    jpype_results_paths: list[Path] | None = None,
) -> dict[str, Any]:
    catalog = load_catalog(catalog_path)
    if not bundle_path.is_file():
        raise ValueError(
            f"Requirements Lab bundle is absent: {bundle_path}; run tools/requirements_lab.py export first"
        )
    validation = validate(catalog_path, bundle_path)
    if not validation["valid"]:
        raise ValueError("; ".join(validation["findings"]))
    scenarios = catalog["scenarios"]
    results = load_results(results_paths)
    scenario_by_id = {item["id"]: item for item in scenarios}
    provider_scenario_statuses: dict[str, list[dict[str, Any]]] = {}
    provider_results: list[dict[str, Any]] = []
    evidence: list[dict[str, Any]] = []
    for result in results:
        data = result["data"]
        provider = data.get("provider", "unknown")
        scenario_results = {item["id"]: item for item in data.get("scenarios", [])}
        for scenario_id in scenario_by_id:
            provider_scenario_statuses.setdefault(scenario_id, []).append(
                {
                    "provider": provider,
                    "status": scenario_results.get(scenario_id, {}).get(
                        "status", "not applicable"
                    ),
                    "artifact": result["path"],
                }
            )
        provider_results.append(
            {
                "provider": provider,
                "artifact": result["path"],
                "summary": {
                    "pass": sum(item.get("status") == "pass" for item in scenario_results.values()),
                    "fail": sum(item.get("status") == "fail" for item in scenario_results.values()),
                    "unsupported": sum(item.get("status") == "unsupported" for item in scenario_results.values()),
                    "not_applicable": sum(item.get("status") == "not applicable" for item in scenario_results.values()),
                },
            }
        )
        for scenario_id, scenario in scenario_by_id.items():
            item = scenario_results.get(scenario_id)
            status = item.get("status", "not applicable") if item else "not applicable"
            evidence.append(
                {
                    "id": f"java-tck.{provider}.{scenario_id}",
                    "kind": "integration-test",
                    "status": "passed" if status == "pass" else "failed" if status == "fail" else "blocked",
                    "execution_mode": "real" if status in {"pass", "fail"} else "unknown",
                    "provider": provider,
                    "artifact": result["path"],
                    "scenario_id": scenario_id,
                    "requirement_ids": scenario["requirement_ids"],
                    "api_methods": scenario["api_methods"],
                    "notes": item.get("message", "No provider result was supplied") if item else "No provider result was supplied",
                }
            )
    requirement_records: list[dict[str, Any]] = []
    tck_requirement_records: list[dict[str, Any]] = []
    api_surface_records: list[dict[str, Any]] = []
    known_lab_requirement_ids = lab_requirement_ids(bundle_path)
    for scenario in scenarios:
        lab_records = contract_requirements(scenario["contract_refs"])
        linked_lab_ids = sorted(
            {
                item["requirements_lab_requirement_id"]
                for item in lab_records
                if item.get("requirements_lab_requirement_id")
            }
        )
        for stable_id in scenario["requirement_ids"]:
            tck_requirement_records.append(
                {
                    "id": stable_id,
                    "tck_scenario_id": scenario["id"],
                    "clause_id": None,
                    "requirements_lab_requirement_ids": (
                        [stable_id]
                        if stable_id in known_lab_requirement_ids
                        else linked_lab_ids
                    ),
                    "standard_api_methods": scenario["api_methods"],
                    "provider_statuses": provider_scenario_statuses.get(scenario["id"], []),
                    "exclusion": scenario.get("exclusion"),
                }
            )
        for requirement in lab_records:
            requirement_records.append(
                {
                    **requirement,
                    "tck_scenario_id": scenario["id"],
                    "stable_requirement_ids": scenario["requirement_ids"],
                    "standard_api_methods": scenario["api_methods"],
                    "provider_statuses": provider_scenario_statuses.get(scenario["id"], []),
                    "exclusion": scenario.get("exclusion"),
                }
            )
    for surface in java_surfaces(bundle_path):
        scenario = scenario_for_surface(surface, scenarios)
        api_surface_records.append(
            {
                "requirements_lab_api_surface_id": surface.get("id"),
                "owner": surface.get("owner"),
                "language": surface.get("language"),
                "signature": surface.get("signature"),
                "tck_scenario_id": scenario["id"],
                "standard_api_method": f"{surface.get('owner')}.{surface.get('name')}",
            }
        )
    artifact = {
        "schema_version": 1,
        "kind": "java-rti-tck-compliance-export",
        "document_id": "hla-1516.1-2025",
        "standard": "IEEE 1516.1-2025",
        "catalog": str(catalog_path),
        "requirements_lab_bundle": str(bundle_path) if bundle_path.is_file() else None,
        "providers": provider_results,
        "scenarios": scenarios,
        "api_surfaces": api_surface_records,
        "requirements": tck_requirement_records,
        "compliance_records": requirement_records,
        "evidence": evidence,
        "jpype_evidence": [load_json(path) for path in (jpype_results_paths or [])],
        "exclusions": catalog.get("exclusions", []),
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(artifact, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    return artifact


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=("validate", "export"))
    parser.add_argument("--catalog", type=Path, default=CATALOG)
    parser.add_argument("--bundle", type=Path, default=DEFAULT_BUNDLE)
    parser.add_argument("--results", type=Path, action="append", default=[])
    parser.add_argument("--jpype-results", type=Path, action="append", default=[])
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    args = parser.parse_args()
    if args.command == "validate":
        validation = validate(args.catalog, args.bundle)
        print(json.dumps(validation, indent=2))
        return 0 if validation["valid"] else 2
    if not args.results:
        parser.error("export requires at least one --results artifact")
    artifact = export(args.catalog, args.bundle, args.results, args.output, args.jpype_results)
    print(json.dumps({"output": str(args.output), "providers": artifact["providers"]}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
