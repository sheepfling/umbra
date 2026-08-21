"""Provider-owned IEEE 1516.1-2025 basic data elements.

The Java API exposes these values through ``encoding.EncoderFactory``. The
C++ API constructs its concrete data elements directly. This package presents
one public contract without recreating any wire encoding in Python: a provider
must delegate each operation to its native or Java data-element object.
"""

from __future__ import annotations

from abc import ABC, abstractmethod
from typing import Self


def _require_integer32(value: int) -> int:
    if type(value) is not int:
        raise TypeError("HLAinteger32BE values must be int")
    if not -(2**31) <= value < 2**31:
        raise ValueError("HLAinteger32BE value is outside the signed 32-bit range")
    return value


def _require_integer16(value: int) -> int:
    if type(value) is not int:
        raise TypeError("HLAinteger16BE values must be int")
    if not -(2**15) <= value < 2**15:
        raise ValueError("HLAinteger16BE value is outside the signed 16-bit range")
    return value


def _require_integer16_le(value: int) -> int:
    if type(value) is not int:
        raise TypeError("HLAinteger16LE values must be int")
    if not -(2**15) <= value < 2**15:
        raise ValueError("HLAinteger16LE value is outside the signed 16-bit range")
    return value


def _require_integer32_le(value: int) -> int:
    if type(value) is not int:
        raise TypeError("HLAinteger32LE values must be int")
    if not -(2**31) <= value < 2**31:
        raise ValueError("HLAinteger32LE value is outside the signed 32-bit range")
    return value


def _require_integer64(value: int) -> int:
    if type(value) is not int:
        raise TypeError("HLAinteger64BE values must be int")
    if not -(2**63) <= value < 2**63:
        raise ValueError("HLAinteger64BE value is outside the signed 64-bit range")
    return value


def _require_integer64_le(value: int) -> int:
    if type(value) is not int:
        raise TypeError("HLAinteger64LE values must be int")
    if not -(2**63) <= value < 2**63:
        raise ValueError("HLAinteger64LE value is outside the signed 64-bit range")
    return value


def _require_unsigned_integer32_le(value: int) -> int:
    if type(value) is not int:
        raise TypeError("HLAunsignedInteger32LE values must be int")
    if not 0 <= value < 2**32:
        raise ValueError("HLAunsignedInteger32LE value is outside the unsigned 32-bit range")
    return value


def _require_float64(value: float | int) -> float:
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise TypeError("HLAfloat64BE values must be real numbers")
    return float(value)


def _require_float64_le(value: float | int) -> float:
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise TypeError("HLAfloat64LE values must be real numbers")
    return float(value)


def _require_float32(value: float | int) -> float:
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise TypeError("HLAfloat32BE values must be real numbers")
    return float(value)


def _require_float32_le(value: float | int) -> float:
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise TypeError("HLAfloat32LE values must be real numbers")
    return float(value)


def _require_unsigned_integer32(value: int) -> int:
    if type(value) is not int:
        raise TypeError("HLAunsignedInteger32BE values must be int")
    if not 0 <= value < 2**32:
        raise ValueError("HLAunsignedInteger32BE value is outside the unsigned 32-bit range")
    return value


def _require_unsigned_integer16(value: int) -> int:
    if type(value) is not int:
        raise TypeError("HLAunsignedInteger16BE values must be int")
    if not 0 <= value < 2**16:
        raise ValueError("HLAunsignedInteger16BE value is outside the unsigned 16-bit range")
    return value


def _require_unsigned_integer16_le(value: int) -> int:
    if type(value) is not int:
        raise TypeError("HLAunsignedInteger16LE values must be int")
    if not 0 <= value < 2**16:
        raise ValueError("HLAunsignedInteger16LE value is outside the unsigned 16-bit range")
    return value


def _require_unsigned_integer64(value: int) -> int:
    if type(value) is not int:
        raise TypeError("HLAunsignedInteger64BE values must be int")
    if not 0 <= value < 2**64:
        raise ValueError("HLAunsignedInteger64BE value is outside the unsigned 64-bit range")
    return value


def _require_unsigned_integer64_le(value: int) -> int:
    if type(value) is not int:
        raise TypeError("HLAunsignedInteger64LE values must be int")
    if not 0 <= value < 2**64:
        raise ValueError("HLAunsignedInteger64LE value is outside the unsigned 64-bit range")
    return value


def _require_byte(value: int) -> int:
    if type(value) is not int:
        raise TypeError("HLAbyte values must be int")
    if not 0 <= value < 2**8:
        raise ValueError("HLAbyte value is outside the unsigned 8-bit range")
    return value


def _require_octet(value: int) -> int:
    if type(value) is not int:
        raise TypeError("HLAoctet values must be int")
    if not 0 <= value < 2**8:
        raise ValueError("HLAoctet value is outside the unsigned 8-bit range")
    return value


def _require_ascii_char(value: int) -> int:
    if type(value) is not int:
        raise TypeError("HLAASCIIchar values must be int")
    if not 0 <= value <= 0x7F:
        raise ValueError("HLAASCIIchar value is outside the ASCII range")
    return value


def _require_unicode_char(value: int) -> int:
    if type(value) is not int:
        raise TypeError("HLAunicodeChar values must be int")
    if not 0 <= value <= 0xFFFF:
        raise ValueError("HLAunicodeChar value is outside the UTF-16 code-unit range")
    return value


def _require_ascii_string(value: str) -> str:
    if not isinstance(value, str):
        raise TypeError("HLAASCIIstring values must be str")
    if any(ord(character) > 0x7F for character in value):
        raise ValueError("HLAASCIIstring value contains a non-ASCII character")
    return value


def _require_octet_pair(value: int) -> int:
    if type(value) is not int:
        raise TypeError("HLAoctetPair values must be int")
    if not 0 <= value < 2**16:
        raise ValueError("HLAoctetPair value is outside the unsigned 16-bit range")
    return value


def _require_opaque_data(value: bytes | bytearray | memoryview) -> bytes:
    if not isinstance(value, (bytes, bytearray, memoryview)):
        raise TypeError("HLAopaqueData values must be bytes-like")
    return bytes(value)


def _require_opaque_index(index: int, size: int) -> int:
    if type(index) is not int:
        raise TypeError("HLAopaqueData index must be int")
    if not 0 <= index < size:
        raise IndexError("HLAopaqueData index is outside the data range")
    return index


def _require_data_element_index(index: int, size: int) -> int:
    if type(index) is not int:
        raise TypeError("data-element index must be int")
    if not 0 <= index < size:
        raise IndexError("data-element index is outside the array range")
    return index


def _require_data_element_size(size: int) -> int:
    if type(size) is not int:
        raise TypeError("data-element array size must be int")
    if size < 0:
        raise ValueError("data-element array size must be non-negative")
    return size


class EncoderException(Exception):
    """The provider could not encode a data element."""


class DecoderException(EncoderException):
    """The supplied octets are not a valid encoding for a data element."""


class DataElement(ABC):
    """Shared Python projection of the standard Java ``DataElement`` API."""

    @abstractmethod
    def getOctetBoundary(self) -> int:
        """Return this element's standard octet boundary."""

    @abstractmethod
    def getEncodedLength(self) -> int:
        """Return the length of the provider-produced encoding."""

    @abstractmethod
    def toByteArray(self) -> bytes:
        """Return a copied provider-produced encoding."""

    def encode(self) -> bytes:
        """Python convenience alias for Java's zero-argument ``encode`` result."""

        return self.toByteArray()

    @abstractmethod
    def decode(self, bytes_: bytes) -> Self:
        """Decode provider-supplied octets into this same element."""


class DataElementFactory(ABC):
    """Java-shaped factory used to seed and decode constructed elements."""

    @abstractmethod
    def createElement(self, index: int) -> DataElement:
        """Create a fresh element for the requested array index."""


def _require_data_element_factory(value: DataElementFactory) -> DataElementFactory:
    if not isinstance(value, DataElementFactory):
        raise TypeError("factory must be a DataElementFactory")
    return value


class HLAinteger32BE(DataElement):
    """The standard signed 32-bit big-endian basic representation."""

    @abstractmethod
    def getValue(self) -> int:
        """Return the signed 32-bit value."""

    @abstractmethod
    def setValue(self, value: int) -> Self:
        """Set the signed 32-bit value and return this element."""


class HLAinteger16BE(DataElement):
    """The standard signed 16-bit big-endian basic representation."""

    @abstractmethod
    def getValue(self) -> int:
        """Return the signed 16-bit value."""

    @abstractmethod
    def setValue(self, value: int) -> Self:
        """Set the signed 16-bit value and return this element."""


class HLAinteger16LE(DataElement):
    """The standard signed 16-bit little-endian basic representation."""

    @abstractmethod
    def getValue(self) -> int:
        """Return the signed 16-bit value."""

    @abstractmethod
    def setValue(self, value: int) -> Self:
        """Set the signed 16-bit value and return this element."""


class HLAinteger32LE(DataElement):
    """The standard signed 32-bit little-endian basic representation."""

    @abstractmethod
    def getValue(self) -> int:
        """Return the signed 32-bit value."""

    @abstractmethod
    def setValue(self, value: int) -> Self:
        """Set the signed 32-bit value and return this element."""


class HLAinteger64BE(DataElement):
    """The standard signed 64-bit big-endian basic representation."""

    @abstractmethod
    def getValue(self) -> int:
        """Return the signed 64-bit value."""

    @abstractmethod
    def setValue(self, value: int) -> Self:
        """Set the signed 64-bit value and return this element."""


class HLAinteger64LE(DataElement):
    """The standard signed 64-bit little-endian basic representation."""

    @abstractmethod
    def getValue(self) -> int:
        """Return the signed 64-bit value."""

    @abstractmethod
    def setValue(self, value: int) -> Self:
        """Set the signed 64-bit value and return this element."""


class HLAfloat64BE(DataElement):
    """The standard IEEE-754 binary64 big-endian representation."""

    @abstractmethod
    def getValue(self) -> float:
        """Return the binary64 value."""

    @abstractmethod
    def setValue(self, value: float | int) -> Self:
        """Set the binary64 value and return this element."""


class HLAfloat32BE(DataElement):
    """The standard IEEE-754 binary32 big-endian representation."""

    @abstractmethod
    def getValue(self) -> float:
        """Return the binary32 value."""

    @abstractmethod
    def setValue(self, value: float | int) -> Self:
        """Set the binary32 value and return this element."""


class HLAfloat64LE(DataElement):
    """The standard IEEE-754 binary64 little-endian representation."""

    @abstractmethod
    def getValue(self) -> float:
        """Return the binary64 value."""

    @abstractmethod
    def setValue(self, value: float | int) -> Self:
        """Set the binary64 value and return this element."""


class HLAfloat32LE(DataElement):
    """The standard IEEE-754 binary32 little-endian representation."""

    @abstractmethod
    def getValue(self) -> float:
        """Return the binary32 value."""

    @abstractmethod
    def setValue(self, value: float | int) -> Self:
        """Set the binary32 value and return this element."""


class HLAunsignedInteger16BE(DataElement):
    """The standard unsigned 16-bit big-endian basic representation."""

    @abstractmethod
    def getValue(self) -> int:
        """Return the unsigned 16-bit value."""

    @abstractmethod
    def setValue(self, value: int) -> Self:
        """Set the unsigned 16-bit value and return this element."""


class HLAunsignedInteger16LE(DataElement):
    """The standard unsigned 16-bit little-endian basic representation."""

    @abstractmethod
    def getValue(self) -> int:
        """Return the unsigned 16-bit value."""

    @abstractmethod
    def setValue(self, value: int) -> Self:
        """Set the unsigned 16-bit value and return this element."""


class HLAunsignedInteger32LE(DataElement):
    """The standard unsigned 32-bit little-endian basic representation."""

    @abstractmethod
    def getValue(self) -> int:
        """Return the unsigned 32-bit value."""

    @abstractmethod
    def setValue(self, value: int) -> Self:
        """Set the unsigned 32-bit value and return this element."""


class HLAunsignedInteger32BE(DataElement):
    """The standard unsigned 32-bit big-endian basic representation."""

    @abstractmethod
    def getValue(self) -> int:
        """Return the unsigned 32-bit value."""

    @abstractmethod
    def setValue(self, value: int) -> Self:
        """Set the unsigned 32-bit value and return this element."""


class HLAunsignedInteger64BE(DataElement):
    """The standard unsigned 64-bit big-endian basic representation."""

    @abstractmethod
    def getValue(self) -> int:
        """Return the unsigned 64-bit value."""

    @abstractmethod
    def setValue(self, value: int) -> Self:
        """Set the unsigned 64-bit value and return this element."""


class HLAunsignedInteger64LE(DataElement):
    """The standard unsigned 64-bit little-endian basic representation."""

    @abstractmethod
    def getValue(self) -> int:
        """Return the unsigned 64-bit value."""

    @abstractmethod
    def setValue(self, value: int) -> Self:
        """Set the unsigned 64-bit value and return this element."""


class HLAbyte(DataElement):
    """The standard one-octet HLA byte representation."""

    @abstractmethod
    def getValue(self) -> int:
        """Return the byte as an unsigned Python integer."""

    @abstractmethod
    def setValue(self, value: int) -> Self:
        """Set the byte and return this element."""


class HLAoctet(DataElement):
    """The standard one-octet HLA octet representation."""

    @abstractmethod
    def getValue(self) -> int:
        """Return the octet as an unsigned Python integer."""

    @abstractmethod
    def setValue(self, value: int) -> Self:
        """Set the octet and return this element."""


class HLAASCIIchar(DataElement):
    """The standard one-octet ASCII character representation."""

    @abstractmethod
    def getValue(self) -> int:
        """Return the ASCII code unit as a Python integer."""

    @abstractmethod
    def setValue(self, value: int) -> Self:
        """Set the ASCII code unit and return this element."""


class HLAASCIIstring(DataElement):
    """The standard element-count-prefixed ASCII string representation."""

    @abstractmethod
    def getValue(self) -> str:
        """Return the ASCII string value."""

    @abstractmethod
    def setValue(self, value: str) -> Self:
        """Set the ASCII string value and return this element."""


class HLAunicodeChar(DataElement):
    """The standard one-UTF-16-code-unit Unicode character representation."""

    @abstractmethod
    def getValue(self) -> int:
        """Return the UTF-16 code unit as a Python integer."""

    @abstractmethod
    def setValue(self, value: int) -> Self:
        """Set the UTF-16 code unit and return this element."""


class HLAoctetPairBE(DataElement):
    """The standard two-octet big-endian pair representation."""

    @abstractmethod
    def getValue(self) -> int:
        """Return the pair's 16-bit value."""

    @abstractmethod
    def setValue(self, value: int) -> Self:
        """Set the pair's 16-bit value and return this element."""


class HLAoctetPairLE(DataElement):
    """The standard two-octet little-endian pair representation."""

    @abstractmethod
    def getValue(self) -> int:
        """Return the pair's 16-bit value."""

    @abstractmethod
    def setValue(self, value: int) -> Self:
        """Set the pair's 16-bit value and return this element."""


class HLAopaqueData(DataElement):
    """The standard HLAbyte variable-array opaque-data representation."""

    @abstractmethod
    def size(self) -> int:
        """Return the number of contained bytes."""

    @abstractmethod
    def get(self, index: int) -> int:
        """Return one contained byte as an unsigned Python integer."""

    @abstractmethod
    def getValue(self) -> bytes:
        """Return a copied byte value."""

    @abstractmethod
    def setValue(self, value: bytes | bytearray | memoryview) -> None:
        """Replace the contained bytes with a copied value."""


class HLAvariableArray(DataElement):
    """The standard dynamic array of one provider-owned element type."""

    @abstractmethod
    def addElement(self, dataElement: DataElement) -> None:
        """Append a copied element matching the array's factory prototype."""

    @abstractmethod
    def size(self) -> int:
        """Return the number of contained elements."""

    @abstractmethod
    def get(self, index: int) -> DataElement:
        """Return a provider-owned decoded element at ``index``."""

    def __iter__(self):
        for index in range(self.size()):
            yield self.get(index)


class HLAfixedArray(DataElement):
    """The standard fixed-cardinality array of one element type."""

    @abstractmethod
    def size(self) -> int:
        """Return the fixed number of contained elements."""

    @abstractmethod
    def get(self, index: int) -> DataElement:
        """Return the provider-owned element at ``index``."""

    @abstractmethod
    def set(self, index: int, dataElement: DataElement) -> None:
        """Replace an element through the portable copied-value convenience."""

    def __iter__(self):
        for index in range(self.size()):
            yield self.get(index)


class HLAfixedRecord(DataElement):
    """The standard ordered record of independently typed elements.

    A fixed record has no prototype factory: each appended component carries
    its own type and octet boundary.  Providers copy an element at the
    ``appendElement``/``set`` boundary, matching the C++ API; ``get`` returns
    a provider-owned component view.
    """

    @abstractmethod
    def appendElement(self, dataElement: DataElement) -> None:
        """Append a copied data element to the record."""

    @abstractmethod
    def size(self) -> int:
        """Return the number of record components."""

    @abstractmethod
    def get(self, index: int) -> DataElement:
        """Return the provider-owned component at ``index``."""

    @abstractmethod
    def set(self, index: int, dataElement: DataElement) -> None:
        """Replace a component with a same-typed copied value."""

    def __iter__(self):
        for index in range(self.size()):
            yield self.get(index)


class HLAvariantRecord(DataElement):
    """The standard discriminated alternative record.

    ``setVariant`` registers or replaces the copied value associated with a
    discriminant, while ``setDiscriminant`` selects the active alternative.
    An unmapped discriminant is valid on the wire and makes ``getValue``
    return ``None``.
    """

    @abstractmethod
    def setVariant(self, discriminant: DataElement, dataElement: DataElement) -> None:
        """Copy/register the value associated with ``discriminant``."""

    @abstractmethod
    def setDiscriminant(self, discriminant: DataElement) -> None:
        """Select the active discriminant."""

    @abstractmethod
    def getDiscriminant(self) -> DataElement:
        """Return the provider-owned active discriminant."""

    @abstractmethod
    def getValue(self) -> DataElement | None:
        """Return the active alternative, or ``None`` when unmapped."""


class HLAboolean(DataElement):
    """The standard four-octet HLA boolean representation."""

    @abstractmethod
    def getValue(self) -> bool:
        """Return the boolean value."""

    @abstractmethod
    def setValue(self, value: bool) -> Self:
        """Set the boolean value and return this element."""


class HLAunicodeString(DataElement):
    """The standard UTF-16BE, element-count-prefixed string representation."""

    @abstractmethod
    def getValue(self) -> str:
        """Return the Unicode string value."""

    @abstractmethod
    def setValue(self, value: str) -> Self:
        """Set the Unicode string value and return this element."""


class EncoderFactory(ABC):
    """Construct provider-owned basic data elements.

    Each optional argument adapts the Java zero-argument and value overloads
    to one Python method. Passing ``None`` selects the zero-argument form.
    """

    @abstractmethod
    def createHLAinteger32BE(self, value: int | None = None) -> HLAinteger32BE:
        """Create a signed 32-bit big-endian element."""

    @abstractmethod
    def createHLAinteger16BE(self, value: int | None = None) -> HLAinteger16BE:
        """Create a signed 16-bit big-endian element."""

    @abstractmethod
    def createHLAinteger16LE(self, value: int | None = None) -> HLAinteger16LE:
        """Create a signed 16-bit little-endian element."""

    @abstractmethod
    def createHLAinteger32LE(self, value: int | None = None) -> HLAinteger32LE:
        """Create a signed 32-bit little-endian element."""

    @abstractmethod
    def createHLAinteger64BE(self, value: int | None = None) -> HLAinteger64BE:
        """Create a signed 64-bit big-endian element."""

    @abstractmethod
    def createHLAinteger64LE(self, value: int | None = None) -> HLAinteger64LE:
        """Create a signed 64-bit little-endian element."""

    @abstractmethod
    def createHLAfloat64BE(self, value: float | int | None = None) -> HLAfloat64BE:
        """Create an IEEE-754 binary64 big-endian element."""

    @abstractmethod
    def createHLAfloat32BE(self, value: float | int | None = None) -> HLAfloat32BE:
        """Create an IEEE-754 binary32 big-endian element."""

    @abstractmethod
    def createHLAfloat64LE(self, value: float | int | None = None) -> HLAfloat64LE:
        """Create an IEEE-754 binary64 little-endian element."""

    @abstractmethod
    def createHLAfloat32LE(self, value: float | int | None = None) -> HLAfloat32LE:
        """Create an IEEE-754 binary32 little-endian element."""

    @abstractmethod
    def createHLAunsignedInteger16BE(
        self, value: int | None = None
    ) -> HLAunsignedInteger16BE:
        """Create an unsigned 16-bit big-endian element."""

    @abstractmethod
    def createHLAunsignedInteger16LE(
        self, value: int | None = None
    ) -> HLAunsignedInteger16LE:
        """Create an unsigned 16-bit little-endian element."""

    @abstractmethod
    def createHLAunsignedInteger32LE(
        self, value: int | None = None
    ) -> HLAunsignedInteger32LE:
        """Create an unsigned 32-bit little-endian element."""

    @abstractmethod
    def createHLAunsignedInteger32BE(self, value: int | None = None) -> HLAunsignedInteger32BE:
        """Create an unsigned 32-bit big-endian element."""

    @abstractmethod
    def createHLAunsignedInteger64BE(
        self, value: int | None = None
    ) -> HLAunsignedInteger64BE:
        """Create an unsigned 64-bit big-endian element."""

    @abstractmethod
    def createHLAunsignedInteger64LE(
        self, value: int | None = None
    ) -> HLAunsignedInteger64LE:
        """Create an unsigned 64-bit little-endian element."""

    @abstractmethod
    def createHLAbyte(self, value: int | None = None) -> HLAbyte:
        """Create a one-octet byte element."""

    @abstractmethod
    def createHLAoctet(self, value: int | None = None) -> HLAoctet:
        """Create a one-octet octet element."""

    @abstractmethod
    def createHLAASCIIchar(self, value: int | None = None) -> HLAASCIIchar:
        """Create a one-octet ASCII character element."""

    @abstractmethod
    def createHLAASCIIstring(self, value: str | None = None) -> HLAASCIIstring:
        """Create an element-count-prefixed ASCII string element."""

    @abstractmethod
    def createHLAunicodeChar(self, value: int | None = None) -> HLAunicodeChar:
        """Create a one-UTF-16-code-unit Unicode character element."""

    @abstractmethod
    def createHLAoctetPairBE(self, value: int | None = None) -> HLAoctetPairBE:
        """Create a two-octet big-endian pair element."""

    @abstractmethod
    def createHLAoctetPairLE(self, value: int | None = None) -> HLAoctetPairLE:
        """Create a two-octet little-endian pair element."""

    @abstractmethod
    def createHLAopaqueData(
        self, value: bytes | bytearray | memoryview | None = None
    ) -> HLAopaqueData:
        """Create an HLAbyte variable-array opaque-data element."""

    @abstractmethod
    def createHLAvariableArray(
        self, factory: DataElementFactory, *elements: DataElement
    ) -> HLAvariableArray:
        """Create a variable array using the standard element factory and elements."""

    @abstractmethod
    def createHLAfixedArray(
        self, factory: DataElementFactory, size: int
    ) -> HLAfixedArray:
        """Create a fixed array using the standard element factory and size."""

    @abstractmethod
    def createHLAfixedRecord(self) -> HLAfixedRecord:
        """Create an empty fixed record."""

    @abstractmethod
    def createHLAvariantRecord(
        self, discriminantPrototype: DataElement
    ) -> HLAvariantRecord:
        """Create a variant record using the supplied discriminant prototype."""

    @abstractmethod
    def createHLAboolean(self, value: bool | None = None) -> HLAboolean:
        """Create a four-octet boolean element."""

    @abstractmethod
    def createHLAunicodeString(self, value: str | None = None) -> HLAunicodeString:
        """Create a UTF-16BE Unicode string element."""
