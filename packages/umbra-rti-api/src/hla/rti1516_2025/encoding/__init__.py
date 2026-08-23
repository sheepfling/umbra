"""Public IEEE 1516.1-2025 encoding namespace."""

from __future__ import annotations

from .public import *
from .public import __all__
from . import contracts as _contracts


def __getattr__(name: str) -> object:
    """Keep provider-private validation helpers import-compatible."""

    return getattr(_contracts, name)
