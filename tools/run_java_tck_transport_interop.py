"""Run the Java public transport TCK against the literal Table 33 peer."""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
from pathlib import Path

from run_cpp_tck_transport_interop import IndependentLoopbackPeer


ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "tools" / "run_java_tck.py"
SCENARIO = "java-tck.transport-public-new-session-interoperability"
CALLBACK_MODELS = ("HLA_EVOKED", "HLA_IMMEDIATE")


def _root_path(value: str) -> Path:
    path = Path(value)
    return path if path.is_absolute() else ROOT / path


def _build_command(arguments: argparse.Namespace, classes_directory: Path) -> list[str]:
    command = [
        sys.executable,
        str(RUNNER),
        "build",
        "--api-jar",
        str(_root_path(arguments.api_jar)),
        "--output-directory",
        str(classes_directory),
    ]
    if arguments.javac:
        command.extend(["--javac", arguments.javac])
    return command


def _run_command(
    arguments: argparse.Namespace,
    callback_model: str,
    peer_address: str,
    classes_directory: Path,
    results_directory: Path,
) -> list[str]:
    command = [
        sys.executable,
        str(RUNNER),
        "run",
        "--api-jar",
        str(_root_path(arguments.api_jar)),
        "--fom-path",
        str(_root_path(arguments.fom_path)),
        "--time-implementation",
        arguments.time_implementation,
        "--provider-id",
        arguments.provider_id,
        "--scenario",
        SCENARIO,
        "--run-unverified",
        "--classes-directory",
        str(classes_directory),
        "--results-path",
        str(results_directory / f"{callback_model.lower()}.results.json"),
        "--junit-path",
        str(results_directory / f"{callback_model.lower()}.junit.xml"),
        "--skip-build",
    ]
    for provider_jar in arguments.provider_jar:
        command.extend(["--provider-jar", str(_root_path(provider_jar))])
    for dependency_jar in arguments.dependency_jar:
        command.extend(["--dependency-jar", str(_root_path(dependency_jar))])
    if arguments.capability_profile:
        command.extend(
            ["--capability-profile", str(_root_path(arguments.capability_profile))]
        )
    if arguments.java:
        command.extend(["--java", arguments.java])
    for jvm_argument in arguments.jvm_argument:
        command.append(f"--jvm-argument={jvm_argument}")
    if arguments.jni_library_path:
        command.append(
            "--jvm-argument=-Djava.library.path="
            + str(_root_path(arguments.jni_library_path))
        )
    command.extend(
        [
            "--jvm-argument=-Dhla.rti.tck.callbackModel=" + callback_model,
            "--jvm-argument=-Dhla.rti.tck.transportPeerAddress=" + peer_address,
        ]
    )
    return command


def main() -> int:
    parser = argparse.ArgumentParser(
        description=(
            "Build and run the focused Java public-API case against a byte-independent "
            "Table 33 peer under both callback models."
        )
    )
    parser.add_argument("--api-jar", required=True, help="pinned official Java API JAR")
    parser.add_argument(
        "--provider-jar", action="append", required=True, help="provider JAR; repeat as needed"
    )
    parser.add_argument(
        "--dependency-jar", action="append", default=[], help="provider dependency JAR"
    )
    parser.add_argument("--fom-path", required=True, help="adapter-supplied FOM module")
    parser.add_argument("--provider-id", required=True, help="provider identity for result records")
    parser.add_argument(
        "--time-implementation", default="HLAinteger64Time", help="adapter logical-time name"
    )
    parser.add_argument("--capability-profile", help="optional Java TCK capability profile")
    parser.add_argument(
        "--output-directory", required=True, help="directory for classes, results, and peer logs"
    )
    parser.add_argument("--jni-library-path", help="optional java.library.path for the provider")
    parser.add_argument(
        "--prepend-path", action="append", default=[], help="directory to prepend to PATH"
    )
    parser.add_argument("--jvm-argument", action="append", default=[])
    parser.add_argument("--java", help="Java executable")
    parser.add_argument("--javac", help="Java compiler executable")
    arguments = parser.parse_args()

    output_directory = _root_path(arguments.output_directory)
    classes_directory = output_directory / "classes"
    results_directory = output_directory / "results"
    peer_directory = output_directory / "peer"
    results_directory.mkdir(parents=True, exist_ok=True)
    peer_directory.mkdir(parents=True, exist_ok=True)

    build = subprocess.run(
        _build_command(arguments, classes_directory), cwd=ROOT, check=False, timeout=120
    )
    if build.returncode != 0:
        return build.returncode

    environment = os.environ.copy()
    if arguments.prepend_path:
        prepend = [str(_root_path(value)) for value in arguments.prepend_path]
        environment["PATH"] = os.pathsep.join([*prepend, environment.get("PATH", "")])

    result_code = 0
    for callback_model in CALLBACK_MODELS:
        peer = IndependentLoopbackPeer(1)
        command = _run_command(
            arguments,
            callback_model,
            peer.address,
            classes_directory,
            results_directory,
        )
        peer_failure: str | None = None
        try:
            runner = subprocess.run(
                command, cwd=ROOT, env=environment, check=False, timeout=90
            )
            if runner.returncode != 0:
                result_code = result_code or runner.returncode
        except subprocess.TimeoutExpired:
            peer_failure = "Java TCK runner exceeded the 90 second bound"
            result_code = result_code or 124
        try:
            peer.finish()
        except RuntimeError as error:
            peer_failure = peer_failure or str(error)
            result_code = result_code or 1

        peer_result = {
            "scenario": SCENARIO,
            "callback_model": callback_model,
            "address": peer.address,
            "expected_connections": peer.expected_connections,
            "connections": peer.connections,
            "failure": peer_failure,
        }
        (peer_directory / f"{callback_model.lower()}.json").write_text(
            json.dumps(peer_result, indent=2) + "\n", encoding="utf-8"
        )
        if peer_failure:
            print(f"{callback_model}: {peer_failure}", file=sys.stderr)

    return result_code


if __name__ == "__main__":
    raise SystemExit(main())
