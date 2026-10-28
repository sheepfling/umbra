#!/usr/bin/env python3
"""Build a local JAR from the official IEEE 1516.1-2025 Java API sources."""

from __future__ import annotations

import argparse
import hashlib
import io
import os
import subprocess
import sys
import time
import zipfile
from pathlib import Path, PurePosixPath

API_CLASSES = (
    "hla/rti1516_2025/RTIambassador.class",
    "hla/rti1516_2025/RtiFactoryFactory.class",
)


class BuildError(RuntimeError):
    pass


def java_sources(
    downloads_archive: Path,
) -> tuple[str, list[tuple[PurePosixPath, bytes]]]:
    try:
        with zipfile.ZipFile(downloads_archive) as downloads:
            nested = [
                item
                for item in downloads.infolist()
                if not item.is_dir()
                and item.filename.lower().endswith(".zip")
                and "api_xml" in PurePosixPath(item.filename).name.lower()
            ]
            if len(nested) > 1:
                raise BuildError(
                    "The downloads archive contains multiple API XML ZIPs; "
                    "pass the intended API source ZIP directly."
                )
            source_bytes = (
                downloads.read(nested[0]) if nested else downloads_archive.read_bytes()
            )
    except (OSError, zipfile.BadZipFile) as error:
        raise BuildError(f"Cannot read IEEE downloads ZIP: {error}") from error

    try:
        with zipfile.ZipFile(io.BytesIO(source_bytes)) as source_archive:
            found: dict[PurePosixPath, bytes] = {}
            for item in source_archive.infolist():
                if item.is_dir() or not item.filename.lower().endswith(".java"):
                    continue
                archive_path = PurePosixPath(item.filename)
                if archive_path.is_absolute() or ".." in archive_path.parts:
                    continue
                parts = archive_path.parts
                for index in range(len(parts) - 2):
                    if parts[index : index + 3] == ("java", "hla", "rti1516_2025"):
                        relative = PurePosixPath(*parts[index + 1 :])
                        found[relative] = source_archive.read(item)
                        break
    except (OSError, zipfile.BadZipFile) as error:
        raise BuildError(
            f"Cannot read the nested IEEE Java API source ZIP: {error}"
        ) from error

    required_sources = {
        PurePosixPath("hla/rti1516_2025/RTIambassador.java"),
        PurePosixPath("hla/rti1516_2025/RtiFactoryFactory.java"),
    }
    missing = sorted(str(path) for path in required_sources - found.keys())
    if missing:
        raise BuildError(
            "The archive does not contain the expected IEEE 1516.1-2025 Java API sources: "
            + ", ".join(missing)
        )
    if len(found) < 100:
        raise BuildError(
            f"Only {len(found)} API Java sources were found; refusing an incomplete build."
        )

    source_name = nested[0].filename if nested else downloads_archive.name
    return source_name, sorted(found.items(), key=lambda pair: str(pair[0]))


def run(command: list[str], *, env: dict[str, str] | None = None) -> None:
    result = subprocess.run(command, check=False, env=env)
    if result.returncode != 0:
        raise BuildError(
            f"Command failed with exit code {result.returncode}: {command[0]}"
        )


def quoted_argfile_path(path: Path) -> str:
    # javac argument files interpret backslashes as escapes. Forward slashes
    # also work with the Windows JDK and keep spaces in paths as one argument.
    return '"' + str(path.resolve()).replace("\\", "/").replace('"', '\\"') + '"'


def build(
    downloads_archive: Path, output_directory: Path, javac: str, jar: str
) -> Path:
    started = time.perf_counter()
    archive = downloads_archive.expanduser().resolve()
    if not archive.is_file():
        raise BuildError(f"IEEE downloads ZIP not found: {archive}")

    archive_digest = hashlib.sha256(archive.read_bytes()).hexdigest()[:12]
    build_directory = output_directory.expanduser().resolve() / archive_digest
    source_root = build_directory / "src"
    classes_directory = build_directory / "classes"
    temp_directory = build_directory / "temp"
    source_name, sources = java_sources(archive)

    for directory in (source_root, classes_directory, temp_directory):
        directory.mkdir(parents=True, exist_ok=True)

    source_paths: list[Path] = []
    for relative, content in sources:
        target = source_root.joinpath(*relative.parts)
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(content)
        source_paths.append(target)

    api_jar = build_directory / "ieee1516.1-2025-java-api.jar"
    argument_file = build_directory / "javac.args"
    arguments = [
        "--release",
        "11",
        "-encoding",
        "UTF-8",
        "-d",
        quoted_argfile_path(classes_directory),
        *(quoted_argfile_path(path) for path in source_paths),
    ]
    argument_file.write_text("\n".join(arguments) + "\n", encoding="utf-8")
    run([javac, "@" + str(argument_file)])

    environment = os.environ.copy()
    environment["TEMP"] = str(temp_directory)
    environment["TMP"] = str(temp_directory)
    run(
        [jar, "--create", "--file", str(api_jar), "-C", str(classes_directory), "."],
        env=environment,
    )

    try:
        with zipfile.ZipFile(api_jar) as built_jar:
            missing_classes = sorted(set(API_CLASSES) - set(built_jar.namelist()))
    except (OSError, zipfile.BadZipFile) as error:
        raise BuildError(f"The Java API JAR is unreadable: {error}") from error
    if missing_classes:
        raise BuildError(
            "The built JAR is missing required API classes: "
            + ", ".join(missing_classes)
        )

    elapsed = time.perf_counter() - started
    print(f"Source archive: {source_name}")
    print(f"Java API sources compiled: {len(source_paths)} (Java release 11)")
    print(f"JAR checks passed: {', '.join(API_CLASSES)}")
    print(f"JAR: {api_jar}")
    print(f"Elapsed: {elapsed:.2f} seconds")
    return api_jar


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--downloads-zip",
        required=True,
        type=Path,
        help="official IEEE 1516.1-2025 downloads ZIP or its nested API XML source ZIP",
    )
    parser.add_argument(
        "--output-directory",
        required=True,
        type=Path,
        help="local build-output directory, preferably under the ignored out/ folder",
    )
    parser.add_argument("--javac", default="javac", help="JDK javac executable")
    parser.add_argument("--jar", default="jar", help="JDK jar executable")
    arguments = parser.parse_args(argv)
    try:
        build(
            arguments.downloads_zip,
            arguments.output_directory,
            arguments.javac,
            arguments.jar,
        )
    except BuildError as error:
        print(f"Java API build failed: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
