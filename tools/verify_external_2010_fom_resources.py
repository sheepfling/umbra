"""Verify the reviewed, unvendored IEEE 1516.1/1516.2-2010 resources."""

from __future__ import annotations

import argparse
import hashlib
import json
import sys
import xml.etree.ElementTree as element_tree
from pathlib import Path
from typing import TypedDict

REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST = (
    REPOSITORY_ROOT / "compliance" / "fom" / "external-2010-fom-resources.json"
)
XSI_SCHEMA_LOCATION = "{http://www.w3.org/2001/XMLSchema-instance}schemaLocation"


class _Resource(TypedDict):
    id: str
    paths: list[str]
    sha256: str
    xml_namespace: str | None
    schema_location: str | None


class _TargetRadar(TypedDict):
    sha256: str
    xml_namespace: str | None
    schema_location: str | None


class _Manifest(TypedDict):
    schema_version: int
    resources: list[_Resource]


class _ManifestWithTargetRadar(_Manifest, total=False):
    target_radar: _TargetRadar


def _sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def _load_manifest(
    path: Path,
) -> tuple[_ManifestWithTargetRadar | None, tuple[str, ...]]:
    try:
        payload: object = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        return None, (f"cannot read 2010 resource manifest: {error}",)
    if not isinstance(payload, dict):
        return None, ("2010 resource manifest must be a schema_version 1 object",)
    schema_version = payload.get("schema_version")
    if (
        not isinstance(schema_version, int)
        or isinstance(schema_version, bool)
        or schema_version != 1
    ):
        return None, ("2010 resource manifest must be a schema_version 1 object",)
    raw_resources = payload.get("resources")
    if not isinstance(raw_resources, list) or not raw_resources:
        return None, (
            "2010 resource manifest must contain a non-empty resources array",
        )

    resources: list[_Resource] = []
    for index, raw_resource in enumerate(raw_resources):
        if not isinstance(raw_resource, dict):
            return None, (
                f"2010 resource manifest resources[{index}] must be an object",
            )
        resource_id = raw_resource.get("id")
        raw_paths = raw_resource.get("paths")
        digest = raw_resource.get("sha256")
        namespace = raw_resource.get("xml_namespace")
        schema_location = raw_resource.get("schema_location")
        if (
            not isinstance(resource_id, str)
            or not resource_id
            or not isinstance(raw_paths, list)
            or not raw_paths
            or not isinstance(digest, str)
            or not digest
            or (namespace is not None and not isinstance(namespace, str))
            or (schema_location is not None and not isinstance(schema_location, str))
        ):
            return None, (
                f"2010 resource manifest resources[{index}] has invalid fields",
            )
        paths: list[str] = []
        for path_index, relative_path in enumerate(raw_paths):
            if (
                not isinstance(relative_path, str)
                or not relative_path
                or Path(relative_path).is_absolute()
            ):
                return None, (
                    f"2010 resource manifest resources[{index}].paths[{path_index}] must be a relative path",
                )
            paths.append(relative_path)
        resources.append(
            {
                "id": resource_id,
                "paths": paths,
                "sha256": digest,
                "xml_namespace": namespace,
                "schema_location": schema_location,
            }
        )

    manifest: _ManifestWithTargetRadar = {
        "schema_version": schema_version,
        "resources": resources,
    }
    raw_target_radar = payload.get("target_radar")
    if raw_target_radar is not None:
        if not isinstance(raw_target_radar, dict):
            return None, ("2010 resource manifest target_radar must be an object",)
        digest = raw_target_radar.get("sha256")
        namespace = raw_target_radar.get("xml_namespace")
        schema_location = raw_target_radar.get("schema_location")
        if (
            not isinstance(digest, str)
            or not digest
            or (namespace is not None and not isinstance(namespace, str))
            or (schema_location is not None and not isinstance(schema_location, str))
        ):
            return None, ("2010 resource manifest target_radar has invalid fields",)
        manifest["target_radar"] = {
            "sha256": digest,
            "xml_namespace": namespace,
            "schema_location": schema_location,
        }
    return manifest, ()


def _safe_path(root: Path, relative_path: str) -> Path | None:
    candidate = (root / relative_path).resolve()
    try:
        candidate.relative_to(root)
    except ValueError:
        return None
    return candidate


def _validate_xml(
    source: Path,
    fixture_id: str,
    expected_namespace: str | None,
    expected_schema_location: str | None,
) -> tuple[str, ...]:
    if expected_namespace is None and expected_schema_location is None:
        return ()
    try:
        document_root = element_tree.parse(source).getroot()
    except (OSError, element_tree.ParseError) as error:
        return (f"{fixture_id}: cannot parse XML: {error}",)

    findings: list[str] = []
    if expected_namespace is not None:
        expected_root = f"{{{expected_namespace}}}objectModel"
        if document_root.tag != expected_root:
            findings.append(f"{fixture_id}: unexpected XML root namespace")
    if (
        expected_schema_location is not None
        and document_root.get(XSI_SCHEMA_LOCATION) != expected_schema_location
    ):
        findings.append(f"{fixture_id}: unexpected xsi:schemaLocation")
    return tuple(findings)


def _verify_root_resources(root: Path, manifest: _Manifest) -> tuple[str, ...]:
    root = root.resolve()
    if not root.is_dir():
        return (f"2010 resource root is not a directory: {root}",)

    findings: list[str] = []
    seen_ids: set[str] = set()
    for resource in manifest["resources"]:
        resource_id = resource["id"]
        if resource_id in seen_ids:
            findings.append(
                "2010 resource manifest contains an invalid or duplicate resource"
            )
            continue
        seen_ids.add(resource_id)

        candidates: list[Path] = []
        for relative_path in resource["paths"]:
            candidate = _safe_path(root, relative_path)
            if candidate is None:
                findings.append(
                    f"{resource_id}: path escapes the configured resource root"
                )
                continue
            if candidate.is_file():
                candidates.append(candidate)
        if not candidates:
            findings.append(f"{resource_id}: none of the declared files exists")
            continue

        for source in candidates:
            if _sha256(source) != resource["sha256"]:
                findings.append(
                    f"{resource_id}: SHA-256 mismatch for {source.relative_to(root)}"
                )
                continue
            findings.extend(
                _validate_xml(
                    source,
                    resource_id,
                    resource["xml_namespace"],
                    resource["schema_location"],
                )
            )

    return tuple(findings)


def _verify_target_radar(
    target_radar_path: Path,
    manifest: _ManifestWithTargetRadar,
) -> tuple[str, ...]:
    expected = manifest.get("target_radar")
    if expected is None:
        return ("the manifest does not define a Target Radar fixture",)
    target_radar_path = target_radar_path.resolve()
    if not target_radar_path.is_file():
        return (f"Target Radar FOM is not a file: {target_radar_path}",)
    if _sha256(target_radar_path) != expected["sha256"]:
        return (f"Target Radar FOM SHA-256 mismatch: {target_radar_path}",)
    return _validate_xml(
        target_radar_path,
        "target_radar",
        expected["xml_namespace"],
        expected["schema_location"],
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--root",
        type=Path,
        required=True,
        help="configured external 2010 resource root",
    )
    parser.add_argument(
        "--target-radar", type=Path, help="optional reviewed Target Radar FOM path"
    )
    parser.add_argument(
        "--manifest",
        type=Path,
        default=DEFAULT_MANIFEST,
        help="Umbra resource manifest",
    )
    arguments = parser.parse_args()

    manifest, findings = _load_manifest(arguments.manifest)
    if manifest is not None:
        findings += _verify_root_resources(arguments.root, manifest)
        if arguments.target_radar is not None:
            findings += _verify_target_radar(arguments.target_radar, manifest)
    if findings:
        print("External IEEE 1516-2010 FOM resources integrity: FAIL", file=sys.stderr)
        for finding in findings:
            print(f"- {finding}", file=sys.stderr)
        return 1

    resource_count = len(manifest["resources"]) if manifest is not None else 0
    target_suffix = ", Target Radar" if arguments.target_radar is not None else ""
    print(
        "External IEEE 1516-2010 FOM resources integrity: "
        f"PASS ({resource_count} resources{target_suffix})"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
