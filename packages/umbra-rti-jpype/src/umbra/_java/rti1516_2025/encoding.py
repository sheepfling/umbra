"""Java-backed implementations of the shared 2025 encoding contract."""

from __future__ import annotations

from typing import Any, Callable, TypeVar

from hla.rti1516_2025.encoding import (
    DecoderException,
    EncoderException,
    EncoderFactory,
    HLAboolean,
    HLAinteger32BE,
    HLAunicodeString,
    HLAunsignedInteger32BE,
    _require_integer32,
    _require_unsigned_integer32,
)
from hla.rti1516_2025.exceptions import RTIexception, RTIinternalError, exceptionForName

from ._runtime import JavaRuntime


_Result = TypeVar("_Result")


def _java_int32(value: int) -> int:
    """Convert a public unsigned 32-bit value to Java's signed ``int`` carrier."""

    value = _require_unsigned_integer32(value)
    return value if value < 2**31 else value - 2**32


def _call_java(
    runtime: JavaRuntime,
    function: Callable[..., _Result],
    *args: object,
    encoding_error: type[EncoderException] | None = None,
) -> _Result:
    try:
        return function(*args)
    except RTIexception:
        raise
    except Exception as error:
        name = runtime.exception_name(error)
        if name in {"EncoderException", "DecoderException"} and encoding_error is not None:
            raise encoding_error(str(error)) from error
        if name is None:
            raise RTIinternalError(f"Java encoding call failed: {error}") from error
        raise exceptionForName(name, str(error)) from error


class _JavaDataElement:
    """Shared forwarding and byte-copy behavior for a Java data element."""

    def __init__(self, implementation: object, runtime: JavaRuntime) -> None:
        self._implementation = implementation
        self._runtime = runtime

    def getOctetBoundary(self) -> int:
        return int(_call_java(self._runtime, getattr(self._implementation, "getOctetBoundary")))

    def getEncodedLength(self) -> int:
        return int(
            _call_java(
                self._runtime,
                getattr(self._implementation, "getEncodedLength"),
                encoding_error=EncoderException,
            )
        )

    def toByteArray(self) -> bytes:
        values = _call_java(
            self._runtime,
            getattr(self._implementation, "toByteArray"),
            encoding_error=EncoderException,
        )
        return bytes(int(value) & 0xFF for value in values)  # type: ignore[union-attr]

    def decode(self, bytes_: bytes):
        _call_java(
            self._runtime,
            getattr(self._implementation, "decode"),
            self._runtime.byte_array(bytes(bytes_)),
            encoding_error=DecoderException,
        )
        return self


class _JavaHLAinteger32BE(_JavaDataElement, HLAinteger32BE):
    def getValue(self) -> int:
        return int(_call_java(self._runtime, getattr(self._implementation, "getValue")))

    def setValue(self, value: int) -> "_JavaHLAinteger32BE":
        _call_java(
            self._runtime,
            getattr(self._implementation, "setValue"),
            _require_integer32(value),
        )
        return self


class _JavaHLAunsignedInteger32BE(_JavaDataElement, HLAunsignedInteger32BE):
    def getValue(self) -> int:
        return int(_call_java(self._runtime, getattr(self._implementation, "getValue"))) & 0xFFFFFFFF

    def setValue(self, value: int) -> "_JavaHLAunsignedInteger32BE":
        _call_java(self._runtime, getattr(self._implementation, "setValue"), _java_int32(value))
        return self


class _JavaHLAboolean(_JavaDataElement, HLAboolean):
    def getValue(self) -> bool:
        return bool(_call_java(self._runtime, getattr(self._implementation, "getValue")))

    def setValue(self, value: bool) -> "_JavaHLAboolean":
        _call_java(self._runtime, getattr(self._implementation, "setValue"), bool(value))
        return self


class _JavaHLAunicodeString(_JavaDataElement, HLAunicodeString):
    def getValue(self) -> str:
        return str(_call_java(self._runtime, getattr(self._implementation, "getValue")))

    def setValue(self, value: str) -> "_JavaHLAunicodeString":
        _call_java(self._runtime, getattr(self._implementation, "setValue"), str(value))
        return self


class JavaEncoderFactory(EncoderFactory):
    """Shared factory façade over a selected Java RTI's ``EncoderFactory``."""

    def __init__(self, implementation: object, runtime: JavaRuntime) -> None:
        self._implementation = implementation
        self._runtime = runtime

    def _create(self, name: str, value: object | None) -> object:
        method = getattr(self._implementation, name)
        if value is None:
            return _call_java(self._runtime, method)
        return _call_java(self._runtime, method, value)

    def createHLAinteger32BE(self, value: int | None = None) -> HLAinteger32BE:
        return _JavaHLAinteger32BE(self._create("createHLAinteger32BE", value), self._runtime)

    def createHLAunsignedInteger32BE(self, value: int | None = None) -> HLAunsignedInteger32BE:
        return _JavaHLAunsignedInteger32BE(
            self._create(
                "createHLAunsignedInteger32BE",
                None if value is None else _java_int32(value),
            ),
            self._runtime,
        )

    def createHLAboolean(self, value: bool | None = None) -> HLAboolean:
        return _JavaHLAboolean(self._create("createHLAboolean", value), self._runtime)

    def createHLAunicodeString(self, value: str | None = None) -> HLAunicodeString:
        return _JavaHLAunicodeString(self._create("createHLAunicodeString", value), self._runtime)
