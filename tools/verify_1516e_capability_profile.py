"""Validate an IEEE 1516e-2010 Java/Python capability profile.

The profile is a declaration of which catalog scenarios a provider is willing
to run; it is not conformance evidence.  This verifier makes the declaration
strict enough for transplanting: every ``scenario.<id>`` key must name a
scenario in one of the checked-in Java or Python 2010 catalogs, every key must
have a supported mode, and duplicate keys are rejected instead of silently
overwriting an earlier declaration.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_JAVA_CATALOG = ROOT / "compliance" / "catalogs" / "java-2010-tck-scenario-catalog.json"
DEFAULT_PYTHON_CATALOG = ROOT / "compliance" / "catalogs" / "python-2010-tck-scenario-catalog.json"
_MODES = {"run", "pass", "unsupported", "skip", "not applicable", "not_applicable", "not-applicable"}


def _load_catalog(path: Path) -> set[str]:
    value = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict) or value.get("standard") != "IEEE 1516.1-2010":
        raise ValueError(f"{path} is not an IEEE 1516.1-2010 catalog")
    scenarios = value.get("scenarios")
    if not isinstance(scenarios, list) or not scenarios:
        raise ValueError(f"{path} has no scenario array")
    identifiers: set[str] = set()
    for scenario in scenarios:
        if not isinstance(scenario, dict) or not isinstance(scenario.get("id"), str):
            raise ValueError(f"{path} contains a scenario without a string id")
        identifier = scenario["id"]
        if identifier in identifiers:
            raise ValueError(f"{path} contains duplicate scenario id {identifier!r}")
        identifiers.add(identifier)
    return identifiers


def _normalize_mode(value: str) -> str:
    normalized = value.lower().replace("_", " ").replace("-", " ").strip()
    if normalized in {"run", "pass"}:
        return "run"
    if normalized in {"unsupported", "skip"}:
        return "unsupported"
    if normalized == "not applicable":
        return "not applicable"
    raise ValueError(f"unknown capability profile mode: {value!r}")


def _parse_profile(path: Path) -> tuple[dict[str, str], list[str]]:
    profile: dict[str, str] = {}
    findings: list[str] = []
    if not path.is_file():
        return {}, [f"capability profile does not exist: {path.resolve()}"]
    for line_number, raw_line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        line = raw_line.strip()
        if not line or line.startswith(("#", ";")):
            continue
        if "=" not in line:
            findings.append(f"line {line_number} has no '=': {raw_line!r}")
            continue
        key, value = (part.strip() for part in line.split("=", 1))
        if not key.startswith("scenario.") or not key[len("scenario.") :]:
            findings.append(f"line {line_number} must use scenario.<id>: {key!r}")
            continue
        identifier = key[len("scenario.") :]
        if identifier in profile:
            findings.append(f"line {line_number} repeats scenario id {identifier!r}")
            continue
        try:
            profile[identifier] = _normalize_mode(value)
        except ValueError as error:
            findings.append(f"line {line_number}: {error}")
    return profile, findings


def verify(
    profile_path: Path,
    *,
    java_catalog: Path = DEFAULT_JAVA_CATALOG,
    python_catalog: Path = DEFAULT_PYTHON_CATALOG,
) -> dict[str, Any]:
    """Return a machine-readable profile validation result."""

    findings: list[str] = []
    try:
        known = _load_catalog(java_catalog) | _load_catalog(python_catalog)
    except (OSError, ValueError, json.JSONDecodeError) as error:
        return {
            "standard": "IEEE 1516.1-2010",
            "profile": str(profile_path),
            "status": "fail",
            "entries": 0,
            "findings": [f"cannot load scenario catalogs: {error}"],
        }
    try:
        profile, parse_findings = _parse_profile(profile_path)
    except (OSError, UnicodeDecodeError) as error:
        profile, parse_findings = {}, [f"cannot read capability profile: {error}"]
    findings.extend(parse_findings)
    for identifier in sorted(profile):
        if identifier not in known:
            findings.append(f"unknown scenario id {identifier!r}; update the 2010 catalog before using it")
    return {
        "standard": "IEEE 1516.1-2010",
        "profile": str(profile_path.resolve()),
        "status": "fail" if findings else "pass",
        "entries": len(profile),
        "findings": findings,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--profile", type=Path, required=True)
    parser.add_argument("--java-catalog", type=Path, default=DEFAULT_JAVA_CATALOG)
    parser.add_argument("--python-catalog", type=Path, default=DEFAULT_PYTHON_CATALOG)
    parser.add_argument("--json", action="store_true", help="emit machine-readable JSON")
    args = parser.parse_args()
    result = verify(args.profile, java_catalog=args.java_catalog, python_catalog=args.python_catalog)
    if args.json:
        print(json.dumps(result, indent=2))
    elif result["findings"]:
        print("IEEE 1516e capability-profile verification: FAIL", file=sys.stderr)
        for finding in result["findings"]:
            print(f"  {finding}", file=sys.stderr)
    else:
        print(
            "IEEE 1516e capability-profile verification: PASS "
            f"({result['entries']} catalog-linked entries; declaration only)"
        )
    return 1 if result["status"] == "fail" else 0


if __name__ == "__main__":
    raise SystemExit(main())
