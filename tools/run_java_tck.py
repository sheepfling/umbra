"""Build and run the portable IEEE 1516.1-2025 Java TCK.

The runner uses only Python's standard library and starts ``javac``/``java``
without a shell.  Provider JARs, FOM/MIM modules, capability profiles, and
JVM arguments stay outside the portable Java sources.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import shlex
import subprocess
import sys
import tempfile
from collections.abc import Iterable
from dataclasses import dataclass
from pathlib import Path
from types import NoneType
from typing import TypeAlias, cast

ROOT = Path(__file__).resolve().parents[1]
TCK_ROOT = ROOT / "packages" / "hla-rti-java-tck"
SOURCE_ROOT = TCK_ROOT / "src" / "main" / "java"
DEFAULT_CLASSES = ROOT / "out" / "java-tck" / "classes"
DEFAULT_API_PACKAGE_PREFIX = "hla.rti1516_2025"


class RunnerError(ValueError):
    """A user-correctable runner configuration or tool error."""


JsonScalar: TypeAlias = NoneType | bool | int | float | str
JsonValue: TypeAlias = JsonScalar | list["JsonValue"] | dict[str, "JsonValue"]
JsonObject: TypeAlias = dict[str, JsonValue]


@dataclass(frozen=True, slots=True)
class _ProviderConfiguration:
    identifier: str
    api_jar: str
    provider_jars: tuple[str, ...]
    factory_name: str
    fom_path: str
    capability_profile: str | None
    dependency_jars: tuple[str, ...]
    api_package_prefix: str
    run_unverified: bool
    mim_path: str | None
    jvm_arguments: tuple[str, ...]
    time_implementation: str


def resolve_path(
    value: str | Path,
    *,
    base: Path | None = None,
    must_exist: bool = True,
    label: str,
) -> Path:
    path = Path(value)
    if not path.is_absolute():
        path = (base if base is not None else Path.cwd()) / path
    path = path.resolve()
    if must_exist and not path.is_file():
        raise RunnerError(f"{label} does not exist: {path}")
    return path


def resolve_directory(
    value: str | Path,
    *,
    base: Path | None = None,
    must_exist: bool = True,
    label: str,
) -> Path:
    path = Path(value)
    if not path.is_absolute():
        path = (base if base is not None else Path.cwd()) / path
    path = path.resolve()
    if must_exist and not path.is_dir():
        raise RunnerError(f"{label} does not exist: {path}")
    return path


def command_text(command: list[str]) -> str:
    if os.name == "nt":
        return subprocess.list2cmdline(command)
    return shlex.join(command)


def run_command(command: list[str], *, cwd: Path) -> int:
    print(f"+ {command_text(command)}", flush=True)
    try:
        completed = subprocess.run(command, cwd=cwd, check=False)
    except FileNotFoundError as error:
        raise RunnerError(
            f"could not start {command[0]!r}; install it or pass its path"
        ) from error
    return completed.returncode


def java_sources() -> list[Path]:
    sources = sorted(SOURCE_ROOT.rglob("*.java"))
    if not sources:
        raise RunnerError(f"no Java TCK sources found below {SOURCE_ROOT}")
    return sources


def validate_api_package_prefix(value: str) -> str:
    if not re.fullmatch(
        r"[A-Za-z_$][A-Za-z0-9_$]*(\.[A-Za-z_$][A-Za-z0-9_$]*)*", value
    ):
        raise RunnerError(f"invalid Java API package prefix: {value!r}")
    return value


def build_tck(
    *,
    api_jar: Path,
    classes_directory: Path,
    javac: str,
    api_package_prefix: str = DEFAULT_API_PACKAGE_PREFIX,
) -> int:
    classes_directory.mkdir(parents=True, exist_ok=True)
    api_package_prefix = validate_api_package_prefix(api_package_prefix)
    with tempfile.TemporaryDirectory(prefix="java-tck-javac-") as temporary:
        temporary_path = Path(temporary)
        sources = java_sources()
        if api_package_prefix != DEFAULT_API_PACKAGE_PREFIX:
            generated_source_root = temporary_path / "src"
            generated_sources = []
            for source in sources:
                generated = generated_source_root / source.relative_to(SOURCE_ROOT)
                generated.parent.mkdir(parents=True, exist_ok=True)
                text = source.read_text(encoding="utf-8")
                generated.write_text(
                    text.replace(DEFAULT_API_PACKAGE_PREFIX, api_package_prefix),
                    encoding="utf-8",
                    newline="\n",
                )
                generated_sources.append(generated)
            sources = generated_sources

        arguments = [
            "-encoding",
            "UTF-8",
            "-source",
            "11",
            "-target",
            "11",
            "-cp",
            str(api_jar),
            "-d",
            str(classes_directory),
            *(str(path) for path in sources),
        ]
        # Windows command lines have a relatively small size limit. Keep the
        # full source inventory in a javac argument file.
        argument_file = temporary_path / "javac.args"
        quoted_arguments = []
        for argument in arguments:
            portable = argument.replace("\\", "/")
            quoted_arguments.append('"' + portable.replace('"', '\\"') + '"')
        argument_file.write_text("\n".join(quoted_arguments) + "\n", encoding="utf-8")
        return run_command([javac, "@" + str(argument_file)], cwd=ROOT)


def add_property(command: list[str], name: str, value: str | None) -> None:
    if value is not None and value != "":
        command.append(f"-D{name}={value}")


def run_tck(
    *,
    api_jar: Path,
    provider_jars: list[Path],
    dependency_jars: list[Path],
    fom_path: Path,
    classes_directory: Path,
    java: str,
    javac: str,
    mim_path: Path | None = None,
    factory_name: str = "",
    time_implementation: str = "HLAinteger64Time",
    capability_profile: Path | None = None,
    scenario: str = "",
    run_unverified: bool = False,
    results_path: Path | None = None,
    junit_path: Path | None = None,
    provider_id: str = "",
    jvm_arguments: Iterable[str] = (),
    skip_build: bool = False,
    api_package_prefix: str = DEFAULT_API_PACKAGE_PREFIX,
) -> int:
    if not provider_jars:
        raise RunnerError("at least one provider JAR is required")
    if not classes_directory.is_dir():
        if skip_build:
            raise RunnerError(f"classes directory does not exist: {classes_directory}")
        classes_directory.mkdir(parents=True, exist_ok=True)
    if not skip_build:
        result = build_tck(
            api_jar=api_jar,
            classes_directory=classes_directory,
            javac=javac,
            api_package_prefix=api_package_prefix,
        )
        if result != 0:
            return result

    runtime_jars = [*provider_jars, *dependency_jars]
    classpath = os.pathsep.join(
        str(path) for path in [classes_directory, api_jar, *runtime_jars]
    )
    command = [java, *jvm_arguments, "-cp", classpath]
    add_property(command, "hla.rti.tck.apiJar", str(api_jar))
    add_property(
        command,
        "hla.rti.tck.providerJars",
        os.pathsep.join(str(path) for path in provider_jars),
    )
    add_property(command, "hla.rti.tck.fom", str(fom_path))
    add_property(command, "hla.rti.tck.time", time_implementation)
    add_property(command, "hla.rti.tck.factory", factory_name)
    add_property(command, "hla.rti.tck.mim", str(mim_path) if mim_path else None)
    add_property(
        command,
        "hla.rti.tck.capabilityProfile",
        str(capability_profile) if capability_profile else None,
    )
    add_property(command, "hla.rti.tck.scenario", scenario)
    if run_unverified:
        add_property(command, "hla.rti.tck.runUnverified", "true")
    add_property(
        command, "hla.rti.tck.results", str(results_path) if results_path else None
    )
    add_property(command, "hla.rti.tck.junit", str(junit_path) if junit_path else None)
    add_property(command, "hla.rti.tck.provider", provider_id)
    command.append("org.hla.rti.tck.RtiTckMain")
    return run_command(command, cwd=ROOT)


def add_common_run_arguments(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--api-jar", required=True, help="official IEEE Java API JAR")
    parser.add_argument(
        "--provider-jar",
        action="append",
        required=True,
        help="provider JAR; repeat for additional provider JARs",
    )
    parser.add_argument(
        "--dependency-jar",
        action="append",
        default=[],
        help="provider dependency JAR; repeat as needed",
    )
    parser.add_argument("--fom-path", required=True, help="adapter-supplied FOM module")
    parser.add_argument(
        "--mim-path", default="", help="optional adapter-supplied MIM module"
    )
    parser.add_argument("--factory-name", default="", help="standard factory name")
    parser.add_argument(
        "--api-package-prefix",
        default=DEFAULT_API_PACKAGE_PREFIX,
        help="standard API package prefix used by the adapter",
    )
    parser.add_argument("--time-implementation", default="HLAinteger64Time")
    parser.add_argument("--capability-profile", default="")
    parser.add_argument(
        "--scenario", default="", help="run only this catalog scenario ID"
    )
    parser.add_argument(
        "--run-unverified",
        action="store_true",
        help="attempt default-disabled scenarios; explicit profile exclusions still apply",
    )
    parser.add_argument(
        "--classes-directory",
        default=str(DEFAULT_CLASSES),
        help=f"compiled classes directory (default: {DEFAULT_CLASSES})",
    )
    parser.add_argument("--results-path", default="")
    parser.add_argument("--junit-path", default="")
    parser.add_argument("--provider-id", default="")
    parser.add_argument(
        "--jvm-argument",
        action="append",
        default=[],
        help="additional JVM argument or TCK fixture property; repeat as needed",
    )
    parser.add_argument("--java", default="java", help="java executable")
    parser.add_argument("--javac", default="javac", help="javac executable")
    parser.add_argument(
        "--skip-build",
        action="store_true",
        help="use an existing classes directory instead of compiling first",
    )


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)

    build = commands.add_parser("build", help="compile the portable Java TCK")
    build.add_argument("--api-jar", required=True, help="official IEEE Java API JAR")
    build.add_argument(
        "--output-directory",
        default=str(DEFAULT_CLASSES),
        help=f"compiled classes directory (default: {DEFAULT_CLASSES})",
    )
    build.add_argument("--javac", default="javac", help="javac executable")
    build.add_argument("--api-package-prefix", default=DEFAULT_API_PACKAGE_PREFIX)

    run = commands.add_parser("run", help="compile and run one provider configuration")
    add_common_run_arguments(run)

    matrix = commands.add_parser(
        "matrix", help="run two or more provider configurations"
    )
    matrix.add_argument("--configuration", required=True, help="JSON provider matrix")
    matrix.add_argument(
        "--classes-directory",
        default=str(DEFAULT_CLASSES),
        help=f"compiled classes directory (default: {DEFAULT_CLASSES})",
    )
    matrix.add_argument(
        "--output-directory",
        default=str(ROOT / "out" / "java-tck" / "matrix"),
        help="result and JUnit output directory",
    )
    matrix.add_argument("--java", default="java", help="java executable")
    matrix.add_argument("--javac", default="javac", help="javac executable")
    matrix.add_argument(
        "--skip-build",
        action="store_true",
        help="use an existing classes directory instead of compiling first",
    )
    return parser


def required_matrix_string(configuration: JsonObject, name: str, index: int) -> str:
    value = configuration.get(name)
    if not isinstance(value, str) or not value:
        raise RunnerError(f"matrix entry {index} {name!r} must be a non-empty string")
    return value


def string_list(
    value: object, *, label: str, allow_empty: bool = False
) -> tuple[str, ...]:
    if isinstance(value, str):
        if value:
            return (value,)
        if allow_empty:
            return ()
    if (
        isinstance(value, list)
        and all(isinstance(item, str) and item for item in value)
        and (value or allow_empty)
    ):
        return tuple(value)
    expectation = (
        "a string or string array"
        if allow_empty
        else "a non-empty string or string array"
    )
    raise RunnerError(f"{label} must be {expectation}")


def string_array(value: object, *, label: str) -> tuple[str, ...]:
    if isinstance(value, list) and all(
        isinstance(item, str) and item for item in value
    ):
        return tuple(value)
    raise RunnerError(f"{label} must be an array of non-empty strings")


def optional_string(value: object, *, label: str) -> str:
    if isinstance(value, str) and value:
        return value
    raise RunnerError(f"{label} must be a non-empty string")


def optional_path(value: object, *, label: str) -> str | None:
    if value is None or value == "":
        return None
    if isinstance(value, str):
        return value
    raise RunnerError(f"{label} must be a string path or empty")


def provider_configuration(value: object, index: int) -> _ProviderConfiguration:
    if not isinstance(value, dict) or not all(isinstance(key, str) for key in value):
        raise RunnerError(f"matrix entry {index} must be a JSON object")
    configuration = cast(JsonObject, value)
    provider_jars = string_list(
        configuration.get("providerJar"),
        label=f"matrix entry {index} providerJar",
    )
    dependency_jars = string_list(
        configuration.get("dependencyJars", []),
        label=f"matrix entry {index} dependencyJars",
        allow_empty=True,
    )
    run_unverified = configuration.get("runUnverified", False)
    if not isinstance(run_unverified, bool):
        raise RunnerError(f"matrix entry {index} runUnverified must be a boolean")
    jvm_arguments = string_array(
        configuration.get("jvmArguments", []),
        label=f"matrix entry {index} jvmArguments",
    )
    api_package_prefix = validate_api_package_prefix(
        optional_string(
            configuration.get("apiPackagePrefix", DEFAULT_API_PACKAGE_PREFIX),
            label=f"matrix entry {index} apiPackagePrefix",
        )
    )
    time_implementation = optional_string(
        configuration.get("timeImplementation", "HLAinteger64Time"),
        label=f"matrix entry {index} timeImplementation",
    )
    return _ProviderConfiguration(
        identifier=required_matrix_string(configuration, "id", index),
        api_jar=required_matrix_string(configuration, "apiJar", index),
        provider_jars=provider_jars,
        factory_name=required_matrix_string(configuration, "factoryName", index),
        fom_path=required_matrix_string(configuration, "fomPath", index),
        capability_profile=optional_path(
            configuration.get("capabilityProfile", ""),
            label=f"matrix entry {index} capabilityProfile",
        ),
        dependency_jars=dependency_jars,
        api_package_prefix=api_package_prefix,
        run_unverified=run_unverified,
        mim_path=optional_path(
            configuration.get("mimPath", ""),
            label=f"matrix entry {index} mimPath",
        ),
        jvm_arguments=jvm_arguments,
        time_implementation=time_implementation,
    )


def matrix_configurations(path: Path) -> list[tuple[_ProviderConfiguration, Path]]:

    try:
        raw = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise RunnerError(f"could not read Java TCK matrix {path}: {error}") from error
    if not isinstance(raw, list) or len(raw) < 2:
        raise RunnerError(
            "the Java TCK matrix requires at least two provider configurations"
        )
    configurations: list[tuple[_ProviderConfiguration, Path]] = []
    seen: set[str] = set()
    for index, item in enumerate(raw, start=1):
        configuration = provider_configuration(item, index)
        if configuration.identifier in seen:
            raise RunnerError(
                f"matrix contains duplicate provider id {configuration.identifier!r}"
            )
        seen.add(configuration.identifier)
        configurations.append((configuration, path.parent))
    return configurations


def run_matrix(arguments: argparse.Namespace) -> int:
    configuration_path = resolve_path(
        arguments.configuration, label="matrix configuration"
    )
    configurations = matrix_configurations(configuration_path)
    classes = resolve_directory(
        arguments.classes_directory,
        must_exist=arguments.skip_build,
        label="classes directory",
    )
    output = resolve_directory(
        arguments.output_directory,
        must_exist=False,
        label="matrix output directory",
    )
    output.mkdir(parents=True, exist_ok=True)

    first_configuration, first_base = configurations[0]
    first_api = resolve_path(
        first_configuration.api_jar,
        base=first_base,
        label="matrix API JAR",
    )
    first_api_package_prefix = first_configuration.api_package_prefix
    if not arguments.skip_build:
        result = build_tck(
            api_jar=first_api,
            classes_directory=classes,
            javac=arguments.javac,
            api_package_prefix=first_api_package_prefix,
        )
        if result != 0:
            return result

    for index, (configuration, base) in enumerate(configurations, start=1):
        identifier = configuration.identifier
        api_package_prefix = configuration.api_package_prefix
        if api_package_prefix != first_api_package_prefix:
            raise RunnerError(
                "all matrix entries must use the same apiPackagePrefix because they share compiled classes"
            )
        api = resolve_path(
            configuration.api_jar,
            base=base,
            label=f"matrix entry {identifier} API JAR",
        )
        providers = [
            resolve_path(
                value, base=base, label=f"matrix entry {identifier} provider JAR"
            )
            for value in configuration.provider_jars
        ]
        dependencies = [
            resolve_path(
                value, base=base, label=f"matrix entry {identifier} dependency JAR"
            )
            for value in configuration.dependency_jars
        ]
        fom = resolve_path(
            configuration.fom_path,
            base=base,
            label=f"matrix entry {identifier} FOM module",
        )
        profile = (
            resolve_path(
                configuration.capability_profile,
                base=base,
                label=f"matrix entry {identifier} capability profile",
            )
            if configuration.capability_profile is not None
            else None
        )
        mim = (
            resolve_path(
                configuration.mim_path,
                base=base,
                label=f"matrix entry {identifier} MIM module",
            )
            if configuration.mim_path is not None
            else None
        )
        result_path = (output / f"{identifier}.results.json").resolve()
        junit_path = (output / f"{identifier}.junit.xml").resolve()
        result = run_tck(
            api_jar=api,
            provider_jars=providers,
            dependency_jars=dependencies,
            fom_path=fom,
            classes_directory=classes,
            java=arguments.java,
            javac=arguments.javac,
            mim_path=mim,
            factory_name=configuration.factory_name,
            time_implementation=configuration.time_implementation,
            capability_profile=profile,
            run_unverified=configuration.run_unverified,
            results_path=result_path,
            junit_path=junit_path,
            provider_id=identifier,
            jvm_arguments=list(configuration.jvm_arguments),
            skip_build=True,
            api_package_prefix=api_package_prefix,
        )
        if result != 0:
            raise RunnerError(
                f"Java TCK failed for provider {identifier!r} with exit code {result}"
            )
    print(output)
    return 0


def run_single(arguments: argparse.Namespace) -> int:
    api = resolve_path(arguments.api_jar, label="API JAR")
    providers = [
        resolve_path(value, label="provider JAR") for value in arguments.provider_jar
    ]
    dependencies = [
        resolve_path(value, label="dependency JAR")
        for value in arguments.dependency_jar
    ]
    fom = resolve_path(arguments.fom_path, label="FOM module")
    classes = resolve_directory(
        arguments.classes_directory,
        must_exist=arguments.skip_build,
        label="classes directory",
    )
    mim = (
        resolve_path(arguments.mim_path, label="MIM module")
        if arguments.mim_path
        else None
    )
    profile = (
        resolve_path(arguments.capability_profile, label="capability profile")
        if arguments.capability_profile
        else None
    )
    results = (
        resolve_path(arguments.results_path, must_exist=False, label="results path")
        if arguments.results_path
        else None
    )
    junit = (
        resolve_path(arguments.junit_path, must_exist=False, label="JUnit path")
        if arguments.junit_path
        else None
    )
    for path in (results, junit):
        if path is not None:
            path.parent.mkdir(parents=True, exist_ok=True)
    return run_tck(
        api_jar=api,
        provider_jars=providers,
        dependency_jars=dependencies,
        fom_path=fom,
        classes_directory=classes,
        java=arguments.java,
        javac=arguments.javac,
        mim_path=mim,
        factory_name=arguments.factory_name,
        time_implementation=arguments.time_implementation,
        capability_profile=profile,
        scenario=arguments.scenario,
        run_unverified=arguments.run_unverified,
        results_path=results,
        junit_path=junit,
        provider_id=arguments.provider_id,
        jvm_arguments=arguments.jvm_argument,
        skip_build=arguments.skip_build,
        api_package_prefix=arguments.api_package_prefix,
    )


def main(argv: list[str] | None = None) -> int:
    arguments = build_parser().parse_args(argv)
    try:
        if arguments.command == "build":
            api = resolve_path(arguments.api_jar, label="API JAR")
            output = resolve_directory(
                arguments.output_directory,
                must_exist=False,
                label="output directory",
            )
            return build_tck(
                api_jar=api,
                classes_directory=output,
                javac=arguments.javac,
                api_package_prefix=arguments.api_package_prefix,
            )
        if arguments.command == "run":
            return run_single(arguments)
        if arguments.command == "matrix":
            return run_matrix(arguments)
        raise RunnerError(f"unknown command: {arguments.command}")
    except RunnerError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
