"""Build the Java fixture in a temporary directory for integration tests."""

from __future__ import annotations

from pathlib import Path
import shutil
import subprocess


FIXTURE_ROOT = Path(__file__).parents[1] / "test-fixtures" / "mock-java-rti"
SMOKE_TEST_CLASS = "org.umbra.testfixture.rti1516_2025.MockSmokeTest"


def java_toolchain_available() -> bool:
    return all(shutil.which(tool) is not None for tool in ("java", "javac", "jar"))


def build_mock_java_rti(output_directory: Path) -> Path:
    """Compile the deliberately small Java fixture and return its JAR path."""

    classes = output_directory / "classes"
    classes.mkdir(parents=True, exist_ok=True)
    source_directory = FIXTURE_ROOT / "src" / "main" / "java"
    sources = sorted(str(source) for source in source_directory.rglob("*.java"))
    if not sources:
        raise RuntimeError(f"No mock Java RTI sources found below {source_directory}")
    subprocess.run(
        ["javac", "-d", str(classes), *sources],
        check=True,
        capture_output=True,
        text=True,
    )
    resources = FIXTURE_ROOT / "src" / "main" / "resources"
    shutil.copytree(resources, classes, dirs_exist_ok=True)
    jar_path = output_directory / "umbra-mock-java-rti.jar"
    subprocess.run(
        ["jar", "--create", "--file", str(jar_path), "-C", str(classes), "."],
        check=True,
        capture_output=True,
        text=True,
    )
    return jar_path
