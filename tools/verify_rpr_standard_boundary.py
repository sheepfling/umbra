"""Verify that the RPR codec stays outside standard RTI translation units."""

from __future__ import annotations

import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SOURCE_ROOT = ROOT / "cpp" / "src"
NEUTRAL_HEADER = SOURCE_ROOT / "internal" / "fom" / "fom_wire_encoding.hpp"
RPR_ADAPTER = SOURCE_ROOT / "internal" / "fom" / "fom_rpr_wire_encoding.hpp"
COMPOSER = SOURCE_ROOT / "internal" / "fom" / "libxml2_fom_composer.cpp"


def source_files() -> list[Path]:
    return [
        path
        for path in SOURCE_ROOT.rglob("*")
        if path.is_file() and path.suffix.lower() in {".c", ".cc", ".cpp", ".h", ".hpp"}
    ]


def verify() -> list[str]:
    findings: list[str] = []

    if not NEUTRAL_HEADER.is_file():
        findings.append(f"neutral FOM wire header is missing: {NEUTRAL_HEADER}")
    else:
        neutral_text = NEUTRAL_HEADER.read_text(encoding="utf-8")
        if 'fom_wire_codec.hpp' in neutral_text:
            findings.append(
                "fom_wire_encoding.hpp must not include the RPR byte-codec implementation"
            )
        if "rprWireCodecAvailable" in neutral_text:
            findings.append(
                "fom_wire_encoding.hpp must not promote RPR codec availability"
            )

    if not RPR_ADAPTER.is_file():
        findings.append(f"RPR wire adapter is missing: {RPR_ADAPTER}")
    if not COMPOSER.is_file():
        findings.append(f"FOM composer is missing: {COMPOSER}")

    allowed_codec_importers = {RPR_ADAPTER}
    allowed_adapter_importers = {COMPOSER}
    for path in source_files():
        text = path.read_text(encoding="utf-8", errors="replace")
        if 'fom_wire_codec.hpp' in text and path not in allowed_codec_importers:
            findings.append(
                f"standard/source translation unit imports the RPR codec directly: {path.relative_to(ROOT)}"
            )
        if 'fom_rpr_wire_encoding.hpp' in text and path not in allowed_adapter_importers:
            findings.append(
                f"standard/source translation unit imports the RPR adapter directly: {path.relative_to(ROOT)}"
            )

    return findings


def main() -> int:
    findings = verify()
    if findings:
        print("RPR standard RTI boundary verification: FAIL", file=sys.stderr)
        for finding in findings:
            print(f"  {finding}", file=sys.stderr)
        return 1
    print("RPR standard RTI boundary verification: PASS")
    print("  neutral FOM descriptor excludes the RPR codec")
    print("  RPR promotion remains confined to the FOM composer adapter")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
