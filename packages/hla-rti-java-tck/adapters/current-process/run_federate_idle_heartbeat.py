#!/usr/bin/env python3
"""Run the Java idle-heartbeat TCK case against the process fixture."""

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

SCENARIO = "java-tck.federate-idle-heartbeat-preserves-session"
ADDITIONAL_SETTINGS = (
    "FED_INT_HEART=0.1;FED_TIMEOUT_HEART=2;FED_TIMEOUT_RECONNECT=0.25"
)


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--api-jar", type=Path, required=True)
    parser.add_argument("--provider-jar", type=Path, action="append", required=True)
    parser.add_argument("--dependency-jar", type=Path, action="append", default=[])
    parser.add_argument("--fom-path", type=Path, required=True)
    parser.add_argument("--mim-path", type=Path)
    parser.add_argument("--process-fixture", type=Path, required=True)
    parser.add_argument("--classes-directory", type=Path, required=True)
    parser.add_argument("--results-directory", type=Path, required=True)
    parser.add_argument("--factory-name", default="")
    parser.add_argument("--api-package-prefix", default="hla.rti1516_2025")
    parser.add_argument("--time-implementation", default="HLAinteger64Time")
    parser.add_argument("--provider-id", default="java-current-process")
    parser.add_argument("--provider-jvm-argument", action="append", default=[])
    parser.add_argument(
        "--callback-model", choices=("both", "evoked", "immediate"), default="both"
    )
    parser.add_argument("--java", default="java")
    parser.add_argument("--javac", default="javac")
    parser.add_argument("--skip-build", action="store_true")
    parser.add_argument("--timeout-ms", type=int, default=5000)
    parser.add_argument("--timeout-s", type=float, default=30.0)
    return parser.parse_args()


def wait_for_port(
    directory: Path, fixture: subprocess.Popen[bytes], timeout_seconds: float
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
            error_path = directory / "public-server-federate-idle-heartbeat.error"
            detail = (
                error_path.read_text(encoding="utf-8", errors="replace").strip()
                if error_path.is_file()
                else ""
            )
            suffix = f": {detail}" if detail else ""
            raise RuntimeError(
                "idle-heartbeat fixture exited before publishing its port" + suffix
            )
        time.sleep(0.025)
    raise TimeoutError("idle-heartbeat fixture did not publish a listening port")


def wait_for_fixture(fixture: subprocess.Popen[bytes], timeout_seconds: float) -> int:
    try:
        return int(fixture.wait(timeout=timeout_seconds))
    except subprocess.TimeoutExpired as error:
        fixture.terminate()
        try:
            fixture.wait(timeout=5)
        except subprocess.TimeoutExpired:
            fixture.kill()
            fixture.wait()
        raise TimeoutError("idle-heartbeat fixture did not terminate") from error


def run_model(arguments: argparse.Namespace, model: str, skip_build: bool) -> int:
    with tempfile.TemporaryDirectory(
        prefix=f"java-tck-idle-heartbeat-{model}-",
        dir=arguments.results_directory,
    ) as temporary_directory:
        directory = Path(temporary_directory)
        ready = directory / "federate-idle-heartbeat-ready.ok"
        observed = directory / "federate-idle-heartbeat-observed.ok"
        inactive = directory / "federate-idle-heartbeat-inactive.ok"
        complete = directory / "federate-idle-heartbeat-complete.ok"
        results_path = arguments.results_directory / f"{model}.results.json"
        junit_path = arguments.results_directory / f"{model}.junit.xml"
        evidence_path = arguments.results_directory / f"{model}.fault-evidence.json"
        fixture: subprocess.Popen[bytes] | None = None
        try:
            fixture = subprocess.Popen(
                [
                    str(arguments.process_fixture),
                    "public-server-federate-idle-heartbeat",
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
                    "-Dhla.rti.tck.callbackModel="
                    + ("HLA_EVOKED" if model == "evoked" else "HLA_IMMEDIATE"),
                    "-Dhla.rti.tck.additionalSettings=" + ADDITIONAL_SETTINGS,
                    "-Dhla.rti.tck.federateIdleHeartbeatReadyMarker=" + str(ready),
                    "-Dhla.rti.tck.federateIdleHeartbeatObservedMarker=" + str(observed),
                    "-Dhla.rti.tck.federateIdleHeartbeatInactiveMarker=" + str(inactive),
                    "-Dhla.rti.tck.federateIdleHeartbeatTimeoutMillis="
                    + str(arguments.timeout_ms),
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
                skip_build=skip_build,
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
                raise RuntimeError("Java TCK did not record a passing idle-heartbeat case")
            if not ready.is_file() or ready.read_text(
                encoding="utf-8"
            ).strip() != "ready":
                raise RuntimeError("fixture did not confirm idle-heartbeat readiness")
            if model == "evoked" and (
                not observed.is_file()
                or observed.read_text(encoding="utf-8").strip() != "heartbeat"
            ):
                raise RuntimeError("fixture did not observe the idle evoked heartbeat")
            if inactive.exists():
                raise RuntimeError("fixture marked the idle federate inactive")

            complete.write_text("complete\n", encoding="utf-8")
            fixture_status = wait_for_fixture(fixture, arguments.timeout_s)
            fixture = None
            if fixture_status != 0:
                raise RuntimeError("idle-heartbeat process fixture failed")
            evidence = {
                "readiness_marker": ready.read_text(encoding="utf-8").strip(),
                "heartbeat_observed_marker": (
                    observed.read_text(encoding="utf-8").strip()
                    if observed.is_file()
                    else None
                ),
                "inactivity_marker_absent": not inactive.exists(),
                "completion_marker_written": complete.is_file(),
                "additional_settings": ADDITIONAL_SETTINGS,
                "callback_model": "HLA_EVOKED" if model == "evoked" else "HLA_IMMEDIATE",
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
        "process_fixture",
        "classes_directory",
        "results_directory",
    ):
        setattr(arguments, name, getattr(arguments, name).resolve())
    if arguments.mim_path is not None:
        arguments.mim_path = arguments.mim_path.resolve()
    arguments.provider_jar = [path.resolve() for path in arguments.provider_jar]
    arguments.dependency_jar = [path.resolve() for path in arguments.dependency_jar]
    for path, description in (
        (arguments.api_jar, "official Java API JAR"),
        (arguments.fom_path, "FOM module"),
        (arguments.process_fixture, "adapter process fixture"),
        *[(path, "provider JAR") for path in arguments.provider_jar],
        *[(path, "provider dependency JAR") for path in arguments.dependency_jar],
    ):
        if not path.is_file():
            raise FileNotFoundError(f"{description} does not exist: {path}")
    if arguments.mim_path is not None and not arguments.mim_path.is_file():
        raise FileNotFoundError(f"MIM module does not exist: {arguments.mim_path}")
    if arguments.timeout_ms <= 0 or arguments.timeout_s <= 0:
        raise ValueError("timeouts must be positive")

    arguments.results_directory.mkdir(parents=True, exist_ok=True)
    models = ("evoked", "immediate") if arguments.callback_model == "both" else (
        arguments.callback_model,
    )
    skip_build = arguments.skip_build
    for model in models:
        result = run_model(arguments, model, skip_build)
        if result != 0:
            return result
        skip_build = True
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(run(parse_arguments()))
    except Exception as error:
        print(f"Java idle-heartbeat adapter failed: {error}", file=sys.stderr)
        raise SystemExit(2) from error
