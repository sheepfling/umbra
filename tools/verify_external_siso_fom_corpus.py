"""Verify an explicitly configured, unvendored SISO FOM corpus snapshot."""

from __future__ import annotations

import argparse
import hashlib
import json
import sys
import xml.etree.ElementTree as element_tree
from pathlib import Path
from typing import Any


REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST = REPOSITORY_ROOT / "compliance" / "external-siso-fom-corpus.json"
XSI_SCHEMA_LOCATION = "{http://www.w3.org/2001/XMLSchema-instance}schemaLocation"


def _sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def _load_manifest(path: Path) -> tuple[dict[str, Any] | None, tuple[str, ...]]:
    try:
        payload = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        return None, (f"cannot read corpus manifest: {error}",)
    if not isinstance(payload, dict) or payload.get("schema_version") != 1:
        return None, ("corpus manifest must be a schema_version 1 object",)
    fixtures = payload.get("fixtures")
    if not isinstance(fixtures, list) or not fixtures:
        return None, ("corpus manifest must contain a non-empty fixtures array",)
    return payload, ()


def _safe_path(root: Path, relative_path: str) -> Path | None:
    candidate = (root / relative_path).resolve()
    try:
        candidate.relative_to(root)
    except ValueError:
        return None
    return candidate


def verify(root: Path, manifest_path: Path) -> tuple[str, ...]:
    """Return deterministic findings for the configured external corpus."""
    root = root.resolve()
    if not root.is_dir():
        return (f"corpus root is not a directory: {root}",)

    manifest, findings = _load_manifest(manifest_path)
    if manifest is None:
        return findings

    validated_ids: set[str] = set()
    for fixture in manifest["fixtures"]:
        if not isinstance(fixture, dict):
            findings += ("corpus manifest contains a non-object fixture",)
            continue
        fixture_id = fixture.get("id")
        relative_path = fixture.get("path")
        digest = fixture.get("sha256")
        namespace = fixture.get("xml_namespace")
        schema_location = fixture.get("schema_location")
        if (
            not isinstance(fixture_id, str)
            or not fixture_id
            or fixture_id in validated_ids
            or not isinstance(relative_path, str)
            or Path(relative_path).is_absolute()
            or not isinstance(digest, str)
            or not isinstance(namespace, str)
            or not isinstance(schema_location, str)
        ):
            findings += ("corpus manifest contains an invalid or duplicate fixture",)
            continue
        validated_ids.add(fixture_id)

        source = _safe_path(root, relative_path)
        if source is None:
            findings += (f"{fixture_id}: path escapes the configured corpus root",)
            continue
        if not source.is_file():
            findings += (f"{fixture_id}: missing file {relative_path}",)
            continue
        if _sha256(source) != digest:
            findings += (f"{fixture_id}: SHA-256 mismatch for {relative_path}",)
            continue
        try:
            document_root = element_tree.parse(source).getroot()
        except (OSError, element_tree.ParseError) as error:
            findings += (f"{fixture_id}: cannot parse XML: {error}",)
            continue
        expected_root = f"{{{namespace}}}objectModel"
        if document_root.tag != expected_root:
            findings += (f"{fixture_id}: unexpected XML root namespace",)
        if document_root.get(XSI_SCHEMA_LOCATION) != schema_location:
            findings += (f"{fixture_id}: unexpected xsi:schemaLocation",)
    return findings


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, required=True, help="configured external SISO corpus root")
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST, help="Umbra corpus manifest")
    arguments = parser.parse_args()

    findings = verify(arguments.root, arguments.manifest)
    if findings:
        print("External SISO FOM corpus integrity: FAIL", file=sys.stderr)
        for finding in findings:
            print(f"- {finding}", file=sys.stderr)
        return 1

    manifest, _ = _load_manifest(arguments.manifest)
    assert manifest is not None
    print(f"External SISO FOM corpus integrity: PASS ({len(manifest['fixtures'])} fixtures)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
