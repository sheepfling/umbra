"""Compatibility entry point for the portable C++ quality gate package.

Run this file directly, or import its public gate functions as before.
"""

from __future__ import annotations

if __package__:
    from .cpp_common_quality_core import (
        _sha256_file,
        check_graph,
        check_package,
        check_source,
        check_tests,
        check_verdict,
        create_manifest,
        main,
    )
else:
    from cpp_common_quality_core import (
        _sha256_file,
        check_graph,
        check_package,
        check_source,
        check_tests,
        check_verdict,
        create_manifest,
        main,
    )

__all__ = [
    "_sha256_file",
    "check_graph",
    "check_package",
    "check_source",
    "check_tests",
    "check_verdict",
    "create_manifest",
    "main",
]

if __name__ == "__main__":
    raise SystemExit(main())
