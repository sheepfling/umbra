"""Run the Java session-cardinality TCK through the current-process peer."""

from __future__ import annotations

import argparse
import importlib.util
import json
import os
import subprocess
import sys
import threading
import time
from collections import Counter, defaultdict
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
SCENARIO = "java-tck.one-transport-connection-per-federate-session"
CALLBACK_MODELS = ("HLA_EVOKED", "HLA_IMMEDIATE")
HEARTBEAT_SETTINGS = (
    "FED_INT_HEART=0.1;FED_TIMEOUT_HEART=2;FED_TIMEOUT_RECONNECT=0.25"
)


def _load_cpp_adapter() -> ModuleType:
    specification = importlib.util.spec_from_file_location(
        "cpp_transport_session_cardinality_adapter", ADAPTER_PATH
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
            "--jvm-argument=-Dhla.rti.tck.transportSessionProxyAddress="
            + proxy_address,
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
                    "reason": int(fields[7]),
                }
            )
    return frames


def _validate_protocol(proxy: object, expected_connections: int) -> dict[str, object]:
    deadline = time.monotonic() + 6.0
    while time.monotonic() < deadline:
        records = list(proxy.protocol_observations)
        stale_count = sum(record.startswith("stale-resume-probe ") for record in records)
        close_count = sum(
            record.startswith("closed ") and record.endswith(" federate-to-rti")
            for record in records
        )
        if stale_count >= expected_connections and close_count >= expected_connections:
            break
        time.sleep(0.025)

    if proxy.count != expected_connections:
        raise RuntimeError(
            f"Java used {proxy.count} TCP connections; expected exactly {expected_connections}"
        )
    if proxy.errors:
        raise RuntimeError("transport observer failed: " + proxy.errors[0])

    records = list(proxy.protocol_observations)
    frames = _frame_observations(proxy)
    requests = [frame for frame in frames if frame["type"] == 1]
    statuses = [frame for frame in frames if frame["type"] == 2]
    no_session = "0000000000000000"
    if len(requests) != expected_connections or any(
        frame["session"] != no_session for frame in requests
    ):
        raise RuntimeError("each Java Connect must send one New Session request with the zero ID")
    if len(statuses) != expected_connections or any(frame["reason"] != 0 for frame in statuses):
        raise RuntimeError("each Java Connect must receive one successful New Session status")
    request_connections = {int(frame["connection"]) for frame in requests}
    status_connections = {int(frame["connection"]) for frame in statuses}
    assigned_sessions = {str(frame["session"]) for frame in statuses}
    if (
        request_connections != status_connections
        or len(request_connections) != expected_connections
        or no_session in assigned_sessions
        or len(assigned_sessions) != expected_connections
    ):
        raise RuntimeError("New Session statuses did not assign distinct IDs on their requesting connections")

    final_sequence: dict[tuple[int, str], int] = {}
    for frame in frames:
        sequence = int(frame["sequence"])
        if int(frame["type"]) == 1 and sequence == 0x80000000:
            continue
        if sequence > 0x7FFFFFFF:
            raise RuntimeError("a transport sequence used its reserved most-significant bit")
        key = (int(frame["connection"]), str(frame["direction"]))
        previous = final_sequence.get(key)
        if previous is not None and not (
            sequence > previous or (previous == 0x7FFFFFFF and sequence == 0)
        ):
            raise RuntimeError("transport sequence numbers did not increase or wrap to zero")
        final_sequence[key] = sequence

    call_requests: dict[int, list[int]] = defaultdict(list)
    call_acknowledgements: dict[int, list[int]] = defaultdict(list)
    for frame in frames:
        message_type = int(frame["type"])
        if message_type == 20:
            if frame["direction"] != "federate-to-rti":
                raise RuntimeError("HLA_CALL_REQUEST traveled in the wrong direction")
            call_requests[int(frame["connection"])].append(int(frame["sequence"]))
        elif message_type == 21:
            if frame["direction"] != "rti-to-federate":
                raise RuntimeError("HLA_CALL_RESPONSE traveled in the wrong direction")
            call_acknowledgements[int(frame["connection"])].append(
                int(frame["last_received"])
            )
        elif message_type == 22 and frame["direction"] != "rti-to-federate":
            raise RuntimeError("HLA_CALLBACK_REQUEST traveled in the wrong direction")
        elif message_type == 23 and frame["direction"] != "federate-to-rti":
            raise RuntimeError("HLA_CALLBACK_RESPONSE traveled in the wrong direction")
    if not call_requests or not call_acknowledgements or set(call_requests) != set(
        call_acknowledgements
    ):
        raise RuntimeError("the two sessions did not observe service requests and responses")
    if any(
        sorted(call_requests[connection]) != sorted(call_acknowledgements[connection])
        for connection in call_requests
    ):
        raise RuntimeError("HLA_CALL_RESPONSE acknowledgements did not match request sequences")

    heartbeat_requests = Counter(
        (int(frame["connection"]), str(frame["session"]), int(frame["sequence"]))
        for frame in frames
        if int(frame["type"]) == 3
    )
    heartbeat_responses = Counter(
        (int(frame["connection"]), str(frame["session"]), int(frame["reason"]))
        for frame in frames
        if int(frame["type"]) == 4
    )
    if not heartbeat_requests or heartbeat_requests != heartbeat_responses:
        raise RuntimeError("heartbeat responses did not pair with federate heartbeats")
    heartbeat_connections = {connection for connection, _, _ in heartbeat_requests}
    if heartbeat_connections != set(range(1, expected_connections + 1)):
        raise RuntimeError("both Java sessions must emit a paired heartbeat")

    close_requests: dict[int, dict[str, object]] = {}
    close_responses: dict[int, dict[str, object]] = {}
    client_close_indices: dict[int, int] = {}
    stale_probes: dict[str, str] = {}
    for index, record in enumerate(records):
        fields = record.split()
        if len(fields) == 8 and fields[0] == "frame":
            connection = int(fields[1])
            message_type = int(fields[5])
            if message_type in (5, 6):
                target = close_requests if message_type == 5 else close_responses
                if connection in target:
                    raise RuntimeError("a session sent a duplicate termination frame")
                target[connection] = {
                    "session": fields[6],
                    "sequence": int(fields[3]),
                    "last_received": int(fields[4]),
                    "index": index,
                    "direction": fields[2],
                }
        elif len(fields) == 3 and fields[0] == "closed":
            if fields[2] == "federate-to-rti":
                connection = int(fields[1])
                if connection in client_close_indices:
                    raise RuntimeError("a transport connection closed more than once")
                client_close_indices[connection] = index
        elif len(fields) == 3 and fields[0] == "stale-resume-probe":
            if fields[1] in stale_probes:
                raise RuntimeError("a terminated session received more than one stale Resume probe")
            stale_probes[fields[1]] = fields[2]

    expected = set(range(1, expected_connections + 1))
    if (
        set(close_requests) != expected
        or set(close_responses) != expected
        or set(client_close_indices) != expected
        or len(stale_probes) != expected_connections
    ):
        raise RuntimeError("Disconnect did not exchange one termination pair per session")
    terminated_sessions: set[str] = set()
    for connection in sorted(expected):
        request = close_requests[connection]
        response = close_responses[connection]
        if request["direction"] != "federate-to-rti" or response["direction"] != "rti-to-federate":
            raise RuntimeError("CTRL_CLOSE/CTRL_CLOSE_ACK traveled in the wrong direction")
        if (
            request["session"] != response["session"]
            or response["last_received"] != request["sequence"]
            or not int(request["index"]) < int(response["index"]) < client_close_indices[connection]
        ):
            raise RuntimeError("CTRL_CLOSE_ACK did not match and precede client EOF")
        terminated_sessions.add(str(request["session"]))
    if set(stale_probes) != terminated_sessions or any(
        outcome != "closed" for outcome in stale_probes.values()
    ):
        raise RuntimeError("stale Resume probes did not close for both terminated Session IDs")

    return {
        "connections": proxy.count,
        "new_session_requests": len(requests),
        "successful_session_statuses": len(statuses),
        "assigned_session_ids": sorted(assigned_sessions),
        "call_requests": sum(len(values) for values in call_requests.values()),
        "call_responses": sum(len(values) for values in call_acknowledgements.values()),
        "heartbeats": sum(heartbeat_requests.values()),
        "heartbeat_responses": sum(heartbeat_responses.values()),
        "close_requests": len(close_requests),
        "close_acknowledgements": len(close_responses),
        "stale_resume_probes": stale_probes,
        "observations": records,
    }


def _wait_for_handshakes(proxy: object, directory: Path, process: subprocess.Popen[bytes],
      timeout_s: float) -> None:
    deadline = time.monotonic() + timeout_s
    ready_marker = directory / "federate-idle-heartbeat-ready.ok"
    while time.monotonic() < deadline:
        if process.poll() is not None:
            raise RuntimeError(
                f"Java TCK exited with {process.returncode} before both session handshakes"
            )
        statuses = [
            frame
            for frame in _frame_observations(proxy)
            if int(frame["type"]) == 2 and int(frame["reason"]) == 0
        ]
        if len(statuses) >= 2:
            ready_marker.write_text("ready\n", encoding="ascii")
            return
        time.sleep(0.025)
    raise TimeoutError("Java TCK did not complete both New Session handshakes")


def _run_model(arguments: argparse.Namespace, callback_model: str,
      classes_directory: Path, environment: dict[str, str]) -> None:
    output_directory = _root_path(arguments.output_directory)
    model_directory = output_directory / callback_model.lower()
    model_directory.mkdir(parents=True, exist_ok=True)
    for generated_name in (
        "port.txt",
        "federate-idle-heartbeat-ready.ok",
        "federate-idle-heartbeat-observed.ok",
        "federate-idle-heartbeat-repeated.ok",
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
    process_fixture = _root_path(arguments.process_fixture)
    fixture = subprocess.Popen(
        [
            str(process_fixture),
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
            name="java-tck-transport-session-observer",
            daemon=True,
        )
        proxy_thread.start()
        proxy_address = f"tcp://127.0.0.1:{proxy.server_address[1]}"
        command = _run_command(
            arguments, callback_model, proxy_address, classes_directory, model_directory
        )
        runner = subprocess.Popen(command, cwd=ROOT, env=environment)
        _wait_for_handshakes(proxy, model_directory, runner, arguments.timeout_s)
        try:
            runner.wait(timeout=arguments.timeout_s)
        except subprocess.TimeoutExpired as error:
            raise TimeoutError(f"Java TCK exceeded {arguments.timeout_s:g} seconds") from error
        if runner.returncode != 0:
            raise RuntimeError(f"Java TCK exited with {runner.returncode}")

        summary = _validate_protocol(proxy, expected_connections=2)
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
                    "heartbeat_settings": HEARTBEAT_SETTINGS,
                    "checks": summary,
                },
                indent=2,
            )
            + "\n",
            encoding="utf-8",
        )
        print(
            f"{callback_model}: {summary['connections']} connections, "
            f"{summary['call_requests']} service requests, "
            f"{summary['heartbeats']} paired heartbeats, "
            f"{summary['close_acknowledgements']} termination acknowledgements"
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
    environment["PATH"] = os.pathsep.join(
        [*prepend, environment.get("PATH", "")]
    )
    for callback_model in CALLBACK_MODELS:
        _run_model(arguments, callback_model, classes_directory, environment)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, TimeoutError, subprocess.SubprocessError) as error:
        print(f"Java transport session-cardinality runner failed: {error}", file=sys.stderr)
        raise SystemExit(1)
