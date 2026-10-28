"""Run Alphus with a saved transcript, Python finding summary, and timings."""

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
import time
from collections import Counter
from collections.abc import Mapping, Sequence
from datetime import datetime, timezone
from importlib.metadata import PackageNotFoundError, version
from pathlib import Path
from typing import Literal, TypedDict, cast

REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
STEP_START = re.compile(r"^==>\s+(.+?)\s*$")
STEP_END = re.compile(
    r"^PROVISIONAL\s+(?P<status>PASS|FAIL)\s+(?P<name>.+?)"
    r"(?:\s+\(exit\s+(?P<exit_code>-?\d+)\))?\s*$"
)
FORMAT_COUNT = re.compile(
    r"(?P<reformat>\d+) files? would be reformatted, "
    r"(?P<formatted>\d+) files? already formatted"
)
FORMAT_FILE = re.compile(r"^(?P<path>.+?):\d+:\d+: unformatted:")
RULE_DIAGNOSTIC = re.compile(r"^(ERROR|WARNING) (?P<rule>alphus/\S+) (?P<details>.+)$")
RULE_PATH = re.compile(r"^(?P<path>.+?):\d+:")
ANSI_ESCAPE = re.compile(r"\x1b\[[0-?]*[ -/]*[@-~]")


class _PathCount(TypedDict):
    path: str
    diagnostic_count: int


class _RuffDiagnostic(TypedDict):
    filename: str
    code: str
    is_fixable: bool


class _RuffSummary(TypedDict):
    diagnostic_count: int
    files_with_diagnostics: int
    diagnostics_by_category: dict[str, int]
    rules_by_category: dict[str, dict[str, int]]
    diagnostics_by_rule: dict[str, int]
    auto_fixable_diagnostic_count: int
    top_paths: list[_PathCount]
    top_production_paths: list[_PathCount]


class _FindingSummary(TypedDict):
    finding_count: int
    files_with_findings: int
    findings_by_severity: dict[str, int]
    findings_by_rule: dict[str, int]
    top_paths: list[_PathCount]


class _FormatSummary(TypedDict):
    files_would_be_reformatted: int | None
    files_already_formatted: int | None
    unformatted_paths_reported: int
    top_paths: list[_PathCount]


class _AlphusFinding(TypedDict):
    severity: str
    rule: str
    path: str


class _StepTiming(TypedDict):
    name: str
    status: Literal["PASS", "FAIL", "incomplete"]
    exit_code: int
    elapsed_seconds: float | None
    matched_start: bool


class _RunTiming(TypedDict):
    schema: Literal[1]
    run_id: str
    started_at_utc: str
    finished_at_utc: str
    elapsed_seconds: float
    alphus_process_seconds: float
    exit_code: int
    steps: list[_StepTiming]


class _PythonSummary(TypedDict):
    schema: Literal[1]
    run_id: str
    started_at_utc: str
    finished_at_utc: str
    elapsed_seconds: float
    tools: dict[str, str | None]
    ruff: _RuffSummary
    format: _FormatSummary
    alphus_structure: _FindingSummary
    file_size: _FindingSummary
    python_step_timings: list[_StepTiming]
    report_paths: dict[str, str]


def _utc_now() -> datetime:
    return datetime.now(timezone.utc)


def _utc_text(value: datetime) -> str:
    return value.isoformat(timespec="microseconds").replace("+00:00", "Z")


# opaque-boundary: Read and validate JSON reports written by Alphus and Ruff.
def _read_json(path: Path) -> object:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise ValueError(f"cannot read JSON report {path}: {error}") from error


# opaque-boundary: Persist one JSON report at the local output boundary.
def _write_json(path: Path, value: Mapping[str, object]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + ".tmp")
    temporary.write_text(
        json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    temporary.replace(path)


# opaque-boundary: Append a validated JSON-ready summary record.
def _append_json_line(path: Path, value: Mapping[str, object]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("a", encoding="utf-8", newline="\n") as stream:
        stream.write(json.dumps(value, sort_keys=True, separators=(",", ":")) + "\n")


def _section(lines: Sequence[str], name: str) -> list[str]:
    start_marker = f"==> {name}"
    start = next(
        (index for index, line in enumerate(lines) if line.strip() == start_marker),
        None,
    )
    if start is None:
        return []
    end = next(
        (
            index
            for index in range(start + 1, len(lines))
            if lines[index].startswith("PROVISIONAL ")
        ),
        len(lines),
    )
    return list(lines[start + 1 : end])


def _file_category(path: str) -> str:
    normalized = path.replace("\\", "/").removeprefix("./")
    parts = normalized.lower().split("/")
    basename = parts[-1] if parts else normalized.lower()
    if any(part in {"tests", "test", "test-fixtures", "fixtures"} for part in parts):
        return "tests-and-fixtures"
    if basename.startswith("test_") or basename.endswith("_test.py"):
        return "tests-and-fixtures"
    if "src" in parts:
        return "production"
    if parts and parts[0] == "tools":
        return "developer-tools"
    if "adapters" in parts:
        return "adapters"
    return "other"


# opaque-boundary: Convert report counts to a JSON-ready path inventory.
def _top_paths(counts: Counter[str], *, limit: int = 10) -> list[_PathCount]:
    return [
        {"path": path, "diagnostic_count": count}
        for path, count in counts.most_common(limit)
    ]


# TODO this should have been fixed by ALphus; we should construct and dump at the very least;
#  opaque-boundary: Aggregate validated Ruff JSON diagnostic objects.
def _ruff_summary(report_path: Path) -> _RuffSummary:
    payload = _read_json(report_path)
    if not isinstance(payload, list):
        raise TypeError("Ruff JSON must be an array of diagnostic objects")

    diagnostics: list[_RuffDiagnostic] = []
    for index, item in enumerate(payload):
        if not isinstance(item, dict):
            raise TypeError(f"Ruff diagnostic {index} must be an object")
        filename = item.get("filename")
        code = item.get("code")
        if not isinstance(filename, str):
            raise TypeError(f"Ruff diagnostic {index} filename must be a string")
        if not isinstance(code, str):
            raise TypeError(f"Ruff diagnostic {index} code must be a string")
        diagnostics.append(
            {
                "filename": filename,
                "code": code,
                "is_fixable": item.get("fix") is not None,
            }
        )

    by_rule: Counter[str] = Counter()
    by_category: Counter[str] = Counter()
    # opaque-boundary: Group validated scanner codes for the JSON summary.
    rules_by_category: dict[str, Counter[str]] = {}
    paths: Counter[str] = Counter()
    production_paths: Counter[str] = Counter()
    fixable_count = 0
    for item in diagnostics:
        path = item["filename"]
        code = item["code"]
        category = _file_category(path)
        by_rule[code] += 1
        by_category[category] += 1
        rules_by_category.setdefault(category, Counter())[code] += 1
        paths[path] += 1
        if category == "production":
            production_paths[path] += 1
        if item["is_fixable"]:
            fixable_count += 1

    return {
        "diagnostic_count": len(payload),
        "files_with_diagnostics": len(paths),
        "diagnostics_by_category": dict(sorted(by_category.items())),
        "rules_by_category": {
            category: dict(sorted(counts.items()))
            for category, counts in sorted(rules_by_category.items())
        },
        "diagnostics_by_rule": dict(sorted(by_rule.items())),
        "auto_fixable_diagnostic_count": fixable_count,
        "top_paths": _top_paths(paths),
        "top_production_paths": _top_paths(production_paths),
    }


# opaque-boundary: Summarize findings emitted by Alphus's Python rule output.
def _alphus_rule_summary(lines: Sequence[str], step_name: str) -> _FindingSummary:
    # opaque-boundary: Store validated report rows for category aggregation.
    findings: list[_AlphusFinding] = []
    for line in _section(lines, step_name):
        match = RULE_DIAGNOSTIC.match(line)
        if match is None:
            continue
        details = match.group("details")
        path_match = RULE_PATH.match(details)
        if path_match is not None:
            finding_path = path_match.group("path")
        else:
            finding_path, separator, _ = details.partition(": ")
            if not separator:
                finding_path = "unknown"
        findings.append(
            {
                "severity": line.split(" ", 1)[0],
                "rule": match.group("rule"),
                "path": finding_path,
            }
        )
    by_rule = Counter(item["rule"] for item in findings)
    by_severity = Counter(item["severity"] for item in findings)
    by_path = Counter(item["path"] for item in findings)
    return {
        "finding_count": len(findings),
        "files_with_findings": len(by_path),
        "findings_by_severity": dict(sorted(by_severity.items())),
        "findings_by_rule": dict(sorted(by_rule.items())),
        "top_paths": _top_paths(by_path),
    }


# opaque-boundary: Return a JSON-ready count from Ruff's formatter output.
def _format_summary(lines: Sequence[str]) -> _FormatSummary:
    section = _section(lines, "Python formatting check all files")
    summary = next(
        (FORMAT_COUNT.search(line) for line in section if FORMAT_COUNT.search(line)),
        None,
    )
    files = Counter(
        match.group("path")
        for line in section
        if (match := FORMAT_FILE.match(line)) is not None
    )
    return {
        "files_would_be_reformatted": int(summary.group("reformat"))
        if summary
        else None,
        "files_already_formatted": int(summary.group("formatted")) if summary else None,
        "unformatted_paths_reported": len(files),
        "top_paths": _top_paths(files),
    }


# opaque-boundary: Assemble per-run Python metrics from Alphus and Ruff reports.
def _python_summary(
    root: Path,
    run_id: str,
    timing: _RunTiming,
    log_path: Path,
    ruff_path: Path,
) -> _PythonSummary:
    lines = log_path.read_text(encoding="utf-8", errors="replace").splitlines()
    ruff = _ruff_summary(ruff_path)
    return {
        "schema": 1,
        "run_id": run_id,
        "started_at_utc": timing["started_at_utc"],
        "finished_at_utc": timing["finished_at_utc"],
        "elapsed_seconds": timing["elapsed_seconds"],
        "tools": {"ruff": _tool_version("ruff"), "alphus": _tool_version("alphus")},
        "ruff": ruff,
        "format": _format_summary(lines),
        "alphus_structure": _alphus_rule_summary(lines, "Python structure candidates"),
        "file_size": _alphus_rule_summary(lines, "Python file-size candidates"),
        "python_step_timings": [
            item for item in timing["steps"] if "Python" in str(item.get("name", ""))
        ],
        "report_paths": {
            "ruff_json": ruff_path.relative_to(root).as_posix(),
            "run_log": log_path.relative_to(root).as_posix(),
        },
    }


def _tool_version(package: str) -> str | None:
    try:
        return version(package)
    except PackageNotFoundError:
        return None


# opaque-boundary: Return measured step records for the local timing report.
def _step_timings(
    process: subprocess.Popen[str], log_path: Path
) -> tuple[int, list[_StepTiming], float]:
    current_name: str | None = None
    current_start: float | None = None
    # opaque-boundary: Store normalized timing records for JSON serialization.
    timings: list[_StepTiming] = []
    start_clock = time.perf_counter()
    ansi = ANSI_ESCAPE
    with log_path.open("w", encoding="utf-8", newline="") as log:
        if process.stdout is None:
            raise RuntimeError("Alphus output stream was not captured")
        for line in process.stdout:
            log.write(line)
            clean = ansi.sub("", line.rstrip("\r\n"))
            start_match = STEP_START.match(clean)
            end_match = STEP_END.match(clean)
            if start_match:
                current_name = start_match.group(1)
                current_start = time.perf_counter()
                print(f"Running {current_name}...", flush=True)
            elif end_match:
                elapsed = (
                    time.perf_counter() - current_start
                    if current_start is not None
                    else None
                )
                status_text = end_match.group("status")
                if status_text not in {"PASS", "FAIL"}:
                    raise ValueError(f"unknown Alphus step status: {status_text}")
                timings.append(
                    {
                        "name": end_match.group("name"),
                        "status": cast(Literal["PASS", "FAIL"], status_text),
                        "exit_code": (
                            int(end_match.group("exit_code"))
                            if end_match.group("exit_code") is not None
                            else 0
                            if end_match.group("status") == "PASS"
                            else 1
                        ),
                        "elapsed_seconds": round(elapsed, 3)
                        if elapsed is not None
                        else None,
                        "matched_start": current_name == end_match.group("name"),
                    }
                )
                print(
                    f"{end_match.group('status')}: {end_match.group('name')}",
                    flush=True,
                )
                current_name = None
                current_start = None
        return_code = process.wait()
        total_elapsed = time.perf_counter() - start_clock
        if current_name is not None:
            timings.append(
                {
                    "name": current_name,
                    "status": "incomplete",
                    "exit_code": return_code,
                    "elapsed_seconds": round(time.perf_counter() - current_start, 3)
                    if current_start is not None
                    else None,
                    "matched_start": True,
                }
            )
    return return_code, timings, total_elapsed


# opaque-boundary: Attach the measured run record to the matching C++ summary.
def _attach_execution(root: Path, timing: _RunTiming) -> None:
    summary_path = root / "out" / "quality" / "alphus-cpp-summary.json"
    history_path = root / "out" / "quality" / "alphus-cpp-history.jsonl"
    summary = _read_json(summary_path)
    if not isinstance(summary, dict):
        raise TypeError("C++ summary must be a JSON object")
    summary["profile_execution"] = timing
    _write_json(summary_path, summary)

    rows = history_path.read_text(encoding="utf-8").splitlines()
    if not rows:
        raise ValueError("C++ statistics history is empty")
    latest = json.loads(rows[-1])
    if not isinstance(latest, dict) or latest.get("run_id") != summary.get("run_id"):
        raise ValueError("latest C++ history row does not match the current summary")
    latest["profile_execution"] = timing
    rows[-1] = json.dumps(latest, sort_keys=True, separators=(",", ":"))
    history_path.write_text("\n".join(rows) + "\n", encoding="utf-8")


def run_profile(root: Path, config: Path, profile: str, log_path: Path) -> int:
    root = root.resolve()
    config_path = config if config.is_absolute() else root / config
    log_path = log_path if log_path.is_absolute() else root / log_path
    report_root = root / "out" / "quality"
    report_root.mkdir(parents=True, exist_ok=True)
    launcher = Path(sys.executable).with_name(
        "alphus.exe" if os.name == "nt" else "alphus"
    )
    if not launcher.is_file():
        raise FileNotFoundError(
            f"Alphus launcher is missing beside {sys.executable}: {launcher}"
        )

    run_started = _utc_now()
    clock_started = time.perf_counter()
    process = subprocess.Popen(
        [
            str(launcher),
            "--root",
            str(root),
            "--config",
            str(config_path),
            "--run",
            "--profile",
            profile,
        ],
        cwd=root,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        encoding="utf-8",
        errors="replace",
        bufsize=1,
    )
    return_code, steps, process_elapsed = _step_timings(process, log_path)
    finished_at = _utc_now()
    run_id = run_started.strftime("%Y%m%dT%H%M%S.%fZ")
    # opaque-boundary: Build the machine-readable timing record for this run.
    timing: _RunTiming = {
        "schema": 1,
        "run_id": run_id,
        "started_at_utc": _utc_text(run_started),
        "finished_at_utc": _utc_text(finished_at),
        "elapsed_seconds": round(time.perf_counter() - clock_started, 3),
        "alphus_process_seconds": round(process_elapsed, 3),
        "exit_code": return_code,
        "steps": steps,
    }

    python_summary = _python_summary(
        root,
        run_id,
        timing,
        log_path,
        report_root / "alphus-python-ruff.json",
    )
    python_summary_path = report_root / "alphus-python-summary.json"
    python_history_path = report_root / "alphus-python-history.jsonl"
    _write_json(python_summary_path, python_summary)
    _append_json_line(python_history_path, python_summary)
    _write_json(report_root / "alphus-run-timing.json", timing)
    _append_json_line(report_root / "alphus-run-timing.jsonl", timing)
    _attach_execution(root, timing)

    with log_path.open("a", encoding="utf-8", newline="") as log:
        log.write("\n=== Alphus step timings ===\n")
        print("\n=== Alphus step timings ===")
        for step in steps:
            duration = step["elapsed_seconds"]
            message = f"{step['name']}: {duration}s ({step['status']})"
            log.write(message + "\n")
            print(message)
        message = f"Full run: {timing['elapsed_seconds']}s"
        log.write(message + "\n")
        print(message)
        message = (
            f"Python: {python_summary['ruff']['diagnostic_count']} Ruff findings; "
            f"{python_summary['format']['files_would_be_reformatted']} files need formatting; "
            f"summary={python_summary_path.relative_to(root).as_posix()}"
        )
        log.write(message + "\n")
        print(message)
        message = f"Detailed log: {log_path.relative_to(root).as_posix()}"
        log.write(message + "\n")
        print(message)
    return return_code


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=REPOSITORY_ROOT)
    parser.add_argument("--config", type=Path, default=Path("quality-gates.toml"))
    parser.add_argument("--profile", default="lint")
    parser.add_argument(
        "--log",
        type=Path,
        default=Path("out/quality/alphus-lint.txt"),
    )
    args = parser.parse_args()
    try:
        return run_profile(args.root, args.config, args.profile, args.log)
    except (OSError, TypeError, ValueError, RuntimeError) as error:
        print(f"Unable to record Alphus statistics: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
