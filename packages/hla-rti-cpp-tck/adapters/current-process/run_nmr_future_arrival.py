#!/usr/bin/env python3
"""Run public NMR/NMRA/TARA C++ TCK cases through separate TCP clients."""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import tempfile
import time
from pathlib import Path
from xml.etree import ElementTree


ROOT = Path(__file__).resolve().parents[4]
DEFAULT_FOM = (
    ROOT
    / "third_party"
    / "ieee1516.2-2025"
    / "resources"
    / "examples"
    / "RestaurantFOMmodule-2025.xml"
)
DEFAULT_MIM = (
    ROOT
    / "third_party"
    / "ieee1516.2-2025"
    / "resources"
    / "mim"
    / "HLAstandardMIM-2025.xml"
)
SERVER_MODE = "public-server-nmr-cross-listener"
COMPLETE_MARKER = "nmr-cross-listener-complete.ok"
SCENARIO_GROUPS = (
    (
        "cpp-tck.nmr-future-tso-arrival-while-pending",
        "cpp-tck.nmr-future-tso-arrival-while-pending-contract",
    ),
    (
        "cpp-tck.nmr-directed-future-tso-arrival-while-pending",
        "cpp-tck.nmr-directed-future-tso-arrival-while-pending-contract",
    ),
    (
        "cpp-tck.nmra-directed-future-tso-arrival-while-pending",
        "cpp-tck.nmra-directed-future-tso-arrival-while-pending-contract",
    ),
    (
        "cpp-tck.timestamped-attribute-update-future-nmr-arrival",
        "cpp-tck.timestamped-attribute-update-future-nmr-arrival-contract",
    ),
    (
        "cpp-tck.timestamped-attribute-update-future-nmra-arrival",
        "cpp-tck.timestamped-attribute-update-future-nmra-arrival-contract",
    ),
    (
        "cpp-tck.timestamped-object-deletion-future-nmra-arrival",
        "cpp-tck.timestamped-object-deletion-future-nmra-arrival-contract",
    ),
    (
        "cpp-tck.timestamped-object-deletion-future-tara-arrival",
        "cpp-tck.timestamped-object-deletion-future-tara-arrival-contract",
    ),
    (
        "cpp-tck.timestamped-attribute-update-future-tara-arrival",
        "cpp-tck.timestamped-attribute-update-future-tara-arrival-contract",
    ),
    (
        "cpp-tck.timestamped-interaction-tara-queued-equal-boundary",
        "cpp-tck.timestamped-interaction-tara-queued-equal-boundary-contract",
    ),
    (
        "cpp-tck.timestamped-interaction-tara-equal-boundary",
        "cpp-tck.timestamped-interaction-tara-equal-boundary-contract",
    ),
    ("cpp-tck.nmra-future-tso-arrival-pending-request",),
    (
        "cpp-tck.nmra-no-tso-waits-for-galt",
        "cpp-tck.nmra-no-tso-waits-for-galt-contract",
    ),
)
SCENARIOS = tuple(scenario for group in SCENARIO_GROUPS for scenario in group)


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Run official C++ NMR/NMRA/TARA scenarios against separate publisher "
            "and requester processes sharing one authoritative coordinator"
        )
    )
    parser.add_argument("--tck-executable", type=Path, required=True)
    parser.add_argument("--process-fixture", type=Path, required=True)
    parser.add_argument("--provider-bin", type=Path, required=True)
    parser.add_argument(
        "--scenario",
        action="append",
        choices=SCENARIOS,
        help="Run only these scenarios; repeat to select a focused pair",
    )
    parser.add_argument("--fom-path", type=Path, default=DEFAULT_FOM)
    parser.add_argument("--mim-fom", type=Path, default=DEFAULT_MIM)
    parser.add_argument("--timeout-ms", type=int, default=10000)
    parser.add_argument(
        "--callback-model",
        choices=("evoked", "immediate"),
        default="evoked",
    )
    parser.add_argument("--startup-timeout-s", type=float, default=30.0)
    parser.add_argument("--scenario-timeout-s", type=float, default=60.0)
    parser.add_argument("--results", type=Path, required=True)
    parser.add_argument("--junit", type=Path, required=True)
    return parser.parse_args()


def wait_for_port(
    marker: Path,
    error_marker: Path,
    fixture: subprocess.Popen[bytes],
    timeout_s: float,
) -> int:
    deadline = time.monotonic() + timeout_s
    while time.monotonic() < deadline:
        try:
            port = int(marker.read_text(encoding="utf-8").strip())
        except (FileNotFoundError, OSError, ValueError):
            port = 0
        if 0 < port <= 65535:
            return port
        if error_marker.is_file():
            raise RuntimeError(
                "The NMR coordinator fixture failed before listening: "
                + error_marker.read_text(encoding="utf-8", errors="replace").strip()
            )
        exit_code = fixture.poll()
        if exit_code is not None:
            raise RuntimeError(
                "The NMR coordinator fixture exited before publishing both ports "
                f"(exit code {exit_code})"
            )
        time.sleep(0.05)
    raise RuntimeError(
        f"The NMR coordinator fixture did not publish {marker.name} "
        f"within {timeout_s:g} seconds"
    )


def run() -> int:
    arguments = parse_arguments()
    for path, label in (
        (arguments.tck_executable, "TCK executable"),
        (arguments.process_fixture, "process fixture"),
        (arguments.provider_bin, "provider runtime directory"),
        (arguments.fom_path, "restaurant FOM"),
        (arguments.mim_fom, "standard MIM"),
    ):
        if not path.exists():
            raise FileNotFoundError(f"The {label} does not exist: {path}")

    arguments.results = arguments.results.resolve()
    arguments.junit = arguments.junit.resolve()
    arguments.results.parent.mkdir(parents=True, exist_ok=True)
    arguments.junit.parent.mkdir(parents=True, exist_ok=True)
    selected_scenarios = arguments.scenario or SCENARIOS
    selected_scenario_set = set(selected_scenarios)
    scenario_groups = tuple(
        group
        for group in (
            tuple(scenario for scenario in group if scenario in selected_scenario_set)
            for group in SCENARIO_GROUPS
        )
        if group
    )
    print(
        "Selected C++ TCK scenarios: " + ", ".join(selected_scenarios),
        flush=True,
    )

    runtime_path = os.pathsep.join(
        (
            str(arguments.provider_bin.resolve()),
            str(arguments.process_fixture.resolve().parent),
            os.environ.get("PATH", ""),
        )
    )
    environment = os.environ.copy()
    environment["PATH"] = runtime_path

    result_documents = []
    junit_suites = []
    with tempfile.TemporaryDirectory(prefix="nmr-cross-listener-") as temporary:
        run_directory = Path(temporary)
        report_directory = run_directory / "service-reports"
        report_directory.mkdir()
        error_marker = run_directory / f"{SERVER_MODE}.error"
        fixture = subprocess.Popen(
            [str(arguments.process_fixture.resolve()), SERVER_MODE, str(run_directory)],
            cwd=ROOT,
            env=environment,
        )
        try:
            publisher_port = wait_for_port(
                run_directory / "publisher-port.txt",
                error_marker,
                fixture,
                arguments.startup_timeout_s,
            )
            requester_port = wait_for_port(
                run_directory / "requester-port.txt",
                error_marker,
                fixture,
                arguments.startup_timeout_s,
            )
            for group_index, scenario_group in enumerate(scenario_groups):
                group_directory = run_directory / f"scenario-group-{group_index}"
                coordination_directory = group_directory / "coordination"
                coordination_directory.mkdir(parents=True)
                group_results = group_directory / "results.json"
                group_junit = group_directory / "junit.xml"
                command = [
                    str(arguments.tck_executable.resolve()),
                    "--fom",
                    str(arguments.fom_path.resolve()),
                    "--mim-fom",
                    str(arguments.mim_fom.resolve()),
                    "--time-implementation",
                    "HLAinteger64Time",
                    "--provider-id",
                    "current-process",
                    "--callback-model",
                    arguments.callback_model,
                    "--timeout-ms",
                    str(arguments.timeout_ms),
                    "--owner-rti-address",
                    f"tcp://127.0.0.1:{requester_port}",
                    "--member-rti-address",
                    f"tcp://127.0.0.1:{publisher_port}",
                    "--owner-configuration-name",
                    "process-tso-cross-service-receiver",
                    "--member-configuration-name",
                    "process-tso-cross-service-sender",
                    "--owner-name",
                    "process-tso-cross-service-receiver",
                    "--member-name",
                    "process-tso-cross-service-sender",
                    "--federate-type",
                    "process-tso-cross-service-type",
                    "--federation-name",
                    f"UmbraNmrFuture{time.time_ns()}",
                    "--interaction-class",
                    "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed",
                    "--parameter",
                    "TimelinessOk",
                    "--attribute",
                    "Efficiency",
                    "--secondary-directed-object-class",
                    "HLAobjectRoot.Employee.Server",
                    "--secondary-directed-interaction-class",
                    "HLAinteractionRoot.ServerAction.TakeOrder",
                    "--service-report-directory",
                    str(report_directory),
                    "--nmr-process-coordination-directory",
                    str(coordination_directory),
                ]
                if any(
                    scenario.startswith(
                        "cpp-tck.timestamped-attribute-update-future-nmr-arrival"
                    )
                    or scenario.startswith(
                        "cpp-tck.timestamped-attribute-update-future-nmra-arrival"
                    )
                    or scenario.startswith(
                        "cpp-tck.timestamped-attribute-update-future-tara-arrival"
                    )
                    or scenario.startswith(
                        "cpp-tck.timestamped-object-deletion-future-nmra-arrival"
                    )
                    or scenario.startswith(
                        "cpp-tck.timestamped-object-deletion-future-tara-arrival"
                    )
                    for scenario in scenario_group
                ):
                    command.extend(
                        ("--object-class", "HLAobjectRoot.Employee.Server")
                    )
                for scenario in scenario_group:
                    command.extend(("--scenario", scenario))
                requester_command = [*command, "--nmr-process-role", "requester"]
                requester_command.extend(
                    ("--results", str(group_results), "--junit", str(group_junit))
                )
                publisher_command = [*command, "--nmr-process-role", "publisher"]
                requester = subprocess.Popen(
                    requester_command,
                    cwd=ROOT,
                    env=environment,
                )
                publisher = subprocess.Popen(
                    publisher_command,
                    cwd=ROOT,
                    env=environment,
                )
                deadline = time.monotonic() + arguments.scenario_timeout_s
                try:
                    while time.monotonic() < deadline:
                        requester_status = requester.poll()
                        publisher_status = publisher.poll()
                        if requester_status not in (None, 0):
                            raise RuntimeError(
                                "The NMR requester TCK process failed with exit code "
                                f"{requester_status}"
                            )
                        if publisher_status not in (None, 0):
                            raise RuntimeError(
                                "The NMR publisher TCK process failed with exit code "
                                f"{publisher_status}"
                            )
                        if requester_status == 0 and publisher_status == 0:
                            break
                        time.sleep(0.05)
                    else:
                        raise TimeoutError(
                            "The independent NMR requester/publisher TCK processes "
                            f"did not finish within {arguments.scenario_timeout_s:g} seconds"
                        )
                finally:
                    for process in (requester, publisher):
                        if process.poll() is None:
                            process.terminate()
                            try:
                                process.wait(timeout=5.0)
                            except subprocess.TimeoutExpired:
                                process.kill()
                                process.wait(timeout=5.0)

                result_document = json.loads(group_results.read_text(encoding="utf-8"))
                reported_scenarios = {
                    result.get("id")
                    for result in result_document.get("results", [])
                    if isinstance(result, dict)
                }
                missing_scenarios = [
                    scenario
                    for scenario in scenario_group
                    if scenario not in reported_scenarios
                ]
                if missing_scenarios:
                    raise RuntimeError(
                        "The TCK process did not report every selected scenario: "
                        + ", ".join(missing_scenarios)
                    )
                result_documents.append(result_document)
                junit_suites.append(ElementTree.parse(group_junit).getroot())
                print(
                    "Completed process-separated scenario group: "
                    + ", ".join(scenario_group),
                    flush=True,
                )
        finally:
            (run_directory / COMPLETE_MARKER).write_text("ok\n", encoding="utf-8")
            try:
                fixture.wait(timeout=5.0)
            except subprocess.TimeoutExpired:
                fixture.terminate()
                fixture.wait(timeout=5.0)

        if fixture.returncode != 0:
            detail = (
                error_marker.read_text(encoding="utf-8", errors="replace").strip()
                if error_marker.is_file()
                else f"fixture exit code {fixture.returncode}"
            )
            raise RuntimeError(f"The NMR coordinator fixture failed: {detail}")
        result_document = dict(result_documents[0])
        result_document["results"] = [
            result
            for document in result_documents
            for result in document.get("results", [])
        ]
        arguments.results.write_text(
            json.dumps(result_document, indent=2) + "\n", encoding="utf-8"
        )
        junit_cases = [
            testcase
            for suite in junit_suites
            for testcase in suite.findall("testcase")
        ]
        junit_failures = sum(
            testcase.find("failure") is not None for testcase in junit_cases
        )
        junit_skipped = sum(
            testcase.find("skipped") is not None for testcase in junit_cases
        )
        junit_root = ElementTree.Element(
            "testsuite",
            {
                "name": "hla-rti-cpp-tck",
                "tests": str(len(junit_cases)),
                "failures": str(junit_failures),
                "skipped": str(junit_skipped),
            },
        )
        junit_root.extend(junit_cases)
        ElementTree.ElementTree(junit_root).write(
            arguments.junit,
            encoding="utf-8",
            xml_declaration=True,
        )
        reported_scenarios = {
            result.get("id")
            for result in result_document.get("results", [])
            if isinstance(result, dict)
        }
        missing_scenarios = [
            scenario
            for scenario in selected_scenarios
            if scenario not in reported_scenarios
        ]
        if missing_scenarios:
            raise RuntimeError(
                "The TCK process did not report every selected scenario: "
                + ", ".join(missing_scenarios)
            )
        return 0


if __name__ == "__main__":
    raise SystemExit(run())
