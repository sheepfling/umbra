"""Regression checks for direct C++ TCK runner evidence handling."""

from __future__ import annotations

import argparse
import json
import subprocess
import tempfile
import xml.etree.ElementTree as ET
from pathlib import Path
from unittest.mock import patch

try:
    from . import run_cpp_tck
except ImportError:
    import run_cpp_tck


def _resolved_inputs(root: Path) -> run_cpp_tck.ResolvedInputs:
    return {
        "package_prefix": root,
        "build_directory": root,
        "service_report_directory": root,
        "fom": root,
        "model_fom": root,
        "rate_fom": root,
        "ddm_fom": root,
        "multi_attribute_fom": root,
        "three_dimensional_fom": root,
        "mim_fom": root,
        "switches_fom": root,
        "auto_provide_fom": root,
        "data_type_merge_consumer_fom": root,
        "data_type_merge_provider_fom": root,
        "data_type_merge_conflicting_provider_fom": root,
        "variant_alternative_base_fom": root,
        "variant_alternative_disjoint_fom": root,
        "variant_alternative_overlap_fom": root,
        "interaction_parameter_base_fom": root,
        "interaction_parameter_disjoint_fom": root,
        "interaction_parameter_conflict_fom": root,
        "object_attribute_base_fom": root,
        "object_attribute_disjoint_fom": root,
        "object_attribute_conflict_fom": root,
        "known_class_fom": root,
        "known_class_disabled_fom": root,
        "ownership_extension_fom": root,
        "ddm_dimensions": [],
        "three_dimensions": [],
        "additional_fom": [],
        "invalid_fom": [],
        "standard_invalid_fom": [],
        "scenario_foms": [],
    }


def _scenario(scenario_id: str) -> run_cpp_tck.CppTckScenario:
    return {
        "id": scenario_id,
        "runner_id": scenario_id,
        "default_status": "run",
        "promotion": "promoted",
    }


def verify_portable_dispatch_direct_evidence() -> None:
    scenario_id = "cpp-tck.pending-negotiated-ownership-package-filesystem-restore"
    with tempfile.TemporaryDirectory(
        prefix="cpp-tck-portable-dispatch-regression-"
    ) as directory:
        root = Path(directory)
        results = root / "results.json"
        junit = root / "junit.xml"
        scenario = _scenario(scenario_id)
        attempted: list[str] = []
        expected_environment = run_cpp_tck.direct_run_environment(
            _resolved_inputs(root)
        )

        def direct_arguments(
            arguments: argparse.Namespace,
            _inputs: run_cpp_tck.ResolvedInputs,
            selected: list[run_cpp_tck.CppTckScenario],
            result_path: Path | None,
            junit_path: Path | None,
        ) -> list[str]:
            if result_path is not None or junit_path is not None:
                raise AssertionError(
                    "portable dispatch forwarded unsupported result switches"
                )
            return [
                "--scenario",
                selected[0]["runner_id"],
                "--callback-model",
                arguments.callback_model,
            ]

        def fake_run(
            command: list[str],
            *,
            cwd: Path,
            env: dict[str, str],
            text: bool,
            capture_output: bool,
            check: bool,
        ) -> subprocess.CompletedProcess[str]:
            if cwd != run_cpp_tck.ROOT or not text or not capture_output or check:
                raise AssertionError(
                    "portable dispatch did not capture the child result"
                )
            if env != expected_environment:
                raise AssertionError(
                    "portable dispatch did not pass the package runtime environment"
                )
            model = command[command.index("--callback-model") + 1]
            attempted.append(model)
            return subprocess.CompletedProcess(
                command,
                0,
                stdout=f"passed {scenario_id} [{model}]\n",
                stderr="",
            )

        with (
            patch.object(
                run_cpp_tck, "is_portable_dispatch_scenario", return_value=True
            ),
            patch.object(run_cpp_tck, "direct_arguments", direct_arguments),
            patch.object(run_cpp_tck.subprocess, "run", fake_run),
        ):
            run_cpp_tck.run_executable_direct(
                argparse.Namespace(callback_model="both"),
                _resolved_inputs(root),
                root / "provider.exe",
                [scenario],
                results,
                junit,
            )

        if attempted != ["evoked", "immediate"]:
            raise AssertionError(
                f"portable dispatch callback matrix was wrong: {attempted}"
            )
        payload = json.loads(results.read_text(encoding="utf-8"))
        if payload.get("kind") != "hla-rti-cpp-tck-evidence":
            raise AssertionError("portable dispatch JSON evidence has the wrong kind")
        result_pairs = {
            (result.get("id"), result.get("callback_model"), result.get("status"))
            for result in payload.get("results", [])
        }
        expected_pairs = {
            (scenario_id, "evoked", "passed"),
            (scenario_id, "immediate", "passed"),
        }
        if result_pairs != expected_pairs:
            raise AssertionError(
                f"portable dispatch JSON evidence was incomplete: {result_pairs}"
            )
        junit_root = ET.parse(junit).getroot()
        if (
            junit_root.attrib.get("tests") != "2"
            or junit_root.attrib.get("failures") != "0"
        ):
            raise AssertionError(
                "portable dispatch JUnit did not preserve both passing models"
            )
        if len(junit_root.findall("testcase")) != 2:
            raise AssertionError("portable dispatch JUnit omitted callback-model cases")


def main() -> int:
    verify_portable_dispatch_direct_evidence()
    scenario_ids = ["cpp-tck.first", "cpp-tck.provider-failure", "cpp-tck.last"]
    failed_scenario = scenario_ids[1]

    with tempfile.TemporaryDirectory(prefix="cpp-tck-runner-regression-") as directory:
        root = Path(directory)
        results = root / "combined.json"
        junit = root / "combined.xml"
        attempted: list[str] = []
        expected_environment = run_cpp_tck.direct_run_environment(
            _resolved_inputs(root)
        )

        def direct_arguments(
            _arguments: object,
            _inputs: run_cpp_tck.ResolvedInputs,
            selected: list[run_cpp_tck.CppTckScenario],
            chunk_results: Path | None,
            chunk_junit: Path | None,
        ) -> list[str]:
            assert chunk_results is not None
            assert chunk_junit is not None
            return [
                "--scenario",
                selected[0]["id"],
                "--results",
                str(chunk_results),
                "--junit",
                str(chunk_junit),
            ]

        def fake_run_command(
            command: list[str], *, cwd: Path, env: dict[str, str]
        ) -> None:
            if cwd != run_cpp_tck.ROOT or env != expected_environment:
                raise AssertionError(
                    "direct runner omitted its working directory or runtime environment"
                )
            scenario_id = command[2]
            chunk_results = Path(command[4])
            chunk_junit = Path(command[6])
            attempted.append(scenario_id)
            chunk_results.write_text(
                json.dumps(
                    {
                        "callback_model": "evoked",
                        "results": [{"id": scenario_id, "callback_model": "evoked"}],
                    }
                ),
                encoding="utf-8",
            )

            failure_count = int(scenario_id == failed_scenario)
            suite = ET.Element(
                "testsuite",
                tests="1",
                failures=str(failure_count),
                errors="0",
                skipped="0",
                time="0.01",
            )
            testcase = ET.SubElement(suite, "testcase", name=scenario_id)
            if failure_count:
                ET.SubElement(testcase, "failure", message="expected test failure")
            ET.ElementTree(suite).write(
                chunk_junit,
                encoding="utf-8",
                xml_declaration=True,
            )

            if failure_count:
                raise subprocess.CalledProcessError(1, command)

        selected = [_scenario(scenario_id) for scenario_id in scenario_ids]
        try:
            with (
                patch.object(run_cpp_tck, "direct_arguments", direct_arguments),
                patch.object(run_cpp_tck, "run_command", fake_run_command),
            ):
                run_cpp_tck.run_executable_direct(
                    argparse.Namespace(callback_model="both"),
                    _resolved_inputs(root),
                    root / "provider.exe",
                    selected,
                    results,
                    junit,
                )
        except ValueError as error:
            message = str(error)
        else:
            raise AssertionError("a failed child process was reported as successful")

        if attempted != scenario_ids:
            raise AssertionError(f"not all selected scenarios ran: {attempted}")
        if not results.is_file() or not junit.is_file():
            raise AssertionError("merged JSON/JUnit evidence was not preserved")

        result_payload = json.loads(results.read_text(encoding="utf-8"))
        observed_ids = [result["id"] for result in result_payload["results"]]
        if observed_ids != scenario_ids:
            raise AssertionError(f"merged JSON is incomplete: {observed_ids}")

        junit_root = ET.parse(junit).getroot()
        testcase_ids = [
            testcase.attrib["name"] for testcase in junit_root.findall("testcase")
        ]
        if testcase_ids != scenario_ids or junit_root.attrib.get("failures") != "1":
            raise AssertionError("merged JUnit did not preserve all pass/failure cases")
        if failed_scenario not in message or str(results) not in message:
            raise AssertionError(
                f"failure summary omitted scenario/evidence paths: {message}"
            )

    print("cpp-tck: direct multi-scenario failure evidence regression passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
