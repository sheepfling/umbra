#!/usr/bin/env python3
"""Survey native C++ Catch2 stems that do not yet have a portable TCK ID.

Private-boundary checks include the closure of local quoted includes, so a
wrapper cannot hide a private harness behind an otherwise clean source file.
"""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path


REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
LOCAL_INCLUDE_PATTERN = re.compile(r'^\s*#include\s+"([^"]+)"', re.MULTILINE)


PRIVATE_MARKERS = (
    "UMBRA_SOURCE_DIRECTORY",
    "EmbeddedFederationRegistry",
    "FederationRegistry",
    "umbra::detail",
    "umbra_binding_detail",
    "#include \"internal/",
    "#include \"umbra/",
    "#include <umbra/",
    "private registry-bound",
    "registry-binding",
    "[process-boundary]",
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--catalog",
        type=Path,
        default=Path("compliance/catalogs/cpp-tck-scenario-catalog.json"),
    )
    parser.add_argument("--native-tests", type=Path, default=Path("cpp/tests"))
    return parser.parse_args()


def runner_id_for(path: Path) -> str:
    stem = path.stem
    if not stem.endswith("_catch2"):
        raise ValueError(f"not a Catch2 source: {path}")
    stem = stem[: -len("_catch2")]
    return "cpp-tck." + stem.replace("_", "-")


def resolve_local_include(source_path: Path, include_name: str) -> Path | None:
    candidates = (
        source_path.parent / include_name,
        REPOSITORY_ROOT / "cpp" / "tests" / include_name,
        REPOSITORY_ROOT / "cpp" / "src" / include_name,
        REPOSITORY_ROOT / "cpp" / "include" / include_name,
        REPOSITORY_ROOT / include_name,
    )
    for candidate in candidates:
        resolved = candidate.resolve()
        if resolved.is_file() and (
            resolved == REPOSITORY_ROOT or REPOSITORY_ROOT in resolved.parents
        ):
            return resolved
    return None


def private_markers_for(path: Path) -> tuple[list[str], list[str]]:
    root = path.resolve()
    pending = [root]
    visited: set[Path] = set()
    markers: set[str] = set()
    marker_files: set[str] = set()
    while pending:
        current = pending.pop()
        if current in visited or not current.is_file():
            continue
        visited.add(current)
        source = current.read_text(encoding="utf-8", errors="ignore")
        current_markers = [marker for marker in PRIVATE_MARKERS if marker in source]
        markers.update(current_markers)
        if current_markers and current != root:
            marker_files.add(current.relative_to(REPOSITORY_ROOT).as_posix())
        for include_name in LOCAL_INCLUDE_PATTERN.findall(source):
            included = resolve_local_include(current, include_name)
            if included is not None and included not in visited:
                pending.append(included)
    return sorted(markers), sorted(marker_files)


def semantic_catalog_matches(catalog: dict[str, object]) -> dict[str, str]:
    matches: dict[str, str] = {}
    for scenario in catalog["scenarios"]:
        scenario_id = scenario["id"]
        for native_runner_id in scenario.get("native_equivalent_runner_ids", []):
            previous = matches.get(native_runner_id)
            if previous is not None and previous != scenario_id:
                raise ValueError(
                    f"native runner {native_runner_id} maps to both "
                    f"{previous} and {scenario_id}"
                )
            matches[native_runner_id] = scenario_id
    return matches


def classify(
    path: Path,
    catalog_ids: set[str],
    semantic_matches: dict[str, str],
) -> dict[str, object]:
    runner_id = runner_id_for(path)
    private_markers, private_marker_files = private_markers_for(path)
    source = path.read_text(encoding="utf-8")
    integration = "[integration]" in source
    semantic_match = semantic_matches.get(runner_id)
    return {
        "path": path.as_posix(),
        "runner_id": runner_id,
        "catalog_match": runner_id in catalog_ids,
        "semantic_catalog_match": semantic_match,
        "integration": integration,
        "private_markers": private_markers,
        "private_marker_files": private_marker_files,
        "portable_candidate": (
            integration
            and not private_markers
            and runner_id not in catalog_ids
            and semantic_match is None
        ),
    }


def main() -> int:
    args = parse_args()
    catalog = json.loads(args.catalog.read_text(encoding="utf-8"))
    catalog_ids = {entry["id"] for entry in catalog["scenarios"]}
    semantic_matches = semantic_catalog_matches(catalog)
    reports = [
        classify(path, catalog_ids, semantic_matches)
        for path in sorted(args.native_tests.glob("*_catch2.cpp"))
    ]
    semantic_covered = [
        report for report in reports if report["semantic_catalog_match"] is not None
    ]
    unmatched = [
        report
        for report in reports
        if not report["catalog_match"] and report["semantic_catalog_match"] is None
    ]
    candidates = [report for report in unmatched if report["portable_candidate"]]
    print(
        json.dumps(
            {
                "catalog_scenario_count": len(catalog_ids),
                "native_catch2_count": len(reports),
                "semantic_coverage_count": len(semantic_covered),
                "semantic_coverage": semantic_covered,
                "unmatched_native_count": len(unmatched),
                "portable_candidate_count": len(candidates),
                "portable_candidates": candidates,
                "unmatched_native": unmatched,
            },
            indent=2,
            sort_keys=True,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
