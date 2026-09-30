#!/usr/bin/env python3
"""Run public ownership service-report TCK cases through a process adapter."""

from __future__ import annotations

import argparse
import json
import shutil
import subprocess
import sys
import tempfile
import time
import xml.etree.ElementTree as ET
from pathlib import Path


ROOT = Path(__file__).resolve().parents[4]
DEFAULT_FOM = ROOT / "packages" / "hla-rti-cpp-tck" / "fom" / "ddm-multi-attribute-tck.xml"
DEFAULT_MIM = (
    ROOT
    / "third_party"
    / "ieee1516.2-2025"
    / "resources"
    / "mim"
    / "HLAstandardMIM-2025.xml"
)
QUERY_OWNERSHIP_SCENARIOS = (
    "cpp-tck.service-report-query-attribute-ownership",
    "cpp-tck.service-report-query-attribute-ownership-contract",
)
CANCEL_ACQUISITION_SCENARIOS = (
    "cpp-tck.service-report-cancel-attribute-ownership-acquisition",
    "cpp-tck.service-report-cancel-attribute-ownership-acquisition-contract",
)
CONFIRM_DIVESTITURE_SCENARIOS = (
    "cpp-tck.service-report-confirm-divestiture",
    "cpp-tck.service-report-confirm-divestiture-contract",
)
SCENARIOS = (
    *QUERY_OWNERSHIP_SCENARIOS,
    *CANCEL_ACQUISITION_SCENARIOS,
    *CONFIRM_DIVESTITURE_SCENARIOS,
)


def fixture_error(directory: Path) -> str:
    for error_path in sorted(directory.glob("public-server-*.error")):
        try:
            detail = error_path.read_text(encoding="utf-8").strip()
        except OSError:
            continue
        if detail:
            return detail
    return ""


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run ownership service-report TCK cases through a process adapter"
    )
    parser.add_argument("--tck-executable", type=Path, required=True)
    parser.add_argument("--process-fixture", type=Path, required=True)
    parser.add_argument("--fom-path", type=Path, default=DEFAULT_FOM)
    parser.add_argument("--mim-fom", type=Path, default=DEFAULT_MIM)
    parser.add_argument("--time-implementation", default="HLAinteger64Time")
    parser.add_argument("--scenario", choices=SCENARIOS, action="append")
    parser.add_argument(
        "--callback-model", choices=("both", "evoked", "immediate"), default="both"
    )
    parser.add_argument("--provider-id", default="current-process")
    parser.add_argument("--timeout-ms", type=int, default=10000)
    parser.add_argument("--federation-name", default="process-confirm-divestiture")
    parser.add_argument("--owner-name", default="package-process-owner")
    parser.add_argument("--member-name", default="package-process-member")
    parser.add_argument("--federate-type", default="package-process-type")
    parser.add_argument("--owner-configuration-name", default="package-process-sender")
    parser.add_argument("--member-configuration-name", default="package-process-receiver")
    parser.add_argument("--configuration-name", default="")
    parser.add_argument("--additional-settings", default="")
    parser.add_argument(
        "--object-class", default="HLAobjectRoot.TckMultiAttributeObject"
    )
    parser.add_argument("--attribute", default="FirstValue")
    parser.add_argument("--secondary-attribute", default="SecondValue")
    parser.add_argument("--startup-timeout-s", type=float, default=30.0)
    parser.add_argument("--fixture-timeout-s", type=float, default=30.0)
    parser.add_argument("--results", type=Path)
    parser.add_argument("--junit", type=Path)
    return parser.parse_args()


def wait_for_port(directory: Path, timeout_s: float) -> int:
    port_path = directory / "port.txt"
    deadline = time.monotonic() + timeout_s
    while time.monotonic() < deadline:
        try:
            port = int(port_path.read_text(encoding="utf-8").strip())
        except (FileNotFoundError, OSError, ValueError):
            port = 0
        if 0 < port <= 65535:
            return port
        time.sleep(0.1)
    detail = fixture_error(directory)
    raise RuntimeError(
        "Confirm Divestiture process fixture did not publish a port"
        + (f": {detail}" if detail else "")
    )


def wait_for_fixture(process: subprocess.Popen[bytes], timeout_s: float) -> int:
    deadline = time.monotonic() + timeout_s
    while process.poll() is None and time.monotonic() < deadline:
        time.sleep(0.1)
    if process.poll() is None:
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()
        raise RuntimeError("Confirm Divestiture process fixture did not terminate")
    return int(process.returncode)


def make_tck_command(
    arguments: argparse.Namespace,
    scenario: str,
    callback_model: str,
    port: int,
    result_path: Path,
    junit_path: Path,
) -> list[str]:
    command = [
        str(arguments.tck_executable),
        "--fom",
        str(arguments.fom_path),
        "--multi-attribute-fom",
        str(arguments.fom_path),
        "--mim-fom",
        str(arguments.mim_fom),
        "--time-implementation",
        arguments.time_implementation,
        "--scenario",
        scenario,
        "--callback-model",
        callback_model,
        "--provider-id",
        arguments.provider_id,
        "--rti-address",
        f"tcp://127.0.0.1:{port}",
        "--federation-name",
        f"{arguments.federation_name}-{callback_model}",
        "--owner-name",
        arguments.owner_name,
        "--member-name",
        arguments.member_name,
        "--federate-type",
        arguments.federate_type,
        "--owner-configuration-name",
        arguments.owner_configuration_name,
        "--member-configuration-name",
        arguments.member_configuration_name,
        # The external server transports process callbacks synchronously. Ask
        # the standard TCK session pump to service immediate-model callbacks
        # at bounded API-call boundaries as it does for existing managed
        # process fixtures.
        "--connection-loss-server-managed",
        "--multi-attribute-object-class",
        arguments.object_class,
        "--multi-attribute-first",
        arguments.attribute,
        "--multi-attribute-second",
        arguments.secondary_attribute,
        "--timeout-ms",
        str(arguments.timeout_ms),
        "--results",
        str(result_path),
        "--junit",
        str(junit_path),
    ]
    if arguments.configuration_name:
        command.extend(["--configuration-name", arguments.configuration_name])
    if arguments.additional_settings:
        command.extend(["--additional-settings", arguments.additional_settings])
    return command


def run_case(
    arguments: argparse.Namespace,
    scenario: str,
    callback_model: str,
    root: Path,
) -> tuple[int, dict[str, object], ET.Element]:
    directory = root / (scenario.replace(".", "-") + "-" + callback_model)
    directory.mkdir()
    result_path = directory / "results.json"
    junit_path = directory / "results.xml"
    if scenario in QUERY_OWNERSHIP_SCENARIOS:
        fixture_mode = "public-server-query-ownership"
    elif scenario in CANCEL_ACQUISITION_SCENARIOS:
        fixture_mode = "public-server-cancel-ownership-acquisition"
    else:
        fixture_mode = "public-server-confirm-divestiture"
    fixture = subprocess.Popen(
        [str(arguments.process_fixture), fixture_mode, str(directory)],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    try:
        port = wait_for_port(directory, arguments.startup_timeout_s)
        result = subprocess.run(
            make_tck_command(
                arguments,
                scenario,
                callback_model,
                port,
                result_path,
                junit_path,
            ),
            check=False,
        )
        if result.returncode != 0:
            detail = fixture_error(directory)
            if detail:
                print(
                    "process fixture: " + detail,
                    file=sys.stderr,
                )
            if fixture.poll() is None:
                fixture.terminate()
                try:
                    fixture.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    fixture.kill()
                    fixture.wait()
            payload = (
                json.loads(result_path.read_text(encoding="utf-8"))
                if result_path.is_file()
                else {}
            )
            junit_root = (
                ET.parse(junit_path).getroot()
                if junit_path.is_file()
                else ET.Element("testsuite")
            )
            return int(result.returncode), payload, junit_root
        fixture_status = wait_for_fixture(fixture, arguments.fixture_timeout_s)
        if fixture_status != 0:
            detail = fixture_error(directory)
            raise RuntimeError(
                f"Ownership service-report process fixture exited {fixture_status}"
                + (f": {detail}" if detail else "")
            )
        payload = (
            json.loads(result_path.read_text(encoding="utf-8"))
            if result_path.is_file()
            else {}
        )
        junit_root = (
            ET.parse(junit_path).getroot()
            if junit_path.is_file()
            else ET.Element("testsuite")
        )
        return int(result.returncode), payload, junit_root
    finally:
        if fixture.poll() is None:
            fixture.terminate()
            try:
                fixture.wait(timeout=5)
            except subprocess.TimeoutExpired:
                fixture.kill()
                fixture.wait()


def write_combined_evidence(
    arguments: argparse.Namespace,
    payloads: list[dict[str, object]],
    junit_roots: list[ET.Element],
) -> None:
    if arguments.results is not None and payloads:
        combined = dict(payloads[0])
        combined["callback_model"] = arguments.callback_model
        combined["results"] = [
            result for payload in payloads for result in payload.get("results", [])
        ]
        arguments.results.parent.mkdir(parents=True, exist_ok=True)
        arguments.results.write_text(
            json.dumps(combined, indent=2, sort_keys=True) + "\n", encoding="utf-8"
        )
    if arguments.junit is not None and junit_roots:
        tests = sum(int(root.attrib.get("tests", "0")) for root in junit_roots)
        failures = sum(int(root.attrib.get("failures", "0")) for root in junit_roots)
        combined_root = ET.Element(
            "testsuite",
            {"name": "hla-rti-cpp-tck", "tests": str(tests), "failures": str(failures)},
        )
        for root in junit_roots:
            combined_root.extend(list(root))
        arguments.junit.parent.mkdir(parents=True, exist_ok=True)
        ET.ElementTree(combined_root).write(
            arguments.junit, encoding="utf-8", xml_declaration=True
        )


def run(arguments: argparse.Namespace) -> int:
    for path, description in (
        (arguments.tck_executable, "TCK executable"),
        (arguments.process_fixture, "process fixture"),
        (arguments.fom_path, "adapter FOM"),
        (arguments.mim_fom, "adapter MIM"),
    ):
        if not path.is_file():
            raise FileNotFoundError(f"{description} does not exist: {path}")
    scenarios = arguments.scenario or list(SCENARIOS)
    models = (
        ("evoked", "immediate")
        if arguments.callback_model == "both"
        else (arguments.callback_model,)
    )
    exit_code = 0
    payloads: list[dict[str, object]] = []
    junit_roots: list[ET.Element] = []
    with tempfile.TemporaryDirectory(prefix="hla-rti-cpp-tck-ownership-service-report-") as temporary:
        root = Path(temporary)
        for callback_model in models:
            for scenario in scenarios:
                code, payload, junit_root = run_case(
                    arguments, scenario, callback_model, root
                )
                exit_code = exit_code or code
                if payload:
                    payloads.append(payload)
                junit_roots.append(junit_root)
    write_combined_evidence(arguments, payloads, junit_roots)
    return exit_code


def main() -> int:
    try:
        return run(parse_arguments())
    except (OSError, RuntimeError, ValueError, json.JSONDecodeError) as error:
        print(f"Ownership service-report process adapter failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
