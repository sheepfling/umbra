#!/usr/bin/env python3
"""Run the Java missing-RTI-message timeout case against the process fixture."""

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

SCENARIO = "java-tck.federate-missing-rti-message-timeout-recovers"
ADDITIONAL_SETTINGS = (
    "FED_INT_HEART=0.05;FED_TIMEOUT_HEART=0.4;FED_TIMEOUT_RECONNECT=4"
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
            error_path = directory / (
                "public-server-federate-missing-rti-message-timeout.error"
            )
            detail = (
                error_path.read_text(encoding="utf-8", errors="replace").strip()
                if error_path.is_file()
                else ""
            )
            suffix = f": {detail}" if detail else ""
            raise RuntimeError(
                "missing-RTI-message timeout fixture exited before publishing its port"
                + suffix
            )
        time.sleep(0.025)
    raise TimeoutError("missing-RTI-message timeout fixture did not publish a listening port")


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
        raise TimeoutError("missing-RTI-message timeout fixture did not terminate") from error


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
    with tempfile.TemporaryDirectory(
        prefix="java-tck-missing-rti-message-timeout-",
        dir=arguments.results_directory,
    ) as temporary_directory:
        directory = Path(temporary_directory)
        ready = directory / "federate-missing-rti-message-timeout-ready.ok"
        suppressed = directory / "federate-missing-rti-message-timeout-reply-suppressed.ok"
        resumed = directory / "federate-missing-rti-message-timeout-resumed.ok"
        complete = directory / "federate-missing-rti-message-timeout-complete.ok"
        result_path = arguments.results_directory / "evoked.results.json"
        junit_path = arguments.results_directory / "evoked.junit.xml"
        evidence_path = arguments.results_directory / "evoked.fault-evidence.json"
        fixture: subprocess.Popen[bytes] | None = None
        try:
            fixture = subprocess.Popen(
                [
                    str(arguments.process_fixture),
                    "public-server-federate-missing-rti-message-timeout",
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
                    "-Dhla.rti.tck.callbackModel=HLA_EVOKED",
                    "-Dhla.rti.tck.additionalSettings=" + ADDITIONAL_SETTINGS,
                    "-Dhla.rti.tck.federateMissingRtiMessageTimeoutReadyMarker=" + str(ready),
                    "-Dhla.rti.tck.federateMissingRtiMessageTimeoutSuppressedMarker="
                    + str(suppressed),
                    "-Dhla.rti.tck.federateMissingRtiMessageTimeoutResumedMarker=" + str(resumed),
                    "-Dhla.rti.tck.federateMissingRtiMessageTimeoutMillis="
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
                results_path=result_path,
                junit_path=junit_path,
                provider_id=arguments.provider_id,
                jvm_arguments=jvm_arguments,
                skip_build=arguments.skip_build,
            )
            if result != 0:
                return int(result)

            result_data = json.loads(result_path.read_text(encoding="utf-8"))
            selected = [
                item for item in result_data.get("scenarios", [])
                if item.get("id") == SCENARIO
            ]
            if len(selected) != 1 or selected[0].get("status") != "pass":
                raise RuntimeError("Java TCK did not record a passing missing-message timeout case")
            for marker, expected, label in (
                (ready, "ready", "readiness"),
                (suppressed, "suppressed", "heartbeat-suppression"),
                (resumed, "resumed", "session-resumption"),
            ):
                if not marker.is_file() or marker.read_text(
                    encoding="utf-8"
                ).strip() != expected:
                    raise RuntimeError(f"fixture did not confirm {label}")

            complete.write_text("complete\n", encoding="utf-8")
            fixture_status = wait_for_fixture(fixture, arguments.timeout_s)
            fixture = None
            if fixture_status != 0:
                raise RuntimeError("missing-RTI-message timeout process fixture failed")
            evidence = {
                "readiness_marker": ready.read_text(encoding="utf-8").strip(),
                "heartbeat_reply_suppressed_marker": suppressed.read_text(
                    encoding="utf-8"
                ).strip(),
                "session_resumed_marker": resumed.read_text(encoding="utf-8").strip(),
                "completion_marker_written": complete.is_file(),
                "additional_settings": ADDITIONAL_SETTINGS,
                "callback_model": "HLA_EVOKED",
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


if __name__ == "__main__":
    try:
        raise SystemExit(run(parse_arguments()))
    except Exception as error:
        print(f"Java missing-RTI-message timeout adapter failed: {error}", file=sys.stderr)
        raise SystemExit(2) from error
