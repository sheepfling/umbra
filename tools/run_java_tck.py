"""Build and run the portable IEEE 1516.1-2025 Java TCK.

The runner uses only Python's standard library and starts ``javac``/``java``
without a shell.  Provider JARs, FOM/MIM modules, capability profiles, and
JVM arguments stay outside the portable Java sources.
"""

from __future__ import annotations

import argparse
import os
import shlex
import subprocess
import sys
from pathlib import Path
from typing import Any, Iterable


ROOT = Path(__file__).resolve().parents[1]
TCK_ROOT = ROOT / "packages" / "hla-rti-java-tck"
SOURCE_ROOT = TCK_ROOT / "src" / "main" / "java"
DEFAULT_CLASSES = ROOT / "out" / "java-tck" / "classes"


class RunnerError(ValueError):
    """A user-correctable runner configuration or tool error."""


def resolve_path(
    value: str | Path,
    *,
    base: Path = Path.cwd(),
    must_exist: bool = True,
    label: str,
) -> Path:
    path = Path(value)
    if not path.is_absolute():
        path = base / path
    path = path.resolve()
    if must_exist and not path.is_file():
        raise RunnerError(f"{label} does not exist: {path}")
    return path


def resolve_directory(
    value: str | Path,
    *,
    base: Path = Path.cwd(),
    must_exist: bool = True,
    label: str,
) -> Path:
    path = Path(value)
    if not path.is_absolute():
        path = base / path
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
        raise RunnerError(f"could not start {command[0]!r}; install it or pass its path") from error
    return completed.returncode


def java_sources() -> list[Path]:
    sources = sorted(SOURCE_ROOT.rglob("*.java"))
    if not sources:
        raise RunnerError(f"no Java TCK sources found below {SOURCE_ROOT}")
    return sources


def build_tck(
    *,
    api_jar: Path,
    classes_directory: Path,
    javac: str,
) -> int:
    classes_directory.mkdir(parents=True, exist_ok=True)
    command = [
        javac,
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
        *(str(path) for path in java_sources()),
    ]
    return run_command(command, cwd=ROOT)


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
    results_path: Path | None = None,
    junit_path: Path | None = None,
    provider_id: str = "",
    jvm_arguments: Iterable[str] = (),
    skip_build: bool = False,
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
    add_property(command, "hla.rti.tck.results", str(results_path) if results_path else None)
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
    parser.add_argument("--mim-path", default="", help="optional adapter-supplied MIM module")
    parser.add_argument("--factory-name", default="", help="standard factory name")
    parser.add_argument("--time-implementation", default="HLAinteger64Time")
    parser.add_argument("--capability-profile", default="")
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
        help="provider-owned JVM argument; repeat as needed",
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

    run = commands.add_parser("run", help="compile and run one provider configuration")
    add_common_run_arguments(run)

    matrix = commands.add_parser("matrix", help="run two or more provider configurations")
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


def required_matrix_value(configuration: dict[str, Any], name: str, index: int) -> Any:
    value = configuration.get(name)
    if value is None or value == "" or value == []:
        raise RunnerError(f"matrix entry {index} is missing {name!r}")
    return value


def string_list(value: Any, *, label: str) -> list[str]:
    if isinstance(value, str):
        return [value]
    if isinstance(value, list) and all(isinstance(item, str) and item for item in value):
        return value
    raise RunnerError(f"{label} must be a string or a non-empty string array")


def matrix_configurations(path: Path) -> list[tuple[dict[str, Any], Path]]:
    import json

    try:
        raw = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise RunnerError(f"could not read Java TCK matrix {path}: {error}") from error
    if not isinstance(raw, list) or len(raw) < 2:
        raise RunnerError("the Java TCK matrix requires at least two provider configurations")
    configurations: list[tuple[dict[str, Any], Path]] = []
    seen: set[str] = set()
    for index, item in enumerate(raw, start=1):
        if not isinstance(item, dict):
            raise RunnerError(f"matrix entry {index} must be a JSON object")
        for required in (
            "id",
            "apiJar",
            "providerJar",
            "factoryName",
            "fomPath",
            "capabilityProfile",
        ):
            required_matrix_value(item, required, index)
        identifier = required_matrix_value(item, "id", index)
        if not isinstance(identifier, str) or not identifier:
            raise RunnerError(f"matrix entry {index} has an invalid id")
        if identifier in seen:
            raise RunnerError(f"matrix contains duplicate provider id {identifier!r}")
        seen.add(identifier)
        configurations.append((item, path.parent))
    return configurations


def run_matrix(arguments: argparse.Namespace) -> int:
    configuration_path = resolve_path(arguments.configuration, label="matrix configuration")
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

    first_api = resolve_path(
        required_matrix_value(configurations[0][0], "apiJar", 1),
        base=configurations[0][1],
        label="matrix API JAR",
    )
    if not arguments.skip_build:
        result = build_tck(api_jar=first_api, classes_directory=classes, javac=arguments.javac)
        if result != 0:
            return result

    for index, (configuration, base) in enumerate(configurations, start=1):
        identifier = str(required_matrix_value(configuration, "id", index))
        api = resolve_path(
            required_matrix_value(configuration, "apiJar", index),
            base=base,
            label=f"matrix entry {identifier} API JAR",
        )
        providers = [
            resolve_path(value, base=base, label=f"matrix entry {identifier} provider JAR")
            for value in string_list(
                required_matrix_value(configuration, "providerJar", index),
                label=f"matrix entry {identifier} providerJar",
            )
        ]
        dependencies = [
            resolve_path(value, base=base, label=f"matrix entry {identifier} dependency JAR")
            for value in string_list(configuration.get("dependencyJars", []), label="dependencyJars")
        ] if configuration.get("dependencyJars") else []
        fom = resolve_path(
            required_matrix_value(configuration, "fomPath", index),
            base=base,
            label=f"matrix entry {identifier} FOM module",
        )
        profile = resolve_path(
            required_matrix_value(configuration, "capabilityProfile", index),
            base=base,
            label=f"matrix entry {identifier} capability profile",
        )
        mim_value = configuration.get("mimPath", "")
        mim = (
            resolve_path(mim_value, base=base, label=f"matrix entry {identifier} MIM module")
            if mim_value
            else None
        )
        jvm_arguments = configuration.get("jvmArguments", [])
        if not isinstance(jvm_arguments, list) or not all(
            isinstance(value, str) and value for value in jvm_arguments
        ):
            raise RunnerError(f"matrix entry {identifier} jvmArguments must be a string array")
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
            factory_name=str(configuration.get("factoryName", "")),
            time_implementation=str(
                configuration.get("timeImplementation", "HLAinteger64Time")
            ),
            capability_profile=profile,
            results_path=result_path,
            junit_path=junit_path,
            provider_id=identifier,
            jvm_arguments=jvm_arguments,
            skip_build=True,
        )
        if result != 0:
            raise RunnerError(f"Java TCK failed for provider {identifier!r} with exit code {result}")
    print(output)
    return 0


def run_single(arguments: argparse.Namespace) -> int:
    api = resolve_path(arguments.api_jar, label="API JAR")
    providers = [resolve_path(value, label="provider JAR") for value in arguments.provider_jar]
    dependencies = [
        resolve_path(value, label="dependency JAR") for value in arguments.dependency_jar
    ]
    fom = resolve_path(arguments.fom_path, label="FOM module")
    classes = resolve_directory(
        arguments.classes_directory,
        must_exist=arguments.skip_build,
        label="classes directory",
    )
    mim = resolve_path(arguments.mim_path, label="MIM module") if arguments.mim_path else None
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
        results_path=results,
        junit_path=junit,
        provider_id=arguments.provider_id,
        jvm_arguments=arguments.jvm_argument,
        skip_build=arguments.skip_build,
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
            return build_tck(api_jar=api, classes_directory=output, javac=arguments.javac)
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
