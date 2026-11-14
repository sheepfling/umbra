#!/usr/bin/env python3
"""Run Java public-service message-history replay cases through the process fixture.

The fixture supplies a process federation service and the shared raw-frame proxy
drops a selected public-service request or accepted response at the socket boundary.
The Java TCK sees only the official API, provider/API JARs, FOM/MIM, callback
model, and endpoint.
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
import threading
import tempfile
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
CPP_ADAPTER = ROOT / "packages" / "hla-rti-cpp-tck" / "adapters" / "current-process"
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(CPP_ADAPTER))
import run_java_tck
import run_message_history_recovery as message_history_recovery

SCENARIO = "java-tck.connection-fault-federate-request-replay"
JOIN_RESPONSE_SCENARIO = "java-tck.connection-fault-join-response-replay"
REGISTRATION_RESPONSE_SCENARIO = (
    "java-tck.connection-fault-registration-response-replay"
)
CREATE_FEDERATION_RESPONSE_SCENARIO = (
    "java-tck.connection-fault-create-federation-response-replay"
)
RESERVATION_RESPONSE_SCENARIO = (
    "java-tck.connection-fault-reservation-response-replay"
)
DELETE_OBJECT_RESPONSE_SCENARIO = (
    "java-tck.connection-fault-delete-object-response-replay"
)
ATTRIBUTE_UPDATE_RESPONSE_SCENARIO = (
    "java-tck.connection-fault-attribute-update-response-replay"
)
SEND_INTERACTION_RESPONSE_SCENARIO = (
    "java-tck.connection-fault-send-interaction-response-replay"
)
TIME_REGULATION_RESPONSE_SCENARIO = (
    "java-tck.connection-fault-time-regulation-response-replay"
)
TIME_CONSTRAINED_RESPONSE_SCENARIO = (
    "java-tck.connection-fault-time-constrained-response-replay"
)
TIME_ADVANCE_REQUEST_RESPONSE_SCENARIO = (
    "java-tck.connection-fault-time-advance-request-response-replay"
)
TIME_ADVANCE_REQUEST_AVAILABLE_RESPONSE_SCENARIO = (
    "java-tck.connection-fault-time-advance-request-available-response-replay"
)
NEXT_MESSAGE_REQUEST_RESPONSE_SCENARIO = (
    "java-tck.connection-fault-next-message-request-response-replay"
)
NEXT_MESSAGE_REQUEST_AVAILABLE_RESPONSE_SCENARIO = (
    "java-tck.connection-fault-next-message-request-available-response-replay"
)
INACTIVE_SESSION_PURGE_SCENARIO = (
    "java-tck.connection-fault-inactive-session-purge"
)
SCENARIO_TO_CPP_FAULT = {
    SCENARIO: message_history_recovery.FEDERATE_REQUEST_SCENARIO,
    JOIN_RESPONSE_SCENARIO: message_history_recovery.SCENARIO,
    REGISTRATION_RESPONSE_SCENARIO: (
        message_history_recovery.REGISTRATION_RESPONSE_SCENARIO
    ),
    CREATE_FEDERATION_RESPONSE_SCENARIO: (
        message_history_recovery.CREATE_FEDERATION_RESPONSE_SCENARIO
    ),
    RESERVATION_RESPONSE_SCENARIO: (
        message_history_recovery.RESERVATION_RESPONSE_SCENARIO
    ),
    DELETE_OBJECT_RESPONSE_SCENARIO: (
        message_history_recovery.DELETE_OBJECT_RESPONSE_SCENARIO
    ),
    ATTRIBUTE_UPDATE_RESPONSE_SCENARIO: (
        message_history_recovery.ATTRIBUTE_UPDATE_RESPONSE_SCENARIO
    ),
    SEND_INTERACTION_RESPONSE_SCENARIO: (
        message_history_recovery.SEND_INTERACTION_RESPONSE_SCENARIO
    ),
    TIME_REGULATION_RESPONSE_SCENARIO: (
        message_history_recovery.TIME_REGULATION_RESPONSE_SCENARIO
    ),
    TIME_CONSTRAINED_RESPONSE_SCENARIO: (
        message_history_recovery.TIME_CONSTRAINED_RESPONSE_SCENARIO
    ),
    TIME_ADVANCE_REQUEST_RESPONSE_SCENARIO: (
        message_history_recovery.TIME_ADVANCE_REQUEST_RESPONSE_SCENARIO
    ),
    TIME_ADVANCE_REQUEST_AVAILABLE_RESPONSE_SCENARIO: (
        message_history_recovery.TIME_ADVANCE_REQUEST_AVAILABLE_RESPONSE_SCENARIO
    ),
    NEXT_MESSAGE_REQUEST_RESPONSE_SCENARIO: (
        message_history_recovery.NEXT_MESSAGE_REQUEST_RESPONSE_SCENARIO
    ),
    NEXT_MESSAGE_REQUEST_AVAILABLE_RESPONSE_SCENARIO: (
        message_history_recovery.NEXT_MESSAGE_REQUEST_AVAILABLE_RESPONSE_SCENARIO
    ),
    INACTIVE_SESSION_PURGE_SCENARIO: message_history_recovery.SESSION_PURGE_SCENARIO,
}


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--api-jar", type=Path, required=True)
    parser.add_argument("--provider-jar", type=Path, action="append", required=True)
    parser.add_argument("--dependency-jar", type=Path, action="append", default=[])
    parser.add_argument("--fom-path", type=Path, required=True)
    parser.add_argument("--mim-path", type=Path, required=True)
    parser.add_argument("--process-fixture", type=Path, required=True)
    parser.add_argument("--factory-name", default="")
    parser.add_argument("--api-package-prefix", default="hla.rti1516_2025")
    parser.add_argument("--time-implementation", default="HLAinteger64Time")
    parser.add_argument("--provider-id", default="java-current-process")
    parser.add_argument("--provider-jvm-argument", action="append", default=[])
    parser.add_argument(
        "--scenario", choices=tuple(SCENARIO_TO_CPP_FAULT), default=SCENARIO
    )
    parser.add_argument("--object-class", default="HLAobjectRoot.Employee.Server")
    parser.add_argument("--attribute", default="Efficiency")
    parser.add_argument("--owner-configuration-name", default="message-history-owner")
    parser.add_argument("--member-configuration-name", default="message-history-member")
    parser.add_argument(
        "--callback-model", choices=("both", "evoked", "immediate"), default="both"
    )
    parser.add_argument(
        "--classes-directory", type=Path, default=ROOT / "out" / "java-tck" / "classes"
    )
    parser.add_argument("--results-directory", type=Path, required=True)
    parser.add_argument("--java", default="java")
    parser.add_argument("--javac", default="javac")
    parser.add_argument("--skip-build", action="store_true")
    parser.add_argument("--startup-timeout-s", type=float, default=30.0)
    parser.add_argument("--fixture-timeout-s", type=float, default=30.0)
    return parser.parse_args()


def wait_for_port(directory: Path, fixture: subprocess.Popen[bytes], timeout: float) -> int:
    port_file = directory / "port.txt"
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        try:
            port = int(port_file.read_text(encoding="utf-8").strip())
        except (OSError, ValueError):
            port = 0
        if 0 < port <= 65535:
            return port
        if fixture.poll() is not None:
            raise RuntimeError(
                f"process fault fixture exited before publishing its port "
                f"(status {fixture.returncode})"
            )
        time.sleep(0.1)
    raise TimeoutError("process fault fixture did not publish a listening port")


def wait_for_fixture(fixture: subprocess.Popen[bytes], timeout: float) -> int:
    try:
        return int(fixture.wait(timeout=timeout))
    except subprocess.TimeoutExpired as error:
        fixture.terminate()
        try:
            fixture.wait(timeout=5)
        except subprocess.TimeoutExpired:
            fixture.kill()
            fixture.wait()
        raise TimeoutError("process fault fixture did not terminate") from error


def release_join_response_fault(
    directory: Path,
    fixture: subprocess.Popen[bytes],
    runner: threading.Thread,
    timeout: float,
) -> None:
    ready_marker = directory / "join-response-ready.ok"
    drop_marker = directory / "drop-join-response.ok"
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if ready_marker.is_file():
            drop_marker.write_text("drop\n", encoding="utf-8")
            return
        if fixture.poll() is not None:
            raise RuntimeError(
                "process fault fixture exited before the accepted Join response"
            )
        if not runner.is_alive():
            raise RuntimeError("Java TCK exited before the Join response fault was armed")
        time.sleep(0.01)
    raise TimeoutError("process fixture did not reach the accepted Join response")


def release_inactive_session_purge_fault(
    directory: Path,
    fixture: subprocess.Popen[bytes],
    runner: threading.Thread,
    timeout: float,
) -> None:
    trigger_marker = directory / "session-purge-trigger.ok"
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if trigger_marker.is_file():
            return
        if fixture.poll() is not None:
            raise RuntimeError(
                "process fixture exited before the inactive-session purge trigger"
            )
        if not runner.is_alive():
            raise RuntimeError(
                "Java TCK exited before the inactive-session purge fault was armed"
            )
        time.sleep(0.01)
    raise TimeoutError("Java TCK did not reach the inactive-session purge trigger")


def run_model(
    arguments: argparse.Namespace,
    model: str,
    temporary_root: Path,
    scenario: str,
    fault_scenario: str,
) -> int:
    directory = temporary_root / model
    directory.mkdir()
    result_file = arguments.results_directory / f"{model}.results.json"
    junit_file = arguments.results_directory / f"{model}.junit.xml"
    evidence_file = arguments.results_directory / f"{model}.fault-evidence.json"
    completion_marker = directory / "message-history-recovery-complete.ok"
    join_response_replay = scenario == JOIN_RESPONSE_SCENARIO
    inactive_session_purge = scenario == INACTIVE_SESSION_PURGE_SCENARIO
    fixture_mode = (
        "public-server-join-response-replay"
        if join_response_replay
        else "public-server-session-purge"
        if inactive_session_purge
        else "public-server-message-history"
    )

    fixture = subprocess.Popen(
        [str(arguments.process_fixture), fixture_mode, str(directory)]
    )
    proxy: message_history_recovery.RawFrameFaultProxy | None = None
    session_purge_proxy: message_history_recovery.SessionPurgeConnectionProxy | None = None
    runner_thread: threading.Thread | None = None
    try:
        port = wait_for_port(directory, fixture, arguments.startup_timeout_s)
        if inactive_session_purge:
            session_purge_proxy = message_history_recovery.SessionPurgeConnectionProxy(port)
        elif not join_response_replay:
            proxy = message_history_recovery.RawFrameFaultProxy(port, fault_scenario)
            port = proxy.port
        jvm_arguments = [
            value.replace("{port}", str(port))
            for value in arguments.provider_jvm_argument
        ]
        jvm_arguments.extend(
            [
                "-Dhla.rti.tck.rtiAddress=tcp://127.0.0.1:" + str(port),
                "-Dhla.rti.tck.callbackModel="
                + ("HLA_EVOKED" if model == "evoked" else "HLA_IMMEDIATE"),
                "-Dhla.rti.tck.ownerConfigurationName="
                + arguments.owner_configuration_name,
                "-Dhla.rti.tck.memberConfigurationName="
                + arguments.member_configuration_name,
                "-Dhla.rti.tck.objectClass=" + arguments.object_class,
                "-Dhla.rti.tck.attribute=" + arguments.attribute,
            ]
        )
        if inactive_session_purge:
            if session_purge_proxy is None:
                raise RuntimeError("inactive-session purge proxy was not started")
            jvm_arguments.extend(
                [
                    "-Dhla.rti.tck.ownerRtiAddress=tcp://127.0.0.1:" + str(port),
                    "-Dhla.rti.tck.memberRtiAddress=tcp://127.0.0.1:"
                    + str(session_purge_proxy.port),
                    "-Dhla.rti.tck.inactiveSessionPurgeTriggerMarker="
                    + str(directory / "session-purge-trigger.ok"),
                    "-Dhla.rti.tck.inactiveSessionPurgeCompletionMarker="
                    + str(completion_marker),
                    "-Dhla.rti.tck.inactiveSessionPurgeTimeoutMillis=5000",
                ]
            )

        def run_java_case() -> int:
            return run_java_tck.run_tck(
                api_jar=arguments.api_jar,
                provider_jars=arguments.provider_jar,
                dependency_jars=arguments.dependency_jar,
                fom_path=arguments.fom_path,
                mim_path=arguments.mim_path,
                classes_directory=arguments.classes_directory,
                java=arguments.java,
                javac=arguments.javac,
                factory_name=arguments.factory_name,
                api_package_prefix=arguments.api_package_prefix,
                time_implementation=arguments.time_implementation,
                scenario=scenario,
                run_unverified=True,
                results_path=result_file,
                junit_path=junit_file,
                provider_id=arguments.provider_id,
                jvm_arguments=jvm_arguments,
                skip_build=True,
            )

        if join_response_replay or inactive_session_purge:
            outcome: dict[str, object] = {}

            def run_java_case_in_thread() -> None:
                try:
                    outcome["result"] = run_java_case()
                except Exception as error:
                    outcome["error"] = error

            runner_thread = threading.Thread(
                target=run_java_case_in_thread,
                name=(
                    "java-tck-inactive-session-purge"
                    if inactive_session_purge
                    else "java-tck-join-response-replay"
                ),
                daemon=True,
            )
            runner_thread.start()
            if join_response_replay:
                release_join_response_fault(
                    directory, fixture, runner_thread, arguments.startup_timeout_s
                )
            else:
                release_inactive_session_purge_fault(
                    directory, fixture, runner_thread, arguments.startup_timeout_s
                )
                if session_purge_proxy is None:
                    raise RuntimeError("inactive-session purge proxy was not started")
                session_purge_proxy.close()
                session_purge_proxy = None
            runner_thread.join(timeout=arguments.fixture_timeout_s)
            if runner_thread.is_alive():
                if inactive_session_purge:
                    raise TimeoutError(
                        "Java TCK did not finish after inactive-session purge"
                    )
                raise TimeoutError("Java TCK did not finish after Join response replay")
            if "error" in outcome:
                raise RuntimeError(f"Java TCK runner failed: {outcome['error']}")
            result = int(outcome.get("result", 1))
        else:
            result = run_java_case()
        if result != 0:
            if proxy is not None:
                print(
                    "raw-frame fault proxy state: "
                    + json.dumps(proxy.diagnostic_state(), sort_keys=True),
                    file=sys.stderr,
                )
            elif join_response_replay:
                print(
                    "Join-response fault markers: "
                    + json.dumps(
                        {
                            name: (directory / name).is_file()
                            for name in (
                                "join-response-ready.ok",
                                "drop-join-response.ok",
                                "join-response-socket-closed.ok",
                            )
                        },
                        sort_keys=True,
                    ),
                    file=sys.stderr,
                )
            elif inactive_session_purge:
                print(
                    "inactive-session purge markers: "
                    + json.dumps(
                        {
                            name: (directory / name).is_file()
                            for name in (
                                "session-purge-trigger.ok",
                                "message-history-recovery-complete.ok",
                            )
                        },
                        sort_keys=True,
                    ),
                    file=sys.stderr,
                )
            return int(result)

        if join_response_replay and not (
            directory / "join-response-socket-closed.ok"
        ).is_file():
            raise RuntimeError("fixture did not confirm the Join-response socket fault")
        if inactive_session_purge:
            if not completion_marker.is_file() or completion_marker.read_text(
                encoding="utf-8"
            ).strip() != "recovered":
                raise RuntimeError(
                    "Java TCK did not confirm inactive-session purge completion"
                )
        else:
            completion_marker.write_text("recovered\n", encoding="utf-8")
        fixture_status = wait_for_fixture(fixture, arguments.fixture_timeout_s)
        fixture = None
        if fixture_status != 0:
            raise RuntimeError(f"process fault fixture exited with status {fixture_status}")
        if join_response_replay:
            evidence = {
                "join_response_ready_before_drop": (
                    directory / "join-response-ready.ok"
                ).is_file(),
                "join_response_drop_requested": (
                    directory / "drop-join-response.ok"
                ).is_file(),
                "join_response_socket_closed": (
                    directory / "join-response-socket-closed.ok"
                ).is_file(),
                "fixture_confirmed_exactly_two_joined_sessions": True,
            }
        elif inactive_session_purge:
            evidence = {
                "session_purge_trigger_requested": (
                    directory / "session-purge-trigger.ok"
                ).is_file(),
                "member_transport_proxy_closed_after_trigger": True,
                "rti_timeout_heart_ms": 40,
                "rti_timeout_purge_ms": 160,
                "java_tck_completion_marker_written": completion_marker.is_file(),
                "fixture_exited_successfully": fixture_status == 0,
            }
        else:
            if proxy is None:
                raise RuntimeError("raw-frame message-history proxy was not started")
            evidence = proxy.evidence()
        evidence_file.write_text(json.dumps(evidence, indent=2) + "\n", encoding="utf-8")
        return 0
    finally:
        if proxy is not None:
            proxy.close()
        if session_purge_proxy is not None:
            session_purge_proxy.close()
        if fixture is not None and fixture.poll() is None:
            fixture.terminate()
            try:
                fixture.wait(timeout=5)
            except subprocess.TimeoutExpired:
                fixture.kill()
                fixture.wait()
        if runner_thread is not None and runner_thread.is_alive():
            runner_thread.join(timeout=5)


def run(arguments: argparse.Namespace) -> int:
    arguments.api_jar = arguments.api_jar.resolve()
    arguments.provider_jar = [path.resolve() for path in arguments.provider_jar]
    arguments.dependency_jar = [path.resolve() for path in arguments.dependency_jar]
    arguments.fom_path = arguments.fom_path.resolve()
    arguments.mim_path = arguments.mim_path.resolve()
    arguments.process_fixture = arguments.process_fixture.resolve()
    arguments.classes_directory = arguments.classes_directory.resolve()
    arguments.results_directory = arguments.results_directory.resolve()
    required_files = [
        (arguments.api_jar, "official Java API JAR"),
        (arguments.fom_path, "FOM module"),
        (arguments.mim_path, "MIM module"),
        (arguments.process_fixture, "adapter process fixture"),
        *[(path, "provider JAR") for path in arguments.provider_jar],
        *[(path, "provider dependency JAR") for path in arguments.dependency_jar],
    ]
    for path, description in required_files:
        if not path.is_file():
            raise FileNotFoundError(f"{description} does not exist: {path}")
    if arguments.startup_timeout_s <= 0 or arguments.fixture_timeout_s <= 0:
        raise ValueError("fixture timeouts must be positive")

    arguments.results_directory.mkdir(parents=True, exist_ok=True)
    if not arguments.skip_build:
        result = run_java_tck.build_tck(
            api_jar=arguments.api_jar,
            classes_directory=arguments.classes_directory,
            javac=arguments.javac,
            api_package_prefix=arguments.api_package_prefix,
        )
        if result != 0:
            return int(result)

    models = (
        ("evoked", "immediate")
        if arguments.callback_model == "both"
        else (arguments.callback_model,)
    )
    with tempfile.TemporaryDirectory(prefix="java-tck-message-history-") as temporary:
        temporary_root = Path(temporary)
        for model in models:
            result = run_model(
                arguments,
                model,
                temporary_root,
                arguments.scenario,
                SCENARIO_TO_CPP_FAULT[arguments.scenario],
            )
            if result != 0:
                return result
    return 0


def main() -> int:
    try:
        return run(parse_arguments())
    except (OSError, RuntimeError, TimeoutError, ValueError) as error:
        print(f"Java message-history adapter failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
