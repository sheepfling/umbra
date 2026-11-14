"""Verify pinned IEEE 1516.1-2025 C++ API header sets and copies."""

from __future__ import annotations

import argparse
import hashlib
import io
import json
import sys
from pathlib import Path, PurePosixPath
from zipfile import BadZipFile, ZipFile

REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
COMPONENT_ROOT = REPOSITORY_ROOT / "third_party" / "ieee1516.1-2025"
HEADER_ROOT = COMPONENT_ROOT / "include"
DIGESTS = COMPONENT_ROOT / "header-digests.json"
MANIFEST = COMPONENT_ROOT / "manifest.json"


def _sha256(content: bytes) -> str:
    return hashlib.sha256(content).hexdigest()


def _expected_headers() -> tuple[dict[str, str], tuple[str, ...]]:
    try:
        manifest = json.loads(DIGESTS.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        return {}, (f"cannot read digest manifest: {error}",)
    if not isinstance(manifest, dict):
        return {}, ("digest manifest must be a JSON object",)
    entries = manifest.get("files")
    if manifest.get("schema_version") != 1 or not isinstance(entries, list):
        return {}, (
            "digest manifest must be a schema_version 1 object with a files array",
        )

    expected: dict[str, str] = {}
    for entry in entries:
        if not isinstance(entry, dict):
            return {}, ("digest manifest contains a non-object entry",)
        path, digest = entry.get("path"), entry.get("sha256")
        relative = PurePosixPath(path) if isinstance(path, str) else None
        if (
            relative is None
            or relative.is_absolute()
            or ".." in relative.parts
            or not relative.parts
            or relative.parts[0] != "RTI"
            or not isinstance(digest, str)
            or path in expected
        ):
            return {}, ("digest manifest contains an invalid or duplicate entry",)
        expected[path] = digest
    return expected, ()


def _verify_header_tree(
    include_root: Path, expected: dict[str, str]
) -> tuple[str, ...]:
    api_root = include_root / "RTI"
    if not api_root.is_dir():
        return (f"API header directory is missing: {api_root}",)
    actual_files = {
        path.relative_to(include_root).as_posix(): path
        for path in api_root.rglob("*")
        if path.is_file()
    }
    findings = [
        f"unexpected API file {path}"
        for path in sorted(actual_files.keys() - expected.keys())
    ]
    findings.extend(
        f"missing API file {path}"
        for path in sorted(expected.keys() - actual_files.keys())
    )
    for relative_path in sorted(actual_files.keys() & expected.keys()):
        if _sha256(actual_files[relative_path].read_bytes()) != expected[relative_path]:
            findings.append(f"digest mismatch for {relative_path}")
    return tuple(findings)


def _verify_archive(
    archive_root: Path,
    label: str,
    record: object,
    expected: dict[str, str],
) -> tuple[str, ...]:
    if not isinstance(record, dict):
        return (f"{label} archive record is not an object",)
    outer_name = record.get("outer_file")
    outer_digest = record.get("outer_sha256")
    inner_name = record.get("inner_file")
    inner_digest = record.get("inner_sha256")
    if not all(
        isinstance(value, str) and value
        for value in (outer_name, outer_digest, inner_name, inner_digest)
    ):
        return (f"{label} archive record is incomplete",)

    outer_path = archive_root / outer_name
    try:
        outer_bytes = outer_path.read_bytes()
    except OSError as error:
        return (f"cannot read {label} outer archive {outer_path}: {error}",)
    if _sha256(outer_bytes) != outer_digest:
        return (f"{label} outer archive SHA-256 mismatch: {outer_path}",)

    try:
        with ZipFile(io.BytesIO(outer_bytes)) as outer_zip:
            inner_paths = [
                member
                for member in outer_zip.namelist()
                if PurePosixPath(member).name == inner_name
            ]
            if len(inner_paths) != 1:
                message = (
                    f"{label} outer archive must contain exactly one {inner_name}; "
                    f"found {len(inner_paths)}"
                )
                return (message,)
            inner_bytes = outer_zip.read(inner_paths[0])
        if _sha256(inner_bytes) != inner_digest:
            return (f"{label} inner archive SHA-256 mismatch: {inner_name}",)

        with ZipFile(io.BytesIO(inner_bytes)) as inner_zip:
            archive_files: dict[str, bytes] = {}
            for member in inner_zip.namelist():
                if member.endswith("/"):
                    continue
                member_parts = PurePosixPath(member).parts
                try:
                    rti_root = member_parts.index("RTI")
                except ValueError:
                    continue
                relative = PurePosixPath(*member_parts[rti_root:]).as_posix()
                if relative in archive_files:
                    return (
                        f"{label} inner archive contains duplicate API file {relative}",
                    )
                archive_files[relative] = inner_zip.read(member)
    except (BadZipFile, KeyError, OSError, RuntimeError) as error:
        return (f"cannot inspect {label} nested API archive: {error}",)

    findings = [
        f"{label} archive has unexpected API file {path}"
        for path in sorted(archive_files.keys() - expected.keys())
    ]
    findings.extend(
        f"{label} archive is missing API file {path}"
        for path in sorted(expected.keys() - archive_files.keys())
    )
    for relative_path in sorted(archive_files.keys() & expected.keys()):
        if _sha256(archive_files[relative_path]) != expected[relative_path]:
            findings.append(f"{label} archive digest mismatch for {relative_path}")
    return tuple(findings)


def verify(
    include_root: Path = HEADER_ROOT,
    archive_root: Path | None = None,
) -> tuple[str, ...]:
    """Return deterministic integrity findings, or an empty tuple.

    ``include_root`` must contain ``RTI/``. If ``archive_root`` is supplied,
    both licensed download bundles recorded in ``manifest.json`` are hash-checked
    and their nested API header sets are compared with ``header-digests.json``.
    """
    expected, manifest_findings = _expected_headers()
    if manifest_findings:
        return manifest_findings

    findings = list(_verify_header_tree(Path(include_root), expected))
    try:
        provenance = json.loads(MANIFEST.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        return (f"cannot read API provenance manifest: {error}",)
    if not isinstance(provenance, dict):
        return ("API provenance manifest must be a JSON object",)
    imported = provenance.get("import")
    if not isinstance(imported, dict) or imported.get("file_count") != len(expected):
        findings.append("API provenance file_count does not match header-digests.json")

    if archive_root is not None:
        upstream = provenance.get("upstream_archive")
        if not isinstance(upstream, dict):
            findings.append("API provenance manifest has no upstream_archive object")
        else:
            root = Path(archive_root)
            findings.extend(
                _verify_archive(
                    root, "import-source", upstream.get("import_source"), expected
                )
            )
            findings.extend(
                _verify_archive(
                    root,
                    "canonical-source",
                    upstream.get("canonical_api_source"),
                    expected,
                )
            )
    return tuple(findings)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--include-root",
        type=Path,
        default=HEADER_ROOT,
        help="include directory containing the RTI/ API headers (default: vendored tree)",
    )
    parser.add_argument(
        "--archive-root",
        type=Path,
        help="optional directory containing the upstream outer ZIP files named in manifest.json",
    )
    args = parser.parse_args()
    findings = verify(args.include_root, args.archive_root)
    if findings:
        print("IEEE header integrity: FAIL", file=sys.stderr)
        for finding in findings:
            print(f"- {finding}", file=sys.stderr)
        return 1
    print("IEEE 1516.1-2025 API headers: PASS (file set and SHA-256 digests)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
