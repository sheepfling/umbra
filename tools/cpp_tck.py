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
EXPECTED_ADAPTER_MANAGED_SKIPS = {
    (
        "cpp-tck.connection-loss-cleanup",
        "requires an adapter-managed connection-loss fixture",
    )
}
STANDARD_HEADERS = {
    "algorithm",
    "chrono",
    "cmath",
    "condition_variable",
    "cstdint",
    "cstdlib",
    "exception",
    "filesystem",
    "fstream",
    "future",
    "functional",
    "iostream",
    "limits",
    "map",
    "memory",
    "mutex",
    "optional",
    "set",
    "sstream",
    "stdexcept",
    "string",
    "thread",
    "type_traits",
    "vector",
}
OFFICIAL_API_HEADERS = {
    "RTI/Enums.h",
    "RTI/Exception.h",
    "RTI/FederateAmbassador.h",
    "RTI/NullFederateAmbassador.h",
    "RTI/Handle.h",
    "RTI/RTI1516.h",
    "RTI/RTIambassador.h",
    "RTI/RTIambassadorFactory.h",
    "RTI/RangeBounds.h",
    "RTI/RtiConfiguration.h",
    "RTI/Typedefs.h",
    "RTI/VariableLengthData.h",
    "RTI/auth/AuthorizationResult.h",
    "RTI/auth/Authorizer.h",
    "RTI/auth/AuthorizerFactory.h",
    "RTI/auth/Credentials.h",
    "RTI/auth/HLAnoCredentials.h",
    "RTI/auth/HLAauthorizerFactoryFactory.h",
    "RTI/auth/HLAplainTextPassword.h",
    "RTI/encoding/BasicDataElements.h",
    "RTI/encoding/EncodingExceptions.h",
    "RTI/encoding/HLAextendableVariantRecord.h",
    "RTI/encoding/HLAfixedArray.h",
    "RTI/encoding/HLAfixedRecord.h",
    "RTI/encoding/HLAlogicalTime.h",
    "RTI/encoding/HLAlogicalTimeInterval.h",
    "RTI/encoding/HLAopaqueData.h",
    "RTI/encoding/HLAvariableArray.h",
    "RTI/encoding/HLAvariantRecord.h",
    "RTI/time/LogicalTime.h",
    "RTI/time/LogicalTimeFactory.h",
    "RTI/time/LogicalTimeInterval.h",
    "RTI/time/HLAfloat64Interval.h",
    "RTI/time/HLAfloat64Time.h",
    "RTI/time/HLAfloat64TimeFactory.h",
    "RTI/time/HLAinteger64Interval.h",
    "RTI/time/HLAinteger64Time.h",
    "RTI/time/HLAinteger64TimeFactory.h",
    "RTI/time/HLAlogicalTimeFactoryFactory.h",
    "RTI/libfedtime/LogicalTimeFactoryFactory.h",
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
    if not isinstance(catalog.get("promotion_rule"), str) or not catalog["promotion_rule"]:
        raise ValueError("C++ TCK catalog must document its promotion rule")
    parity_path = ROOT / parity_catalog
    parity = load_json(parity_path)
    if parity.get("kind") != "portable-java-tck-catalog":
        raise ValueError(f"unsupported parity catalog: {parity_path}")
    parity_ids = {
        item.get("id") for item in parity.get("scenarios", [])
        if isinstance(item, dict)
    }
    for scenario in scenarios:
        for field in (
            "category",
            "title",
            "cpp_test",
            "runner_id",
            "default_status",
            "promotion",
        ):
            if not isinstance(scenario.get(field), str) or not scenario[field]:
                raise ValueError(f"{scenario.get('id', '<unknown>')} missing {field}")
        if not scenario.get("api_methods"):
            raise ValueError(f"{scenario['id']} must link standard C++ API methods")
        if not scenario.get("requirement_ids"):
            raise ValueError(f"{scenario['id']} must have stable requirement IDs")
        if not scenario.get("contract_refs"):
            raise ValueError(f"{scenario['id']} must reference compliance contracts")
        native_equivalents = scenario.get("native_equivalent_runner_ids", [])
        if not isinstance(native_equivalents, list) or any(
            not isinstance(item, str) or not item for item in native_equivalents
        ):
            raise ValueError(
                f"{scenario['id']} has invalid native equivalent runner IDs"
            )
        if scenario["default_status"] not in {"run", "adapter-required", "unsupported"}:
            raise ValueError(f"{scenario['id']} has invalid default_status")
        if scenario["promotion"] not in {"promoted", "candidate"}:
            raise ValueError(f"{scenario['id']} has invalid promotion")
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
    represented_java_ids = {
        scenario["id"]
        for scenario in scenarios
        if scenario["id"].startswith("java-tck.")
    }
    for scenario in scenarios:
        parity_id = scenario.get("parity")
        if isinstance(parity_id, str) and parity_id.startswith("java-tck."):
            if parity_id not in parity_ids:
                raise ValueError(
                    f"{scenario['id']} references absent Java parity scenario {parity_id}"
                )
            represented_java_ids.add(parity_id)
    missing_java_ids = sorted(
        scenario["id"]
        for scenario in parity.get("scenarios", [])
        if scenario.get("default_status") == "run"
        and scenario.get("id") not in represented_java_ids
    )
    if missing_java_ids:
        raise ValueError(
            "Java TCK run scenarios are missing from the C++ parity catalog: "
            + ", ".join(missing_java_ids)
        )
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
    fom_directory = SOURCE_ROOT / "fom"
    required_fom_names = (
        "p0-tck.xml",
        "fom-model-tck.xml",
        "ddm-tck.xml",
        "ddm-multi-attribute-tck.xml",
        "ddm-three-dimensional-tck.xml",
        "switches-tck.xml",
        "fom-extension-tck.xml",
        "invalid-malformed-tck.xml",
        "invalid-namespace-tck.xml",
        "invalid-duplicate-tck.xml",
    )
    for name in required_fom_names:
        fom = fom_directory / name
        if not fom.is_file():
            findings.append(f"portable C++ TCK FOM is absent: {fom}")
    for fom_path in sorted(fom_directory.glob("*.xml")):
        if any(token in fom_path.read_text(encoding="utf-8").lower() for token in forbidden_tokens):
            findings.append(
                f"portable C++ TCK FOM contains a forbidden provider token: {fom_path}"
            )
    return findings


def validate(catalog_path: Path = DEFAULT_CATALOG) -> dict[str, Any]:
    catalog = load_catalog(catalog_path)
    findings = validate_sources()
    promotion_counts = {
        state: sum(1 for item in catalog["scenarios"] if item["promotion"] == state)
        for state in ("promoted", "candidate")
    }
    return {
        "valid": not findings,
        "findings": findings,
        "scenario_count": len(catalog["scenarios"]),
        "promotion_counts": promotion_counts,
        "source_boundary": catalog["source_boundary"],
    }


def is_expected_adapter_skip(result: dict[str, Any]) -> bool:
    return (
        result.get("status"),
        result.get("id"),
        result.get("message"),
    ) in {
        ("skipped", scenario_id, message)
        for scenario_id, message in EXPECTED_ADAPTER_MANAGED_SKIPS
    }


def validate_results(
    path: Path,
    catalog: dict[str, Any],
    promotion: str | None = None,
) -> list[str]:
    data = load_json(path)
    findings: list[str] = []
    if data.get("kind") != "hla-rti-cpp-tck-evidence":
        findings.append(f"not C++ TCK evidence: {path}")
    records = data.get("results")
    if not isinstance(records, list):
        return ["evidence results must be an array"]
    selected = [
        item
        for item in catalog["scenarios"]
        if promotion is None or item["promotion"] == promotion
    ]
    expected = {item["runner_id"] for item in selected}
    actual = {item.get("id") for item in records}
    missing = sorted(expected - actual)
    if missing:
        findings.append("evidence is missing scenarios: " + ", ".join(missing))
    unexpected = sorted(actual - expected)
    if unexpected:
        findings.append("evidence contains unexpected scenarios: " + ", ".join(unexpected))
    seen: set[tuple[Any, Any]] = set()
    for result in records:
        if result.get("status") not in {"passed", "skipped", "failed"}:
            findings.append(f"evidence has invalid result status: {result}")
        if result.get("callback_model") not in {"evoked", "immediate"}:
            findings.append(f"evidence has invalid callback model: {result}")
        if result.get("id") not in expected:
            continue
        key = (result.get("id"), result.get("callback_model"))
        if key in seen:
            findings.append(f"evidence contains duplicate scenario result: {key}")
        seen.add(key)
    expected_keys = {
        (scenario_id, callback_model)
        for scenario_id in expected
        for callback_model in ("evoked", "immediate")
    }
    missing_keys = sorted(expected_keys - seen)
    if missing_keys:
        findings.append(
            "evidence is missing scenario/callback results: "
            + ", ".join(f"{scenario}/{callback}" for scenario, callback in missing_keys)
        )
    if any(result.get("status") == "failed" for result in records):
        findings.append("evidence contains failed scenarios")
    if promotion == "promoted" and any(
        result.get("status") != "passed"
        and not is_expected_adapter_skip(result)
        for result in records
        if result.get("id") in expected
    ):
        findings.append("promoted evidence cannot contain skipped or failed scenarios")
    return findings


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--catalog", type=Path, default=DEFAULT_CATALOG)
    parser.add_argument("--results", type=Path)
    parser.add_argument(
        "--promotion",
        choices=("all", "promoted", "candidate"),
        default="all",
        help="Evidence set to validate; promoted requires passing evoked and immediate results",
    )
    arguments = parser.parse_args()
    try:
        result = validate(arguments.catalog)
        if arguments.results:
            catalog = load_catalog(arguments.catalog)
            selected_promotion = None if arguments.promotion == "all" else arguments.promotion
            result["findings"].extend(
                validate_results(arguments.results, catalog, selected_promotion)
            )
            result["valid"] = not result["findings"]
        print(json.dumps(result, indent=2, sort_keys=True))
        return 0 if result["valid"] else 1
    except (OSError, ValueError, json.JSONDecodeError) as error:
        print(json.dumps({"valid": False, "findings": [str(error)]}, indent=2))
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
