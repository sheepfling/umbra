"""Measure compile-time and include-fanout costs in a CMake/Ninja C++ build.

Ninja's build log and dependency database provide per-object timings and
transitive include sets without re-running the preprocessor. Metrics are
observational because compiler timing varies by machine and build mode.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
from collections import Counter
from pathlib import Path
from typing import Any

REPOSITORY_ROOT = Path(__file__).resolve().parents[1]


def _path_key(path: str | Path, base: Path | None = None) -> str:
    candidate = Path(path)
    if not candidate.is_absolute() and base is not None:
        candidate = base / candidate
    return os.path.normcase(os.path.abspath(candidate)).replace("\\", "/")


def _compile_outputs(database_path: Path) -> dict[str, str]:
    entries = json.loads(database_path.read_text(encoding="utf-8"))
    outputs: dict[str, str] = {}
    for entry in entries:
        output = entry.get("output")
        source = entry.get("file")
        directory = Path(entry["directory"])
        if not output or not source:
            continue
        outputs[_path_key(output, directory)] = str(Path(source).resolve(strict=False))
    return outputs


def _ninja_durations(log_path: Path) -> dict[str, int]:
    durations: dict[str, int] = {}
    for line in log_path.read_text(encoding="utf-8", errors="replace").splitlines():
        if not line or line.startswith("#"):
            continue
        fields = line.split("\t")
        if len(fields) < 4:
            continue
        try:
            start_ms, end_ms = int(fields[0]), int(fields[1])
        except ValueError:
            continue
        durations[_path_key(fields[3], log_path.parent)] = max(0, end_ms - start_ms)
    return durations


def _ninja_dependencies(output: str, build_dir: Path) -> dict[str, list[str]]:
    dependencies: dict[str, list[str]] = {}
    current_output: str | None = None
    for line in output.splitlines():
        if " : #deps " in line or ": #deps " in line:
            output_path, marker, _ = line.partition(": #deps ")
            if marker:
                current_output = _path_key(output_path, build_dir)
                dependencies[current_output] = []
            continue
        if current_output is not None and line[:1].isspace():
            dependency = line.strip()
            if dependency:
                dependencies[current_output].append(_path_key(dependency))
            continue
        current_output = None
    return dependencies


def _project_relative(path_key: str) -> str | None:
    repository_key = _path_key(REPOSITORY_ROOT).rstrip("/") + "/"
    if path_key.startswith(repository_key):
        return path_key[len(repository_key) :]
    return None


def _is_bundled_or_generated_header(relative_path: str) -> bool:
    return Path(relative_path).parts[0].casefold() in {
        ".build",
        "out",
        "third_party",
        "vendor",
    }


def _target_name(output: str) -> str:
    match = re.search(r"(?:^|/)cmakefiles/([^/]+)\.dir/", output, re.IGNORECASE)
    return match.group(1) if match else "other"


def _source_lines(path: str) -> int:
    try:
        with Path(path).open("rb") as stream:
            return sum(1 for _ in stream)
    except OSError:
        return 0


def collect(
    build_dir: Path, ninja: str = "ninja"
) -> tuple[
    list[dict[str, Any]],
    list[dict[str, Any]],
    list[dict[str, Any]],
    list[dict[str, Any]],
    list[dict[str, Any]],
    int,
]:
    database_path = build_dir / "compile_commands.json"
    log_path = build_dir / ".ninja_log"
    if not database_path.is_file():
        raise FileNotFoundError(f"compile database not found: {database_path}")
    if not log_path.is_file():
        raise FileNotFoundError(f"Ninja build log not found: {log_path}")

    outputs = _compile_outputs(database_path)
    durations = _ninja_durations(log_path)
    dep_result = subprocess.run(
        [ninja, "-C", str(build_dir), "-t", "deps"],
        check=False,
        text=True,
        capture_output=True,
    )
    if dep_result.returncode:
        raise RuntimeError(f"ninja -t deps failed: {dep_result.stderr.strip()}")
    dependencies = _ninja_dependencies(dep_result.stdout, build_dir)
    records = [
        {
            "source": source,
            "output": output,
            "target": _target_name(output),
            "elapsed_ms": durations[output],
            "source_lines": _source_lines(source),
            "dependency_count": len(dependencies.get(output, [])),
            "first_party_header_count": 0,
            "bundled_header_count": 0,
            "external_dependency_count": 0,
        }
        for output, source in outputs.items()
        if output in durations
    ]
    header_consumers: Counter[str] = Counter()
    bundled_header_consumers: Counter[str] = Counter()
    for item in records:
        source_key = _path_key(item["source"])
        first_party_headers: set[str] = set()
        bundled_headers: set[str] = set()
        external_count = 0
        for dependency in dependencies.get(item["output"], []):
            if dependency == source_key:
                continue
            relative = _project_relative(dependency)
            if relative is None:
                external_count += 1
            elif _is_bundled_or_generated_header(relative):
                bundled_headers.add(dependency)
            else:
                first_party_headers.add(dependency)
        item["first_party_header_count"] = len(first_party_headers)
        item["bundled_header_count"] = len(bundled_headers)
        item["external_dependency_count"] = external_count
        header_consumers.update(first_party_headers)
        bundled_header_consumers.update(bundled_headers)

    records.sort(key=lambda item: (-item["elapsed_ms"], item["source"].casefold()))
    headers = [
        {"path": _project_relative(path) or path, "translation_units": count}
        for path, count in header_consumers.items()
    ]
    headers.sort(key=lambda item: (-item["translation_units"], item["path"].casefold()))
    bundled_headers = [
        {"path": _project_relative(path) or path, "translation_units": count}
        for path, count in bundled_header_consumers.items()
    ]
    bundled_headers.sort(
        key=lambda item: (-item["translation_units"], item["path"].casefold())
    )

    target_totals: Counter[str] = Counter()
    target_units: Counter[str] = Counter()
    for item in records:
        target_totals[item["target"]] += item["elapsed_ms"]
        target_units[item["target"]] += 1
    targets = [
        {"target": name, "elapsed_ms": elapsed, "translation_units": target_units[name]}
        for name, elapsed in target_totals.items()
    ]
    targets.sort(key=lambda item: -item["elapsed_ms"])

    compile_outputs = set(outputs)
    link_records = [
        {"output": output, "elapsed_ms": elapsed_ms}
        for output, elapsed_ms in durations.items()
        if output not in compile_outputs
        and Path(output).suffix.casefold() in {".exe", ".dll", ".a", ".lib"}
    ]
    link_records.sort(key=lambda item: -item["elapsed_ms"])
    return (
        records,
        headers,
        bundled_headers,
        targets,
        link_records,
        len(outputs) - len(records),
    )


def _load_baseline(path: Path) -> dict[str, Any]:
    payload = json.loads(path.read_text(encoding="utf-8"))
    if payload.get("schema") not in {1, 2} or not isinstance(
        payload.get("records"), list
    ):
        raise ValueError(f"unsupported compile-time baseline format: {path}")
    return {
        "records": {
            str(item["source"]): item
            for item in payload["records"]
            if "source" in item and "elapsed_ms" in item
        },
        "header_fanout": {
            str(item["path"]): int(item["translation_units"])
            for item in payload.get("header_fanout", [])
            if "path" in item and "translation_units" in item
        },
        "target_totals": {
            str(item["target"]): int(item["elapsed_ms"])
            for item in payload.get("target_totals", [])
            if "target" in item and "elapsed_ms" in item
        },
    }


def _snapshot(
    build_dir: Path,
    records: list[dict[str, Any]],
    headers: list[dict[str, Any]],
    bundled_headers: list[dict[str, Any]],
    targets: list[dict[str, Any]],
    link_records: list[dict[str, Any]],
) -> dict[str, Any]:
    return {
        "schema": 2,
        "build_dir": str(build_dir.resolve(strict=False)),
        "records": records,
        "header_fanout": headers,
        "bundled_header_fanout": bundled_headers,
        "target_totals": targets,
        "link_actions": link_records,
    }


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Summarize C++ compile time, include fanout, and link actions from CMake/Ninja."
    )
    parser.add_argument("build_dir", type=Path, help="CMake/Ninja binary directory")
    parser.add_argument("--ninja", default="ninja", help="Ninja executable")
    parser.add_argument("--top", type=int, default=20, help="number of slowest files")
    parser.add_argument(
        "--baseline", type=Path, help="compare with a saved JSON report"
    )
    parser.add_argument(
        "--write-json", type=Path, help="save this report for later comparison"
    )
    arguments = parser.parse_args()

    try:
        build_dir = arguments.build_dir.resolve(strict=False)
        if not build_dir.is_dir():
            raise FileNotFoundError(f"build directory not found: {build_dir}")
        (
            records,
            headers,
            bundled_headers,
            targets,
            link_records,
            unmatched,
        ) = collect(build_dir, arguments.ninja)
        baseline = _load_baseline(arguments.baseline) if arguments.baseline else {}
        snapshot = _snapshot(
            build_dir, records, headers, bundled_headers, targets, link_records
        )
        if arguments.write_json:
            arguments.write_json.parent.mkdir(parents=True, exist_ok=True)
            arguments.write_json.write_text(
                json.dumps(snapshot, indent=2) + "\n", encoding="utf-8"
            )
    except (OSError, RuntimeError, ValueError, KeyError, json.JSONDecodeError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2

    if not records:
        print("No completed C++ compile commands found in this Ninja build log.")
        return 0

    total_ms = sum(item["elapsed_ms"] for item in records)
    print(f"C++ translation units recorded: {len(records)}")
    print(f"Summed compiler time: {total_ms / 1000:.1f} s (CPU time; not wall time)")
    print(
        "Header dependencies (first-party / bundled-generated / external): "
        f"{sum(item['first_party_header_count'] for item in records):,} / "
        f"{sum(item['bundled_header_count'] for item in records):,} / "
        f"{sum(item['external_dependency_count'] for item in records):,} "
        "(per-translation-unit transitive fanout)"
    )
    print(f"Distinct first-party headers: {len(headers)}")
    if unmatched:
        print(f"Compile database entries without a Ninja timing record: {unmatched}")
    print("\nSlowest translation units:")
    print("  seconds  delta  lines  1st/bundled/ext deps  source")
    for item in records[: max(0, arguments.top)]:
        source = item["source"]
        elapsed_ms = item["elapsed_ms"]
        change = ""
        baseline_record = baseline.get("records", {}).get(source)
        if baseline_record:
            delta = elapsed_ms - int(baseline_record["elapsed_ms"])
            change = f"{delta / 1000:+.1f}s"
        fanout = (
            f"{item['first_party_header_count']}/"
            f"{item['bundled_header_count']}/{item['external_dependency_count']}"
        )
        print(
            f"  {elapsed_ms / 1000:7.2f}  {change:>6}  "
            f"{item['source_lines']:5}  {fanout:>21}  {source}"
        )
    print("\nHighest-fanout project headers:")
    print("  units  delta  header")
    baseline_fanout = baseline.get("header_fanout", {})
    for item in headers[: max(0, arguments.top)]:
        path = item["path"]
        old_count = baseline_fanout.get(path)
        delta = (
            f"{item['translation_units'] - old_count:+d}"
            if old_count is not None
            else ""
        )
        print(f"  {item['translation_units']:5}  {delta:>5}  {path}")
    print("\nHighest-fanout bundled/generated headers:")
    print("  translation units  header")
    for item in bundled_headers[: max(0, arguments.top)]:
        print(f"  {item['translation_units']:17}  {item['path']}")
    print("\nBuild-target compile totals:")
    baseline_targets = baseline.get("target_totals", {})
    for item in targets[: max(0, min(arguments.top, 10))]:
        old_total = baseline_targets.get(item["target"])
        delta = (
            f"{(item['elapsed_ms'] - old_total) / 1000:+.2f}s"
            if old_total is not None
            else ""
        )
        print(
            f"  {item['elapsed_ms'] / 1000:7.2f}s  {delta:>8}  "
            f"{item['translation_units']:4} translation units  {item['target']}"
        )
    if link_records:
        print("\nSlowest link/archive actions:")
        for item in link_records[: max(0, min(arguments.top, 10))]:
            print(f"  {item['elapsed_ms'] / 1000:7.2f}s  {item['output']}")
    if arguments.write_json:
        print(f"\nSaved report: {arguments.write_json}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
