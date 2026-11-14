#!/usr/bin/env python3
"""Run the ServiceGroup service-report case against an adapter-supplied provider."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / "tools"))
import run_java_tck

SCENARIO = "java-tck.service-report-service-group-file"


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--api-jar", type=Path, required=True)
    parser.add_argument("--provider-jar", type=Path, action="append", required=True)
    parser.add_argument("--dependency-jar", type=Path, action="append", default=[])
    parser.add_argument("--fom-path", type=Path, required=True)
    parser.add_argument("--mim-path", type=Path, required=True)
    parser.add_argument("--classes-directory", type=Path, required=True)
    parser.add_argument("--results-directory", type=Path, required=True)
    parser.add_argument("--factory-name", default="")
    parser.add_argument("--api-package-prefix", default="hla.rti1516_2025")
    parser.add_argument("--time-implementation", default="HLAinteger64Time")
    parser.add_argument("--provider-id", default="java-current-process")
    parser.add_argument("--provider-jvm-argument", action="append", default=[])
    parser.add_argument(
        "--callback-model", choices=("both", "evoked", "immediate"), default="both"
    )
    parser.add_argument("--java", default="java")
    parser.add_argument("--javac", default="javac")
    parser.add_argument("--skip-build", action="store_true")
    return parser.parse_args()


def run_model(arguments: argparse.Namespace, model: str) -> int:
    service_report_directory = (
        arguments.results_directory / f"{model}-service-reports"
    )
    service_report_directory.mkdir(parents=True, exist_ok=True)
    results_path = arguments.results_directory / f"{model}.results.json"
    junit_path = arguments.results_directory / f"{model}.junit.xml"
    evidence_path = arguments.results_directory / f"{model}.adapter-evidence.json"
    additional_settings = f"serviceReportDirectory={service_report_directory}"
    jvm_arguments = [
        value.replace("{service_report_directory}", str(service_report_directory))
        for value in arguments.provider_jvm_argument
    ]
    jvm_arguments.extend(
        [
            "-Dhla.rti.tck.configurationName=service-report-service-group-file",
            "-Dhla.rti.tck.callbackModel="
            + ("HLA_EVOKED" if model == "evoked" else "HLA_IMMEDIATE"),
            "-Dhla.rti.tck.additionalSettings=" + additional_settings,
            "-Dhla.rti.tck.serviceReportDirectory=" + str(service_report_directory),
        ]
    )
    result = run_java_tck.run_tck(
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
        scenario=SCENARIO,
        run_unverified=True,
        results_path=results_path,
        junit_path=junit_path,
        provider_id=arguments.provider_id,
        jvm_arguments=jvm_arguments,
        skip_build=True,
    )
    if result != 0:
        return int(result)

    result_data = json.loads(results_path.read_text(encoding="utf-8"))
    selected = [
        item
        for item in result_data.get("scenarios", [])
        if item.get("id") == SCENARIO
    ]
    if len(selected) != 1 or selected[0].get("status") != "pass":
        raise RuntimeError("Java TCK did not record a passing ServiceGroup case")
    report_files = sorted(
        path for path in service_report_directory.rglob("*") if path.is_file()
    )
    if not report_files:
        raise RuntimeError("the provider did not create a service-report file")
    evidence = {
        "service_report_directory": str(service_report_directory),
        "additional_settings": additional_settings,
        "advertised_report_file_present": True,
        "report_files": [
            {"path": str(path), "size_bytes": path.stat().st_size}
            for path in report_files
        ],
    }
    evidence_path.write_text(
        json.dumps(evidence, indent=2) + "\n", encoding="utf-8"
    )
    return 0


def run(arguments: argparse.Namespace) -> int:
    for name in (
        "api_jar",
        "fom_path",
        "mim_path",
        "classes_directory",
        "results_directory",
    ):
        setattr(arguments, name, getattr(arguments, name).resolve())
    arguments.provider_jar = [path.resolve() for path in arguments.provider_jar]
    arguments.dependency_jar = [path.resolve() for path in arguments.dependency_jar]
    arguments.results_directory.mkdir(parents=True, exist_ok=True)
    models = ("evoked", "immediate") if arguments.callback_model == "both" else (
        arguments.callback_model,
    )
    for model in models:
        result = run_model(arguments, model)
        if result != 0:
            return result
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(run(parse_arguments()))
    except Exception as error:
        print(f"Java ServiceGroup adapter failed: {error}", file=sys.stderr)
        raise SystemExit(2) from error
