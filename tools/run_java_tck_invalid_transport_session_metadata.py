"""Run the Java Connect rejection case against the malformed-metadata peer."""

from __future__ import annotations

import argparse
import importlib.util
import json
import os
import subprocess
import sys
import threading
import time
from pathlib import Path
from types import ModuleType

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "tools" / "run_java_tck.py"
ADAPTER_PATH = (
    ROOT
    / "packages"
    / "hla-rti-cpp-tck"
    / "adapters"
    / "current-process"
    / "run_invalid_transport_session_metadata.py"
)
SCENARIO = "java-tck.reject-invalid-transport-session-metadata"
CALLBACK_MODELS = ("HLA_EVOKED", "HLA_IMMEDIATE")


def _load_peer() -> ModuleType:
    specification = importlib.util.spec_from_file_location(
        "cpp_invalid_transport_session_metadata_peer", ADAPTER_PATH
    )
    if specification is None or specification.loader is None:
        raise RuntimeError(f"could not load invalid-metadata peer {ADAPTER_PATH}")
    module = importlib.util.module_from_spec(specification)
    specification.loader.exec_module(module)
    return module


PEER = _load_peer()


def _root_path(value: str | Path) -> Path:
    path = Path(value)
    return path.resolve() if path.is_absolute() else (ROOT / path).resolve()


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--api-jar", required=True, help="pinned official Java API JAR")
    parser.add_argument(
        "--provider-jar", action="append", required=True, help="provider JAR; repeat as needed"
    )
    parser.add_argument(
        "--dependency-jar", action="append", default=[], help="provider dependency JAR"
    )
    parser.add_argument("--fom-path", required=True, help="TCK FOM argument required by the runner")
    parser.add_argument("--provider-id", required=True, help="provider identity for results")
    parser.add_argument("--time-implementation", default="HLAinteger64Time")
    parser.add_argument("--capability-profile")
    parser.add_argument("--output-directory", required=True)
    parser.add_argument("--classes-directory")
    parser.add_argument("--java")
    parser.add_argument("--javac")
    parser.add_argument("--jni-library-path")
    parser.add_argument("--prepend-path", action="append", default=[])
    parser.add_argument("--jvm-argument", action="append", default=[])
    parser.add_argument("--timeout-s", type=float, default=60.0)
    parser.add_argument("--peer-timeout-s", type=float, default=10.0)
    return parser.parse_args()


def _build_tck(arguments: argparse.Namespace, classes_directory: Path) -> None:
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
    completed = subprocess.run(command, cwd=ROOT, check=False, timeout=180)
    if completed.returncode != 0:
        raise RuntimeError(f"Java TCK compilation failed with exit {completed.returncode}")


def _run_command(
    arguments: argparse.Namespace,
    callback_model: str,
    rti_address: str,
    classes_directory: Path,
    model_directory: Path,
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
        str(model_directory / "results.json"),
        "--junit-path",
        str(model_directory / "results.xml"),
        "--skip-build",
    ]
    for provider_jar in arguments.provider_jar:
        command.extend(["--provider-jar", str(_root_path(provider_jar))])
    for dependency_jar in arguments.dependency_jar:
        command.extend(["--dependency-jar", str(_root_path(dependency_jar))])
    if arguments.capability_profile:
        command.extend(["--capability-profile", str(_root_path(arguments.capability_profile))])
    if arguments.java:
        command.extend(["--java", arguments.java])
    if arguments.jni_library_path:
        command.append(
            "--jvm-argument=-Djava.library.path="
            + str(_root_path(arguments.jni_library_path))
        )
    for jvm_argument in arguments.jvm_argument:
        command.append("--jvm-argument=" + jvm_argument)
    command.extend(
        [
            "--jvm-argument=-Dhla.rti.tck.callbackModel=" + callback_model,
            "--jvm-argument=-Dhla.rti.tck.rtiAddress=" + rti_address,
        ]
    )
    return command


def _run_model(
    arguments: argparse.Namespace,
    callback_model: str,
    classes_directory: Path,
    environment: dict[str, str],
) -> None:
    model_directory = _root_path(arguments.output_directory) / callback_model.lower()
    model_directory.mkdir(parents=True, exist_ok=True)
    for generated_name in ("results.json", "results.xml", "peer.json"):
        (model_directory / generated_name).unlink(missing_ok=True)

    server = PEER._InvalidMetadataServer(arguments.peer_timeout_s)
    server_thread = threading.Thread(
        target=server.serve_forever,
        name="java-tck-invalid-session-metadata-peer",
        daemon=True,
    )
    server_thread.start()
    try:
        rti_address = f"tcp://127.0.0.1:{server.server_address[1]}"
        command = _run_command(
            arguments, callback_model, rti_address, classes_directory, model_directory
        )
        try:
            completed = subprocess.run(
                command,
                cwd=ROOT,
                env=environment,
                check=False,
                timeout=arguments.timeout_s,
            )
        except subprocess.TimeoutExpired as error:
            raise TimeoutError(f"Java TCK exceeded {arguments.timeout_s:g} seconds") from error
        if completed.returncode != 0:
            raise RuntimeError(f"Java TCK exited with {completed.returncode}")

        deadline = time.monotonic() + arguments.peer_timeout_s
        while time.monotonic() < deadline and server.closed_sockets < 2:
            time.sleep(0.025)
        if server.errors:
            raise RuntimeError("invalid-metadata peer failed: " + server.errors[0])
        if server.requests != 2 or server.closed_sockets != 2:
            raise RuntimeError(
                "expected two malformed New Session Status handshakes and two client EOFs; "
                f"observed {server.requests} handshakes and {server.closed_sockets} EOFs"
            )

        (model_directory / "peer.json").write_text(
            json.dumps(
                {
                    "scenario": SCENARIO,
                    "callback_model": callback_model,
                    "rti_address": rti_address,
                    "attempts": [
                        {
                            "malformed_status": "reserved sequence number",
                            "session_id": 1,
                            "client_closed_socket": True,
                        },
                        {
                            "malformed_status": "zero Session ID",
                            "session_id": 0,
                            "client_closed_socket": True,
                        },
                    ],
                },
                indent=2,
            )
            + "\n",
            encoding="utf-8",
        )
        print(
            f"{callback_model}: two ConnectionFailed results and two client EOFs"
        )
    finally:
        server.shutdown()
        server.server_close()
        server_thread.join(timeout=5.0)


def main() -> int:
    arguments = parse_arguments()
    if arguments.timeout_s <= 0 or arguments.peer_timeout_s <= 0:
        raise ValueError("timeouts must be positive")
    output_directory = _root_path(arguments.output_directory)
    output_directory.mkdir(parents=True, exist_ok=True)
    classes_directory = (
        _root_path(arguments.classes_directory)
        if arguments.classes_directory
        else output_directory / "classes"
    )
    _build_tck(arguments, classes_directory)

    environment = os.environ.copy()
    prepend = [str(_root_path(path)) for path in arguments.prepend_path]
    environment["PATH"] = os.pathsep.join([*prepend, environment.get("PATH", "")])
    for callback_model in CALLBACK_MODELS:
        _run_model(arguments, callback_model, classes_directory, environment)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, TimeoutError, subprocess.SubprocessError) as error:
        print(f"Java invalid-session-metadata runner failed: {error}", file=sys.stderr)
        raise SystemExit(1)
