#!/usr/bin/env python3
"""Run the portable connection-loss TCK case with a process adapter.

The provider fixture is deliberately an adapter input.  The reusable TCK
executable receives only standard API configuration and never starts a
provider-specific process itself.  This wrapper uses argument lists only and
does not require a command shell.
"""

from __future__ import annotations

import argparse
import json
import shutil
import subprocess
import sys
import tempfile
import time
import xml.etree.ElementTree as ET
from pathlib import Path


DEFAULT_FOM = (
    Path(__file__).resolve().parents[4]
    / "third_party"
    / "ieee1516.2-2025"
    / "resources"
    / "examples"
    / "RestaurantFOMmodule-2025.xml"
)


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run cpp-tck.connection-loss-cleanup through a process adapter"
    )
    parser.add_argument("--tck-executable", type=Path, required=True)
    parser.add_argument("--process-fixture", type=Path, required=True)
    parser.add_argument("--fom-path", type=Path, default=DEFAULT_FOM)
    parser.add_argument(
        "--callback-model",
        choices=("both", "evoked", "immediate"),
        default="both",
    )
    parser.add_argument("--provider-id", default="current-process")
    parser.add_argument("--timeout-ms", type=int, default=5000)
    parser.add_argument("--federation-name", default="process-execution")
    parser.add_argument("--owner-name", default="package-process-sender")
    parser.add_argument("--member-name", default="package-process-receiver")
    parser.add_argument("--federate-type", default="package-process-type")
    parser.add_argument("--owner-configuration-name", default="package-process-sender")
    parser.add_argument("--member-configuration-name", default="package-process-receiver")
    parser.add_argument("--configuration-name", default="")
    parser.add_argument("--additional-settings", default="")
    parser.add_argument(
        "--interaction-class",
        default="HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed",
    )
    parser.add_argument("--startup-timeout-s", type=float, default=30.0)
    parser.add_argument("--fixture-timeout-s", type=float, default=30.0)
    parser.add_argument("--results", type=Path)
    parser.add_argument("--junit", type=Path)
    return parser.parse_args()


def wait_for_port(directory: Path, timeout_s: float) -> int:
    port_path = directory / "port.txt"
    deadline = time.monotonic() + timeout_s
    while time.monotonic() < deadline:
        try:
            port = int(port_path.read_text(encoding="utf-8").strip())
        except (FileNotFoundError, OSError, ValueError):
            port = 0
        if 0 < port <= 65535:
            return port
        time.sleep(0.1)
    raise RuntimeError("The provider loss fixture did not publish a listening port")


def wait_for_fixture(process: subprocess.Popen[bytes], timeout_s: float) -> int:
    deadline = time.monotonic() + timeout_s
    while process.poll() is None and time.monotonic() < deadline:
        time.sleep(0.1)
    if process.poll() is None:
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()
        raise RuntimeError("The provider loss fixture did not terminate")
    return int(process.returncode)


def tck_command(
    arguments: argparse.Namespace,
    callback_model: str,
    port: int,
    marker: Path,
    results: Path,
    junit: Path,
) -> list[str]:
    command = [
        str(arguments.tck_executable),
        "--fom",
        str(arguments.fom_path),
        "--scenario",
        "cpp-tck.connection-loss-cleanup",
        "--callback-model",
        callback_model,
        "--provider-id",
        arguments.provider_id,
        "--rti-address",
        f"tcp://127.0.0.1:{port}",
        "--federation-name",
        arguments.federation_name,
        "--owner-name",
        arguments.owner_name,
        "--member-name",
        arguments.member_name,
        "--federate-type",
        arguments.federate_type,
        "--owner-configuration-name",
        arguments.owner_configuration_name,
        "--member-configuration-name",
        arguments.member_configuration_name,
        "--interaction-class",
        arguments.interaction_class,
        "--connection-loss-server-managed",
        "--connection-loss-marker",
        str(marker),
        "--timeout-ms",
        str(arguments.timeout_ms),
        "--results",
        str(results),
        "--junit",
        str(junit),
    ]
    if arguments.configuration_name:
        command.extend(["--configuration-name", arguments.configuration_name])
    if arguments.additional_settings:
        command.extend(["--additional-settings", arguments.additional_settings])
    return command


def run_one(
    arguments: argparse.Namespace,
    callback_model: str,
    root: Path,
) -> tuple[int, dict[str, object], ET.Element]:
    marker_directory = root / callback_model
    marker_directory.mkdir()
    results = marker_directory / "results.json"
    junit = marker_directory / "results.xml"
    marker = marker_directory / "receiver-loss.ok"
    fixture: subprocess.Popen[bytes] | None = None
    try:
        fixture = subprocess.Popen(
            [str(arguments.process_fixture), "public-server-loss", str(marker_directory)]
        )
        port = wait_for_port(marker_directory, arguments.startup_timeout_s)
        tck_result = subprocess.run(
            tck_command(arguments, callback_model, port, marker, results, junit),
            check=False,
        )
        payload = json.loads(results.read_text(encoding="utf-8")) if results.is_file() else {}
        junit_root = ET.parse(junit).getroot() if junit.is_file() else ET.Element("testsuite")
        if tck_result.returncode != 0:
            return int(tck_result.returncode), payload, junit_root
        if not marker.is_file():
            raise RuntimeError("The TCK did not write the connection-loss completion marker")
        fixture_status = wait_for_fixture(fixture, arguments.fixture_timeout_s)
        fixture = None
        if fixture_status != 0:
            raise RuntimeError(
                f"The provider loss fixture returned exit code {fixture_status}"
            )
        return 0, payload, junit_root
    finally:
        if fixture is not None and fixture.poll() is None:
            fixture.terminate()
            try:
                fixture.wait(timeout=5)
            except subprocess.TimeoutExpired:
                fixture.kill()
                fixture.wait()


def write_combined_evidence(
    arguments: argparse.Namespace,
    payloads: list[dict[str, object]],
    junit_roots: list[ET.Element],
) -> None:
    if arguments.results is not None and payloads:
        combined = dict(payloads[0])
        combined["callback_model"] = arguments.callback_model
        combined["results"] = [
            result
            for payload in payloads
            for result in payload.get("results", [])
        ]
        arguments.results.parent.mkdir(parents=True, exist_ok=True)
        arguments.results.write_text(
            json.dumps(combined, indent=2, sort_keys=True) + "\n",
            encoding="utf-8",
        )
    if arguments.junit is not None and junit_roots:
        test_count = sum(int(root.attrib.get("tests", "0")) for root in junit_roots)
        failure_count = sum(int(root.attrib.get("failures", "0")) for root in junit_roots)
        combined_root = ET.Element(
            "testsuite",
            {"name": "hla-rti-cpp-tck", "tests": str(test_count), "failures": str(failure_count)},
        )
        for root in junit_roots:
            combined_root.extend(list(root))
        arguments.junit.parent.mkdir(parents=True, exist_ok=True)
        ET.ElementTree(combined_root).write(
            arguments.junit,
            encoding="utf-8",
            xml_declaration=True,
        )


def run(arguments: argparse.Namespace) -> int:
    for path, description in (
        (arguments.tck_executable, "TCK executable"),
        (arguments.process_fixture, "process fixture"),
        (arguments.fom_path, "FOM"),
    ):
        if not path.is_file():
            raise FileNotFoundError(f"{description} does not exist: {path}")

    root = Path(tempfile.mkdtemp(prefix="hla-rti-cpp-tck-"))
    try:
        models = ("evoked", "immediate") if arguments.callback_model == "both" else (arguments.callback_model,)
        exit_code = 0
        payloads: list[dict[str, object]] = []
        junit_roots: list[ET.Element] = []
        for callback_model in models:
            code, payload, junit_root = run_one(arguments, callback_model, root)
            exit_code = exit_code or code
            if payload:
                payloads.append(payload)
            junit_roots.append(junit_root)
        write_combined_evidence(arguments, payloads, junit_roots)
        return int(exit_code)
    finally:
        shutil.rmtree(root, ignore_errors=True)


def main() -> int:
    try:
        return run(parse_arguments())
    except (OSError, RuntimeError, ValueError) as error:
        print(f"connection-loss adapter failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
