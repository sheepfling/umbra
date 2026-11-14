"""Run the Java WebSocket TCK against the process fixture and frame observer."""

from __future__ import annotations

import argparse
import importlib.util
import json
import os
import subprocess
import sys
import threading
import time
import xml.etree.ElementTree as ET
from pathlib import Path
from types import ModuleType

ROOT = Path(__file__).resolve().parents[1]
JAVA_RUNNER = ROOT / "tools" / "run_java_tck.py"
ADAPTER_PATH = (
    ROOT
    / "packages"
    / "hla-rti-cpp-tck"
    / "adapters"
    / "current-process"
    / "run_websocket_binary_frames.py"
)
SCENARIO = "java-tck.websocket-transport-support"
CALLBACK_MODELS = ("HLA_EVOKED", "HLA_IMMEDIATE")
DEFAULT_FOM = ROOT / "packages" / "hla-rti-java-tck" / "fom" / "p0-tck-without-switches.xml"
DEFAULT_MIM = (
    ROOT
    / "third_party"
    / "ieee1516.2-2025"
    / "resources"
    / "mim"
    / "HLAstandardMIM-2025.xml"
)


def _load_adapter() -> ModuleType:
    specification = importlib.util.spec_from_file_location(
        "cpp_websocket_binary_frame_adapter", ADAPTER_PATH
    )
    if specification is None or specification.loader is None:
        raise RuntimeError(f"could not load WebSocket frame adapter {ADAPTER_PATH}")
    module = importlib.util.module_from_spec(specification)
    specification.loader.exec_module(module)
    return module


ADAPTER = _load_adapter()


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
    parser.add_argument("--process-fixture", required=True, help="WebSocket process fixture")
    parser.add_argument("--fom-path", default=str(DEFAULT_FOM))
    parser.add_argument("--mim-path", default=str(DEFAULT_MIM))
    parser.add_argument("--provider-id", required=True)
    parser.add_argument("--output-directory", required=True)
    parser.add_argument("--classes-directory")
    parser.add_argument("--java")
    parser.add_argument("--javac")
    parser.add_argument("--jni-library-path")
    parser.add_argument("--prepend-path", action="append", default=[])
    parser.add_argument("--jvm-argument", action="append", default=[])
    parser.add_argument("--timeout-s", type=float, default=60.0)
    parser.add_argument(
        "--callback-model", choices=("both", "evoked", "immediate"), default="both"
    )
    return parser.parse_args()


def _build_tck(arguments: argparse.Namespace, classes_directory: Path) -> None:
    command = [
        sys.executable,
        str(JAVA_RUNNER),
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


def _java_command(
    arguments: argparse.Namespace,
    callback_model: str,
    rti_address: str,
    classes_directory: Path,
    model_directory: Path,
) -> list[str]:
    command = [
        sys.executable,
        str(JAVA_RUNNER),
        "run",
        "--api-jar",
        str(_root_path(arguments.api_jar)),
        "--fom-path",
        str(_root_path(arguments.fom_path)),
        "--mim-path",
        str(_root_path(arguments.mim_path)),
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
    if arguments.java:
        command.extend(["--java", arguments.java])
    if arguments.jni_library_path:
        command.append(
            "--jvm-argument=-Djava.library.path="
            + str(_root_path(arguments.jni_library_path))
        )
    command.extend(
        [
            "--jvm-argument=-Dhla.rti.tck.callbackModel=" + callback_model,
            "--jvm-argument=-Dhla.rti.tck.rtiAddress=" + rti_address,
            "--jvm-argument=-Dhla.rti.tck.webSocketTransportTimeoutMillis="
            + str(int(arguments.timeout_s * 1000)),
        ]
    )
    command.extend("--jvm-argument=" + value for value in arguments.jvm_argument)
    return command


def _terminate(process: subprocess.Popen[bytes]) -> None:
    if process.poll() is not None:
        return
    process.terminate()
    try:
        process.wait(timeout=5)
    except subprocess.TimeoutExpired:
        process.kill()
        process.wait()


def _run_model(
    arguments: argparse.Namespace,
    callback_model: str,
    classes_directory: Path,
    environment: dict[str, str],
) -> dict[str, object]:
    model_directory = _root_path(arguments.output_directory) / callback_model.lower()
    model_directory.mkdir(parents=True, exist_ok=True)
    stale_names = (
        "port.txt",
        "websocket-tck-complete.ok",
        "websocket-negative-ready.ok",
        "websocket-transport-complete.ok",
    )
    stale = [name for name in stale_names if (model_directory / name).exists()]
    if stale:
        raise RuntimeError("refusing stale WebSocket fixture markers: " + ", ".join(stale))
    for name in ("results.json", "results.xml", "fault-evidence.json"):
        (model_directory / name).unlink(missing_ok=True)

    fixture_path = _root_path(arguments.process_fixture)
    fixture = subprocess.Popen(
        [str(fixture_path), "public-server-websocket-transport", str(model_directory)],
        cwd=ROOT,
        env=environment,
    )
    proxy = None
    proxy_thread = None
    try:
        upstream_port = ADAPTER.wait_for_port(
            model_directory, arguments.timeout_s, fixture
        )
        proxy = ADAPTER._WebSocketFrameProxy(("127.0.0.1", upstream_port))
        proxy_thread = threading.Thread(
            target=proxy.serve_forever,
            name="java-tck-websocket-frame-inspector",
            daemon=True,
        )
        proxy_thread.start()
        rti_address = f"ws://127.0.0.1:{proxy.server_address[1]}/hla"
        command = _java_command(
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
            raise TimeoutError(f"Java WebSocket TCK exceeded {arguments.timeout_s:g} seconds") from error
        if completed.returncode != 0:
            raise RuntimeError(f"Java WebSocket TCK exited with {completed.returncode}")

        client_frames, server_frames = proxy.frame_snapshot()
        client_data = [frame for frame in client_frames if int(frame["opcode"]) < 8]
        server_data = [frame for frame in server_frames if int(frame["opcode"]) < 8]
        client_opcodes = [int(frame["opcode"]) for frame in client_data]
        server_opcodes = [int(frame["opcode"]) for frame in server_data]
        client_close_seen = any(int(frame["opcode"]) == 8 for frame in client_frames)
        server_close_seen = any(int(frame["opcode"]) == 8 for frame in server_frames)
        post_close_socket_abort = (
            client_close_seen
            and server_close_seen
            and bool(proxy.errors)
            and all("[WinError 10053]" in error for error in proxy.errors)
        )
        if proxy.connection_count != 1:
            raise RuntimeError(
                f"Java WebSocket TCK used {proxy.connection_count} client connections instead of one"
            )
        if proxy.errors and not post_close_socket_abort:
            raise RuntimeError(
                "WebSocket frame proxy failed: " + proxy.errors[0]
                + f"; client_frames={client_frames}; server_frames={server_frames}"
            )
        if not client_opcodes or not server_opcodes:
            raise RuntimeError("the Java exchange did not carry data frames in both directions")
        if any(opcode not in (0, 1, 2) for opcode in client_opcodes + server_opcodes):
            raise RuntimeError("a reserved WebSocket data opcode appeared in the Java exchange")
        if 1 in client_opcodes or 1 in server_opcodes:
            raise RuntimeError("a text WebSocket data frame appeared in the Java exchange")
        if 2 not in client_opcodes or 2 not in server_opcodes:
            raise RuntimeError("binary WebSocket data frames were not seen in both directions")
        if any(not bool(frame["masked"]) for frame in client_frames):
            raise RuntimeError("the Java WebSocket client sent an unmasked frame")
        if any(bool(frame["masked"]) for frame in server_frames):
            raise RuntimeError("the WebSocket server sent a masked frame")

        (model_directory / "websocket-tck-complete.ok").write_text(
            "complete\n", encoding="ascii"
        )
        ADAPTER.wait_for_marker(
            model_directory,
            "websocket-negative-ready.ok",
            "ready",
            arguments.timeout_s,
            fixture,
        )
        close_status = ADAPTER.inject_text_frame_and_get_close_status(
            upstream_port, arguments.timeout_s
        )
        if close_status != 1003:
            raise RuntimeError(
                f"WebSocket host returned close status {close_status}, expected 1003"
            )
        (model_directory / "websocket-transport-complete.ok").write_text(
            "complete\n", encoding="ascii"
        )
        if fixture.wait(timeout=arguments.timeout_s) != 0:
            raise RuntimeError("the WebSocket process fixture failed")

        java_results = json.loads(
            (model_directory / "results.json").read_text(encoding="utf-8")
        )
        fault_evidence: dict[str, object] = {
            "scenario": SCENARIO,
            "callback_model": callback_model,
            "transport": "ws://127.0.0.1:<ephemeral-port>/hla",
            "client_to_rti_data_frame_opcodes": client_opcodes,
            "rti_to_client_data_frame_opcodes": server_opcodes,
            "client_frames_masked": all(bool(frame["masked"]) for frame in client_frames),
            "rti_frames_unmasked": all(not bool(frame["masked"]) for frame in server_frames),
            "client_to_rti_text_frame_count": client_opcodes.count(1),
            "rti_to_client_text_frame_count": server_opcodes.count(1),
            "client_connection_count": proxy.connection_count,
            "client_and_server_close_frames_seen": client_close_seen and server_close_seen,
            "post_close_client_socket_abort": post_close_socket_abort,
            "proxy_errors_after_close_frames": list(proxy.errors) if post_close_socket_abort else [],
            "injected_text_frame_close_status": close_status,
        }
        (model_directory / "fault-evidence.json").write_text(
            json.dumps(fault_evidence, indent=2) + "\n", encoding="utf-8"
        )
        return {"java_results": java_results, "fault_evidence": fault_evidence}
    finally:
        if proxy is not None:
            proxy.shutdown()
            proxy.server_close()
        if proxy_thread is not None:
            proxy_thread.join(timeout=5)
        _terminate(fixture)


def main() -> int:
    arguments = parse_arguments()
    if arguments.timeout_s <= 0:
        raise ValueError("timeout must be positive")
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

    models = (
        CALLBACK_MODELS
        if arguments.callback_model == "both"
        else ("HLA_EVOKED",)
        if arguments.callback_model == "evoked"
        else ("HLA_IMMEDIATE",)
    )
    results: dict[str, object] = {}
    junit_cases: list[ET.Element] = []
    for model in models:
        results[model] = _run_model(arguments, model, classes_directory, environment)
        junit_path = output_directory / model.lower() / "results.xml"
        junit_cases.extend(list(ET.parse(junit_path).getroot()))

    aggregate = {
        "kind": "hla-rti-java-tck-process-adapter-evidence",
        "standard": "IEEE 1516.1-2025",
        "scenario": SCENARIO,
        "callback_models": list(models),
        "results": results,
    }
    (output_directory / "results.json").write_text(
        json.dumps(aggregate, indent=2) + "\n", encoding="utf-8"
    )
    suite = ET.Element("testsuite", tests=str(len(junit_cases)))
    for case in junit_cases:
        suite.append(case)
    ET.ElementTree(suite).write(
        output_directory / "results.xml", encoding="utf-8", xml_declaration=True
    )
    print(f"{SCENARIO}: passed in {', '.join(models)} callback model(s)")
    print(f"Evidence: {output_directory}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, TimeoutError, ValueError, subprocess.SubprocessError) as error:
        print(f"Java WebSocket transport runner failed: {error}", file=sys.stderr)
        raise SystemExit(1)
