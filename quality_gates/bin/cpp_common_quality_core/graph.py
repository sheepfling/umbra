"""CMake codemodel and build-graph gate."""

from __future__ import annotations

import time
from collections import defaultdict
from pathlib import Path
from typing import cast

from .common import _finding, _matches, _read_json, _report
from .models import (
    CMakeCodemodel,
    CMakeIndex,
    CMakeTargetRecord,
    CompileReport,
    GateReport,
    GraphPolicy,
    GraphTarget,
    HeaderFanout,
    JsonObject,
    NormalizedGraphReport,
)


def _codemodel_reply(build: Path) -> tuple[Path, CMakeCodemodel]:
    reply_dir = build / ".cmake" / "api" / "v1" / "reply"
    indexes = sorted(
        reply_dir.glob("index-*.json"), key=lambda path: path.stat().st_mtime
    )
    if not indexes:
        raise ValueError(
            "CMake File API codemodel is missing; create query/codemodel-v2 and reconfigure"
        )
    index = cast(CMakeIndex, _read_json(indexes[-1]))
    entry = index.get("reply", {}).get("codemodel-v2")
    if entry is None:
        entry = next(
            (
                item
                for item in index.get("objects", [])
                if item.get("kind") == "codemodel"
                and item.get("version", {}).get("major") == 2
            ),
            None,
        )
    if not isinstance(entry, dict) or not entry.get("jsonFile"):
        raise ValueError("latest CMake File API index has no codemodel-v2 reply")
    return reply_dir, cast(CMakeCodemodel, _read_json(reply_dir / entry["jsonFile"]))


def _graph_targets(
    build: Path, policy: GraphPolicy, graph_report: Path | None
) -> tuple[dict[str, GraphTarget], str | None]:
    if graph_report is not None:
        graph = cast(NormalizedGraphReport, _read_json(graph_report))
        if graph.get("schema") != 1 or not isinstance(graph.get("targets"), list):
            raise ValueError("graph report needs schema 1 and a targets array")
        configuration = graph.get("configuration")
        if policy.get("configuration") not in (None, configuration):
            raise ValueError("graph report has the wrong configuration")
        raw_targets = graph["targets"]
    else:
        reply_dir, codemodel = _codemodel_reply(build)
        selected = [
            item
            for item in codemodel.get("configurations", [])
            if policy.get("configuration") is None
            or item.get("name") == policy.get("configuration")
        ]
        if len(selected) != 1:
            raise ValueError("select exactly one CMake codemodel configuration")
        configuration = selected[0].get("name")
        raw_targets = []
        for entry in selected[0].get("targets", []):
            target = cast(CMakeTargetRecord, _read_json(reply_dir / entry["jsonFile"]))
            raw_targets.append(
                {
                    "id": target["id"],
                    "name": target["name"],
                    "type": target["type"],
                    "compiled_source_count": sum(
                        "compileGroupIndex" in source
                        for source in target.get("sources", [])
                    ),
                    "artifacts": [
                        artifact["path"] for artifact in target.get("artifacts", [])
                    ],
                    "dependencies": [
                        dependency["id"]
                        for dependency in target.get("dependencies", [])
                    ],
                }
            )
    targets: dict[str, GraphTarget] = {}
    for target in raw_targets:
        if not isinstance(target, dict) or not all(
            isinstance(target.get(key), str) and target[key]
            for key in ("id", "name", "type")
        ):
            raise ValueError("graph target needs nonempty id, name, and type")
        target_type = target["type"].upper()
        if target_type == "EXECUTABLE" and "compiled_source_count" not in target:
            raise ValueError("executable graph target needs compiled_source_count")
        count = target.get("compiled_source_count", 0)
        dependencies = target.get("dependencies", [])
        artifacts = target.get("artifacts", [])
        if (
            not isinstance(count, int)
            or isinstance(count, bool)
            or count < 0
            or not isinstance(dependencies, list)
            or any(not isinstance(item, str) or not item for item in dependencies)
            or not isinstance(artifacts, list)
            or any(not isinstance(item, str) or not item for item in artifacts)
        ):
            raise ValueError(
                "graph target needs a source count, dependency IDs, and artifact paths"
            )
        if target["id"] in targets:
            raise ValueError(f"duplicate graph target id: {target['id']}")
        normalized_artifacts = []
        for value in artifacts:
            artifact_path = Path(value)
            if artifact_path.is_absolute():
                if graph_report is not None:
                    raise ValueError("normalized graph artifact paths must be relative")
                try:
                    artifact_path = artifact_path.resolve().relative_to(build.resolve())
                except ValueError as error:
                    raise ValueError(
                        f"CMake artifact is outside the build root: {value}"
                    ) from error
            if ".." in artifact_path.parts or artifact_path == Path("."):
                raise ValueError(f"invalid graph artifact path: {value}")
            normalized_artifacts.append(artifact_path.as_posix())
        targets[target["id"]] = {
            "id": target["id"],
            "name": target["name"],
            "type": target_type,
            "compiled_source_count": count,
            "dependencies": dependencies,
            "artifacts": normalized_artifacts,
        }
    if graph_report is not None:
        for target in targets.values():
            unknown = set(target["dependencies"]) - targets.keys()
            if unknown:
                raise ValueError(
                    f"graph target {target['id']} references unknown dependencies: {sorted(unknown)}"
                )
    return targets, configuration


def check_graph(
    build: Path,
    policy: GraphPolicy,
    compile_report: Path | None,
    run_id: str | None,
    graph_report: Path | None = None,
) -> GateReport:
    build = build.resolve()
    targets, configuration = _graph_targets(build, policy, graph_report)
    include = policy.get("include_targets", ["*"])
    exclude = policy.get("exclude_targets", [])
    if (
        not isinstance(include, list)
        or not include
        or not isinstance(exclude, list)
        or any(not isinstance(item, str) or not item for item in [*include, *exclude])
    ):
        raise ValueError("graph target filters must be nonempty glob arrays")
    executables = [
        target
        for target in targets.values()
        if target.get("type") == "EXECUTABLE"
        and _matches(target["name"], include)
        and not _matches(target["name"], exclude)
    ]
    static_targets = {
        key: value
        for key, value in targets.items()
        if value.get("type") == "STATIC_LIBRARY"
    }
    fanout: dict[str, list[str]] = defaultdict(list)
    findings: list[dict[str, str]] = []
    if "require_executable_artifacts" in policy and not isinstance(
        policy["require_executable_artifacts"], bool
    ):
        raise TypeError("graph.require_executable_artifacts must be boolean")
    executable_names = [target["name"] for target in executables]
    if len(executable_names) != len(set(executable_names)):
        findings.append(
            _finding("duplicate_executable_name", "selected targets share a name")
        )
    executable_paths: list[str] = []
    for executable in executables:
        paths = executable["artifacts"]
        if policy.get("require_executable_artifacts", False) and not paths:
            findings.append(
                _finding("missing_executable_artifact_path", executable["name"])
            )
        executable_paths.extend(paths)
    if len(executable_paths) != len(set(executable_paths)):
        findings.append(
            _finding(
                "duplicate_executable_artifact_path",
                "selected targets share an artifact path",
            )
        )
    age_limit = int(policy.get("max_evidence_age_seconds", 21600))
    if age_limit < 0:
        raise ValueError("graph.max_evidence_age_seconds must be nonnegative")
    if graph_report is not None:
        evidence_path = graph_report
        if run_id is not None and _read_json(graph_report).get("run_id") != run_id:
            findings.append(_finding("wrong_graph_run_id", str(graph_report)))
    else:
        indexes = list((build / ".cmake" / "api" / "v1" / "reply").glob("index-*.json"))
        evidence_path = max(indexes, key=lambda path: path.stat().st_mtime)
    if time.time() - evidence_path.stat().st_mtime > age_limit:
        findings.append(_finding("stale_graph_evidence", str(evidence_path)))
    for executable in executables:
        for dependency in executable["dependencies"]:
            linked = static_targets.get(dependency)
            if linked is not None:
                fanout[linked["name"]].append(executable["name"])
        source_count = executable["compiled_source_count"]
        limit = policy.get("max_sources_per_executable")
        if limit is not None and source_count > int(limit):
            findings.append(
                _finding(
                    "executable_source_count",
                    f"{source_count} compiled sources > {limit}",
                    path=executable["name"],
                )
            )
    min_count = int(policy.get("min_executable_targets", 1))
    if min_count < 1:
        raise ValueError("graph.min_executable_targets must be at least 1")
    max_count = policy.get("max_executable_targets")
    if len(executables) < min_count:
        findings.append(
            _finding("missing_executable_targets", f"{len(executables)} < {min_count}")
        )
    if max_count is not None and len(executables) > int(max_count):
        findings.append(
            _finding("executable_target_count", f"{len(executables)} > {max_count}")
        )
    fanout_limit = policy.get("max_executables_per_static_library")
    if fanout_limit is not None:
        for name, owners in fanout.items():
            if len(owners) > int(fanout_limit):
                findings.append(
                    _finding(
                        "static_library_executable_fanout",
                        f"{len(owners)} executable targets depend on this archive; limit {fanout_limit}",
                        path=name,
                    )
                )
    ranked = sorted(
        (
            {
                "target": name,
                "executable_count": len(owners),
                "examples": sorted(owners)[:8],
            }
            for name, owners in fanout.items()
        ),
        key=lambda item: item["executable_count"],
        reverse=True,
    )
    header_rows: list[HeaderFanout] = []
    if compile_report is None:
        if "max_header_fanout" in policy or "min_measured_translation_units" in policy:
            findings.append(_finding("missing_compile_report", "pass --compile-report"))
    else:
        compile_data = cast(CompileReport, _read_json(compile_report))
        if time.time() - compile_report.stat().st_mtime > age_limit:
            findings.append(_finding("stale_compile_report", str(compile_report)))
        records = compile_data.get("records")
        headers = compile_data.get("header_fanout")
        if not isinstance(records, list) or not isinstance(headers, list):
            raise ValueError("compile report needs records and header_fanout arrays")
        minimum = int(policy.get("min_measured_translation_units", 1))
        if len(records) < minimum:
            findings.append(
                _finding(
                    "too_few_measured_translation_units", f"{len(records)} < {minimum}"
                )
            )
        header_rows = sorted(
            cast(list[HeaderFanout], headers),
            key=lambda item: item["translation_units"],
            reverse=True,
        )
        limit = policy.get("max_header_fanout")
        if limit is not None:
            for header in header_rows:
                if int(header["translation_units"]) > int(limit):
                    findings.append(
                        _finding(
                            "header_fanout",
                            f"{header['translation_units']} translation units > {limit}",
                            path=header["path"],
                        )
                    )
    return _report(
        "graph",
        findings,
        cast(
            JsonObject,
            {
                "executable_target_count": len(executables),
                "executable_names": sorted(target["name"] for target in executables),
                "executable_paths": sorted(executable_paths),
                "static_library_count": len(static_targets),
                "static_dependency_fanout": ranked[:20],
                "configuration": configuration,
                "highest_header_fanout": header_rows[:20],
            },
        ),
        run_id,
    )
