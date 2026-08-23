"""Java-backed implementations of the shared 2025 encoding contract."""

from __future__ import annotations

import struct
from typing import Any, Callable, TypeVar

from hla.rti1516_2025.byte_types import BytesLike
from hla.rti1516_2025.encoding import (
    DecoderException,
    DataElement,
    DataElementFactory,
    EncoderException,
    EncoderFactory,
    HLAfixedArray,
    HLAfixedRecord,
    HLAvariantRecord,
    HLAboolean,
    HLAASCIIchar,
    HLAASCIIstring,
    HLAbyte,
    HLAfloat32BE,
    HLAfloat32LE,
    HLAfloat64BE,
    HLAfloat64LE,
    HLAinteger16BE,
    HLAinteger16LE,
    HLAinteger32BE,
    HLAinteger32LE,
    HLAinteger64BE,
    HLAinteger64LE,
    HLAunicodeString,
    HLAunsignedInteger16BE,
    HLAunsignedInteger16LE,
    HLAunsignedInteger32BE,
    HLAunsignedInteger32LE,
    HLAunsignedInteger64BE,
    HLAunsignedInteger64LE,
    HLAoctet,
    HLAoctetPairBE,
    HLAoctetPairLE,
    HLAopaqueData,
    HLAvariableArray,
    HLAunicodeChar,
    _require_integer32,
    _require_integer16,
    _require_integer16_le,
    _require_integer32_le,
    _require_integer64,
    _require_integer64_le,
    _require_float64,
    _require_float64_le,
    _require_float32,
    _require_float32_le,
    _require_unsigned_integer32,
    _require_unsigned_integer16,
    _require_unsigned_integer16_le,
    _require_unsigned_integer64,
    _require_byte,
    _require_octet,
    _require_ascii_char,
    _require_ascii_string,
    _require_unicode_char,
    _require_octet_pair,
    _require_opaque_index,
    _require_opaque_data,
    _require_data_element_factory,
    _require_data_element_index,
    _require_data_element_size,
)
from hla.rti1516_2025.exceptions import RTIexception, RTIinternalError, exceptionForName
from hla.rti1516_2025.core import (
    AttributeHandle,
    DimensionHandle,
    FederateHandle,
    HLAfloat64Interval,
    HLAfloat64Time,
    HLAinteger64Interval,
    HLAinteger64Time,
    InteractionClassHandle,
    LogicalTime,
    LogicalTimeInterval,
    MessageRetractionHandle,
    ObjectClassHandle,
    ObjectInstanceHandle,
    ParameterHandle,
    RegionHandle,
    TransportationTypeHandle,
)

from ._runtime import JavaRuntime


_Result = TypeVar("_Result")


def _java_int32(value: int) -> int:
    """Convert a public unsigned 32-bit value to Java's signed ``int`` carrier."""

    value = _require_unsigned_integer32(value)
    return value if value < 2**31 else value - 2**32


def _java_int16(value: int) -> int:
    """Convert a public unsigned 16-bit value to Java's signed ``short`` carrier."""

    value = _require_unsigned_integer16(value)
    return value if value < 2**15 else value - 2**16


def _java_int16_le(value: int) -> int:
    """Convert a public little-endian unsigned 16-bit value to Java's short carrier."""

    value = _require_unsigned_integer16_le(value)
    return value if value < 2**15 else value - 2**16


def _java_int64(value: int) -> int:
    """Convert a public unsigned 64-bit value to Java's signed ``long`` carrier."""

    value = _require_unsigned_integer64(value)
    return value if value < 2**63 else value - 2**64


def _java_byte(value: int, validator: Callable[[int], int]) -> int:
    value = validator(value)
    return value if value < 2**7 else value - 2**8


def _java_octet_pair(value: int) -> int:
    value = _require_octet_pair(value)
    return value if value < 2**15 else value - 2**16


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
        if encoding_error is not None and isinstance(
            error, (struct.error, UnicodeError, IndexError)
        ):
            raise encoding_error(str(error)) from error
        name = runtime.exception_name(error)
        if name is not None:
            # A JPype proxy can expose either a simple or fully-qualified
            # checked-exception name.  Keep the mapping keyed to the standard
            # Java simple class name even when the runtime returns the latter.
            name = name.rsplit(".", 1)[-1].rsplit("$", 1)[-1]
        if name in {
            "EncoderException",
            "DecoderException",
            "IllegalArgumentException",
            "IndexOutOfBoundsException",
            "BufferUnderflowException",
        } and encoding_error is not None:
            raise encoding_error(str(error)) from error
        if name is None:
            raise RTIinternalError(f"Java encoding call failed: {error}") from error
        raise exceptionForName(name, str(error)) from error


def _require_selected_logical_time_value(
    runtime: JavaRuntime, ambassador: object, value: LogicalTime | LogicalTimeInterval
) -> None:
    """Reject a carrier from a different selected logical-time factory.

    The standard ``HLAlogicalTime`` data element is typed by the RTI's
    selected factory.  Its encoded bytes are intentionally opaque, so merely
    decoding a float carrier as an integer (or vice versa) would silently
    corrupt the value.  Keep the C++ type-identity check at the Python edge
    before a raw Java carrier is passed through JNI.
    """

    factory = runtime.logical_time_factory(ambassador)
    expected = str(_call_java(runtime, getattr(factory, "getName")))
    actual = value.implementationName()
    if actual != expected:
        raise EncoderException(
            f"logical-time data element requires {expected}, received {actual}"
        )


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

    def encode(self, byte_wrapper: object | None = None):
        """Use Python bytes or the exact Java ``ByteWrapper`` overload.

        The provider-neutral contract calls ``encode()`` with no arguments
        and returns copied bytes.  A Java-backed caller may additionally pass
        an actual standard ``ByteWrapper``; JPype then invokes Java's cursor
        overload directly and C++ still owns the representation.
        """

        if byte_wrapper is None:
            return self.toByteArray()
        _call_java(
            self._runtime,
            getattr(self._implementation, "encode"),
            byte_wrapper,
            encoding_error=EncoderException,
        )
        return self

    def decode(self, bytes_: object):
        """Decode copied bytes or an exact Java ``ByteWrapper`` cursor."""

        if isinstance(bytes_, (bytes, bytearray, memoryview)):
            bytes_ = self._runtime.byte_array(bytes(bytes_))
        _call_java(
            self._runtime,
            getattr(self._implementation, "decode"),
            bytes_,
            encoding_error=DecoderException,
        )
        return self


class _JavaHandleDataElement(_JavaDataElement, DataElement):
    """Provider-scoped façade for the standard Java handle data elements.

    IEEE 1516.1-2025 puts these creators on ``EncoderFactory`` but the
    provider-neutral Python contract intentionally does not invent separate
    public carrier classes for them.  This façade keeps the exact Java
    creator, while converting only the handle value at the Python boundary.
    The encoded bytes and all validation remain owned by C++ through JNI.
    """

    def __init__(
        self,
        implementation: object,
        runtime: JavaRuntime,
        ambassador: object,
        handle_type: type[object],
        factory_method_name: str,
    ) -> None:
        super().__init__(implementation, runtime)
        self._ambassador = ambassador
        self._handle_type = handle_type
        self._factory_method_name = factory_method_name

    def getValue(self) -> object:
        raw_handle = _call_java(self._runtime, getattr(self._implementation, "getValue"))
        return self._handle_type(self._runtime.handle_bytes(raw_handle))

    def setValue(self, value: object) -> _JavaHandleDataElement:
        if not isinstance(value, self._handle_type):
            raise TypeError(f"value must be {self._handle_type.__name__}")
        raw_handle = self._runtime.decode_handle(
            self._ambassador,
            self._factory_method_name,
            value.encodedValue,
        )
        _call_java(self._runtime, getattr(self._implementation, "setValue"), raw_handle)
        return self


def _java_time_value(value: object) -> object:
    """Read the exact Java time value across the standard/legacy shapes."""

    getter = getattr(value, "getTimeValue", None)
    if getter is not None:
        return getter()
    getter = getattr(value, "getTime", None)
    if getter is not None:
        return getter()
    getter = getattr(value, "getValue", None)
    return getter() if getter is not None else None


def _java_time_implementation_name(value: object, numeric: object) -> str:
    legacy = getattr(value, "implementationName", None)
    if callable(legacy):
        return str(legacy())
    return "HLAfloat64Time" if isinstance(numeric, float) else "HLAinteger64Time"


def _java_logical_time(runtime: JavaRuntime, value: object) -> LogicalTime:
    numeric = _java_time_value(value)
    implementation = _java_time_implementation_name(value, numeric)
    value_type = HLAfloat64Time if implementation == "HLAfloat64Time" else HLAinteger64Time
    return value_type(
        runtime.handle_bytes(value),
        implementation,
        bool(value.isInitial()),
        bool(value.isFinal()),
        numeric,
        str(value.toString()),
    )


def _java_logical_interval(runtime: JavaRuntime, value: object) -> LogicalTimeInterval:
    getter = getattr(value, "getIntervalValue", None)
    if getter is None:
        getter = getattr(value, "getInterval", None)
    if getter is None:
        getter = getattr(value, "getValue", None)
    numeric = getter() if getter is not None else None
    implementation = _java_time_implementation_name(value, numeric)
    value_type = (
        HLAfloat64Interval if implementation == "HLAfloat64Time" else HLAinteger64Interval
    )
    return value_type(
        runtime.handle_bytes(value),
        implementation,
        bool(value.isZero()),
        bool(value.isEpsilon()),
        numeric,
        str(value.toString()),
    )


class _JavaLogicalTimeDataElement(_JavaDataElement, DataElement):
    """Provider-scoped façade for the standard Java logical-time carrier."""

    def __init__(self, implementation: object, runtime: JavaRuntime, ambassador: object) -> None:
        super().__init__(implementation, runtime)
        self._ambassador = ambassador

    def getValue(self) -> LogicalTime:
        return _java_logical_time(
            self._runtime,
            _call_java(self._runtime, getattr(self._implementation, "getValue")),
        )

    def setValue(self, value: LogicalTime) -> _JavaLogicalTimeDataElement:
        if not isinstance(value, LogicalTime):
            raise TypeError("value must be LogicalTime")
        _require_selected_logical_time_value(self._runtime, self._ambassador, value)
        raw_value = self._runtime.decode_logical_time(self._ambassador, value.encodedValue)
        _call_java(self._runtime, getattr(self._implementation, "setValue"), raw_value)
        return self


class _JavaLogicalTimeIntervalDataElement(_JavaDataElement, DataElement):
    """Provider-scoped façade for the standard Java interval carrier."""

    def __init__(self, implementation: object, runtime: JavaRuntime, ambassador: object) -> None:
        super().__init__(implementation, runtime)
        self._ambassador = ambassador

    def getValue(self) -> LogicalTimeInterval:
        return _java_logical_interval(
            self._runtime,
            _call_java(self._runtime, getattr(self._implementation, "getValue")),
        )

    def setValue(self, value: LogicalTimeInterval) -> _JavaLogicalTimeIntervalDataElement:
        if not isinstance(value, LogicalTimeInterval):
            raise TypeError("value must be LogicalTimeInterval")
        _require_selected_logical_time_value(self._runtime, self._ambassador, value)
        raw_value = self._runtime.decode_logical_interval(self._ambassador, value.encodedValue)
        _call_java(self._runtime, getattr(self._implementation, "setValue"), raw_value)
        return self


class _JavaHLAinteger32BE(_JavaDataElement, HLAinteger32BE):
    def getValue(self) -> int:
        return int(_call_java(self._runtime, getattr(self._implementation, "getValue")))

    def setValue(self, value: int) -> _JavaHLAinteger32BE:
        _call_java(
            self._runtime,
            getattr(self._implementation, "setValue"),
            _require_integer32(value),
        )
        return self


class _JavaHLAinteger16BE(_JavaDataElement, HLAinteger16BE):
    def getValue(self) -> int:
        return int(_call_java(self._runtime, getattr(self._implementation, "getValue")))

    def setValue(self, value: int) -> _JavaHLAinteger16BE:
        _call_java(
            self._runtime,
            getattr(self._implementation, "setValue"),
            _require_integer16(value),
        )
        return self


class _JavaHLAinteger16LE(_JavaDataElement, HLAinteger16LE):
    def getValue(self) -> int:
        return int(_call_java(self._runtime, getattr(self._implementation, "getValue")))

    def setValue(self, value: int) -> _JavaHLAinteger16LE:
        _call_java(
            self._runtime,
            getattr(self._implementation, "setValue"),
            _require_integer16_le(value),
        )
        return self


class _JavaHLAinteger32LE(_JavaDataElement, HLAinteger32LE):
    def getValue(self) -> int:
        return int(_call_java(self._runtime, getattr(self._implementation, "getValue")))

    def setValue(self, value: int) -> _JavaHLAinteger32LE:
        _call_java(
            self._runtime,
            getattr(self._implementation, "setValue"),
            _require_integer32_le(value),
        )
        return self


class _JavaHLAinteger64BE(_JavaDataElement, HLAinteger64BE):
    def getValue(self) -> int:
        return int(_call_java(self._runtime, getattr(self._implementation, "getValue")))

    def setValue(self, value: int) -> _JavaHLAinteger64BE:
        _call_java(
            self._runtime,
            getattr(self._implementation, "setValue"),
            _require_integer64(value),
        )
        return self


class _JavaHLAinteger64LE(_JavaDataElement, HLAinteger64LE):
    def getValue(self) -> int:
        return int(_call_java(self._runtime, getattr(self._implementation, "getValue")))

    def setValue(self, value: int) -> _JavaHLAinteger64LE:
        _call_java(
            self._runtime,
            getattr(self._implementation, "setValue"),
            _require_integer64_le(value),
        )
        return self


class _JavaHLAfloat64BE(_JavaDataElement, HLAfloat64BE):
    def getValue(self) -> float:
        return float(_call_java(self._runtime, getattr(self._implementation, "getValue")))

    def setValue(self, value: float | int) -> _JavaHLAfloat64BE:
        _call_java(
            self._runtime,
            getattr(self._implementation, "setValue"),
            _require_float64(value),
        )
        return self


class _JavaHLAfloat32BE(_JavaDataElement, HLAfloat32BE):
    def getValue(self) -> float:
        return float(_call_java(self._runtime, getattr(self._implementation, "getValue")))

    def setValue(self, value: float | int) -> _JavaHLAfloat32BE:
        _call_java(
            self._runtime,
            getattr(self._implementation, "setValue"),
            _require_float32(value),
        )
        return self


class _JavaHLAfloat64LE(_JavaDataElement, HLAfloat64LE):
    def getValue(self) -> float:
        return float(_call_java(self._runtime, getattr(self._implementation, "getValue")))

    def setValue(self, value: float | int) -> _JavaHLAfloat64LE:
        _call_java(
            self._runtime,
            getattr(self._implementation, "setValue"),
            _require_float64_le(value),
        )
        return self


class _JavaHLAfloat32LE(_JavaDataElement, HLAfloat32LE):
    def getValue(self) -> float:
        return float(_call_java(self._runtime, getattr(self._implementation, "getValue")))

    def setValue(self, value: float | int) -> _JavaHLAfloat32LE:
        _call_java(
            self._runtime,
            getattr(self._implementation, "setValue"),
            _require_float32_le(value),
        )
        return self


class _JavaHLAunsignedInteger16BE(_JavaDataElement, HLAunsignedInteger16BE):
    def getValue(self) -> int:
        return int(_call_java(self._runtime, getattr(self._implementation, "getValue"))) & 0xFFFF

    def setValue(self, value: int) -> _JavaHLAunsignedInteger16BE:
        _call_java(
            self._runtime,
            getattr(self._implementation, "setValue"),
            _java_int16(value),
        )
        return self


class _JavaHLAunsignedInteger16LE(_JavaDataElement, HLAunsignedInteger16LE):
    def getValue(self) -> int:
        return int(_call_java(self._runtime, getattr(self._implementation, "getValue"))) & 0xFFFF

    def setValue(self, value: int) -> _JavaHLAunsignedInteger16LE:
        _call_java(
            self._runtime,
            getattr(self._implementation, "setValue"),
            _java_int16_le(value),
        )
        return self


class _JavaHLAunsignedInteger32LE(_JavaDataElement, HLAunsignedInteger32LE):
    def getValue(self) -> int:
        return int(_call_java(self._runtime, getattr(self._implementation, "getValue"))) & 0xFFFFFFFF

    def setValue(self, value: int) -> _JavaHLAunsignedInteger32LE:
        _call_java(self._runtime, getattr(self._implementation, "setValue"), _java_int32(value))
        return self


class _JavaHLAunsignedInteger32BE(_JavaDataElement, HLAunsignedInteger32BE):
    def getValue(self) -> int:
        return int(_call_java(self._runtime, getattr(self._implementation, "getValue"))) & 0xFFFFFFFF

    def setValue(self, value: int) -> _JavaHLAunsignedInteger32BE:
        _call_java(self._runtime, getattr(self._implementation, "setValue"), _java_int32(value))
        return self


class _JavaHLAunsignedInteger64BE(_JavaDataElement, HLAunsignedInteger64BE):
    def getValue(self) -> int:
        return int(_call_java(self._runtime, getattr(self._implementation, "getValue"))) & 0xFFFFFFFFFFFFFFFF

    def setValue(self, value: int) -> _JavaHLAunsignedInteger64BE:
        _call_java(
            self._runtime,
            getattr(self._implementation, "setValue"),
            _java_int64(value),
        )
        return self


class _JavaHLAunsignedInteger64LE(_JavaDataElement, HLAunsignedInteger64LE):
    def getValue(self) -> int:
        return int(_call_java(self._runtime, getattr(self._implementation, "getValue"))) & 0xFFFFFFFFFFFFFFFF

    def setValue(self, value: int) -> _JavaHLAunsignedInteger64LE:
        _call_java(self._runtime, getattr(self._implementation, "setValue"), _java_int64(value))
        return self


class _JavaHLAbyte(_JavaDataElement, HLAbyte):
    def getValue(self) -> int:
        return int(_call_java(self._runtime, getattr(self._implementation, "getValue"))) & 0xFF

    def setValue(self, value: int) -> _JavaHLAbyte:
        _call_java(
            self._runtime,
            getattr(self._implementation, "setValue"),
            _java_byte(value, _require_byte),
        )
        return self


class _JavaHLAoctet(_JavaDataElement, HLAoctet):
    def getValue(self) -> int:
        return int(_call_java(self._runtime, getattr(self._implementation, "getValue"))) & 0xFF

    def setValue(self, value: int) -> _JavaHLAoctet:
        _call_java(
            self._runtime,
            getattr(self._implementation, "setValue"),
            _java_byte(value, _require_octet),
        )
        return self


class _JavaHLAASCIIchar(_JavaDataElement, HLAASCIIchar):
    def getValue(self) -> int:
        return int(_call_java(self._runtime, getattr(self._implementation, "getValue"))) & 0xFF

    def setValue(self, value: int) -> _JavaHLAASCIIchar:
        _call_java(
            self._runtime,
            getattr(self._implementation, "setValue"),
            _java_byte(value, _require_ascii_char),
        )
        return self


class _JavaHLAASCIIstring(_JavaDataElement, HLAASCIIstring):
    def getValue(self) -> str:
        return str(_call_java(self._runtime, getattr(self._implementation, "getValue")))

    def setValue(self, value: str) -> _JavaHLAASCIIstring:
        _call_java(
            self._runtime,
            getattr(self._implementation, "setValue"),
            _require_ascii_string(value),
        )
        return self


class _JavaHLAunicodeChar(_JavaDataElement, HLAunicodeChar):
    def getValue(self) -> int:
        return int(_call_java(self._runtime, getattr(self._implementation, "getValue"))) & 0xFFFF

    def setValue(self, value: int) -> _JavaHLAunicodeChar:
        _call_java(
            self._runtime,
            getattr(self._implementation, "setValue"),
            _java_int16(value),
        )
        return self


class _JavaHLAoctetPairBE(_JavaDataElement, HLAoctetPairBE):
    def getValue(self) -> int:
        return int(_call_java(self._runtime, getattr(self._implementation, "getValue"))) & 0xFFFF

    def setValue(self, value: int) -> _JavaHLAoctetPairBE:
        _call_java(
            self._runtime,
            getattr(self._implementation, "setValue"),
            _java_octet_pair(value),
        )
        return self


class _JavaHLAoctetPairLE(_JavaDataElement, HLAoctetPairLE):
    def getValue(self) -> int:
        return int(_call_java(self._runtime, getattr(self._implementation, "getValue"))) & 0xFFFF

    def setValue(self, value: int) -> _JavaHLAoctetPairLE:
        _call_java(
            self._runtime,
            getattr(self._implementation, "setValue"),
            _java_octet_pair(value),
        )
        return self


class _JavaHLAopaqueData(_JavaDataElement, HLAopaqueData):
    def size(self) -> int:
        return int(_call_java(self._runtime, getattr(self._implementation, "size")))

    def get(self, index: int) -> int:
        return int(
            _call_java(
                self._runtime,
                getattr(self._implementation, "get"),
                _require_opaque_index(index, self.size()),
            )
        ) & 0xFF

    def getValue(self) -> bytes:
        values = _call_java(self._runtime, getattr(self._implementation, "getValue"))
        return bytes(int(value) & 0xFF for value in values)  # type: ignore[union-attr]

    def setValue(self, value: BytesLike) -> None:
        _call_java(
            self._runtime,
            getattr(self._implementation, "setValue"),
            self._runtime.byte_array(_require_opaque_data(value)),
        )


class _JavaHLAvariableArray(_JavaDataElement, HLAvariableArray):
    def __init__(
        self,
        implementation: object,
        runtime: JavaRuntime,
        factory: DataElementFactory,
        prototype: DataElement,
        factory_proxy: object,
    ) -> None:
        super().__init__(implementation, runtime)
        self._factory = factory
        self._prototype = prototype
        self._factory_proxy = factory_proxy

    def addElement(self, dataElement: DataElement) -> None:
        if not isinstance(dataElement, _JavaDataElement):
            raise TypeError("Java variable arrays require Java DataElements")
        _call_java(
            self._runtime,
            getattr(self._implementation, "addElement"),
            getattr(dataElement, "_implementation"),
            encoding_error=EncoderException,
        )

    def resize(self, size: int) -> _JavaHLAvariableArray:
        """Expose Java 2025's provider-scoped ``resize`` operation."""

        _call_java(
            self._runtime,
            getattr(self._implementation, "resize"),
            _require_data_element_size(size),
            encoding_error=EncoderException,
        )
        return self

    def size(self) -> int:
        return int(_call_java(self._runtime, getattr(self._implementation, "size")))

    def get(self, index: int) -> DataElement:
        index = _require_data_element_index(index, self.size())
        element = self._factory.createElement(index)
        if not isinstance(element, type(self._prototype)):
            raise TypeError("factory returned an element that does not match the prototype")
        implementation = _call_java(
            self._runtime,
            getattr(self._implementation, "get"),
            index,
        )
        if not isinstance(element, _JavaDataElement):
            raise TypeError("Java variable arrays require Java DataElement factories")
        element._implementation = implementation
        return element


class _JavaHLAfixedArray(_JavaDataElement, HLAfixedArray):
    def __init__(
        self,
        implementation: object,
        runtime: JavaRuntime,
        factory: DataElementFactory | None,
        prototype: DataElement | None,
        factory_proxy: object | None,
        elements: tuple[DataElement, ...] | None = None,
    ) -> None:
        super().__init__(implementation, runtime)
        self._factory = factory
        self._prototype = prototype
        self._factory_proxy = factory_proxy
        self._elements = list(elements) if elements is not None else None

    def size(self) -> int:
        return int(_call_java(self._runtime, getattr(self._implementation, "size")))

    def get(self, index: int) -> DataElement:
        index = _require_data_element_index(index, self.size())
        if self._elements is not None:
            element = self._elements[index]
            implementation = _call_java(
                self._runtime,
                getattr(self._implementation, "get"),
                index,
                encoding_error=EncoderException,
            )
            if not isinstance(element, _JavaDataElement):
                raise TypeError("Java fixed arrays require Java DataElements")
            element._implementation = implementation
            return element
        if self._factory is None or self._prototype is None:
            raise EncoderException("Java fixed array element factory is unavailable")
        element = self._factory.createElement(index)
        if not isinstance(element, type(self._prototype)):
            raise TypeError("factory returned an element that does not match the prototype")
        implementation = _call_java(
            self._runtime,
            getattr(self._implementation, "get"),
            index,
        )
        if not isinstance(element, _JavaDataElement):
            raise TypeError("Java fixed arrays require Java DataElement factories")
        element._implementation = implementation
        return element

    def set(self, index: int, dataElement: DataElement) -> None:
        index = _require_data_element_index(index, self.size())
        if not isinstance(dataElement, _JavaDataElement):
            raise TypeError("Java fixed arrays require Java DataElements")
        native_set = getattr(self._implementation, "set", None)
        if native_set is not None:
            _call_java(
                self._runtime,
                native_set,
                index,
                getattr(dataElement, "_implementation"),
                encoding_error=EncoderException,
            )
            if self._elements is not None:
                self._elements[index] = dataElement
            return
        self.get(index).decode(dataElement.toByteArray())


class _JavaHLAfixedRecord(_JavaDataElement, HLAfixedRecord):
    def __init__(
        self,
        implementation: object,
        runtime: JavaRuntime,
    ) -> None:
        super().__init__(implementation, runtime)
        self._elements: list[DataElement] = []

    def appendElement(self, dataElement: DataElement) -> None:
        if not isinstance(dataElement, _JavaDataElement):
            raise TypeError("Java fixed records require Java DataElements")
        _call_java(
            self._runtime,
            getattr(self._implementation, "add"),
            getattr(dataElement, "_implementation"),
            encoding_error=EncoderException,
        )
        self._elements.append(dataElement)

    def size(self) -> int:
        return int(_call_java(self._runtime, getattr(self._implementation, "size")))

    def get(self, index: int) -> DataElement:
        index = _require_data_element_index(index, self.size())
        if index >= len(self._elements):
            raise EncoderException("Java fixed record component state is unavailable")
        element = self._elements[index]
        implementation = _call_java(
            self._runtime,
            getattr(self._implementation, "get"),
            index,
            encoding_error=EncoderException,
        )
        if not isinstance(element, _JavaDataElement):
            raise TypeError("Java fixed records require Java DataElement components")
        element._implementation = implementation
        return element

    def set(self, index: int, dataElement: DataElement) -> None:
        index = _require_data_element_index(index, self.size())
        if index >= len(self._elements):
            raise EncoderException("Java fixed record component state is unavailable")
        if not isinstance(dataElement, _JavaDataElement):
            raise TypeError("Java fixed records require Java DataElement components")
        native_set = getattr(self._implementation, "set", None)
        if native_set is not None:
            if not isinstance(dataElement, _JavaDataElement):
                raise TypeError("Java fixed records require Java DataElement components")
            _call_java(
                self._runtime,
                native_set,
                index,
                getattr(dataElement, "_implementation"),
                encoding_error=EncoderException,
            )
            self._elements[index] = dataElement
            return
        self.get(index).decode(dataElement.toByteArray())


class _JavaHLAvariantRecord(_JavaDataElement, HLAvariantRecord):
    def __init__(
        self,
        implementation: object,
        runtime: JavaRuntime,
        discriminant: DataElement,
    ) -> None:
        super().__init__(implementation, runtime)
        self._discriminant = discriminant
        self._variants: dict[bytes, DataElement] = {}

    def _require_discriminant(self, discriminant: DataElement) -> bytes:
        if not isinstance(discriminant, _JavaDataElement):
            raise TypeError("Java variant records require Java discriminants")
        return discriminant.toByteArray()

    def setVariant(self, discriminant: DataElement, dataElement: DataElement) -> None:
        key = self._require_discriminant(discriminant)
        if not isinstance(dataElement, _JavaDataElement):
            raise TypeError("Java variant records require Java DataElements")
        _call_java(
            self._runtime,
            getattr(self._implementation, "setVariant"),
            getattr(discriminant, "_implementation"),
            getattr(dataElement, "_implementation"),
            encoding_error=EncoderException,
        )
        self._variants[key] = dataElement

    def setDiscriminant(self, discriminant: DataElement) -> None:
        self._require_discriminant(discriminant)
        _call_java(
            self._runtime,
            getattr(self._implementation, "setDiscriminant"),
            getattr(discriminant, "_implementation"),
            encoding_error=EncoderException,
        )

    def getDiscriminant(self) -> DataElement:
        implementation = _call_java(
            self._runtime,
            getattr(self._implementation, "getDiscriminant"),
            encoding_error=EncoderException,
        )
        self._discriminant._implementation = implementation  # type: ignore[attr-defined]
        return self._discriminant

    def getValue(self) -> DataElement | None:
        implementation = _call_java(
            self._runtime,
            getattr(self._implementation, "getValue"),
            encoding_error=EncoderException,
        )
        if implementation is None:
            return None
        key = bytes(self.getDiscriminant().toByteArray())
        element = self._variants.get(key)
        if element is None or not isinstance(element, _JavaDataElement):
            raise EncoderException("Java variant record returned an unmapped alternative")
        element._implementation = implementation
        return element


class _JavaHLAextendableVariantRecord(_JavaHLAvariantRecord):
    """Exact Java extendable-variant carrier backed by the C++ JNI element."""


class _JavaHLAboolean(_JavaDataElement, HLAboolean):
    def getValue(self) -> bool:
        return bool(_call_java(self._runtime, getattr(self._implementation, "getValue")))

    def setValue(self, value: bool) -> _JavaHLAboolean:
        _call_java(self._runtime, getattr(self._implementation, "setValue"), bool(value))
        return self


class _JavaHLAunicodeString(_JavaDataElement, HLAunicodeString):
    def getValue(self) -> str:
        return str(_call_java(self._runtime, getattr(self._implementation, "getValue")))

    def setValue(self, value: str) -> _JavaHLAunicodeString:
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

    def createHLAinteger16BE(self, value: int | None = None) -> HLAinteger16BE:
        return _JavaHLAinteger16BE(
            self._create("createHLAinteger16BE", None if value is None else _require_integer16(value)),
            self._runtime,
        )

    def createHLAinteger16LE(self, value: int | None = None) -> HLAinteger16LE:
        return _JavaHLAinteger16LE(
            self._create(
                "createHLAinteger16LE",
                None if value is None else _require_integer16_le(value),
            ),
            self._runtime,
        )

    def createHLAinteger32LE(self, value: int | None = None) -> HLAinteger32LE:
        return _JavaHLAinteger32LE(
            self._create(
                "createHLAinteger32LE",
                None if value is None else _require_integer32_le(value),
            ),
            self._runtime,
        )

    def createHLAinteger64BE(self, value: int | None = None) -> HLAinteger64BE:
        return _JavaHLAinteger64BE(
            self._create(
                "createHLAinteger64BE",
                None if value is None else _require_integer64(value),
            ),
            self._runtime,
        )

    def createHLAinteger64LE(self, value: int | None = None) -> HLAinteger64LE:
        return _JavaHLAinteger64LE(
            self._create(
                "createHLAinteger64LE",
                None if value is None else _require_integer64_le(value),
            ),
            self._runtime,
        )

    def createHLAfloat64BE(self, value: float | int | None = None) -> HLAfloat64BE:
        return _JavaHLAfloat64BE(
            self._create("createHLAfloat64BE", None if value is None else _require_float64(value)),
            self._runtime,
        )

    def createHLAfloat32BE(self, value: float | int | None = None) -> HLAfloat32BE:
        return _JavaHLAfloat32BE(
            self._create(
                "createHLAfloat32BE",
                None if value is None else _require_float32(value),
            ),
            self._runtime,
        )

    def createHLAfloat64LE(self, value: float | int | None = None) -> HLAfloat64LE:
        return _JavaHLAfloat64LE(
            self._create(
                "createHLAfloat64LE",
                None if value is None else _require_float64_le(value),
            ),
            self._runtime,
        )

    def createHLAfloat32LE(self, value: float | int | None = None) -> HLAfloat32LE:
        return _JavaHLAfloat32LE(
            self._create(
                "createHLAfloat32LE",
                None if value is None else _require_float32_le(value),
            ),
            self._runtime,
        )

    def createHLAunsignedInteger16BE(
        self, value: int | None = None
    ) -> HLAunsignedInteger16BE:
        return _JavaHLAunsignedInteger16BE(
            self._create(
                "createHLAunsignedInteger16BE",
                None if value is None else _java_int16(value),
            ),
            self._runtime,
        )

    def createHLAunsignedInteger16LE(
        self, value: int | None = None
    ) -> HLAunsignedInteger16LE:
        return _JavaHLAunsignedInteger16LE(
            self._create(
                "createHLAunsignedInteger16LE",
                None if value is None else _java_int16_le(value),
            ),
            self._runtime,
        )

    def createHLAunsignedInteger32LE(
        self, value: int | None = None
    ) -> HLAunsignedInteger32LE:
        return _JavaHLAunsignedInteger32LE(
            self._create(
                "createHLAunsignedInteger32LE",
                None if value is None else _java_int32(value),
            ),
            self._runtime,
        )

    def createHLAunsignedInteger32BE(self, value: int | None = None) -> HLAunsignedInteger32BE:
        return _JavaHLAunsignedInteger32BE(
            self._create(
                "createHLAunsignedInteger32BE",
                None if value is None else _java_int32(value),
            ),
            self._runtime,
        )

    def createHLAunsignedInteger64BE(
        self, value: int | None = None
    ) -> HLAunsignedInteger64BE:
        return _JavaHLAunsignedInteger64BE(
            self._create(
                "createHLAunsignedInteger64BE",
                None if value is None else _java_int64(value),
            ),
            self._runtime,
        )

    def createHLAunsignedInteger64LE(
        self, value: int | None = None
    ) -> HLAunsignedInteger64LE:
        return _JavaHLAunsignedInteger64LE(
            self._create(
                "createHLAunsignedInteger64LE",
                None if value is None else _java_int64(value),
            ),
            self._runtime,
        )

    def createHLAbyte(self, value: int | None = None) -> HLAbyte:
        return _JavaHLAbyte(
            self._create(
                "createHLAbyte",
                None if value is None else _java_byte(value, _require_byte),
            ),
            self._runtime,
        )

    def createHLAoctet(self, value: int | None = None) -> HLAoctet:
        return _JavaHLAoctet(
            self._create(
                "createHLAoctet",
                None if value is None else _java_byte(value, _require_octet),
            ),
            self._runtime,
        )

    def createHLAASCIIchar(self, value: int | None = None) -> HLAASCIIchar:
        return _JavaHLAASCIIchar(
            self._create(
                "createHLAASCIIchar",
                None if value is None else _java_byte(value, _require_ascii_char),
            ),
            self._runtime,
        )

    def createHLAASCIIstring(self, value: str | None = None) -> HLAASCIIstring:
        return _JavaHLAASCIIstring(
            self._create(
                "createHLAASCIIstring",
                None if value is None else _require_ascii_string(value),
            ),
            self._runtime,
        )

    def createHLAunicodeChar(self, value: int | None = None) -> HLAunicodeChar:
        return _JavaHLAunicodeChar(
            self._create(
                "createHLAunicodeChar",
                None if value is None else _java_int16(value),
            ),
            self._runtime,
        )

    def createHLAoctetPairBE(self, value: int | None = None) -> HLAoctetPairBE:
        return _JavaHLAoctetPairBE(
            self._create(
                "createHLAoctetPairBE",
                None if value is None else _java_octet_pair(value),
            ),
            self._runtime,
        )

    def createHLAoctetPairLE(self, value: int | None = None) -> HLAoctetPairLE:
        return _JavaHLAoctetPairLE(
            self._create(
                "createHLAoctetPairLE",
                None if value is None else _java_octet_pair(value),
            ),
            self._runtime,
        )

    def createHLAopaqueData(
        self, value: BytesLike | None = None
    ) -> HLAopaqueData:
        return _JavaHLAopaqueData(
            self._create(
                "createHLAopaqueData",
                None if value is None else _require_opaque_data(value),
            ),
            self._runtime,
        )

    def createHLAvariableArray(
        self, factory: DataElementFactory, *elements: DataElement
    ) -> HLAvariableArray:
        factory = _require_data_element_factory(factory)
        prototype = factory.createElement(0)
        if not isinstance(prototype, _JavaDataElement):
            raise TypeError("Java variable arrays require a Java DataElement factory")
        for element in elements:
            if not isinstance(element, _JavaDataElement):
                raise TypeError("Java variable arrays require Java DataElements")
        factory_proxy = self._runtime.data_element_factory(factory)
        implementation = _call_java(
            self._runtime,
            getattr(self._implementation, "createHLAvariableArray"),
            factory_proxy,
            self._runtime.data_element_array(
                tuple(getattr(element, "_implementation") for element in elements)
            ),
        )
        return _JavaHLAvariableArray(
            implementation,
            self._runtime,
            factory,
            prototype,
            factory_proxy,
        )

    def createHLAfixedArray(
        self,
        factory_or_element: DataElementFactory | DataElement,
        size_or_element: int | DataElement | None = None,
        *elements: DataElement,
    ) -> HLAfixedArray:
        """Create either standard fixed-array overload.

        The provider-neutral contract uses ``(DataElementFactory, size)``.
        Java's exact 2025 interface additionally declares a ``DataElement...``
        varargs overload; Java-backed callers may use that form directly while
        all component implementations remain the selected Java/JNI objects.
        """

        if isinstance(factory_or_element, DataElementFactory):
            if not isinstance(size_or_element, int) or isinstance(size_or_element, bool):
                raise TypeError("Java fixed arrays require a factory and integer size")
            if elements:
                raise TypeError("Java fixed-array factory overload does not accept elements")
            factory = _require_data_element_factory(factory_or_element)
            size = _require_data_element_size(size_or_element)
            prototype = factory.createElement(0)
            if not isinstance(prototype, _JavaDataElement):
                raise TypeError("Java fixed arrays require a Java DataElement factory")
            factory_proxy = self._runtime.data_element_factory(factory)
            implementation = _call_java(
                self._runtime,
                getattr(self._implementation, "createHLAfixedArray"),
                factory_proxy,
                size,
            )
            return _JavaHLAfixedArray(
                implementation,
                self._runtime,
                factory,
                prototype,
                factory_proxy,
            )

        values: tuple[DataElement, ...] = (
            (factory_or_element,)
            + (() if size_or_element is None else (size_or_element,))
            + elements
        )
        if not values or any(not isinstance(element, _JavaDataElement) for element in values):
            raise TypeError("Java fixed-array varargs require Java DataElements")
        implementation = _call_java(
            self._runtime,
            getattr(self._implementation, "createHLAfixedArray"),
            self._runtime.data_element_array(
                tuple(getattr(element, "_implementation") for element in values)
            ),
        )
        return _JavaHLAfixedArray(
            implementation,
            self._runtime,
            None,
            values[0],
            None,
            values,
        )

    def createHLAfixedRecord(self) -> HLAfixedRecord:
        return _JavaHLAfixedRecord(
            _call_java(
                self._runtime,
                getattr(self._implementation, "createHLAfixedRecord"),
            ),
            self._runtime,
        )

    def createHLAvariantRecord(self, discriminantPrototype: DataElement) -> HLAvariantRecord:
        if not isinstance(discriminantPrototype, _JavaDataElement):
            raise TypeError("Java variant records require a Java discriminant prototype")
        return _JavaHLAvariantRecord(
            _call_java(
                self._runtime,
                getattr(self._implementation, "createHLAvariantRecord"),
                getattr(discriminantPrototype, "_implementation"),
            ),
            self._runtime,
            discriminantPrototype,
        )

    def createHLAextendableVariantRecord(
        self, discriminantPrototype: DataElement
    ) -> DataElement:
        if not isinstance(discriminantPrototype, _JavaDataElement):
            raise TypeError(
                "Java extendable variant records require a Java discriminant prototype"
            )
        return _JavaHLAextendableVariantRecord(
            _call_java(
                self._runtime,
                getattr(self._implementation, "createHLAextendableVariantRecord"),
                getattr(discriminantPrototype, "_implementation"),
            ),
            self._runtime,
            discriminantPrototype,
        )

    def createHLAboolean(self, value: bool | None = None) -> HLAboolean:
        return _JavaHLAboolean(self._create("createHLAboolean", value), self._runtime)

    def createHLAunicodeString(self, value: str | None = None) -> HLAunicodeString:
        return _JavaHLAunicodeString(self._create("createHLAunicodeString", value), self._runtime)

    @staticmethod
    def _raw_ambassador(ambassador: object) -> object:
        unwrap = getattr(ambassador, "unwrap_java_object", None)
        return unwrap() if callable(unwrap) else ambassador

    def _create_handle_element(
        self,
        method_name: str,
        ambassador: object,
        value: object | None,
        handle_type: type[object],
        factory_method_name: str,
    ) -> _JavaHandleDataElement:
        raw_ambassador = self._raw_ambassador(ambassador)
        arguments: list[object] = [raw_ambassador]
        if value is not None:
            if not isinstance(value, handle_type):
                raise TypeError(f"value must be {handle_type.__name__}")
            arguments.append(
                self._runtime.decode_handle(
                    raw_ambassador,
                    factory_method_name,
                    value.encodedValue,
                )
            )
        implementation = _call_java(
            self._runtime,
            getattr(self._implementation, method_name),
            *arguments,
        )
        return _JavaHandleDataElement(
            implementation,
            self._runtime,
            raw_ambassador,
            handle_type,
            factory_method_name,
        )

    def createHLAfederateHandle(
        self, ambassador: object, value: FederateHandle | None = None
    ) -> DataElement:
        return self._create_handle_element(
            "createHLAfederateHandle",
            ambassador,
            value,
            FederateHandle,
            "getFederateHandleFactory",
        )

    def createHLAobjectClassHandle(
        self, ambassador: object, value: ObjectClassHandle | None = None
    ) -> DataElement:
        return self._create_handle_element(
            "createHLAobjectClassHandle",
            ambassador,
            value,
            ObjectClassHandle,
            "getObjectClassHandleFactory",
        )

    def createHLAinteractionClassHandle(
        self, ambassador: object, value: InteractionClassHandle | None = None
    ) -> DataElement:
        return self._create_handle_element(
            "createHLAinteractionClassHandle",
            ambassador,
            value,
            InteractionClassHandle,
            "getInteractionClassHandleFactory",
        )

    def createHLAobjectInstanceHandle(
        self, ambassador: object, value: ObjectInstanceHandle | None = None
    ) -> DataElement:
        return self._create_handle_element(
            "createHLAobjectInstanceHandle",
            ambassador,
            value,
            ObjectInstanceHandle,
            "getObjectInstanceHandleFactory",
        )

    def createHLAattributeHandle(
        self, ambassador: object, value: AttributeHandle | None = None
    ) -> DataElement:
        return self._create_handle_element(
            "createHLAattributeHandle",
            ambassador,
            value,
            AttributeHandle,
            "getAttributeHandleFactory",
        )

    def createHLAparameterHandle(
        self, ambassador: object, value: ParameterHandle | None = None
    ) -> DataElement:
        return self._create_handle_element(
            "createHLAparameterHandle",
            ambassador,
            value,
            ParameterHandle,
            "getParameterHandleFactory",
        )

    def createHLAdimensionHandle(
        self, ambassador: object, value: DimensionHandle | None = None
    ) -> DataElement:
        return self._create_handle_element(
            "createHLAdimensionHandle",
            ambassador,
            value,
            DimensionHandle,
            "getDimensionHandleFactory",
        )

    def createHLAregionHandle(
        self, ambassador: object, value: RegionHandle | None = None
    ) -> DataElement:
        return self._create_handle_element(
            "createHLAregionHandle",
            ambassador,
            value,
            RegionHandle,
            "getRegionHandleFactory",
        )

    def createHLAtransportationTypeHandle(
        self, ambassador: object, value: TransportationTypeHandle | None = None
    ) -> DataElement:
        return self._create_handle_element(
            "createHLAtransportationTypeHandle",
            ambassador,
            value,
            TransportationTypeHandle,
            "getTransportationTypeHandleFactory",
        )

    def createHLAmessageRetractionHandle(
        self, ambassador: object, value: MessageRetractionHandle | None = None
    ) -> DataElement:
        return self._create_handle_element(
            "createHLAmessageRetractionHandle",
            ambassador,
            value,
            MessageRetractionHandle,
            "getMessageRetractionHandleFactory",
        )

    def createHLAlogicalTime(
        self, ambassador: object, value: LogicalTime | None = None
    ) -> DataElement:
        raw_ambassador = self._raw_ambassador(ambassador)
        arguments: list[object] = [raw_ambassador]
        if value is not None:
            if not isinstance(value, LogicalTime):
                raise TypeError("value must be LogicalTime")
            _require_selected_logical_time_value(self._runtime, raw_ambassador, value)
            arguments.append(self._runtime.decode_logical_time(raw_ambassador, value.encodedValue))
        implementation = _call_java(
            self._runtime,
            getattr(self._implementation, "createHLAlogicalTime"),
            *arguments,
        )
        return _JavaLogicalTimeDataElement(implementation, self._runtime, raw_ambassador)

    def createHLAlogicalTimeInterval(
        self, ambassador: object, value: LogicalTimeInterval | None = None
    ) -> DataElement:
        raw_ambassador = self._raw_ambassador(ambassador)
        arguments: list[object] = [raw_ambassador]
        if value is not None:
            if not isinstance(value, LogicalTimeInterval):
                raise TypeError("value must be LogicalTimeInterval")
            _require_selected_logical_time_value(self._runtime, raw_ambassador, value)
            arguments.append(
                self._runtime.decode_logical_interval(raw_ambassador, value.encodedValue)
            )
        implementation = _call_java(
            self._runtime,
            getattr(self._implementation, "createHLAlogicalTimeInterval"),
            *arguments,
        )
        return _JavaLogicalTimeIntervalDataElement(
            implementation,
            self._runtime,
            raw_ambassador,
        )
