"""Audit library provenance in an isolated C++ TCK linker response file.

The adapter supplies the provider package and link response. This tool does
not know any provider-specific library name or modify the build.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import shlex
from pathlib import Path


LIBRARY_SUFFIXES = (".a", ".lib", ".so", ".dylib", ".dll")


def _is_library_token(token: str) -> bool:
    name = Path(token).name.lower()
    return name.endswith(LIBRARY_SUFFIXES) or ".so." in name


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as library:
        for chunk in iter(lambda: library.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def inspect_link_response(
    response_file: Path,
    package_prefix: Path,
    *,
    working_directory: Path | None = None,
    require_package_libraries: bool = False,
) -> dict:
    response_file = response_file.resolve()
    package_prefix = package_prefix.resolve()
    working_directory = (working_directory or response_file.parent).resolve()
    # Windows response files may contain backslash paths. POSIX shlex would
    # consume those backslashes as escapes, so use the native quoting mode.
    tokens = shlex.split(
        response_file.read_text(encoding="utf-8"), posix=os.name != "nt"
    )
    libraries: dict[Path, int] = {}
    for token in tokens:
        if len(token) >= 2 and token[0] == token[-1] and token[0] in ("'", '"'):
            token = token[1:-1]
        if token.startswith("-") or not _is_library_token(token):
            continue
        path = Path(token)
        if not path.is_absolute():
            path = working_directory / path
        path = path.resolve()
        libraries[path] = libraries.get(path, 0) + 1
    if not libraries:
        raise ValueError("link response contains no explicit library paths")

    outside = [path for path in libraries if not path.is_relative_to(package_prefix)]
    if require_package_libraries and outside:
        raise ValueError(
            "linked library outside --package-prefix: "
            + ", ".join(str(path) for path in sorted(outside))
        )
    missing = [path for path in libraries if not path.is_file()]
    if missing:
        raise ValueError(
            "linked library was not found: "
            + ", ".join(str(path) for path in sorted(missing))
        )

    return {
        "valid": True,
        "response_file": str(response_file),
        "package_prefix": str(package_prefix),
        "working_directory": str(working_directory),
        "libraries": [
            {
                "path": str(path),
                "sha256": _sha256(path),
                "references": libraries[path],
                "within_package_prefix": path.is_relative_to(package_prefix),
            }
            for path in sorted(libraries)
        ],
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--response-file", type=Path, required=True)
    parser.add_argument("--package-prefix", type=Path, required=True)
    parser.add_argument("--working-directory", type=Path)
    parser.add_argument("--require-package-libraries", action="store_true")
    arguments = parser.parse_args()
    try:
        result = inspect_link_response(
            arguments.response_file,
            arguments.package_prefix,
            working_directory=arguments.working_directory,
            require_package_libraries=arguments.require_package_libraries,
        )
    except (OSError, ValueError) as error:
        print(json.dumps({"valid": False, "finding": str(error)}, indent=2))
        return 1
    print(json.dumps(result, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
