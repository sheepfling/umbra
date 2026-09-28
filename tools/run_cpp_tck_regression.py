"""Regression checks for direct C++ TCK runner evidence handling."""

from __future__ import annotations

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


def main() -> int:
    scenario_ids = ["cpp-tck.first", "cpp-tck.provider-failure", "cpp-tck.last"]
    failed_scenario = scenario_ids[1]

    with tempfile.TemporaryDirectory(prefix="cpp-tck-runner-regression-") as directory:
        root = Path(directory)
        results = root / "combined.json"
        junit = root / "combined.xml"
        attempted: list[str] = []

        def direct_arguments(
            _arguments: object,
            _inputs: dict[str, object],
            selected: list[dict[str, str]],
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

        def fake_run_command(command: list[str], *, cwd: Path) -> None:
            del cwd
            scenario_id = command[2]
            chunk_results = Path(command[4])
            chunk_junit = Path(command[6])
            attempted.append(scenario_id)
            chunk_results.write_text(
                json.dumps(
                    {
                        "callback_model": "evoked",
                        "results": [
                            {"id": scenario_id, "callback_model": "evoked"}
                        ],
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

        selected = [{"id": scenario_id} for scenario_id in scenario_ids]
        try:
            with (
                patch.object(run_cpp_tck, "direct_arguments", direct_arguments),
                patch.object(run_cpp_tck, "run_command", fake_run_command),
            ):
                run_cpp_tck.run_executable_direct(
                    object(),
                    {"build_directory": root},
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
        if (
            testcase_ids != scenario_ids
            or junit_root.attrib.get("failures") != "1"
        ):
            raise AssertionError("merged JUnit did not preserve all pass/failure cases")
        if failed_scenario not in message or str(results) not in message:
            raise AssertionError(f"failure summary omitted scenario/evidence paths: {message}")

    print("cpp-tck: direct multi-scenario failure evidence regression passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
