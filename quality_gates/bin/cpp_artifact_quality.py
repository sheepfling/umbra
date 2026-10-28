"""Inventory C++ build artifacts and enforce project supplied size budgets.

The scanner has no repository or build-system dependency. It measures file sizes
on every platform and uses GNU objdump (or llvm-objdump) when available to
measure debug sections in executables and shared libraries.
"""

from __future__ import annotations

import argparse
import fnmatch
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path
from typing import TypedDict, cast

DEFAULT_INCLUDES = (
    "*.exe",
    "*.dll",
    "*.so",
    "*.so.*",
    "*.dylib",
    "*.a",
    "*.lib",
    "*.pdb",
    "*.debug",
    "*.dwp",
    "*.dSYM/Contents/Resources/DWARF/*",
)
SECTION_RE = re.compile(r"^\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+", re.MULTILINE)
LIMIT_KEYS = {
    "min_executable_count",
    "max_executable_count",
    "min_static_library_count",
    "max_executable_bytes",
    "max_executable_debug_bytes",
    "max_static_library_bytes",
    "max_total_executable_bytes",
    "max_total_static_library_bytes",
    "max_total_artifact_bytes",
    "max_new_executable_bytes",
    "max_executable_growth_bytes",
    "max_total_executable_growth_bytes",
}
BASELINE_LIMIT_KEYS = {
    "max_new_executable_bytes",
    "max_executable_growth_bytes",
    "max_total_executable_growth_bytes",
}


class _SectionMetricsOptional(TypedDict, total=False):
    section_error: str


class SectionMetrics(_SectionMetricsOptional):
    section_status: str
    debug_bytes: int | None
    text_bytes: int | None


class _ArtifactRecordOptional(TypedDict, total=False):
    section_status: str
    section_error: str
    debug_bytes: int | None
    text_bytes: int | None


class ArtifactRecord(_ArtifactRecordOptional):
    path: str
    kind: str
    bytes: int


class KindTotals(TypedDict):
    count: int
    bytes: int


class ArtifactTotals(TypedDict):
    count: int
    bytes: int
    executable_count: int
    executable_paths: list[str]
    executable_bytes: int
    static_library_count: int
    static_library_bytes: int
    measured_debug_bytes: int
    section_measured_count: int
    section_unmeasured_count: int
    by_kind: dict[str, KindTotals]


class DebugDominantSignal(TypedDict):
    code: str
    count: int
    total_debug_bytes: int
    examples: list[str]


class _ArtifactReportOptional(TypedDict, total=False):
    run_id: str | None
    manifest_sha256: str
    gate: ArtifactEvaluation


class ArtifactReport(_ArtifactReportOptional):
    schema: int
    root: str
    include: list[str]
    exclude: list[str]
    include_unix_executables: bool
    section_tool: str | None
    artifacts: list[ArtifactRecord]
    totals: ArtifactTotals
    review_signals: list[DebugDominantSignal]


class _ArtifactPolicyOptional(TypedDict, total=False):
    name: str
    include: list[str]
    exclude: list[str]


class ArtifactPolicy(_ArtifactPolicyOptional):
    schema: int
    limits: dict[str, int]


class _ArtifactFindingOptional(TypedDict, total=False):
    path: str


class ArtifactFinding(_ArtifactFindingOptional):
    code: str
    observed: int | None
    limit: int


class ArtifactComparison(TypedDict):
    new_artifacts: list[str]
    removed_artifacts: list[str]
    total_executable_growth_bytes: int


class ArtifactEvaluation(TypedDict):
    passed: bool
    policy: str
    findings: list[ArtifactFinding]
    comparison: ArtifactComparison | None


def _matches(path: str, patterns: tuple[str, ...] | list[str]) -> bool:
    folded = path.casefold()
    return any(fnmatch.fnmatchcase(folded, pattern.casefold()) for pattern in patterns)


def _kind(path: Path) -> str:
    name = path.name.casefold()
    if ".dsym/contents/resources/dwarf/" in path.as_posix().casefold():
        return "symbols"
    if name.endswith(".exe"):
        return "executable"
    if name.endswith((".dll", ".dylib", ".so")) or ".so." in name:
        return "shared_library"
    if name.endswith((".a", ".lib")):
        return "static_library"
    if name.endswith((".pdb", ".debug", ".dwp")):
        return "symbols"
    with path.open("rb") as binary:
        magic = binary.read(4)
    if (
        magic.startswith(b"MZ")
        or magic == b"\x7fELF"
        or magic
        in {
            b"\xfe\xed\xfa\xce",
            b"\xce\xfa\xed\xfe",
            b"\xfe\xed\xfa\xcf",
            b"\xcf\xfa\xed\xfe",
        }
    ):
        return "executable"
    return "other"


def _default_unix_executable(path: Path) -> bool:
    """Include extensionless native executables on Unix without reading all files."""
    return os.name != "nt" and os.access(path, os.X_OK) and _kind(path) == "executable"


def parse_objdump_sections(output: str) -> dict[str, int]:
    """Return section sizes in bytes from GNU/LLVM objdump -h output."""
    sections: dict[str, int] = {}
    for name, hexadecimal_size in SECTION_RE.findall(output):
        sections[name] = sections.get(name, 0) + int(hexadecimal_size, 16)
    if not sections:
        raise ValueError("objdump returned no section headers")
    return sections


def _section_metrics(path: Path, section_tool: str | None) -> SectionMetrics:
    if section_tool is None:
        return {
            "section_status": "unavailable",
            "section_error": "objdump was not found",
            "debug_bytes": None,
            "text_bytes": None,
        }
    try:
        result = subprocess.run(
            [section_tool, "-h", str(path)],
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=30,
            check=False,
        )
        if result.returncode:
            raise ValueError(result.stderr.strip()[:240] or "objdump failed")
        sections = parse_objdump_sections(result.stdout)
    except (OSError, ValueError, subprocess.TimeoutExpired) as error:
        return {
            "section_status": "error",
            "section_error": str(error)[:240],
            "debug_bytes": None,
            "text_bytes": None,
        }
    debug_bytes = sum(
        size
        for name, size in sections.items()
        if name.startswith((".debug", ".zdebug"))
        or name in {".gnu_debugdata", ".stab", ".stabstr"}
    )
    text_bytes = sum(
        size for name, size in sections.items() if name.startswith(".text")
    )
    return cast(
        SectionMetrics,
        {
            "section_status": "measured",
            "debug_bytes": debug_bytes,
            "text_bytes": text_bytes,
        },
    )


def scan_directory(
    root: Path,
    *,
    includes: tuple[str, ...] | list[str] = DEFAULT_INCLUDES,
    excludes: tuple[str, ...] | list[str] = (),
    section_tool: str = "objdump",
    measure_sections: bool = True,
    include_unix_executables: bool = True,
) -> ArtifactReport:
    root = root.resolve()
    if not root.is_dir():
        raise ValueError(f"artifact root is not a directory: {root}")
    includes = tuple(includes)
    excludes = tuple(excludes)
    if not includes:
        raise ValueError("at least one include pattern is required")
    analyzer = shutil.which(section_tool) if measure_sections else None
    artifacts: list[ArtifactRecord] = []
    for path in root.rglob("*"):
        if path.is_symlink() or not path.is_file():
            continue
        relative = path.relative_to(root).as_posix()
        if (
            not _matches(relative, includes)
            and not (include_unix_executables and _default_unix_executable(path))
        ) or _matches(relative, excludes):
            continue
        kind = _kind(path)
        item: ArtifactRecord = {
            "path": relative,
            "kind": kind,
            "bytes": path.stat().st_size,
        }
        if kind in {"executable", "shared_library"}:
            if measure_sections:
                metrics = _section_metrics(path, analyzer)
                item["section_status"] = metrics["section_status"]
                item["debug_bytes"] = metrics["debug_bytes"]
                item["text_bytes"] = metrics["text_bytes"]
                if "section_error" in metrics:
                    item["section_error"] = metrics["section_error"]
            else:
                item["section_status"] = "skipped"
                item["debug_bytes"] = None
                item["text_bytes"] = None
        artifacts.append(item)
    artifacts.sort(key=lambda item: item["path"])
    executables = [item for item in artifacts if item["kind"] == "executable"]
    static_libraries = [item for item in artifacts if item["kind"] == "static_library"]
    by_kind: dict[str, KindTotals] = {}
    for item in artifacts:
        bucket = by_kind.setdefault(item["kind"], {"count": 0, "bytes": 0})
        bucket["count"] += 1
        bucket["bytes"] += item["bytes"]
    debug_dominant: list[ArtifactRecord] = []
    for item in executables:
        debug_bytes = item.get("debug_bytes")
        if (
            debug_bytes is not None
            and debug_bytes >= 64 * 1024 * 1024
            and debug_bytes * 2 >= item["bytes"]
        ):
            debug_dominant.append(item)
    debug_dominant.sort(key=lambda item: item.get("debug_bytes") or 0, reverse=True)
    signals: list[DebugDominantSignal] = []
    if debug_dominant:
        signals.append(
            {
                "code": "debug_dominant_executables",
                "count": len(debug_dominant),
                "total_debug_bytes": sum(
                    item.get("debug_bytes") or 0 for item in debug_dominant
                ),
                "examples": [item["path"] for item in debug_dominant[:8]],
            }
        )
    return {
        "schema": 1,
        "root": str(root),
        "include": list(includes),
        "exclude": list(excludes),
        "include_unix_executables": include_unix_executables,
        "section_tool": analyzer,
        "artifacts": artifacts,
        "totals": {
            "count": len(artifacts),
            "bytes": sum(item["bytes"] for item in artifacts),
            "executable_count": len(executables),
            "executable_paths": sorted(item["path"] for item in executables),
            "executable_bytes": sum(item["bytes"] for item in executables),
            "static_library_count": len(static_libraries),
            "static_library_bytes": sum(item["bytes"] for item in static_libraries),
            "measured_debug_bytes": sum(
                item.get("debug_bytes") or 0 for item in artifacts
            ),
            "section_measured_count": sum(
                item.get("section_status") == "measured" for item in artifacts
            ),
            "section_unmeasured_count": sum(
                item.get("section_status") in {"skipped", "unavailable", "error"}
                for item in artifacts
            ),
            "by_kind": by_kind,
        },
        "review_signals": signals,
    }


def _manifest_digest(path: Path, run_id: str | None) -> str:
    raw = path.read_bytes()
    manifest = json.loads(raw)
    if (
        not isinstance(manifest, dict)
        or type(manifest.get("schema")) is not int
        or manifest["schema"] != 1
        or manifest.get("kind") != "cpp-quality-build-manifest"
        or not isinstance(run_id, str)
        or not run_id
        or manifest.get("run_id") != run_id
    ):
        raise ValueError("manifest needs schema 1, build kind, and matching --run-id")
    return hashlib.sha256(raw).hexdigest()


def load_policy(path: Path) -> ArtifactPolicy:
    policy = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(policy, dict) or policy.get("schema") != 1:
        raise ValueError("policy must be a schema 1 JSON object")
    limits = policy.get("limits")
    if not isinstance(limits, dict) or not limits:
        raise ValueError("policy requires a nonempty limits object")
    unknown = set(limits) - LIMIT_KEYS
    if unknown:
        raise ValueError("unknown policy limit(s): " + ", ".join(sorted(unknown)))
    for name, value in limits.items():
        if isinstance(value, bool) or not isinstance(value, int) or value < 0:
            raise ValueError(f"{name} must be a nonnegative integer")
    for name in ("include", "exclude"):
        patterns = policy.get(name, [])
        if not isinstance(patterns, list) or not all(
            isinstance(pattern, str) and pattern for pattern in patterns
        ):
            raise ValueError(f"{name} must be a list of nonempty glob patterns")
    policy_name = policy.get("name")
    if policy_name is not None and not isinstance(policy_name, str):
        raise ValueError("policy name must be a string")
    return cast(ArtifactPolicy, policy)


def _finding(
    code: str, observed: int | None, limit: int, path: str | None = None
) -> ArtifactFinding:
    result: ArtifactFinding = {
        "code": code,
        "observed": observed,
        "limit": limit,
    }
    if path is not None:
        result["path"] = path
    return result


def evaluate(
    report: ArtifactReport,
    policy: ArtifactPolicy,
    baseline: ArtifactReport | None = None,
) -> ArtifactEvaluation:
    limits = policy["limits"]
    if BASELINE_LIMIT_KEYS.intersection(limits) and baseline is None:
        raise ValueError("growth limits require --baseline")
    if baseline is not None:
        if baseline.get("schema") != 1 or not isinstance(
            baseline.get("artifacts"), list
        ):
            raise ValueError("baseline must be a schema 1 artifact report")
        if (
            report["include"],
            report["exclude"],
            report["include_unix_executables"],
        ) != (
            baseline.get("include"),
            baseline.get("exclude"),
            baseline.get("include_unix_executables"),
        ):
            raise ValueError("baseline and current scan selection differ")
    findings: list[ArtifactFinding] = []
    artifacts = report["artifacts"]
    executables = [item for item in artifacts if item["kind"] == "executable"]
    static_libraries = [item for item in artifacts if item["kind"] == "static_library"]
    for name, field in (
        ("min_executable_count", "executable_count"),
        ("max_executable_count", "executable_count"),
        ("max_total_executable_bytes", "executable_bytes"),
        ("max_total_artifact_bytes", "bytes"),
    ):
        if name not in limits:
            continue
        observed = cast(int, report["totals"][field])
        violated = (
            observed < limits[name]
            if name.startswith("min_")
            else observed > limits[name]
        )
        if violated:
            findings.append(_finding(name, observed, limits[name]))
    for name, observed in (
        ("min_static_library_count", report["totals"]["static_library_count"]),
        ("max_total_static_library_bytes", report["totals"]["static_library_bytes"]),
    ):
        if name in limits:
            violated = (
                observed < limits[name]
                if name.startswith("min_")
                else observed > limits[name]
            )
            if violated:
                findings.append(_finding(name, observed, limits[name]))
    if "max_static_library_bytes" in limits:
        for item in static_libraries:
            if item["bytes"] > limits["max_static_library_bytes"]:
                findings.append(
                    _finding(
                        "max_static_library_bytes",
                        item["bytes"],
                        limits["max_static_library_bytes"],
                        item["path"],
                    )
                )
    previous = (
        {item["path"]: item for item in baseline["artifacts"]}
        if baseline is not None
        else {}
    )
    current_paths = {item["path"] for item in artifacts}
    for item in executables:
        path = item["path"]
        if (
            "max_executable_bytes" in limits
            and item["bytes"] > limits["max_executable_bytes"]
        ):
            findings.append(
                _finding(
                    "max_executable_bytes",
                    item["bytes"],
                    limits["max_executable_bytes"],
                    path,
                )
            )
        if "max_executable_debug_bytes" in limits:
            debug_bytes = item.get("debug_bytes")
            if (
                debug_bytes is None
                or debug_bytes > limits["max_executable_debug_bytes"]
            ):
                findings.append(
                    _finding(
                        "max_executable_debug_bytes",
                        debug_bytes,
                        limits["max_executable_debug_bytes"],
                        path,
                    )
                )
        old = previous.get(path)
        if old is None:
            if (
                "max_new_executable_bytes" in limits
                and item["bytes"] > limits["max_new_executable_bytes"]
            ):
                findings.append(
                    _finding(
                        "max_new_executable_bytes",
                        item["bytes"],
                        limits["max_new_executable_bytes"],
                        path,
                    )
                )
        elif (
            "max_executable_growth_bytes" in limits
            and item["bytes"] - old["bytes"] > limits["max_executable_growth_bytes"]
        ):
            findings.append(
                _finding(
                    "max_executable_growth_bytes",
                    item["bytes"] - old["bytes"],
                    limits["max_executable_growth_bytes"],
                    path,
                )
            )
    comparison: ArtifactComparison | None = None
    if baseline is not None:
        comparison = {
            "new_artifacts": sorted(
                item["path"] for item in artifacts if item["path"] not in previous
            ),
            "removed_artifacts": sorted(
                path for path in previous if path not in current_paths
            ),
            "total_executable_growth_bytes": (
                report["totals"]["executable_bytes"]
                - baseline["totals"]["executable_bytes"]
            ),
        }
        if (
            "max_total_executable_growth_bytes" in limits
            and comparison["total_executable_growth_bytes"]
            > limits["max_total_executable_growth_bytes"]
        ):
            findings.append(
                _finding(
                    "max_total_executable_growth_bytes",
                    comparison["total_executable_growth_bytes"],
                    limits["max_total_executable_growth_bytes"],
                )
            )
    return {
        "passed": not findings,
        "policy": policy.get("name", "unnamed"),
        "findings": findings,
        "comparison": comparison,
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    for command in ("scan", "check"):
        child = commands.add_parser(command)
        child.add_argument("root", type=Path)
        child.add_argument("--include", action="append")
        child.add_argument("--exclude", action="append")
        child.add_argument("--objdump", default="objdump")
        child.add_argument("--no-sections", action="store_true")
        child.add_argument("--run-id")
        child.add_argument("--manifest", type=Path)
        child.add_argument("--output", type=Path)
        if command == "check":
            child.add_argument("--policy", required=True, type=Path)
            child.add_argument("--baseline", type=Path)
    arguments = parser.parse_args(argv)
    try:
        policy = load_policy(arguments.policy) if arguments.command == "check" else None
        policy_includes = policy.get("include") if policy else None
        policy_excludes = policy.get("exclude", []) if policy else []
        includes = arguments.include or policy_includes or DEFAULT_INCLUDES
        excludes = arguments.exclude or policy_excludes
        report = scan_directory(
            arguments.root,
            includes=includes,
            excludes=excludes,
            section_tool=arguments.objdump,
            measure_sections=not arguments.no_sections,
            include_unix_executables=not arguments.include and not policy_includes,
        )
        report["run_id"] = arguments.run_id
        if arguments.manifest:
            report["manifest_sha256"] = _manifest_digest(
                arguments.manifest, arguments.run_id
            )
        if policy is not None:
            baseline = (
                json.loads(arguments.baseline.read_text(encoding="utf-8"))
                if arguments.baseline
                else None
            )
            report["gate"] = evaluate(report, policy, baseline)
        output = json.dumps(report, indent=2, sort_keys=True) + "\n"
        if arguments.output:
            arguments.output.parent.mkdir(parents=True, exist_ok=True)
            arguments.output.write_text(output, encoding="utf-8")
            totals = report["totals"]
            print(
                f"artifacts={totals['count']} executables={totals['executable_count']} "
                f"bytes={totals['bytes']} "
                f"findings={len(report.get('gate', {}).get('findings', []))} "
                f"report={arguments.output}"
            )
            for finding in report.get("gate", {}).get("findings", []):
                location = f" {finding['path']}" if "path" in finding else ""
                print(
                    f"FAIL {finding['code']}{location}: "
                    f"observed={finding['observed']} limit={finding['limit']}"
                )
        else:
            print(output, end="")
        return 0 if report.get("gate", {}).get("passed", True) else 1
    except (OSError, ValueError, KeyError, json.JSONDecodeError) as error:
        print(f"artifact quality: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
