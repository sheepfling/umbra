"""Public IEEE 1516.1-2010 encoding namespace."""

from __future__ import annotations

from .contracts import *
from .contracts import __all__

for _name in __all__:
    _value = globals().get(_name)
    if isinstance(_value, type):
        _value.__module__ = __package__
