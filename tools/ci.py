"""Run Umbra's local and pipeline CI routes.

The command is intentionally pipeline-agnostic. A CI service only needs to
prepare a supported Windows/Python/Java environment and invoke the
``python -m tools.ci`` entry point. The same route commands are available to a
developer checkout, which keeps local and hosted failure behavior close.

The historical ``native`` profiles remain supported for compatibility. New
route-oriented commands use the following shape::

    python -m tools.ci test --standard 2010 --route cpp
    python -m tools.ci test --standard 2025 --route jni
    python -m tools.ci test --standard all --route all
    python -m tools.ci lint
    python -m tools.ci fix
    python -m tools.ci clean

The Java API archives are intentionally not vendored. 2010 Java/JNI routes
therefore require ``--api-jar`` (or ``UMBRA_2010_API_JAR``); the 2025 Java/JNI
routes can use the checked-in mock fixture when no provider archive is passed.
"""

from __future__ import annotations

import argparse
import os
import shlex
import shutil
import subprocess
import sys
import zipfile
from collections.abc import Iterable, Sequence
from dataclasses import dataclass
from pathlib import Path

REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
STAGES = ("configure", "build", "test")
STANDARDS = ("2010", "2025")
ROUTES = ("cpp", "java", "jni", "python")


@dataclass(frozen=True)
class Profile:
    """The matching configure, build, and test presets for one native lane."""

    description: str
    configure_preset: str
    build_preset: str
    test_preset: str


PROFILES: dict[str, Profile] = {
    "native": Profile(
        "Core native build, smoke tests, contracts, and package smoke.",
        "windows-default",
        "windows-default-debug",
        "windows-default-debug",
    ),
    "native-catch2": Profile(
        "Focused native Catch2 behavior tests; may fetch Catch2.",
        "windows-catch2",
        "windows-catch2-debug",
        "windows-catch2-debug",
    ),
    "native-fom": Profile(
        "FOM validation and XML/XSD tests; may fetch Catch2 and libxml2.",
        "windows-fom",
        "windows-fom-debug",
        "windows-fom-debug",
    ),
    "native-fom-services": Profile(
        "Installable embedded federation-management profile with process endpoint and package smoke.",
        "windows-fom-services",
        "windows-fom-services-debug",
        "windows-fom-services-debug",
    ),
}


class CiError(RuntimeError):
    """A user-actionable CI configuration or tool failure."""


def command_display(command: Sequence[str]) -> str:
    return subprocess.list2cmdline(command) if os.name == "nt" else shlex.join(command)


def run_command(
    command: Sequence[str],
    *,
    dry_run: bool = False,
    env: dict[str, str] | None = None,
) -> None:
    print(f"+ {command_display(command)}", flush=True)
    if not dry_run:
        subprocess.run(command, cwd=REPOSITORY_ROOT, check=True, env=env)


def require_python_version() -> None:
    if sys.version_info < (3, 11):
        raise CiError("Umbra CI routes require Python 3.11 or newer.")


def resolve_path(
    value: str | None, *, label: str, required: bool = True
) -> Path | None:
    if value is None:
        if required:
            raise CiError(f"Missing {label}.")
        return None
    path = Path(value).expanduser()
    if not path.is_absolute():
        path = REPOSITORY_ROOT / path
    path = path.resolve()
    if required and not path.is_file():
        raise CiError(f"{label} does not exist: {path}")
    return path


def first_existing(candidates: Iterable[Path]) -> Path | None:
    for candidate in candidates:
        if candidate.is_file():
            return candidate.resolve()
    return None


def requested_api_jar(arguments: argparse.Namespace, standard: str) -> str | None:
    specific = getattr(arguments, f"api_jar_{standard}", None)
    if specific:
        return specific
    # With --standard all, keep the convenient generic --api-jar focused on
    # the external-only 2010 lane. A 2010 API archive must not accidentally be
    # passed to the independent 2025 fixture lane.
    if standard == "2025" and arguments.standard == "all" and arguments.api_jar:
        return None
    return arguments.api_jar


def api_jar(arguments: argparse.Namespace, standard: str) -> Path:
    explicit = requested_api_jar(arguments, standard)
    environment = os.environ.get(f"UMBRA_{standard}_API_JAR")
    candidates = [
        (
            REPOSITORY_ROOT / "out" / "java-tck-2010" / "ieee1516e-api-staged.jar"
            if standard == "2010"
            else REPOSITORY_ROOT / "out" / "java-tck" / "ieee1516e-api-staged.jar"
        )
    ]
    path = resolve_path(explicit, label=f"{standard} Java API JAR", required=False)
    if path is None and environment:
        path = resolve_path(
            environment, label=f"{standard} Java API JAR", required=False
        )
    if path is None:
        path = first_existing(candidates)
    if path is None:
        raise CiError(
            f"The {standard} Java API JAR is required for this route. "
            f"Pass --api-jar PATH or set UMBRA_{standard}_API_JAR."
        )
    return path


def provider_jars(arguments: argparse.Namespace, standard: str) -> list[Path]:
    values = list(arguments.provider_jar)
    if not values:
        environment = os.environ.get(f"UMBRA_{standard}_PROVIDER_JAR")
        if environment:
            values = [item for item in environment.split(os.pathsep) if item]
    paths: list[Path] = []
    for value in values:
        path = resolve_path(value, label=f"{standard} provider JAR")
        assert path is not None
        paths.append(path)
    return paths


def dependency_jars(arguments: argparse.Namespace, standard: str) -> list[Path]:
    values = list(arguments.dependency_jar)
    if not values:
        environment = os.environ.get(f"UMBRA_{standard}_DEPENDENCY_JAR")
        if environment:
            values = [item for item in environment.split(os.pathsep) if item]
    paths: list[Path] = []
    for value in values:
        path = resolve_path(value, label=f"{standard} provider dependency JAR")
        assert path is not None
        paths.append(path)
    return paths


def find_executable(value: str, *, label: str) -> str:
    executable = shutil.which(value)
    if executable is None:
        raise CiError(f"Cannot find {label} executable {value!r} on PATH.")
    return executable


def add_test_arguments(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--standard", choices=("2010", "2025", "all"), default="all")
    parser.add_argument(
        "--route", choices=("cpp", "java", "jni", "python", "all"), default="all"
    )
    parser.add_argument(
        "--api-jar",
        help="Java API JAR for the selected standard; 2010 archives are never vendored.",
    )
    parser.add_argument(
        "--api-jar-2010", help="2010 API JAR when running both standards."
    )
    parser.add_argument(
        "--api-jar-2025", help="2025 API JAR when running both standards."
    )
    parser.add_argument(
        "--provider-jar",
        action="append",
        default=[],
        help="Provider JAR; repeat when multiple provider implementations are intentional.",
    )
    parser.add_argument(
        "--dependency-jar",
        action="append",
        default=[],
        help="Provider dependency JAR; repeat as needed.",
    )
    parser.add_argument("--factory-name", help="Named Java RtiFactory to select.")
    parser.add_argument("--fom-path", help="Optional FOM module for Java TCK routes.")
    parser.add_argument("--mim-path", help="Optional MIM module for Java TCK routes.")
    parser.add_argument(
        "--capability-profile", help="Optional 2010 Java/Python capability profile."
    )
    parser.add_argument("--native-library", help="Optional JNI native library path.")
    parser.add_argument(
        "--build-root", default="out/ci", help="Disposable route output root."
    )
    parser.add_argument(
        "--configuration", default="Debug", help="Native build configuration."
    )
    parser.add_argument("--cmake", default="cmake", help="CMake executable.")
    parser.add_argument("--ctest", default="ctest", help="CTest executable.")
    parser.add_argument(
        "--dry-run", action="store_true", help="Print commands without running them."
    )


def parse_legacy_arguments(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "profile",
        nargs="?",
        choices=sorted(PROFILES),
        help="Legacy native CI profile; use --list-profiles to inspect choices.",
    )
    parser.add_argument("--stage", choices=("all", *STAGES), default="all")
    parser.add_argument("--cmake", default="cmake", help="CMake executable.")
    parser.add_argument("--ctest", default="ctest", help="CTest executable.")
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--list-profiles", action="store_true")
    arguments = parser.parse_args(argv)
    if not arguments.list_profiles and arguments.profile is None:
        parser.error("profile is required unless --list-profiles is used")
    return arguments


def list_profiles() -> None:
    print("Legacy native profiles:")
    for name, profile in PROFILES.items():
        print(f"  {name}: {profile.description}")
        print(
            f"    configure={profile.configure_preset} "
            f"build={profile.build_preset} test={profile.test_preset}"
        )


def stage_commands(
    arguments: argparse.Namespace, profile: Profile
) -> dict[str, list[str]]:
    return {
        "configure": [arguments.cmake, "--preset", profile.configure_preset],
        "build": [arguments.cmake, "--build", "--preset", profile.build_preset],
        "test": [arguments.ctest, "--preset", profile.test_preset],
    }


def run_legacy(argv: Sequence[str]) -> int:
    arguments = parse_legacy_arguments(argv)
    require_python_version()
    if arguments.list_profiles:
        list_profiles()
        return 0

    profile = PROFILES[arguments.profile]
    commands = stage_commands(arguments, profile)
    stages = STAGES if arguments.stage == "all" else (arguments.stage,)
    print(
        f"Running legacy CI profile '{arguments.profile}' from {REPOSITORY_ROOT}",
        flush=True,
    )
    try:
        for stage in stages:
            run_command(commands[stage], dry_run=arguments.dry_run)
    except FileNotFoundError as error:
        print(
            f"Cannot run {error.filename!r}; install CMake and ensure it is on PATH.",
            file=sys.stderr,
        )
        return 2
    except subprocess.CalledProcessError as error:
        print(
            f"CI profile '{arguments.profile}' failed during the {stage} stage "
            f"with exit code {error.returncode}.",
            file=sys.stderr,
        )
        return error.returncode or 1
    return 0


def selected_lanes(arguments: argparse.Namespace) -> list[tuple[str, str]]:
    standards = STANDARDS if arguments.standard == "all" else (arguments.standard,)
    routes = ROUTES if arguments.route == "all" else (arguments.route,)
    return [(standard, route) for standard in standards for route in routes]


def native_route(arguments: argparse.Namespace, standard: str) -> None:
    if os.name != "nt":
        raise CiError("The current CMake presets target the Windows MSVC toolchain.")
    if standard == "2025":
        run_command(
            [sys.executable, "tools/cpp_tck.py"],
            dry_run=arguments.dry_run,
            env=python_environment(),
        )
    cmake = find_executable(arguments.cmake, label="CMake")
    ctest = find_executable(arguments.ctest, label="CTest")
    targets = {
        "2010": (
            "umbra_ieee1516_2010_headers_smoke",
            "umbra_ieee1516_2010_binding_shell_smoke",
            "umbra_ieee1516_2010_encoding_smoke",
            "umbra_ieee1516_2010_composite_encoding_smoke",
            "umbra_ieee1516_2010_integer_time_smoke",
            "umbra_ieee1516_2010_float_time_smoke",
            "umbra_ieee1516_2010_time_marshal_smoke",
        ),
        "2025": (
            "umbra_ieee1516_2025_headers_smoke",
            "umbra_ieee1516_2025_binding_shell_smoke",
            "umbra_federate_lifecycle_smoke",
        ),
    }[standard]
    test_regex = {
        "2010": r"umbra\.ieee1516e_2010\.(headers|binding_shell|binding_shell_generation|provider_boundaries|encoding|composite_encoding|integer_time|float_time|time_marshal)$",
        "2025": r"umbra\.ieee1516_2025\.(headers|binding_shell|integrity|exception_binding|binding_inventory|binding_shell_generation)$|umbra\.kernel\.federate_lifecycle$",
    }[standard]
    run_command([cmake, "--preset", "windows-default"], dry_run=arguments.dry_run)
    run_command(
        [
            cmake,
            "--build",
            "--preset",
            "windows-default-debug",
            "--target",
            *targets,
        ],
        dry_run=arguments.dry_run,
    )
    run_command(
        [
            ctest,
            "--test-dir",
            str(REPOSITORY_ROOT / "out" / "cmake" / "default"),
            "-C",
            arguments.configuration,
            "-R",
            test_regex,
            "--output-on-failure",
        ],
        dry_run=arguments.dry_run,
    )


def python_environment() -> dict[str, str]:
    environment = os.environ.copy()
    source_roots = [
        REPOSITORY_ROOT / "packages" / "umbra-rti-api" / "src",
        REPOSITORY_ROOT / "packages" / "umbra-rti-test-support" / "src",
        REPOSITORY_ROOT / "packages" / "umbra-rti-jpype" / "src",
        REPOSITORY_ROOT / "packages" / "umbra-rti-native" / "src",
    ]
    existing = environment.get("PYTHONPATH")
    paths = [str(path) for path in source_roots]
    if existing:
        paths.append(existing)
    environment["PYTHONPATH"] = os.pathsep.join(paths)
    return environment


def python_2010_surface_checks(arguments: argparse.Namespace) -> None:
    """Run the source-checkout-safe 2010 contract and transplant gates.

    These checks deliberately do not require a vendor JAR, a JVM, or the
    optional native extension.  Provider-specific execution belongs to the
    Java/JNI/native routes; the Python route must still prove that the
    provider-neutral surface and its catalog/profile boundaries are intact.
    """

    profile_root = REPOSITORY_ROOT / "packages" / "umbra-rti-java-tck-2010" / "profiles"
    profiles = sorted(profile_root.glob("*.properties"))
    if not profiles:
        raise CiError(f"No 2010 capability profiles found below {profile_root}")
    checks = [
        [
            sys.executable,
            "tools/verify_1516e_python_surface.py",
            "--check-jpype",
            "--check-native-source",
        ],
        [sys.executable, "tools/verify_1516e_tck_catalog.py"],
        [sys.executable, "tools/verify_1516e_tck_parity.py"],
        [sys.executable, "tools/verify_1516e_type_roundtrip.py"],
    ]
    checks.extend(
        [
            sys.executable,
            "tools/verify_1516e_capability_profile.py",
            "--profile",
            str(profile),
        ]
        for profile in profiles
    )
    for check in checks:
        run_command(
            check,
            dry_run=arguments.dry_run,
            env=python_environment(),
        )


def python_route(arguments: argparse.Namespace, standard: str) -> None:
    if standard == "2010":
        python_2010_surface_checks(arguments)
    pytest = [
        REPOSITORY_ROOT
        / "packages"
        / "umbra-rti-api"
        / "tests"
        / ("test_2010_contracts.py" if standard == "2010" else "test_contracts.py"),
        REPOSITORY_ROOT
        / "packages"
        / "umbra-rti-api"
        / "tests"
        / "test_python_surface_report.py",
    ]
    if standard == "2010":
        pytest.extend(
            [
                REPOSITORY_ROOT
                / "packages"
                / "umbra-rti-jpype"
                / "tests"
                / "test_java_2010_provider.py",
                REPOSITORY_ROOT
                / "packages"
                / "umbra-rti-jpype"
                / "tests"
                / "test_2010_provider_jar_preflight.py",
                REPOSITORY_ROOT
                / "packages"
                / "umbra-rti-jpype"
                / "tests"
                / "test_2010_capability_profile.py",
                REPOSITORY_ROOT
                / "packages"
                / "umbra-rti-native"
                / "tests"
                / "test_native_2010.py",
            ]
        )
    else:
        pytest.extend(
            [
                REPOSITORY_ROOT
                / "packages"
                / "umbra-rti-jpype"
                / "tests"
                / "test_java_provider.py",
                REPOSITORY_ROOT
                / "packages"
                / "umbra-rti-jpype"
                / "tests"
                / "test_umbra_jni_factory.py",
                REPOSITORY_ROOT
                / "packages"
                / "umbra-rti-jpype"
                / "tests"
                / "test_mock_java_fixture.py",
            ]
        )
    # Opt-in bridge/JVM suites are included when explicitly enabled, while
    # keeping the source-checkout Python route independent of a JDK, CMake,
    # vendor JAR, or an already-built native artifact.  The tests themselves
    # retain their skip guards so a missing optional toolchain is explicit.
    optional_tests: list[Path] = []
    if standard == "2010":
        if os.environ.get("UMBRA_ENABLE_JNI_2010_TYPE_ROUNDTRIP_TESTS") == "1":
            optional_tests.append(
                REPOSITORY_ROOT
                / "packages"
                / "umbra-rti-jpype"
                / "tests"
                / "test_jpype_2010_jni_type_roundtrip.py"
            )
        if os.environ.get("UMBRA_ENABLE_JPYPE_2010_MOCK_INTEGRATION") == "1":
            optional_tests.append(
                REPOSITORY_ROOT
                / "packages"
                / "umbra-rti-jpype"
                / "tests"
                / "test_jpype_2010_mock_integration.py"
            )
    else:
        if os.environ.get("UMBRA_ENABLE_JNI_INTEGRATION_TESTS") == "1":
            optional_tests.append(
                REPOSITORY_ROOT
                / "packages"
                / "umbra-rti-jpype"
                / "tests"
                / "test_jpype_jni_integration.py"
            )
        if os.environ.get("UMBRA_ENABLE_JPYPE_STATE_SPACE_MATRIX") == "1":
            optional_tests.append(
                REPOSITORY_ROOT
                / "packages"
                / "umbra-rti-jpype"
                / "tests"
                / "test_jpype_mock_integration.py"
            )
    pytest.extend(optional_tests)
    run_command(
        [
            sys.executable,
            "-m",
            "pytest",
            "-q",
            "-p",
            "no:cacheprovider",
            *(str(path) for path in pytest),
        ],
        dry_run=arguments.dry_run,
        env=python_environment(),
    )


def java_tool(name: str) -> str:
    return find_executable(name, label=name)


def java_sources(source_root: Path, *, label: str) -> list[Path]:
    sources = sorted(source_root.rglob("*.java"))
    if not sources:
        raise CiError(f"No {label} Java sources found below {source_root}")
    return sources


def copy_resource_tree(source: Path, destination: Path, *, dry_run: bool) -> None:
    if not source.is_dir():
        return
    if not dry_run:
        shutil.copytree(source, destination, dirs_exist_ok=True)


def build_java_fixture(
    arguments: argparse.Namespace,
    standard: str,
    output: Path,
    *,
    run_smoke: bool = True,
) -> Path:
    javac = java_tool("javac")
    jar_tool = java_tool("jar")
    output = output.resolve()
    classes = output / "classes"
    if not arguments.dry_run:
        classes.mkdir(parents=True, exist_ok=True)

    if standard == "2010":
        api = api_jar(arguments, standard)
        fixture_root = (
            REPOSITORY_ROOT
            / "packages"
            / "umbra-rti-java-tck-2010"
            / "test-fixtures"
            / "mock-java-rti-2010"
        )
        sources = java_sources(
            fixture_root / "src" / "main" / "java", label="2010 mock fixture"
        )
        compile_command = [
            javac,
            "-encoding",
            "UTF-8",
            "-source",
            "11",
            "-target",
            "11",
            "-cp",
            str(api),
            "-d",
            str(classes),
            *(str(path) for path in sources),
        ]
        jar_name = "umbra-mock-java-rti-1516e.jar"
        resource_root = fixture_root / "src" / "main" / "resources"
        factory = "Umbra mock Java 1516e"
    else:
        fixture_root = (
            REPOSITORY_ROOT
            / "packages"
            / "umbra-rti-jpype"
            / "test-fixtures"
            / "mock-java-rti"
        )
        sources = java_sources(
            fixture_root / "src" / "main" / "java", label="2025 mock fixture"
        )
        compile_command = [
            javac,
            "-d",
            str(classes),
            *(str(path) for path in sources),
        ]
        jar_name = "umbra-mock-java-rti.jar"
        resource_root = fixture_root / "src" / "main" / "resources"
        factory = "Umbra Mock Java RTI"

    run_command(compile_command, dry_run=arguments.dry_run)
    copy_resource_tree(resource_root, classes, dry_run=arguments.dry_run)
    jar = output / jar_name
    run_command(
        [jar_tool, "--create", "--file", str(jar), "-C", str(classes), "."],
        dry_run=arguments.dry_run,
    )
    if run_smoke:
        smoke_class = (
            "org.umbra.testfixture.rti1516_2025.MockSmokeTest"
            if standard == "2025"
            else None
        )
        if smoke_class:
            run_command(
                [java_tool("java"), "-cp", str(jar), smoke_class],
                dry_run=arguments.dry_run,
            )
    if not arguments.dry_run and not jar.is_file():
        raise CiError(f"Java fixture build did not produce {jar}")
    arguments._ci_fixture_factory = factory
    return jar


def compile_java_tck(
    arguments: argparse.Namespace, standard: str, api: Path, output: Path
) -> None:
    javac = java_tool("javac")
    tck_root = (
        REPOSITORY_ROOT
        / "packages"
        / ("umbra-rti-java-tck-2010" if standard == "2010" else "hla-rti-java-tck")
    )
    sources = java_sources(tck_root / "src" / "main" / "java", label=f"{standard} TCK")
    if not arguments.dry_run:
        output.mkdir(parents=True, exist_ok=True)
    run_command(
        [
            javac,
            "-encoding",
            "UTF-8",
            "-source",
            "11",
            "-target",
            "11",
            "-cp",
            str(api),
            "-d",
            str(output),
            *(str(path) for path in sources),
        ],
        dry_run=arguments.dry_run,
    )


def java_tck_run_command(
    arguments: argparse.Namespace,
    standard: str,
    api: Path,
    providers: list[Path],
    dependencies: list[Path],
    factory: str,
    classes: Path,
    result: Path,
    profile: str | None,
) -> list[str]:
    java = java_tool("java")
    runtime_jars = [*providers, *dependencies]
    classpath = os.pathsep.join(str(path) for path in [classes, api, *runtime_jars])
    if standard == "2010":
        command = [
            java,
            "-cp",
            classpath,
            f"-Dumbra.rti.tck2010.results={result.resolve()}",
            f"-Dumbra.rti.tck2010.factory={factory}",
        ]
        property_prefix = "umbra.rti.tck2010"
        main_class = "umbra.rti.tck1516e.RtiTck2010Main"
    else:
        if arguments.fom_path is None and not arguments.dry_run:
            raise CiError("The 2025 Java TCK requires --fom-path.")
        command = [
            java,
            "-cp",
            classpath,
            "-Dhla.rti.tck.time=HLAinteger64Time",
            f"-Dhla.rti.tck.apiJar={api.resolve()}",
            f"-Dhla.rti.tck.providerJars={os.pathsep.join(str(path) for path in runtime_jars)}",
        ]
        if arguments.fom_path is not None:
            fom = resolve_path(arguments.fom_path, label="2025 FOM module")
            assert fom is not None
            command.append(f"-Dhla.rti.tck.fom={fom}")
        command.append(f"-Dhla.rti.tck.factory={factory}")
        property_prefix = "hla.rti.tck"
        main_class = "org.hla.rti.tck.RtiTckMain"

    if arguments.fom_path is not None and standard == "2010":
        fom = resolve_path(arguments.fom_path, label="2010 FOM module")
        assert fom is not None
        command.append(f"-D{property_prefix}.fom={fom}")
    if arguments.mim_path is not None:
        mim = resolve_path(arguments.mim_path, label=f"{standard} MIM module")
        assert mim is not None
        command.append(f"-D{property_prefix}.mim={mim}")
    if profile is not None:
        profile_path = resolve_path(profile, label="Java capability profile")
        assert profile_path is not None
        command.append(f"-D{property_prefix}.capabilityProfile={profile_path}")
    if standard == "2025" and arguments.native_library is not None:
        native = resolve_path(arguments.native_library, label="JNI native library")
        assert native is not None
        command.append(f"-Dumbra.rti.jni.library={native}")
    command.append(main_class)
    return command


def java_route(arguments: argparse.Namespace, standard: str) -> None:
    root = REPOSITORY_ROOT / arguments.build_root
    output = root / f"java-{standard}"
    if not arguments.dry_run:
        output.mkdir(parents=True, exist_ok=True)
    supplied_provider = bool(arguments.provider_jar) or bool(
        os.environ.get(f"UMBRA_{standard}_PROVIDER_JAR")
    )
    if (
        standard == "2025"
        and requested_api_jar(arguments, standard)
        and not supplied_provider
    ):
        raise CiError(
            "A caller-supplied 2025 API JAR must be paired with --provider-jar; "
            "the checked-in mock fixture uses its own API surface."
        )
    api = (
        api_jar(arguments, standard)
        if standard == "2010" or requested_api_jar(arguments, standard)
        else None
    )
    providers = provider_jars(arguments, standard)
    dependencies = dependency_jars(arguments, standard)
    if dependencies and not providers:
        raise CiError("--dependency-jar requires at least one --provider-jar.")
    if not providers:
        fixture_output = output / "fixture"
        provider = build_java_fixture(arguments, standard, fixture_output)
        providers = [provider]
        if api is None:
            api = provider
        if standard == "2025" and not requested_api_jar(arguments, standard):
            print(
                "2025 Java fixture smoke is complete; pass --api-jar and --provider-jar for the full TCK."
            )
            return 0
    if api is None:
        api = api_jar(arguments, standard)
    assert api is not None
    factory = arguments.factory_name or getattr(arguments, "_ci_fixture_factory", None)
    if not factory:
        raise CiError("A Java provider JAR requires --factory-name.")
    if standard == "2010":
        preflight = [
            sys.executable,
            "tools/verify_1516e_provider_jar.py",
            "--api-jar",
            str(api),
        ]
        for provider in providers:
            preflight.extend(("--provider-jar", str(provider)))
        for dependency in dependencies:
            preflight.extend(("--dependency-jar", str(dependency)))
        run_command(preflight, dry_run=arguments.dry_run)
        tck_root = REPOSITORY_ROOT / "packages" / "umbra-rti-java-tck-2010"
        profile = arguments.capability_profile or (
            str(tck_root / "profiles" / "mock-java-rti-2010.properties")
            if factory == "Umbra mock Java 1516e"
            else None
        )
        result = output / "results.json"
        classes = output / "classes"
        compile_java_tck(arguments, standard, api, classes)
        run_command(
            java_tck_run_command(
                arguments,
                standard,
                api,
                providers,
                dependencies,
                factory,
                classes,
                result,
                profile,
            ),
            dry_run=arguments.dry_run,
        )
        verify = [sys.executable, "tools/verify_1516e_tck_results.py", str(result)]
        export = [
            sys.executable,
            "tools/java_tck_2010.py",
            str(result),
            "--provider",
            factory,
            "--output",
            str(output / "coverage.json"),
        ]
    else:
        result = output / "results.json"
        classes = output / "classes"
        compile_java_tck(arguments, standard, api, classes)
        run_command(
            java_tck_run_command(
                arguments,
                standard,
                api,
                providers,
                dependencies,
                factory,
                classes,
                result,
                None,
            ),
            dry_run=arguments.dry_run,
        )
        verify = [sys.executable, "tools/java_tck.py", "validate"]
        export = []

    run_command(verify, dry_run=arguments.dry_run)
    if export:
        run_command(export, dry_run=arguments.dry_run)


def validate_java_api_jar(api_path: Path, standard: str) -> None:
    if standard != "2010":
        return
    required = {
        "hla/rti1516e/RtiFactory.class",
        "hla/rti1516e/RtiFactoryFactory.class",
        "hla/rti1516e/RTIambassador.class",
        "hla/rti1516e/FederateAmbassador.class",
        "hla/rti1516e/LogicalTimeFactory.class",
        "hla/rti1516e/LogicalTimeFactoryFactory.class",
        "hla/rti1516e/time/HLAinteger64TimeFactory.class",
        "hla/rti1516e/time/HLAfloat64TimeFactory.class",
        "hla/rti1516e/encoding/EncoderFactory.class",
        "hla/rti1516e/exceptions/RTIinternalError.class",
    }
    with zipfile.ZipFile(api_path) as archive:
        missing = sorted(required - set(archive.namelist()))
    if missing:
        raise CiError(
            "The supplied 2010 API JAR is missing standard entries: "
            + ", ".join(missing)
        )


def validate_jni_archive(archive_path: Path, standard: str) -> None:
    with zipfile.ZipFile(archive_path) as archive:
        entries = set(archive.namelist())
        api_prefix = "hla/rti1516e/" if standard == "2010" else "hla/rti1516_2025/"
        shadowed = sorted(entry for entry in entries if entry.startswith(api_prefix))
        if shadowed:
            raise CiError(
                f"{standard} JNI bridge must not bundle IEEE API classes: "
                + ", ".join(shadowed)
            )
        descriptors = (
            {
                "META-INF/services/hla.rti1516e.RtiFactory": [
                    "org.umbra.jni.rti1516e.NativeRtiFactory"
                ],
                "META-INF/services/hla.rti1516e.LogicalTimeFactory": [
                    "org.umbra.jni.rti1516e.NativeInteger64TimeFactory",
                    "org.umbra.jni.rti1516e.NativeFloat64TimeFactory",
                ],
            }
            if standard == "2010"
            else {
                "META-INF/services/hla.rti1516_2025.RtiFactory": [
                    "org.umbra.jni.rti1516_2025.NativeRtiFactory"
                ],
                "META-INF/services/hla.rti1516_2025.auth.AuthorizerFactory": [
                    "org.umbra.jni.rti1516_2025.NativeAuthorizerFactory"
                ],
                "META-INF/services/hla.rti1516_2025.time.LogicalTimeFactory": [
                    "org.umbra.jni.rti1516_2025.NativeInteger64TimeFactory",
                    "org.umbra.jni.rti1516_2025.NativeFloat64TimeFactory",
                ],
            }
        )
        for descriptor, expected in descriptors.items():
            if descriptor not in entries:
                raise CiError(
                    f"JNI bridge is missing ServiceLoader descriptor: {descriptor}"
                )
            providers = [
                line.strip()
                for line in archive.read(descriptor).decode("utf-8").splitlines()
                if line.strip()
            ]
            if providers != expected:
                raise CiError(
                    f"unexpected {standard} JNI ServiceLoader provider "
                    f"for {descriptor}: {providers}"
                )
            for provider in expected:
                provider_entry = provider.replace(".", "/") + ".class"
                if provider_entry not in entries:
                    raise CiError(
                        f"{standard} JNI ServiceLoader provider class is missing: "
                        f"{provider_entry}"
                    )


def jni_route(arguments: argparse.Namespace, standard: str) -> None:
    output = (REPOSITORY_ROOT / arguments.build_root / f"jni-{standard}").resolve()
    package = (
        REPOSITORY_ROOT
        / "packages"
        / ("umbra-rti-jni-2010" if standard == "2010" else "umbra-rti-jni")
    )
    native_build = output / "native"
    classes = output / "classes"
    jar_path = output / (
        "umbra-rti-jni-2010.jar" if standard == "2010" else "umbra-rti-jni.jar"
    )
    native_library = output / (
        "umbra_rti_jni_2010.dll" if standard == "2010" else "umbra_rti_jni.dll"
    )
    if not arguments.dry_run:
        classes.mkdir(parents=True, exist_ok=True)

    if standard == "2010":
        api = api_jar(arguments, standard)
        if not arguments.dry_run:
            validate_java_api_jar(api, standard)
        cmake_arguments = ["-S", str(package), "-B", str(native_build)]
        target = "umbra_rti_jni_2010"
        source_label = "2010 JNI"
        source_level = True
    else:
        requested_api = requested_api_jar(arguments, standard)
        if requested_api:
            api = resolve_path(requested_api, label="2025 Java API JAR")
            assert api is not None
        else:
            api = build_java_fixture(
                arguments, standard, output / "mock-java-api", run_smoke=False
            )
        cmake_arguments = [
            "-S",
            str(package),
            "-B",
            str(native_build),
            "-DUMBRA_ENABLE_LIBXML2_FOM_VALIDATOR=ON",
            "-DUMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=ON",
            "-DUMBRA_FETCH_LIBXML2=ON",
        ]
        cached_libxml = (
            REPOSITORY_ROOT
            / "packages"
            / "umbra-rti-native"
            / "build"
            / "cp312-cp312-win_amd64"
            / "_deps"
            / "libxml2-src"
        )
        if cached_libxml.is_dir():
            cmake_arguments.append(f"-DFETCHCONTENT_SOURCE_DIR_LIBXML2={cached_libxml}")
        target = "umbra_rti_jni"
        source_label = "2025 JNI"
        source_level = False

    cmake = find_executable(arguments.cmake, label="CMake")
    run_command([cmake, *cmake_arguments], dry_run=arguments.dry_run)
    run_command(
        [
            cmake,
            "--build",
            str(native_build),
            "--config",
            "Release",
            "--target",
            target,
        ],
        dry_run=arguments.dry_run,
    )
    if not arguments.dry_run:
        names = (
            (
                "umbra_rti_jni_2010.dll",
                "libumbra_rti_jni_2010.so",
                "libumbra_rti_jni_2010.dylib",
            )
            if standard == "2010"
            else (
                "umbra_rti_jni.dll",
                "libumbra_rti_jni.so",
                "libumbra_rti_jni.dylib",
            )
        )
        built = next(
            (path for name in names for path in native_build.rglob(name)), None
        )
        if built is None:
            raise CiError(
                f"CMake did not produce the {standard} JNI native library below "
                f"{native_build}"
            )
        shutil.copy2(built, native_library)

    source_root = package / "src" / "main" / "java"
    sources = java_sources(source_root, label=source_label)
    javac = java_tool("javac")
    compile_command = [javac]
    if source_level:
        compile_command.extend(("-encoding", "UTF-8", "-source", "11", "-target", "11"))
    compile_command.extend(
        ("-cp", str(api), "-d", str(classes), *(str(path) for path in sources))
    )
    run_command(compile_command, dry_run=arguments.dry_run)
    copy_resource_tree(
        package / "src" / "main" / "resources", classes, dry_run=arguments.dry_run
    )
    run_command(
        [
            java_tool("jar"),
            "--create",
            "--file",
            str(jar_path),
            "-C",
            str(classes),
            ".",
        ],
        dry_run=arguments.dry_run,
    )
    if not arguments.dry_run:
        validate_jni_archive(jar_path, standard)
    classpath = os.pathsep.join((str(api), str(jar_path)))
    smoke_classes = (
        (
            "org.umbra.jni.rti1516e.SurfaceSmokeTest",
            "org.umbra.jni.rti1516e.NativeSmokeTest",
            "org.umbra.jni.rti1516e.NativeTypeRoundTripTest",
        )
        if standard == "2010"
        else (
            "org.umbra.jni.rti1516_2025.StandardSurfaceSmokeTest",
            "org.umbra.jni.rti1516_2025.NativeSmokeTest",
        )
    )
    property_name = (
        "umbra.rti.jni.2010.library" if standard == "2010" else "umbra.rti.jni.library"
    )
    for smoke_class in smoke_classes:
        smoke = [
            java_tool("java"),
            f"-D{property_name}={native_library}",
            "-cp",
            classpath,
            smoke_class,
        ]
        if standard == "2025":
            smoke_fom = (
                REPOSITORY_ROOT
                / "third_party"
                / "ieee1516.2-2025"
                / "resources"
                / "examples"
                / "RestaurantFOMmodule-2025.xml"
            )
            if smoke_fom.is_file():
                smoke.append(str(smoke_fom))
        run_command(smoke, dry_run=arguments.dry_run)


def run_tests(arguments: argparse.Namespace) -> int:
    require_python_version()
    lanes = selected_lanes(arguments)
    print("Selected CI lanes:", flush=True)
    for standard, route in lanes:
        print(f"  {standard}-{route}", flush=True)
    for standard, route in lanes:
        print(f"\n=== {standard}-{route} ===", flush=True)
        if route == "cpp":
            native_route(arguments, standard)
        elif route == "java":
            java_route(arguments, standard)
        elif route == "jni":
            jni_route(arguments, standard)
        elif route == "python":
            python_route(arguments, standard)
    return 0


def tracked_paths(*, suffixes: tuple[str, ...] | None = None) -> list[Path]:
    result = subprocess.run(
        ["git", "ls-files", "--cached", "--others", "--exclude-standard"],
        cwd=REPOSITORY_ROOT,
        check=True,
        capture_output=True,
        text=True,
    )
    paths: list[Path] = []
    for item in result.stdout.splitlines():
        path = REPOSITORY_ROOT / item
        if path.is_file() and (suffixes is None or path.suffix.lower() in suffixes):
            paths.append(path)
    return paths


def changed_paths(*, suffixes: tuple[str, ...]) -> list[Path]:
    result = subprocess.run(
        ["git", "diff", "--name-only", "--diff-filter=ACMR", "HEAD", "--", "."],
        cwd=REPOSITORY_ROOT,
        check=True,
        capture_output=True,
        text=True,
    )
    paths: list[Path] = []
    for item in result.stdout.splitlines():
        path = REPOSITORY_ROOT / item
        if path.is_file() and path.suffix.lower() in suffixes:
            paths.append(path)
    return paths


def lint_scope_paths(scope: str) -> list[Path]:
    if scope == "ci":
        return [
            REPOSITORY_ROOT / "tools" / "ci.py",
            REPOSITORY_ROOT / "tools" / "__init__.py",
        ]
    if scope == "changed":
        return changed_paths(suffixes=(".py", ".pyi"))
    return tracked_paths(suffixes=(".py", ".pyi"))


def run_python_integrity_checks(arguments: argparse.Namespace) -> None:
    checks = [
        [sys.executable, "tools/generate_rti_ambassador_shell.py", "--check"],
        [sys.executable, "tools/generate_1516e_cpp_shell.py", "--check"],
        [sys.executable, "tools/verify_1516e_provider_boundaries.py"],
        [
            sys.executable,
            "tools/verify_hla_symbolic_names.py",
            "--root",
            str(REPOSITORY_ROOT),
        ],
        [sys.executable, "tools/verify_ieee_headers.py"],
        [sys.executable, "tools/verify_ieee_exception_binding.py"],
        [sys.executable, "tools/verify_python_surface_report.py"],
    ]
    for check in checks:
        run_command(
            check,
            dry_run=arguments.dry_run,
            env=python_environment(),
        )
    python_2010_surface_checks(arguments)


def run_ruff(arguments: argparse.Namespace, *, fix: bool) -> None:
    executable = shutil.which("ruff")
    if executable is None:
        message = (
            "ruff is not installed; install it to run Python formatting/lint checks."
        )
        if arguments.strict:
            raise CiError(message)
        print(f"! {message}", file=sys.stderr)
        return
    paths = lint_scope_paths(arguments.scope)
    if not paths:
        print(f"! no Python files found for lint scope '{arguments.scope}'")
        return
    path_arguments = [str(path.relative_to(REPOSITORY_ROOT)) for path in paths]
    if fix:
        run_command(
            [executable, "check", "--fix", *path_arguments], dry_run=arguments.dry_run
        )
        run_command([executable, "format", *path_arguments], dry_run=arguments.dry_run)
    else:
        run_command([executable, "check", *path_arguments], dry_run=arguments.dry_run)
        run_command(
            [executable, "format", "--check", *path_arguments],
            dry_run=arguments.dry_run,
        )


def run_clang_format(arguments: argparse.Namespace, *, fix: bool) -> None:
    executable = shutil.which("clang-format")
    if executable is None:
        message = "clang-format is not installed; C++ formatting was skipped."
        if arguments.strict:
            raise CiError(message)
        print(f"! {message}", file=sys.stderr)
        return
    suffixes = (".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp")
    paths = (
        changed_paths(suffixes=suffixes)
        if arguments.scope == "changed"
        else tracked_paths(suffixes=suffixes)
    )
    paths = [
        path
        for path in paths
        if "third_party" not in path.relative_to(REPOSITORY_ROOT).parts
        and "cpp/generated" not in str(path.relative_to(REPOSITORY_ROOT))
    ]
    if arguments.scope == "ci":
        paths = []
    if not paths:
        return
    path_arguments = [str(path.relative_to(REPOSITORY_ROOT)) for path in paths]
    if fix:
        run_command([executable, "-i", *path_arguments], dry_run=arguments.dry_run)
    else:
        run_command(
            [executable, "--dry-run", "--Werror", *path_arguments],
            dry_run=arguments.dry_run,
        )


def run_whitespace_check() -> None:
    result = subprocess.run(
        ["git", "diff", "--check"], cwd=REPOSITORY_ROOT, check=False
    )
    if result.returncode:
        raise CiError("git diff --check found whitespace errors in the current diff.")


def lint_command(arguments: argparse.Namespace) -> int:
    require_python_version()
    run_python_integrity_checks(arguments)
    run_ruff(arguments, fix=False)
    run_clang_format(arguments, fix=False)
    if not arguments.dry_run:
        run_whitespace_check()
    return 0


def fix_command(arguments: argparse.Namespace) -> int:
    require_python_version()
    print(">> Regenerating checked-in source artifacts", flush=True)
    generators = [
        [sys.executable, "tools/generate_rti_ambassador_shell.py"],
        [sys.executable, "tools/generate_1516e_cpp_shell.py"],
        [sys.executable, "tools/generate_binding_inventory.py"],
    ]
    for generator in generators:
        run_command(generator, dry_run=arguments.dry_run)
    run_ruff(arguments, fix=True)
    run_clang_format(arguments, fix=True)
    return 0


SAFE_CLEAN_TARGETS = {
    "ci": (REPOSITORY_ROOT / "out" / "ci",),
    "cmake": (REPOSITORY_ROOT / "out" / "cmake",),
    "java": (REPOSITORY_ROOT / "out" / "java-tck",),
    "all": (
        REPOSITORY_ROOT / "out" / "ci",
        REPOSITORY_ROOT / "out" / "cmake",
        REPOSITORY_ROOT / "out" / "java-tck",
    ),
}


def clean_command(arguments: argparse.Namespace) -> int:
    target_paths = SAFE_CLEAN_TARGETS[arguments.target]
    for path in target_paths:
        resolved = path.resolve()
        if REPOSITORY_ROOT not in resolved.parents or resolved == REPOSITORY_ROOT:
            raise CiError(
                f"Refusing to clean a path outside the repository output boundary: {resolved}"
            )
        if not resolved.exists():
            print(f"- absent {resolved}")
            continue
        print(f"- remove {resolved}")
        if not arguments.dry_run:
            shutil.rmtree(resolved)
    return 0


def doctor_command(arguments: argparse.Namespace) -> int:
    require_python_version()
    print(f"repository: {REPOSITORY_ROOT}")
    print(f"python: {sys.executable} ({sys.version.split()[0]})")
    for name in ("cmake", "ctest", "java", "javac", "jar", "ruff", "clang-format"):
        executable = shutil.which(name)
        print(f"{name}: {executable or 'missing'}")
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command")

    subparsers.add_parser(
        "list", help="List route commands and legacy native profiles."
    )

    test_parser = subparsers.add_parser(
        "test", help="Run one or more standard/transport test lanes."
    )
    add_test_arguments(test_parser)

    all_parser = subparsers.add_parser(
        "all", help="Run lint checks followed by every route lane."
    )
    add_test_arguments(all_parser)
    all_parser.set_defaults(run_lint=True)

    for name, help_text in (
        ("lint", "Run integrity, formatting, and whitespace checks."),
        ("fix", "Regenerate artifacts and apply available formatter fixes."),
    ):
        command_parser = subparsers.add_parser(name, help=help_text)
        command_parser.add_argument(
            "--scope", choices=("ci", "changed", "all"), default="ci"
        )
        command_parser.add_argument(
            "--strict",
            action="store_true",
            help="Fail when optional formatters are unavailable.",
        )
        command_parser.add_argument(
            "--dry-run",
            action="store_true",
            help="Print commands without running them.",
        )

    clean_parser = subparsers.add_parser(
        "clean", help="Remove disposable output below out/."
    )
    clean_parser.add_argument(
        "--target", choices=sorted(SAFE_CLEAN_TARGETS), default="ci"
    )
    clean_parser.add_argument(
        "--dry-run", action="store_true", help="Print removals without deleting them."
    )

    subparsers.add_parser(
        "doctor", help="Report the executables visible to the CI runner."
    )
    return parser


def list_routes() -> None:
    print("Route commands:")
    print("  python -m tools.ci test --standard 2010 --route cpp")
    print("  python -m tools.ci test --standard 2025 --route java")
    print("  python -m tools.ci test --standard 2010 --route jni --api-jar PATH")
    print("  python -m tools.ci test --standard 2025 --route python")
    print("  python -m tools.ci test --standard all --route all")
    print("  python -m tools.ci lint [--scope ci|changed|all]")
    print("  python -m tools.ci fix [--scope ci|changed|all]")
    print("  python -m tools.ci clean [--target ci|cmake|java|all]")
    print("\nJava API archives are caller-supplied and are never vendored.")
    list_profiles()


def new_command(argv: Sequence[str]) -> int:
    parser = build_parser()
    arguments = parser.parse_args(argv)
    if arguments.command is None:
        parser.print_help()
        return 2
    if arguments.command == "list":
        list_routes()
        return 0
    if arguments.command == "test":
        return run_tests(arguments)
    if arguments.command == "all":
        lint_arguments = argparse.Namespace(
            scope="ci", strict=False, dry_run=arguments.dry_run
        )
        lint_command(lint_arguments)
        return run_tests(arguments)
    if arguments.command == "lint":
        return lint_command(arguments)
    if arguments.command == "fix":
        return fix_command(arguments)
    if arguments.command == "clean":
        return clean_command(arguments)
    if arguments.command == "doctor":
        return doctor_command(arguments)
    parser.error(f"unknown command: {arguments.command}")
    return 2


def main(argv: Sequence[str] | None = None) -> int:
    arguments = list(sys.argv[1:] if argv is None else argv)
    if arguments and (arguments[0] in PROFILES or arguments[0] == "--list-profiles"):
        return run_legacy(arguments)
    try:
        return new_command(arguments)
    except CiError as error:
        print(f"CI error: {error}", file=sys.stderr)
        return 2
    except FileNotFoundError as error:
        print(f"CI tool not found: {error.filename}", file=sys.stderr)
        return 2
    except subprocess.CalledProcessError as error:
        print(f"CI command failed with exit code {error.returncode}.", file=sys.stderr)
        return error.returncode or 1


if __name__ == "__main__":
    raise SystemExit(main())
