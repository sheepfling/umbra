"""Verify that every official IEEE 1516.1-2025 exception type has a binding definition."""

from __future__ import annotations

import re
import sys
from pathlib import Path


REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
OFFICIAL_HEADER = (
    REPOSITORY_ROOT / "third_party" / "ieee1516.1-2025" / "include" / "RTI" / "Exception.h"
)
BINDING_SOURCE = REPOSITORY_ROOT / "cpp" / "src" / "ieee1516_2025_binding_shell.cpp"

_OFFICIAL_EXCEPTION = re.compile(r"^\s*RTI_EXCEPTION\((?P<name>[A-Za-z_]\w*)\)\s*$", re.MULTILINE)
_BINDING_EXCEPTION = re.compile(
    r'^\s*UMBRA_DEFINE_EXCEPTION\((?P<name>[A-Za-z_]\w*),\s*L"(?P<literal>[A-Za-z_]\w*)"\)\s*$',
    re.MULTILINE,
)


def _names(pattern: re.Pattern[str], source: str) -> list[str]:
    return [match.group("name") for match in pattern.finditer(source)]


def verify() -> tuple[str, ...]:
    """Return deterministic findings, or an empty tuple when the binding matches."""
    try:
        official_source = OFFICIAL_HEADER.read_text(encoding="utf-8")
        binding_source = BINDING_SOURCE.read_text(encoding="utf-8")
    except OSError as error:
        return (f"cannot read exception baseline: {error}",)

    official = _names(_OFFICIAL_EXCEPTION, official_source)
    definitions = list(_BINDING_EXCEPTION.finditer(binding_source))
    defined = [match.group("name") for match in definitions]
    literals = {match.group("name"): match.group("literal") for match in definitions}

    findings: list[str] = []
    if len(official) != len(set(official)):
        findings.append("official Exception.h contains duplicate RTI_EXCEPTION declarations")
    if len(defined) != len(set(defined)):
        findings.append("binding source contains duplicate UMBRA_DEFINE_EXCEPTION definitions")

    official_set = set(official)
    defined_set = set(defined)
    findings.extend(f"missing binding definition for {name}" for name in sorted(official_set - defined_set))
    findings.extend(f"unexpected binding definition for {name}" for name in sorted(defined_set - official_set))
    findings.extend(
        f"binding exception literal drift for {name}"
        for name in sorted(official_set & defined_set)
        if literals[name] != name
    )
    return tuple(findings)


def main() -> int:
    findings = verify()
    if findings:
        print("IEEE exception binding: FAIL", file=sys.stderr)
        for finding in findings:
            print(f"- {finding}", file=sys.stderr)
        return 1
    print("IEEE exception binding: PASS (109 official types)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
