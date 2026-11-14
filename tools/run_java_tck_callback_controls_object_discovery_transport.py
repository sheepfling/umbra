"""Run the Java callback-control TCK through the current-process transport observer."""

from __future__ import annotations

import argparse
import importlib.util
import json
import os
import subprocess
import sys
import threading
import time
from collections import Counter
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
    / "run_transport_session_cardinality.py"
)
SCENARIO = "java-tck.callback-controls-object-discovery-transport-direction"
CALLBACK_MODELS = ("HLA_EVOKED", "HLA_IMMEDIATE")
HEARTBEAT_SETTINGS = (
    "FED_INT_HEART=0.1;FED_TIMEOUT_HEART=2;FED_TIMEOUT_RECONNECT=0.25"
)


def _load_cpp_adapter() -> ModuleType:
    specification = importlib.util.spec_from_file_location(
        "cpp_transport_callback_direction_adapter", ADAPTER_PATH
    )
    if specification is None or specification.loader is None:
        raise RuntimeError(f"could not load transport observer {ADAPTER_PATH}")
    module = importlib.util.module_from_spec(specification)
    specification.loader.exec_module(module)
    return module


CPP_ADAPTER = _load_cpp_adapter()


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
    parser.add_argument("--fom-path", required=True, help="adapter-supplied FOM module")
    parser.add_argument("--process-fixture", required=True, help="process service fixture")
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
    parser.add_argument("--timeout-s", type=float, default=90.0)
    parser.add_argument("--fixture-timeout-s", type=float, default=30.0)
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
    proxy_address: str,
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
            "--jvm-argument=-Dhla.rti.tck.rtiAddress=" + proxy_address,
            "--jvm-argument=-Dhla.rti.tck.additionalSettings=" + HEARTBEAT_SETTINGS,
            "--jvm-argument=-Dhla.rti.tck.objectClass=HLAobjectRoot.TckObject",
            "--jvm-argument=-Dhla.rti.tck.attribute=Value",
        ]
    )
    return command


def _frame_observations(proxy: object) -> list[dict[str, object]]:
    frames: list[dict[str, object]] = []
    for record in list(proxy.protocol_observations):
        fields = record.split()
        if len(fields) == 8 and fields[0] == "frame":
            frames.append(
                {
                    "connection": int(fields[1]),
                    "direction": fields[2],
                    "sequence": int(fields[3]),
                    "last_received": int(fields[4]),
                    "type": int(fields[5]),
                    "session": fields[6],
                    "response_to": int(fields[7]),
                }
            )
    return frames


def _validate_protocol(proxy: object, expected_connections: int = 2) -> dict[str, object]:
    deadline = time.monotonic() + 5.0
    frames: list[dict[str, object]] = []
    while time.monotonic() < deadline:
        frames = _frame_observations(proxy)
        requests = [frame for frame in frames if frame["type"] == 22]
        responses = [frame for frame in frames if frame["type"] == 23]
        if requests and len(requests) == len(responses):
            break
        time.sleep(0.025)

    if proxy.count != expected_connections:
        raise RuntimeError(
            f"Java used {proxy.count} TCP connections; expected exactly {expected_connections}"
        )
    if proxy.errors:
        raise RuntimeError("transport observer failed: " + proxy.errors[0])

    required_directions = {
        20: "federate-to-rti",
        21: "rti-to-federate",
        22: "rti-to-federate",
        23: "federate-to-rti",
    }
    for message_type, direction in required_directions.items():
        observed = [frame for frame in frames if frame["type"] == message_type]
        if not observed:
            raise RuntimeError(f"the Java scenario emitted no protocol type {message_type}")
        if any(frame["direction"] != direction for frame in observed):
            raise RuntimeError(f"protocol type {message_type} traveled in the wrong direction")

    call_requests = Counter(
        (frame["connection"], frame["session"], frame["sequence"])
        for frame in frames
        if frame["type"] == 20
    )
    call_responses = Counter(
        (frame["connection"], frame["session"], frame["last_received"])
        for frame in frames
        if frame["type"] == 21
    )
    if call_requests != call_responses:
        raise RuntimeError("HLA_CALL_RESPONSE acknowledgements did not match service requests")

    callback_requests = Counter(
        (frame["connection"], frame["session"], frame["sequence"])
        for frame in frames
        if frame["type"] == 22
    )
    callback_responses = Counter(
        (frame["connection"], frame["session"], frame["response_to"])
        for frame in frames
        if frame["type"] == 23
    )
    if sum(callback_requests.values()) < 1:
        raise RuntimeError("the scenario emitted no HLA_CALLBACK_REQUEST")
    if callback_requests != callback_responses:
        raise RuntimeError(
            "HLA_CALLBACK_REQUEST and HLA_CALLBACK_RESPONSE were not paired one-for-one "
            "by connection, session, and response-to sequence"
        )

    return {
        "connections": proxy.count,
        "call_requests": sum(call_requests.values()),
        "call_responses": sum(call_responses.values()),
        "callback_requests": sum(callback_requests.values()),
        "callback_responses": sum(callback_responses.values()),
        "callback_pairs": [
            {
                "connection": connection,
                "session": session,
                "sequence": sequence,
                "count": count,
            }
            for (connection, session, sequence), count in sorted(callback_requests.items())
        ],
        "observations": list(proxy.protocol_observations),
    }


def _run_model(
    arguments: argparse.Namespace,
    callback_model: str,
    classes_directory: Path,
    environment: dict[str, str],
) -> None:
    output_directory = _root_path(arguments.output_directory)
    model_directory = output_directory / callback_model.lower()
    model_directory.mkdir(parents=True, exist_ok=True)
    for generated_name in (
        "port.txt",
        "federate-idle-heartbeat-ready.ok",
        "federate-idle-heartbeat-observed.ok",
        "federate-idle-heartbeat-inactive.ok",
        "federate-idle-heartbeat-complete.ok",
        "transport-connection-count.txt",
        "transport-connection-count.tmp",
        "transport-session-protocol.txt",
        "transport-session-protocol.tmp",
        "results.json",
        "results.xml",
        "peer.json",
    ):
        (model_directory / generated_name).unlink(missing_ok=True)

    fixture = subprocess.Popen(
        [
            str(_root_path(arguments.process_fixture)),
            "public-server-federate-idle-heartbeat",
            str(model_directory),
        ],
        env=environment,
    )
    proxy = None
    proxy_thread = None
    runner = None
    try:
        host_port = CPP_ADAPTER.wait_for_port(
            model_directory, arguments.fixture_timeout_s, fixture
        )
        proxy = CPP_ADAPTER._CountingTcpProxy(
            ("127.0.0.1", host_port), model_directory / "transport-connection-count.txt"
        )
        proxy_thread = threading.Thread(
            target=proxy.serve_forever,
            name="java-tck-callback-transport-observer",
            daemon=True,
        )
        proxy_thread.start()
        proxy_address = f"tcp://127.0.0.1:{proxy.server_address[1]}"
        (model_directory / "federate-idle-heartbeat-ready.ok").write_text(
            "ready\n", encoding="ascii"
        )
        command = _run_command(
            arguments, callback_model, proxy_address, classes_directory, model_directory
        )
        runner = subprocess.Popen(command, cwd=ROOT, env=environment)
        try:
            runner.wait(timeout=arguments.timeout_s)
        except subprocess.TimeoutExpired as error:
            raise TimeoutError(f"Java TCK exceeded {arguments.timeout_s:g} seconds") from error
        if runner.returncode != 0:
            raise RuntimeError(f"Java TCK exited with {runner.returncode}")

        summary = _validate_protocol(proxy)
        if (model_directory / "federate-idle-heartbeat-inactive.ok").exists():
            raise RuntimeError("the process fixture marked a federate session inactive")
        (model_directory / "federate-idle-heartbeat-complete.ok").write_text(
            "complete\n", encoding="ascii"
        )
        try:
            fixture_status = fixture.wait(timeout=arguments.fixture_timeout_s)
        except subprocess.TimeoutExpired as error:
            raise TimeoutError("the process service fixture did not finish") from error
        if fixture_status != 0:
            raise RuntimeError(f"process service fixture exited with {fixture_status}")
        (model_directory / "peer.json").write_text(
            json.dumps(
                {
                    "scenario": SCENARIO,
                    "callback_model": callback_model,
                    "proxy_address": proxy_address,
                    "additional_settings": HEARTBEAT_SETTINGS,
                    "checks": summary,
                },
                indent=2,
            )
            + "\n",
            encoding="utf-8",
        )
        print(
            f"{callback_model}: {summary['connections']} connections, "
            f"{summary['call_requests']} matched service calls, "
            f"{summary['callback_requests']} correlated callback responses"
        )
    finally:
        if runner is not None and runner.poll() is None:
            runner.terminate()
            try:
                runner.wait(timeout=5)
            except subprocess.TimeoutExpired:
                runner.kill()
                runner.wait()
        if proxy is not None:
            proxy.shutdown()
            proxy.server_close()
        if proxy_thread is not None:
            proxy_thread.join(timeout=5)
        if fixture.poll() is None:
            fixture.terminate()
            try:
                fixture.wait(timeout=5)
            except subprocess.TimeoutExpired:
                fixture.kill()
                fixture.wait()


def main() -> int:
    arguments = parse_arguments()
    if arguments.timeout_s <= 0 or arguments.fixture_timeout_s <= 0:
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
    prepend.append(str(_root_path(arguments.process_fixture).parent))
    environment["PATH"] = os.pathsep.join([*prepend, environment.get("PATH", "")])
    for callback_model in CALLBACK_MODELS:
        _run_model(arguments, callback_model, classes_directory, environment)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, TimeoutError, subprocess.SubprocessError) as error:
        print(f"Java callback-control transport runner failed: {error}", file=sys.stderr)
        raise SystemExit(1)
