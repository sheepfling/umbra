"""Build the opt-in C++ JNI integration fixture for the JPype test lane."""

from __future__ import annotations

from dataclasses import dataclass
import os
from pathlib import Path
import shutil
import subprocess


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
    artifacts = JniBridgeArtifacts(
        api_jar=(
            Path(configured_api_jar)
            if configured_api_jar
            else output_directory / "mock-java-api" / "umbra-mock-java-rti.jar"
        ),
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
