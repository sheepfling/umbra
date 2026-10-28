"""Public IEEE 1516.1-2025 encoding namespace."""

from __future__ import annotations

from . import contracts as _contracts
from .public import *
from .public import __all__ as __all__


def __getattr__(name: str) -> object:
    """Keep provider-private validation helpers import-compatible."""

    return getattr(_contracts, name)
