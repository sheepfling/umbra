"""Run the WebSocket C++ TCK case with adapter-observed frame opcodes."""

from __future__ import annotations

import argparse
import base64
import json
import os
import select
import socket
import socketserver
import subprocess
import tempfile
import threading
import time
from pathlib import Path


SCENARIO = "cpp-tck.websocket-transport-support"
HTTP_HEADER_END = b"\r\n\r\n"


class _FrameInspector:
    def __init__(self) -> None:
        self._buffer = bytearray()
        self._handshake_complete = False
        self.frames: list[dict[str, object]] = []

    def feed(self, payload: bytes) -> list[dict[str, object]]:
        self._buffer.extend(payload)
        if not self._handshake_complete:
            boundary = self._buffer.find(HTTP_HEADER_END)
            if boundary < 0:
                return []
            del self._buffer[: boundary + len(HTTP_HEADER_END)]
            self._handshake_complete = True

        observed: list[dict[str, object]] = []
        while len(self._buffer) >= 2:
            first = self._buffer[0]
            second = self._buffer[1]
            payload_length = second & 0x7F
            header_length = 2
            if payload_length == 126:
                if len(self._buffer) < 4:
                    break
                payload_length = int.from_bytes(self._buffer[2:4], "big")
                header_length = 4
            elif payload_length == 127:
                if len(self._buffer) < 10:
                    break
                payload_length = int.from_bytes(self._buffer[2:10], "big")
                header_length = 10
            masked = bool(second & 0x80)
            frame_length = header_length + (4 if masked else 0) + payload_length
            if len(self._buffer) < frame_length:
                break
            observed.append(
                {"opcode": first & 0x0F, "masked": masked, "payload_bytes": payload_length}
            )
            del self._buffer[:frame_length]
        self.frames.extend(observed)
        return observed


class _WebSocketForwarder(socketserver.BaseRequestHandler):
    def handle(self) -> None:
        server = self.server
        try:
            upstream = socket.create_connection(server.upstream, timeout=5.0)
        except OSError as error:
            server.errors.append(str(error))
            return
        server.record_connection()
        with upstream:
            endpoints = (self.request, upstream)
            while True:
                readable, _, _ = select.select(endpoints, (), (), 0.25)
                for source in readable:
                    try:
                        payload = source.recv(65536)
                    except OSError as error:
                        server.errors.append(str(error))
                        return
                    if not payload:
                        return
                    from_client = source is self.request
                    server.inspect_frame_bytes(from_client, payload)
                    destination = upstream if from_client else self.request
                    try:
                        destination.sendall(payload)
                    except OSError as error:
                        server.errors.append(str(error))
                        return


class _WebSocketFrameProxy(socketserver.ThreadingTCPServer):
    allow_reuse_address = True
    daemon_threads = True

    def __init__(self, upstream: tuple[str, int]) -> None:
        super().__init__(("127.0.0.1", 0), _WebSocketForwarder)
        self.upstream = upstream
        self.client_frames = _FrameInspector()
        self.server_frames = _FrameInspector()
        self.connection_count = 0
        self.errors: list[str] = []
        self._lock = threading.Lock()

    def record_connection(self) -> None:
        with self._lock:
            self.connection_count += 1

    def inspect_frame_bytes(self, from_client: bool, payload: bytes) -> None:
        with self._lock:
            inspector = self.client_frames if from_client else self.server_frames
            inspector.feed(payload)

    def frame_snapshot(
        self,
    ) -> tuple[list[dict[str, object]], list[dict[str, object]]]:
        with self._lock:
            return list(self.client_frames.frames), list(self.server_frames.frames)


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tck-executable", type=Path, required=True)
    parser.add_argument("--process-fixture", type=Path, required=True)
    parser.add_argument("--fom-path", type=Path, required=True)
    parser.add_argument("--mim-fom", type=Path, required=True)
    parser.add_argument("--time-implementation", required=True)
    parser.add_argument(
        "--callback-model", choices=("evoked", "immediate"), required=True
    )
    parser.add_argument("--provider-id", default="installed-package")
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
            raise RuntimeError("WebSocket process fixture exited before publishing its port")
        time.sleep(0.025)
    raise RuntimeError("WebSocket process fixture did not publish a listening port")


def wait_for_marker(
    directory: Path,
    marker_name: str,
    expected_value: str,
    timeout_s: float,
    fixture: subprocess.Popen[bytes],
) -> None:
    marker_path = directory / marker_name
    deadline = time.monotonic() + timeout_s
    while time.monotonic() < deadline:
        try:
            value = marker_path.read_text(encoding="ascii").strip()
        except (FileNotFoundError, OSError):
            value = ""
        if value == expected_value:
            return
        if fixture.poll() is not None:
            raise RuntimeError(
                f"WebSocket fixture exited before publishing {marker_name}"
            )
        time.sleep(0.025)
    raise RuntimeError(f"WebSocket fixture did not publish {marker_name}")


def receive_exact(connection: socket.socket, count: int) -> bytes:
    payload = bytearray()
    while len(payload) < count:
        chunk = connection.recv(count - len(payload))
        if not chunk:
            raise RuntimeError("WebSocket peer closed before a complete frame arrived")
        payload.extend(chunk)
    return bytes(payload)


def receive_frame(connection: socket.socket) -> tuple[int, bytes]:
    first, second = receive_exact(connection, 2)
    opcode = first & 0x0F
    payload_length = second & 0x7F
    if payload_length == 126:
        payload_length = int.from_bytes(receive_exact(connection, 2), "big")
    elif payload_length == 127:
        payload_length = int.from_bytes(receive_exact(connection, 8), "big")
    if second & 0x80:
        mask = receive_exact(connection, 4)
    else:
        mask = None
    payload = receive_exact(connection, payload_length)
    if mask is not None:
        payload = bytes(value ^ mask[index % 4] for index, value in enumerate(payload))
    return opcode, payload


def inject_text_frame_and_get_close_status(port: int, timeout_s: float) -> int:
    key = base64.b64encode(os.urandom(16)).decode("ascii")
    with socket.create_connection(("127.0.0.1", port), timeout=timeout_s) as connection:
        connection.settimeout(timeout_s)
        request = (
            "GET /hla HTTP/1.1\r\n"
            f"Host: 127.0.0.1:{port}\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Version: 13\r\n"
            f"Sec-WebSocket-Key: {key}\r\n\r\n"
        ).encode("ascii")
        connection.sendall(request)
        response = bytearray()
        while HTTP_HEADER_END not in response:
            chunk = connection.recv(4096)
            if not chunk:
                raise RuntimeError("WebSocket host closed during the upgrade handshake")
            response.extend(chunk)
        if not response.startswith(b"HTTP/1.1 101 "):
            raise RuntimeError("WebSocket host did not accept the negative-control upgrade")

        text = b"HLA text frames are forbidden"
        mask = os.urandom(4)
        if len(text) < 126:
            frame = bytes((0x81, 0x80 | len(text)))
        else:
            raise AssertionError("negative-control payload unexpectedly needs extended length")
        frame += mask + bytes(value ^ mask[index % 4] for index, value in enumerate(text))
        connection.sendall(frame)
        opcode, payload = receive_frame(connection)
        if opcode != 0x8 or len(payload) < 2:
            raise RuntimeError("Text-frame negative control did not receive a close frame")
        return int.from_bytes(payload[:2], "big")


def run() -> int:
    arguments = parse_arguments()
    arguments.results.parent.mkdir(parents=True, exist_ok=True)
    arguments.junit.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(
        prefix="cpp-tck-websocket-binary-frames-",
        dir=arguments.results.parent,
    ) as temporary_directory:
        directory = Path(temporary_directory)
        model_results = directory / "results.json"
        model_junit = directory / "results.xml"
        fixture: subprocess.Popen[bytes] | None = None
        proxy: _WebSocketFrameProxy | None = None
        proxy_thread: threading.Thread | None = None
        try:
            fixture = subprocess.Popen(
                [
                    str(arguments.process_fixture),
                    "public-server-websocket-transport",
                    str(directory),
                ]
            )
            upstream_port = wait_for_port(directory, arguments.timeout_s, fixture)
            proxy = _WebSocketFrameProxy(("127.0.0.1", upstream_port))
            proxy_thread = threading.Thread(
                target=proxy.serve_forever,
                name="cpp-tck-websocket-frame-inspector",
                daemon=True,
            )
            proxy_thread.start()
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
                f"ws://127.0.0.1:{proxy.server_address[1]}/hla",
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

            client_frames, server_frames = proxy.frame_snapshot()
            client_data_frames = [
                frame for frame in client_frames if int(frame["opcode"]) < 8
            ]
            server_data_frames = [
                frame for frame in server_frames if int(frame["opcode"]) < 8
            ]
            client_data_opcodes = [
                int(frame["opcode"])
                for frame in client_data_frames
            ]
            server_data_opcodes = [
                int(frame["opcode"])
                for frame in server_data_frames
            ]
            if proxy.connection_count != 1:
                raise RuntimeError(
                    "The WebSocket TCK exchange used "
                    f"{proxy.connection_count} client connections instead of one"
                )
            if proxy.errors:
                raise RuntimeError("WebSocket frame proxy failed: " + proxy.errors[0])
            if not client_data_opcodes or not server_data_opcodes:
                raise RuntimeError("The TCK exchange did not carry data frames both ways")
            if any(opcode not in (0, 1, 2) for opcode in client_data_opcodes + server_data_opcodes):
                raise RuntimeError("A reserved WebSocket data opcode appeared in the TCK exchange")
            if 1 in client_data_opcodes or 1 in server_data_opcodes:
                raise RuntimeError("A text data frame appeared in the TCK exchange")
            if 2 not in client_data_opcodes or 2 not in server_data_opcodes:
                raise RuntimeError("Binary data frames were not observed in both directions")
            if any(not bool(frame["masked"]) for frame in client_frames):
                raise RuntimeError("The WebSocket client sent an unmasked frame")
            if any(bool(frame["masked"]) for frame in server_frames):
                raise RuntimeError("The WebSocket server sent a masked frame")

            (directory / "websocket-tck-complete.ok").write_text(
                "complete\n", encoding="ascii"
            )
            wait_for_marker(
                directory,
                "websocket-negative-ready.ok",
                "ready",
                arguments.timeout_s,
                fixture,
            )
            close_status = inject_text_frame_and_get_close_status(
                upstream_port, arguments.timeout_s
            )
            if close_status != 1003:
                raise RuntimeError(
                    "The WebSocket host did not reject a text data frame with close "
                    f"status 1003 (received {close_status})"
                )
            (directory / "websocket-transport-complete.ok").write_text(
                "complete\n", encoding="ascii"
            )
            if fixture.wait(timeout=arguments.timeout_s) != 0:
                raise RuntimeError("The WebSocket process fixture failed")

            evidence = json.loads(model_results.read_text(encoding="utf-8"))
            for result in evidence.get("results", []):
                if result.get("id") == SCENARIO:
                    result["adapter_evidence"] = {
                        "transport": "ws://127.0.0.1:<ephemeral-port>/hla",
                        "client_to_rti_data_frame_opcodes": client_data_opcodes,
                        "rti_to_client_data_frame_opcodes": server_data_opcodes,
                        "client_frames_masked": all(
                            bool(frame["masked"]) for frame in client_frames
                        ),
                        "rti_frames_unmasked": all(
                            not bool(frame["masked"]) for frame in server_frames
                        ),
                        "client_to_rti_text_frame_count": client_data_opcodes.count(1),
                        "rti_to_client_text_frame_count": server_data_opcodes.count(1),
                        "injected_text_frame_close_status": close_status,
                    }
            arguments.results.write_text(
                json.dumps(evidence, indent=2, sort_keys=True) + "\n",
                encoding="utf-8",
            )
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
