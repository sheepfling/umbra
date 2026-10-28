"""Portable, standard-library C++ quality gate package."""

from .cli import main
from .common import _sha256_file
from .graph import check_graph
from .manifest import create_manifest
from .package_gate import check_package
from .source import check_source
from .test_gate import check_tests
from .verdict import check_verdict

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
