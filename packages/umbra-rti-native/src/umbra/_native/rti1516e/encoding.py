"""Direct C++ IEEE 1516.1-2010 encoder façade.

The public package owns only the provider-neutral contracts.  These classes
delegate every value, byte order, and malformed-buffer decision to the
official ``rti1516e`` C++ data-element implementation exposed by pybind11.
The native provider owns the scalar family, ``HLAopaqueData``, and the official
fixed/variable array and fixed/variant record composites. RTI/MOM service
families outside the encoder/time/reference-object slice remain explicit
unsupported services.
"""

from __future__ import annotations

from abc import update_abstractmethods
from collections.abc import Callable
from typing import Any, TypeVar

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

from . import _native_2010


_Result = TypeVar("_Result")


def _call_native(
    function: Callable[..., _Result],
    *args: object,
    encoding_error: type[Exception] | None = None,
) -> _Result:
    try:
        return function(*args)
    except _native_2010.Native2010RtiError as error:
        name, separator, message = str(error).partition(": ")
        if encoding_error is not None and name in {"EncoderException", "DecoderException"}:
            raise encoding_error(message if separator else name) from error
        # The native extension uses the same standard exception names as the
        # provider-neutral API for lifecycle failures.
        from hla.rti1516e.exceptions import exceptionForName

        raise exceptionForName(
            name, message if separator else name, error
        ) from error


def _signed(value: int, bits: int) -> int:
    value = int(value)
    low, high = -(1 << (bits - 1)), (1 << bits) - 1
    if value < low or value > high:
        raise ValueError(f"value outside {bits}-bit range: {value}")
    return value - (1 << bits) if value >= 1 << (bits - 1) else value


def _octet_bits(value: object) -> int:
    return _signed(int(value), 8) & 0xFF


def _ascii_char(value: object) -> int:
    return _signed(int(value), 8)


def _unicode_char(value: object) -> int:
    return _signed(int(value), 16) & 0xFFFF


def _octet_pair(value: object) -> int:
    return _signed(int(value), 16) & 0xFFFF


def _ascii_string(value: object) -> str:
    value = str(value)
    value.encode("ascii")
    return value


def _decode_length(implementation: Any, raw: bytes) -> int:
    """Return the one-element length needed for a Java-shaped ByteWrapper."""

    # Fixed-width scalar encodings can use the provider's current width.
    name = type(implementation).__name__
    if "ASCIIstring" in name:
        width = 1
    elif "unicodeString" in name:
        width = 2
    elif "opaqueData" in name:
        width = 1
    else:
        return int(_call_native(implementation.get_encoded_length))
    if len(raw) < 4:
        raise DecoderException("string encoding is truncated")
    count = int.from_bytes(raw[:4], "big", signed=False)
    return 4 + count * width


class _NativeDataElement(DataElement):
    """Common forwarding logic for a provider-owned C++ data element."""

    def __init__(self, implementation: Any) -> None:
        self._implementation = implementation

    def getOctetBoundary(self) -> int:
        return int(_call_native(self._implementation.get_octet_boundary))

    def getEncodedLength(self) -> int:
        return int(
            _call_native(
                self._implementation.get_encoded_length,
                encoding_error=EncoderException,
            )
        )

    def toByteArray(self) -> bytes:
        return bytes(
            _call_native(
                self._implementation.to_byte_array,
                encoding_error=EncoderException,
            )
        )

    def encode(self, byteWrapper: ByteWrapper | None = None) -> object:
        encoded = self.toByteArray()
        if byteWrapper is None:
            return encoded
        if not isinstance(byteWrapper, ByteWrapper):
            raise TypeError("byteWrapper must be ByteWrapper")
        byteWrapper.put(encoded)
        return self

    def decode(self, value: ByteWrapper | BytesLike) -> object:
        if isinstance(value, ByteWrapper):
            position = value.position()
            raw = bytes(value.getBuffer()[position : position + value.remaining()])
            decode_from = getattr(self._implementation, "decode_from", None)
            if callable(decode_from):
                length = int(
                    _call_native(
                        decode_from,
                        raw,
                        0,
                        encoding_error=DecoderException,
                    )
                )
            else:
                length = _decode_length(self._implementation, raw)
                _call_native(
                    self._implementation.decode,
                    raw[:length],
                    encoding_error=DecoderException,
                )
            value.advance(length)
        else:
            _call_native(
                self._implementation.decode,
                bytes(value),
                encoding_error=DecoderException,
            )
        return self


class _NativeHLAopaqueData(_NativeDataElement, HLAopaqueData):
    """Provider-owned opaque octets with the standard count prefix."""

    def size(self) -> int:
        return int(_call_native(self._implementation.data_length))

    def get(self, index: int) -> int:
        value = self.getValue()
        try:
            return int(value[int(index)])
        except IndexError as error:
            raise IndexError(f"opaque-data index out of range: {index}") from error

    def getValue(self) -> bytes:
        return bytes(_call_native(self._implementation.get_value))

    def setValue(self, value: BytesLike) -> None:
        _call_native(
            self._implementation.set_value,
            bytes(value),
            encoding_error=EncoderException,
        )

    def __iter__(self):
        for index in range(self.size()):
            yield self.get(index)


class _NativeHLAvariableArray(_NativeDataElement, HLAvariableArray):
    """Provider-owned 2010 variable array backed by the official C++ type."""

    def __init__(
        self,
        implementation: Any,
        factory: DataElementFactory,
        prototype: DataElement,
    ) -> None:
        super().__init__(implementation)
        self._factory = factory
        self._prototype = prototype

    def addElement(self, dataElement: DataElement) -> None:
        if not isinstance(dataElement, type(self._prototype)):
            raise TypeError("dataElement does not match the variable-array prototype")
        _call_native(
            self._implementation.add_element,
            dataElement.toByteArray(),
            encoding_error=EncoderException,
        )

    def resize(self, newSize: int) -> None:
        if type(newSize) is not int or newSize < 0:
            raise ValueError("variable-array size must be a non-negative int")
        while self.size() < newSize:
            self.addElement(self._factory.createElement(self.size()))
        if self.size() > newSize:
            raise NotImplementedError("native 2010 variable-array shrinking is not supported")

    def size(self) -> int:
        return int(_call_native(self._implementation.size))

    def get(self, index: int) -> DataElement:
        if type(index) is not int or not 0 <= index < self.size():
            raise IndexError("data-element index is outside the variable-array range")
        element = self._factory.createElement(index)
        if not isinstance(element, type(self._prototype)):
            raise TypeError("factory returned an element that does not match the prototype")
        element.decode(
            bytes(
                _call_native(
                    self._implementation.get_element_bytes,
                    index,
                    encoding_error=EncoderException,
                )
            )
        )
        return element


class _NativeHLAfixedArray(_NativeDataElement, HLAfixedArray):
    """Provider-owned 2010 fixed array backed by the official C++ type."""

    def __init__(
        self,
        implementation: Any,
        factory: DataElementFactory | None,
        prototype: DataElement,
        elements: list[DataElement] | None = None,
    ) -> None:
        super().__init__(implementation)
        self._factory = factory
        self._prototype = prototype
        self._elements = list(elements or [])

    def size(self) -> int:
        return int(_call_native(self._implementation.size))

    def set(self, index: int, dataElement: DataElement) -> None:
        if type(index) is not int or not 0 <= index < self.size():
            raise IndexError("data-element index is outside the fixed-array range")
        if not isinstance(dataElement, type(self._prototype)):
            raise TypeError("dataElement does not match the fixed-array prototype")
        _call_native(
            self._implementation.set_element,
            index,
            dataElement.toByteArray(),
            encoding_error=EncoderException,
        )

    def get(self, index: int) -> DataElement:
        if type(index) is not int or not 0 <= index < self.size():
            raise IndexError("data-element index is outside the fixed-array range")
        if index < len(self._elements):
            element = self._elements[index]
        else:
            if self._factory is None:
                raise EncoderException("native fixed-array element factory is unavailable")
            element = self._factory.createElement(index)
            if not isinstance(element, type(self._prototype)):
                raise TypeError("factory returned an element that does not match the prototype")
        element.decode(
            bytes(
                _call_native(
                    self._implementation.get_element_bytes,
                    index,
                    encoding_error=EncoderException,
                )
            )
        )
        return element


class _NativeHLAfixedRecord(_NativeDataElement, HLAfixedRecord):
    """Provider-owned heterogeneous 2010 fixed record."""

    def __init__(self, implementation: Any) -> None:
        super().__init__(implementation)
        self._elements: list[DataElement] = []

    def add(self, dataElement: DataElement) -> None:
        self.appendElement(dataElement)

    def appendElement(self, dataElement: DataElement) -> None:
        if not isinstance(dataElement, _NativeDataElement):
            raise TypeError("native fixed records require native DataElements")
        _call_native(
            self._implementation.append_element,
            dataElement._implementation,
            dataElement.toByteArray(),
            encoding_error=EncoderException,
        )
        self._elements.append(dataElement)

    def size(self) -> int:
        return int(_call_native(self._implementation.size))

    def get(self, index: int) -> DataElement:
        if type(index) is not int or not 0 <= index < self.size():
            raise IndexError("data-element index is outside the fixed-record range")
        if index >= len(self._elements):
            raise EncoderException("native fixed record component state is unavailable")
        element = self._elements[index]
        element.decode(
            bytes(
                _call_native(
                    self._implementation.get_element_bytes,
                    index,
                    encoding_error=EncoderException,
                )
            )
        )
        return element

    def set(self, index: int, dataElement: DataElement) -> None:
        if type(index) is not int or not 0 <= index < self.size():
            raise IndexError("data-element index is outside the fixed-record range")
        if index >= len(self._elements):
            raise EncoderException("native fixed record component state is unavailable")
        if not isinstance(dataElement, type(self._elements[index])):
            raise TypeError("dataElement does not match the fixed-record component type")
        if not isinstance(dataElement, _NativeDataElement):
            raise TypeError("native fixed records require native DataElements")
        _call_native(
            self._implementation.set_element,
            index,
            dataElement._implementation,
            dataElement.toByteArray(),
            encoding_error=EncoderException,
        )
        self._elements[index].decode(dataElement.toByteArray())


class _NativeHLAvariantRecord(_NativeDataElement, HLAvariantRecord):
    """Provider-owned 2010 discriminated variant record."""

    def __init__(self, implementation: Any, discriminant: DataElement) -> None:
        super().__init__(implementation)
        self._discriminant = discriminant
        self._variants: dict[bytes, DataElement] = {}

    def _require_discriminant(self, discriminant: DataElement) -> bytes:
        if not isinstance(discriminant, type(self._discriminant)):
            raise TypeError("discriminant does not match the variant-record prototype")
        return discriminant.toByteArray()

    def setVariant(self, discriminant: DataElement, dataElement: DataElement) -> None:
        key = self._require_discriminant(discriminant)
        if not isinstance(dataElement, _NativeDataElement):
            raise TypeError("native variant records require native DataElements")
        existing = self._variants.get(key)
        if existing is not None and not isinstance(dataElement, type(existing)):
            raise TypeError("dataElement does not match the mapped variant type")
        method = self._implementation.set_variant if existing is not None else self._implementation.add_variant
        _call_native(
            method,
            self._discriminant._implementation,
            key,
            dataElement._implementation,
            dataElement.toByteArray(),
            encoding_error=EncoderException,
        )
        self._variants[key] = dataElement

    def setDiscriminant(self, discriminant: DataElement) -> None:
        key = self._require_discriminant(discriminant)
        _call_native(
            self._implementation.set_discriminant,
            self._discriminant._implementation,
            key,
            encoding_error=EncoderException,
        )

    def getDiscriminant(self) -> DataElement:
        self._discriminant.decode(
            bytes(
                _call_native(
                    self._implementation.get_discriminant_bytes,
                    encoding_error=EncoderException,
                )
            )
        )
        return self._discriminant

    def getValue(self) -> DataElement | None:
        key = bytes(self.getDiscriminant().toByteArray())
        element = self._variants.get(key)
        if element is None:
            return None
        element.decode(
            bytes(
                _call_native(
                    self._implementation.get_variant_bytes,
                    encoding_error=EncoderException,
                )
            )
        )
        return element


def _make_scalar_wrapper(
    base: type[DataElement],
    setter: Callable[[object], object],
    getter: Callable[[object], object],
) -> type[_NativeDataElement]:
    class Scalar(_NativeDataElement, base):
        def getValue(self) -> object:
            return getter(_call_native(self._implementation.get_value))

        def setValue(self, value: object) -> object:
            _call_native(
                self._implementation.set_value,
                setter(value),
                encoding_error=EncoderException,
            )
            return self

    Scalar.__name__ = f"_Native{base.__name__}"
    Scalar.__qualname__ = Scalar.__name__
    return Scalar


_SCALAR_WRAPPERS: dict[str, type[_NativeDataElement]] = {
    "HLAinteger16BE": _make_scalar_wrapper(HLAinteger16BE, lambda value: _signed(int(value), 16), int),
    "HLAinteger16LE": _make_scalar_wrapper(HLAinteger16LE, lambda value: _signed(int(value), 16), int),
    "HLAinteger32BE": _make_scalar_wrapper(HLAinteger32BE, lambda value: _signed(int(value), 32), int),
    "HLAinteger32LE": _make_scalar_wrapper(HLAinteger32LE, lambda value: _signed(int(value), 32), int),
    "HLAinteger64BE": _make_scalar_wrapper(HLAinteger64BE, lambda value: _signed(int(value), 64), int),
    "HLAinteger64LE": _make_scalar_wrapper(HLAinteger64LE, lambda value: _signed(int(value), 64), int),
    "HLAfloat32BE": _make_scalar_wrapper(HLAfloat32BE, float, float),
    "HLAfloat32LE": _make_scalar_wrapper(HLAfloat32LE, float, float),
    "HLAfloat64BE": _make_scalar_wrapper(HLAfloat64BE, float, float),
    "HLAfloat64LE": _make_scalar_wrapper(HLAfloat64LE, float, float),
    "HLAbyte": _make_scalar_wrapper(HLAbyte, _octet_bits, lambda value: _signed(int(value), 8)),
    "HLAoctet": _make_scalar_wrapper(HLAoctet, _octet_bits, lambda value: _signed(int(value), 8)),
    "HLAASCIIchar": _make_scalar_wrapper(HLAASCIIchar, _ascii_char, lambda value: _signed(int(value), 8)),
    "HLAunicodeChar": _make_scalar_wrapper(HLAunicodeChar, _unicode_char, lambda value: _signed(int(value), 16)),
    "HLAoctetPairBE": _make_scalar_wrapper(HLAoctetPairBE, _octet_pair, lambda value: _signed(int(value), 16)),
    "HLAoctetPairLE": _make_scalar_wrapper(HLAoctetPairLE, _octet_pair, lambda value: _signed(int(value), 16)),
    "HLAboolean": _make_scalar_wrapper(HLAboolean, bool, bool),
    "HLAASCIIstring": _make_scalar_wrapper(HLAASCIIstring, _ascii_string, str),
    "HLAunicodeString": _make_scalar_wrapper(HLAunicodeString, str, str),
}


class Native2010EncoderFactory(EncoderFactory):
    """Provider-owned 2010 encoder factory for the direct C++ route."""

    def _create(self, name: str, value: object | None) -> DataElement:
        native_type = getattr(_native_2010, f"Native{name}")
        implementation = (
            _call_native(native_type)
            if value is None
            else _call_native(native_type, value)
        )
        return _SCALAR_WRAPPERS[name](implementation)

    def createHLAopaqueData(self, value: BytesLike | None = None) -> HLAopaqueData:
        native_type = _native_2010.NativeHLAopaqueData
        implementation = (
            _call_native(native_type)
            if value is None
            else _call_native(native_type, bytes(value))
        )
        return _NativeHLAopaqueData(implementation)


for _method_name, _type_name, _converter in (
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
    ("createHLAbyte", "HLAbyte", _octet_bits),
    ("createHLAoctet", "HLAoctet", _octet_bits),
    ("createHLAASCIIchar", "HLAASCIIchar", _ascii_char),
    ("createHLAunicodeChar", "HLAunicodeChar", _unicode_char),
    ("createHLAoctetPairBE", "HLAoctetPairBE", _octet_pair),
    ("createHLAoctetPairLE", "HLAoctetPairLE", _octet_pair),
    ("createHLAboolean", "HLAboolean", bool),
    ("createHLAASCIIstring", "HLAASCIIstring", _ascii_string),
    ("createHLAunicodeString", "HLAunicodeString", str),
):
    def _create_scalar(
        self: Native2010EncoderFactory,
        value: object | None = None,
        *,
        _type_name: str = _type_name,
        _converter: Callable[[object], object] = _converter,
    ) -> DataElement:
        return self._create(_type_name, None if value is None else _converter(value))

    _create_scalar.__name__ = _method_name
    _create_scalar.__qualname__ = f"Native2010EncoderFactory.{_method_name}"
    setattr(Native2010EncoderFactory, _method_name, _create_scalar)


def _native_factory_for(factory: DataElementFactory, name: str) -> tuple[DataElementFactory, DataElement]:
    if not isinstance(factory, DataElementFactory):
        raise TypeError("factory must be a DataElementFactory")
    prototype = factory.createElement(0)
    if not isinstance(prototype, _NativeDataElement):
        raise TypeError(f"native {name} requires a native DataElement factory")
    return factory, prototype


def _create_variable_array(
    self: Native2010EncoderFactory,
    factory: DataElementFactory,
    *elements: DataElement,
) -> HLAvariableArray:
    factory, prototype = _native_factory_for(factory, "variable arrays")
    implementation = _call_native(_native_2010.NativeHLAvariableArray, prototype._implementation)
    result = _NativeHLAvariableArray(implementation, factory, prototype)
    for element in elements:
        result.addElement(element)
    return result


def _create_fixed_array(
    self: Native2010EncoderFactory,
    factory_or_element: object | None = None,
    *elements_or_size: object,
) -> HLAfixedArray:
    if isinstance(factory_or_element, DataElementFactory):
        if len(elements_or_size) != 1 or type(elements_or_size[0]) is not int:
            raise TypeError("factory-shaped fixed arrays require a size")
        factory, prototype = _native_factory_for(factory_or_element, "fixed arrays")
        size = int(elements_or_size[0])
        if size < 0:
            raise ValueError("fixed-array size must be a non-negative int")
        implementation = _call_native(
            _native_2010.NativeHLAfixedArray,
            prototype._implementation,
            size,
        )
        return _NativeHLAfixedArray(implementation, factory, prototype)

    elements = [factory_or_element, *elements_or_size]
    elements = [element for element in elements if element is not None]
    if not elements or any(not isinstance(element, _NativeDataElement) for element in elements):
        raise TypeError("variadic fixed arrays require native DataElements")
    prototype = elements[0]
    if any(not isinstance(element, type(prototype)) for element in elements):
        raise TypeError("variadic fixed-array elements must have one type")
    implementation = _call_native(
        _native_2010.NativeHLAfixedArray,
        prototype._implementation,
        len(elements),
    )
    result = _NativeHLAfixedArray(implementation, None, prototype, list(elements))
    for index, element in enumerate(elements):
        result.set(index, element)
    return result


def _create_fixed_record(self: Native2010EncoderFactory) -> HLAfixedRecord:
    return _NativeHLAfixedRecord(_call_native(_native_2010.NativeHLAfixedRecord))


def _create_variant_record(
    self: Native2010EncoderFactory,
    discriminantPrototype: DataElement,
) -> HLAvariantRecord:
    if not isinstance(discriminantPrototype, _NativeDataElement):
        raise TypeError("native variant records require a native discriminant prototype")
    implementation = _call_native(
        _native_2010.NativeHLAvariantRecord,
        discriminantPrototype._implementation,
    )
    return _NativeHLAvariantRecord(implementation, discriminantPrototype)


Native2010EncoderFactory.createHLAvariableArray = _create_variable_array  # type: ignore[attr-defined]
Native2010EncoderFactory.createHLAfixedArray = _create_fixed_array  # type: ignore[attr-defined]
Native2010EncoderFactory.createHLAfixedRecord = _create_fixed_record  # type: ignore[attr-defined]
Native2010EncoderFactory.createHLAvariantRecord = _create_variant_record  # type: ignore[attr-defined]


def _unsupported_factory_service(name: str):
    def invoke(self: Native2010EncoderFactory, *args: object, **kwargs: object) -> object:
        raise NotImplementedError(
            f"IEEE 1516.1-2010 native encoder factory has not implemented {name}"
        )

    invoke.__name__ = name
    invoke.__qualname__ = f"Native2010EncoderFactory.{name}"
    return invoke


update_abstractmethods(Native2010EncoderFactory)


__all__ = ["Native2010EncoderFactory"]
