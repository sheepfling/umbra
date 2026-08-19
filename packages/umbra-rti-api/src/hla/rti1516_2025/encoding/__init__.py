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


def _require_unsigned_integer32(value: int) -> int:
    if type(value) is not int:
        raise TypeError("HLAunsignedInteger32BE values must be int")
    if not 0 <= value < 2**32:
        raise ValueError("HLAunsignedInteger32BE value is outside the unsigned 32-bit range")
    return value


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


class HLAinteger32BE(DataElement):
    """The standard signed 32-bit big-endian basic representation."""

    @abstractmethod
    def getValue(self) -> int:
        """Return the signed 32-bit value."""

    @abstractmethod
    def setValue(self, value: int) -> Self:
        """Set the signed 32-bit value and return this element."""


class HLAunsignedInteger32BE(DataElement):
    """The standard unsigned 32-bit big-endian basic representation."""

    @abstractmethod
    def getValue(self) -> int:
        """Return the unsigned 32-bit value."""

    @abstractmethod
    def setValue(self, value: int) -> Self:
        """Set the unsigned 32-bit value and return this element."""


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
    def createHLAunsignedInteger32BE(self, value: int | None = None) -> HLAunsignedInteger32BE:
        """Create an unsigned 32-bit big-endian element."""

    @abstractmethod
    def createHLAboolean(self, value: bool | None = None) -> HLAboolean:
        """Create a four-octet boolean element."""

    @abstractmethod
    def createHLAunicodeString(self, value: str | None = None) -> HLAunicodeString:
        """Create a UTF-16BE Unicode string element."""
