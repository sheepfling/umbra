"""Export and check Umbra's portable HLA Requirements Lab baselines.

The Requirements Lab owns semantic HLA data and verification. This helper
keeps the repository boundary explicit: it invokes the Lab's public export
module, then validates an Umbra-owned API baseline or implementation contract
against the exported JSON without importing any Lab package into library code.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
from pathlib import Path
from typing import Any


REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_BUNDLE = REPOSITORY_ROOT / ".compliance" / "corpus-bundle.json"
DEFAULT_CONTRACT = REPOSITORY_ROOT / "compliance" / "requirements-lab" / "api-baseline.json"
DEFAULT_LOCK = REPOSITORY_ROOT / "compliance" / "requirements-lab" / "requirements-lab.lock.json"
DEFAULT_LAB_ROOT = REPOSITORY_ROOT.parent / "Document-Recreation"
DEFAULT_OBSERVATIONS = REPOSITORY_ROOT / "docs" / "testing" / "REQUIREMENTS-LAB-OBSERVATIONS.md"
OBSERVATION_HEADING = re.compile(r"^###\\s+(RL-\\d{3})\\s+—", re.MULTILINE)


def _lab_root(value: Path | None) -> Path:
    root = value or Path(os.environ.get("HLA_REQUIREMENTS_LAB_ROOT", DEFAULT_LAB_ROOT))
    root = root.resolve()
    if not (root / "hla_lab" / "publication" / "export_compliance_bundle.py").is_file():
        raise ValueError(
            f"{root} is not an HLA Requirements Lab checkout; set "
            "HLA_REQUIREMENTS_LAB_ROOT or pass --lab-root"
        )
    return root


def export_bundle(
    *,
    lab_root: Path,
    output: Path,
    revision: str,
    edition: str,
    python: str,
) -> None:
    """Run the Lab's public bundle exporter with a controlled working directory."""
    output = output.resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    command = (
        python,
        "-m",
        "hla_lab.publication.export_compliance_bundle",
        "--corpus-root",
        str(lab_root),
        "--edition",
        edition,
        "--revision",
        revision,
        "--bundle-id",
        "umbra-hla-requirements",
        "--output",
        str(output),
    )
    subprocess.run(command, cwd=lab_root, check=True)


def _json_object(path: Path) -> dict[str, Any]:
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise ValueError(f"cannot read JSON from {path}: {error}") from error
    if not isinstance(data, dict):
        raise ValueError(f"{path} must contain a JSON object")
    return data


def _locked_revision(lock_path: Path) -> str:
    lock = _json_object(lock_path)
    if lock.get("schema_version") != 1:
        raise ValueError(f"{lock_path} has an unsupported schema_version")
    revision = lock.get("revision")
    if not isinstance(revision, str) or not revision.strip():
        raise ValueError(f"{lock_path} must declare a non-empty revision")
    return revision


def _document(bundle: dict[str, Any], document_id: str) -> dict[str, Any]:
    documents = bundle.get("documents")
    if not isinstance(documents, list):
        raise ValueError("bundle has no documents array")
    matches = [item for item in documents if item.get("document_id") == document_id]
    if len(matches) != 1:
        raise ValueError(f"bundle must contain exactly one {document_id!r} document")
    return matches[0]


def _ids(record: dict[str, Any], name: str) -> tuple[str, ...]:
    values = record.get(name, [])
    if not isinstance(values, list) or not all(isinstance(item, str) for item in values):
        raise ValueError(f"record {record.get('id', '<unknown>')} has invalid {name}")
    return tuple(values)


def _header_path(value: object) -> Path | None:
    if not isinstance(value, str):
        return None
    root = (REPOSITORY_ROOT / "third_party" / "ieee1516.1-2025" / "include").resolve()
    candidate = (root / value).resolve()
    if not candidate.is_relative_to(root):
        return None
    return candidate


def _check_observations(path: Path = DEFAULT_OBSERVATIONS) -> tuple[str, ...]:
    """Return findings for duplicate Requirements Lab observation identifiers."""
    try:
        text = path.read_text(encoding="utf-8")
    except OSError as error:
        return (f"cannot read observations log {path}: {error}",)
    identifiers = OBSERVATION_HEADING.findall(text)
    counts: dict[str, int] = {}
    for identifier in identifiers:
        counts[identifier] = counts.get(identifier, 0) + 1
    return tuple(
        f"observations log contains duplicate identifier {identifier} ({count} headings)"
        for identifier, count in sorted(counts.items())
        if count > 1
    )


def _check_source_and_tests(owned_id: object, owned: dict[str, Any]) -> list[str]:
    findings: list[str] = []
    source = owned.get("source")
    source_path = REPOSITORY_ROOT / source if isinstance(source, str) else None
    if source_path is None or not source_path.is_file():
        findings.append(f"{owned_id}: source path {source!r} is absent")
    else:
        source_symbol = owned.get("source_symbol")
        if not isinstance(source_symbol, str) or source_symbol not in source_path.read_text(
            encoding="utf-8"
        ):
            findings.append(f"{owned_id}: source symbol {source_symbol!r} is absent from {source}")
    tests = owned.get("tests")
    if not isinstance(tests, list) or not tests:
        findings.append(f"{owned_id}: at least one test reference is required")
    else:
        for test in tests:
            test_reference = str(test)
            test_file, separator, test_name = test_reference.partition("::")
            test_path = REPOSITORY_ROOT / test_file
            if not test_path.is_file():
                findings.append(f"{owned_id}: test path {test_file!r} is absent")
            elif separator and (
                not test_name or test_name not in test_path.read_text(encoding="utf-8")
            ):
                findings.append(f"{owned_id}: test selector {test_reference!r} is absent")
    return findings


def _check_requirements_reference_contract(
    contract: dict[str, Any], document: dict[str, Any]
) -> tuple[str, ...]:
    requirements = document.get("requirements")
    if not isinstance(requirements, list):
        return (f"bundle document {document.get('document_id')} has no requirements array",)
    lab_requirements = {item.get("id"): item for item in requirements if isinstance(item, dict)}
    owned_requirements = contract.get("requirements")
    if not isinstance(owned_requirements, list) or not owned_requirements:
        return ("requirements-reference contract must contain at least one requirement",)

    findings: list[str] = []
    for owned in owned_requirements:
        if not isinstance(owned, dict):
            findings.append("requirements-reference contract contains a non-object requirement")
            continue
        owned_id = owned.get("id", "<unknown>")
        lab_id = owned.get("requirements_lab_requirement_id")
        if not isinstance(lab_id, str):
            findings.append(f"{owned_id}: requirements_lab_requirement_id must be a string")
            continue
        lab_requirement = lab_requirements.get(lab_id)
        if lab_requirement is None:
            findings.append(f"{owned_id}: Lab requirement {lab_id!r} is absent")
            continue
        expected_clause = owned.get("clause_id")
        if not isinstance(expected_clause, str):
            findings.append(f"{owned_id}: clause_id must be a string")
        elif lab_requirement.get("clause_id") != expected_clause:
            findings.append(
                f"{owned_id}: clause ID drifted; expected {expected_clause!r}, "
                f"Lab has {lab_requirement.get('clause_id')!r}"
            )
        findings.extend(_check_source_and_tests(owned_id, owned))
    return tuple(findings)


def _check_api_reference_contract(
    contract: dict[str, Any], document: dict[str, Any]
) -> tuple[str, ...]:
    api_surfaces = document.get("api_surfaces")
    if not isinstance(api_surfaces, list):
        return (f"bundle document {document.get('document_id')} has no api_surfaces array",)
    lab_api_surfaces = {item.get("id"): item for item in api_surfaces if isinstance(item, dict)}
    owned_api_surfaces = contract.get("api_surfaces")
    if not isinstance(owned_api_surfaces, list) or not owned_api_surfaces:
        return ("api-reference contract must contain at least one api surface",)

    findings: list[str] = []
    for owned in owned_api_surfaces:
        if not isinstance(owned, dict):
            findings.append("api-reference contract contains a non-object api surface")
            continue
        owned_id = owned.get("id", "<unknown>")
        lab_id = owned.get("requirements_lab_api_surface_id")
        if not isinstance(lab_id, str):
            findings.append(f"{owned_id}: requirements_lab_api_surface_id must be a string")
            continue
        lab_api_surface = lab_api_surfaces.get(lab_id)
        if lab_api_surface is None:
            findings.append(f"{owned_id}: Lab API surface {lab_id!r} is absent")
            continue

        for field in ("owner", "language", "signature"):
            expected = owned.get(field)
            if not isinstance(expected, str):
                findings.append(f"{owned_id}: {field} must be a string")
            elif lab_api_surface.get(field) != expected:
                findings.append(
                    f"{owned_id}: {field} drifted; expected {expected!r}, "
                    f"Lab has {lab_api_surface.get(field)!r}"
                )
        try:
            expected_exceptions = _ids(owned, "exception_types")
            actual_exceptions = _ids(lab_api_surface, "exception_types")
        except ValueError as error:
            findings.append(str(error))
        else:
            if expected_exceptions != actual_exceptions:
                findings.append(
                    f"{owned_id}: exception types drifted; expected {expected_exceptions}, "
                    f"Lab has {actual_exceptions}"
                )
        findings.extend(_check_source_and_tests(owned_id, owned))
    return tuple(findings)


def check_contract(contract_path: Path, bundle_path: Path) -> tuple[str, ...]:
    """Return deterministic contract drift findings, or an empty tuple."""
    contract = _json_object(contract_path)
    bundle = _json_object(bundle_path)
    if contract.get("schema_version") != 1:
        return ("contract schema_version must be 1",)
    kind = contract.get("kind", "implementation-contract")
    if kind not in {
        "api-baseline",
        "implementation-contract",
        "requirements-reference",
        "api-reference",
    }:
        return (f"unsupported contract kind {kind!r}",)
    document_id = contract.get("document_id")
    if not isinstance(document_id, str):
        return ("contract document_id must be a string",)
    document = _document(bundle, document_id)
    if kind == "requirements-reference":
        return _check_requirements_reference_contract(contract, document)
    if kind == "api-reference":
        return _check_api_reference_contract(contract, document)
    mappings = document.get("mappings")
    if not isinstance(mappings, list):
        return (f"bundle document {document_id} has no mappings array",)
    lab_mappings = {item.get("id"): item for item in mappings if isinstance(item, dict)}
    findings: list[str] = []
    owned_mappings = contract.get("mappings")
    if not isinstance(owned_mappings, list) or not owned_mappings:
        return ("contract must contain at least one mapping",)
    for owned in owned_mappings:
        if not isinstance(owned, dict):
            findings.append("contract contains a non-object mapping")
            continue
        owned_id = owned.get("id", "<unknown>")
        lab_id = owned.get("requirements_lab_mapping_id")
        if not isinstance(lab_id, str):
            findings.append(f"{owned_id}: requirements_lab_mapping_id must be a string")
            continue
        lab_mapping = lab_mappings.get(lab_id)
        if lab_mapping is None:
            findings.append(f"{owned_id}: Lab mapping {lab_id!r} is absent")
            continue
        try:
            expected_requirements = _ids(owned, "requirement_ids")
            actual_requirements = _ids(lab_mapping, "requirement_ids")
            expected_transitions = _ids(owned, "transition_ids")
            actual_transitions = _ids(lab_mapping, "transition_ids")
        except ValueError as error:
            findings.append(str(error))
            continue
        if expected_requirements != actual_requirements:
            findings.append(
                f"{owned_id}: requirement IDs drifted; expected {expected_requirements}, "
                f"Lab has {actual_requirements}"
            )
        if expected_transitions != actual_transitions:
            findings.append(
                f"{owned_id}: transition IDs drifted; expected {expected_transitions}, "
                f"Lab has {actual_transitions}"
            )
        if kind == "api-baseline":
            header = owned.get("header")
            header_path = _header_path(header)
            if header_path is None or not header_path.is_file():
                findings.append(f"{owned_id}: header path {header!r} is absent")
                continue
            marker = owned.get("declaration_marker")
            minimum_matches = owned.get("minimum_matches")
            if not isinstance(marker, str) or not isinstance(minimum_matches, int):
                findings.append(f"{owned_id}: API baseline needs marker and minimum_matches")
                continue
            actual_matches = header_path.read_text(encoding="utf-8").count(marker)
            if actual_matches < minimum_matches:
                findings.append(
                    f"{owned_id}: expected at least {minimum_matches} occurrences of "
                    f"{marker!r} in {header}, found {actual_matches}"
                )
            continue

        findings.extend(_check_source_and_tests(owned_id, owned))
    return tuple(findings)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)

    export = subparsers.add_parser("export", help="Export a Lab CorpusBundle into .compliance")
    export.add_argument("--lab-root", type=Path)
    export.add_argument("--output", type=Path, default=DEFAULT_BUNDLE)
    export.add_argument("--lock", type=Path, default=DEFAULT_LOCK)
    export.add_argument("--revision", help="Override the reviewed Requirements Lab revision")
    export.add_argument("--edition", choices=("2010", "2025", "all"), default="all")
    export.add_argument("--python", default=sys.executable, help="Python environment for the Lab")

    check = subparsers.add_parser("check", help="Check an Umbra baseline or contract against a bundle")
    check.add_argument("--bundle", type=Path, default=DEFAULT_BUNDLE)
    check.add_argument("--contract", type=Path, default=DEFAULT_CONTRACT)

    args = parser.parse_args()
    try:
        if args.command == "export":
            export_bundle(
                lab_root=_lab_root(args.lab_root),
                output=args.output,
                revision=args.revision or _locked_revision(args.lock),
                edition=args.edition,
                python=args.python,
            )
            print(f"exported Requirements Lab bundle to {args.output}")
            return 0
        findings = list(check_contract(args.contract, args.bundle))
        findings.extend(_check_observations())
    except (OSError, RuntimeError, ValueError, subprocess.CalledProcessError) as error:
        print(f"requirements-lab: {error}", file=sys.stderr)
        return 2
    if findings:
        print("requirements-lab: contract drift detected", file=sys.stderr)
        for finding in findings:
            print(f"- {finding}", file=sys.stderr)
        return 1
    print("requirements-lab: baseline or contract matches the supplied bundle")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
