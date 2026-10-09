"""Capture and query local C++ build-performance and quality snapshots."""

from __future__ import annotations

import argparse
import json
import platform
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
if __package__:
    from tools.check_source_file_sizes import DEFAULT_CONFIG, _load_config, _scan
    from tools.report_compile_times import collect
else:
    from check_source_file_sizes import DEFAULT_CONFIG, _load_config, _scan
    from report_compile_times import collect

DEFAULT_HISTORY = REPOSITORY_ROOT / "out" / "build-metrics" / "history.jsonl"


def _git_value(*arguments: str) -> str:
    result = subprocess.run(
        ["git", *arguments],
        cwd=REPOSITORY_ROOT,
        check=False,
        text=True,
        capture_output=True,
    )
    return result.stdout.strip() if result.returncode == 0 else "unknown"


def _traceability_summary() -> dict[str, Any]:
    result = subprocess.run(
        [
            sys.executable,
            "tools/query_rti_work.py",
            "resume",
            "--json",
            "--compact",
            "--summary",
        ],
        cwd=REPOSITORY_ROOT,
        check=False,
        text=True,
        capture_output=True,
    )
    if result.returncode:
        raise RuntimeError(f"RTI work summary failed: {result.stderr.strip()}")
    payload = json.loads(result.stdout)
    mapping = payload["mapping_counts"]
    plans = payload["plan_counts"]
    return {
        "roadmap_complete": payload["roadmap_counts"]["complete"],
        "roadmap_open": payload["roadmap_counts"]["open"],
        "catch2_cases": mapping["catch2_plan_cases"],
        "catch2_mapped": plans["mapped_cases"],
        "catch2_unmapped": mapping["catch2_cases_without_lab_requirement_mapping"],
        "catch2_unclassified": mapping["catch2_unmapped_unclassified"],
        "cpp_source_without_plan_row": mapping["cpp_source_cases_without_plan_row"],
        "actionable_unlocated_cases": plans["source_unlocated_actionable_cases"],
        "source_health": payload["source_health"]["status"],
        "index_matches_live": payload["index_snapshot"]["matches_live"],
    }


def _mapping_check_summary() -> dict[str, Any]:
    result = subprocess.run(
        [
            sys.executable,
            "tools/query_rti_work.py",
            "check",
            "--json",
            "--summary",
            "--compact",
        ],
        cwd=REPOSITORY_ROOT,
        check=False,
        text=True,
        capture_output=True,
    )
    try:
        payload = json.loads(result.stdout)
    except json.JSONDecodeError as error:
        raise RuntimeError(
            f"RTI mapping check did not return JSON: {result.stderr.strip()}"
        ) from error
    errors = payload.get("errors", [])
    warnings = payload.get("warnings", [])
    return {
        "ok": bool(payload.get("ok", result.returncode == 0)),
        "error_count": len(errors),
        "warning_count": len(warnings),
        "mapped_test_count": payload.get("mapped_test_count", 0),
        "repository_test_count": payload.get("repository_test_count", 0),
        "unclassified_count": payload.get("unclassified_count", 0),
        "tests_without_source_location": payload.get(
            "tests_without_source_location", 0
        ),
        "unlocated_test_ids": payload.get("unlocated_test_ids", [])[:10],
        "error_samples": errors[:10],
        "warning_samples": warnings[:10],
    }


def _build_environment(build_dir: Path) -> dict[str, str]:
    cache_path = build_dir / "CMakeCache.txt"
    cache: dict[str, str] = {}
    if cache_path.is_file():
        for line in cache_path.read_text(
            encoding="utf-8", errors="replace"
        ).splitlines():
            if line.startswith(("//", "#")) or "=" not in line:
                continue
            key, value = line.split("=", maxsplit=1)
            cache[key.split(":", maxsplit=1)[0]] = value
    compiler = cache.get("CMAKE_CXX_COMPILER", "unknown")
    version = "unknown"
    if compiler != "unknown":
        result = subprocess.run(
            [compiler, "--version"],
            check=False,
            text=True,
            capture_output=True,
            timeout=5,
        )
        if result.returncode == 0:
            version = (result.stdout or result.stderr).splitlines()[0]
    return {
        "machine": platform.platform(),
        "generator": cache.get("CMAKE_GENERATOR", "unknown"),
        "build_type": cache.get("CMAKE_BUILD_TYPE", "unknown"),
        "compiler": compiler,
        "compiler_version": version,
        "cxx_flags": cache.get("CMAKE_CXX_FLAGS", ""),
        "cxx_flags_debug": cache.get("CMAKE_CXX_FLAGS_DEBUG", ""),
    }


def _capture(
    build_dir: Path,
    label: str,
    ninja: str,
    build_status: str,
    note: str | None,
) -> dict[str, Any]:
    build_dir = build_dir.resolve(strict=False)
    if not build_dir.is_dir():
        raise FileNotFoundError(f"build directory not found: {build_dir}")
    records, headers, bundled_headers, targets, links, unmatched = collect(
        build_dir, ninja
    )
    policies, baseline = _load_config(DEFAULT_CONFIG)
    findings, errors = _scan(policies, baseline)
    now = datetime.now(timezone.utc)
    commit = _git_value("rev-parse", "HEAD")
    status = _git_value("status", "--porcelain")
    return {
        "schema": 2,
        "id": f"{now.strftime('%Y%m%dT%H%M%S.%fZ')}-{commit[:8]}",
        "recorded_at_utc": now.isoformat(timespec="seconds"),
        "label": label,
        "build_status": build_status,
        "note": note,
        "branch": _git_value("branch", "--show-current"),
        "commit": commit,
        "working_tree_dirty": bool(status),
        "build_dir": str(build_dir),
        "environment": _build_environment(build_dir),
        "build": {
            "translation_units": len(records),
            "unmeasured_compile_commands": unmatched,
            "compiler_time_ms": sum(item["elapsed_ms"] for item in records),
            "first_party_include_edges": sum(
                item["first_party_header_count"] for item in records
            ),
            "bundled_include_edges": sum(
                item["bundled_header_count"] for item in records
            ),
            "external_include_edges": sum(
                item["external_dependency_count"] for item in records
            ),
            "first_party_header_count": len(headers),
            "records": records,
            "header_fanout": headers,
            "bundled_header_fanout": bundled_headers,
            "target_totals": targets,
            "link_actions": links,
        },
        "quality": {
            "source_size_soft_findings": len(findings),
            "source_size_blocking_errors": len(errors),
            "source_size_findings": findings,
            "source_size_errors": errors,
            "traceability": _traceability_summary(),
            "mapping_check": _mapping_check_summary(),
        },
    }


def _read_history(path: Path) -> list[dict[str, Any]]:
    if not path.is_file():
        return []
    entries = []
    for line_number, line in enumerate(
        path.read_text(encoding="utf-8").splitlines(), start=1
    ):
        if not line.strip():
            continue
        try:
            entry = json.loads(line)
        except json.JSONDecodeError as error:
            raise ValueError(f"invalid history JSON at {path}:{line_number}: {error}")
        if entry.get("schema") not in {1, 2}:
            raise ValueError(f"unsupported history schema at {path}:{line_number}")
        entries.append(entry)
    return entries


def _append_history(path: Path, entry: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("a", encoding="utf-8", newline="\n") as stream:
        stream.write(json.dumps(entry, separators=(",", ":")) + "\n")


def _show_history(
    entries: list[dict[str, Any]],
    limit: int,
    label: str | None,
    status: str | None = None,
) -> None:
    if label:
        entries = [entry for entry in entries if entry["label"] == label]
    if status:
        entries = [
            entry for entry in entries if entry.get("build_status", "unknown") == status
        ]
    entries = entries[-max(0, limit) :] if limit else []
    if not entries:
        print("No matching build-quality snapshots.")
        return
    print(
        "recorded UTC              label             status   commit    TUs  compile CPU  "
        "1st-party  size warn/block  map errors  unclassified  index-current"
    )
    for entry in entries:
        build = entry["build"]
        quality = entry["quality"]
        traceability = quality["traceability"]
        mapping_check = quality.get("mapping_check")
        mapping_error_count = (
            str(mapping_check["error_count"]) if mapping_check else "n/a"
        )
        print(
            f"{entry['recorded_at_utc']:<24} {entry['label']:<17} "
            f"{entry.get('build_status', 'unknown'):<8} "
            f"{entry['commit'][:8]:<8} {build['translation_units']:>4}  "
            f"{build['compiler_time_ms'] / 1000:>8.1f}s  "
            f"{build['first_party_include_edges']:>9}  "
            f"{quality['source_size_soft_findings']:>4}/"
            f"{quality['source_size_blocking_errors']:<4}  "
            f"{mapping_error_count:>10}  "
            f"{traceability['catch2_unclassified']:>12}  "
            f"{traceability['index_matches_live']!s:>13}"
        )


def _show_details(
    entry: dict[str, Any],
    top: int,
    source_filter: str | None = None,
    header_filter: str | None = None,
    target_filter: str | None = None,
) -> None:
    build = entry["build"]
    quality = entry["quality"]
    print(
        f"{entry['id']}  {entry['label']}  {entry['branch']}@{entry['commit'][:8]}  "
        f"status={entry.get('build_status', 'unknown')}  "
        f"dirty={entry['working_tree_dirty']}"
    )
    if entry.get("note"):
        print(f"Note: {entry['note']}")
    environment = entry.get("environment")
    if environment:
        print(
            f"Environment: {environment['generator']} / {environment['build_type']} / "
            f"{environment['compiler_version']} / {environment['machine']}"
        )
    print(
        f"Build: {build['translation_units']} translation units, "
        f"{build['compiler_time_ms'] / 1000:.1f}s summed compiler time, "
        f"{build['unmeasured_compile_commands']} unmeasured compile commands"
    )
    print(
        "Includes: "
        f"{build['first_party_include_edges']} first-party, "
        f"{build['bundled_include_edges']} bundled/generated, "
        f"{build['external_include_edges']} external dependency edges"
    )
    source_rows = build["records"]
    if source_filter:
        source_rows = [
            item
            for item in source_rows
            if source_filter.casefold() in item["source"].casefold()
        ]
    else:
        source_rows = source_rows[: max(0, top)]
    print("Matching translation units:" if source_filter else "Slow translation units:")
    for item in source_rows:
        print(
            f"  {item['elapsed_ms'] / 1000:7.2f}s  {item['source_lines']:5} lines  "
            f"{item['first_party_header_count']:3} first-party headers  {item['source']}"
        )
    header_rows = build["header_fanout"]
    if header_filter:
        header_rows = [
            item
            for item in header_rows
            if header_filter.casefold() in item["path"].casefold()
        ]
    else:
        header_rows = header_rows[: max(0, top)]
    print(
        "Matching first-party headers:"
        if header_filter
        else "Highest-fanout first-party headers:"
    )
    for item in header_rows:
        print(f"  {item['translation_units']:4} translation units  {item['path']}")
    target_rows = build["target_totals"]
    if target_filter:
        target_rows = [
            item
            for item in target_rows
            if target_filter.casefold() in item["target"].casefold()
        ]
    else:
        target_rows = target_rows[: max(0, top)]
    print(
        "Matching build targets:" if target_filter else "Build-target compile totals:"
    )
    for item in target_rows:
        print(
            f"  {item['elapsed_ms'] / 1000:7.2f}s  "
            f"{item['translation_units']:4} translation units  {item['target']}"
        )
    print("Slow link/archive actions:")
    for item in build["link_actions"][: max(0, min(top, 10))]:
        print(f"  {item['elapsed_ms'] / 1000:7.2f}s  {item['output']}")
    traceability = quality["traceability"]
    print(
        "Quality: "
        f"size soft findings={quality['source_size_soft_findings']}, "
        f"size blockers={quality['source_size_blocking_errors']}, "
        f"Catch2 mapped={traceability['catch2_mapped']}/"
        f"{traceability['catch2_cases']}, "
        f"unclassified={traceability['catch2_unclassified']}, "
        f"source health={traceability['source_health']}, "
        f"roadmap index matches live={traceability['index_matches_live']}"
    )
    mapping_check = quality.get("mapping_check")
    if mapping_check:
        print(
            "Roadmap/test mapping gate: "
            f"{'passed' if mapping_check['ok'] else 'FAILED'}, "
            f"{mapping_check['error_count']} errors, "
            f"{mapping_check['warning_count']} warnings, "
            f"{mapping_check['tests_without_source_location']} source locations missing"
        )
        for message in mapping_check["error_samples"][:3]:
            print(f"  mapping error: {message}")
        for test_id in mapping_check.get("unlocated_test_ids", [])[:3]:
            print(f"  missing source location: {test_id}")
    if quality["source_size_findings"]:
        print("Largest source-size findings:")
        for item in sorted(
            quality["source_size_findings"],
            key=lambda finding: (-finding["lines"], finding["path"]),
        )[: max(0, top)]:
            print(f"  {item['lines']:6} lines  {item['path']}")
    for error in quality["source_size_errors"]:
        print(f"BLOCKER: {error}")


def _show_comparison(current: dict[str, Any], previous: dict[str, Any]) -> None:
    current_build = current["build"]
    previous_build = previous["build"]
    print(
        "Change from previous same-label capture: "
        f"compiler CPU {current_build['compiler_time_ms'] - previous_build['compiler_time_ms']:+} ms; "
        f"first-party include edges "
        f"{current_build['first_party_include_edges'] - previous_build['first_party_include_edges']:+}"
    )

    old_sources = {
        item["source"]: item["elapsed_ms"] for item in previous_build["records"]
    }
    source_deltas = [
        (item["elapsed_ms"] - old_sources[item["source"]], item["source"])
        for item in current_build["records"]
        if item["source"] in old_sources
    ]
    regressions = sorted((item for item in source_deltas if item[0] > 0), reverse=True)[
        :5
    ]
    if regressions:
        print("Largest per-file compile-time increases:")
        for delta, source in regressions:
            print(f"  {delta / 1000:+.2f}s  {source}")

    old_headers = {
        item["path"]: item["translation_units"]
        for item in previous_build["header_fanout"]
    }
    header_deltas = [
        (item["translation_units"] - old_headers[item["path"]], item["path"])
        for item in current_build["header_fanout"]
        if item["path"] in old_headers
    ]
    fanout_increases = sorted(
        (item for item in header_deltas if item[0] > 0), reverse=True
    )[:5]
    if fanout_increases:
        print("Largest first-party header fanout increases:")
        for delta, path in fanout_increases:
            print(f"  {delta:+} translation units  {path}")

    current_trace = current["quality"]["traceability"]
    previous_trace = previous["quality"]["traceability"]
    current_mapping_errors = current["quality"].get("mapping_check", {}).get(
        "error_count", 0
    )
    previous_mapping_errors = previous["quality"].get("mapping_check", {}).get(
        "error_count", 0
    )
    print(
        "Quality-count change: "
        f"source-size warnings "
        f"{current['quality']['source_size_soft_findings'] - previous['quality']['source_size_soft_findings']:+}; "
        f"mapping-check errors {current_mapping_errors - previous_mapping_errors:+}; "
        f"unclassified tests "
        f"{current_trace['catch2_unclassified'] - previous_trace['catch2_unclassified']:+}; "
        f"unmapped tests "
        f"{current_trace['catch2_unmapped'] - previous_trace['catch2_unmapped']:+}"
    )


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)
    capture = subparsers.add_parser(
        "capture", help="record a build and quality snapshot"
    )
    capture.add_argument("build_dir", type=Path)
    capture.add_argument(
        "--label", required=True, help="profile/target label for history filters"
    )
    capture.add_argument("--ninja", default="ninja")
    capture.add_argument("--history", type=Path, default=DEFAULT_HISTORY)
    capture.add_argument(
        "--status",
        choices=("passed", "failed", "partial", "unknown"),
        default="unknown",
        help="outcome of the build/test run represented by this snapshot",
    )
    capture.add_argument("--note", help="short context, such as a failing target")
    history = subparsers.add_parser("history", help="query recent build snapshots")
    history.add_argument("--history", type=Path, default=DEFAULT_HISTORY)
    history.add_argument("--limit", type=int, default=20)
    history.add_argument("--label")
    history.add_argument("--status", choices=("passed", "failed", "partial", "unknown"))
    inspect = subparsers.add_parser("inspect", help="inspect one snapshot")
    inspect.add_argument("--history", type=Path, default=DEFAULT_HISTORY)
    inspect.add_argument("--id", default="latest", help="snapshot ID or latest")
    inspect.add_argument("--top", type=int, default=10)
    inspect.add_argument(
        "--source", help="show translation units matching this path fragment"
    )
    inspect.add_argument(
        "--header", help="show first-party headers matching this path fragment"
    )
    inspect.add_argument(
        "--target", help="show build targets matching this name fragment"
    )
    arguments = parser.parse_args(argv)

    try:
        if arguments.command == "capture":
            entry = _capture(
                arguments.build_dir,
                arguments.label,
                arguments.ninja,
                arguments.status,
                arguments.note,
            )
            _append_history(arguments.history, entry)
            previous = [
                item
                for item in _read_history(arguments.history)[:-1]
                if item["label"] == arguments.label
            ]
            _show_details(entry, top=5)
            if previous:
                _show_comparison(entry, previous[-1])
            print(f"History: {arguments.history}")
            return 0 if entry["quality"]["source_size_blocking_errors"] == 0 else 1

        entries = _read_history(arguments.history)
        if arguments.command == "history":
            _show_history(entries, arguments.limit, arguments.label, arguments.status)
            return 0
        if not entries:
            raise FileNotFoundError(f"no build-quality history at {arguments.history}")
        if arguments.id == "latest":
            entry = entries[-1]
        else:
            matches = [item for item in entries if item["id"] == arguments.id]
            if not matches:
                raise ValueError(f"snapshot ID not found: {arguments.id}")
            entry = matches[-1]
        _show_details(
            entry,
            arguments.top,
            source_filter=arguments.source,
            header_filter=arguments.header,
            target_filter=arguments.target,
        )
        return 0
    except (OSError, RuntimeError, ValueError, KeyError, json.JSONDecodeError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
