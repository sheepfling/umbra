"""Run smoke checks from a staged Umbra Java/JNI product directory.

This file is copied to a product directory as ``run.py`` and intentionally
uses only Python's standard library so the bundle remains movable.
"""

from __future__ import annotations

import argparse
import os
import shlex
import shutil
import subprocess
import sys
from pathlib import Path


def _tool(name: str) -> Path:
    candidate = shutil.which(name)
    if candidate:
        return Path(candidate)
    java_home = os.environ.get("JAVA_HOME")
    if java_home and name == "java":
        for path in (
            Path(java_home).expanduser() / "bin" / "java",
            Path(java_home).expanduser() / "bin" / "java.exe",
        ):
            if path.is_file():
                return path
    raise RuntimeError(
        f"Required tool {name!r} was not found on PATH or below JAVA_HOME."
    )


def _required_file(value: str | Path, label: str) -> Path:
    path = Path(value).expanduser().resolve()
    if not path.is_file():
        raise RuntimeError(f"{label} does not exist: {path}")
    return path


def _provider(root: Path, edition: str) -> Path:
    name = "umbra-rti-jni.jar" if edition == "2025" else "umbra-rti-jni-2010.jar"
    return _required_file(root / name, f"Umbra {edition} provider JAR")


def _detect_edition(root: Path, requested: str) -> str:
    if requested != "auto":
        _provider(root, requested)
        return requested
    found = [
        edition
        for edition in ("2025", "2010")
        if (
            root
            / ("umbra-rti-jni.jar" if edition == "2025" else "umbra-rti-jni-2010.jar")
        ).is_file()
    ]
    if len(found) != 1:
        names = ", ".join(found) or "none"
        raise RuntimeError(
            f"Could not detect exactly one Umbra provider JAR in {root} (found: {names}); "
            "pass --edition 2025 or --edition 2010."
        )
    return found[0]


def _api(root: Path, provider: Path, explicit: str | Path | None) -> Path:
    if explicit is not None:
        return _required_file(explicit, "Java API JAR")
    candidates = sorted(
        path
        for path in root.glob("*.jar")
        if path.is_file() and path.name != provider.name
    )
    if len(candidates) != 1:
        names = ", ".join(path.name for path in candidates) or "none"
        raise RuntimeError(
            "Pass --java-api-jar when the product directory does not contain "
            f"exactly one API JAR (found: {names})"
        )
    return candidates[0].resolve()


def _native(root: Path, edition: str) -> Path:
    names = (
        (
            "umbra_rti_jni.dll",
            "libumbra_rti_jni.so",
            "libumbra_rti_jni.dylib",
        )
        if edition == "2025"
        else (
            "umbra_rti_jni_2010.dll",
            "libumbra_rti_jni_2010.so",
            "libumbra_rti_jni_2010.dylib",
        )
    )
    for name in names:
        path = root / name
        if path.is_file():
            return path.resolve()
    raise RuntimeError(f"Umbra {edition} native library is missing from {root}")


def _parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Run an Umbra Java/JNI product bundle smoke test."
    )
    parser.add_argument("--edition", choices=("auto", "2025", "2010"), default="auto")
    parser.add_argument(
        "--mode", choices=("surface", "native", "types"), default="surface"
    )
    parser.add_argument("--java-api-jar", type=Path)
    parser.add_argument(
        "--fom-path",
        type=Path,
        help="Optional 2025 FOM path passed to NativeSmokeTest.",
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    arguments = _parser().parse_args(argv)
    root = Path(__file__).resolve().parent
    try:
        edition = _detect_edition(root, arguments.edition)
        if arguments.mode == "types" and edition != "2010":
            raise RuntimeError(
                "--mode types is available only for the IEEE 1516e-2010 bundle"
            )
        if arguments.fom_path is not None and edition != "2025":
            raise RuntimeError(
                "--fom-path is available only for the IEEE 1516.1-2025 bundle"
            )
        provider = _provider(root, edition)
        api = _api(root, provider, arguments.java_api_jar)
        native = _native(root, edition)
        if arguments.fom_path is not None:
            fom = _required_file(arguments.fom_path, "FOM smoke path")
        else:
            fom = None
        if edition == "2025":
            property_name = "umbra.rti.jni.library"
            classes = {
                "surface": "org.umbra.jni.rti1516_2025.StandardSurfaceSmokeTest",
                "native": "org.umbra.jni.rti1516_2025.NativeSmokeTest",
            }
        else:
            property_name = "umbra.rti.jni.2010.library"
            classes = {
                "surface": "org.umbra.jni.rti1516e.SurfaceSmokeTest",
                "native": "org.umbra.jni.rti1516e.NativeSmokeTest",
                "types": "org.umbra.jni.rti1516e.NativeTypeRoundTripTest",
            }
        command: list[str | Path] = [
            _tool("java"),
            f"-D{property_name}={native}",
            "-cp",
            os.pathsep.join((str(api), str(provider))),
            classes[arguments.mode],
        ]
        if edition == "2025" and arguments.mode == "native" and fom is not None:
            command.append(fom)
        display = [str(value) for value in command]
        print(
            "+ "
            + (
                subprocess.list2cmdline(display)
                if os.name == "nt"
                else shlex.join(display)
            ),
            flush=True,
        )
        subprocess.run(display, cwd=root, check=True)
    except FileNotFoundError as error:
        print(f"error: required file or tool was not found: {error}", file=sys.stderr)
        return 2
    except subprocess.CalledProcessError as error:
        print(
            f"error: Java {arguments.mode} smoke test failed with exit code {error.returncode}",
            file=sys.stderr,
        )
        return error.returncode or 1
    except (OSError, RuntimeError, KeyError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
