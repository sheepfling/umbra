"""Shared Python implementation for the Umbra Java/JNI product workflow.

The product is deliberately a directory rather than a shaded JAR.  It keeps
the standard IEEE API JAR authoritative, places the bounded bridge and native
library beside it, and records the exact files and hashes in JSON manifests.
The module is used by the build and verification entry points in this
directory; the copied ``run.py`` launcher is self-contained so a staged
directory can be moved without the repository checkout.
"""

from __future__ import annotations

import hashlib
import json
import os
import shlex
import shutil
import subprocess
import zipfile
from collections.abc import Sequence
from dataclasses import dataclass
from pathlib import Path


class ProductError(RuntimeError):
    """A user-actionable product build or verification failure."""


@dataclass(frozen=True)
class EditionConfig:
    key: str
    label: str
    package_root: Path
    bridge_jar_name: str
    native_names: tuple[str, ...]
    api_prefix: str
    required_api_entries: tuple[str, ...]
    service_descriptors: tuple[tuple[str, tuple[str, ...]], ...]
    smoke_classes: tuple[str, ...]
    property_name: str
    capability_profile: str
    manifest_name: str
    dependency_manifest_name: str
    default_api_coordinate: str
    default_api_version: str


@dataclass(frozen=True)
class BuildArtifacts:
    build_root: Path
    api_jar: Path
    bridge_jar: Path
    native_library: Path


@dataclass(frozen=True)
class ProductArtifacts:
    artifact_root: Path
    api_jar: Path
    bridge_jar: Path
    native_library: Path


def repository_root() -> Path:
    return Path(__file__).resolve().parents[1]


def edition_config(edition: str, root: Path | None = None) -> EditionConfig:
    """Return the immutable file and API contract for one IEEE edition."""

    root = (root or repository_root()).resolve()
    packages = root / "packages"
    if edition == "2025":
        return EditionConfig(
            key="2025",
            label="IEEE 1516.1-2025",
            package_root=packages / "umbra-rti-jni",
            bridge_jar_name="umbra-rti-jni.jar",
            native_names=(
                "umbra_rti_jni.dll",
                "libumbra_rti_jni.so",
                "libumbra_rti_jni.dylib",
            ),
            api_prefix="hla/rti1516_2025/",
            required_api_entries=(
                "hla/rti1516_2025/RtiFactoryFactory.class",
                "hla/rti1516_2025/RtiFactory.class",
                "hla/rti1516_2025/RTIambassador.class",
                "hla/rti1516_2025/FederateAmbassador.class",
                "hla/rti1516_2025/encoding/EncoderFactory.class",
                "hla/rti1516_2025/time/LogicalTimeFactory.class",
                "hla/rti1516_2025/time/LogicalTimeFactoryFactory.class",
                "hla/rti1516_2025/time/HLAinteger64TimeFactory.class",
                "hla/rti1516_2025/time/HLAfloat64TimeFactory.class",
                "hla/rti1516_2025/auth/AuthorizerFactory.class",
                "hla/rti1516_2025/auth/AuthorizerFactoryFactory.class",
            ),
            service_descriptors=(
                (
                    "META-INF/services/hla.rti1516_2025.RtiFactory",
                    ("org.umbra.jni.rti1516_2025.NativeRtiFactory",),
                ),
                (
                    "META-INF/services/hla.rti1516_2025.auth.AuthorizerFactory",
                    ("org.umbra.jni.rti1516_2025.NativeAuthorizerFactory",),
                ),
                (
                    "META-INF/services/hla.rti1516_2025.time.LogicalTimeFactory",
                    (
                        "org.umbra.jni.rti1516_2025.NativeInteger64TimeFactory",
                        "org.umbra.jni.rti1516_2025.NativeFloat64TimeFactory",
                    ),
                ),
            ),
            smoke_classes=(
                "org.umbra.jni.rti1516_2025.StandardSurfaceSmokeTest",
                "org.umbra.jni.rti1516_2025.NativeSmokeTest",
            ),
            property_name="umbra.rti.jni.library",
            capability_profile="bounded-jni-bridge",
            manifest_name="umbra-rti-jni-manifest.json",
            dependency_manifest_name="umbra-rti-jni-dependencies.json",
            default_api_coordinate="se.pitch.oss.fedpro:hla-4-api",
            default_api_version="2.1.0",
        )
    if edition == "2010":
        return EditionConfig(
            key="2010",
            label="IEEE 1516e-2010",
            package_root=packages / "umbra-rti-jni-2010",
            bridge_jar_name="umbra-rti-jni-2010.jar",
            native_names=(
                "umbra_rti_jni_2010.dll",
                "libumbra_rti_jni_2010.so",
                "libumbra_rti_jni_2010.dylib",
            ),
            api_prefix="hla/rti1516e/",
            required_api_entries=(
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
            ),
            service_descriptors=(
                (
                    "META-INF/services/hla.rti1516e.RtiFactory",
                    ("org.umbra.jni.rti1516e.NativeRtiFactory",),
                ),
                (
                    "META-INF/services/hla.rti1516e.LogicalTimeFactory",
                    (
                        "org.umbra.jni.rti1516e.NativeInteger64TimeFactory",
                        "org.umbra.jni.rti1516e.NativeFloat64TimeFactory",
                    ),
                ),
            ),
            smoke_classes=(
                "org.umbra.jni.rti1516e.SurfaceSmokeTest",
                "org.umbra.jni.rti1516e.NativeSmokeTest",
                "org.umbra.jni.rti1516e.NativeTypeRoundTripTest",
            ),
            property_name="umbra.rti.jni.2010.library",
            capability_profile="bounded-null-provider",
            manifest_name="umbra-rti-jni-2010-manifest.json",
            dependency_manifest_name="umbra-rti-jni-2010-dependencies.json",
            default_api_coordinate="ieee:1516e-java-api",
            default_api_version="2010",
        )
    raise ProductError(f"Unsupported IEEE JNI edition: {edition!r}")


def required_file(value: str | Path, label: str) -> Path:
    path = Path(value).expanduser().resolve()
    if not path.is_file():
        raise ProductError(f"{label} does not exist: {path}")
    return path


def _tool_candidates(name: str) -> list[Path]:
    candidates: list[Path] = []
    from_path = shutil.which(name)
    if from_path:
        candidates.append(Path(from_path))
    java_home = os.environ.get("JAVA_HOME")
    if java_home and name in {"java", "javac", "jar"}:
        bin_root = Path(java_home).expanduser() / "bin"
        candidates.extend((bin_root / name, bin_root / f"{name}.exe"))
    return candidates


def required_tool(name: str) -> Path:
    for candidate in _tool_candidates(name):
        if candidate.is_file():
            return candidate
    raise ProductError(
        f"Required tool {name!r} was not found on PATH or below JAVA_HOME."
    )


def _display_command(command: Sequence[str | Path]) -> str:
    values = [str(value) for value in command]
    return subprocess.list2cmdline(values) if os.name == "nt" else shlex.join(values)


def run_command(command: Sequence[str | Path], *, cwd: Path, description: str) -> None:
    values = [str(value) for value in command]
    print(f"+ {_display_command(values)}", flush=True)
    try:
        subprocess.run(values, cwd=cwd, check=True)
    except FileNotFoundError as exc:
        raise ProductError(f"{description} could not start: {values[0]}") from exc
    except subprocess.CalledProcessError as exc:
        raise ProductError(
            f"{description} failed with exit code {exc.returncode}"
        ) from exc


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def _archive_entries(path: Path) -> set[str]:
    try:
        with zipfile.ZipFile(path) as archive:
            return set(archive.namelist())
    except (OSError, zipfile.BadZipFile) as exc:
        raise ProductError(f"Could not inspect JAR archive: {path}") from exc


def validate_java_api_jar(api_path: Path, config: EditionConfig) -> None:
    """Require the standard classes used by the bridge and smoke tests."""

    entries = _archive_entries(api_path)
    missing = sorted(set(config.required_api_entries) - entries)
    if missing:
        raise ProductError(
            f"The supplied {config.label} Java API JAR is missing standard entries: "
            + ", ".join(missing)
        )


def _service_providers(data: bytes) -> list[str]:
    try:
        text = data.decode("utf-8")
    except UnicodeDecodeError as exc:
        raise ProductError("A ServiceLoader descriptor is not valid UTF-8") from exc
    return [line.strip() for line in text.splitlines() if line.strip()]


def validate_jni_archive(archive_path: Path, config: EditionConfig) -> None:
    """Validate API ownership and exact provider registration in a bridge JAR."""

    try:
        with zipfile.ZipFile(archive_path) as archive:
            entries = set(archive.namelist())
            shadowed = sorted(
                entry for entry in entries if entry.startswith(config.api_prefix)
            )
            if shadowed:
                raise ProductError(
                    f"{config.label} JNI bridge must not bundle IEEE API classes: "
                    + ", ".join(shadowed)
                )

            for descriptor, expected in config.service_descriptors:
                if descriptor not in entries:
                    raise ProductError(
                        f"{config.label} JNI bridge is missing ServiceLoader "
                        f"descriptor: {descriptor}"
                    )
                providers = _service_providers(archive.read(descriptor))
                if providers != list(expected):
                    raise ProductError(
                        f"unexpected {config.label} JNI ServiceLoader providers "
                        f"for {descriptor}: {providers}"
                    )
                for provider in expected:
                    provider_entry = provider.replace(".", "/") + ".class"
                    if provider_entry not in entries:
                        raise ProductError(
                            f"{config.label} JNI ServiceLoader provider class is "
                            f"missing: {provider_entry}"
                        )
    except (OSError, zipfile.BadZipFile) as exc:
        raise ProductError(f"Could not inspect JNI bridge JAR: {archive_path}") from exc


def _built_native_library(native_build: Path, config: EditionConfig) -> Path:
    for name in config.native_names:
        matches = sorted(native_build.rglob(name)) if native_build.is_dir() else []
        if matches:
            return matches[0].resolve()
    names = ", ".join(config.native_names)
    raise ProductError(
        f"CMake did not produce a {config.label} JNI native library below "
        f"{native_build}; expected one of {names}"
    )


def build_bridge(
    config: EditionConfig, api_path: Path, build_root: Path
) -> BuildArtifacts:
    """Configure CMake, build native JNI, compile Java, and create the bridge JAR."""

    api_path = required_file(api_path, f"{config.label} Java API JAR")
    validate_java_api_jar(api_path, config)
    if api_path.name == config.bridge_jar_name:
        raise ProductError(
            f"The API JAR cannot be named {config.bridge_jar_name}; it would "
            "overwrite the JNI bridge in the product directory."
        )

    build_root = build_root.expanduser().resolve()
    build_root.mkdir(parents=True, exist_ok=True)
    native_build = build_root / "native"
    classes = build_root / "classes"
    if classes.exists():
        shutil.rmtree(classes)
    classes.mkdir(parents=True, exist_ok=True)

    root = config.package_root.parents[1]
    cmake = required_tool("cmake")
    cmake_arguments: list[str | Path] = [
        "-S",
        config.package_root,
        "-B",
        native_build,
    ]
    if config.key == "2025":
        cmake_arguments.extend(
            (
                "-DUMBRA_ENABLE_LIBXML2_FOM_VALIDATOR=ON",
                "-DUMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=ON",
                "-DUMBRA_FETCH_LIBXML2=ON",
            )
        )
        cached_libxml = (
            root
            / "packages"
            / "umbra-rti-native"
            / "build"
            / "cp312-cp312-win_amd64"
            / "_deps"
            / "libxml2-src"
        )
        if cached_libxml.is_dir():
            cmake_arguments.append(f"-DFETCHCONTENT_SOURCE_DIR_LIBXML2={cached_libxml}")
    run_command([cmake, *cmake_arguments], cwd=root, description="CMake configuration")
    target = "umbra_rti_jni" if config.key == "2025" else "umbra_rti_jni_2010"
    run_command(
        [
            cmake,
            "--build",
            native_build,
            "--config",
            "Release",
            "--target",
            target,
        ],
        cwd=root,
        description=f"{config.label} native JNI build",
    )
    built_native = _built_native_library(native_build, config)
    native_target = build_root / built_native.name
    shutil.copy2(built_native, native_target)

    source_root = config.package_root / "src" / "main" / "java"
    sources = sorted(source_root.rglob("*.java")) if source_root.is_dir() else []
    if not sources:
        raise ProductError(
            f"No {config.label} JNI Java sources found below {source_root}"
        )
    javac = required_tool("javac")
    compile_command: list[str | Path] = [javac]
    if config.key == "2010":
        compile_command.extend(("-encoding", "UTF-8", "-source", "11", "-target", "11"))
    compile_command.extend(("-cp", api_path, "-d", classes, *sources))
    run_command(
        compile_command, cwd=root, description=f"{config.label} Java bridge compile"
    )

    resources = config.package_root / "src" / "main" / "resources"
    if resources.is_dir():
        shutil.copytree(resources, classes, dirs_exist_ok=True)
    bridge_jar = build_root / config.bridge_jar_name
    if bridge_jar.exists():
        bridge_jar.unlink()
    run_command(
        [required_tool("jar"), "--create", "--file", bridge_jar, "-C", classes, "."],
        cwd=root,
        description=f"{config.label} JNI bridge JAR creation",
    )
    validate_jni_archive(bridge_jar, config)
    return BuildArtifacts(build_root, api_path, bridge_jar, native_target)


def _artifact_native_library(artifact_root: Path, config: EditionConfig) -> Path:
    for name in config.native_names:
        candidate = artifact_root / name
        if candidate.is_file():
            return candidate.resolve()
    raise ProductError(
        f"{config.label} JNI native library is missing from {artifact_root}"
    )


def discover_api_jar(
    artifact_root: Path, config: EditionConfig, explicit: str | Path | None = None
) -> Path:
    if explicit is not None:
        return required_file(explicit, f"{config.label} Java API JAR")
    candidates = sorted(
        path
        for path in artifact_root.glob("*.jar")
        if path.is_file() and path.name != config.bridge_jar_name
    )
    if len(candidates) != 1:
        names = ", ".join(path.name for path in candidates) or "none"
        raise ProductError(
            f"Pass --java-api-jar when the product directory does not contain "
            f"exactly one API JAR (found: {names})"
        )
    return candidates[0].resolve()


def _copy_file(source: Path, target: Path) -> None:
    target.parent.mkdir(parents=True, exist_ok=True)
    if source.resolve() != target.resolve():
        shutil.copy2(source, target)


def _relative_or_name(path: Path, root: Path) -> str:
    try:
        return path.resolve().relative_to(root.resolve()).as_posix()
    except ValueError:
        return path.name


def _write_json(path: Path, payload: object) -> None:
    path.write_text(
        json.dumps(payload, indent=2, sort_keys=False) + "\n", encoding="utf-8"
    )


def write_product_manifests(
    config: EditionConfig,
    artifact_root: Path,
    api_path: Path,
    bridge_path: Path,
    native_path: Path,
    *,
    api_coordinate: str,
    api_version: str,
    api_source: str | None,
) -> tuple[Path, Path]:
    """Write portable verification and dependency manifests for a bundle."""

    api_name = _relative_or_name(api_path, artifact_root)
    bridge_name = _relative_or_name(bridge_path, artifact_root)
    native_name = _relative_or_name(native_path, artifact_root)
    manifest = {
        "schemaVersion": 1,
        "edition": config.label,
        "capabilityProfile": config.capability_profile,
        "launcher": "run.py",
        "javaApiJar": api_name,
        "javaApiSha256": sha256_file(api_path),
        "bridgeJar": bridge_name,
        "bridgeJarSha256": sha256_file(bridge_path),
        "nativeLibrary": native_name,
        "nativeLibrarySha256": sha256_file(native_path),
    }
    dependency = {
        "schemaVersion": 1,
        "edition": config.label,
        "capabilityProfile": config.capability_profile,
        "launcher": "run.py",
        "bridge": {
            "file": bridge_name,
            "sha256": sha256_file(bridge_path),
        },
        "native": {
            "file": native_name,
            "sha256": sha256_file(native_path),
        },
        "javaApi": {
            "coordinate": api_coordinate,
            "version": api_version,
            "file": api_name,
            "sha256": sha256_file(api_path),
            "bundled": api_path.resolve().parent == artifact_root.resolve(),
        },
        "notes": [
            f"The {config.label} Java API remains the authoritative standard surface.",
            "The product directory includes the declared Java API dependency and a standard ServiceLoader launcher.",
            "The bridge exposes bounded RTI behavior; this bundle is not a full RTI-conformance claim.",
            "Redistribution rights and the API source URL must be recorded by the release owner.",
        ],
    }
    if api_source:
        dependency["javaApi"]["source"] = api_source
    manifest_path = artifact_root / config.manifest_name
    dependency_path = artifact_root / config.dependency_manifest_name
    _write_json(manifest_path, manifest)
    _write_json(dependency_path, dependency)
    return manifest_path, dependency_path


def stage_product(
    config: EditionConfig,
    artifacts: BuildArtifacts,
    output_root: Path,
    *,
    api_coordinate: str,
    api_version: str,
    api_source: str | None,
) -> ProductArtifacts:
    """Stage a movable product directory and its Python launcher."""

    output_root = output_root.expanduser().resolve()
    output_root.mkdir(parents=True, exist_ok=True)
    bridge_target = output_root / config.bridge_jar_name
    native_target = output_root / artifacts.native_library.name
    api_target = output_root / artifacts.api_jar.name
    _copy_file(artifacts.bridge_jar, bridge_target)
    _copy_file(artifacts.native_library, native_target)
    _copy_file(artifacts.api_jar, api_target)

    launcher_source = Path(__file__).resolve().parent / "run_umbra_jni.py"
    launcher_target = output_root / "run.py"
    _copy_file(
        required_file(launcher_source, "Python product launcher"), launcher_target
    )
    write_product_manifests(
        config,
        output_root,
        api_target,
        bridge_target,
        native_target,
        api_coordinate=api_coordinate,
        api_version=api_version,
        api_source=api_source,
    )
    return ProductArtifacts(output_root, api_target, bridge_target, native_target)


def _manifest_file_name(value: object, label: str) -> str:
    if not isinstance(value, str) or not value:
        raise ProductError(f"Product manifest field {label} is missing or invalid")
    return Path(value).name


def _validate_product_manifest(
    config: EditionConfig,
    artifact_root: Path,
    api_path: Path,
    bridge_path: Path,
    native_path: Path,
) -> None:
    manifest_path = artifact_root / config.manifest_name
    dependency_path = artifact_root / config.dependency_manifest_name
    for path, description in (
        (manifest_path, "verification manifest"),
        (dependency_path, "dependency manifest"),
        (artifact_root / "run.py", "Python product launcher"),
    ):
        if not path.is_file():
            raise ProductError(f"Product {description} is missing: {path}")
    try:
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        dependency = json.loads(dependency_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ProductError(
            f"Product manifests could not be read below {artifact_root}"
        ) from exc
    if manifest.get("edition") != config.label:
        raise ProductError(
            f"Product manifest edition is {manifest.get('edition')!r}, expected {config.label!r}"
        )
    if manifest.get("launcher") != "run.py":
        raise ProductError("Product manifest must identify the Python launcher run.py")
    expected_names = {
        "javaApiJar": api_path.name,
        "bridgeJar": bridge_path.name,
        "nativeLibrary": native_path.name,
    }
    for field, expected in expected_names.items():
        actual = _manifest_file_name(manifest.get(field), field)
        if actual != expected:
            raise ProductError(
                f"Product manifest {field} is {actual!r}, expected {expected!r}"
            )
    expected_hashes = {
        "javaApiSha256": sha256_file(api_path),
        "bridgeJarSha256": sha256_file(bridge_path),
        "nativeLibrarySha256": sha256_file(native_path),
    }
    for field, expected in expected_hashes.items():
        actual = manifest.get(field)
        if actual != expected:
            raise ProductError(
                f"Product manifest {field} is {actual!r}, expected {expected!r}"
            )
    if (
        dependency.get("edition") != config.label
        or dependency.get("launcher") != "run.py"
    ):
        raise ProductError(
            "Product dependency manifest has the wrong edition or launcher"
        )


def _check_expected_hash(path: Path, expected: str | None, label: str) -> str:
    actual = sha256_file(path)
    if expected and actual != expected.strip().upper():
        raise ProductError(
            f"{label} SHA-256 mismatch: expected {expected}, actual {actual}"
        )
    return actual


def run_java_smoke_tests(
    config: EditionConfig,
    api_path: Path,
    bridge_path: Path,
    native_path: Path,
    *,
    fom_path: str | Path | None = None,
) -> None:
    """Run the edition's release smoke classes against a staged directory."""

    java = required_tool("java")
    classpath = os.pathsep.join((str(api_path), str(bridge_path)))
    explicit_fom: Path | None = None
    if fom_path is not None:
        explicit_fom = required_file(fom_path, "FOM smoke path")
    default_fom = (
        config.package_root.parents[1]
        / "third_party"
        / "ieee1516.2-2025"
        / "resources"
        / "examples"
        / "RestaurantFOMmodule-2025.xml"
    )
    for smoke_class in config.smoke_classes:
        command: list[str | Path] = [
            java,
            f"-D{config.property_name}={native_path}",
            "-cp",
            classpath,
            smoke_class,
        ]
        if (
            config.key == "2025"
            and smoke_class.endswith("NativeSmokeTest")
            and (explicit_fom or default_fom.is_file())
        ):
            command.append(explicit_fom or default_fom)
        run_command(
            command,
            cwd=config.package_root.parents[1],
            description=f"{config.label} Java smoke test {smoke_class}",
        )


def verify_product(
    config: EditionConfig,
    artifact_directory: str | Path,
    *,
    api_jar: str | Path | None = None,
    expected_api_sha256: str | None = None,
    expected_bridge_sha256: str | None = None,
    expected_native_sha256: str | None = None,
    run_smoke: bool = False,
    fom_path: str | Path | None = None,
) -> ProductArtifacts:
    """Verify a staged product, including registrations and optional smoke."""

    artifact_root = Path(artifact_directory).expanduser().resolve()
    if not artifact_root.is_dir():
        raise ProductError(f"Product directory does not exist: {artifact_root}")
    bridge_path = required_file(
        artifact_root / config.bridge_jar_name, f"{config.label} JNI bridge JAR"
    )
    native_path = _artifact_native_library(artifact_root, config)
    api_path = discover_api_jar(artifact_root, config, api_jar)
    _check_expected_hash(api_path, expected_api_sha256, f"{config.label} Java API")
    _check_expected_hash(
        bridge_path, expected_bridge_sha256, f"{config.label} JNI bridge"
    )
    _check_expected_hash(
        native_path, expected_native_sha256, f"{config.label} JNI native library"
    )
    validate_java_api_jar(api_path, config)
    validate_jni_archive(bridge_path, config)
    _validate_product_manifest(
        config, artifact_root, api_path, bridge_path, native_path
    )
    if run_smoke:
        run_java_smoke_tests(
            config,
            api_path,
            bridge_path,
            native_path,
            fom_path=fom_path,
        )
    return ProductArtifacts(artifact_root, api_path, bridge_path, native_path)
