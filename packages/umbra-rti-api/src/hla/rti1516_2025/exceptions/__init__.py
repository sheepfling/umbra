"""Public exception namespace for IEEE 1516.1-2025."""

from __future__ import annotations

from . import types as _types
from .types import *
from .types import exceptionForName

# Keep the historical private inventory available to provider adapters and
# tests without putting its implementation in the package initializer.
_EXCEPTION_TYPES = _types._EXCEPTION_TYPES
_STANDARD_EXCEPTION_NAMES = _types._STANDARD_EXCEPTION_NAMES

__all__ = [*_types.__all__, "exceptionForName"]

for _name in __all__:
    _value = globals().get(_name)
    if isinstance(_value, type):
        _value.__module__ = __name__
