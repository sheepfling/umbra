#!/usr/bin/env python3
"""Relink existing Ninja TCK objects for an isolated installed-package gate.

This is for a configured build whose full rebuild would compile unrelated work.
It never replaces the shared TCK executable and refuses missing or stale
scenario entry-point objects. The provider remains the installed package used
by the configured adapter, and the selected CTest commands retain its inputs.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import shlex
import subprocess
import sys
from pathlib import Path

TARGET = "portable-tck/hla_rti_cpp_tck" + (".exe" if os.name == "nt" else "")
ADAPTER_SUPPORT = (
    Path(__file__).resolve().parents[1]
    / "packages"
    / "hla-rti-cpp-tck"
    / "adapters"
    / "current-process"
)
sys.path.insert(0, str(ADAPTER_SUPPORT))

from tck_evidence import (
    TckEvidencePayload,
    TckEvidenceResult,
    combine_tck_evidence,
    load_tck_evidence,
)


def configured_link(build: Path) -> tuple[list[str], dict[str, str]]:
    lines = (build / "build.ninja").read_text(encoding="utf-8").splitlines()
    marker = f"build {TARGET}: "
    matches = [index for index, line in enumerate(lines) if line.startswith(marker)]
    if len(matches) != 1:
        raise RuntimeError("expected exactly one portable TCK Ninja link edge")
    index = matches[0]
    inputs = lines[index][len(marker) :].split(" | ", 1)[0].split()
    if not inputs or not inputs[0].startswith("CXX_EXECUTABLE_LINKER__"):
        raise RuntimeError("unexpected portable TCK Ninja link rule")
    objects = inputs[1:]
    if not objects or any(not name.endswith((".obj", ".o")) for name in objects):
        raise RuntimeError("portable TCK link inputs are not C++ object files")
    missing = [name for name in objects if not (build / name).is_file()]
    if missing:
        raise RuntimeError(
            f"focused link needs {len(missing)} missing objects; run normal build"
        )
    fields: dict[str, str] = {}
    for line in lines[index + 1 :]:
        if not line.startswith("  "):
            break
        key, separator, value = line.strip().partition(" = ")
        if separator:
            fields[key] = value
    if "LINK_LIBRARIES" not in fields:
        raise RuntimeError(
            "portable TCK Ninja link edge has no installed-package libraries"
        )
    return objects, fields


def check_fresh(build: Path, source: Path, objects: list[str]) -> None:
    matches = [
        name
        for name in objects
        if Path(name).name in {source.name + ".obj", source.name + ".o"}
    ]
    if len(matches) != 1:
        raise RuntimeError(f"expected one link object for {source}")
    obj = build / matches[0]
    if obj.stat().st_mtime_ns < source.stat().st_mtime_ns:
        raise RuntimeError(f"compile the focused object first: {obj}")


def compiler_from_cache(build: Path) -> str:
    cache = (build / "CMakeCache.txt").read_text(encoding="utf-8")
    matches = re.findall(r"^CMAKE_CXX_COMPILER:[^=]+=(.+)$", cache, re.MULTILINE)
    if len(matches) != 1:
        raise RuntimeError("configured C++ compiler is missing or ambiguous")
    return matches[0]


def selected_tests(
    build: Path, scenarios: list[str], executable: Path
) -> tuple[str, list[list[str]]]:
    source = (build / "portable-tck" / "CTestTestfile.cmake").read_text(
        encoding="utf-8"
    )
    lines: list[str] = []
    commands: list[list[str]] = []
    for scenario in scenarios:
        slug = scenario.replace(".", "_").replace("-", "_")
        for model in ("evoked", "immediate"):
            name = f"hla_rti_cpp_tck.{slug}.{model}"
            selected = [
                line
                for line in source.splitlines()
                if f"[=[{name}]=]" in line
                and line.startswith(("add_test(", "set_tests_properties("))
            ]
            if len(selected) != 2:
                raise RuntimeError(f"expected one test and one property row for {name}")
            add_test = next(line for line in selected if line.startswith("add_test("))
            tokens = re.findall(r'"(?:\\.|[^"\\])*"', add_test)
            argv = [json.loads(token) for token in tokens]
            if not argv or Path(argv[0]).name != Path(TARGET).name:
                raise RuntimeError(f"unexpected executable in {name}")
            argv[0] = str(executable)
            commands.append(argv)
            lines.extend(
                line.replace(tokens[0], json.dumps(str(executable)))
                if line.startswith("add_test(")
                else line
                for line in selected
            )
    return "\n".join(lines) + "\n", commands


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-directory", required=True, type=Path)
    parser.add_argument("--scenario", action="append", required=True)
    parser.add_argument(
        "--source",
        action="append",
        required=True,
        type=Path,
        help="C++ source with a focused object that must be current",
    )
    parser.add_argument(
        "--linker-flag",
        action="append",
        default=[],
        help="Optional linker driver flag for a constrained focused relink",
    )
    parser.add_argument("--results", required=True, type=Path)
    args = parser.parse_args()
    build = args.build_directory.resolve()
    results = args.results.resolve()
    objects, fields = configured_link(build)
    repository = Path(__file__).resolve().parents[1]
    sources = [
        repository / "packages/hla-rti-cpp-tck/src/main.cpp",
        *(
            source if source.is_absolute() else repository / source
            for source in args.source
        ),
    ]
    for source in sources:
        check_fresh(build, source, objects)
    compiler = compiler_from_cache(build)
    rsp = build / "CMakeFiles" / "hla_rti_cpp_tck_focused.rsp"
    rsp.write_text(
        " ".join(objects)
        + " "
        + fields.get("LINK_PATH", "")
        + " "
        + fields["LINK_LIBRARIES"]
        + "\n",
        encoding="utf-8",
    )
    executable = (
        build
        / "portable-tck"
        / ("hla_rti_cpp_tck_focused" + (".exe" if os.name == "nt" else ""))
    )
    command = [
        compiler,
        *shlex.split(fields.get("FLAGS", "")),
        *shlex.split(fields.get("LINK_FLAGS", "")),
        *args.linker_flag,
        f"@{rsp.relative_to(build)}",
        "-o",
        str(executable.relative_to(build)),
    ]
    if os.name == "nt":
        command.extend(
            [
                "-Wl,--out-implib,portable-tck/libhla_rti_cpp_tck_focused.dll.a",
                "-Wl,--major-image-version,0,--minor-image-version,0",
            ]
        )
    print(
        f"Relinking {len(objects)} existing objects without recompilation", flush=True
    )
    subprocess.run(command, cwd=build, check=True)
    test_file, direct_commands = selected_tests(build, args.scenario, executable)
    test_directory = build / "focused-cpp-tck"
    test_directory.mkdir(exist_ok=True)
    (test_directory / "CTestTestfile.cmake").write_text(test_file, encoding="utf-8")
    subprocess.run(
        [
            "ctest",
            "--test-dir",
            str(test_directory),
            "--no-tests=error",
            "--output-on-failure",
        ],
        check=True,
    )
    evidence_payloads: list[TckEvidencePayload] = []
    results.parent.mkdir(parents=True, exist_ok=True)
    for index, argv in enumerate(direct_commands):
        child_result = results.with_name(f"{results.stem}-child-{index}.json")
        subprocess.run([*argv, "--results", str(child_result)], check=True)
        evidence_payloads.append(load_tck_evidence(child_result))
    combined = combine_tck_evidence(evidence_payloads, "both")
    direct_results: list[TckEvidenceResult] = [
        result for payload in evidence_payloads for result in payload.results
    ]
    results.write_text(json.dumps(combined, indent=2) + "\n", encoding="utf-8")
    if len(direct_results) != len(direct_commands) or any(
        result["status"] != "passed" for result in direct_results
    ):
        raise RuntimeError("focused direct gate was not entirely green")
    print(
        f"Focused direct gate passed {len(direct_results)}/{len(direct_commands)}; {results}"
    )


if __name__ == "__main__":
    main()
