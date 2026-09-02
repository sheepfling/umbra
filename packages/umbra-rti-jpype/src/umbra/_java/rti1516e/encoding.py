"""Java-backed IEEE 1516.1-2010 encoding façade.

The public 2010 encoding classes are abstract contracts.  This module is the
provider-owned implementation used after a vendor ``RtiFactory`` has been
loaded through JPype.  Values, padding, and malformed-buffer validation stay
inside the Java provider; Python only adapts standard arguments and snapshots
returned elements.
"""

from __future__ import annotations

from abc import update_abstractmethods
from typing import Any, Callable

from hla.rti1516e.byte_types import BytesLike
from hla.rti1516e.encoding import (
    ByteWrapper,
    DataElement,
    DataElementFactory,
    DecoderException,
    EncoderException,
    EncoderFactory,
    HLAASCIIchar,
    HLAASCIIstring,
    HLAboolean,
    HLAbyte,
    HLAfixedArray,
    HLAfixedRecord,
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
    HLAoctet,
    HLAoctetPairBE,
    HLAoctetPairLE,
    HLAopaqueData,
    HLAunicodeChar,
    HLAunicodeString,
    HLAvariableArray,
    HLAvariantRecord,
)
from hla.rti1516e.exceptions import RTIexception, RTIinternalError, exceptionForName


def _simple_name(value: str) -> str:
    return value.rsplit(".", 1)[-1].rsplit("$", 1)[-1]


def _call_java(
    runtime: Any,
    function: Callable[..., object],
    *args: object,
    encoding_error: type[Exception] | None = None,
) -> object:
    try:
        return function(*args)
    except RTIexception:
        raise
    except Exception as error:
        if encoding_error is not None and isinstance(error, (IndexError, ValueError, UnicodeError)):
            raise encoding_error(str(error)) from error
        name = runtime.exception_name(error)
        if name is None:
            raise RTIinternalError(f"Java 1516e encoding call failed: {error}") from error
        name = _simple_name(name)
        if name in {
            "EncoderException",
            "DecoderException",
            "IllegalArgumentException",
            "IndexOutOfBoundsException",
            "ArrayIndexOutOfBoundsException",
            "BufferUnderflowException",
        } and encoding_error is not None:
            raise encoding_error(str(error)) from error
        raise exceptionForName(name, str(error), error) from error


class _JavaDataElement(DataElement):
    def __init__(self, implementation: object, runtime: Any) -> None:
        self._implementation = implementation
        self._runtime = runtime

    def getOctetBoundary(self) -> int:
        return int(_call_java(self._runtime, self._implementation.getOctetBoundary))

    def getEncodedLength(self) -> int:
        return int(
            _call_java(
                self._runtime,
                self._implementation.getEncodedLength,
                encoding_error=EncoderException,
            )
        )

    def toByteArray(self) -> bytes:
        value = _call_java(
            self._runtime,
            self._implementation.toByteArray,
            encoding_error=EncoderException,
        )
        return bytes(int(item) & 0xFF for item in value)  # type: ignore[union-attr]

    def encode(self, byteWrapper: ByteWrapper | object | None = None) -> object:
        if byteWrapper is None:
            return self.toByteArray()
        remaining = getattr(byteWrapper, "remaining", None)
        if callable(remaining):
            try:
                available = int(remaining())
            except Exception:  # noqa: BLE001 - foreign cursor may not expose capacity cleanly
                available = None
            if available is not None and available < self.getEncodedLength():
                # Guard the Python boundary before a provider-specific JNI
                # implementation can enter native code with an undersized
                # destination window and fail to return a typed exception.
                raise EncoderException(
                    "ByteWrapper does not contain enough space for this data element"
                )
        raw_wrapper = self._runtime.java_byte_wrapper(byteWrapper)
        _call_java(
            self._runtime,
            self._implementation.encode,
            raw_wrapper,
            encoding_error=EncoderException,
        )
        self._runtime.copy_from_java_byte_wrapper(raw_wrapper, byteWrapper)
        return self

    def decode(self, value: ByteWrapper | BytesLike) -> object:
        is_wrapper = isinstance(value, ByteWrapper)
        raw_value = self._runtime.java_byte_wrapper(value) if is_wrapper else self._runtime.byte_array(value)
        _call_java(
            self._runtime,
            self._implementation.decode,
            raw_value,
            encoding_error=DecoderException,
        )
        if is_wrapper:
            # Java's ByteWrapper overload advances the caller's wrapper as it
            # consumes the element.  Copy that position back so a sequence of
            # provider-owned decodes has the same cursor semantics as Java.
            self._runtime.copy_from_java_byte_wrapper(raw_value, value)
        return self


def _signed(value: int, bits: int) -> int:
    value = int(value)
    low, high = -(1 << (bits - 1)), (1 << bits) - 1
    if value < low or value > high:
        raise ValueError(f"value outside {bits}-bit Java range: {value}")
    if value >= 1 << (bits - 1):
        return value - (1 << bits)
    return value


def _primitive(runtime: Any, value: object, kind: str) -> object:
    converter = getattr(runtime, f"java_{kind}", None)
    if callable(converter):
        return converter(value)
    return value


def _make_scalar_wrapper(
    base: type[DataElement],
    kind: str,
    cast: Callable[[object], object],
) -> type[_JavaDataElement]:
    class Scalar(_JavaDataElement, base):
        def getValue(self) -> object:
            return cast(_call_java(self._runtime, self._implementation.getValue))

        def setValue(self, value: object) -> "Scalar":
            converted = cast(value)
            _call_java(
                self._runtime,
                self._implementation.setValue,
                _primitive(self._runtime, converted, kind),
                encoding_error=EncoderException,
            )
            return self

    Scalar.__name__ = f"_Java{base.__name__}"
    Scalar.__qualname__ = Scalar.__name__
    return Scalar


_SCALAR_WRAPPERS: dict[str, type[_JavaDataElement]] = {
    "HLAinteger16BE": _make_scalar_wrapper(HLAinteger16BE, "short", lambda value: _signed(int(value), 16)),
    "HLAinteger16LE": _make_scalar_wrapper(HLAinteger16LE, "short", lambda value: _signed(int(value), 16)),
    "HLAinteger32BE": _make_scalar_wrapper(HLAinteger32BE, "int", lambda value: _signed(int(value), 32)),
    "HLAinteger32LE": _make_scalar_wrapper(HLAinteger32LE, "int", lambda value: _signed(int(value), 32)),
    "HLAinteger64BE": _make_scalar_wrapper(HLAinteger64BE, "long", lambda value: _signed(int(value), 64)),
    "HLAinteger64LE": _make_scalar_wrapper(HLAinteger64LE, "long", lambda value: _signed(int(value), 64)),
    "HLAfloat32BE": _make_scalar_wrapper(HLAfloat32BE, "float", float),
    "HLAfloat32LE": _make_scalar_wrapper(HLAfloat32LE, "float", float),
    "HLAfloat64BE": _make_scalar_wrapper(HLAfloat64BE, "double", float),
    "HLAfloat64LE": _make_scalar_wrapper(HLAfloat64LE, "double", float),
    "HLAbyte": _make_scalar_wrapper(HLAbyte, "byte", lambda value: _signed(int(value), 8)),
    "HLAoctet": _make_scalar_wrapper(HLAoctet, "byte", lambda value: _signed(int(value), 8)),
    "HLAASCIIchar": _make_scalar_wrapper(HLAASCIIchar, "byte", lambda value: _signed(int(value), 8)),
    "HLAunicodeChar": _make_scalar_wrapper(HLAunicodeChar, "short", lambda value: _signed(int(value), 16)),
    "HLAoctetPairBE": _make_scalar_wrapper(HLAoctetPairBE, "short", lambda value: _signed(int(value), 16)),
    "HLAoctetPairLE": _make_scalar_wrapper(HLAoctetPairLE, "short", lambda value: _signed(int(value), 16)),
}


class _JavaHLAASCIIstring(_JavaDataElement, HLAASCIIstring):
    def getValue(self) -> str:
        return str(_call_java(self._runtime, self._implementation.getValue))

    def setValue(self, value: str) -> "_JavaHLAASCIIstring":
        _call_java(self._runtime, self._implementation.setValue, str(value), encoding_error=EncoderException)
        return self


class _JavaHLAunicodeString(_JavaDataElement, HLAunicodeString):
    def getValue(self) -> str:
        return str(_call_java(self._runtime, self._implementation.getValue))

    def setValue(self, value: str) -> "_JavaHLAunicodeString":
        _call_java(self._runtime, self._implementation.setValue, str(value), encoding_error=EncoderException)
        return self


class _JavaHLAboolean(_JavaDataElement, HLAboolean):
    def getValue(self) -> bool:
        return bool(_call_java(self._runtime, self._implementation.getValue))

    def setValue(self, value: bool) -> "_JavaHLAboolean":
        _call_java(self._runtime, self._implementation.setValue, bool(value), encoding_error=EncoderException)
        return self


class _JavaHLAopaqueData(_JavaDataElement, HLAopaqueData):
    def size(self) -> int:
        return int(_call_java(self._runtime, self._implementation.size))

    def get(self, index: int) -> int:
        return int(_call_java(self._runtime, self._implementation.get, int(index)))

    def getValue(self) -> bytes:
        value = _call_java(self._runtime, self._implementation.getValue)
        return bytes(int(item) & 0xFF for item in value)  # type: ignore[union-attr]

    def setValue(self, value: BytesLike) -> None:
        _call_java(self._runtime, self._implementation.setValue, self._runtime.byte_array(value), encoding_error=EncoderException)

    def __iter__(self):
        for index in range(self.size()):
            yield self.get(index)


class _JavaElementFactory(DataElementFactory):
    def __init__(self, implementation: object, runtime: Any) -> None:
        self._implementation = implementation
        self._runtime = runtime

    def createElement(self, index: int) -> DataElement:
        return _wrap_element(
            _call_java(self._runtime, self._implementation.createElement, int(index)),
            self._runtime,
        )


def _raw_element(value: object) -> object:
    return getattr(value, "_implementation", value)


def _raw_factory(value: object, runtime: Any) -> object:
    implementation = getattr(value, "_implementation", None)
    if implementation is not None:
        return implementation
    return runtime.data_element_factory(value)


class _JavaHLAvariableArray(_JavaDataElement, HLAvariableArray):
    def addElement(self, dataElement: DataElement) -> None:
        _call_java(self._runtime, self._implementation.addElement, _raw_element(dataElement), encoding_error=EncoderException)

    def resize(self, newSize: int) -> "_JavaHLAvariableArray":
        _call_java(self._runtime, self._implementation.resize, int(newSize), encoding_error=EncoderException)
        return self

    def size(self) -> int:
        return int(_call_java(self._runtime, self._implementation.size))

    def get(self, index: int) -> DataElement:
        return _wrap_element(_call_java(self._runtime, self._implementation.get, int(index)), self._runtime)

    def __iter__(self):
        for index in range(self.size()):
            yield self.get(index)


class _JavaHLAfixedArray(_JavaDataElement, HLAfixedArray):
    def size(self) -> int:
        return int(_call_java(self._runtime, self._implementation.size))

    def get(self, index: int) -> DataElement:
        return _wrap_element(_call_java(self._runtime, self._implementation.get, int(index)), self._runtime)

    def __iter__(self):
        for index in range(self.size()):
            yield self.get(index)


class _JavaHLAfixedRecord(_JavaDataElement, HLAfixedRecord):
    def add(self, dataElement: DataElement) -> None:
        _call_java(self._runtime, self._implementation.add, _raw_element(dataElement), encoding_error=EncoderException)

    def size(self) -> int:
        return int(_call_java(self._runtime, self._implementation.size))

    def get(self, index: int) -> DataElement:
        return _wrap_element(_call_java(self._runtime, self._implementation.get, int(index)), self._runtime)

    def __iter__(self):
        for index in range(self.size()):
            yield self.get(index)


class _JavaHLAvariantRecord(_JavaDataElement, HLAvariantRecord):
    def setVariant(self, discriminant: DataElement, dataElement: DataElement) -> None:
        _call_java(
            self._runtime,
            self._implementation.setVariant,
            _raw_element(discriminant),
            _raw_element(dataElement),
            encoding_error=EncoderException,
        )

    def setDiscriminant(self, discriminant: DataElement) -> None:
        _call_java(self._runtime, self._implementation.setDiscriminant, _raw_element(discriminant), encoding_error=EncoderException)

    def getDiscriminant(self) -> DataElement:
        return _wrap_element(_call_java(self._runtime, self._implementation.getDiscriminant), self._runtime)

    def getValue(self) -> DataElement | None:
        value = _call_java(self._runtime, self._implementation.getValue)
        return None if value is None else _wrap_element(value, self._runtime)


def _wrap_element(value: object, runtime: Any) -> DataElement:
    # JPype represents Java dynamic proxies as classes such as
    # ``jdk.proxy2.$Proxy7``.  Their simple class name does not contain the
    # standard encoder interface, even though ``isinstance`` against that
    # interface is true.  Ask the runtime for the authoritative Java
    # interface identity before falling back to class-name heuristics (which
    # keeps the fake/runtime-neutral tests usable).
    is_instance = getattr(runtime, "is_java_instance", None)
    if callable(is_instance):
        for interface_name, wrapper in (
            *tuple(_SCALAR_WRAPPERS.items()),
            ("HLAASCIIstring", _JavaHLAASCIIstring),
            ("HLAunicodeString", _JavaHLAunicodeString),
            ("HLAboolean", _JavaHLAboolean),
            ("HLAopaqueData", _JavaHLAopaqueData),
            ("HLAvariableArray", _JavaHLAvariableArray),
            ("HLAfixedArray", _JavaHLAfixedArray),
            ("HLAfixedRecord", _JavaHLAfixedRecord),
            ("HLAvariantRecord", _JavaHLAvariantRecord),
        ):
            try:
                if is_instance(value, f"hla.rti1516e.encoding.{interface_name}"):
                    return wrapper(value, runtime)
            except Exception:
                # A provider may return a concrete object from a different
                # class loader.  Class-name dispatch below remains the safe
                # fallback for that case.
                continue
    name = str(getattr(getattr(value, "getClass", lambda: None)(), "getSimpleName", lambda: "")())
    if not name:
        name = type(value).__name__
    for interface_name, wrapper in _SCALAR_WRAPPERS.items():
        # A provider-owned implementation is allowed to contain a standard
        # interface token in its class name (for example,
        # ``VendorHLAinteger32BE``).  Substring dispatch would silently
        # retype that carrier and can then call methods that it does not
        # implement.  JPype's interface probe above is authoritative; this
        # fallback is deliberately limited to an exact simple class name.
        if interface_name == name:
            return wrapper(value, runtime)
    # The provider can expose a concrete implementation name rather than the
    # interface.  Probe the standard methods only after scalar names.
    for interface, wrapper in (
        ("HLAASCIIstring", _JavaHLAASCIIstring),
        ("HLAunicodeString", _JavaHLAunicodeString),
        ("HLAboolean", _JavaHLAboolean),
        ("HLAopaqueData", _JavaHLAopaqueData),
        ("HLAvariableArray", _JavaHLAvariableArray),
        ("HLAfixedArray", _JavaHLAfixedArray),
        ("HLAfixedRecord", _JavaHLAfixedRecord),
        ("HLAvariantRecord", _JavaHLAvariantRecord),
    ):
        if interface == name:
            return wrapper(value, runtime)
    # JPype proxies may not expose a useful class simple name.  A generic data
    # element is still preferable to leaking a raw provider object.
    return _JavaDataElement(value, runtime)


class JavaEncoderFactory(EncoderFactory):
    """Provider-scoped façade over the exact Java 2010 ``EncoderFactory``."""

    def __init__(self, implementation: object, runtime: Any) -> None:
        self._implementation = implementation
        self._runtime = runtime

    def _create(self, method_name: str, value: object | None, converter: Callable[[object], object] | None = None) -> DataElement:
        method = getattr(self._implementation, method_name)
        if value is None:
            raw = _call_java(self._runtime, method)
        else:
            raw = _call_java(self._runtime, method, converter(value) if converter else value)
        return _wrap_element(raw, self._runtime)

    def createHLAfixedArray(
        self, factory_or_element: object | None = None, *elements_or_size: object
    ) -> HLAfixedArray:
        method = self._implementation.createHLAfixedArray
        if factory_or_element is None and not elements_or_size:
            raw = _call_java(self._runtime, method)
        elif len(elements_or_size) == 1 and isinstance(elements_or_size[0], int):
            raw = _call_java(
                self._runtime,
                method,
                _raw_factory(factory_or_element, self._runtime),
                int(elements_or_size[0]),
            )
        else:
            elements = (
                list(factory_or_element)
                if isinstance(factory_or_element, (list, tuple)) and not elements_or_size
                else [factory_or_element, *elements_or_size]
            )
            elements = [item for item in elements if item is not None]
            raw = _call_java(self._runtime, method, *[_raw_element(item) for item in elements])
        return _JavaHLAfixedArray(raw, self._runtime)

    def createHLAfixedRecord(self) -> HLAfixedRecord:
        return _JavaHLAfixedRecord(_call_java(self._runtime, self._implementation.createHLAfixedRecord), self._runtime)

    def createHLAvariantRecord(self, discriminant: DataElement) -> HLAvariantRecord:
        return _JavaHLAvariantRecord(
            _call_java(self._runtime, self._implementation.createHLAvariantRecord, _raw_element(discriminant)),
            self._runtime,
        )

    def createHLAvariableArray(self, factory: DataElementFactory, *elements: DataElement) -> HLAvariableArray:
        raw = _call_java(
            self._runtime,
            self._implementation.createHLAvariableArray,
            _raw_factory(factory, self._runtime),
            *[_raw_element(item) for item in elements],
        )
        return _JavaHLAvariableArray(raw, self._runtime)

    def createHLAopaqueData(self, value: BytesLike | None = None) -> HLAopaqueData:
        return _JavaHLAopaqueData(
            _call_java(
                self._runtime,
                self._implementation.createHLAopaqueData,
                *(() if value is None else (self._runtime.byte_array(value),)),
            ),
            self._runtime,
        )


def _install_scalar_factory(method_name: str, wrapper_name: str, converter: Callable[[object], object]) -> None:
    def create(self: JavaEncoderFactory, value: object | None = None) -> DataElement:
        return self._create(method_name, value, converter)

    create.__name__ = method_name
    create.__qualname__ = f"JavaEncoderFactory.{method_name}"
    setattr(JavaEncoderFactory, method_name, create)


for _name, _base, _converter in (
    ("createHLAinteger16BE", "HLAinteger16BE", lambda value: _signed(int(value), 16)),
    ("createHLAinteger16LE", "HLAinteger16LE", lambda value: _signed(int(value), 16)),
    ("createHLAinteger32BE", "HLAinteger32BE", lambda value: _signed(int(value), 32)),
    ("createHLAinteger32LE", "HLAinteger32LE", lambda value: _signed(int(value), 32)),
    ("createHLAinteger64BE", "HLAinteger64BE", lambda value: _signed(int(value), 64)),
    ("createHLAinteger64LE", "HLAinteger64LE", lambda value: _signed(int(value), 64)),
    ("createHLAfloat32BE", "HLAfloat32BE", float),
    ("createHLAfloat32LE", "HLAfloat32LE", float),
    ("createHLAfloat64BE", "HLAfloat64BE", float),
    ("createHLAfloat64LE", "HLAfloat64LE", float),
    ("createHLAbyte", "HLAbyte", lambda value: _signed(int(value), 8)),
    ("createHLAoctet", "HLAoctet", lambda value: _signed(int(value), 8)),
    ("createHLAASCIIchar", "HLAASCIIchar", lambda value: _signed(int(value), 8)),
    ("createHLAunicodeChar", "HLAunicodeChar", lambda value: _signed(int(value), 16)),
    ("createHLAoctetPairBE", "HLAoctetPairBE", lambda value: _signed(int(value), 16)),
    ("createHLAoctetPairLE", "HLAoctetPairLE", lambda value: _signed(int(value), 16)),
):
    _install_scalar_factory(_name, _base, _converter)


def _install_string_factory(method_name: str) -> None:
    def create(self: JavaEncoderFactory, value: str | None = None) -> DataElement:
        return self._create(method_name, value, str)

    create.__name__ = method_name
    setattr(JavaEncoderFactory, method_name, create)


_install_string_factory("createHLAASCIIstring")
_install_string_factory("createHLAunicodeString")


def _create_boolean(self: JavaEncoderFactory, value: bool | None = None) -> DataElement:
    return self._create("createHLAboolean", value, bool)


JavaEncoderFactory.createHLAboolean = _create_boolean  # type: ignore[method-assign]
update_abstractmethods(JavaEncoderFactory)


__all__ = ["JavaEncoderFactory"]
