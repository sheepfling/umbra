#!/usr/bin/env python3
"""Run the public C++ Federate Resigned TCK with an adapter fixture trigger."""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
import tempfile
import time
from pathlib import Path


ROOT = Path(__file__).resolve().parents[4]


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tck-executable", type=Path, required=True)
    parser.add_argument("--process-fixture", type=Path, required=True)
    parser.add_argument("--provider-bin", type=Path, required=True)
    parser.add_argument("--startup-timeout-s", type=float, default=30.0)
    parser.add_argument("--shutdown-timeout-s", type=float, default=15.0)
    parser.add_argument("tck_arguments", nargs=argparse.REMAINDER)
    arguments = parser.parse_args()
    if arguments.tck_arguments[:1] == ["--"]:
        arguments.tck_arguments = arguments.tck_arguments[1:]
    return arguments


def runtime_environment(arguments: argparse.Namespace) -> dict[str, str]:
    environment = dict(os.environ)
    runtime_directories = [
        str(arguments.process_fixture.resolve().parent),
        str(arguments.provider_bin.resolve()),
    ]
    if os.name == "nt":
        variable = "PATH"
    elif sys.platform == "darwin":
        variable = "DYLD_LIBRARY_PATH"
    else:
        variable = "LD_LIBRARY_PATH"
    existing = environment.get(variable)
    if existing:
        runtime_directories.append(existing)
    environment[variable] = os.pathsep.join(runtime_directories)
    return environment


def wait_for_port(
    marker: Path,
    fixture: subprocess.Popen[bytes],
    timeout_s: float,
) -> int:
    deadline = time.monotonic() + timeout_s
    while time.monotonic() < deadline:
        try:
            port = int(marker.read_text(encoding="utf-8").strip())
        except (FileNotFoundError, OSError, ValueError):
            port = 0
        if 0 < port <= 65535:
            return port
        exit_code = fixture.poll()
        if exit_code is not None:
            raise RuntimeError(
                "The forced-resignation fixture exited before listening "
                f"(exit code {exit_code})"
            )
        time.sleep(0.05)
    raise RuntimeError(
        f"The forced-resignation fixture did not publish {marker.name} "
        f"within {timeout_s:g} seconds"
    )


def run() -> int:
    arguments = parse_arguments()
    for path, label in (
        (arguments.tck_executable, "TCK executable"),
        (arguments.process_fixture, "process fixture"),
    ):
        if not path.is_file():
            raise FileNotFoundError(f"The {label} is not a file: {path}")
    if not arguments.provider_bin.is_dir():
        raise FileNotFoundError(
            "The provider runtime directory does not exist: "
            f"{arguments.provider_bin}"
        )
    if not arguments.tck_arguments:
        raise ValueError("The forced-resignation adapter received no TCK arguments")

    environment = runtime_environment(arguments)
    fixture: subprocess.Popen[bytes] | None = None
    with tempfile.TemporaryDirectory(prefix="cpp-tck-federate-resigned-") as raw:
        coordination_directory = Path(raw)
        fixture = subprocess.Popen(
            [
                str(arguments.process_fixture.resolve()),
                "public-server-forced-resignation",
                str(coordination_directory),
            ],
            cwd=ROOT,
            env=environment,
        )
        try:
            port = wait_for_port(
                coordination_directory / "port.txt",
                fixture,
                arguments.startup_timeout_s,
            )
            tck_command = [
                str(arguments.tck_executable.resolve()),
                *arguments.tck_arguments,
                "--rti-address",
                f"tcp://127.0.0.1:{port}",
                "--federate-resignation-coordination-directory",
                str(coordination_directory),
                "--federation-name",
                "process-execution",
                "--owner-name",
                "package-process-sender",
                "--member-name",
                "package-process-receiver",
                "--federate-type",
                "package-process-type",
            ]
            result = subprocess.run(
                tck_command,
                cwd=ROOT,
                env=environment,
                check=False,
            )
            if result.returncode != 0:
                return result.returncode
            try:
                fixture_result = fixture.wait(
                    timeout=arguments.shutdown_timeout_s
                )
            except subprocess.TimeoutExpired as error:
                raise RuntimeError(
                    "The forced-resignation fixture did not finish after the "
                    "TCK completed"
                ) from error
            if fixture_result != 0:
                raise RuntimeError(
                    "The forced-resignation fixture failed "
                    f"(exit code {fixture_result})"
                )
            if not (coordination_directory / "server.ok").is_file():
                raise RuntimeError(
                    "The forced-resignation fixture omitted its completion marker"
                )
            return 0
        finally:
            if fixture.poll() is None:
                fixture.terminate()
                try:
                    fixture.wait(timeout=2.0)
                except subprocess.TimeoutExpired:
                    fixture.kill()
                    fixture.wait(timeout=2.0)


if __name__ == "__main__":
    try:
        raise SystemExit(run())
    except Exception as error:
        print(f"Forced-resignation TCK adapter failed: {error}", file=sys.stderr)
        raise SystemExit(1)
