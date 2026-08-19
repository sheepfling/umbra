"""Verify an explicitly configured, unvendored 2025-native FOM corpus snapshot."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from verify_external_siso_fom_corpus import _load_manifest, verify


REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST = REPOSITORY_ROOT / "compliance" / "external-2025-fom-corpus.json"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, required=True, help="configured external 2025 corpus root")
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST, help="Umbra corpus manifest")
    arguments = parser.parse_args()

    findings = verify(arguments.root, arguments.manifest)
    if findings:
        print("External 2025 FOM corpus integrity: FAIL", file=sys.stderr)
        for finding in findings:
            print(f"- {finding}", file=sys.stderr)
        return 1

    manifest, _ = _load_manifest(arguments.manifest)
    assert manifest is not None
    fixture_count = len(manifest["fixtures"]) + len(manifest.get("schema_negative_fixtures", []))
    print(f"External 2025 FOM corpus integrity: PASS ({fixture_count} fixtures)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
