"""Exercise the public C++ Connect path against invalid session metadata."""

from __future__ import annotations

import argparse
import socket
import socketserver
import struct
import subprocess
import tempfile
import threading
from pathlib import Path


SCENARIO = "cpp-tck.reject-invalid-transport-session-metadata"
INVALID_SEQUENCE = 0x80000000
NEW_SESSION = 1
NEW_SESSION_STATUS = 2
PROTOCOL_VERSION = 1


def receive_exact(connection: socket.socket, size: int) -> bytes:
    chunks = bytearray()
    while len(chunks) < size:
        part = connection.recv(size - len(chunks))
        if not part:
            raise RuntimeError(
                f"peer closed after {len(chunks)} of {size} handshake bytes"
            )
        chunks.extend(part)
    return bytes(chunks)


def make_status(sequence: int, session_id: int) -> bytes:
    # Table 33 header followed by the four-byte success reason.
    return struct.pack(
        ">IIQIII",
        28,
        sequence,
        session_id,
        INVALID_SEQUENCE,
        NEW_SESSION_STATUS,
        0,
    )


class _InvalidMetadataHandler(socketserver.BaseRequestHandler):
    def handle(self) -> None:
        server: _InvalidMetadataServer = self.server  # type: ignore[assignment]
        self.request.settimeout(server.timeout_s)
        try:
            request = receive_exact(self.request, 28)
            server.requests += 1
            if int.from_bytes(request[0:4], "big") != 28:
                raise RuntimeError("New Session request did not have a 28-byte frame")
            if int.from_bytes(request[20:24], "big") != NEW_SESSION:
                raise RuntimeError("client did not send CTRL_NEW_SESSION")
            if request[8:16] != bytes(8):
                raise RuntimeError("New Session request used a nonzero Session ID")
            if int.from_bytes(request[24:28], "big") != PROTOCOL_VERSION:
                raise RuntimeError("client sent an unexpected transport protocol version")

            if server.requests == 1:
                # First response: valid Session ID, invalid sequence number.
                response = make_status(INVALID_SEQUENCE, 1)
            elif server.requests == 2:
                # Second response: valid sequence number, invalid zero Session ID.
                response = make_status(0, 0)
            else:
                raise RuntimeError("received more than the two expected Connect attempts")

            self.request.sendall(response)
            while True:
                data = self.request.recv(1)
                if not data:
                    server.closed_sockets += 1
                    return
                raise RuntimeError(
                    "client sent additional bytes instead of closing after invalid metadata"
                )
        except (OSError, RuntimeError) as error:
            server.errors.append(str(error))


class _InvalidMetadataServer(socketserver.ThreadingTCPServer):
    allow_reuse_address = True
    daemon_threads = True

    def __init__(self, timeout_s: float) -> None:
        super().__init__(("127.0.0.1", 0), _InvalidMetadataHandler)
        self.timeout_s = timeout_s
        self.requests = 0
        self.closed_sockets = 0
        self.errors: list[str] = []


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tck-executable", type=Path, required=True)
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


def run() -> int:
    arguments = parse_arguments()
    arguments.results.parent.mkdir(parents=True, exist_ok=True)
    arguments.junit.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(
        prefix="cpp-tck-invalid-session-metadata-",
        dir=arguments.results.parent,
    ) as temporary_directory:
        directory = Path(temporary_directory)
        model_results = directory / "results.json"
        model_junit = directory / "results.xml"
        server = _InvalidMetadataServer(arguments.timeout_s)
        server_thread = threading.Thread(
            target=server.serve_forever,
            name="cpp-tck-invalid-session-metadata-peer",
            daemon=True,
        )
        server_thread.start()
        try:
            command = [
                str(arguments.tck_executable),
                "--fom",
                str(arguments.fom_path),
                "--mim-fom",
                str(arguments.mim_fom),
                "--time-implementation",
                arguments.time_implementation,
                "--scenario",
                SCENARIO,
                "--callback-model",
                arguments.callback_model,
                "--provider-id",
                arguments.provider_id,
                "--rti-address",
                f"tcp://127.0.0.1:{server.server_address[1]}",
                "--timeout-ms",
                str(max(int(arguments.timeout_s * 1000), 3000)),
                "--results",
                str(model_results),
                "--junit",
                str(model_junit),
            ]
            subprocess.run(command, check=True, timeout=arguments.timeout_s)
        finally:
            server.shutdown()
            server.server_close()
            server_thread.join(timeout=5.0)

        if server.errors:
            raise RuntimeError("invalid-metadata peer failed: " + server.errors[0])
        if server.requests != 2 or server.closed_sockets != 2:
            raise RuntimeError(
                "expected two invalid-metadata handshakes and two client socket "
                f"closures; observed {server.requests} handshakes and "
                f"{server.closed_sockets} closures"
            )
        arguments.results.write_bytes(model_results.read_bytes())
        arguments.junit.write_bytes(model_junit.read_bytes())
    return 0


if __name__ == "__main__":
    raise SystemExit(run())
