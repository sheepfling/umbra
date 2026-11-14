"""Run the focused C++/Java IEEE 1516.1-2025 language-transparency TCK case."""

from __future__ import annotations

import argparse
import json
import os
import socket
import subprocess
import sys
import tempfile
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SCENARIO = "cpp-tck.language-api-transparency"
JAVA_MAIN = "org.hla.rti.tck.LanguageTransparencyPeer"
JAVA_SOURCE = (
    ROOT
    / "packages/hla-rti-java-tck/src/main/java/org/hla/rti/tck/LanguageTransparencyPeer.java"
)


def _absolute(path: str, label: str) -> Path:
    candidate = Path(path).expanduser()
    if not candidate.is_absolute():
        candidate = ROOT / candidate
    result = candidate.resolve()
    if not result.exists():
        raise FileNotFoundError(f"{label} does not exist: {result}")
    return result


def _reserve_loopback_port() -> int:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as listener:
        listener.bind(("127.0.0.1", 0))
        return int(listener.getsockname()[1])


def _wait_for_marker(
    marker: Path,
    java_peer: subprocess.Popen[bytes],
    coordinator: subprocess.Popen[bytes],
    timeout_seconds: float,
) -> None:
    deadline = time.monotonic() + timeout_seconds
    while time.monotonic() < deadline:
        if marker.exists():
            return
        if java_peer.poll() is not None:
            raise RuntimeError(
                f"Java peer exited before becoming ready (exit {java_peer.returncode})"
            )
        if coordinator.poll() is not None:
            raise RuntimeError(
                "process coordinator exited before the Java peer became ready "
                f"(exit {coordinator.returncode})"
            )
        time.sleep(0.05)
    raise TimeoutError("Java API peer did not become ready within the startup bound")


def _run(command: list[str], *, cwd: Path, env: dict[str, str] | None = None) -> None:
    print("+ " + subprocess.list2cmdline(command), flush=True)
    subprocess.run(command, cwd=cwd, env=env, check=True)


def _runtime_environment(
    package_prefix: Path, process_service: Path, jni_library: Path
) -> dict[str, str]:
    environment = dict(os.environ)
    runtime_directories = [
        package_prefix / "bin",
        process_service.parent,
        jni_library.parent,
    ]
    existing_path = environment.get("PATH", "")
    environment["PATH"] = os.pathsep.join(
        [*(str(directory) for directory in runtime_directories), existing_path]
    )
    return environment


def _parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--package-prefix", required=True)
    parser.add_argument("--build-directory", default=".build/cpp-tck-python")
    parser.add_argument("--process-service", required=True)
    parser.add_argument("--java-api-jar", required=True)
    parser.add_argument("--java-provider-jar", required=True)
    parser.add_argument("--jni-library", required=True)
    parser.add_argument("--java", default="java")
    parser.add_argument("--javac", default="javac")
    parser.add_argument("--cmake", default="cmake")
    parser.add_argument("--build-parallel", type=int, default=max(1, os.cpu_count() or 1))
    parser.add_argument(
        "--output-directory", default=".build/cpp-tck-language-transparency"
    )
    parser.add_argument("--timeout-seconds", type=float, default=45.0)
    return parser


def main() -> int:
    arguments = _parser().parse_args()
    package_prefix = _absolute(arguments.package_prefix, "installed package prefix")
    process_service = _absolute(arguments.process_service, "process-service executable")
    api_jar = _absolute(arguments.java_api_jar, "official Java API JAR")
    provider_jar = _absolute(arguments.java_provider_jar, "Java provider JAR")
    jni_library = _absolute(arguments.jni_library, "JNI library")
    runtime_environment = _runtime_environment(
        package_prefix, process_service, jni_library
    )
    build_directory = (ROOT / arguments.build_directory).resolve()
    output_directory = (ROOT / arguments.output_directory).resolve()
    output_directory.mkdir(parents=True, exist_ok=True)
    results = output_directory / "language-transparency-results.json"
    junit = output_directory / "language-transparency-junit.xml"

    _run(
        [
            arguments.cmake,
            "--build",
            str(build_directory),
            "--config",
            "Debug",
            "--target",
            "hla_rti_cpp_tck",
            "--parallel",
            str(max(1, arguments.build_parallel)),
        ],
        cwd=ROOT,
    )

    with tempfile.TemporaryDirectory(
        prefix="java-language-peer-", dir=output_directory
    ) as temporary:
        temporary_root = Path(temporary)
        classes = temporary_root / "classes"
        classes.mkdir()
        service_reports = temporary_root / "reports"
        service_reports.mkdir()
        _run(
            [
                arguments.javac,
                "-encoding",
                "UTF-8",
                "-cp",
                str(api_jar),
                "-d",
                str(classes),
                str(JAVA_SOURCE),
            ],
            cwd=ROOT,
        )

        port = _reserve_loopback_port()
        address = f"tcp://127.0.0.1:{port}"
        federation = f"UmbraCppTckLanguageTransparency{time.time_ns()}"
        ready = temporary_root / "java-peer-ready"
        stop = temporary_root / "stop-java-peer"
        java_result = temporary_root / "java-peer-result.txt"
        coordinator_log = (output_directory / "coordinator.log").open(
            "w", encoding="utf-8"
        )
        java_log = (output_directory / "java-peer.log").open("w", encoding="utf-8")
        creationflags = getattr(subprocess, "CREATE_NO_WINDOW", 0)
        coordinator: subprocess.Popen[bytes] | None = None
        java_peer: subprocess.Popen[bytes] | None = None
        try:
            coordinator = subprocess.Popen(
                [
                    str(process_service),
                    "127.0.0.1",
                    str(port),
                    str(service_reports),
                ],
                cwd=ROOT,
                stdout=coordinator_log,
                stderr=subprocess.STDOUT,
                env=runtime_environment,
                creationflags=creationflags,
            )
            java_classpath = os.pathsep.join(
                (str(classes), str(api_jar), str(provider_jar))
            )
            java_peer = subprocess.Popen(
                [
                    arguments.java,
                    f"-Dumbra.rti.jni.library={jni_library}",
                    "-cp",
                    java_classpath,
                    JAVA_MAIN,
                    address,
                    federation,
                    str(ROOT / "packages/hla-rti-cpp-tck/fom/p0-tck.xml"),
                    "HLAinteger64Time",
                    str(ready),
                    str(stop),
                    str(java_result),
                ],
                cwd=ROOT,
                stdout=java_log,
                stderr=subprocess.STDOUT,
                env=runtime_environment,
                creationflags=creationflags,
            )
            _wait_for_marker(ready, java_peer, coordinator, arguments.timeout_seconds)

            _run(
                [
                    sys.executable,
                    str(ROOT / "tools/run_cpp_tck.py"),
                    "--package-prefix",
                    str(package_prefix),
                    "--build-directory",
                    str(build_directory),
                    "--scenario",
                    SCENARIO,
                    "--callback-model",
                    "both",
                    "--rti-address",
                    address,
                    "--federation-name",
                    federation,
                    "--service-report-directory",
                    str(service_reports),
                    "--results",
                    str(results),
                    "--junit",
                    str(junit),
                    "--skip-configure",
                    "--skip-build",
                    "--skip-ctest",
                ],
                cwd=ROOT,
                env=runtime_environment,
            )
            stop.write_text("stop\n", encoding="utf-8")
            try:
                java_exit = java_peer.wait(timeout=arguments.timeout_seconds)
            except subprocess.TimeoutExpired as error:
                raise TimeoutError("Java API peer did not finish within the bound") from error
            if java_exit != 0:
                raise RuntimeError(f"Java API peer failed with exit {java_exit}")
            if not java_result.exists() or java_result.read_text(encoding="utf-8").strip() != "received=2":
                raise RuntimeError("Java API peer did not validate both C++ callback-model messages")

            evidence = json.loads(results.read_text(encoding="utf-8"))
            evidence["mixed_language_peer"] = {
                "api_profile": "IEEE 1516.1-2025 Java API",
                "federate_name": "java-tck-language-peer",
                "received_cpp_messages": 2,
                "callback_models": ["HLA_EVOKED", "HLA_IMMEDIATE"],
                "result": "passed",
            }
            results.write_text(
                json.dumps(evidence, indent=2, sort_keys=True) + "\n",
                encoding="utf-8",
            )
            print(f"PASS: {SCENARIO} in both callback models")
            print(f"C++ evidence: {results}")
            print(f"JUnit: {junit}")
            return 0
        finally:
            stop.write_text("stop\n", encoding="utf-8")
            if java_peer is not None and java_peer.poll() is None:
                try:
                    java_peer.wait(timeout=5.0)
                except subprocess.TimeoutExpired:
                    java_peer.terminate()
                    java_peer.wait(timeout=5.0)
            if coordinator is not None and coordinator.poll() is None:
                coordinator.terminate()
                try:
                    coordinator.wait(timeout=5.0)
                except subprocess.TimeoutExpired:
                    coordinator.kill()
                    coordinator.wait(timeout=5.0)
            coordinator_log.close()
            java_log.close()


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, subprocess.CalledProcessError, TimeoutError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        raise SystemExit(1)
