#!/usr/bin/env python3
"""Run the provider-specific reconnect-timeout case against a process host."""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
import tempfile
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / "tools"))
import run_java_tck

SCENARIO = "java-tck.federate-reconnect-timeout"
ADDITIONAL_SETTINGS = "FED_INT_HEART=0.02;FED_TIMEOUT_RECONNECT=0.25"


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--api-jar", type=Path, required=True)
    parser.add_argument("--provider-jar", type=Path, action="append", required=True)
    parser.add_argument("--dependency-jar", type=Path, action="append", default=[])
    parser.add_argument("--fom-path", type=Path, required=True)
    parser.add_argument("--mim-path", type=Path, required=True)
    parser.add_argument("--process-fixture", type=Path, required=True)
    parser.add_argument("--classes-directory", type=Path, required=True)
    parser.add_argument("--results-directory", type=Path, required=True)
    parser.add_argument("--factory-name", default="")
    parser.add_argument("--api-package-prefix", default="hla.rti1516_2025")
    parser.add_argument("--time-implementation", default="HLAinteger64Time")
    parser.add_argument("--provider-id", default="java-current-process")
    parser.add_argument("--provider-jvm-argument", action="append", default=[])
    parser.add_argument("--callback-model", choices=("both", "evoked", "immediate"), default="both")
    parser.add_argument("--java", default="java")
    parser.add_argument("--javac", default="javac")
    parser.add_argument("--skip-build", action="store_true")
    parser.add_argument("--timeout-s", type=float, default=30.0)
    return parser.parse_args()


def wait_for_port(
    directory: Path,
    fixture: subprocess.Popen[bytes],
    timeout_seconds: float,
) -> int:
    port_path = directory / "port.txt"
    deadline = time.monotonic() + timeout_seconds
    while time.monotonic() < deadline:
        try:
            port = int(port_path.read_text(encoding="utf-8").strip())
        except (OSError, ValueError):
            port = 0
        if 0 < port <= 65535:
            return port
        if fixture.poll() is not None:
            raise RuntimeError(
                "reconnect-timeout fixture exited before publishing its port"
            )
        time.sleep(0.025)
    raise TimeoutError("reconnect-timeout fixture did not publish a listening port")


def wait_for_fixture(
    fixture: subprocess.Popen[bytes], timeout_seconds: float
) -> int:
    try:
        return int(fixture.wait(timeout=timeout_seconds))
    except subprocess.TimeoutExpired as error:
        fixture.terminate()
        try:
            fixture.wait(timeout=5)
        except subprocess.TimeoutExpired:
            fixture.kill()
            fixture.wait()
        raise TimeoutError("reconnect-timeout fixture did not terminate") from error


def run_model(arguments: argparse.Namespace, model: str) -> int:
    with tempfile.TemporaryDirectory(
        prefix=f"java-tck-federate-reconnect-{model}-",
        dir=arguments.results_directory,
    ) as temporary_directory:
        directory = Path(temporary_directory)
        ready_marker = directory / "federate-reconnect-outage-ready.ok"
        callback_marker = directory / "federate-reconnect-outage-callback.ok"
        server_stopped_marker = directory / "federate-reconnect-server-stopped.ok"
        completion_marker = directory / "federate-reconnect-complete.ok"
        results_path = arguments.results_directory / f"{model}.results.json"
        junit_path = arguments.results_directory / f"{model}.junit.xml"
        evidence_path = arguments.results_directory / f"{model}.fault-evidence.json"
        fixture: subprocess.Popen[bytes] | None = None
        try:
            fixture = subprocess.Popen(
                [
                    str(arguments.process_fixture),
                    "public-server-federate-reconnect-timeout",
                    str(directory),
                ]
            )
            port = wait_for_port(directory, fixture, arguments.timeout_s)
            jvm_arguments = [
                value.replace("{port}", str(port))
                for value in arguments.provider_jvm_argument
            ]
            jvm_arguments.extend(
                [
                    "-Dhla.rti.tck.rtiAddress=tcp://127.0.0.1:" + str(port),
                    "-Dhla.rti.tck.configurationName=federate-reconnect-timeout",
                    "-Dhla.rti.tck.callbackModel="
                    + ("HLA_EVOKED" if model == "evoked" else "HLA_IMMEDIATE"),
                    "-Dhla.rti.tck.additionalSettings=" + ADDITIONAL_SETTINGS,
                    "-Dhla.rti.tck.federateReconnectTimeoutReadyMarker="
                    + str(ready_marker),
                    "-Dhla.rti.tck.federateReconnectTimeoutServerStoppedMarker="
                    + str(server_stopped_marker),
                    "-Dhla.rti.tck.federateReconnectTimeoutCallbackMarker="
                    + str(callback_marker),
                    "-Dhla.rti.tck.federateReconnectTimeoutMillis=5000",
                ]
            )
            result = run_java_tck.run_tck(
                api_jar=arguments.api_jar,
                provider_jars=arguments.provider_jar,
                dependency_jars=arguments.dependency_jar,
                fom_path=arguments.fom_path,
                mim_path=arguments.mim_path,
                classes_directory=arguments.classes_directory,
                java=arguments.java,
                javac=arguments.javac,
                factory_name=arguments.factory_name,
                api_package_prefix=arguments.api_package_prefix,
                time_implementation=arguments.time_implementation,
                scenario=SCENARIO,
                run_unverified=True,
                results_path=results_path,
                junit_path=junit_path,
                provider_id=arguments.provider_id,
                jvm_arguments=jvm_arguments,
                skip_build=True,
            )
            if result != 0:
                return int(result)

            result_data = json.loads(results_path.read_text(encoding="utf-8"))
            selected = [
                item
                for item in result_data.get("scenarios", [])
                if item.get("id") == SCENARIO
            ]
            if len(selected) != 1 or selected[0].get("status") != "pass":
                raise RuntimeError("Java TCK did not record a passing reconnect-timeout case")
            if not callback_marker.is_file() or callback_marker.read_text(
                encoding="utf-8"
            ).strip() != "callback-ok":
                raise RuntimeError(
                    "FED_TIMEOUT_RECONNECT did not produce the expected connectionLost callback"
                )
            if not server_stopped_marker.is_file() or server_stopped_marker.read_text(
                encoding="utf-8"
            ).strip() != "stopped":
                raise RuntimeError("the process fixture did not confirm the server outage")

            completion_marker.write_text("complete\n", encoding="utf-8")
            fixture_status = wait_for_fixture(fixture, arguments.timeout_s)
            fixture = None
            if fixture_status != 0:
                raise RuntimeError("reconnect-timeout process fixture failed")
            evidence = {
                "outage_ready_marker_written": ready_marker.is_file(),
                "fixture_confirmed_server_stopped": server_stopped_marker.is_file(),
                "connection_lost_callback_marker": callback_marker.read_text(
                    encoding="utf-8"
                ).strip(),
                "completion_marker_written": completion_marker.is_file(),
                "additional_settings": ADDITIONAL_SETTINGS,
                "fixture_exited_successfully": fixture_status == 0,
            }
            evidence_path.write_text(
                json.dumps(evidence, indent=2) + "\n", encoding="utf-8"
            )
            return 0
        finally:
            if fixture is not None and fixture.poll() is None:
                fixture.terminate()
                try:
                    fixture.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    fixture.kill()
                    fixture.wait()


def run(arguments: argparse.Namespace) -> int:
    for name in (
        "api_jar",
        "fom_path",
        "mim_path",
        "process_fixture",
        "classes_directory",
        "results_directory",
    ):
        setattr(arguments, name, getattr(arguments, name).resolve())
    arguments.provider_jar = [path.resolve() for path in arguments.provider_jar]
    arguments.dependency_jar = [path.resolve() for path in arguments.dependency_jar]
    required_files = [
        (arguments.api_jar, "official Java API JAR"),
        (arguments.fom_path, "FOM module"),
        (arguments.mim_path, "MIM module"),
        (arguments.process_fixture, "process fixture"),
        *[(path, "provider JAR") for path in arguments.provider_jar],
        *[(path, "provider dependency JAR") for path in arguments.dependency_jar],
    ]
    for path, description in required_files:
        if not path.is_file():
            raise FileNotFoundError(f"{description} does not exist: {path}")
    if arguments.timeout_s <= 0:
        raise ValueError("timeout must be positive")

    arguments.results_directory.mkdir(parents=True, exist_ok=True)
    if not arguments.skip_build:
        result = run_java_tck.build_tck(
            api_jar=arguments.api_jar,
            classes_directory=arguments.classes_directory,
            javac=arguments.javac,
            api_package_prefix=arguments.api_package_prefix,
        )
        if result != 0:
            return int(result)
    if not arguments.classes_directory.is_dir():
        raise FileNotFoundError(
            f"compiled Java TCK classes do not exist: {arguments.classes_directory}"
        )

    models = (
        ("evoked", "immediate")
        if arguments.callback_model == "both"
        else (arguments.callback_model,)
    )
    for model in models:
        result = run_model(arguments, model)
        if result != 0:
            return result
    return 0


def main() -> int:
    try:
        return run(parse_arguments())
    except (OSError, RuntimeError, TimeoutError, ValueError) as error:
        print(f"Java reconnect-timeout adapter failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
