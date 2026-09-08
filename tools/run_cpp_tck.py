"""Configure, build, and run the installed-package C++ TCK adapter.

The runner intentionally uses only the Python standard library.  CMake owns
the provider/package boundary and CTest owns the configured matrix; this
module only assembles argument lists and starts those tools without a shell.
"""

from __future__ import annotations

import argparse
import json
import os
import shlex
import shutil
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import Any, Iterable

try:
    from .cpp_tck import DEFAULT_CATALOG, ROOT, load_catalog
except ImportError:  # Direct invocation: python tools/run_cpp_tck.py
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    from cpp_tck import DEFAULT_CATALOG, ROOT, load_catalog


ADAPTER = ROOT / "packages" / "hla-rti-cpp-tck" / "adapters" / "current-package"
FOM_ROOT = ROOT / "packages" / "hla-rti-cpp-tck" / "fom"
DEFAULT_MIM = (
    ROOT
    / "third_party"
    / "ieee1516.2-2025"
    / "resources"
    / "mim"
    / "HLAstandardMIM-2025.xml"
)
DEFAULT_INVALID_FOM = (
    FOM_ROOT / "invalid-malformed-tck.xml",
    FOM_ROOT / "invalid-namespace-tck.xml",
    FOM_ROOT / "invalid-duplicate-tck.xml",
)


def default_path(relative: str) -> Path:
    return ROOT / relative


def resolve_path(value: str | Path, *, must_exist: bool, label: str) -> Path:
    path = Path(value)
    if not path.is_absolute():
        path = Path.cwd() / path
    path = path.resolve()
    if must_exist and not path.is_file():
        raise ValueError(f"{label} does not exist: {path}")
    return path


def resolve_directory(value: str | Path, *, must_exist: bool, label: str) -> Path:
    path = Path(value)
    if not path.is_absolute():
        path = Path.cwd() / path
    path = path.resolve()
    if must_exist and not path.is_dir():
        raise ValueError(f"{label} does not exist: {path}")
    return path


def path_list(values: Iterable[str], defaults: Iterable[Path], label: str) -> list[Path]:
    selected = list(values)
    if not selected:
        selected = [str(value) for value in defaults]
    return [resolve_path(value, must_exist=True, label=label) for value in selected]


def join_cmake_list(values: Iterable[Path | str]) -> str:
    return ";".join(str(value) for value in values)


def command_text(command: list[str]) -> str:
    if os.name == "nt":
        return subprocess.list2cmdline(command)
    return shlex.join(command)


def run_command(command: list[str], *, cwd: Path) -> None:
    print(f"+ {command_text(command)}", flush=True)
    subprocess.run(command, cwd=cwd, check=True)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--package-prefix",
        required=True,
        help="Explicit installed provider package prefix; registry lookup is disabled",
    )
    parser.add_argument(
        "--build-directory",
        default=".build/cpp-tck-python",
        help="CMake build directory (default: .build/cpp-tck-python)",
    )
    parser.add_argument("--generator", help="Optional CMake generator name")
    parser.add_argument("--cmake", default="cmake", help="CMake executable")
    parser.add_argument("--ctest", default="ctest", help="CTest executable")
    parser.add_argument(
        "--configuration",
        default="Debug",
        help="Build/CTest configuration (default: Debug)",
    )
    parser.add_argument(
        "--build-parallel",
        type=int,
        default=max(1, min(4, os.cpu_count() or 1)),
        help="Parallel build jobs (default: bounded by available CPUs)",
    )
    parser.add_argument(
        "--scenario-set",
        choices=("verified", "all"),
        default="verified",
        help="Catalog promotion set configured for CTest (default: verified)",
    )
    parser.add_argument(
        "--scenario",
        action="append",
        default=[],
        help="Run this catalog scenario directly; repeat for a focused slice",
    )
    parser.add_argument(
        "--callback-model",
        choices=("both", "evoked", "immediate"),
        default="both",
        help="Callback model for CTest and direct evidence (default: both)",
    )
    parser.add_argument(
        "--time-implementation",
        default="HLAinteger64Time",
        help="Logical-time implementation selected by the adapter",
    )
    parser.add_argument("--provider-id", default="installed-package")
    parser.add_argument("--timeout-ms", type=int, default=10000)

    parser.add_argument("--fom", default=str(default_path("packages/hla-rti-cpp-tck/fom/p0-tck.xml")))
    parser.add_argument(
        "--model-fom",
        default=str(default_path("packages/hla-rti-cpp-tck/fom/fom-model-tck.xml")),
    )
    parser.add_argument(
        "--ddm-fom",
        default=str(default_path("packages/hla-rti-cpp-tck/fom/ddm-tck.xml")),
    )
    parser.add_argument(
        "--multi-attribute-fom",
        default=str(default_path("packages/hla-rti-cpp-tck/fom/ddm-multi-attribute-tck.xml")),
    )
    parser.add_argument(
        "--three-dimensional-fom",
        default=str(default_path("packages/hla-rti-cpp-tck/fom/ddm-three-dimensional-tck.xml")),
    )
    parser.add_argument("--mim-fom", default=str(DEFAULT_MIM))
    parser.add_argument(
        "--switches-fom",
        default=str(default_path("packages/hla-rti-cpp-tck/fom/switches-tck.xml")),
    )
    parser.add_argument(
        "--auto-provide-fom",
        default=str(default_path("packages/hla-rti-cpp-tck/fom/auto-provide-tck.xml")),
    )
    parser.add_argument(
        "--ddm-dimension",
        action="append",
        default=[],
        help="DDM dimension name; repeat for the two-dimensional fixture",
    )
    parser.add_argument(
        "--three-dimensional-dimension",
        action="append",
        default=[],
        help="Three-dimensional DDM dimension name; repeat at least three times",
    )
    parser.add_argument(
        "--three-dimensional-object-class",
        default="HLAobjectRoot.TckThreeDimensionalObject",
    )
    parser.add_argument("--three-dimensional-attribute", default="Value")
    parser.add_argument(
        "--custom-regional-object-class",
        default="HLAobjectRoot.TckTransportRegionalObject",
    )
    parser.add_argument("--custom-regional-attribute", default="RegionalValue")
    parser.add_argument(
        "--custom-regional-interaction-class",
        default="HLAinteractionRoot.TckTransportRegionalInteraction",
    )
    parser.add_argument("--custom-regional-parameter", default="RegionalPayload")
    parser.add_argument(
        "--additional-fom",
        action="append",
        default=[],
        help="Additional FOM module; repeat to replace the default extension",
    )
    parser.add_argument(
        "--invalid-fom",
        action="append",
        default=[],
        help="Invalid FOM module; repeat to replace the default invalid corpus",
    )

    parser.add_argument("--object-class", default="HLAobjectRoot.TckObject")
    parser.add_argument("--attribute", default="Value")
    parser.add_argument(
        "--auto-provide-object-class",
        default="HLAobjectRoot.TckAutoProvideObject",
    )
    parser.add_argument("--auto-provide-first", default="ProviderAValue")
    parser.add_argument("--auto-provide-second", default="ProviderBValue")
    parser.add_argument(
        "--multi-attribute-object-class",
        default="HLAobjectRoot.TckMultiAttributeObject",
    )
    parser.add_argument("--multi-attribute-first", default="FirstValue")
    parser.add_argument("--multi-attribute-second", default="SecondValue")
    parser.add_argument(
        "--rate-object-class",
        default="HLAobjectRoot.TckRateObject",
    )
    parser.add_argument("--rate-reliable-attribute", default="ReliableValue")
    parser.add_argument("--rate-best-effort-attribute", default="BestEffortValue")
    parser.add_argument("--rate-designator", default="TckSlow")
    parser.add_argument("--interaction-class", default="HLAinteractionRoot.TckInteraction")
    parser.add_argument("--parameter", default="Payload")

    parser.add_argument("--rti-address", default="")
    parser.add_argument("--configuration-name", default="")
    parser.add_argument("--owner-configuration-name", default="")
    parser.add_argument("--member-configuration-name", default="")
    parser.add_argument("--additional-settings", default="")
    parser.add_argument("--federation-prefix", default="cpp-tck")
    parser.add_argument("--federation-name", default="")
    parser.add_argument("--owner-name", default="tck-owner")
    parser.add_argument("--member-name", default="tck-member")
    parser.add_argument("--federate-type", default="tck-type")

    parser.add_argument("--connection-loss-marker", default="")
    parser.add_argument("--connection-loss-server-managed", action="store_true")
    parser.add_argument(
        "--connection-loss-fixture",
        default="",
        help=(
            "Adapter-managed connection-loss fixture executable. When supplied, "
            "the Python adapter harness contributes the connection-loss evidence "
            "to the direct result instead of invoking that scenario in-process."
        ),
    )
    parser.add_argument("--connection-loss-fom", default="")
    parser.add_argument("--connection-loss-federation-name", default="")
    parser.add_argument("--connection-loss-owner-name", default="")
    parser.add_argument("--connection-loss-member-name", default="")
    parser.add_argument("--connection-loss-federate-type", default="")
    parser.add_argument("--connection-loss-owner-configuration-name", default="")
    parser.add_argument("--connection-loss-member-configuration-name", default="")
    parser.add_argument("--connection-loss-configuration-name", default="")
    parser.add_argument("--connection-loss-additional-settings", default="")
    parser.add_argument("--connection-loss-interaction-class", default="")
    parser.add_argument("--results", help="Write direct-run JSON evidence to this path")
    parser.add_argument("--junit", help="Write direct-run JUnit evidence to this path")
    parser.add_argument(
        "--skip-configure",
        action="store_true",
        help="Reuse an existing CMake configuration",
    )
    parser.add_argument(
        "--skip-build",
        action="store_true",
        help="Reuse an existing built executable",
    )
    parser.add_argument(
        "--skip-ctest",
        action="store_true",
        help="Do not run the configured CTest matrix",
    )
    return parser


def load_arguments(parser: argparse.ArgumentParser) -> argparse.Namespace:
    arguments = parser.parse_args()
    if arguments.build_parallel < 1:
        parser.error("--build-parallel must be at least 1")
    if arguments.timeout_ms < 1:
        parser.error("--timeout-ms must be positive")
    if not shutil.which(arguments.cmake):
        parser.error(f"CMake executable was not found: {arguments.cmake}")
    if not arguments.skip_ctest and not shutil.which(arguments.ctest):
        parser.error(f"CTest executable was not found: {arguments.ctest}")
    return arguments


def resolve_inputs(arguments: argparse.Namespace) -> dict[str, Any]:
    if arguments.connection_loss_marker:
        arguments.connection_loss_marker = str(
            resolve_path(
                arguments.connection_loss_marker,
                must_exist=False,
                label="connection-loss marker",
            )
        )
    if arguments.connection_loss_fixture:
        arguments.connection_loss_fixture = str(
            resolve_path(
                arguments.connection_loss_fixture,
                must_exist=True,
                label="connection-loss fixture",
            )
        )
    if arguments.connection_loss_fom:
        arguments.connection_loss_fom = str(
            resolve_path(
                arguments.connection_loss_fom,
                must_exist=True,
                label="connection-loss FOM",
            )
        )
    inputs: dict[str, Any] = {
        "package_prefix": resolve_directory(
            arguments.package_prefix,
            must_exist=True,
            label="package prefix",
        ),
        "build_directory": resolve_directory(
            arguments.build_directory,
            must_exist=False,
            label="build directory",
        ),
        "fom": resolve_path(arguments.fom, must_exist=True, label="FOM"),
        "model_fom": resolve_path(arguments.model_fom, must_exist=True, label="model FOM"),
        "ddm_fom": resolve_path(arguments.ddm_fom, must_exist=True, label="DDM FOM"),
        "multi_attribute_fom": resolve_path(
            arguments.multi_attribute_fom,
            must_exist=True,
            label="multi-attribute FOM",
        ),
        "three_dimensional_fom": resolve_path(
            arguments.three_dimensional_fom,
            must_exist=True,
            label="three-dimensional FOM",
        ),
        "mim_fom": resolve_path(arguments.mim_fom, must_exist=True, label="MIM FOM"),
        "switches_fom": resolve_path(
            arguments.switches_fom,
            must_exist=True,
            label="switch-declaration FOM",
        ),
        "auto_provide_fom": resolve_path(
            arguments.auto_provide_fom,
            must_exist=True,
            label="Auto Provide FOM",
        ),
    }
    ddm_dimensions = arguments.ddm_dimension or ["TckDimensionX", "TckDimensionY"]
    three_dimensions = arguments.three_dimensional_dimension or [
        "TckDimensionX",
        "TckDimensionY",
        "TckDimensionZ",
    ]
    if len(ddm_dimensions) < 2:
        raise ValueError("at least two --ddm-dimension values are required")
    if len(three_dimensions) < 3:
        raise ValueError("at least three --three-dimensional-dimension values are required")
    inputs["ddm_dimensions"] = ddm_dimensions
    inputs["three_dimensions"] = three_dimensions
    inputs["additional_fom"] = path_list(
        arguments.additional_fom,
        (FOM_ROOT / "fom-extension-tck.xml",),
        "additional FOM",
    )
    inputs["invalid_fom"] = path_list(
        arguments.invalid_fom,
        DEFAULT_INVALID_FOM,
        "invalid FOM",
    )
    return inputs


def cmake_definitions(arguments: argparse.Namespace, inputs: dict[str, Any]) -> list[str]:
    definitions = {
        "CMAKE_BUILD_TYPE": arguments.configuration,
        "BUILD_TESTING": "ON",
        "CMAKE_PREFIX_PATH": inputs["package_prefix"],
        "HLA_RTI_TCK_ADAPTER_FOM": inputs["fom"],
        "HLA_RTI_TCK_ADAPTER_MODEL_FOM": inputs["model_fom"],
        "HLA_RTI_TCK_ADAPTER_DDM_FOM": inputs["ddm_fom"],
        "HLA_RTI_TCK_ADAPTER_MULTI_ATTRIBUTE_FOM": inputs["multi_attribute_fom"],
        "HLA_RTI_TCK_ADAPTER_THREE_DIMENSIONAL_FOM": inputs["three_dimensional_fom"],
        "HLA_RTI_TCK_ADAPTER_MIM_FOM": inputs["mim_fom"],
        "HLA_RTI_TCK_ADAPTER_SWITCHES_FOM": inputs["switches_fom"],
        "HLA_RTI_TCK_ADAPTER_AUTO_PROVIDE_FOM": inputs["auto_provide_fom"],
        "HLA_RTI_TCK_ADAPTER_DDM_DIMENSIONS": ";".join(inputs["ddm_dimensions"]),
        "HLA_RTI_TCK_ADAPTER_THREE_DIMENSIONAL_DIMENSIONS": ";".join(
            inputs["three_dimensions"]
        ),
        "HLA_RTI_TCK_ADAPTER_THREE_DIMENSIONAL_OBJECT_CLASS": arguments.three_dimensional_object_class,
        "HLA_RTI_TCK_ADAPTER_THREE_DIMENSIONAL_ATTRIBUTE": arguments.three_dimensional_attribute,
        "HLA_RTI_TCK_ADAPTER_CUSTOM_REGIONAL_OBJECT_CLASS": arguments.custom_regional_object_class,
        "HLA_RTI_TCK_ADAPTER_CUSTOM_REGIONAL_ATTRIBUTE": arguments.custom_regional_attribute,
        "HLA_RTI_TCK_ADAPTER_CUSTOM_REGIONAL_INTERACTION_CLASS": arguments.custom_regional_interaction_class,
        "HLA_RTI_TCK_ADAPTER_CUSTOM_REGIONAL_PARAMETER": arguments.custom_regional_parameter,
        "HLA_RTI_TCK_ADAPTER_OBJECT_CLASS": arguments.object_class,
        "HLA_RTI_TCK_ADAPTER_ATTRIBUTE": arguments.attribute,
        "HLA_RTI_TCK_ADAPTER_AUTO_PROVIDE_OBJECT_CLASS": arguments.auto_provide_object_class,
        "HLA_RTI_TCK_ADAPTER_AUTO_PROVIDE_FIRST": arguments.auto_provide_first,
        "HLA_RTI_TCK_ADAPTER_AUTO_PROVIDE_SECOND": arguments.auto_provide_second,
        "HLA_RTI_TCK_ADAPTER_MULTI_ATTRIBUTE_OBJECT_CLASS": arguments.multi_attribute_object_class,
        "HLA_RTI_TCK_ADAPTER_MULTI_ATTRIBUTE_FIRST": arguments.multi_attribute_first,
        "HLA_RTI_TCK_ADAPTER_MULTI_ATTRIBUTE_SECOND": arguments.multi_attribute_second,
        "HLA_RTI_TCK_ADAPTER_INTERACTION_CLASS": arguments.interaction_class,
        "HLA_RTI_TCK_ADAPTER_PARAMETER": arguments.parameter,
        "HLA_RTI_TCK_ADAPTER_ADDITIONAL_FOM_MODULES": join_cmake_list(inputs["additional_fom"]),
        "HLA_RTI_TCK_ADAPTER_INVALID_FOM_MODULES": join_cmake_list(inputs["invalid_fom"]),
        "HLA_RTI_TCK_ADAPTER_CALLBACK_MODEL": arguments.callback_model,
        "HLA_RTI_TCK_ADAPTER_TIMEOUT_MS": str(arguments.timeout_ms),
        "HLA_RTI_TCK_ADAPTER_SCENARIO_SET": arguments.scenario_set,
        "HLA_RTI_TCK_ADAPTER_PROVIDER_ID": arguments.provider_id,
        "HLA_RTI_TCK_ADAPTER_TIME_IMPLEMENTATION": arguments.time_implementation,
    }
    optional = {
        "HLA_RTI_TCK_ADAPTER_RTI_ADDRESS": arguments.rti_address,
        "HLA_RTI_TCK_ADAPTER_CONFIGURATION_NAME": arguments.configuration_name,
        "HLA_RTI_TCK_ADAPTER_ADDITIONAL_SETTINGS": arguments.additional_settings,
        "HLA_RTI_TCK_ADAPTER_CONNECTION_LOSS_MARKER": arguments.connection_loss_marker,
    }
    definitions.update({key: value for key, value in optional.items() if value})
    if arguments.connection_loss_fixture:
        definitions[
            "HLA_RTI_TCK_ADAPTER_EXCLUDED_SCENARIOS"
        ] = CONNECTION_LOSS_SCENARIO
    if arguments.connection_loss_server_managed:
        definitions["HLA_RTI_TCK_ADAPTER_CONNECTION_LOSS_SERVER_MANAGED"] = "ON"
    return [f"-D{key}={value}" for key, value in definitions.items()]


def configure(arguments: argparse.Namespace, inputs: dict[str, Any]) -> None:
    build_directory: Path = inputs["build_directory"]
    command = [
        arguments.cmake,
        "-S",
        str(ADAPTER),
        "-B",
        str(build_directory),
    ]
    if arguments.generator:
        command.extend(["-G", arguments.generator])
    command.extend(cmake_definitions(arguments, inputs))
    run_command(command, cwd=ROOT)


def build(arguments: argparse.Namespace, inputs: dict[str, Any]) -> None:
    command = [
        arguments.cmake,
        "--build",
        str(inputs["build_directory"]),
        "--config",
        arguments.configuration,
        "--parallel",
        str(arguments.build_parallel),
    ]
    run_command(command, cwd=ROOT)


def run_ctest(arguments: argparse.Namespace, inputs: dict[str, Any]) -> None:
    command = [
        arguments.ctest,
        "--test-dir",
        str(inputs["build_directory"]),
        "-C",
        arguments.configuration,
        "-L",
        "^portable-cpp-tck$",
        "--output-on-failure",
    ]
    run_command(command, cwd=ROOT)


def find_executable(build_directory: Path, configuration: str) -> Path:
    names = ("hla_rti_cpp_tck.exe", "hla_rti_cpp_tck")
    preferred_roots = (
        build_directory / "portable-tck" / configuration,
        build_directory / "portable-tck",
        build_directory / configuration,
        build_directory,
    )
    for root in preferred_roots:
        for name in names:
            candidate = root / name
            if candidate.is_file():
                return candidate.resolve()
    candidates = sorted(
        candidate
        for name in names
        for candidate in build_directory.rglob(name)
        if candidate.is_file()
    )
    if candidates:
        return candidates[0].resolve()
    raise ValueError(
        "portable TCK executable was not found under "
        f"{build_directory}; configure/build the adapter first"
    )


def select_scenarios(arguments: argparse.Namespace, catalog: dict[str, Any]) -> list[dict[str, Any]]:
    by_id = {scenario["id"]: scenario for scenario in catalog["scenarios"]}
    if arguments.scenario:
        unknown = [scenario_id for scenario_id in arguments.scenario if scenario_id not in by_id]
        if unknown:
            raise ValueError("unknown catalog scenario(s): " + ", ".join(unknown))
        if len(set(arguments.scenario)) != len(arguments.scenario):
            raise ValueError("--scenario values must be unique")
        selected = [by_id[scenario_id] for scenario_id in arguments.scenario]
    else:
        selected = [
            scenario
            for scenario in catalog["scenarios"]
            if scenario["default_status"] != "unsupported"
            and (
                arguments.scenario_set == "all"
                or scenario["promotion"] == "promoted"
            )
        ]
    unsupported = [
        scenario["id"] for scenario in selected if scenario["default_status"] == "unsupported"
    ]
    if unsupported:
        raise ValueError("unsupported catalog scenario(s): " + ", ".join(unsupported))
    return selected


def direct_arguments(
    arguments: argparse.Namespace,
    inputs: dict[str, Any],
    selected: list[dict[str, Any]],
    results: Path | None,
    junit: Path | None,
) -> list[str]:
    command = [
        "--fom",
        str(inputs["fom"]),
        "--model-fom",
        str(inputs["model_fom"]),
        "--ddm-fom",
        str(inputs["ddm_fom"]),
        "--multi-attribute-fom",
        str(inputs["multi_attribute_fom"]),
        "--three-dimensional-fom",
        str(inputs["three_dimensional_fom"]),
        "--mim-fom",
        str(inputs["mim_fom"]),
        "--switches-fom",
        str(inputs["switches_fom"]),
        "--auto-provide-fom",
        str(inputs["auto_provide_fom"]),
        "--time-implementation",
        arguments.time_implementation,
        "--provider-id",
        arguments.provider_id,
        "--timeout-ms",
        str(arguments.timeout_ms),
        "--callback-model",
        arguments.callback_model,
        "--object-class",
        arguments.object_class,
        "--attribute",
        arguments.attribute,
        "--auto-provide-object-class",
        arguments.auto_provide_object_class,
        "--auto-provide-first",
        arguments.auto_provide_first,
        "--auto-provide-second",
        arguments.auto_provide_second,
        "--multi-attribute-object-class",
        arguments.multi_attribute_object_class,
        "--multi-attribute-first",
        arguments.multi_attribute_first,
        "--multi-attribute-second",
        arguments.multi_attribute_second,
        "--rate-object-class",
        arguments.rate_object_class,
        "--rate-reliable-attribute",
        arguments.rate_reliable_attribute,
        "--rate-best-effort-attribute",
        arguments.rate_best_effort_attribute,
        "--rate-designator",
        arguments.rate_designator,
        "--three-dimensional-object-class",
        arguments.three_dimensional_object_class,
        "--three-dimensional-attribute",
        arguments.three_dimensional_attribute,
        "--custom-regional-object-class",
        arguments.custom_regional_object_class,
        "--custom-regional-attribute",
        arguments.custom_regional_attribute,
        "--custom-regional-interaction-class",
        arguments.custom_regional_interaction_class,
        "--custom-regional-parameter",
        arguments.custom_regional_parameter,
        "--interaction-class",
        arguments.interaction_class,
        "--parameter",
        arguments.parameter,
        "--federation-prefix",
        arguments.federation_prefix,
        "--owner-name",
        arguments.owner_name,
        "--member-name",
        arguments.member_name,
        "--federate-type",
        arguments.federate_type,
    ]
    for flag, value in (
        ("--rti-address", arguments.rti_address),
        ("--configuration-name", arguments.configuration_name),
        ("--owner-configuration-name", arguments.owner_configuration_name),
        ("--member-configuration-name", arguments.member_configuration_name),
        ("--additional-settings", arguments.additional_settings),
        ("--federation-name", arguments.federation_name),
    ):
        if value:
            command.extend([flag, value])
    for path in inputs["additional_fom"]:
        command.extend(["--additional-fom", str(path)])
    for dimension in inputs["ddm_dimensions"]:
        command.extend(["--ddm-dimension", dimension])
    for dimension in inputs["three_dimensions"]:
        command.extend(["--three-dimensional-dimension", dimension])
    for path in inputs["invalid_fom"]:
        command.extend(["--invalid-fom", str(path)])
    for scenario in selected:
        command.extend(["--scenario", scenario["runner_id"]])
    if arguments.connection_loss_marker:
        command.extend(["--connection-loss-marker", arguments.connection_loss_marker])
    if arguments.connection_loss_server_managed:
        command.append("--connection-loss-server-managed")
    if results is not None:
        command.extend(["--results", str(results)])
    if junit is not None:
        command.extend(["--junit", str(junit)])
    return command


CONNECTION_LOSS_SCENARIO = "cpp-tck.connection-loss-cleanup"
CONNECTION_LOSS_ADAPTER = (
    ROOT / "packages" / "hla-rti-cpp-tck" / "adapters" / "current-process" /
    "run_connection_loss.py"
)


def run_executable_direct(
    arguments: argparse.Namespace,
    inputs: dict[str, Any],
    executable: Path,
    selected: list[dict[str, Any]],
    results: Path | None,
    junit: Path | None,
) -> None:
    command = [str(executable)] + direct_arguments(
        arguments,
        inputs,
        selected,
        results,
        junit,
    )
    run_command(command, cwd=ROOT)


def run_connection_loss_adapter(
    arguments: argparse.Namespace,
    inputs: dict[str, Any],
    executable: Path,
    results: Path,
    junit: Path,
) -> None:
    command = [
        sys.executable,
        str(CONNECTION_LOSS_ADAPTER),
        "--tck-executable",
        str(executable),
        "--process-fixture",
        arguments.connection_loss_fixture,
        "--callback-model",
        arguments.callback_model,
        "--provider-id",
        arguments.provider_id,
        "--timeout-ms",
        str(arguments.timeout_ms),
        "--results",
        str(results),
        "--junit",
        str(junit),
    ]
    optional_arguments = (
        ("--fom-path", arguments.connection_loss_fom),
        ("--federation-name", arguments.connection_loss_federation_name),
        ("--owner-name", arguments.connection_loss_owner_name),
        ("--member-name", arguments.connection_loss_member_name),
        ("--federate-type", arguments.connection_loss_federate_type),
        (
            "--owner-configuration-name",
            arguments.connection_loss_owner_configuration_name,
        ),
        (
            "--member-configuration-name",
            arguments.connection_loss_member_configuration_name,
        ),
        ("--configuration-name", arguments.connection_loss_configuration_name),
        ("--additional-settings", arguments.connection_loss_additional_settings),
        ("--interaction-class", arguments.connection_loss_interaction_class),
    )
    for flag, value in optional_arguments:
        if value:
            command.extend([flag, value])
    run_command(command, cwd=ROOT)


def run_connection_loss_check(
    arguments: argparse.Namespace,
    inputs: dict[str, Any],
    executable: Path,
) -> None:
    with tempfile.TemporaryDirectory(
        prefix="cpp-tck-connection-loss-",
        dir=inputs["build_directory"],
    ) as temporary_directory:
        temporary_root = Path(temporary_directory)
        run_connection_loss_adapter(
            arguments,
            inputs,
            executable,
            temporary_root / "connection-loss.json",
            temporary_root / "connection-loss.xml",
        )


def merge_json_evidence(
    base: Path | None,
    connection_loss: Path,
    output: Path,
) -> None:
    loss_payload = json.loads(connection_loss.read_text(encoding="utf-8"))
    if base is None:
        payload = loss_payload
    else:
        payload = json.loads(base.read_text(encoding="utf-8"))
        payload["results"] = list(payload.get("results", [])) + list(
            loss_payload.get("results", [])
        )
        payload["callback_model"] = loss_payload.get(
            "callback_model", payload.get("callback_model")
        )
    output.write_text(
        json.dumps(payload, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )


def merge_junit_evidence(
    base: Path | None,
    connection_loss: Path,
    output: Path,
) -> None:
    roots = []
    if base is not None:
        roots.append(ET.parse(base).getroot())
    roots.append(ET.parse(connection_loss).getroot())
    merged = ET.Element("testsuite")
    totals = {name: 0 for name in ("tests", "failures", "errors", "skipped")}
    total_time = 0.0
    for root in roots:
        suites = [root] if root.tag == "testsuite" else list(root)
        for suite in suites:
            for name in totals:
                try:
                    totals[name] += int(suite.attrib.get(name, "0"))
                except ValueError:
                    pass
            try:
                total_time += float(suite.attrib.get("time", "0"))
            except ValueError:
                pass
            for child in list(suite):
                merged.append(child)
    merged.attrib.update({name: str(value) for name, value in totals.items()})
    merged.set("time", str(total_time))
    ET.ElementTree(merged).write(output, encoding="utf-8", xml_declaration=True)


def run_direct(
    arguments: argparse.Namespace,
    inputs: dict[str, Any],
    catalog: dict[str, Any],
) -> dict[str, Any]:
    selected = select_scenarios(arguments, catalog)
    results = (
        resolve_path(arguments.results, must_exist=False, label="results output")
        if arguments.results
        else None
    )
    junit = (
        resolve_path(arguments.junit, must_exist=False, label="JUnit output")
        if arguments.junit
        else None
    )
    for output in (results, junit):
        if output is not None:
            output.parent.mkdir(parents=True, exist_ok=True)
    executable = find_executable(inputs["build_directory"], arguments.configuration)
    has_connection_loss = any(
        scenario["id"] == CONNECTION_LOSS_SCENARIO for scenario in selected
    )
    if arguments.connection_loss_fixture and has_connection_loss:
        direct_selected = [
            scenario
            for scenario in selected
            if scenario["id"] != CONNECTION_LOSS_SCENARIO
        ]
        with tempfile.TemporaryDirectory(
            prefix="cpp-tck-connection-loss-",
            dir=inputs["build_directory"],
        ) as temporary_directory:
            temporary_root = Path(temporary_directory)
            base_results = (
                temporary_root / "base.json"
                if results is not None and direct_selected
                else None
            )
            base_junit = (
                temporary_root / "base.xml"
                if junit is not None and direct_selected
                else None
            )
            loss_results = temporary_root / "connection-loss.json"
            loss_junit = temporary_root / "connection-loss.xml"
            if direct_selected:
                run_executable_direct(
                    arguments,
                    inputs,
                    executable,
                    direct_selected,
                    base_results,
                    base_junit,
                )
            run_connection_loss_adapter(
                arguments,
                inputs,
                executable,
                loss_results,
                loss_junit,
            )
            if results is not None:
                merge_json_evidence(base_results, loss_results, results)
            if junit is not None:
                merge_junit_evidence(base_junit, loss_junit, junit)
    else:
        run_executable_direct(arguments, inputs, executable, selected, results, junit)
    return {
        "executable": str(executable),
        "scenario_count": len(selected),
        "scenario_ids": [scenario["id"] for scenario in selected],
        "callback_model": arguments.callback_model,
        "results": str(results) if results is not None else None,
        "junit": str(junit) if junit is not None else None,
    }


def main() -> int:
    parser = build_parser()
    try:
        arguments = load_arguments(parser)
        inputs = resolve_inputs(arguments)
        catalog_path = resolve_path(DEFAULT_CATALOG, must_exist=True, label="catalog")
        catalog = load_catalog(catalog_path)
        selected = select_scenarios(arguments, catalog)
        if not arguments.skip_configure:
            configure(arguments, inputs)
        if not arguments.skip_build:
            build(arguments, inputs)
        if not arguments.skip_ctest:
            run_ctest(arguments, inputs)
            if (
                arguments.connection_loss_fixture
                and not (arguments.results or arguments.junit)
                and any(
                    scenario["id"] == CONNECTION_LOSS_SCENARIO
                    for scenario in selected
                )
            ):
                run_connection_loss_check(
                    arguments,
                    inputs,
                    find_executable(inputs["build_directory"], arguments.configuration),
                )
        direct_summary = None
        if arguments.results or arguments.junit:
            direct_summary = run_direct(arguments, inputs, catalog)
        summary = {
            "valid": True,
            "adapter": str(ADAPTER),
            "package_prefix": str(inputs["package_prefix"]),
            "build_directory": str(inputs["build_directory"]),
            "scenario_set": arguments.scenario_set,
            "configured_scenario_count": len(
                [
                    scenario
                    for scenario in catalog["scenarios"]
                    if scenario["default_status"] != "unsupported"
                    and (
                        arguments.scenario_set == "all"
                        or scenario["promotion"] == "promoted"
                    )
                ]
            ),
            "direct_scenario_count": len(selected) if (arguments.results or arguments.junit) else 0,
            "ctest": not arguments.skip_ctest,
            "direct": direct_summary,
        }
        print(json.dumps(summary, indent=2, sort_keys=True))
        return 0
    except (OSError, ValueError, json.JSONDecodeError, subprocess.CalledProcessError) as error:
        if isinstance(error, subprocess.CalledProcessError):
            message = f"command failed with exit code {error.returncode}: {command_text(error.cmd)}"
        else:
            message = str(error)
        print(json.dumps({"valid": False, "findings": [message]}, indent=2), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
