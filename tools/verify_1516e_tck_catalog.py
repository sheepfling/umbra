"""Verify the portable IEEE 1516e (2010) TCK catalog against the Lab export."""

from __future__ import annotations

import argparse
import ast
import json
import sys
from pathlib import Path
from typing import TypeAlias, cast

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_CATALOG = (
    ROOT / "compliance" / "catalogs" / "java-2010-tck-scenario-catalog.json"
)
DEFAULT_BUNDLE = ROOT / ".compliance" / "corpus-bundle-v0.1.0.a1-all.json"
DEFAULT_PYTHON_SOURCE = ROOT / "packages" / "umbra-rti-api" / "src" / "hla" / "rti1516e"

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


def _python_api_inventory(source_root: Path) -> dict[str, set[str]]:
    """Build a source-only class/method inventory for the 2010 contract.

    The catalog is intentionally checked without importing the package.  This
    keeps the verifier usable in a clean checkout, while still catching a
    scenario that accidentally names a 2025-only method or a stale class.
    ``ClassVar`` metadata and inherited methods are not needed here: every
    standard method named by the catalog is declared by one of the generated
    2010 classes.
    """

    inventory: dict[str, set[str]] = {}
    for path in sorted(source_root.rglob("*.py")):
        tree = ast.parse(path.read_text(encoding="utf-8"), filename=str(path))
        for node in tree.body:
            if not isinstance(node, ast.ClassDef):
                continue
            methods = {
                child.name
                for child in node.body
                if isinstance(child, (ast.FunctionDef, ast.AsyncFunctionDef))
            }
            inventory.setdefault(node.name, set()).update(methods)
    if not inventory:
        raise ValueError(f"2010 Python contract source is empty: {source_root}")
    return inventory


def _api_reference_findings(
    catalog: JsonObject, python_source: Path = DEFAULT_PYTHON_SOURCE
) -> list[str]:
    """Return catalog references that are absent from the Python contract."""

    inventory = _python_api_inventory(python_source)
    findings: list[str] = []
    scenarios = catalog.get("scenarios", [])
    if not isinstance(scenarios, list):
        return findings
    for raw_scenario in scenarios:
        if not isinstance(raw_scenario, dict) or not all(
            isinstance(key, str) for key in raw_scenario
        ):
            continue
        scenario = cast(JsonObject, raw_scenario)
        scenario_id = scenario.get("id", "<unknown scenario>")
        if not isinstance(scenario_id, str):
            scenario_id = "<unknown scenario>"
        references = scenario.get("api_methods", [])
        if not isinstance(references, list):
            continue
        for reference in references:
            if not isinstance(reference, str) or not reference:
                findings.append(
                    f"{scenario_id}: api_methods contains a non-empty string requirement"
                )
                continue

            # A bare type name (for example ``CallbackModel``) is a valid
            # catalog reference even though it has no method suffix.
            if "." not in reference:
                if reference not in inventory:
                    findings.append(
                        f"{scenario_id}: unknown 2010 API class in {reference!r}"
                    )
                continue

            # Fully-qualified exception/type names are type references rather
            # than class-method references, e.g.
            # ``hla.rti1516e.encoding.DecoderException``.
            final_name = reference.rsplit(".", 1)[-1]
            if reference.startswith("hla.rti1516e.") and final_name in inventory:
                continue

            owner, separator, method = reference.partition(".")
            if not separator or owner not in inventory:
                findings.append(
                    f"{scenario_id}: unknown 2010 API class in {reference!r}"
                )
                continue
            if method == "*":
                continue
            if method not in inventory[owner]:
                findings.append(
                    f"{scenario_id}: {owner}.{method} is not declared by the 2010 Python contract"
                )
    return findings


def verify(
    catalog_path: Path,
    bundle_path: Path,
    python_source: Path = DEFAULT_PYTHON_SOURCE,
) -> list[str]:
    catalog = _load(catalog_path)
    bundle = _load(bundle_path)
    findings: list[str] = []
    if catalog.get("standard") != "IEEE 1516.1-2010":
        findings.append("catalog standard must be IEEE 1516.1-2010")
    source_boundary = catalog.get("source_boundary")
    kind = catalog.get("kind")
    if kind == "portable-python-tck-catalog":
        if source_boundary != "packages/umbra-rti-java-tck-2010/jpype_smoke.py":
            findings.append(
                "Python catalog source boundary must be the JPype smoke runner"
            )
    elif source_boundary != "packages/umbra-rti-java-tck-2010":
        findings.append(
            "Java catalog source boundary must be the 2010 Java TCK package"
        )
    document_id = catalog.get("document_id")
    documents = bundle.get("documents")
    if not isinstance(document_id, str) or not isinstance(documents, list):
        return findings + ["catalog document_id or bundle documents are invalid"]
    documents_by_id: dict[str, JsonObject] = {}
    for item in documents:
        if not isinstance(item, dict) or not all(isinstance(key, str) for key in item):
            continue
        document_record = cast(JsonObject, item)
        bundle_document_id = document_record.get("document_id")
        if isinstance(bundle_document_id, str):
            documents_by_id[bundle_document_id] = document_record
    document = documents_by_id.get(document_id)
    if document is None:
        return findings + [f"Requirements Lab document {document_id!r} is absent"]
    requirements = _record_ids(document, "requirements", findings)
    mappings = _record_ids(document, "mappings", findings)
    transitions = _record_ids(document, "transitions", findings)
    scenarios = catalog.get("scenarios")
    if not isinstance(scenarios, list) or not scenarios:
        return findings + ["catalog scenarios must be a non-empty array"]
    try:
        findings.extend(_api_reference_findings(catalog, python_source))
    except (OSError, SyntaxError, ValueError) as error:
        findings.append(f"Python 2010 API inventory could not be checked: {error}")
    seen: set[str] = set()
    for scenario in scenarios:
        if not isinstance(scenario, dict) or not all(
            isinstance(key, str) for key in scenario
        ):
            findings.append("catalog contains a non-object scenario")
            continue
        scenario_record = cast(JsonObject, scenario)
        scenario_id = scenario_record.get("id")
        if not isinstance(scenario_id, str):
            findings.append("scenario id must be a string")
            continue
        if scenario_id in seen:
            findings.append(f"duplicate scenario id {scenario_id}")
        seen.add(scenario_id)
        for field, allowed in (
            ("requirements_lab_requirement_ids", requirements),
            ("requirements_lab_mapping_ids", mappings),
            ("transition_ids", transitions),
        ):
            values = scenario_record.get(field, [])
            if not isinstance(values, list):
                findings.append(f"{scenario_id}: {field} must be an array")
                continue
            for value in values:
                if not isinstance(value, str):
                    findings.append(f"{scenario_id}: {field} contains a non-string id")
                elif value not in allowed:
                    findings.append(
                        f"{scenario_id}: {field} references missing {value!r}"
                    )
    return findings


def _record_ids(document: JsonObject, field: str, findings: list[str]) -> set[str]:
    records = document.get(field, [])
    if not isinstance(records, list):
        findings.append(f"Requirements Lab document {field} must be an array")
        return set()
    identifiers: set[str] = set()
    for index, record in enumerate(records):
        if not isinstance(record, dict) or not all(
            isinstance(key, str) for key in record
        ):
            findings.append(
                f"Requirements Lab document {field}[{index}] must be an object"
            )
            continue
        identifier = record.get("id")
        if not isinstance(identifier, str):
            findings.append(
                f"Requirements Lab document {field}[{index}].id must be a string"
            )
            continue
        identifiers.add(identifier)
    return identifiers


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--catalog", type=Path, default=DEFAULT_CATALOG)
    parser.add_argument("--bundle", type=Path, default=DEFAULT_BUNDLE)
    parser.add_argument(
        "--python-source",
        type=Path,
        default=DEFAULT_PYTHON_SOURCE,
        help="2010 Python contract source root used to validate catalog API references",
    )
    args = parser.parse_args()
    try:
        findings = verify(args.catalog, args.bundle, args.python_source)
    except (OSError, ValueError, json.JSONDecodeError) as error:
        print(f"IEEE 1516e TCK catalog verification: FAIL: {error}", file=sys.stderr)
        return 1
    if findings:
        print("IEEE 1516e TCK catalog verification: FAIL", file=sys.stderr)
        for finding in findings:
            print(f"  {finding}", file=sys.stderr)
        return 1
    print("IEEE 1516e TCK catalog verification: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
