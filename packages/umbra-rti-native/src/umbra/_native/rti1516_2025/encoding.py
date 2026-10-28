"""Native 2025 HLA data-element and encoder implementations."""

from __future__ import annotations

from typing import Any

from hla.rti1516_2025 import BytesLike
from hla.rti1516_2025.encoding import (
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
    HLAunsignedInteger16BE,
    HLAunsignedInteger16LE,
    HLAunsignedInteger32BE,
    HLAunsignedInteger32LE,
    HLAunsignedInteger64BE,
    HLAunsignedInteger64LE,
    HLAvariableArray,
    HLAvariantRecord,
    _require_ascii_char,
    _require_ascii_string,
    _require_byte,
    _require_data_element_factory,
    _require_data_element_index,
    _require_data_element_size,
    _require_float32,
    _require_float32_le,
    _require_float64,
    _require_float64_le,
    _require_integer16,
    _require_integer16_le,
    _require_integer32,
    _require_integer32_le,
    _require_integer64,
    _require_integer64_le,
    _require_octet,
    _require_octet_pair,
    _require_opaque_data,
    _require_opaque_index,
    _require_unicode_char,
    _require_unsigned_integer16,
    _require_unsigned_integer16_le,
    _require_unsigned_integer32,
    _require_unsigned_integer32_le,
    _require_unsigned_integer64,
    _require_unsigned_integer64_le,
)

from . import _native
from ._native_calls import _call_native


class _NativeDataElement:
    """Common forwarding logic for values owned by the C++ binding."""

    def __init__(self, implementation: Any) -> None:
        self._implementation = implementation

    def getOctetBoundary(self) -> int:
        return int(_call_native(self._implementation.get_octet_boundary))

    def getEncodedLength(self) -> int:
        return int(
            _call_native(
                self._implementation.get_encoded_length, encoding_error=EncoderException
            )
        )

    def toByteArray(self) -> bytes:
        return bytes(
            _call_native(
                self._implementation.to_byte_array, encoding_error=EncoderException
            )
        )

    def decode(self, bytes_: BytesLike):
        _call_native(
            self._implementation.decode,
            bytes(bytes_),
            encoding_error=DecoderException,
        )
        return self


class _NativeHLAinteger32BE(_NativeDataElement, HLAinteger32BE):
    def getValue(self) -> int:
        return int(_call_native(self._implementation.get_value))

    def setValue(self, value: int) -> _NativeHLAinteger32BE:
        _call_native(self._implementation.set_value, _require_integer32(value))
        return self


class _NativeHLAinteger16BE(_NativeDataElement, HLAinteger16BE):
    def getValue(self) -> int:
        return int(_call_native(self._implementation.get_value))

    def setValue(self, value: int) -> _NativeHLAinteger16BE:
        _call_native(self._implementation.set_value, _require_integer16(value))
        return self


class _NativeHLAinteger16LE(_NativeDataElement, HLAinteger16LE):
    def getValue(self) -> int:
        return int(_call_native(self._implementation.get_value))

    def setValue(self, value: int) -> _NativeHLAinteger16LE:
        _call_native(self._implementation.set_value, _require_integer16_le(value))
        return self


class _NativeHLAinteger32LE(_NativeDataElement, HLAinteger32LE):
    def getValue(self) -> int:
        return int(_call_native(self._implementation.get_value))

    def setValue(self, value: int) -> _NativeHLAinteger32LE:
        _call_native(self._implementation.set_value, _require_integer32_le(value))
        return self


class _NativeHLAinteger64BE(_NativeDataElement, HLAinteger64BE):
    def getValue(self) -> int:
        return int(_call_native(self._implementation.get_value))

    def setValue(self, value: int) -> _NativeHLAinteger64BE:
        _call_native(self._implementation.set_value, _require_integer64(value))
        return self


class _NativeHLAinteger64LE(_NativeDataElement, HLAinteger64LE):
    def getValue(self) -> int:
        return int(_call_native(self._implementation.get_value))

    def setValue(self, value: int) -> _NativeHLAinteger64LE:
        _call_native(self._implementation.set_value, _require_integer64_le(value))
        return self


class _NativeHLAfloat64BE(_NativeDataElement, HLAfloat64BE):
    def getValue(self) -> float:
        return float(_call_native(self._implementation.get_value))

    def setValue(self, value: float) -> _NativeHLAfloat64BE:
        _call_native(self._implementation.set_value, _require_float64(value))
        return self


class _NativeHLAfloat32BE(_NativeDataElement, HLAfloat32BE):
    def getValue(self) -> float:
        return float(_call_native(self._implementation.get_value))

    def setValue(self, value: float) -> _NativeHLAfloat32BE:
        _call_native(self._implementation.set_value, _require_float32(value))
        return self


class _NativeHLAfloat64LE(_NativeDataElement, HLAfloat64LE):
    def getValue(self) -> float:
        return float(_call_native(self._implementation.get_value))

    def setValue(self, value: float) -> _NativeHLAfloat64LE:
        _call_native(self._implementation.set_value, _require_float64_le(value))
        return self


class _NativeHLAfloat32LE(_NativeDataElement, HLAfloat32LE):
    def getValue(self) -> float:
        return float(_call_native(self._implementation.get_value))

    def setValue(self, value: float) -> _NativeHLAfloat32LE:
        _call_native(self._implementation.set_value, _require_float32_le(value))
        return self


class _NativeHLAunsignedInteger16BE(_NativeDataElement, HLAunsignedInteger16BE):
    def getValue(self) -> int:
        return int(_call_native(self._implementation.get_value))

    def setValue(self, value: int) -> _NativeHLAunsignedInteger16BE:
        _call_native(self._implementation.set_value, _require_unsigned_integer16(value))
        return self


class _NativeHLAunsignedInteger16LE(_NativeDataElement, HLAunsignedInteger16LE):
    def getValue(self) -> int:
        return int(_call_native(self._implementation.get_value))

    def setValue(self, value: int) -> _NativeHLAunsignedInteger16LE:
        _call_native(
            self._implementation.set_value, _require_unsigned_integer16_le(value)
        )
        return self


class _NativeHLAunsignedInteger32LE(_NativeDataElement, HLAunsignedInteger32LE):
    def getValue(self) -> int:
        return int(_call_native(self._implementation.get_value))

    def setValue(self, value: int) -> _NativeHLAunsignedInteger32LE:
        _call_native(
            self._implementation.set_value, _require_unsigned_integer32_le(value)
        )
        return self


class _NativeHLAunsignedInteger64BE(_NativeDataElement, HLAunsignedInteger64BE):
    def getValue(self) -> int:
        return int(_call_native(self._implementation.get_value))

    def setValue(self, value: int) -> _NativeHLAunsignedInteger64BE:
        _call_native(self._implementation.set_value, _require_unsigned_integer64(value))
        return self


class _NativeHLAunsignedInteger64LE(_NativeDataElement, HLAunsignedInteger64LE):
    def getValue(self) -> int:
        return int(_call_native(self._implementation.get_value))

    def setValue(self, value: int) -> _NativeHLAunsignedInteger64LE:
        _call_native(
            self._implementation.set_value, _require_unsigned_integer64_le(value)
        )
        return self


class _NativeHLAbyte(_NativeDataElement, HLAbyte):
    def getValue(self) -> int:
        return int(_call_native(self._implementation.get_value))

    def setValue(self, value: int) -> _NativeHLAbyte:
        _call_native(self._implementation.set_value, _require_byte(value))
        return self


class _NativeHLAoctet(_NativeDataElement, HLAoctet):
    def getValue(self) -> int:
        return int(_call_native(self._implementation.get_value))

    def setValue(self, value: int) -> _NativeHLAoctet:
        _call_native(self._implementation.set_value, _require_octet(value))
        return self


class _NativeHLAASCIIchar(_NativeDataElement, HLAASCIIchar):
    def getValue(self) -> int:
        return int(_call_native(self._implementation.get_value))

    def setValue(self, value: int) -> _NativeHLAASCIIchar:
        _call_native(self._implementation.set_value, _require_ascii_char(value))
        return self


class _NativeHLAASCIIstring(_NativeDataElement, HLAASCIIstring):
    def getValue(self) -> str:
        return str(_call_native(self._implementation.get_value))

    def setValue(self, value: str) -> _NativeHLAASCIIstring:
        _call_native(self._implementation.set_value, _require_ascii_string(value))
        return self


class _NativeHLAunicodeChar(_NativeDataElement, HLAunicodeChar):
    def getValue(self) -> int:
        return int(_call_native(self._implementation.get_value))

    def setValue(self, value: int) -> _NativeHLAunicodeChar:
        _call_native(self._implementation.set_value, _require_unicode_char(value))
        return self


class _NativeHLAoctetPairBE(_NativeDataElement, HLAoctetPairBE):
    def getValue(self) -> int:
        return int(_call_native(self._implementation.get_value))

    def setValue(self, value: int) -> _NativeHLAoctetPairBE:
        _call_native(self._implementation.set_value, _require_octet_pair(value))
        return self


class _NativeHLAoctetPairLE(_NativeDataElement, HLAoctetPairLE):
    def getValue(self) -> int:
        return int(_call_native(self._implementation.get_value))

    def setValue(self, value: int) -> _NativeHLAoctetPairLE:
        _call_native(self._implementation.set_value, _require_octet_pair(value))
        return self


class _NativeHLAopaqueData(_NativeDataElement, HLAopaqueData):
    def size(self) -> int:
        return int(_call_native(self._implementation.data_length))

    def get(self, index: int) -> int:
        return self.getValue()[_require_opaque_index(index, self.size())]

    def getValue(self) -> bytes:
        return bytes(_call_native(self._implementation.get_value))

    def setValue(self, value: BytesLike) -> None:
        _call_native(self._implementation.set_value, _require_opaque_data(value))


class _NativeHLAvariableArray(_NativeDataElement, HLAvariableArray):
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

    def size(self) -> int:
        return int(_call_native(self._implementation.size))

    def get(self, index: int) -> DataElement:
        index = _require_data_element_index(index, self.size())
        element = self._factory.createElement(index)
        if not isinstance(element, type(self._prototype)):
            raise TypeError(
                "factory returned an element that does not match the prototype"
            )
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
    def __init__(
        self,
        implementation: Any,
        factory: DataElementFactory,
        prototype: DataElement,
    ) -> None:
        super().__init__(implementation)
        self._factory = factory
        self._prototype = prototype

    def size(self) -> int:
        return int(_call_native(self._implementation.size))

    def set(self, index: int, dataElement: DataElement) -> None:
        index = _require_data_element_index(index, self.size())
        if not isinstance(dataElement, type(self._prototype)):
            raise TypeError("dataElement does not match the fixed-array prototype")
        _call_native(
            self._implementation.set_element,
            index,
            dataElement.toByteArray(),
            encoding_error=EncoderException,
        )

    def get(self, index: int) -> DataElement:
        index = _require_data_element_index(index, self.size())
        element = self._factory.createElement(index)
        if not isinstance(element, type(self._prototype)):
            raise TypeError(
                "factory returned an element that does not match the prototype"
            )
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
    """Native façade for a heterogeneous C++ ``HLAfixedRecord``.

    The C++ object owns copies of appended components.  Python keeps typed
    component façades so ``get`` can expose a normal public data element and
    synchronize it from the native record before returning it.
    """

    def __init__(self, implementation: Any) -> None:
        super().__init__(implementation)
        self._elements: list[DataElement] = []

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

    def _component(self, index: int) -> DataElement:
        index = _require_data_element_index(index, self.size())
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

    def get(self, index: int) -> DataElement:
        return self._component(index)

    def set(self, index: int, dataElement: DataElement) -> None:
        index = _require_data_element_index(index, self.size())
        if index >= len(self._elements):
            raise EncoderException("native fixed record component state is unavailable")
        if not isinstance(dataElement, type(self._elements[index])):
            raise TypeError(
                "dataElement does not match the fixed-record component type"
            )
        _call_native(
            self._implementation.set_element,
            index,
            dataElement._implementation,
            dataElement.toByteArray(),
            encoding_error=EncoderException,
        )
        self._elements[index].decode(dataElement.toByteArray())


class _NativeHLAvariantRecord(_NativeDataElement, HLAvariantRecord):
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
        if key in self._variants and not isinstance(
            dataElement, type(self._variants[key])
        ):
            raise TypeError("dataElement does not match the mapped variant type")
        method = (
            self._implementation.set_variant
            if key in self._variants
            else self._implementation.add_variant
        )
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


class _NativeHLAextendableVariantRecord(_NativeDataElement, DataElement):
    """Native-only façade for C++ ``HLAextendableVariantRecord``.

    IEEE 1516.1-2025's C++ encoding helper has no matching Java
    ``EncoderFactory`` surface in the verified Java API, so this deliberately
    remains an edition/provider-specific method on the native factory rather
    than part of the shared abstract Python contract.
    """

    def __init__(self, implementation: Any, discriminant: DataElement) -> None:
        super().__init__(implementation)
        self._discriminant = discriminant
        self._variants: dict[bytes, DataElement] = {}

    def _require_discriminant(self, discriminant: DataElement) -> bytes:
        if not isinstance(discriminant, type(self._discriminant)):
            raise TypeError(
                "discriminant does not match the extendable-variant prototype"
            )
        return discriminant.toByteArray()

    def addVariant(self, discriminant: DataElement, dataElement: DataElement) -> None:
        key = self._require_discriminant(discriminant)
        if not isinstance(dataElement, _NativeDataElement):
            raise TypeError("native extendable variants require native DataElements")
        if key in self._variants:
            raise EncoderException("extendable-variant discriminant is already mapped")
        _call_native(
            self._implementation.add_variant,
            self._discriminant._implementation,
            key,
            dataElement._implementation,
            dataElement.toByteArray(),
            encoding_error=EncoderException,
        )
        self._variants[key] = dataElement

    def setVariant(self, discriminant: DataElement, dataElement: DataElement) -> None:
        key = self._require_discriminant(discriminant)
        if not isinstance(dataElement, _NativeDataElement):
            raise TypeError("native extendable variants require native DataElements")
        existing = self._variants.get(key)
        if existing is None:
            raise EncoderException("extendable-variant discriminant is not mapped")
        if not isinstance(dataElement, type(existing)):
            raise TypeError(
                "dataElement does not match the mapped extendable variant type"
            )
        _call_native(
            self._implementation.set_variant,
            self._discriminant._implementation,
            key,
            dataElement._implementation,
            dataElement.toByteArray(),
            encoding_error=EncoderException,
        )
        existing.decode(dataElement.toByteArray())

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


class _NativeHLAunsignedInteger32BE(_NativeDataElement, HLAunsignedInteger32BE):
    def getValue(self) -> int:
        return int(_call_native(self._implementation.get_value))

    def setValue(self, value: int) -> _NativeHLAunsignedInteger32BE:
        _call_native(self._implementation.set_value, _require_unsigned_integer32(value))
        return self


class _NativeHLAboolean(_NativeDataElement, HLAboolean):
    def getValue(self) -> bool:
        return bool(_call_native(self._implementation.get_value))

    def setValue(self, value: bool) -> _NativeHLAboolean:
        _call_native(self._implementation.set_value, value)
        return self


class _NativeHLAunicodeString(_NativeDataElement, HLAunicodeString):
    def getValue(self) -> str:
        return str(_call_native(self._implementation.get_value))

    def setValue(self, value: str) -> _NativeHLAunicodeString:
        _call_native(self._implementation.set_value, str(value))
        return self


class _NativeEncoderFactory(EncoderFactory):
    """Java-shaped factory façade over Umbra's concrete C++ data elements."""

    def createHLAinteger32BE(self, value: int | None = None) -> HLAinteger32BE:
        implementation = (
            _call_native(_native.NativeHLAinteger32BE)
            if value is None
            else _call_native(_native.NativeHLAinteger32BE, _require_integer32(value))
        )
        return _NativeHLAinteger32BE(implementation)

    def createHLAinteger16BE(self, value: int | None = None) -> HLAinteger16BE:
        implementation = (
            _call_native(_native.NativeHLAinteger16BE)
            if value is None
            else _call_native(_native.NativeHLAinteger16BE, _require_integer16(value))
        )
        return _NativeHLAinteger16BE(implementation)

    def createHLAinteger16LE(self, value: int | None = None) -> HLAinteger16LE:
        implementation = (
            _call_native(_native.NativeHLAinteger16LE)
            if value is None
            else _call_native(
                _native.NativeHLAinteger16LE, _require_integer16_le(value)
            )
        )
        return _NativeHLAinteger16LE(implementation)

    def createHLAinteger32LE(self, value: int | None = None) -> HLAinteger32LE:
        implementation = (
            _call_native(_native.NativeHLAinteger32LE)
            if value is None
            else _call_native(
                _native.NativeHLAinteger32LE, _require_integer32_le(value)
            )
        )
        return _NativeHLAinteger32LE(implementation)

    def createHLAinteger64BE(self, value: int | None = None) -> HLAinteger64BE:
        implementation = (
            _call_native(_native.NativeHLAinteger64BE)
            if value is None
            else _call_native(_native.NativeHLAinteger64BE, _require_integer64(value))
        )
        return _NativeHLAinteger64BE(implementation)

    def createHLAinteger64LE(self, value: int | None = None) -> HLAinteger64LE:
        implementation = (
            _call_native(_native.NativeHLAinteger64LE)
            if value is None
            else _call_native(
                _native.NativeHLAinteger64LE, _require_integer64_le(value)
            )
        )
        return _NativeHLAinteger64LE(implementation)

    def createHLAfloat64BE(self, value: float | None = None) -> HLAfloat64BE:
        implementation = (
            _call_native(_native.NativeHLAfloat64BE)
            if value is None
            else _call_native(_native.NativeHLAfloat64BE, _require_float64(value))
        )
        return _NativeHLAfloat64BE(implementation)

    def createHLAfloat32BE(self, value: float | None = None) -> HLAfloat32BE:
        implementation = (
            _call_native(_native.NativeHLAfloat32BE)
            if value is None
            else _call_native(_native.NativeHLAfloat32BE, _require_float32(value))
        )
        return _NativeHLAfloat32BE(implementation)

    def createHLAfloat64LE(self, value: float | None = None) -> HLAfloat64LE:
        implementation = (
            _call_native(_native.NativeHLAfloat64LE)
            if value is None
            else _call_native(_native.NativeHLAfloat64LE, _require_float64_le(value))
        )
        return _NativeHLAfloat64LE(implementation)

    def createHLAfloat32LE(self, value: float | None = None) -> HLAfloat32LE:
        implementation = (
            _call_native(_native.NativeHLAfloat32LE)
            if value is None
            else _call_native(_native.NativeHLAfloat32LE, _require_float32_le(value))
        )
        return _NativeHLAfloat32LE(implementation)

    def createHLAunsignedInteger16BE(
        self, value: int | None = None
    ) -> HLAunsignedInteger16BE:
        implementation = (
            _call_native(_native.NativeHLAunsignedInteger16BE)
            if value is None
            else _call_native(
                _native.NativeHLAunsignedInteger16BE,
                _require_unsigned_integer16(value),
            )
        )
        return _NativeHLAunsignedInteger16BE(implementation)

    def createHLAunsignedInteger16LE(
        self, value: int | None = None
    ) -> HLAunsignedInteger16LE:
        implementation = (
            _call_native(_native.NativeHLAunsignedInteger16LE)
            if value is None
            else _call_native(
                _native.NativeHLAunsignedInteger16LE,
                _require_unsigned_integer16_le(value),
            )
        )
        return _NativeHLAunsignedInteger16LE(implementation)

    def createHLAunsignedInteger32LE(
        self, value: int | None = None
    ) -> HLAunsignedInteger32LE:
        implementation = (
            _call_native(_native.NativeHLAunsignedInteger32LE)
            if value is None
            else _call_native(
                _native.NativeHLAunsignedInteger32LE,
                _require_unsigned_integer32_le(value),
            )
        )
        return _NativeHLAunsignedInteger32LE(implementation)

    def createHLAunsignedInteger32BE(
        self, value: int | None = None
    ) -> HLAunsignedInteger32BE:
        implementation = (
            _call_native(_native.NativeHLAunsignedInteger32BE)
            if value is None
            else _call_native(
                _native.NativeHLAunsignedInteger32BE,
                _require_unsigned_integer32(value),
            )
        )
        return _NativeHLAunsignedInteger32BE(implementation)

    def createHLAunsignedInteger64BE(
        self, value: int | None = None
    ) -> HLAunsignedInteger64BE:
        implementation = (
            _call_native(_native.NativeHLAunsignedInteger64BE)
            if value is None
            else _call_native(
                _native.NativeHLAunsignedInteger64BE,
                _require_unsigned_integer64(value),
            )
        )
        return _NativeHLAunsignedInteger64BE(implementation)

    def createHLAunsignedInteger64LE(
        self, value: int | None = None
    ) -> HLAunsignedInteger64LE:
        implementation = (
            _call_native(_native.NativeHLAunsignedInteger64LE)
            if value is None
            else _call_native(
                _native.NativeHLAunsignedInteger64LE,
                _require_unsigned_integer64_le(value),
            )
        )
        return _NativeHLAunsignedInteger64LE(implementation)

    def createHLAbyte(self, value: int | None = None) -> HLAbyte:
        implementation = (
            _call_native(_native.NativeHLAbyte)
            if value is None
            else _call_native(_native.NativeHLAbyte, _require_byte(value))
        )
        return _NativeHLAbyte(implementation)

    def createHLAoctet(self, value: int | None = None) -> HLAoctet:
        implementation = (
            _call_native(_native.NativeHLAoctet)
            if value is None
            else _call_native(_native.NativeHLAoctet, _require_octet(value))
        )
        return _NativeHLAoctet(implementation)

    def createHLAASCIIchar(self, value: int | None = None) -> HLAASCIIchar:
        implementation = (
            _call_native(_native.NativeHLAASCIIchar)
            if value is None
            else _call_native(_native.NativeHLAASCIIchar, _require_ascii_char(value))
        )
        return _NativeHLAASCIIchar(implementation)

    def createHLAASCIIstring(self, value: str | None = None) -> HLAASCIIstring:
        implementation = (
            _call_native(_native.NativeHLAASCIIstring)
            if value is None
            else _call_native(
                _native.NativeHLAASCIIstring, _require_ascii_string(value)
            )
        )
        return _NativeHLAASCIIstring(implementation)

    def createHLAunicodeChar(self, value: int | None = None) -> HLAunicodeChar:
        implementation = (
            _call_native(_native.NativeHLAunicodeChar)
            if value is None
            else _call_native(
                _native.NativeHLAunicodeChar, _require_unicode_char(value)
            )
        )
        return _NativeHLAunicodeChar(implementation)

    def createHLAoctetPairBE(self, value: int | None = None) -> HLAoctetPairBE:
        implementation = (
            _call_native(_native.NativeHLAoctetPairBE)
            if value is None
            else _call_native(_native.NativeHLAoctetPairBE, _require_octet_pair(value))
        )
        return _NativeHLAoctetPairBE(implementation)

    def createHLAoctetPairLE(self, value: int | None = None) -> HLAoctetPairLE:
        implementation = (
            _call_native(_native.NativeHLAoctetPairLE)
            if value is None
            else _call_native(_native.NativeHLAoctetPairLE, _require_octet_pair(value))
        )
        return _NativeHLAoctetPairLE(implementation)

    def createHLAopaqueData(self, value: BytesLike | None = None) -> HLAopaqueData:
        implementation = (
            _call_native(_native.NativeHLAopaqueData)
            if value is None
            else _call_native(_native.NativeHLAopaqueData, _require_opaque_data(value))
        )
        return _NativeHLAopaqueData(implementation)

    def createHLAvariableArray(
        self, factory: DataElementFactory, *elements: DataElement
    ) -> HLAvariableArray:
        factory = _require_data_element_factory(factory)
        prototype = factory.createElement(0)
        if not isinstance(prototype, _NativeDataElement):
            raise TypeError(
                "native variable arrays require a native DataElement factory"
            )
        implementation = _call_native(
            _native.NativeHLAvariableArray,
            prototype._implementation,
        )
        result = _NativeHLAvariableArray(implementation, factory, prototype)
        for element in elements:
            result.addElement(element)
        return result

    def createHLAfixedArray(
        self, factory: DataElementFactory, size: int
    ) -> HLAfixedArray:
        factory = _require_data_element_factory(factory)
        size = _require_data_element_size(size)
        prototype = factory.createElement(0)
        if not isinstance(prototype, _NativeDataElement):
            raise TypeError("native fixed arrays require a native DataElement factory")
        implementation = _call_native(
            _native.NativeHLAfixedArray,
            prototype._implementation,
            size,
        )
        return _NativeHLAfixedArray(implementation, factory, prototype)

    def createHLAfixedRecord(self) -> HLAfixedRecord:
        return _NativeHLAfixedRecord(_call_native(_native.NativeHLAfixedRecord))

    def createHLAvariantRecord(
        self, discriminantPrototype: DataElement
    ) -> HLAvariantRecord:
        if not isinstance(discriminantPrototype, _NativeDataElement):
            raise TypeError(
                "native variant records require a native discriminant prototype"
            )
        implementation = _call_native(
            _native.NativeHLAvariantRecord,
            discriminantPrototype._implementation,
        )
        return _NativeHLAvariantRecord(implementation, discriminantPrototype)

    def createHLAextendableVariantRecord(
        self, discriminantPrototype: DataElement
    ) -> DataElement:
        """Create the native-only C++ extendable-variant encoding helper."""
        if not isinstance(discriminantPrototype, _NativeDataElement):
            raise TypeError(
                "native extendable variants require a native discriminant prototype"
            )
        implementation = _call_native(
            _native.NativeHLAextendableVariantRecord,
            discriminantPrototype._implementation,
        )
        return _NativeHLAextendableVariantRecord(implementation, discriminantPrototype)

    def createHLAboolean(self, value: bool | None = None) -> HLAboolean:
        implementation = (
            _call_native(_native.NativeHLAboolean)
            if value is None
            else _call_native(_native.NativeHLAboolean, value)
        )
        return _NativeHLAboolean(implementation)

    def createHLAunicodeString(self, value: str | None = None) -> HLAunicodeString:
        implementation = (
            _call_native(_native.NativeHLAunicodeString)
            if value is None
            else _call_native(_native.NativeHLAunicodeString, str(value))
        )
        return _NativeHLAunicodeString(implementation)
