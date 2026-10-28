"""Portable, policy-driven C++ source, build, test, and package quality gates.

Each check emits the same JSON verdict shape. Project-specific paths, targets,
test selectors, and requirement identifiers belong in the supplied config.
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
import time
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import cast

from .common import _config, _emit, _finding, _report
from .graph import check_graph
from .manifest import _manifest_binding, create_manifest
from .models import (
    GraphPolicy,
    PackagePolicy,
    SourcePolicy,
    TestsPolicy,
    VerdictPolicy,
)
from .package_gate import check_package
from .source import check_source
from .test_gate import check_tests
from .verdict import check_verdict


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    manifest_command = commands.add_parser("manifest")
    manifest_command.add_argument("--source-root", required=True, type=Path)
    manifest_command.add_argument("--run-id", required=True)
    manifest_command.add_argument("--configuration", required=True)
    manifest_command.add_argument("--toolchain", required=True)
    manifest_command.add_argument("--artifact-root", type=Path)
    manifest_command.add_argument("--target", action="append", required=True)
    manifest_command.add_argument("--expected-executable", action="append", default=[])
    manifest_command.add_argument("--input", action="append", required=True)
    manifest_command.add_argument("--require-clean", action="store_true")
    manifest_command.add_argument("--output", required=True, type=Path)
    build_command = commands.add_parser("run-build")
    build_command.add_argument("--manifest", required=True, type=Path)
    build_command.add_argument("--run-id", required=True)
    build_command.add_argument("--cwd", type=Path, default=Path.cwd())
    build_command.add_argument("--output", required=True, type=Path)
    build_command.add_argument("build_command", nargs=argparse.REMAINDER)
    for name in ("source", "graph", "tests", "package", "verdict"):
        child = commands.add_parser(name)
        if name != "verdict":
            child.add_argument(
                "root",
                type=Path,
                nargs="?" if name == "graph" else None,
                default=Path(".") if name == "graph" else None,
                help="source root or CMake build directory",
            )
        child.add_argument("--config", required=True, type=Path)
        child.add_argument("--output", type=Path)
        child.add_argument("--run-id")
        child.add_argument("--manifest", type=Path)
        if name == "tests":
            child.add_argument(
                "--inventory-json", "--ctest-json", dest="inventory_json", type=Path
            )
            child.add_argument("--junit", type=Path)
            child.add_argument("--execution-json", type=Path)
        if name == "graph":
            child.add_argument("--graph-report", type=Path)
            child.add_argument("--compile-report", type=Path)
    args = parser.parse_args(argv)
    try:
        if args.command == "manifest":
            manifest = create_manifest(
                args.source_root,
                args.run_id,
                args.configuration,
                args.toolchain,
                args.target,
                args.expected_executable,
                args.input,
                args.require_clean,
                args.artifact_root,
            )
            args.output.parent.mkdir(parents=True, exist_ok=True)
            args.output.write_text(
                json.dumps(manifest, indent=2, sort_keys=True) + "\n",
                encoding="utf-8",
            )
            print(f"manifest: {args.output}")
            return 0
        if args.command == "run-build":
            command = args.build_command
            if command and command[0] == "--":
                command = command[1:]
            if not command:
                raise ValueError("run-build needs a command after --")
            manifest_digest = _manifest_binding(args.manifest, args.run_id)[1]
            started = time.monotonic()
            result = subprocess.run(command, cwd=args.cwd.resolve(), check=False)
            report = _report(
                "build",
                []
                if result.returncode == 0
                else [_finding("build_command_failed", f"exit {result.returncode}")],
                {
                    "command": command,
                    "cwd": str(args.cwd.resolve()),
                    "exit_code": result.returncode,
                    "duration_seconds": round(time.monotonic() - started, 3),
                },
                args.run_id,
            )
            report["manifest_sha256"] = manifest_digest
            return _emit(report, args.output)
        section = _config(args.config, args.command)
        if args.command == "source":
            report = check_source(args.root, cast(SourcePolicy, section), args.run_id)
        elif args.command == "graph":
            report = check_graph(
                args.root,
                cast(GraphPolicy, section),
                args.compile_report,
                args.run_id,
                args.graph_report,
            )
        elif args.command == "tests":
            report = check_tests(
                args.root,
                cast(TestsPolicy, section),
                args.inventory_json,
                args.junit,
                args.run_id,
                args.execution_json,
            )
        elif args.command == "package":
            report = check_package(args.root, cast(PackagePolicy, section), args.run_id)
        else:
            report = check_verdict(
                args.config, cast(VerdictPolicy, section), args.run_id, args.manifest
            )
        if args.manifest and args.command != "verdict":
            report["manifest_sha256"] = _manifest_binding(args.manifest, args.run_id)[1]
        return _emit(report, args.output)
    except (
        OSError,
        ValueError,
        KeyError,
        TypeError,
        ET.ParseError,
        json.JSONDecodeError,
    ) as error:
        print(f"cpp common quality: {error}", file=sys.stderr)
        return 2
