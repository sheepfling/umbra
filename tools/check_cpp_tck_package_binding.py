"""Read-only audit of the installed package selected by a C++ TCK build."""

from __future__ import annotations

import argparse
import json
from pathlib import Path

try:
    from .run_cpp_tck import verify_configured_package
except ImportError:  # Direct invocation: python tools/check_cpp_tck_package_binding.py
    from run_cpp_tck import verify_configured_package


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-directory", type=Path, required=True)
    parser.add_argument("--package-prefix", type=Path, required=True)
    arguments = parser.parse_args()
    inputs = {
        "build_directory": arguments.build_directory.resolve(),
        "package_prefix": arguments.package_prefix.resolve(),
    }
    try:
        configured = verify_configured_package(inputs)
    except ValueError as error:
        print(json.dumps({"valid": False, "finding": str(error)}, indent=2))
        return 1
    print(
        json.dumps(
            {
                "valid": True,
                "package_prefix": str(inputs["package_prefix"]),
                "configured_package": {
                    key: str(value) for key, value in configured.items()
                },
            },
            indent=2,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
