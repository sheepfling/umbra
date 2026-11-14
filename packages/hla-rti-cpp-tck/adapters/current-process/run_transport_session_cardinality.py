"""Run the public C++ transport-connection/federate-session TCK case."""

from __future__ import annotations

import argparse
import os
import select
import socket
import socketserver
import struct
import subprocess
import tempfile
import threading
import time
from collections import Counter
from pathlib import Path


SCENARIO = "cpp-tck.one-transport-connection-per-federate-session"
CALLBACK_DIRECTION_SCENARIO = (
    "cpp-tck.callback-controls-object-discovery-transport-direction"
)
ADDITIONAL_SETTINGS = (
    "FED_INT_HEART=0.1;FED_TIMEOUT_HEART=2;FED_TIMEOUT_RECONNECT=0.25"
)


class _ForwardingHandler(socketserver.BaseRequestHandler):
    def handle(self) -> None:
        server = self.server
        try:
            upstream = socket.create_connection(server.upstream, timeout=5.0)
        except OSError as error:
            server.errors.append(str(error))
            return
        connection_index = server.record_connection()
        with upstream:
            endpoints = (self.request, upstream)
            frame_buffers = {endpoint: bytearray() for endpoint in endpoints}
            closed_endpoints: set[socket.socket] = set()
            while len(closed_endpoints) != len(endpoints):
                readable, _, _ = select.select(
                    [endpoint for endpoint in endpoints if endpoint not in closed_endpoints],
                    (),
                    (),
                    0.25,
                )
                for source in readable:
                    try:
                        payload = source.recv(65536)
                    except OSError as error:
                        server.errors.append(str(error))
                        return
                    destination = upstream if source is self.request else self.request
                    direction = (
                        "federate-to-rti"
                        if source is self.request
                        else "rti-to-federate"
                    )
                    if not payload:
                        server.record_close_event(connection_index, direction)
                        closed_endpoints.add(source)
                        try:
                            destination.shutdown(socket.SHUT_WR)
                        except OSError as error:
                            server.errors.append(str(error))
                            return
                        continue
                    server.observe_stream(
                        connection_index,
                        direction,
                        frame_buffers[source],
                        payload,
                    )
                    try:
                        destination.sendall(payload)
                    except OSError as error:
                        server.errors.append(str(error))
                        return
        server.probe_terminated_session(connection_index)


class _CountingTcpProxy(socketserver.ThreadingTCPServer):
    allow_reuse_address = True
    daemon_threads = True

    def __init__(self, upstream: tuple[str, int], count_path: Path) -> None:
        super().__init__(("127.0.0.1", 0), _ForwardingHandler)
        self.upstream = upstream
        self.count_path = count_path
        self.protocol_observation_path = count_path.with_name(
            "transport-session-protocol.txt"
        )
        self.count = 0
        self.errors: list[str] = []
        self.protocol_observations: list[str] = []
        self.termination_requests: dict[int, tuple[str, int, int]] = {}
        self._count_lock = threading.Lock()

    def record_connection(self) -> int:
        with self._count_lock:
            self.count += 1
            temporary = self.count_path.with_suffix(".tmp")
            temporary.write_text(f"{self.count}\n", encoding="ascii")
            temporary.replace(self.count_path)
            return self.count

    def record_close_event(self, connection_index: int, direction: str) -> None:
        with self._count_lock:
            self.protocol_observations.append(
                f"closed {connection_index} {direction}"
            )
            temporary = self.protocol_observation_path.with_suffix(".tmp")
            temporary.write_text(
                "\n".join(self.protocol_observations) + "\n",
                encoding="ascii",
            )
            temporary.replace(self.protocol_observation_path)

    def record_termination_request(
        self,
        connection_index: int,
        session_id: str,
        sequence_number: int,
        last_received_message_number: int,
    ) -> None:
        with self._count_lock:
            self.termination_requests[connection_index] = (
                session_id,
                sequence_number,
                last_received_message_number,
            )

    def probe_terminated_session(self, connection_index: int) -> None:
        with self._count_lock:
            termination = self.termination_requests.get(connection_index)
        if termination is None:
            return

        session_id, close_sequence, last_received = termination
        resume_sequence = (close_sequence + 1) & 0x7FFFFFFF
        request = struct.pack(
            "!IIQIIII",
            32,
            resume_sequence,
            int(session_id, 16),
            last_received,
            10,  # CTRL_RESUME_REQUEST
            last_received,
            0x80000000,  # No unacknowledged federate service messages remain.
        )
        outcome = "error"
        try:
            with socket.create_connection(self.upstream, timeout=3.0) as probe:
                probe.settimeout(3.0)
                probe.sendall(request)
                response_header = bytearray()
                while len(response_header) < 4:
                    chunk = probe.recv(4 - len(response_header))
                    if not chunk:
                        break
                    response_header.extend(chunk)
                if not response_header:
                    outcome = "closed"
                elif len(response_header) != 4:
                    outcome = "truncated"
                else:
                    frame_size = int.from_bytes(response_header, "big")
                    if frame_size < 24 or frame_size > 4 * 1024 * 1024 + 24:
                        outcome = "malformed-frame"
                    else:
                        response = bytearray(response_header)
                        while len(response) < frame_size:
                            chunk = probe.recv(frame_size - len(response))
                            if not chunk:
                                break
                            response.extend(chunk)
                        if len(response) != frame_size:
                            outcome = "truncated"
                        else:
                            response_type = int.from_bytes(response[20:24], "big")
                            outcome = f"response-{response_type}"
        except (OSError, TimeoutError) as error:
            with self._count_lock:
                self.errors.append(
                    f"stale-session Resume Request probe failed: {error}"
                )

        with self._count_lock:
            self.protocol_observations.append(
                f"stale-resume-probe {session_id} {outcome}"
            )
            temporary = self.protocol_observation_path.with_suffix(".tmp")
            temporary.write_text(
                "\n".join(self.protocol_observations) + "\n",
                encoding="ascii",
            )
            temporary.replace(self.protocol_observation_path)

    def observe_stream(
        self,
        connection_index: int,
        direction: str,
        buffer: bytearray,
        payload: bytes,
    ) -> None:
        buffer.extend(payload)
        while len(buffer) >= 4:
            frame_size = int.from_bytes(buffer[:4], "big")
            if frame_size < 24:
                self.errors.append(
                    f"transport frame on connection {connection_index} has "
                    f"invalid {frame_size}-byte header"
                )
                buffer.clear()
                return
            if len(buffer) < frame_size:
                return
            frame = bytes(buffer[:frame_size])
            del buffer[:frame_size]
            if len(frame) < 24:
                self.errors.append(
                    f"transport frame on connection {connection_index} is truncated"
                )
                continue

            sequence_number = int.from_bytes(frame[4:8], "big")
            last_received_message_number = int.from_bytes(frame[16:20], "big")
            message_type = int.from_bytes(frame[20:24], "big")
            session_id = frame[8:16].hex()
            reason = 0
            if message_type == 1:
                if direction != "federate-to-rti":
                    self.errors.append("CTRL_NEW_SESSION traveled in the wrong direction")
            elif message_type == 2:
                if direction != "rti-to-federate":
                    self.errors.append(
                        "CTRL_NEW_SESSION_STATUS traveled in the wrong direction"
                    )
                if len(frame) < 28:
                    self.errors.append(
                        f"session-control message {message_type} on connection "
                        f"{connection_index} has no four-byte payload"
                    )
                    continue
                reason = int.from_bytes(frame[24:28], "big")
            elif message_type == 4:
                if direction != "rti-to-federate":
                    self.errors.append(
                        "CTRL_HEARTBEAT_RESPONSE traveled in the wrong direction"
                    )
                if len(frame) < 28:
                    self.errors.append(
                        f"heartbeat response on connection {connection_index} "
                        "has no response-to sequence number"
                    )
                    continue
                reason = int.from_bytes(frame[24:28], "big")
            elif message_type == 3 and direction != "federate-to-rti":
                self.errors.append("CTRL_HEARTBEAT traveled in the wrong direction")
            elif message_type == 5:
                if direction != "federate-to-rti":
                    self.errors.append("CTRL_CLOSE traveled in the wrong direction")
                if len(frame) != 24:
                    self.errors.append("CTRL_CLOSE must have an empty payload")
                self.record_termination_request(
                    connection_index,
                    session_id,
                    sequence_number,
                    last_received_message_number,
                )
            elif message_type == 6:
                if direction != "rti-to-federate":
                    self.errors.append("CTRL_CLOSE_ACK traveled in the wrong direction")
                if len(frame) != 24:
                    self.errors.append("CTRL_CLOSE_ACK must have an empty payload")
            elif message_type == 23:
                if direction != "federate-to-rti":
                    self.errors.append(
                        "HLA_CALLBACK_RESPONSE traveled in the wrong direction"
                    )
                if len(frame) < 30:
                    self.errors.append(
                        f"HLA_CALLBACK_RESPONSE on connection {connection_index} "
                        "has no In response to field and protobuf result"
                    )
                    continue
                reason = int.from_bytes(frame[24:28], "big")
                if frame[28:] != b"\x0a\x00":
                    self.errors.append(
                        "The successful object-discovery callback response did not "
                        "encode CallbackSucceeded"
                    )
            elif message_type == 20 and direction != "federate-to-rti":
                self.errors.append(
                    f"federate-originated service message {message_type} traveled "
                    "in the wrong direction"
                )
            elif message_type in (21, 22) and direction != "rti-to-federate":
                self.errors.append(
                    f"RTI-originated service message {message_type} traveled "
                    "in the wrong direction"
                )

            observation = (
                f"frame {connection_index} {direction} {sequence_number} "
                f"{last_received_message_number} {message_type} "
                f"{session_id} {reason}"
            )

            with self._count_lock:
                self.protocol_observations.append(observation)
                temporary = self.protocol_observation_path.with_suffix(".tmp")
                temporary.write_text(
                    "\n".join(self.protocol_observations) + "\n",
                    encoding="ascii",
                )
                temporary.replace(self.protocol_observation_path)


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--scenario", default=SCENARIO)
    parser.add_argument("--expected-connections", type=int, default=2)
    parser.add_argument("--minimum-callback-request-count", type=int, default=1)
    parser.add_argument("--require-call-callback-directions", action="store_true")
    parser.add_argument(
        "--require-callback-response-correlation", action="store_true"
    )
    parser.add_argument("--require-federate-heartbeat-response-pairing", action="store_true")
    parser.add_argument("--tck-executable", type=Path, required=True)
    parser.add_argument("--process-fixture", type=Path, required=True)
    parser.add_argument("--fom-path", type=Path, required=True)
    parser.add_argument("--mim-fom", type=Path, required=True)
    parser.add_argument("--time-implementation", required=True)
    parser.add_argument(
        "--callback-model", choices=("evoked", "immediate"), required=True
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
            port = int(port_path.read_text(encoding="ascii").strip())
        except (FileNotFoundError, OSError, ValueError):
            port = 0
        if 0 < port <= 65535:
            return port
        if fixture.poll() is not None:
            raise RuntimeError("The process fixture exited before publishing its port")
        time.sleep(0.025)
    raise RuntimeError("The process fixture did not publish a listening port")


def run() -> int:
    arguments = parse_arguments()
    if arguments.expected_connections < 1:
        raise ValueError("--expected-connections must be positive")
    if arguments.minimum_callback_request_count < 1:
        raise ValueError("--minimum-callback-request-count must be positive")
    arguments.results.parent.mkdir(parents=True, exist_ok=True)
    arguments.junit.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(
        prefix="cpp-tck-session-cardinality-",
        dir=arguments.results.parent,
    ) as temporary_directory:
        directory = Path(temporary_directory)
        model_results = directory / "results.json"
        model_junit = directory / "results.xml"
        fixture: subprocess.Popen[bytes] | None = None
        proxy: _CountingTcpProxy | None = None
        proxy_thread: threading.Thread | None = None
        try:
            fixture = subprocess.Popen(
                [
                    str(arguments.process_fixture),
                    "public-server-federate-idle-heartbeat",
                    str(directory),
                ]
            )
            host_port = wait_for_port(directory, arguments.timeout_s, fixture)
            proxy = _CountingTcpProxy(
                ("127.0.0.1", host_port),
                directory / "transport-connection-count.txt",
            )
            proxy_thread = threading.Thread(
                target=proxy.serve_forever,
                name="cpp-tck-connection-counter",
                daemon=True,
            )
            proxy_thread.start()
            if (
                arguments.require_call_callback_directions
                or arguments.require_callback_response_correlation
            ):
                (directory / "federate-idle-heartbeat-ready.ok").write_text(
                    "ready\n", encoding="ascii"
                )
            command = [
                str(arguments.tck_executable),
                "--fom",
                str(arguments.fom_path),
                "--mim-fom",
                str(arguments.mim_fom),
                "--time-implementation",
                arguments.time_implementation,
                "--scenario",
                arguments.scenario,
                "--callback-model",
                arguments.callback_model,
                "--provider-id",
                arguments.provider_id,
                "--rti-address",
                f"tcp://127.0.0.1:{proxy.server_address[1]}",
                "--additional-settings",
                ADDITIONAL_SETTINGS,
                "--timeout-ms",
                str(max(int(arguments.timeout_s * 1000), 3000)),
                "--results",
                str(model_results),
                "--junit",
                str(model_junit),
            ]
            environment = os.environ.copy()
            runtime_directory = str(arguments.process_fixture.resolve().parent)
            existing_path = environment.get("PATH", "")
            environment["PATH"] = os.pathsep.join(
                value for value in (existing_path, runtime_directory) if value
            )
            subprocess.run(
                command,
                check=True,
                timeout=arguments.timeout_s,
                env=environment,
            )
            if proxy.count != arguments.expected_connections:
                raise RuntimeError(
                    "The scenario used "
                    f"{proxy.count} TCP transport connections instead of exactly "
                    f"{arguments.expected_connections}"
                )
            if proxy.errors:
                raise RuntimeError("The TCP counting proxy failed: " + proxy.errors[0])
            if arguments.require_call_callback_directions:
                direction_observations = [
                    fields
                    for observation in proxy.protocol_observations
                    if (fields := observation.split())
                    and len(fields) == 8
                    and fields[0] == "frame"
                    and fields[5] in {"21", "22"}
                ]
                observed_message_types = {fields[5] for fields in direction_observations}
                if observed_message_types != {"21", "22"}:
                    raise RuntimeError(
                        "The object-discovery scenario did not traverse both "
                        "HLA_CALL_RESPONSE and HLA_CALLBACK_REQUEST frames"
                    )
                if any(fields[2] != "rti-to-federate" for fields in direction_observations):
                    raise RuntimeError(
                        "An HLA_CALL_RESPONSE or HLA_CALLBACK_REQUEST was not sent by the RTI"
                    )
                federate_originated_types = {
                    fields[5]
                    for observation in proxy.protocol_observations
                    if (fields := observation.split())
                    and len(fields) == 8
                    and fields[0] == "frame"
                    and fields[2] == "federate-to-rti"
                    and fields[5] in {"20", "23"}
                }
                if federate_originated_types != {"20", "23"}:
                    raise RuntimeError(
                        "The object-discovery scenario did not traverse both "
                        "federate-originated HLA_CALL_REQUEST and "
                        "HLA_CALLBACK_RESPONSE frames"
                    )
            if arguments.require_callback_response_correlation:
                callback_requests = Counter(
                    (fields[1], fields[6], fields[3])
                    for observation in proxy.protocol_observations
                    if (fields := observation.split())
                    and len(fields) == 8
                    and fields[0] == "frame"
                    and fields[2] == "rti-to-federate"
                    and fields[5] == "22"
                )
                callback_responses = Counter(
                    (fields[1], fields[6], fields[7])
                    for observation in proxy.protocol_observations
                    if (fields := observation.split())
                    and len(fields) == 8
                    and fields[0] == "frame"
                    and fields[2] == "federate-to-rti"
                    and fields[5] == "23"
                )
                if len(callback_requests) < arguments.minimum_callback_request_count:
                    raise RuntimeError(
                        "The scenario emitted "
                        f"{len(callback_requests)} distinct HLA_CALLBACK_REQUEST frames; "
                        "expected at least "
                        f"{arguments.minimum_callback_request_count}"
                    )
                if callback_requests != callback_responses:
                    missing_responses = callback_requests - callback_responses
                    unexpected_responses = callback_responses - callback_requests
                    raise RuntimeError(
                        "HLA_CALLBACK_REQUEST and HLA_CALLBACK_RESPONSE frames were not "
                        "paired one-for-one by connection, session, and In response to sequence; "
                        f"missing responses={dict(missing_responses)}, "
                        f"unexpected responses={dict(unexpected_responses)}"
                    )
            if arguments.require_federate_heartbeat_response_pairing:
                heartbeat_frames = [
                    fields
                    for observation in proxy.protocol_observations
                    if (fields := observation.split())
                    and len(fields) == 8
                    and fields[0] == "frame"
                    and fields[5] == "3"
                ]
                heartbeat_response_frames = [
                    fields
                    for observation in proxy.protocol_observations
                    if (fields := observation.split())
                    and len(fields) == 8
                    and fields[0] == "frame"
                    and fields[5] == "4"
                ]
                if not heartbeat_frames:
                    raise RuntimeError(
                        "The TCK session scenario emitted no CTRL_HEARTBEAT frame"
                    )
                heartbeat_requests = Counter(
                    (fields[1], fields[6], fields[3])
                    for fields in heartbeat_frames
                )
                heartbeat_replies = Counter(
                    (fields[1], fields[6], fields[7])
                    for fields in heartbeat_response_frames
                )
                if heartbeat_requests != heartbeat_replies:
                    raise RuntimeError(
                        "CTRL_HEARTBEAT and CTRL_HEARTBEAT_RESPONSE frames were not "
                        "paired one-for-one by connection, session, and request sequence"
                    )
            if (directory / "federate-idle-heartbeat-inactive.ok").exists():
                raise RuntimeError("The process fixture marked a federate session inactive")
            (directory / "federate-idle-heartbeat-complete.ok").write_text(
                "complete\n", encoding="ascii"
            )
            if fixture.wait(timeout=arguments.timeout_s) != 0:
                raise RuntimeError("The process fixture failed")
            arguments.results.write_bytes(model_results.read_bytes())
            arguments.junit.write_bytes(model_junit.read_bytes())
        finally:
            if proxy is not None:
                proxy.shutdown()
                proxy.server_close()
            if proxy_thread is not None:
                proxy_thread.join(timeout=5.0)
            if fixture is not None and fixture.poll() is None:
                fixture.terminate()
                try:
                    fixture.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    fixture.kill()
                    fixture.wait()
    return 0


if __name__ == "__main__":
    raise SystemExit(run())
