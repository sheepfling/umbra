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
DEFAULT_STANDARD_INVALID_FOM = (
    FOM_ROOT / "invalid-zero-update-rate-tck.xml",
    FOM_ROOT / "invalid-dimension-default-value-tck.xml",
    FOM_ROOT / "invalid-array-cardinality-tck.xml",
    FOM_ROOT / "invalid-variable-array-encoding-tck.xml",
    FOM_ROOT / "invalid-fixed-array-encoding-tck.xml",
    FOM_ROOT / "invalid-reference-object-name-tck.xml",
    FOM_ROOT / "invalid-reference-basic-representation-tck.xml",
)

# Keep direct child-process command lines below the limits imposed by common
# process launchers.  The C++ executable accepts repeated --scenario values,
# so large catalog runs can be partitioned without changing the test binary.
DIRECT_COMMAND_LENGTH_LIMIT = 24000


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


def command_length(command: list[str]) -> int:
    return len(os.fsencode(command_text(command)))


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
    parser.add_argument("--fom-custom-transportation", default="TckBestEffort")
    parser.add_argument(
        "--fom-additional-transportation",
        default="TckAdditionalTransport",
    )
    parser.add_argument("--fom-dimension", default="TckDimension")
    parser.add_argument(
        "--fom-additional-dimension",
        default="TckAdditionalDimension",
    )
    parser.add_argument(
        "--fom-additional-dimension-object-class",
        default="HLAobjectRoot.TckExtensionDimensionalObject",
    )
    parser.add_argument(
        "--fom-additional-dimension-attribute",
        default="AdditionalValue",
    )
    parser.add_argument(
        "--rate-fom",
        default=str(
            default_path("packages/hla-rti-cpp-tck/fom/attribute-relevance-rate-tck.xml")
        ),
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
        "--known-class-fom",
        default=str(
            default_path("packages/hla-rti-cpp-tck/fom/known-class-advisory-tck.xml")
        ),
    )
    parser.add_argument(
        "--known-class-disabled-fom",
        default=str(
            default_path(
                "packages/hla-rti-cpp-tck/fom/known-class-advisory-disabled-tck.xml"
            )
        ),
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
    parser.add_argument(
        "--standard-invalid-fom",
        action="append",
        default=[],
        help="Standard-invalid FOM module; repeat to replace the default corpus",
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
    parser.add_argument("--fom-update-rate", default="TckFast")
    parser.add_argument("--fom-additional-update-rate", default="TckAdditionalRate")
    parser.add_argument("--interaction-class", default="HLAinteractionRoot.TckInteraction")
    parser.add_argument("--parameter", default="Payload")
    parser.add_argument(
        "--known-class-object-class",
        default="HLAobjectRoot.TckKnownClassObject",
    )
    parser.add_argument(
        "--known-class-derived-object-class",
        default="HLAobjectRoot.TckKnownClassObject.TckKnownClassDerivedObject",
    )
    parser.add_argument("--known-class-known-attribute", default="Name")
    parser.add_argument("--known-class-derived-attribute", default="Efficiency")

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
    parser.add_argument("--connection-loss-object-class", default="")
    parser.add_argument("--connection-loss-attribute", default="")
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
        "rate_fom": resolve_path(
            arguments.rate_fom,
            must_exist=True,
            label="attribute relevance rate advisory FOM",
        ),
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
        "known_class_fom": resolve_path(
            arguments.known_class_fom,
            must_exist=True,
            label="known-class advisory FOM",
        ),
        "known_class_disabled_fom": resolve_path(
            arguments.known_class_disabled_fom,
            must_exist=True,
            label="known-class-disabled advisory FOM",
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
    inputs["standard_invalid_fom"] = path_list(
        arguments.standard_invalid_fom,
        DEFAULT_STANDARD_INVALID_FOM,
        "standard-invalid FOM",
    )
    return inputs


def cmake_definitions(arguments: argparse.Namespace, inputs: dict[str, Any]) -> list[str]:
    definitions = {
        "CMAKE_BUILD_TYPE": arguments.configuration,
        "BUILD_TESTING": "ON",
        "CMAKE_PREFIX_PATH": inputs["package_prefix"],
        "HLA_RTI_TCK_ADAPTER_FOM": inputs["fom"],
        "HLA_RTI_TCK_ADAPTER_MODEL_FOM": inputs["model_fom"],
        "HLA_RTI_TCK_ADAPTER_FOM_CUSTOM_TRANSPORTATION": arguments.fom_custom_transportation,
        "HLA_RTI_TCK_ADAPTER_FOM_ADDITIONAL_TRANSPORTATION": arguments.fom_additional_transportation,
        "HLA_RTI_TCK_ADAPTER_FOM_DIMENSION": arguments.fom_dimension,
        "HLA_RTI_TCK_ADAPTER_FOM_ADDITIONAL_DIMENSION": arguments.fom_additional_dimension,
        "HLA_RTI_TCK_ADAPTER_FOM_ADDITIONAL_DIMENSION_OBJECT_CLASS":
            arguments.fom_additional_dimension_object_class,
        "HLA_RTI_TCK_ADAPTER_FOM_ADDITIONAL_DIMENSION_ATTRIBUTE":
            arguments.fom_additional_dimension_attribute,
        "HLA_RTI_TCK_ADAPTER_FOM_ADDITIONAL_UPDATE_RATE":
            arguments.fom_additional_update_rate,
        "HLA_RTI_TCK_ADAPTER_RATE_FOM": inputs["rate_fom"],
        "HLA_RTI_TCK_ADAPTER_DDM_FOM": inputs["ddm_fom"],
        "HLA_RTI_TCK_ADAPTER_MULTI_ATTRIBUTE_FOM": inputs["multi_attribute_fom"],
        "HLA_RTI_TCK_ADAPTER_THREE_DIMENSIONAL_FOM": inputs["three_dimensional_fom"],
        "HLA_RTI_TCK_ADAPTER_MIM_FOM": inputs["mim_fom"],
        "HLA_RTI_TCK_ADAPTER_SWITCHES_FOM": inputs["switches_fom"],
        "HLA_RTI_TCK_ADAPTER_AUTO_PROVIDE_FOM": inputs["auto_provide_fom"],
        "HLA_RTI_TCK_ADAPTER_KNOWN_CLASS_FOM": inputs["known_class_fom"],
        "HLA_RTI_TCK_ADAPTER_KNOWN_CLASS_DISABLED_FOM": inputs[
            "known_class_disabled_fom"
        ],
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
        "HLA_RTI_TCK_ADAPTER_KNOWN_CLASS_OBJECT_CLASS": arguments.known_class_object_class,
        "HLA_RTI_TCK_ADAPTER_KNOWN_CLASS_DERIVED_OBJECT_CLASS": (
            arguments.known_class_derived_object_class
        ),
        "HLA_RTI_TCK_ADAPTER_KNOWN_CLASS_KNOWN_ATTRIBUTE": (
            arguments.known_class_known_attribute
        ),
        "HLA_RTI_TCK_ADAPTER_KNOWN_CLASS_DERIVED_ATTRIBUTE": (
            arguments.known_class_derived_attribute
        ),
        "HLA_RTI_TCK_ADAPTER_ADDITIONAL_FOM_MODULES": join_cmake_list(inputs["additional_fom"]),
        "HLA_RTI_TCK_ADAPTER_INVALID_FOM_MODULES": join_cmake_list(inputs["invalid_fom"]),
        "HLA_RTI_TCK_ADAPTER_STANDARD_INVALID_FOM_MODULES": join_cmake_list(inputs["standard_invalid_fom"]),
        "HLA_RTI_TCK_ADAPTER_CALLBACK_MODEL": arguments.callback_model,
        "HLA_RTI_TCK_ADAPTER_TIMEOUT_MS": str(arguments.timeout_ms),
        "HLA_RTI_TCK_ADAPTER_SCENARIO_SET": arguments.scenario_set,
        "HLA_RTI_TCK_ADAPTER_PROVIDER_ID": arguments.provider_id,
        "HLA_RTI_TCK_ADAPTER_TIME_IMPLEMENTATION": arguments.time_implementation,
        "HLA_RTI_TCK_ADAPTER_SCENARIO_FILTER": ";".join(arguments.scenario),
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
        ] = ";".join(CONNECTION_LOSS_SCENARIOS)
        definitions["HLA_RTI_TCK_ADAPTER_CONNECTION_LOSS_SERVER_MANAGED"] = "ON"
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
        "--fom-custom-transportation",
        arguments.fom_custom_transportation,
        "--fom-additional-transportation",
        arguments.fom_additional_transportation,
        "--fom-dimension",
        arguments.fom_dimension,
        "--fom-additional-dimension",
        arguments.fom_additional_dimension,
        "--fom-additional-dimension-object-class",
        arguments.fom_additional_dimension_object_class,
        "--fom-additional-dimension-attribute",
        arguments.fom_additional_dimension_attribute,
        "--rate-fom",
        str(inputs["rate_fom"]),
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
        "--known-class-fom",
        str(inputs["known_class_fom"]),
        "--known-class-disabled-fom",
        str(inputs["known_class_disabled_fom"]),
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
        "--fom-update-rate",
        arguments.fom_update_rate,
        "--fom-additional-update-rate",
        arguments.fom_additional_update_rate,
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
        "--known-class-object-class",
        arguments.known_class_object_class,
        "--known-class-derived-object-class",
        arguments.known_class_derived_object_class,
        "--known-class-known-attribute",
        arguments.known_class_known_attribute,
        "--known-class-derived-attribute",
        arguments.known_class_derived_attribute,
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
    for path in inputs["standard_invalid_fom"]:
        command.extend(["--standard-invalid-fom", str(path)])
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


def direct_scenario_chunks(
    arguments: argparse.Namespace,
    inputs: dict[str, Any],
    executable: Path,
    selected: list[dict[str, Any]],
    results: Path | None,
    junit: Path | None,
) -> list[list[dict[str, Any]]]:
    """Partition a direct run when its child-process command line is large."""
    if not selected:
        return []
    full_command = [str(executable)] + direct_arguments(
        arguments,
        inputs,
        selected,
        results,
        junit,
    )
    if command_length(full_command) <= DIRECT_COMMAND_LENGTH_LIMIT:
        return [selected]

    chunks: list[list[dict[str, Any]]] = []
    current: list[dict[str, Any]] = []
    for scenario in selected:
        candidate = current + [scenario]
        candidate_command = [str(executable)] + direct_arguments(
            arguments,
            inputs,
            candidate,
            results,
            junit,
        )
        if current and command_length(candidate_command) > DIRECT_COMMAND_LENGTH_LIMIT:
            chunks.append(current)
            current = [scenario]
            candidate_command = [str(executable)] + direct_arguments(
                arguments,
                inputs,
                current,
                results,
                junit,
            )
        else:
            current = candidate
        if command_length(candidate_command) > DIRECT_COMMAND_LENGTH_LIMIT:
            raise ValueError(
                "a single C++ TCK scenario command exceeds the direct "
                f"command-line budget of {DIRECT_COMMAND_LENGTH_LIMIT} bytes"
            )
    if current:
        chunks.append(current)
    return chunks


CONNECTION_LOSS_SCENARIO = "cpp-tck.connection-loss-cleanup"
CONNECTION_LOSS_SCENARIOS = (
    CONNECTION_LOSS_SCENARIO,
    "cpp-tck.connection-loss-cleanup-contract",
    "cpp-tck.connection-loss-automatic-unconditional-divestiture",
    "cpp-tck.connection-loss-automatic-unconditional-divestiture-contract",
    "cpp-tck.connection-loss-automatic-cancel-pending-acquisition",
    "cpp-tck.connection-loss-automatic-cancel-pending-acquisition-contract",
    "cpp-tck.automatic-resign-directive-delete-objects",
    "cpp-tck.automatic-resign-directive-delete-objects-contract",
)
PUBLIC_HANDLE_DECODING_SCENARIOS = (
    "cpp-tck.public-handle-decoding",
    "cpp-tck.public-handle-decoding-contract",
)
PORTABLE_DISPATCH_EXACT_SCENARIOS = (
    "java-tck.factory-discovery",
    "cpp-tck.rti-ambassador-factory-contract",
    "java-tck.logical-time-factory",
    "cpp-tck.variable-length-data-contract",
    "cpp-tck.logical-time-contract",
    "cpp-tck.logical-time-factory-factory-contract",
    "cpp-tck.logical-time-data-elements-contract",
    "cpp-tck.exception-hierarchy-contract",
    "cpp-tck.enum-contract",
    "cpp-tck.handle-and-collection-contract",
    "cpp-tck.data-element-contract",
    "cpp-tck.basic-data-elements-contract",
    "cpp-tck.composite-data-elements-contract",
    "cpp-tck.configuration-and-authorization-contract",
    "cpp-tck.runtime-identity-contract",
    "cpp-tck.authorizer-factory-factory-contract",
    "cpp-tck.null-federate-ambassador-contract",
    "java-tck.federation-membership",
    "cpp-tck.federation-lifecycle-contract",
    "cpp-tck.unnamed-join-overload",
    "cpp-tck.unnamed-join-overload-contract",
    "cpp-tck.federation-list-services",
    "cpp-tck.federation-list-services-contract",
    "cpp-tck.federate-lookup-lifecycle",
    "cpp-tck.federate-lookup-lifecycle-contract",
    "cpp-tck.explicit-mim-creation",
    "cpp-tck.explicit-mim-creation-contract",
    "cpp-tck.federation-mom-current-fdd",
    "cpp-tck.federation-mom-current-fdd-contract",
    "cpp-tck.federation-mom-content-reports",
    "cpp-tck.federation-mom-content-reports-contract",
    "cpp-tck.automatic-resign-directive-delete-objects",
    "cpp-tck.automatic-resign-directive-delete-objects-contract",
    "cpp-tck.mom-transportation-type-change-request",
    "cpp-tck.mom-transportation-type-change-request-contract",
    "cpp-tck.joined-federate-mom-registered-object-count",
    "cpp-tck.joined-federate-mom-registered-object-count-contract",
    "cpp-tck.joined-federate-mom-object-instances-that-can-be-deleted-report",
    "cpp-tck.joined-federate-mom-object-instances-that-can-be-deleted-report-contract",
    "cpp-tck.joined-federate-mom-deletable-object-count",
    "cpp-tck.joined-federate-mom-deletable-object-count-contract",
    "cpp-tck.receive-order-attribute-update",
    "cpp-tck.receive-order-attribute-update-contract",
    "cpp-tck.receive-order-interaction",
    "cpp-tck.receive-order-interaction-contract",
    "cpp-tck.receive-order-object-removal",
    "cpp-tck.receive-order-object-removal-contract",
    "cpp-tck.federation-restore-abort",
    "cpp-tck.federation-restore-abort-contract",
    "cpp-tck.federation-restore-work-item-ownership-assumption",
    "cpp-tck.federation-restore-work-item-ownership-assumption-contract",
    "cpp-tck.ownership-acquisition-if-available",
    "cpp-tck.ownership-acquisition-if-available-contract",
    "cpp-tck.attribute-ownership-acquisition-cancellation",
    "cpp-tck.attribute-ownership-acquisition-cancellation-contract",
    "cpp-tck.timestamped-directed-interaction-retraction",
    "cpp-tck.timestamped-directed-interaction-retraction-contract",
    "cpp-tck.timestamped-directed-interaction-retraction-fanout",
    "cpp-tck.timestamped-directed-interaction-retraction-fanout-contract",
    "cpp-tck.modify-lookahead",
    "cpp-tck.modify-lookahead-contract",
    "cpp-tck.transportation-type-change",
    "cpp-tck.transportation-type-change-contract",
    "cpp-tck.order-type-change",
    "cpp-tck.order-type-change-contract",
    "cpp-tck.transport-order",
    "cpp-tck.transport-order-contract",
    "java-tck.transport-order",
    "cpp-tck.standard-order-and-transportation-lookups",
    "cpp-tck.standard-order-and-transportation-lookups-contract",
    "java-tck.synchronization",
    "cpp-tck.synchronization-points",
    "cpp-tck.synchronization-point-contract",
    "cpp-tck.unconditional-attribute-ownership-divestiture",
    "cpp-tck.unconditional-attribute-ownership-divestiture-contract",
    "cpp-tck.auto-provide-disabled-discovery-only",
    "cpp-tck.auto-provide-disabled-discovery-only-contract",
    "cpp-tck.auto-provide-disabled-explicit-request",
    "cpp-tck.auto-provide-disabled-explicit-request-contract",
    "cpp-tck.object-registration-service-boundaries",
    "cpp-tck.object-registration-service-boundaries-contract",
    "cpp-tck.object-deletion-service-boundaries",
    "cpp-tck.object-deletion-service-boundaries-contract",
    "cpp-tck.attribute-update-service-boundaries",
    "cpp-tck.attribute-update-service-boundaries-contract",
    "cpp-tck.interaction-service-boundaries",
    "cpp-tck.interaction-service-boundaries-contract",
    "cpp-tck.attribute-value-request-service-boundaries",
    "cpp-tck.attribute-value-request-service-boundaries-contract",
    "cpp-tck.connection-service-boundaries",
    "cpp-tck.connection-service-boundaries-contract",
    "cpp-tck.ownership-service-boundaries",
    "cpp-tck.ownership-service-boundaries-contract",
    "cpp-tck.custom-transportation-directed-interaction-delivery",
    "cpp-tck.custom-transportation-directed-interaction-delivery-contract",
    "cpp-tck.federation-save-restore-interlocks",
    "cpp-tck.federation-save-restore-interlocks-contract",
    "cpp-tck.allow-relaxed-ddm",
    "cpp-tck.allow-relaxed-ddm-contract",
    "cpp-tck.ownership-transfer-regional-update",
    "cpp-tck.ownership-transfer-regional-update-contract",
)
# portable_tck.cpp routes these families through a dedicated top-level handler;
# keep them out of the ordinary multi-scenario command so that handler does
# not discard the other IDs in that command.
PORTABLE_DISPATCH_PREFIXES = (
    "cpp-tck.delay-subscription-evaluation-",
    "cpp-tck.timed-",
    "cpp-tck.regional-",
    "cpp-tck.timestamped-regional-",
    "cpp-tck.default-region-",
    "cpp-tck.passive-regional-",
    "cpp-tck.multi-region-",
    "cpp-tck.zero-dimensional-",
    "cpp-tck.ownership-transfer-",
)
CUSTOM_TRANSPORTATION_TIMESTAMPED_REGIONAL_INTERACTION_SCENARIOS = (
    "cpp-tck.custom-transportation-timestamped-regional-interaction-delivery",
    "cpp-tck.custom-transportation-timestamped-regional-interaction-delivery-contract",
)
CUSTOM_TRANSPORTATION_ATTRIBUTE_SCENARIOS = (
    "cpp-tck.custom-transportation-attribute-delivery",
    "cpp-tck.custom-transportation-attribute-delivery-contract",
)
CUSTOM_TRANSPORTATION_INTERACTION_SCENARIOS = (
    "cpp-tck.custom-transportation-interaction-delivery",
    "cpp-tck.custom-transportation-interaction-delivery-contract",
)
CUSTOM_TRANSPORTATION_TIMESTAMPED_DELIVERY_SCENARIOS = (
    "cpp-tck.custom-transportation-timestamped-delivery",
    "cpp-tck.custom-transportation-timestamped-delivery-contract",
)
CUSTOM_TRANSPORTATION_TIMESTAMPED_ATTRIBUTE_SCENARIOS = (
    "cpp-tck.custom-transportation-timestamped-attribute-delivery",
    "cpp-tck.custom-transportation-timestamped-attribute-delivery-contract",
)
CUSTOM_TRANSPORTATION_TIMESTAMPED_ATTRIBUTE_ALTERNATE_ADVANCES_SCENARIOS = (
    "cpp-tck.custom-transportation-timestamped-attribute-alternate-advances",
    "cpp-tck.custom-transportation-timestamped-attribute-alternate-advances-contract",
)
CUSTOM_TRANSPORTATION_TIMESTAMPED_INTERACTION_ALTERNATE_ADVANCES_SCENARIOS = (
    "cpp-tck.custom-transportation-timestamped-interaction-alternate-advances",
    "cpp-tck.custom-transportation-timestamped-interaction-alternate-advances-contract",
)
CUSTOM_TRANSPORTATION_TIMESTAMPED_DIRECTED_DELIVERY_SCENARIOS = (
    "cpp-tck.custom-transportation-timestamped-directed-delivery",
    "cpp-tck.custom-transportation-timestamped-directed-delivery-contract",
)
CUSTOM_TRANSPORTATION_TIMESTAMPED_DIRECTED_INTERACTION_ALTERNATE_ADVANCES_SCENARIOS = (
    "cpp-tck.custom-transportation-timestamped-directed-interaction-alternate-advances",
    "cpp-tck.custom-transportation-timestamped-directed-interaction-alternate-advances-contract",
)
CONNECTION_LOSS_ADAPTER = (
    ROOT / "packages" / "hla-rti-cpp-tck" / "adapters" / "current-process" /
    "run_connection_loss.py"
)


def is_connection_loss_scenario(scenario_id: str) -> bool:
    return scenario_id in CONNECTION_LOSS_SCENARIOS


def is_public_handle_decoding_scenario(scenario_id: str) -> bool:
    return scenario_id in PUBLIC_HANDLE_DECODING_SCENARIOS


def is_portable_dispatch_scenario(scenario_id: str) -> bool:
    return scenario_id in PORTABLE_DISPATCH_EXACT_SCENARIOS or any(
        scenario_id.startswith(prefix) for prefix in PORTABLE_DISPATCH_PREFIXES
    )


def is_custom_transportation_timestamped_regional_interaction_scenario(
    scenario_id: str,
) -> bool:
    return scenario_id in CUSTOM_TRANSPORTATION_TIMESTAMPED_REGIONAL_INTERACTION_SCENARIOS


def is_custom_transportation_attribute_scenario(scenario_id: str) -> bool:
    return scenario_id in CUSTOM_TRANSPORTATION_ATTRIBUTE_SCENARIOS


def is_custom_transportation_interaction_scenario(scenario_id: str) -> bool:
    return scenario_id in CUSTOM_TRANSPORTATION_INTERACTION_SCENARIOS


def is_custom_transportation_timestamped_delivery_scenario(scenario_id: str) -> bool:
    return scenario_id in CUSTOM_TRANSPORTATION_TIMESTAMPED_DELIVERY_SCENARIOS


def is_custom_transportation_timestamped_attribute_scenario(scenario_id: str) -> bool:
    return scenario_id in CUSTOM_TRANSPORTATION_TIMESTAMPED_ATTRIBUTE_SCENARIOS


def is_custom_transportation_timestamped_attribute_alternate_advances_scenario(
    scenario_id: str,
) -> bool:
    return scenario_id in CUSTOM_TRANSPORTATION_TIMESTAMPED_ATTRIBUTE_ALTERNATE_ADVANCES_SCENARIOS


def is_custom_transportation_timestamped_interaction_alternate_advances_scenario(
    scenario_id: str,
) -> bool:
    return scenario_id in CUSTOM_TRANSPORTATION_TIMESTAMPED_INTERACTION_ALTERNATE_ADVANCES_SCENARIOS


def is_custom_transportation_timestamped_directed_delivery_scenario(
    scenario_id: str,
) -> bool:
    return scenario_id in CUSTOM_TRANSPORTATION_TIMESTAMPED_DIRECTED_DELIVERY_SCENARIOS


def is_custom_transportation_timestamped_directed_interaction_alternate_advances_scenario(
    scenario_id: str,
) -> bool:
    return scenario_id in CUSTOM_TRANSPORTATION_TIMESTAMPED_DIRECTED_INTERACTION_ALTERNATE_ADVANCES_SCENARIOS


def run_executable_direct(
    arguments: argparse.Namespace,
    inputs: dict[str, Any],
    executable: Path,
    selected: list[dict[str, Any]],
    results: Path | None,
    junit: Path | None,
) -> None:
    chunks = direct_scenario_chunks(
        arguments,
        inputs,
        executable,
        selected,
        results,
        junit,
    )
    if len(chunks) <= 1:
        command = [str(executable)] + direct_arguments(
            arguments,
            inputs,
            selected,
            results,
            junit,
        )
        run_command(command, cwd=ROOT)
        return

    print(
        f"Direct run split into {len(chunks)} child-process batches "
        f"for {len(selected)} scenarios",
        flush=True,
    )
    with tempfile.TemporaryDirectory(
        prefix="cpp-tck-direct-",
        dir=inputs["build_directory"],
    ) as temporary_directory:
        temporary_root = Path(temporary_directory)
        result_parts: list[Path] = []
        junit_parts: list[Path] = []
        for index, chunk in enumerate(chunks):
            chunk_results = (
                temporary_root / f"chunk-{index}.json" if results is not None else None
            )
            chunk_junit = (
                temporary_root / f"chunk-{index}.xml" if junit is not None else None
            )
            command = [str(executable)] + direct_arguments(
                arguments,
                inputs,
                chunk,
                chunk_results,
                chunk_junit,
            )
            run_command(command, cwd=ROOT)
            if chunk_results is not None:
                result_parts.append(chunk_results)
            if chunk_junit is not None:
                junit_parts.append(chunk_junit)
        if results is not None:
            merge_json_evidence_parts(result_parts, results)
        if junit is not None:
            merge_junit_evidence_parts(junit_parts, junit)


def run_connection_loss_adapter(
    arguments: argparse.Namespace,
    inputs: dict[str, Any],
    executable: Path,
    scenario_id: str,
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
        "--scenario",
        scenario_id,
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
        ("--object-class", arguments.connection_loss_object_class),
        ("--attribute", arguments.connection_loss_attribute),
    )
    for flag, value in optional_arguments:
        if value:
            command.extend([flag, value])
    run_command(command, cwd=ROOT)


def run_connection_loss_check(
    arguments: argparse.Namespace,
    inputs: dict[str, Any],
    executable: Path,
    scenario_ids: list[str],
) -> None:
    with tempfile.TemporaryDirectory(
        prefix="cpp-tck-connection-loss-",
        dir=inputs["build_directory"],
    ) as temporary_directory:
        temporary_root = Path(temporary_directory)
        for index, scenario_id in enumerate(scenario_ids):
            run_connection_loss_adapter(
                arguments,
                inputs,
                executable,
                scenario_id,
                temporary_root / f"connection-loss-{index}.json",
                temporary_root / f"connection-loss-{index}.xml",
            )


def merge_json_evidence_parts(parts: Iterable[Path], output: Path) -> None:
    paths = list(parts)
    if not paths:
        raise ValueError("cannot merge empty JSON evidence")
    payload = json.loads(paths[0].read_text(encoding="utf-8"))
    for part in paths[1:]:
        next_payload = json.loads(part.read_text(encoding="utf-8"))
        payload["results"] = list(payload.get("results", [])) + list(
            next_payload.get("results", [])
        )
        payload["callback_model"] = next_payload.get(
            "callback_model", payload.get("callback_model")
        )
    output.write_text(
        json.dumps(payload, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )


def merge_json_evidence(
    base: Path | None,
    connection_loss: Path,
    output: Path,
) -> None:
    parts = [connection_loss] if base is None else [base, connection_loss]
    merge_json_evidence_parts(parts, output)


def merge_junit_evidence_parts(parts: Iterable[Path], output: Path) -> None:
    roots = []
    for part in parts:
        roots.append(ET.parse(part).getroot())
    if not roots:
        raise ValueError("cannot merge empty JUnit evidence")
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


def merge_junit_evidence(
    base: Path | None,
    connection_loss: Path,
    output: Path,
) -> None:
    parts = [connection_loss] if base is None else [base, connection_loss]
    merge_junit_evidence_parts(parts, output)


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
    connection_loss_selected = [
        scenario
        for scenario in selected
        if is_connection_loss_scenario(scenario["id"])
    ]
    public_handle_decoding_selected = [
        scenario
        for scenario in selected
        if is_public_handle_decoding_scenario(scenario["id"])
    ]
    custom_transportation_timestamped_regional_interaction_selected = [
        scenario
        for scenario in selected
        if is_custom_transportation_timestamped_regional_interaction_scenario(
            scenario["id"]
        )
    ]
    custom_transportation_attribute_selected = [
        scenario
        for scenario in selected
        if is_custom_transportation_attribute_scenario(scenario["id"])
    ]
    custom_transportation_interaction_selected = [
        scenario
        for scenario in selected
        if is_custom_transportation_interaction_scenario(scenario["id"])
    ]
    custom_transportation_timestamped_delivery_selected = [
        scenario
        for scenario in selected
        if is_custom_transportation_timestamped_delivery_scenario(scenario["id"])
    ]
    custom_transportation_timestamped_attribute_selected = [
        scenario
        for scenario in selected
        if is_custom_transportation_timestamped_attribute_scenario(scenario["id"])
    ]
    custom_transportation_timestamped_attribute_alternate_advances_selected = [
        scenario
        for scenario in selected
        if is_custom_transportation_timestamped_attribute_alternate_advances_scenario(
            scenario["id"]
        )
    ]
    custom_transportation_timestamped_interaction_alternate_advances_selected = [
        scenario
        for scenario in selected
        if is_custom_transportation_timestamped_interaction_alternate_advances_scenario(
            scenario["id"]
        )
    ]
    custom_transportation_timestamped_directed_delivery_selected = [
        scenario
        for scenario in selected
        if is_custom_transportation_timestamped_directed_delivery_scenario(scenario["id"])
    ]
    custom_transportation_timestamped_directed_interaction_alternate_advances_selected = [
        scenario
        for scenario in selected
        if is_custom_transportation_timestamped_directed_interaction_alternate_advances_scenario(
            scenario["id"]
        )
    ]
    managed_connection_loss_selected = (
        connection_loss_selected if arguments.connection_loss_fixture else []
    )
    portable_dispatch_selected = [
        scenario
        for scenario in selected
        if is_portable_dispatch_scenario(scenario["id"])
        and not (
            arguments.connection_loss_fixture
            and is_connection_loss_scenario(scenario["id"])
        )
    ]
    special_selected = (
        public_handle_decoding_selected
        + custom_transportation_attribute_selected
        + custom_transportation_interaction_selected
        + custom_transportation_timestamped_delivery_selected
        + custom_transportation_timestamped_attribute_selected
        + custom_transportation_timestamped_attribute_alternate_advances_selected
        + custom_transportation_timestamped_interaction_alternate_advances_selected
        + custom_transportation_timestamped_directed_delivery_selected
        + custom_transportation_timestamped_directed_interaction_alternate_advances_selected
        + custom_transportation_timestamped_regional_interaction_selected
        + portable_dispatch_selected
        + managed_connection_loss_selected
    )
    if special_selected:
        special_ids = {scenario["id"] for scenario in special_selected}
        direct_selected = [
            scenario
            for scenario in selected
            if scenario["id"] not in special_ids
        ]
        with tempfile.TemporaryDirectory(
            prefix="cpp-tck-special-slices-",
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
            public_results_parts: list[Path] = []
            public_junit_parts: list[Path] = []
            for index, scenario in enumerate(public_handle_decoding_selected):
                public_results = temporary_root / f"public-handle-decoding-{index}.json"
                public_junit = temporary_root / f"public-handle-decoding-{index}.xml"
                run_executable_direct(
                    arguments,
                    inputs,
                    executable,
                    [scenario],
                    public_results if results is not None else None,
                    public_junit if junit is not None else None,
                )
                if results is not None:
                    public_results_parts.append(public_results)
                if junit is not None:
                    public_junit_parts.append(public_junit)
            custom_transportation_attribute_results_parts: list[Path] = []
            custom_transportation_attribute_junit_parts: list[Path] = []
            for index, scenario in enumerate(custom_transportation_attribute_selected):
                custom_results = temporary_root / (
                    f"custom-transportation-attribute-{index}.json"
                )
                custom_junit = temporary_root / (
                    f"custom-transportation-attribute-{index}.xml"
                )
                run_executable_direct(
                    arguments,
                    inputs,
                    executable,
                    [scenario],
                    custom_results if results is not None else None,
                    custom_junit if junit is not None else None,
                )
                if results is not None:
                    custom_transportation_attribute_results_parts.append(custom_results)
                if junit is not None:
                    custom_transportation_attribute_junit_parts.append(custom_junit)
            custom_transportation_interaction_results_parts: list[Path] = []
            custom_transportation_interaction_junit_parts: list[Path] = []
            for index, scenario in enumerate(custom_transportation_interaction_selected):
                custom_results = temporary_root / (
                    f"custom-transportation-interaction-{index}.json"
                )
                custom_junit = temporary_root / (
                    f"custom-transportation-interaction-{index}.xml"
                )
                run_executable_direct(
                    arguments,
                    inputs,
                    executable,
                    [scenario],
                    custom_results if results is not None else None,
                    custom_junit if junit is not None else None,
                )
                if results is not None:
                    custom_transportation_interaction_results_parts.append(custom_results)
                if junit is not None:
                    custom_transportation_interaction_junit_parts.append(custom_junit)
            custom_transportation_timestamped_delivery_results_parts: list[Path] = []
            custom_transportation_timestamped_delivery_junit_parts: list[Path] = []
            for index, scenario in enumerate(custom_transportation_timestamped_delivery_selected):
                custom_results = temporary_root / (
                    f"custom-transportation-timestamped-delivery-{index}.json"
                )
                custom_junit = temporary_root / (
                    f"custom-transportation-timestamped-delivery-{index}.xml"
                )
                run_executable_direct(
                    arguments,
                    inputs,
                    executable,
                    [scenario],
                    custom_results if results is not None else None,
                    custom_junit if junit is not None else None,
                )
                if results is not None:
                    custom_transportation_timestamped_delivery_results_parts.append(custom_results)
                if junit is not None:
                    custom_transportation_timestamped_delivery_junit_parts.append(custom_junit)
            custom_transportation_timestamped_attribute_results_parts: list[Path] = []
            custom_transportation_timestamped_attribute_junit_parts: list[Path] = []
            for index, scenario in enumerate(
                custom_transportation_timestamped_attribute_selected
            ):
                custom_results = temporary_root / (
                    f"custom-transportation-timestamped-attribute-{index}.json"
                )
                custom_junit = temporary_root / (
                    f"custom-transportation-timestamped-attribute-{index}.xml"
                )
                run_executable_direct(
                    arguments,
                    inputs,
                    executable,
                    [scenario],
                    custom_results if results is not None else None,
                    custom_junit if junit is not None else None,
                )
                if results is not None:
                    custom_transportation_timestamped_attribute_results_parts.append(
                        custom_results
                    )
                if junit is not None:
                    custom_transportation_timestamped_attribute_junit_parts.append(
                        custom_junit
                    )
            custom_transportation_timestamped_attribute_alternate_advances_results_parts: list[Path] = []
            custom_transportation_timestamped_attribute_alternate_advances_junit_parts: list[Path] = []
            for index, scenario in enumerate(
                custom_transportation_timestamped_attribute_alternate_advances_selected
            ):
                custom_results = temporary_root / (
                    f"custom-transportation-timestamped-attribute-alternate-advances-{index}.json"
                )
                custom_junit = temporary_root / (
                    f"custom-transportation-timestamped-attribute-alternate-advances-{index}.xml"
                )
                run_executable_direct(
                    arguments,
                    inputs,
                    executable,
                    [scenario],
                    custom_results if results is not None else None,
                    custom_junit if junit is not None else None,
                )
                if results is not None:
                    custom_transportation_timestamped_attribute_alternate_advances_results_parts.append(
                        custom_results
                    )
                if junit is not None:
                    custom_transportation_timestamped_attribute_alternate_advances_junit_parts.append(
                        custom_junit
                    )
            custom_transportation_timestamped_interaction_alternate_advances_results_parts: list[Path] = []
            custom_transportation_timestamped_interaction_alternate_advances_junit_parts: list[Path] = []
            for index, scenario in enumerate(
                custom_transportation_timestamped_interaction_alternate_advances_selected
            ):
                custom_results = temporary_root / (
                    f"custom-transportation-timestamped-interaction-alternate-advances-{index}.json"
                )
                custom_junit = temporary_root / (
                    f"custom-transportation-timestamped-interaction-alternate-advances-{index}.xml"
                )
                run_executable_direct(
                    arguments,
                    inputs,
                    executable,
                    [scenario],
                    custom_results if results is not None else None,
                    custom_junit if junit is not None else None,
                )
                if results is not None:
                    custom_transportation_timestamped_interaction_alternate_advances_results_parts.append(
                        custom_results
                    )
                if junit is not None:
                    custom_transportation_timestamped_interaction_alternate_advances_junit_parts.append(
                        custom_junit
                    )
            custom_transportation_timestamped_directed_delivery_results_parts: list[Path] = []
            custom_transportation_timestamped_directed_delivery_junit_parts: list[Path] = []
            for index, scenario in enumerate(custom_transportation_timestamped_directed_delivery_selected):
                custom_results = temporary_root / (
                    f"custom-transportation-timestamped-directed-delivery-{index}.json"
                )
                custom_junit = temporary_root / (
                    f"custom-transportation-timestamped-directed-delivery-{index}.xml"
                )
                run_executable_direct(
                    arguments,
                    inputs,
                    executable,
                    [scenario],
                    custom_results if results is not None else None,
                    custom_junit if junit is not None else None,
                )
                if results is not None:
                    custom_transportation_timestamped_directed_delivery_results_parts.append(
                        custom_results
                    )
                if junit is not None:
                    custom_transportation_timestamped_directed_delivery_junit_parts.append(
                        custom_junit
                    )
            custom_transportation_timestamped_directed_interaction_alternate_advances_results_parts: list[Path] = []
            custom_transportation_timestamped_directed_interaction_alternate_advances_junit_parts: list[Path] = []
            for index, scenario in enumerate(
                custom_transportation_timestamped_directed_interaction_alternate_advances_selected
            ):
                custom_results = temporary_root / (
                    f"custom-transportation-timestamped-directed-interaction-alternate-advances-{index}.json"
                )
                custom_junit = temporary_root / (
                    f"custom-transportation-timestamped-directed-interaction-alternate-advances-{index}.xml"
                )
                run_executable_direct(
                    arguments,
                    inputs,
                    executable,
                    [scenario],
                    custom_results if results is not None else None,
                    custom_junit if junit is not None else None,
                )
                if results is not None:
                    custom_transportation_timestamped_directed_interaction_alternate_advances_results_parts.append(
                        custom_results
                    )
                if junit is not None:
                    custom_transportation_timestamped_directed_interaction_alternate_advances_junit_parts.append(
                        custom_junit
                    )
            custom_transportation_timestamped_regional_interaction_results_parts: list[Path] = []
            custom_transportation_timestamped_regional_interaction_junit_parts: list[Path] = []
            for index, scenario in enumerate(
                custom_transportation_timestamped_regional_interaction_selected
            ):
                custom_results = temporary_root / (
                    f"custom-transportation-timestamped-regional-interaction-{index}.json"
                )
                custom_junit = temporary_root / (
                    f"custom-transportation-timestamped-regional-interaction-{index}.xml"
                )
                run_executable_direct(
                    arguments,
                    inputs,
                    executable,
                    [scenario],
                    custom_results if results is not None else None,
                    custom_junit if junit is not None else None,
                )
                if results is not None:
                    custom_transportation_timestamped_regional_interaction_results_parts.append(
                        custom_results
                    )
                if junit is not None:
                    custom_transportation_timestamped_regional_interaction_junit_parts.append(
                        custom_junit
                    )
            portable_dispatch_results_parts: list[Path] = []
            portable_dispatch_junit_parts: list[Path] = []
            for index, scenario in enumerate(portable_dispatch_selected):
                portable_results = temporary_root / f"portable-dispatch-{index}.json"
                portable_junit = temporary_root / f"portable-dispatch-{index}.xml"
                run_executable_direct(
                    arguments,
                    inputs,
                    executable,
                    [scenario],
                    portable_results if results is not None else None,
                    portable_junit if junit is not None else None,
                )
                if results is not None:
                    portable_dispatch_results_parts.append(portable_results)
                if junit is not None:
                    portable_dispatch_junit_parts.append(portable_junit)
            loss_results_parts: list[Path] = []
            loss_junit_parts: list[Path] = []
            if direct_selected:
                run_executable_direct(
                    arguments,
                    inputs,
                    executable,
                    direct_selected,
                    base_results,
                    base_junit,
                )
            for index, scenario in enumerate(managed_connection_loss_selected):
                loss_results = temporary_root / f"connection-loss-{index}.json"
                loss_junit = temporary_root / f"connection-loss-{index}.xml"
                run_connection_loss_adapter(
                    arguments,
                    inputs,
                    executable,
                    scenario["id"],
                    loss_results,
                    loss_junit,
                )
                loss_results_parts.append(loss_results)
                loss_junit_parts.append(loss_junit)
            if results is not None:
                json_parts = (
                    [base_results] if base_results is not None else []
                ) + public_results_parts + custom_transportation_attribute_results_parts + (
                    custom_transportation_interaction_results_parts
                ) + (
                    custom_transportation_timestamped_delivery_results_parts
                ) + (
                    custom_transportation_timestamped_attribute_results_parts
                ) + (
                    custom_transportation_timestamped_attribute_alternate_advances_results_parts
                ) + (
                    custom_transportation_timestamped_interaction_alternate_advances_results_parts
                ) + (
                    custom_transportation_timestamped_directed_delivery_results_parts
                ) + (
                    custom_transportation_timestamped_directed_interaction_alternate_advances_results_parts
                ) + (
                    custom_transportation_timestamped_regional_interaction_results_parts
                ) + portable_dispatch_results_parts + loss_results_parts
                merge_json_evidence_parts(json_parts, results)
            if junit is not None:
                junit_parts = (
                    [base_junit] if base_junit is not None else []
                ) + public_junit_parts + custom_transportation_attribute_junit_parts + (
                    custom_transportation_interaction_junit_parts
                ) + (
                    custom_transportation_timestamped_delivery_junit_parts
                ) + (
                    custom_transportation_timestamped_attribute_junit_parts
                ) + (
                    custom_transportation_timestamped_attribute_alternate_advances_junit_parts
                ) + (
                    custom_transportation_timestamped_interaction_alternate_advances_junit_parts
                ) + (
                    custom_transportation_timestamped_directed_delivery_junit_parts
                ) + (
                    custom_transportation_timestamped_directed_interaction_alternate_advances_junit_parts
                ) + (
                    custom_transportation_timestamped_regional_interaction_junit_parts
                ) + portable_dispatch_junit_parts + loss_junit_parts
                merge_junit_evidence_parts(junit_parts, junit)
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
                    is_connection_loss_scenario(scenario["id"])
                    for scenario in selected
                )
            ):
                run_connection_loss_check(
                    arguments,
                    inputs,
                    find_executable(inputs["build_directory"], arguments.configuration),
                    [
                        scenario["id"]
                        for scenario in selected
                        if is_connection_loss_scenario(scenario["id"])
                    ],
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
