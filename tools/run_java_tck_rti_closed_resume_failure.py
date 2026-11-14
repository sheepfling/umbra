"""Run the Java RTI-closed resume-failure scenario through the process fault proxy."""

from __future__ import annotations

import argparse
import importlib.util
import json
import os
import subprocess
import sys
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
    / "run_message_history_recovery.py"
)
JAVA_SCENARIO = "java-tck.connection-fault-rti-closed-resume-fails"
CPP_SCENARIO = "cpp-tck.connection-fault-rti-closed-resume-fails"
CALLBACK_MODELS = ("HLA_EVOKED", "HLA_IMMEDIATE")
REQUIRED_SETTINGS = "FED_INT_HEART=0.02;FED_TIMEOUT_RECONNECT=0.25"
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
    adapter_directory = str(ADAPTER_PATH.parent)
    if adapter_directory not in sys.path:
        sys.path.insert(0, adapter_directory)
    specification = importlib.util.spec_from_file_location(
        "cpp_message_history_recovery_adapter", ADAPTER_PATH
    )
    if specification is None or specification.loader is None:
        raise RuntimeError(f"could not load message-history adapter {ADAPTER_PATH}")
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
    parser.add_argument("--process-fixture", required=True, help="message-history process fixture")
    parser.add_argument("--fom-path", default=str(DEFAULT_FOM))
    parser.add_argument("--mim-path", default=str(DEFAULT_MIM))
    parser.add_argument("--provider-id", required=True)
    parser.add_argument("--time-implementation", default="HLAinteger64Time")
    parser.add_argument("--output-directory", required=True)
    parser.add_argument("--classes-directory")
    parser.add_argument("--java")
    parser.add_argument("--javac")
    parser.add_argument("--jni-library-path")
    parser.add_argument("--prepend-path", action="append", default=[])
    parser.add_argument("--jvm-argument", action="append", default=[])
    parser.add_argument("--timeout-s", type=float, default=60.0)
    parser.add_argument("--callback-model", choices=("both", "evoked", "immediate"), default="both")
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
    upstream_address: str,
    member_address: str,
    classes_directory: Path,
    model_directory: Path,
) -> list[str]:
    trigger = model_directory / "resume-failure-trigger.ok"
    completion = model_directory / "message-history-recovery-complete.ok"
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
        "--time-implementation",
        arguments.time_implementation,
        "--provider-id",
        arguments.provider_id,
        "--scenario",
        JAVA_SCENARIO,
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
    properties = [
        ("callbackModel", callback_model),
        ("ownerRtiAddress", upstream_address),
        ("memberRtiAddress", member_address),
        ("additionalSettings", REQUIRED_SETTINGS),
        ("messageHistoryResumeFailureTriggerMarker", str(trigger)),
        ("messageHistoryResumeFailureCompletionMarker", str(completion)),
        ("messageHistoryResumeFailureTimeoutMillis", str(int(arguments.timeout_s * 1000))),
    ]
    command.extend("--jvm-argument=-Dhla.rti.tck." + key + "=" + value for key, value in properties)
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


def _wait_for_marker(marker: Path, fixture: subprocess.Popen[bytes], deadline: float) -> None:
    while time.monotonic() < deadline:
        if marker.is_file():
            return
        fixture_status = fixture.poll()
        if fixture_status is not None:
            raise RuntimeError(
                f"process fixture exited with code {fixture_status} before {marker.name}"
            )
        time.sleep(0.01)
    raise TimeoutError(f"timed out waiting for process fixture marker {marker.name}")


def _run_model(
    arguments: argparse.Namespace,
    callback_model: str,
    classes_directory: Path,
    environment: dict[str, str],
) -> dict[str, object]:
    model_directory = _root_path(arguments.output_directory) / callback_model.lower()
    model_directory.mkdir(parents=True, exist_ok=True)
    trigger = model_directory / "resume-failure-trigger.ok"
    completion = model_directory / "message-history-recovery-complete.ok"
    stale = [
        path.name
        for path in (model_directory / "port.txt", trigger, completion)
        if path.exists()
    ]
    if stale:
        raise RuntimeError(
            "refusing stale process-fixture markers " + ", ".join(stale)
        )
    for name in ("results.json", "results.xml", "fault-evidence.json"):
        (model_directory / name).unlink(missing_ok=True)

    fixture = subprocess.Popen(
        [
            str(_root_path(arguments.process_fixture)),
            "public-server-message-history",
            str(model_directory),
        ],
        cwd=ROOT,
        env=environment,
    )
    proxy = None
    try:
        deadline = time.monotonic() + arguments.timeout_s
        port_path = model_directory / "port.txt"
        _wait_for_marker(port_path, fixture, deadline)
        upstream_port = int(port_path.read_text(encoding="utf-8").strip())
        if not 0 < upstream_port <= 65535:
            raise RuntimeError(f"process fixture published invalid TCP port {upstream_port}")
        proxy = ADAPTER.RawFrameFaultProxy(
            upstream_port, CPP_SCENARIO, trigger
        )
        command = _java_command(
            arguments,
            callback_model,
            f"tcp://127.0.0.1:{upstream_port}",
            f"tcp://127.0.0.1:{proxy.port}",
            classes_directory,
            model_directory,
        )
        try:
            completed = subprocess.run(
                command,
                cwd=ROOT,
                env=environment,
                check=False,
                timeout=max(0.1, deadline - time.monotonic()),
            )
        except subprocess.TimeoutExpired as error:
            raise TimeoutError(f"Java TCK exceeded {arguments.timeout_s:g} seconds") from error
        if completed.returncode != 0:
            raise RuntimeError(f"Java TCK exited with {completed.returncode}")
        if not completion.is_file():
            raise RuntimeError("Java TCK did not confirm the public failed-resume assertions")

        fault_evidence = proxy.evidence()
        if fault_evidence.get("resume_status_count") != 0:
            raise RuntimeError(
                "the RTI sent Resume Status after closing the original federate socket; "
                f"evidence={fault_evidence}"
            )
        fault_evidence.update(
            {
                "scenario": JAVA_SCENARIO,
                "callback_model": callback_model,
                "resume_status_absent": True,
                "public_connection_lost_and_not_connected_assertions_passed": True,
            }
        )
        (model_directory / "fault-evidence.json").write_text(
            json.dumps(fault_evidence, indent=2) + "\n", encoding="utf-8"
        )
        java_results_path = model_directory / "results.json"
        java_results = json.loads(java_results_path.read_text(encoding="utf-8"))
        return {"java_results": java_results, "fault_evidence": fault_evidence}
    finally:
        if proxy is not None:
            proxy.close()
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
        root = ET.parse(junit_path).getroot()
        junit_cases.extend(list(root))

    aggregate = {
        "kind": "hla-rti-java-tck-process-adapter-evidence",
        "standard": "IEEE 1516.1-2025",
        "scenario": JAVA_SCENARIO,
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
    print(f"{JAVA_SCENARIO}: passed in {', '.join(models)} callback model(s)")
    print(f"Evidence: {output_directory}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, TimeoutError, ValueError, subprocess.SubprocessError) as error:
        print(f"Java RTI-closed resume-failure runner failed: {error}", file=sys.stderr)
        raise SystemExit(1)
