"""Build the opt-in C++ JNI integration fixture for the JPype test lane."""

from __future__ import annotations

from dataclasses import dataclass
import os
from pathlib import Path
import shutil
import subprocess
import zipfile


JNI_ROOT = Path(__file__).parents[2] / "umbra-rti-jni"


@dataclass(frozen=True, slots=True)
class JniBridgeArtifacts:
    api_jar: Path
    bridge_jar: Path
    native_library: Path


def jni_toolchain_available() -> bool:
    return all(
        shutil.which(tool) is not None
        for tool in ("cmake", "java", "javac", "jar", "powershell")
    )


def build_jni_bridge(output_directory: Path, *, run_smoke_test: bool = False) -> JniBridgeArtifacts:
    """Build the JNI library/JAR pair in a temporary integration-test directory."""

    cached_directory = os.environ.get("UMBRA_JNI_BRIDGE_ARTIFACT_DIRECTORY")
    if cached_directory:
        return _artifacts(Path(cached_directory))

    command = [
        "powershell",
        "-NoProfile",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        str(JNI_ROOT / "build.ps1"),
        "-OutputDirectory",
        str(output_directory),
    ]
    if run_smoke_test:
        command.append("-RunSmokeTest")
    result = subprocess.run(command, capture_output=True, text=True)
    if result.returncode != 0:
        raise RuntimeError(
            "JNI bridge build failed:\n"
            f"{result.stdout}\n{result.stderr}"
        )
    return _artifacts(output_directory)


def _artifacts(output_directory: Path) -> JniBridgeArtifacts:
    configured_api_jar = os.environ.get("UMBRA_JNI_JAVA_API_JAR")
    if configured_api_jar:
        api_jar = Path(configured_api_jar)
    else:
        # The repository mock fixture is both an API declaration JAR and a
        # vendor fixture, so it carries its own ServiceLoader provider.  That
        # provider must not participate in the JNI integration class path:
        # the bridge should be the sole provider discovered by the standard
        # no-argument RtiFactoryFactory API.
        source_api = output_directory / "mock-java-api" / "umbra-mock-java-rti.jar"
        api_jar = output_directory / "mock-java-api" / "umbra-mock-java-api.jar"
        if not api_jar.is_file():
            with zipfile.ZipFile(source_api) as source, zipfile.ZipFile(
                api_jar, "w", compression=zipfile.ZIP_DEFLATED
            ) as target:
                for entry in source.infolist():
                    if entry.filename.startswith("META-INF/services/"):
                        continue
                    target.writestr(entry, source.read(entry.filename))
    artifacts = JniBridgeArtifacts(
        api_jar=api_jar,
        bridge_jar=output_directory / "umbra-rti-jni.jar",
        native_library=output_directory / "umbra_rti_jni.dll",
    )
    missing = [
        path
        for path in (artifacts.api_jar, artifacts.bridge_jar, artifacts.native_library)
        if not path.is_file()
    ]
    if missing:
        raise RuntimeError(f"JNI bridge build did not produce: {', '.join(map(str, missing))}")
    return artifacts
