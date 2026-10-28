"""Summarize Alphus's C++ reports and keep local per-run history."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import subprocess
import sys
from collections import Counter
from collections.abc import Mapping, Sequence
from dataclasses import asdict, dataclass
from datetime import datetime, timezone
from importlib.metadata import PackageNotFoundError, version
from pathlib import Path
from statistics import median
from typing import cast

import tomllib

REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
LIZARD_STEP_NAME = "C++ cyclomatic complexity candidates (Lizard)"
LIZARD_ROW = re.compile(
    r"^(?P<path>.+):(?P<line>\d+): warning: (?P<function>.+) has "
    r"(?P<nloc>\d+) NLOC, (?P<ccn>\d+) CCN, (?P<token>\d+) token, "
    r"(?P<parameters>\d+) PARAM, (?P<length>\d+) length, "
    r"(?P<nd>\d+) ND\s*$"
)


@dataclass(frozen=True, slots=True)
class LizardFunction:
    path: str
    line: int
    function: str
    nloc: int
    ccn: int
    tokens: int
    parameters: int
    length: int
    nd: int


# opaque-boundary: Validate JSON objects emitted by the selected scanners.
def _require_mapping(value: object, label: str) -> Mapping[str, object]:
    if not isinstance(value, dict) or any(not isinstance(key, str) for key in value):
        raise TypeError(f"{label} must be a JSON object with string keys")
    return cast(Mapping[str, object], value)


# opaque-boundary: Validate the external Alphus finding-row representation.
def _require_findings(value: object, label: str) -> Sequence[Mapping[str, object]]:
    if not isinstance(value, list) or any(not isinstance(item, dict) for item in value):
        raise TypeError(f"{label} must be a JSON array of objects")
    return cast(Sequence[Mapping[str, object]], value)


# opaque-boundary: Parse and validate a report at the file boundary.
def _load_json(path: Path) -> Mapping[str, object]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise ValueError(f"cannot read report {path}: {error}") from error
    return _require_mapping(value, f"report {path}")


# opaque-boundary: Count validated finding fields without changing scanner data.
def _counter(items: Sequence[Mapping[str, object]], field: str) -> dict[str, int]:
    counts = Counter(str(item.get(field, "unknown")) for item in items)
    return dict(sorted(counts.items()))


# opaque-boundary: Rank validated scanner paths for the summary report.
def _top_paths(
    items: Sequence[Mapping[str, object]], *, limit: int = 10
) -> list[dict[str, object]]:
    counts = Counter(str(item["path"]) for item in items if item.get("path"))
    return [
        {"path": path, "finding_count": count}
        for path, count in counts.most_common(limit)
    ]


# opaque-boundary: Summarize the pinned Alphus source report schema.
def _source_summary(report: Mapping[str, object]) -> dict[str, object]:
    findings = _require_findings(report.get("findings"), "C++ source findings")
    metrics = _require_mapping(report.get("metrics"), "C++ source metrics")
    return {
        "files_scanned": metrics.get("file_count"),
        "fragment_count": metrics.get("fragment_count"),
        "finding_count": len(findings),
        "findings_by_severity": _counter(findings, "severity"),
        "findings_by_code": _counter(findings, "code"),
        "files_with_findings": len(
            {str(item.get("path")) for item in findings if item.get("path")}
        ),
        "top_finding_paths": _top_paths(findings),
        "largest_files": metrics.get("largest", [])[:10],
        "passed": report.get("passed"),
    }


# opaque-boundary: Summarize the pinned Alphus style report schema.
def _style_summary(report: Mapping[str, object]) -> dict[str, object]:
    findings = _require_findings(report.get("findings"), "C++ style findings")
    metrics = _require_mapping(report.get("metrics"), "C++ style metrics")
    return {
        "files_scanned": metrics.get("source_file_count"),
        "include_count": metrics.get("include_count"),
        "finding_count": metrics.get("finding_count", len(findings)),
        "findings_reported": len(findings),
        "findings_by_severity": _counter(findings, "severity"),
        "findings_by_confidence": _counter(findings, "confidence"),
        "findings_by_code": _counter(findings, "code"),
        "files_with_findings": len(
            {str(item.get("path")) for item in findings if item.get("path")}
        ),
        "top_finding_paths": _top_paths(findings),
        "possible_unused_include_count": metrics.get("possible_unused_include_count"),
        "estimated_high_complexity_count": metrics.get("high_complexity_count"),
        "large_file_count": metrics.get("large_file_count"),
        "duplicate_code_groups": {
            "total": metrics.get("duplicate_code_group_count"),
            "reported": metrics.get("duplicate_code_groups_reported"),
            "truncated": metrics.get("duplicate_code_groups_truncated"),
        },
        "blocking": report.get("blocking"),
        "status": report.get("status"),
    }


def _parse_lizard(path: Path, root: Path) -> list[LizardFunction]:
    try:
        lines = path.read_text(encoding="utf-8").splitlines()
    except OSError as error:
        raise ValueError(f"cannot read Lizard report {path}: {error}") from error

    records: list[LizardFunction] = []
    malformed: list[int] = []
    for line_number, line in enumerate(lines, start=1):
        if not line.strip():
            continue
        match = LIZARD_ROW.match(line)
        if match is None:
            malformed.append(line_number)
            continue
        record = match.groupdict()
        raw_path = Path(record["path"])
        try:
            display_path = raw_path.resolve().relative_to(root).as_posix()
        except (OSError, ValueError):
            display_path = record["path"]
        records.append(
            LizardFunction(
                path=display_path,
                line=int(record["line"]),
                function=record["function"],
                nloc=int(record["nloc"]),
                ccn=int(record["ccn"]),
                tokens=int(record["token"]),
                parameters=int(record["parameters"]),
                length=int(record["length"]),
                nd=int(record["nd"]),
            )
        )
    if malformed:
        preview = ", ".join(str(number) for number in malformed[:8])
        raise ValueError(
            f"Lizard report has {len(malformed)} unrecognized non-empty line(s) "
            f"at line(s) {preview}; statistics were not recorded"
        )
    return records


def _lizard_threshold(config_path: Path) -> int:
    try:
        config = tomllib.loads(config_path.read_text(encoding="utf-8"))
    except (OSError, tomllib.TOMLDecodeError) as error:
        raise ValueError(f"cannot read Alphus config {config_path}: {error}") from error
    steps = config.get("orchestration", {}).get("steps", [])
    for step in steps:
        if step.get("name") != LIZARD_STEP_NAME:
            continue
        argv = step.get("argv", [])
        try:
            return int(argv[argv.index("--CCN") + 1])
        except (ValueError, IndexError, TypeError) as error:
            raise ValueError("Lizard step has no valid --CCN threshold") from error
    raise ValueError(f"Alphus config has no step named {LIZARD_STEP_NAME!r}")


# opaque-boundary: Emit a JSON-ready aggregate from parsed Lizard rows.
def _lizard_summary(
    records: Sequence[LizardFunction], threshold: int
) -> dict[str, object]:
    ranked = sorted(
        records,
        key=lambda item: (-item.ccn, -item.nloc, item.path, item.line),
    )
    ccn_values = [record.ccn for record in records]
    nloc_values = [record.nloc for record in records]
    return {
        "configured_ccn_threshold": threshold,
        "reported_function_count": len(records),
        "files_with_reported_functions": len(
            {record.path.casefold() for record in records}
        ),
        "ccn": {
            "maximum": max(ccn_values, default=0),
            "median": median(ccn_values) if ccn_values else 0,
        },
        "nloc": {
            "maximum": max(nloc_values, default=0),
            "median": median(nloc_values) if nloc_values else 0,
        },
        "top_functions": [asdict(record) for record in ranked[:10]],
    }


def _sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


# opaque-boundary: Emit Git state as JSON fields in the run record.
def _git_context(root: Path) -> dict[str, object]:
    try:
        commit = subprocess.run(
            ["git", "rev-parse", "HEAD"],
            cwd=root,
            check=True,
            capture_output=True,
            text=True,
        ).stdout.strip()
        status = subprocess.run(
            ["git", "status", "--porcelain", "--untracked-files=normal"],
            cwd=root,
            check=True,
            capture_output=True,
            text=True,
        ).stdout.splitlines()
    except (OSError, subprocess.CalledProcessError):
        return {"commit": None, "working_tree_dirty": None, "changed_path_count": None}
    return {
        "commit": commit,
        "working_tree_dirty": bool(status),
        "changed_path_count": len(status),
    }


def _tool_version(package: str) -> str | None:
    try:
        return version(package)
    except PackageNotFoundError:
        return None


# opaque-boundary: Serialize the validated run-summary object to JSON.
def _atomic_write_json(path: Path, value: Mapping[str, object]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + ".tmp")
    temporary.write_text(
        json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    temporary.replace(path)


# opaque-boundary: Assemble one JSON-ready record from the local run evidence.
def summarize(root: Path, output: Path, history: Path) -> dict[str, object]:
    root = root.resolve()
    quality = root / "out" / "quality"
    source_report = _load_json(quality / "alphus-cpp-source.json")
    style_report = _load_json(quality / "alphus-cpp-style.json")
    lizard_path = quality / "alphus-cpp-complexity.txt"
    threshold = _lizard_threshold(root / "quality-gates.toml")
    source = _source_summary(source_report)
    style = _style_summary(style_report)
    lizard = _lizard_summary(_parse_lizard(lizard_path, root), threshold)
    timestamp = datetime.now(timezone.utc)

    report = {
        "schema": 1,
        "run_id": timestamp.strftime("%Y%m%dT%H%M%S.%fZ"),
        "generated_at_utc": timestamp.isoformat(timespec="microseconds").replace(
            "+00:00", "Z"
        ),
        "repository": _git_context(root),
        "runtime": {"python": sys.version.split()[0]},
        "tools": {"alphus": _tool_version("alphus"), "lizard": _tool_version("lizard")},
        "configuration": {
            "quality_gates_sha256": _sha256(root / "quality-gates.toml"),
            "cpp_common_quality_sha256": _sha256(root / "cpp-common-quality.json"),
        },
        "cpp": {
            "source": source,
            "style": style,
            "lizard": lizard,
            "source_and_style_file_counts_match": source["files_scanned"]
            == style["files_scanned"],
        },
    }

    output_path = output if output.is_absolute() else root / output
    history_path = history if history.is_absolute() else root / history
    _atomic_write_json(output_path, report)
    history_path.parent.mkdir(parents=True, exist_ok=True)
    with history_path.open("a", encoding="utf-8", newline="\n") as stream:
        stream.write(json.dumps(report, sort_keys=True, separators=(",", ":")) + "\n")
    return report


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=REPOSITORY_ROOT)
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("out/quality/alphus-cpp-summary.json"),
        help="latest structured summary path (default: out/quality/alphus-cpp-summary.json)",
    )
    parser.add_argument(
        "--history",
        type=Path,
        default=Path("out/quality/alphus-cpp-history.jsonl"),
        help="append-only local run history (default: out/quality/alphus-cpp-history.jsonl)",
    )
    args = parser.parse_args()
    try:
        report = summarize(args.root, args.output, args.history)
    except (OSError, TypeError, ValueError) as error:
        print(f"Alphus C++ statistics failed: {error}", file=sys.stderr)
        return 2

    cpp = report["cpp"]
    print(
        "C++ statistics: "
        f"source files={cpp['source']['files_scanned']}, "
        f"source findings={cpp['source']['finding_count']}, "
        f"style findings={cpp['style']['finding_count']}, "
        f"duplicate groups={cpp['style']['duplicate_code_groups']['total']}, "
        f"Lizard functions={cpp['lizard']['reported_function_count']} "
        f"(CCN threshold {cpp['lizard']['configured_ccn_threshold']}, "
        f"max CCN {cpp['lizard']['ccn']['maximum']}); "
        f"summary={args.output}, history={args.history}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
