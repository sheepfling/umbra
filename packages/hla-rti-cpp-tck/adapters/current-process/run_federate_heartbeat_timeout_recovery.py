"""Run the provider-specific heartbeat-timeout recovery TCK case."""

from __future__ import annotations

import argparse
import copy
import json
import os
import subprocess
import tempfile
import time
import xml.etree.ElementTree as ET
from pathlib import Path


SCENARIO = "cpp-tck.federate-heartbeat-timeout-recovery"
ADDITIONAL_SETTINGS = (
    "FED_INT_HEART=2;FED_TIMEOUT_HEART=2.5;FED_TIMEOUT_RECONNECT=4"
)


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tck-executable", type=Path, required=True)
    parser.add_argument("--process-fixture", type=Path, required=True)
    parser.add_argument("--fom-path", type=Path, required=True)
    parser.add_argument("--mim-fom", type=Path, required=True)
    parser.add_argument("--time-implementation", required=True)
    parser.add_argument(
        "--callback-model",
        choices=("evoked", "immediate", "both"),
        required=True,
    )
    parser.add_argument("--provider-id", default="current-process")
    parser.add_argument("--timeout-s", type=float, default=30.0)
    parser.add_argument("--results", type=Path, required=True)
    parser.add_argument("--junit", type=Path, required=True)
    return parser.parse_args()


def wait_for_port(
    directory: Path,
    timeout_s: float,
    fixture: subprocess.Popen[bytes],
) -> int:
    port_path = directory / "port.txt"
    deadline = time.monotonic() + timeout_s
    while time.monotonic() < deadline:
        try:
            port = int(port_path.read_text(encoding="utf-8").strip())
        except (FileNotFoundError, OSError, ValueError):
            port = 0
        if 0 < port <= 65535:
            return port
        if fixture.poll() is not None:
            raise RuntimeError(
                "The heartbeat-recovery process fixture exited before publishing its port"
            )
        time.sleep(0.025)
    raise RuntimeError("The heartbeat-recovery fixture did not publish a port")


def run_one(
    arguments: argparse.Namespace,
    callback_model: str,
) -> tuple[dict[str, object], ET.Element]:
    with tempfile.TemporaryDirectory(
        prefix=f"cpp-tck-federate-heartbeat-{callback_model}-",
        dir=arguments.results.parent,
    ) as temporary_directory:
        directory = Path(temporary_directory)
        model_results = directory / "results.json"
        model_junit = directory / "results.xml"
        fixture: subprocess.Popen[bytes] | None = None
        try:
            fixture = subprocess.Popen(
                [
                    str(arguments.process_fixture),
                    "public-server-federate-heartbeat-timeout-recovery",
                    str(directory),
                ]
            )
            port = wait_for_port(directory, arguments.timeout_s, fixture)
            command = [
                str(arguments.tck_executable),
                "--fom",
                str(arguments.fom_path),
                "--mim-fom",
                str(arguments.mim_fom),
                "--time-implementation",
                arguments.time_implementation,
                "--scenario",
                SCENARIO,
                "--callback-model",
                callback_model,
                "--provider-id",
                arguments.provider_id,
                "--rti-address",
                f"tcp://127.0.0.1:{port}",
                "--additional-settings",
                ADDITIONAL_SETTINGS,
                "--federate-heartbeat-recovery-server-managed",
                "--federate-heartbeat-timeout-ready-marker",
                str(directory / "federate-heartbeat-timeout-ready.ok"),
                "--federate-heartbeat-inactive-marker",
                str(directory / "federate-heartbeat-inactive.ok"),
                "--federate-heartbeat-resumed-marker",
                str(directory / "federate-heartbeat-resumed.ok"),
                "--timeout-ms",
                "10000",
                "--results",
                str(model_results),
                "--junit",
                str(model_junit),
            ]
            environment = os.environ.copy()
            runtime_directory = str(arguments.process_fixture.resolve().parent)
            environment["PATH"] = runtime_directory + os.pathsep + environment.get(
                "PATH", ""
            )
            subprocess.run(
                command,
                check=True,
                timeout=arguments.timeout_s,
                env=environment,
            )
            expected_markers = (
                (
                    ("federate-heartbeat-inactive.ok", "inactive"),
                    ("federate-heartbeat-resumed.ok", "resumed"),
                )
                if callback_model == "evoked"
                else ()
            )
            for marker_name, expected in expected_markers:
                marker = directory / marker_name
                if (
                    not marker.is_file()
                    or marker.read_text(encoding="utf-8").strip() != expected
                ):
                    raise RuntimeError(
                        f"The heartbeat-recovery fixture did not publish {expected!r}"
                    )
            (directory / "federate-heartbeat-recovery-complete.ok").write_text(
                "complete\n", encoding="utf-8"
            )
            if fixture.wait(timeout=arguments.timeout_s) != 0:
                raise RuntimeError("The heartbeat-recovery process fixture failed")
            return (
                json.loads(model_results.read_text(encoding="utf-8")),
                ET.parse(model_junit).getroot(),
            )
        finally:
            if fixture is not None and fixture.poll() is None:
                fixture.terminate()
                try:
                    fixture.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    fixture.kill()
                    fixture.wait()


def run() -> int:
    arguments = parse_arguments()
    arguments.results.parent.mkdir(parents=True, exist_ok=True)
    arguments.junit.parent.mkdir(parents=True, exist_ok=True)
    callback_models = (
        ("evoked", "immediate")
        if arguments.callback_model == "both"
        else (arguments.callback_model,)
    )

    combined_results: dict[str, object] | None = None
    junit_cases: list[ET.Element] = []
    for callback_model in callback_models:
        model_results, model_junit = run_one(arguments, callback_model)
        if combined_results is None:
            combined_results = copy.deepcopy(model_results)
            combined_results["results"] = []
        elif any(
            model_results.get(key) != combined_results.get(key)
            for key in combined_results
            if key != "results"
        ):
            raise RuntimeError("TCK callback-model result metadata did not match")
        combined_results["results"].extend(model_results["results"])
        junit_cases.extend(copy.deepcopy(list(model_junit)))

    assert combined_results is not None
    arguments.results.write_text(
        json.dumps(combined_results, indent=2) + "\n",
        encoding="utf-8",
    )
    junit_suite = ET.Element(
        "testsuite",
        {
            "name": "hla-rti-cpp-tck",
            "tests": str(len(junit_cases)),
            "failures": "0",
        },
    )
    for testcase in junit_cases:
        junit_suite.append(testcase)
    ET.indent(junit_suite, space="  ")
    arguments.junit.write_text(
        ET.tostring(junit_suite, encoding="unicode") + "\n",
        encoding="utf-8",
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(run())
