"""Run the public C++ transport TCK against an independent Table 33 peer."""

from __future__ import annotations

import argparse
import json
import socket
import struct
import subprocess
import sys
import threading
import time
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "tools" / "run_cpp_tck.py"
SCENARIO = "cpp-tck.transport-public-new-session-interoperability"
HEADER = struct.Struct(">IIQII")
SESSION_ID = 0x0102030405060708
INVALID_SEQUENCE = 0x80000000
NEW_SESSION_REQUEST = bytes.fromhex(
    "0000001c800000000000000000000000800000000000000100000001"
)
NEW_SESSION_STATUS = bytes.fromhex(
    "0000001c000000000102030405060708800000000000000200000000"
)


def _read_exact(connection: socket.socket, size: int) -> bytes:
    result = bytearray()
    while len(result) < size:
        chunk = connection.recv(size - len(result))
        if not chunk:
            raise RuntimeError("RTI closed in the middle of a Table 33 frame")
        result.extend(chunk)
    return bytes(result)


def _read_frame(connection: socket.socket) -> bytes:
    prefix = _read_exact(connection, 4)
    packet_size = struct.unpack(">I", prefix)[0]
    if packet_size < HEADER.size or packet_size > 4 * 1024 * 1024:
        raise RuntimeError(f"invalid Table 33 packet size {packet_size}")
    frame = prefix + _read_exact(connection, packet_size - len(prefix))
    if HEADER.unpack_from(frame)[0] != len(frame):
        raise RuntimeError("Table 33 packet-size field does not match the frame")
    return frame


class IndependentLoopbackPeer:
    """Literal Table 33/35/36 oracle; it imports no Umbra implementation code."""

    def __init__(self, expected_connections: int) -> None:
        self.expected_connections = expected_connections
        self.listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.listener.bind(("127.0.0.1", 0))
        self.listener.listen(expected_connections)
        self.listener.settimeout(0.2)
        self.address = f"tcp://127.0.0.1:{self.listener.getsockname()[1]}"
        self.stop_requested = threading.Event()
        self.connections: list[dict[str, object]] = []
        self.failure: str | None = None
        self.worker = threading.Thread(target=self._serve, name="hla-table33-peer")
        self.worker.start()

    def _serve(self) -> None:
        try:
            while (
                len(self.connections) < self.expected_connections
                and not self.stop_requested.is_set()
            ):
                try:
                    connection, _ = self.listener.accept()
                except socket.timeout:
                    continue
                with connection:
                    connection.settimeout(5.0)
                    self._serve_one(connection)
        except Exception as error:  # surfaced by finish() in the runner thread
            self.failure = str(error)

    def _serve_one(self, connection: socket.socket) -> None:
        request = _read_frame(connection)
        request_session_id = HEADER.unpack_from(request)[2]
        if request_session_id != 0:
            raise RuntimeError(
                "New Session Request used a nonzero Session ID before a session existed"
            )
        if request != NEW_SESSION_REQUEST:
            raise RuntimeError(
                "RTIambassador.connect did not send the literal Table 33 / "
                "Table 35 New Session request bytes: " + request.hex()
            )

        connection.sendall(NEW_SESSION_STATUS)
        status_sent_at = time.monotonic()
        peer_sequence = 1
        client_heartbeat_sequences: list[int] = []
        client_heartbeat_last_received_values: list[int] = []
        peer_heartbeat_response_sequences: list[int] = []
        peer_heartbeat_response_to_values: list[int] = []
        peer_heartbeat_response_last_received_values: list[int] = []
        first_heartbeat_elapsed_ms: int | None = None
        client_sequences: list[int] = []
        protocol_violations: list[str] = []
        close_frame: bytes | None = None
        while True:
            frame = _read_frame(connection)
            packet_size, sequence, session_id, last_received, message_type = (
                HEADER.unpack_from(frame)
            )
            if session_id != SESSION_ID:
                protocol_violations.append(
                    "The client did not retain the assigned Session ID"
                )
            if sequence > 0x7FFFFFFF:
                protocol_violations.append(
                    "The client sent a reserved-invalid post-session sequence"
                )
            if client_sequences and sequence <= client_sequences[-1]:
                protocol_violations.append(
                    "post-session Table 33 sequence numbers were not strictly increasing: "
                    + repr([*client_sequences, sequence])
                )
            client_sequences.append(sequence)
            if message_type == 3:  # Table 34 Heartbeat
                if packet_size != HEADER.size:
                    protocol_violations.append(
                        "Federate CTRL_HEARTBEAT carried an unexpected payload"
                    )
                client_heartbeat_sequences.append(sequence)
                client_heartbeat_last_received_values.append(last_received)
                if first_heartbeat_elapsed_ms is None:
                    first_heartbeat_elapsed_ms = round(
                        (time.monotonic() - status_sent_at) * 1000
                    )
                if last_received != INVALID_SEQUENCE:
                    protocol_violations.append(
                        "Control Heartbeat was incorrectly included in the "
                        "last-received HLA service message number"
                    )
                response = HEADER.pack(
                    HEADER.size + 4,
                    peer_sequence,
                    SESSION_ID,
                    INVALID_SEQUENCE,
                    4,
                ) + struct.pack(">I", sequence)
                connection.sendall(response)
                peer_heartbeat_response_sequences.append(peer_sequence)
                peer_heartbeat_response_to_values.append(sequence)
                peer_heartbeat_response_last_received_values.append(
                    INVALID_SEQUENCE
                )
                peer_sequence += 1
                continue
            if message_type == 4:  # Table 34 Heartbeat Response
                protocol_violations.append(
                    "The federate sent RTI-originated CTRL_HEARTBEAT_RESPONSE"
                )
            elif message_type == 5:  # Table 34 Close
                if close_frame is not None:
                    protocol_violations.append("The client sent more than one Close")
                if packet_size != HEADER.size:
                    protocol_violations.append("Close carried an unexpected payload")
                close_frame = frame
                if last_received != INVALID_SEQUENCE:
                    protocol_violations.append(
                        "Close reported a service message even though no HLA "
                        f"service call occurred (last-received={last_received})"
                    )
            else:
                protocol_violations.append(
                    f"unexpected Table 34 message type {message_type}"
                )
            if close_frame is None:
                continue

            close_sequence = HEADER.unpack_from(close_frame)[1]
            if close_sequence > 0x7FFFFFFF:
                raise RuntimeError("Close used a reserved-invalid sequence number")

            acknowledgement = HEADER.pack(
                HEADER.size,
                peer_sequence,
                SESSION_ID,
                close_sequence,
                6,
            )
            connection.sendall(acknowledgement)
            observed_eof = connection.recv(1) == b""
            if not observed_eof:
                raise RuntimeError(
                    "Client did not close its transport after Close Acknowledgement"
                )
            self.connections.append(
                {
                    "new_session_request_hex": request.hex(),
                    "new_session_request_session_id": request_session_id,
                    "new_session_status_hex": NEW_SESSION_STATUS.hex(),
                    "client_heartbeat_sequences": client_heartbeat_sequences,
                    "client_heartbeat_last_received_values":
                        client_heartbeat_last_received_values,
                    "peer_heartbeat_response_sequences":
                        peer_heartbeat_response_sequences,
                    "peer_heartbeat_response_to_values":
                        peer_heartbeat_response_to_values,
                    "peer_heartbeat_response_last_received_values":
                        peer_heartbeat_response_last_received_values,
                    "first_heartbeat_elapsed_ms": first_heartbeat_elapsed_ms,
                    "client_valid_sequence_numbers": client_sequences,
                    "strictly_increasing_sequence_numbers": all(
                        left < right
                        for left, right in zip(client_sequences, client_sequences[1:])
                    ),
                    "close_request_hex": close_frame.hex(),
                    "close_acknowledgement_hex": acknowledgement.hex(),
                    "client_closed_after_acknowledgement": observed_eof,
                    "protocol_violations": protocol_violations,
                }
            )
            return

    def finish(self) -> None:
        self.stop_requested.set()
        self.worker.join(timeout=8.0)
        if self.worker.is_alive():
            self.listener.close()
            self.worker.join(timeout=2.0)
        self.listener.close()
        if self.worker.is_alive():
            raise RuntimeError("independent peer did not stop within its bound")
        if self.failure:
            raise RuntimeError("independent peer failed: " + self.failure)
        if len(self.connections) != self.expected_connections:
            raise RuntimeError(
                f"independent peer observed {len(self.connections)} of "
                f"{self.expected_connections} expected public sessions"
            )
        for connection in self.connections:
            if not connection["client_heartbeat_sequences"]:
                raise RuntimeError(
                    "the federate did not send a Table 34 CTRL_HEARTBEAT "
                    "during the bounded configured interval"
                )
            if connection["protocol_violations"]:
                raise RuntimeError(
                    "independent peer observed protocol violations: "
                    + repr(connection["protocol_violations"])
                )
            if connection["first_heartbeat_elapsed_ms"] is None or connection[
                "first_heartbeat_elapsed_ms"
            ] > 550:
                raise RuntimeError(
                    "the configured 100 ms federate heartbeat did not arrive "
                    "within the 550 ms tolerance"
                )


def _option_value(arguments: list[str], name: str, default: str) -> str:
    for index, argument in enumerate(arguments):
        if argument == name and index + 1 < len(arguments):
            return arguments[index + 1]
        if argument.startswith(name + "="):
            return argument.split("=", 1)[1]
    return default


def main() -> int:
    parser = argparse.ArgumentParser(
        description=(
            "Launch a literal Table 33 peer, then run the focused portable "
            "C++ TCK through CTest and the direct runner."
        )
    )
    parser.add_argument(
        "--peer-results",
        help="Optional JSON path for the independent peer's verified frame observations",
    )
    wrapper_options, runner_arguments = parser.parse_known_args()
    if any(
        argument in {"--scenario", "--scenario-set", "--rti-address", "--skip-configure"}
        for argument in runner_arguments
    ):
        parser.error(
            "this focused wrapper owns --scenario, --scenario-set, --rti-address, "
            "and requires configuration for its ephemeral endpoint"
        )

    callback_model = _option_value(runner_arguments, "--callback-model", "both")
    model_count = 2 if callback_model == "both" else 1
    expected_connections = model_count * (
        1 if "--skip-ctest" in runner_arguments else 2
    )
    peer = IndependentLoopbackPeer(expected_connections)
    command = [
        sys.executable,
        str(RUNNER),
        "--scenario-set",
        "all",
        "--scenario",
        SCENARIO,
        "--rti-address",
        peer.address,
        *runner_arguments,
    ]
    runner = subprocess.run(command, cwd=ROOT, check=False)
    peer_failure: str | None = None
    try:
        peer.finish()
    except RuntimeError as error:
        peer_failure = str(error)

    if wrapper_options.peer_results:
        result_path = Path(wrapper_options.peer_results)
        if not result_path.is_absolute():
            result_path = ROOT / result_path
        result_path.parent.mkdir(parents=True, exist_ok=True)
        result_path.write_text(
            json.dumps(
                {
                    "address": peer.address,
                    "expected_connections": expected_connections,
                    "connections": peer.connections,
                    "failure": peer_failure,
                },
                indent=2,
            )
            + "\n",
            encoding="utf-8",
        )

    if peer_failure:
        print(peer_failure, file=sys.stderr)
    return runner.returncode if runner.returncode != 0 else int(peer_failure is not None)


if __name__ == "__main__":
    raise SystemExit(main())
