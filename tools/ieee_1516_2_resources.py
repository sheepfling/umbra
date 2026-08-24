"""Import and verify the exact IEEE 1516.2-2025 OMT resource set."""

from __future__ import annotations

import argparse
import hashlib
import io
import json
import sys
from pathlib import Path
from zipfile import BadZipFile, ZipFile


REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
COMPONENT_ROOT = REPOSITORY_ROOT / "third_party" / "ieee1516.2-2025"
RESOURCE_ROOT = COMPONENT_ROOT / "resources"
DIGESTS = COMPONENT_ROOT / "resource-digests.json"

OUTER_ARCHIVE_SHA256 = "61e537450f25ce28c4be373add7d0c6c946e5d10116e8a84377efee82d516121"
INNER_ARCHIVE_NAME = "hla4xml-submission-ready-2025-05-07.zip"
INNER_ARCHIVE_SHA256 = "9730c8d4925b50a23e5fa18726ff99cf1e09059e6bb6d2a0c2d6fa2641dbdaf6"

RESOURCES = {
    "IEEE1516-DIF-2025.xsd": "schemas/IEEE1516-DIF-2025.xsd",
    "IEEE1516-FDD-2025.xsd": "schemas/IEEE1516-FDD-2025.xsd",
    "IEEE1516-OMT-2025.xsd": "schemas/IEEE1516-OMT-2025.xsd",
    "HLAstandardMIM-2025.xml": "mim/HLAstandardMIM-2025.xml",
    "RestaurantExtensionFOMmodule-2025.xml": "examples/RestaurantExtensionFOMmodule-2025.xml",
    "RestaurantFOMmodule-2025.xml": "examples/RestaurantFOMmodule-2025.xml",
    "RestaurantSOMmodule-2025.xml": "examples/RestaurantSOMmodule-2025.xml",
}


def _sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def _read_resources(source_archive: Path) -> dict[str, bytes]:
    outer_bytes = source_archive.read_bytes()
    if _sha256(outer_bytes) != OUTER_ARCHIVE_SHA256:
        raise ValueError("outer archive SHA-256 does not match the reviewed IEEE source")

    try:
        with ZipFile(io.BytesIO(outer_bytes)) as outer:
            inner_bytes = outer.read(INNER_ARCHIVE_NAME)
        if _sha256(inner_bytes) != INNER_ARCHIVE_SHA256:
            raise ValueError("inner OMT archive SHA-256 does not match the reviewed IEEE source")
        with ZipFile(io.BytesIO(inner_bytes)) as inner:
            return {source: inner.read(source) for source in RESOURCES}
    except (BadZipFile, KeyError) as error:
        raise ValueError(f"cannot read the required IEEE OMT resources: {error}") from error


def import_resources(source_archive: Path) -> int:
    try:
        source_resources = _read_resources(source_archive)
    except (OSError, ValueError) as error:
        print(f"IEEE 1516.2 resource import: FAIL: {error}", file=sys.stderr)
        return 1

    digest_entries = []
    for source_name, relative_destination in RESOURCES.items():
        data = source_resources[source_name]
        destination = RESOURCE_ROOT / relative_destination
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_bytes(data)
        digest_entries.append({"path": relative_destination, "sha256": _sha256(data)})

    DIGESTS.write_text(
        json.dumps(
            {
                "schema_version": 1,
                "component": "IEEE 1516.2-2025 OMT schemas, MIM, and examples",
                "files": sorted(digest_entries, key=lambda entry: entry["path"]),
            },
            indent=2,
        )
        + "\n",
        encoding="utf-8",
    )
    print(f"IEEE 1516.2 resource import: PASS ({len(digest_entries)} resources)")
    return 0


def verify_resources(
    resource_root: Path = RESOURCE_ROOT,
    digest_manifest: Path = DIGESTS,
) -> tuple[str, ...]:
    """Return deterministic findings for a staged 1516.2 resource set.

    The default arguments keep the historical source-tree check intact.  The
    explicit paths are also used by the installed-package smoke test so that
    packaging verifies the files that consumers will actually load rather
    than only the vendored checkout.
    """
    resource_root = resource_root.resolve()
    digest_manifest = digest_manifest.resolve()
    if not resource_root.is_dir():
        return (f"resource root is not a directory: {resource_root}",)
    try:
        manifest = json.loads(digest_manifest.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        return (f"cannot read digest manifest: {error}",)

    entries = manifest.get("files") if isinstance(manifest, dict) else None
    if manifest.get("schema_version") != 1 or not isinstance(entries, list):
        return ("digest manifest must be a schema_version 1 object with a files array",)

    expected: dict[str, str] = {}
    for entry in entries:
        if not isinstance(entry, dict):
            return ("digest manifest contains a non-object entry",)
        path, digest = entry.get("path"), entry.get("sha256")
        if not isinstance(path, str) or not isinstance(digest, str) or path in expected:
            return ("digest manifest contains an invalid or duplicate entry",)
        expected[path] = digest

    actual_files = {
        path.relative_to(resource_root).as_posix(): path
        for pattern in ("*.xsd", "*.xml")
        for path in resource_root.rglob(pattern)
    }
    findings = [f"unexpected resource {path}" for path in sorted(actual_files.keys() - expected.keys())]
    findings.extend(f"missing resource {path}" for path in sorted(expected.keys() - actual_files.keys()))
    for relative_path in sorted(actual_files.keys() & expected.keys()):
        if _sha256(actual_files[relative_path].read_bytes()) != expected[relative_path]:
            findings.append(f"digest mismatch for {relative_path}")
    return tuple(findings)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, help="local IEEE 1516-2025 downloads archive")
    parser.add_argument("--check", action="store_true", help="verify the vendored resource set")
    parser.add_argument(
        "--root",
        type=Path,
        default=RESOURCE_ROOT,
        help="resource root to verify (defaults to the vendored source tree)",
    )
    parser.add_argument(
        "--manifest",
        type=Path,
        default=DIGESTS,
        help="digest manifest to verify (defaults to the vendored manifest)",
    )
    arguments = parser.parse_args()

    if arguments.check:
        findings = verify_resources(arguments.root, arguments.manifest)
        if findings:
            print("IEEE 1516.2 resource integrity: FAIL", file=sys.stderr)
            for finding in findings:
                print(f"- {finding}", file=sys.stderr)
            return 1
        print(f"IEEE 1516.2 resource integrity: PASS ({len(RESOURCES)} resources)")
        return 0
    if arguments.source is None:
        parser.error("--source is required unless --check is used")
    return import_resources(arguments.source)


if __name__ == "__main__":
    raise SystemExit(main())
