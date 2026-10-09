"""Check source-file size policy without rewriting legacy files.

The policy is intentionally incremental. Files over 3,000 lines are review
warnings; files over 5,000 deserve a search for a real ownership or behavior
boundary, not a line-only extraction. The default code ceiling is 10,000
expanded lines; cohesive files may remain intact when no safe source boundary
exists, with an explicit growth-guarded exception. Included ``.inc`` bodies
count toward their owning translation unit. Existing exceptions are listed in
the checked-in baseline and may shrink, but they may not grow. New files must
stay below their category's hard limit unless an explicit exception is recorded.
Use ``--report`` for the size survey used when choosing the next split.
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any

REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_CONFIG = REPOSITORY_ROOT / "docs" / "development" / "source-size-policy.json"
LOCAL_INCLUDE_PATTERN = re.compile(r'^\s*#\s*include\s*"([^"]+\.inc)"')
TRANSLATION_UNIT_SUFFIXES = {".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp"}


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

    if relative == "CMakeLists.txt" or (
        relative.startswith("cmake/") and suffix == ".cmake"
    ):
        return "cmake", Policy(soft=3000, hard=10000)
    if relative.startswith("cpp/tests/") and suffix in {".cpp", ".inc"}:
        return "test_cpp", Policy(soft=3000, hard=10000)
    if relative.startswith("cpp/src/") and suffix == ".inc":
        return "production_cpp", Policy(soft=3000, hard=10000)
    if relative.startswith("cpp/") and suffix in {".cpp", ".cc", ".cxx"}:
        return "production_cpp", Policy(soft=3000, hard=10000)
    if relative.startswith("cpp/") and suffix in {".h", ".hh", ".hpp"}:
        return "header_cpp", Policy(soft=3000, hard=10000)
    if relative.startswith("docs/") and suffix == ".md":
        return "documentation", Policy(soft=3000, hard=6000)
    return None


def _line_count(path: Path) -> int:
    with path.open("rb") as stream:
        return sum(1 for _ in stream)


def _resolve_local_fragment(
    includer: Path,
    include_name: str,
    repository_paths: set[Path],
) -> Path | None:
    include_path = Path(include_name)
    search_roots = (
        includer.parent,
        REPOSITORY_ROOT,
        REPOSITORY_ROOT / "cpp" / "src",
        REPOSITORY_ROOT / "cpp" / "tests",
    )
    for root in search_roots:
        candidate = (root / include_path).resolve()
        if candidate in repository_paths:
            return candidate
    return None


def _expanded_line_count(
    path: Path,
    repository_paths: set[Path],
    cache: dict[Path, tuple[int, tuple[Path, ...]]],
    active: frozenset[Path] = frozenset(),
) -> tuple[int, tuple[Path, ...]]:
    """Count a source and its recursively included local .inc fragments."""
    path = path.resolve()
    cached = cache.get(path)
    if cached is not None:
        return cached

    physical_lines = _line_count(path)
    if path in active or path.suffix.lower() not in TRANSLATION_UNIT_SUFFIXES | {".inc"}:
        return physical_lines, ()

    expanded_lines = physical_lines
    fragments: list[Path] = []
    next_active = active | {path}
    for line in path.read_text(encoding="utf-8").splitlines():
        match = LOCAL_INCLUDE_PATTERN.match(line)
        if match is None:
            continue
        fragment = _resolve_local_fragment(path, match.group(1), repository_paths)
        if fragment is None or fragment in next_active:
            continue
        fragment_lines, nested_fragments = _expanded_line_count(
            fragment, repository_paths, cache, next_active
        )
        expanded_lines += fragment_lines
        fragments.append(fragment)
        fragments.extend(nested_fragments)

    result = expanded_lines, tuple(fragments)
    cache[path] = result
    return result


def _scan(
    policies: dict[str, Policy],
    baseline: dict[str, dict[str, Any]],
) -> tuple[list[dict[str, Any]], list[str]]:
    findings: list[dict[str, Any]] = []
    errors: list[str] = []
    tracked_paths = _tracked_paths()
    repository_paths = {path.resolve() for path in tracked_paths}
    expanded_cache: dict[Path, tuple[int, tuple[Path, ...]]] = {}
    present = set()

    for path in tracked_paths:
        classified = _classify(path)
        if classified is None:
            continue
        kind, default_policy = classified
        policy = policies.get(kind, default_policy)
        relative = path.relative_to(REPOSITORY_ROOT).as_posix()
        present.add(relative)
        physical_lines = _line_count(path)
        lines = physical_lines
        included_fragments: tuple[Path, ...] = ()
        if path.suffix.lower() in TRANSLATION_UNIT_SUFFIXES:
            lines, included_fragments = _expanded_line_count(
                path, repository_paths, expanded_cache
            )
        entry = {
            "path": relative,
            "kind": kind,
            "lines": lines,
            "physical_lines": physical_lines,
            "included_fragments": len(included_fragments),
            "soft": policy.soft,
            "hard": policy.hard,
        }
        if lines > policy.soft:
            findings.append(entry)

        exemption = baseline.get(relative)
        if exemption is not None:
            maximum = int(exemption["max_lines"])
            if physical_lines > maximum:
                errors.append(
                    f"legacy file grew: {relative} has {physical_lines} physical lines; "
                    f"baseline is {maximum}"
                )
            if lines > policy.hard and included_fragments:
                expanded_maximum = exemption.get("max_expanded_lines")
                if expanded_maximum is None or lines > int(expanded_maximum):
                    errors.append(
                        f"legacy source plus included fragments exceeds its ratchet: "
                        f"{relative} has {lines} expanded lines; baseline is "
                        f"{expanded_maximum if expanded_maximum is not None else 'unrecorded'}"
                    )
        elif lines > policy.hard:
            errors.append(
                f"file exceeds {kind} hard limit without a legacy exemption: "
                f"{relative} has {lines} expanded lines; limit is {policy.hard}"
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
                f"soft={finding['soft']:<5} hard={finding['hard']:<5} "
                f"{finding['path']}"
                + (
                    f" (file {finding['physical_lines']}, plus "
                    f"{finding['included_fragments']} included fragments)"
                    if finding["included_fragments"]
                    else ""
                )
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
