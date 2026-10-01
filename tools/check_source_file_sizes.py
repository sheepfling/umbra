"""Check source-file size policy without rewriting legacy files.

The policy is intentionally incremental. Existing oversized files are listed in
the checked-in baseline and may shrink, but they may not grow. New files must
stay below the hard limit for their category. Use ``--report`` for the bounded
size survey used when choosing the next split.
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any

REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_CONFIG = REPOSITORY_ROOT / "docs" / "development" / "source-size-policy.json"


@dataclass(frozen=True)
class Policy:
    soft: int
    hard: int


def _load_config(path: Path) -> tuple[dict[str, Policy], dict[str, dict[str, Any]]]:
    payload = json.loads(path.read_text(encoding="utf-8"))
    policies = {
        name: Policy(int(values["soft"]), int(values["hard"]))
        for name, values in payload["policies"].items()
    }
    baseline = {item["path"]: item for item in payload.get("legacy_exemptions", [])}
    return policies, baseline


def _tracked_paths() -> list[Path]:
    result = subprocess.run(
        ["git", "ls-files", "-z", "--cached", "--others", "--exclude-standard"],
        cwd=REPOSITORY_ROOT,
        check=True,
        stdout=subprocess.PIPE,
    )
    return [
        REPOSITORY_ROOT / raw.decode("utf-8")
        for raw in result.stdout.split(b"\0")
        if raw
    ]


def _classify(path: Path) -> tuple[str, Policy] | None:
    relative = path.relative_to(REPOSITORY_ROOT).as_posix()
    suffix = path.suffix.lower()

    if relative == "CMakeLists.txt":
        return "cmake", Policy(soft=3000, hard=8000)
    if relative.startswith("cpp/tests/") and suffix == ".cpp":
        return "test_cpp", Policy(soft=2500, hard=5000)
    if relative.startswith("cpp/") and suffix in {".cpp", ".cc", ".cxx"}:
        return "production_cpp", Policy(soft=2000, hard=4000)
    if relative.startswith("cpp/") and suffix in {".h", ".hh", ".hpp"}:
        return "header_cpp", Policy(soft=1200, hard=3000)
    if relative.startswith("docs/") and suffix == ".md":
        return "documentation", Policy(soft=3000, hard=6000)
    return None


def _line_count(path: Path) -> int:
    with path.open("rb") as stream:
        return sum(1 for _ in stream)


def _scan(
    policies: dict[str, Policy],
    baseline: dict[str, dict[str, Any]],
) -> tuple[list[dict[str, Any]], list[str]]:
    findings: list[dict[str, Any]] = []
    errors: list[str] = []
    present = set()

    for path in _tracked_paths():
        classified = _classify(path)
        if classified is None:
            continue
        kind, default_policy = classified
        policy = policies.get(kind, default_policy)
        relative = path.relative_to(REPOSITORY_ROOT).as_posix()
        present.add(relative)
        lines = _line_count(path)
        entry = {
            "path": relative,
            "kind": kind,
            "lines": lines,
            "soft": policy.soft,
            "hard": policy.hard,
        }
        if lines > policy.soft:
            findings.append(entry)

        exemption = baseline.get(relative)
        if exemption is not None:
            maximum = int(exemption["max_lines"])
            if lines > maximum:
                errors.append(
                    f"legacy file grew: {relative} has {lines} lines; "
                    f"baseline is {maximum}"
                )
        elif lines > policy.hard:
            errors.append(
                f"file exceeds {kind} hard limit without a legacy exemption: "
                f"{relative} has {lines} lines; limit is {policy.hard}"
            )

    for relative in sorted(set(baseline) - present):
        errors.append(f"size-policy baseline names a missing source file: {relative}")
    return findings, errors


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--report", action="store_true", help="print every soft-limit finding"
    )
    parser.add_argument("--config", type=Path, default=DEFAULT_CONFIG)
    arguments = parser.parse_args(argv)

    config_path = arguments.config
    if not config_path.is_absolute():
        config_path = REPOSITORY_ROOT / config_path
    policies, baseline = _load_config(config_path)
    findings, errors = _scan(policies, baseline)

    if arguments.report:
        for finding in sorted(
            findings, key=lambda item: (-item["lines"], item["path"])
        ):
            print(
                f"{finding['lines']:>6} lines  {finding['kind']:<15} "
                f"soft={finding['soft']:<5} hard={finding['hard']:<5} {finding['path']}"
            )
    else:
        print(
            "Source-size policy: "
            f"{len(findings)} files exceed a soft limit; "
            f"{len(baseline)} legacy exemptions are growth-guarded."
        )

    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    if errors and not arguments.report:
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
