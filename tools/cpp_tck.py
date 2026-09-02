"""Validate the portable C++ IEEE 1516.1-2025 TCK boundary and evidence."""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_CATALOG = ROOT / "compliance" / "catalogs" / "cpp-tck-scenario-catalog.json"
SOURCE_ROOT = ROOT / "packages" / "hla-rti-cpp-tck"
STANDARD_HEADERS = {
    "algorithm",
    "chrono",
    "cstdint",
    "cstdlib",
    "exception",
    "filesystem",
    "fstream",
    "future",
    "functional",
    "iostream",
    "map",
    "memory",
    "mutex",
    "optional",
    "set",
    "sstream",
    "stdexcept",
    "string",
    "thread",
    "vector",
}
OFFICIAL_API_HEADERS = {
    "RTI/Enums.h",
    "RTI/FederateAmbassador.h",
    "RTI/RTIambassador.h",
    "RTI/RTIambassadorFactory.h",
    "RTI/RtiConfiguration.h",
    "RTI/VariableLengthData.h",
}


def load_json(path: Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8"))


def load_catalog(path: Path) -> dict[str, Any]:
    catalog = load_json(path)
    if catalog.get("schema_version") != 1 or catalog.get("kind") != "portable-cpp-tck-catalog":
        raise ValueError(f"unsupported C++ TCK catalog: {path}")
    scenarios = catalog.get("scenarios")
    if not isinstance(scenarios, list) or not scenarios:
        raise ValueError("C++ TCK catalog must contain scenarios")
    ids = [item.get("id") for item in scenarios]
    if any(not isinstance(item, str) or not item for item in ids) or len(ids) != len(set(ids)):
        raise ValueError("C++ TCK scenario IDs must be unique non-empty strings")
    parity_catalog = catalog.get("parity_catalog")
    if not isinstance(parity_catalog, str) or not parity_catalog:
        raise ValueError("C++ TCK catalog must name its cross-language parity catalog")
    parity_path = ROOT / parity_catalog
    parity = load_json(parity_path)
    if parity.get("kind") != "portable-java-tck-catalog":
        raise ValueError(f"unsupported parity catalog: {parity_path}")
    parity_ids = {
        item.get("id") for item in parity.get("scenarios", [])
        if isinstance(item, dict)
    }
    for scenario in scenarios:
        for field in ("category", "title", "cpp_test", "runner_id", "default_status"):
            if not isinstance(scenario.get(field), str) or not scenario[field]:
                raise ValueError(f"{scenario.get('id', '<unknown>')} missing {field}")
        if not scenario.get("api_methods"):
            raise ValueError(f"{scenario['id']} must link standard C++ API methods")
        if not scenario.get("requirement_ids"):
            raise ValueError(f"{scenario['id']} must have stable requirement IDs")
        if not scenario.get("contract_refs"):
            raise ValueError(f"{scenario['id']} must reference compliance contracts")
        if scenario["default_status"] not in {"run", "adapter-required", "unsupported"}:
            raise ValueError(f"{scenario['id']} has invalid default_status")
        for reference in scenario["contract_refs"]:
            contract = ROOT / "compliance" / "requirements-lab" / reference
            if not contract.is_file():
                raise ValueError(f"{scenario['id']} references absent contract {reference}")
        source, separator, symbol = scenario["cpp_test"].partition("#")
        if not separator or not symbol:
            raise ValueError(f"{scenario['id']} cpp_test must be source#symbol")
        source_path = SOURCE_ROOT / source
        if not source_path.is_file():
            raise ValueError(f"{scenario['id']} references absent source {source}")
        if symbol not in source_path.read_text(encoding="utf-8"):
            raise ValueError(f"{scenario['id']} references absent source symbol {symbol}")
        if scenario["id"].startswith("java-tck."):
            if scenario["id"] not in parity_ids:
                raise ValueError(
                    f"{scenario['id']} is not present in the Java TCK parity catalog"
                )
            if scenario["runner_id"] != scenario["id"]:
                raise ValueError(
                    f"{scenario['id']} must reuse its Java TCK ID as runner_id"
                )
        elif scenario["id"].startswith("cpp-tck.") and not scenario.get("parity"):
            raise ValueError(f"{scenario['id']} must explain its parity extension status")
    return catalog


def validate_sources() -> list[str]:
    findings: list[str] = []
    source = SOURCE_ROOT / "src"
    if not source.is_dir():
        return [f"portable C++ TCK source root is absent: {source}"]
    forbidden_tokens = (
        "umbra",
        "he" + "lios",
        "internal/",
        "jni",
        "nativebridge",
        "catch2",
    )
    include_pattern = re.compile(r"^\s*#include\s+([<\"])([^>\"]+)[>\"]\s*$")
    for path in source.rglob("*"):
        if path.suffix.lower() not in {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp"}:
            continue
        text = path.read_text(encoding="utf-8")
        lowered = text.lower()
        for token in forbidden_tokens:
            if token in lowered:
                findings.append(f"portable C++ TCK contains forbidden token {token!r}: {path}")
        for line_number, line in enumerate(text.splitlines(), start=1):
            match = include_pattern.match(line)
            if match is None:
                continue
            delimiter, include = match.groups()
            if delimiter == '"':
                findings.append(f"portable C++ TCK uses a local include at {path}:{line_number}: {include}")
            elif include not in OFFICIAL_API_HEADERS and include not in STANDARD_HEADERS:
                findings.append(f"portable C++ TCK includes a non-standard header at {path}:{line_number}: {include}")
    fom = SOURCE_ROOT / "fom" / "p0-tck.xml"
    if not fom.is_file():
        findings.append(f"portable C++ TCK FOM is absent: {fom}")
    elif any(token in fom.read_text(encoding="utf-8").lower() for token in forbidden_tokens):
        findings.append(f"portable C++ TCK FOM contains a forbidden provider token: {fom}")
    return findings


def validate(catalog_path: Path = DEFAULT_CATALOG) -> dict[str, Any]:
    catalog = load_catalog(catalog_path)
    findings = validate_sources()
    return {
        "valid": not findings,
        "findings": findings,
        "scenario_count": len(catalog["scenarios"]),
        "source_boundary": catalog["source_boundary"],
    }


def validate_results(path: Path, catalog: dict[str, Any]) -> list[str]:
    data = load_json(path)
    findings: list[str] = []
    if data.get("kind") != "hla-rti-cpp-tck-evidence":
        findings.append(f"not C++ TCK evidence: {path}")
    records = data.get("results")
    if not isinstance(records, list):
        return ["evidence results must be an array"]
    expected = {item["runner_id"] for item in catalog["scenarios"]}
    actual = {item.get("id") for item in records}
    missing = sorted(expected - actual)
    if missing:
        findings.append("evidence is missing scenarios: " + ", ".join(missing))
    seen: set[tuple[Any, Any]] = set()
    for result in records:
        if result.get("status") not in {"passed", "skipped", "failed"}:
            findings.append(f"evidence has invalid result status: {result}")
        key = (result.get("id"), result.get("callback_model"))
        if key in seen:
            findings.append(f"evidence contains duplicate scenario result: {key}")
        seen.add(key)
    if any(result.get("status") == "failed" for result in records):
        findings.append("evidence contains failed scenarios")
    return findings


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--catalog", type=Path, default=DEFAULT_CATALOG)
    parser.add_argument("--results", type=Path)
    arguments = parser.parse_args()
    try:
        result = validate(arguments.catalog)
        if arguments.results:
            catalog = load_catalog(arguments.catalog)
            result["findings"].extend(validate_results(arguments.results, catalog))
            result["valid"] = not result["findings"]
        print(json.dumps(result, indent=2, sort_keys=True))
        return 0 if result["valid"] else 1
    except (OSError, ValueError, json.JSONDecodeError) as error:
        print(json.dumps({"valid": False, "findings": [str(error)]}, indent=2))
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
