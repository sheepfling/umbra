"""Translate errors raised at the native 2025 extension boundary."""

from __future__ import annotations

from collections.abc import Callable
from typing import TypeVar

from hla.rti1516_2025.encoding import EncoderException
from hla.rti1516_2025.exceptions import exceptionForName

from . import _native

_Result = TypeVar("_Result")


def _call_native(
    function: Callable[..., _Result],
    *args: object,
    encoding_error: type[EncoderException] | None = None,
) -> _Result:
    try:
        return function(*args)
    except _native.NativeRtiError as error:
        name, separator, message = str(error).partition(": ")
        if name == "EncoderException" and encoding_error is not None:
            raise encoding_error(message if separator else name) from error
        # The 2025 API accepts name/message; retain the native cause through
        # Python exception chaining, not the separate 2010 factory signature.
        raise exceptionForName(name, message if separator else name) from error
