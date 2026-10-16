#!/usr/bin/env python3
"""Run pure Java connection-loss scenarios through an adapter fixture.

The fixture owns provider startup, endpoint allocation, and the transport fault.
The Java TCK receives only provider/API JARs, FOM/MIM, callback configuration,
and standard HLA scenario inputs. No provider classes are imported by the TCK.
"""

from __future__ import annotations

import argparse
import subprocess
import sys
import tempfile
import time
from pathlib import Path


ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / "tools"))
import run_java_tck  # noqa: E402


SCENARIOS = (
    "java-tck.connection-loss-cleanup",
    "java-tck.federate-lost-mom-report",
    "java-tck.automatic-resign-directive-delete-objects",
    "java-tck.connection-loss-automatic-unconditional-divestiture",
    "java-tck.connection-loss-automatic-delete-then-divest",
    "java-tck.connection-loss-automatic-cancel-pending-acquisition",
)


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--api-jar", type=Path, required=True)
    parser.add_argument("--provider-jar", type=Path, action="append", required=True)
    parser.add_argument("--dependency-jar", type=Path, action="append", default=[])
    parser.add_argument("--fom-path", type=Path, required=True)
    parser.add_argument("--mim-path", type=Path)
    parser.add_argument("--scenario", choices=SCENARIOS, default=SCENARIOS[0])
    parser.add_argument("--process-fixture", type=Path, required=True)
    parser.add_argument("--factory-name", default="")
    parser.add_argument("--api-package-prefix", default="hla.rti1516_2025")
    parser.add_argument("--time-implementation", default="HLAinteger64Time")
    parser.add_argument("--interaction-class")
    parser.add_argument("--provider-id", default="java-current-process")
    parser.add_argument("--provider-jvm-argument", action="append", default=[])
    parser.add_argument("--capability-profile", type=Path, required=True)
    parser.add_argument(
        "--callback-model", choices=("both", "evoked", "immediate"), default="both"
    )
    parser.add_argument("--classes-directory", type=Path,
                        default=ROOT / "out" / "java-tck" / "classes")
    parser.add_argument("--results-directory", type=Path, required=True)
    parser.add_argument("--java", default="java")
    parser.add_argument("--javac", default="javac")
    parser.add_argument("--skip-build", action="store_true")
    parser.add_argument("--timeout-ms", type=int, default=5000)
    parser.add_argument("--startup-timeout-s", type=float, default=30.0)
    parser.add_argument("--fixture-timeout-s", type=float, default=30.0)
    return parser.parse_args()


def wait_for_port(directory: Path, timeout_seconds: float) -> int:
    port_file = directory / "port.txt"
    deadline = time.monotonic() + timeout_seconds
    while time.monotonic() < deadline:
        try:
            port = int(port_file.read_text(encoding="utf-8").strip())
        except (OSError, ValueError):
            port = 0
        if 0 < port <= 65535:
            return port
        time.sleep(0.1)
    raise TimeoutError("connection-loss fixture did not publish a listening port")


def wait_for_fixture(process: subprocess.Popen[bytes], timeout_seconds: float) -> int:
    try:
        return int(process.wait(timeout=timeout_seconds))
    except subprocess.TimeoutExpired as error:
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()
        raise TimeoutError("connection-loss fixture did not terminate") from error


def run_model(arguments: argparse.Namespace, model: str, root: Path) -> int:
    markers = root / model
    markers.mkdir()
    (markers / (model + ".mode")).write_text("enabled\n", encoding="utf-8")
    ready = markers / "connection-loss-ready.ok"
    complete = markers / "receiver-loss.ok"
    if arguments.scenario == "java-tck.connection-loss-automatic-delete-then-divest":
        (markers / "automatic-delete-then-divest.mode").write_text(
            "enabled\n", encoding="utf-8"
        )
    result_file = arguments.results_directory / (model + ".results.json")
    junit_file = arguments.results_directory / (model + ".junit.xml")

    fixture = subprocess.Popen(
        [str(arguments.process_fixture), "public-server-loss", str(markers)]
    )
    try:
        port = wait_for_port(markers, arguments.startup_timeout_s)
        jvm_arguments = [
            value.replace("{port}", str(port)) for value in arguments.provider_jvm_argument
        ]
        jvm_arguments.extend([
            "-Dhla.rti.tck.callbackModel="
            + ("HLA_EVOKED" if model == "evoked" else "HLA_IMMEDIATE"),
            "-Dhla.rti.tck.connectionLossReadyMarker=" + str(ready),
            "-Dhla.rti.tck.connectionLossCompletionMarker=" + str(complete),
            "-Dhla.rti.tck.connectionLossTimeoutMillis=" + str(arguments.timeout_ms),
        ])
        if arguments.interaction_class:
            jvm_arguments.append(
                "-Dhla.rti.tck.interactionClass=" + arguments.interaction_class
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
            capability_profile=arguments.capability_profile,
            scenario=arguments.scenario,
            results_path=result_file,
            junit_path=junit_file,
            provider_id=arguments.provider_id,
            jvm_arguments=jvm_arguments,
            skip_build=True,
        )
        if result != 0:
            fixture_error = markers / "public-server-loss.error"
            if fixture_error.is_file():
                print("connection-loss fixture: "
                      + fixture_error.read_text(encoding="utf-8").strip(), file=sys.stderr)
            return int(result)
        if not complete.is_file():
            raise RuntimeError("Java TCK did not write its connection-loss completion marker")
        fixture_status = wait_for_fixture(fixture, arguments.fixture_timeout_s)
        fixture = None
        if fixture_status != 0:
            raise RuntimeError(f"connection-loss fixture exited with status {fixture_status}")
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
    arguments.api_jar = arguments.api_jar.resolve()
    arguments.fom_path = arguments.fom_path.resolve()
    if arguments.mim_path is not None:
        arguments.mim_path = arguments.mim_path.resolve()
    arguments.process_fixture = arguments.process_fixture.resolve()
    arguments.capability_profile = arguments.capability_profile.resolve()
    arguments.provider_jar = [path.resolve() for path in arguments.provider_jar]
    arguments.dependency_jar = [path.resolve() for path in arguments.dependency_jar]
    arguments.classes_directory = arguments.classes_directory.resolve()
    arguments.results_directory = arguments.results_directory.resolve()
    for path, description in (
        (arguments.api_jar, "official Java API JAR"),
        (arguments.fom_path, "FOM module"),
        (arguments.process_fixture, "adapter process fixture"),
        (arguments.capability_profile, "adapter capability profile"),
        *[(item, "provider JAR") for item in arguments.provider_jar],
        *[(item, "provider dependency JAR") for item in arguments.dependency_jar],
    ):
        if not path.is_file():
            raise FileNotFoundError(f"{description} does not exist: {path}")
    if arguments.scenario == "java-tck.federate-lost-mom-report":
        if arguments.mim_path is None or not arguments.mim_path.is_file():
            raise FileNotFoundError(
                "--mim-path is required and must name the adapter-supplied MIM module"
            )
    elif not arguments.interaction_class:
        raise ValueError("--interaction-class is required for this connection-loss scenario")
    if arguments.timeout_ms <= 0:
        raise ValueError("--timeout-ms must be positive")
    if not any("{port}" in value for value in arguments.provider_jvm_argument):
        raise ValueError(
            "at least one --provider-jvm-argument must contain {port} so the adapter endpoint is used"
        )

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

    models = ("evoked", "immediate") if arguments.callback_model == "both" else (
        arguments.callback_model,
    )
    with tempfile.TemporaryDirectory(prefix="java-tck-connection-loss-") as temporary:
        root = Path(temporary)
        for model in models:
            result = run_model(arguments, model, root)
            if result != 0:
                return result
    return 0


def main() -> int:
    try:
        return run(parse_arguments())
    except (OSError, RuntimeError, TimeoutError, ValueError) as error:
        print(f"Java connection-loss adapter failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
