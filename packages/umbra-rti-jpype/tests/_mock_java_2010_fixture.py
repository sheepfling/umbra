"""Build the small 1516e Java fixture used by opt-in JPype tests."""

from __future__ import annotations

import os
import shutil
import subprocess
from pathlib import Path

REPOSITORY_ROOT = Path(__file__).resolve().parents[3]
FIXTURE_ROOT = (
    REPOSITORY_ROOT
    / "packages"
    / "umbra-rti-java-tck-2010"
    / "test-fixtures"
    / "mock-java-rti-2010"
)
DEFAULT_API_JAR = REPOSITORY_ROOT / "out" / "java-tck-2010" / "ieee1516e-api-staged.jar"
FACTORY_NAME = "Umbra mock Java 1516e"


def java_toolchain_available() -> bool:
    """Return whether the local Java compiler/runtime can build the fixture."""

    return all(shutil.which(tool) is not None for tool in ("java", "javac", "jar"))


def api_jar_path() -> Path:
    """Resolve the API declaration JAR used to compile the fixture."""

    configured = os.environ.get("UMBRA_JAVA_1516E_API_JAR")
    return Path(configured).resolve() if configured else DEFAULT_API_JAR


def build_mock_java_rti_2010(
    output_directory: Path,
    *,
    api_jar: Path | None = None,
) -> tuple[Path, Path]:
    """Compile the fixture and return ``(api_jar, provider_jar)``."""

    api = (api_jar or api_jar_path()).resolve()
    if not api.is_file():
        raise FileNotFoundError(f"2010 Java API JAR does not exist: {api}")
    classes = output_directory / "classes"
    classes.mkdir(parents=True, exist_ok=True)
    source_directory = FIXTURE_ROOT / "src" / "main" / "java"
    sources = sorted(str(source) for source in source_directory.rglob("*.java"))
    if not sources:
        raise RuntimeError(f"No 2010 mock Java sources found below {source_directory}")
    subprocess.run(
        [
            "javac",
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
            *sources,
        ],
        check=True,
        capture_output=True,
        text=True,
    )
    resources = FIXTURE_ROOT / "src" / "main" / "resources"
    shutil.copytree(resources, classes, dirs_exist_ok=True)
    jar_path = output_directory / "umbra-mock-java-rti-1516e.jar"
    subprocess.run(
        ["jar", "--create", "--file", str(jar_path), "-C", str(classes), "."],
        check=True,
        capture_output=True,
        text=True,
    )
    return api, jar_path


__all__ = [
    "FACTORY_NAME",
    "api_jar_path",
    "build_mock_java_rti_2010",
    "java_toolchain_available",
]
