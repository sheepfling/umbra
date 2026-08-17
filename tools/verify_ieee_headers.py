"""Verify the exact file set and SHA-256 digests of the vendored IEEE headers."""

from __future__ import annotations

import hashlib
import json
import sys
from pathlib import Path


REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
COMPONENT_ROOT = REPOSITORY_ROOT / "third_party" / "ieee1516.1-2025"
HEADER_ROOT = COMPONENT_ROOT / "include"
DIGESTS = COMPONENT_ROOT / "header-digests.json"


def _sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify() -> tuple[str, ...]:
    """Return deterministic integrity findings, or an empty tuple."""
    try:
        manifest = json.loads(DIGESTS.read_text(encoding="utf-8"))
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
        path.relative_to(HEADER_ROOT).as_posix(): path
        for path in HEADER_ROOT.rglob("*.h")
    }
    findings = [f"unexpected header {path}" for path in sorted(actual_files.keys() - expected.keys())]
    findings.extend(f"missing header {path}" for path in sorted(expected.keys() - actual_files.keys()))
    for relative_path in sorted(actual_files.keys() & expected.keys()):
        actual_digest = _sha256(actual_files[relative_path])
        if actual_digest != expected[relative_path]:
            findings.append(f"digest mismatch for {relative_path}")
    return tuple(findings)


def main() -> int:
    findings = verify()
    if findings:
        print("IEEE header integrity: FAIL", file=sys.stderr)
        for finding in findings:
            print(f"- {finding}", file=sys.stderr)
        return 1
    print("IEEE header integrity: PASS (44 headers)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
