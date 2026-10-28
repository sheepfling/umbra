"""Public IEEE 1516.1-2010 exception namespace."""

from __future__ import annotations

from . import types as _types
from .types import *
from .types import exceptionForName as exceptionForName

_EXCEPTION_TYPES = _types._EXCEPTION_TYPES
_STANDARD_EXCEPTION_NAMES = _types._STANDARD_EXCEPTION_NAMES

# `types.py` owns the canonical string inventory for generated exception names.
__all__ = [*_types.__all__]  # noqa: PLE0604

for _name in __all__:
    _value = globals().get(_name)
    if isinstance(_value, type):
        _value.__module__ = __name__
