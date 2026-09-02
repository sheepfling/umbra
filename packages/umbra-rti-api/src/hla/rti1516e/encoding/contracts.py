"""Provider-neutral IEEE 1516.1-2010 encoding contracts."""

from __future__ import annotations

from abc import ABC, abstractmethod
from typing import Iterable, TypeVar

from ..byte_types import BytesLike, WritableBytes, copy_bytes


_ScalarSelf = TypeVar("_ScalarSelf", bound="_ScalarDataElement")


class ByteWrapper:
    """Small Python equivalent of the standard Java ``ByteWrapper`` helper."""

    def __init__(self, buffer: BytesLike | int = b"", offset: int = 0, length: int | None = None) -> None:
        if isinstance(buffer, int):
            buffer = bytes(buffer)
        self.reassign(buffer, offset, len(buffer) - offset if length is None else length)

    def reassign(self, buffer: BytesLike, offset: int = 0, length: int | None = None) -> None:
        data = buffer if isinstance(buffer, bytearray) else bytearray(buffer)
        length = len(data) - offset if length is None else length
        if offset < 0 or length < 0 or offset + length > len(data):
            raise IndexError("ByteWrapper segment is outside the backing buffer")
        self._buffer = data
        self._offset = offset
        self._pos = offset
        self._limit = offset + length

    def reset(self) -> None:
        self._pos = self._offset

    def getInt(self) -> int:
        """Read a big-endian signed 32-bit integer, like the Java helper."""

        self.verify(4)
        value = int.from_bytes(self._buffer[self._pos : self._pos + 4], "big", signed=True)
        self._pos += 4
        return value

    def putInt(self, value: int) -> None:
        """Write a big-endian signed 32-bit integer, like the Java helper."""

        self.verify(4)
        self._buffer[self._pos : self._pos + 4] = int(value).to_bytes(4, "big", signed=True)
        self._pos += 4

    def remaining(self) -> int:
        return self._limit - self._pos

    def getBuffer(self) -> bytearray:
        return self._buffer

    def array(self) -> bytearray:
        return self._buffer

    def position(self) -> int:
        return self._pos

    def setPosition(self, position: int) -> None:
        if position < self._offset or position > self._limit:
            raise IndexError(position)
        self._pos = position

    def verify(self, length: int) -> None:
        if length < 0 or self._pos + length > self._limit:
            raise IndexError("ByteWrapper does not contain enough data")

    def get(self, destination: int | bytearray | memoryview | None = None) -> int | bytes | None:
        """Implement Java's ``get()``, ``get(byte[])`` and a byte-count helper.

        Java overloads are represented by one Python method.  A no-argument
        call returns an unsigned octet; an integer reads that many bytes; and
        a mutable bytes-like destination is filled in-place.
        """

        if destination is None:
            self.verify(1)
            value = int(self._buffer[self._pos]) & 0xFF
            self._pos += 1
            return value
        if isinstance(destination, int):
            self.verify(destination)
            value = bytes(self._buffer[self._pos : self._pos + destination])
            self._pos += destination
            return value
        length = len(destination)
        self.verify(length)
        destination[:] = self._buffer[self._pos : self._pos + length]
        self._pos += length
        return None

    def put(
        self,
        value: int | BytesLike,
        offset: int = 0,
        count: int | None = None,
    ) -> None:
        """Implement Java's ``put(int)``, ``put(byte[])`` and slice overload."""

        if isinstance(value, int):
            self.verify(1)
            self._buffer[self._pos] = value & 0xFF
            self._pos += 1
            return
        data = copy_bytes(value)
        if count is None:
            count = len(data) - offset
        if offset < 0 or count < 0 or offset + count > len(data):
            raise IndexError("source slice is outside the backing buffer")
        self.verify(count)
        self._buffer[self._pos : self._pos + count] = data[offset : offset + count]
        self._pos += count

    def getPos(self) -> int:
        return self._pos

    def advance(self, count: int) -> None:
        self.verify(count)
        self._pos += count

    def align(self, alignment: int) -> None:
        if alignment <= 0:
            raise ValueError("alignment must be positive")
        while (self._pos - self._offset) % alignment:
            self.advance(1)

    def slice(self, length: int | None = None) -> "ByteWrapper":
        if length is None:
            # Match the Java no-argument overload: it starts at the current
            # position and extends to the end of the backing array, rather
            # than inheriting this wrapper's bounded segment limit.
            length = len(self._buffer) - self._pos
            return ByteWrapper(self._buffer, self._pos, length)
        self.verify(length)
        return ByteWrapper(self._buffer, self._pos, length)

    def toString(self) -> str:
        return (
            f"ByteWrapper{{_offset={self._offset}, _pos={self._pos}, "
            f"_limit={self._limit}, _buffer={self._buffer!r}}}"
        )

    def __str__(self) -> str:
        return self.toString()


class DataElement(ABC):
    @abstractmethod
    def getOctetBoundary(self) -> int:
        raise NotImplementedError

    @abstractmethod
    def encode(self, byteWrapper: ByteWrapper) -> None:
        raise NotImplementedError

    @abstractmethod
    def getEncodedLength(self) -> int:
        raise NotImplementedError

    def toByteArray(self) -> bytes:
        wrapper = ByteWrapper(self.getEncodedLength())
        self.encode(wrapper)
        return bytes(wrapper.getBuffer())

    @abstractmethod
    def decode(self, value: ByteWrapper | BytesLike) -> None:
        raise NotImplementedError


class DataElementFactory(ABC):
    @abstractmethod
    def createElement(self, index: int) -> DataElement:
        raise NotImplementedError


class EncoderException(Exception):
    """The provider could not encode a data element."""


class DecoderException(Exception):
    """The supplied octets are not a valid data-element encoding."""


class _ScalarDataElement(DataElement, ABC):
    """Common abstract shape for the scalar 2010 encoder interfaces."""

    @abstractmethod
    def getValue(self) -> object:
        raise NotImplementedError

    @abstractmethod
    def setValue(self: _ScalarSelf, value: object) -> _ScalarSelf:
        raise NotImplementedError


class HLAinteger16BE(_ScalarDataElement):
    pass


class HLAinteger16LE(_ScalarDataElement):
    pass


class HLAinteger32BE(_ScalarDataElement):
    pass


class HLAinteger32LE(_ScalarDataElement):
    pass


class HLAinteger64BE(_ScalarDataElement):
    pass


class HLAinteger64LE(_ScalarDataElement):
    pass


class HLAfloat32BE(_ScalarDataElement):
    pass


class HLAfloat32LE(_ScalarDataElement):
    pass


class HLAfloat64BE(_ScalarDataElement):
    pass


class HLAfloat64LE(_ScalarDataElement):
    pass


class HLAbyte(_ScalarDataElement):
    pass


class HLAoctet(_ScalarDataElement):
    pass


class HLAASCIIchar(_ScalarDataElement):
    pass


class HLAASCIIstring(_ScalarDataElement):
    pass


class HLAunicodeChar(_ScalarDataElement):
    pass


class HLAoctetPairBE(_ScalarDataElement):
    pass


class HLAoctetPairLE(_ScalarDataElement):
    pass


class HLAboolean(_ScalarDataElement):
    pass


class HLAunicodeString(_ScalarDataElement):
    pass


class HLAopaqueData(DataElement, ABC):
    @abstractmethod
    def size(self) -> int:
        raise NotImplementedError

    @abstractmethod
    def get(self, index: int) -> int:
        raise NotImplementedError

    @abstractmethod
    def getValue(self) -> bytes:
        raise NotImplementedError

    @abstractmethod
    def setValue(self, value: BytesLike) -> None:
        raise NotImplementedError

    def __iter__(self) -> Iterable[int]:
        for index in range(self.size()):
            yield self.get(index)


class HLAvariableArray(DataElement, ABC):
    @abstractmethod
    def addElement(self, dataElement: DataElement) -> None:
        raise NotImplementedError

    @abstractmethod
    def resize(self, newSize: int) -> None:
        raise NotImplementedError

    @abstractmethod
    def size(self) -> int:
        raise NotImplementedError

    @abstractmethod
    def get(self, index: int) -> DataElement:
        raise NotImplementedError

    def __iter__(self) -> Iterable[DataElement]:
        for index in range(self.size()):
            yield self.get(index)


class HLAfixedArray(DataElement, ABC):
    @abstractmethod
    def size(self) -> int:
        raise NotImplementedError

    @abstractmethod
    def get(self, index: int) -> DataElement:
        raise NotImplementedError

    def __iter__(self) -> Iterable[DataElement]:
        for index in range(self.size()):
            yield self.get(index)


class HLAfixedRecord(DataElement, ABC):
    @abstractmethod
    def add(self, dataElement: DataElement) -> None:
        raise NotImplementedError

    @abstractmethod
    def size(self) -> int:
        raise NotImplementedError

    @abstractmethod
    def get(self, index: int) -> DataElement:
        raise NotImplementedError

    def __iter__(self) -> Iterable[DataElement]:
        for index in range(self.size()):
            yield self.get(index)


class HLAvariantRecord(DataElement, ABC):
    @abstractmethod
    def setVariant(self, discriminant: DataElement, dataElement: DataElement) -> None:
        raise NotImplementedError

    @abstractmethod
    def setDiscriminant(self, discriminant: DataElement) -> None:
        raise NotImplementedError

    @abstractmethod
    def getDiscriminant(self) -> DataElement:
        raise NotImplementedError

    @abstractmethod
    def getValue(self) -> DataElement | None:
        raise NotImplementedError


class EncoderFactory(ABC):
    """Abstract factory matching every 2010 Java encoder overload family.

    The Java API has one no-argument and one value overload for each scalar
    type. Python expresses that pair as one optional-value method; a concrete
    provider must still perform the actual standard encoding.
    """

    @abstractmethod
    def createHLAASCIIchar(self, value: int | None = None) -> HLAASCIIchar:
        raise NotImplementedError

    @abstractmethod
    def createHLAASCIIstring(self, value: str | None = None) -> HLAASCIIstring:
        raise NotImplementedError

    @abstractmethod
    def createHLAboolean(self, value: bool | None = None) -> HLAboolean:
        raise NotImplementedError

    @abstractmethod
    def createHLAbyte(self, value: int | None = None) -> HLAbyte:
        raise NotImplementedError

    @abstractmethod
    def createHLAvariantRecord(self, discriminant: DataElement) -> HLAvariantRecord:
        raise NotImplementedError

    @abstractmethod
    def createHLAfixedRecord(self) -> HLAfixedRecord:
        raise NotImplementedError

    @abstractmethod
    def createHLAfixedArray(self, factory_or_element: object | None = None, *elements_or_size: object) -> HLAfixedArray:
        raise NotImplementedError

    @abstractmethod
    def createHLAfloat32BE(self, value: float | int | None = None) -> HLAfloat32BE:
        raise NotImplementedError

    @abstractmethod
    def createHLAfloat32LE(self, value: float | int | None = None) -> HLAfloat32LE:
        raise NotImplementedError

    @abstractmethod
    def createHLAfloat64BE(self, value: float | int | None = None) -> HLAfloat64BE:
        raise NotImplementedError

    @abstractmethod
    def createHLAfloat64LE(self, value: float | int | None = None) -> HLAfloat64LE:
        raise NotImplementedError

    @abstractmethod
    def createHLAinteger16BE(self, value: int | None = None) -> HLAinteger16BE:
        raise NotImplementedError

    @abstractmethod
    def createHLAinteger16LE(self, value: int | None = None) -> HLAinteger16LE:
        raise NotImplementedError

    @abstractmethod
    def createHLAinteger32BE(self, value: int | None = None) -> HLAinteger32BE:
        raise NotImplementedError

    @abstractmethod
    def createHLAinteger32LE(self, value: int | None = None) -> HLAinteger32LE:
        raise NotImplementedError

    @abstractmethod
    def createHLAinteger64BE(self, value: int | None = None) -> HLAinteger64BE:
        raise NotImplementedError

    @abstractmethod
    def createHLAinteger64LE(self, value: int | None = None) -> HLAinteger64LE:
        raise NotImplementedError

    @abstractmethod
    def createHLAoctet(self, value: int | None = None) -> HLAoctet:
        raise NotImplementedError

    @abstractmethod
    def createHLAoctetPairBE(self, value: int | None = None) -> HLAoctetPairBE:
        raise NotImplementedError

    @abstractmethod
    def createHLAoctetPairLE(self, value: int | None = None) -> HLAoctetPairLE:
        raise NotImplementedError

    @abstractmethod
    def createHLAopaqueData(self, value: BytesLike | None = None) -> HLAopaqueData:
        raise NotImplementedError

    @abstractmethod
    def createHLAunicodeChar(self, value: int | None = None) -> HLAunicodeChar:
        raise NotImplementedError

    @abstractmethod
    def createHLAunicodeString(self, value: str | None = None) -> HLAunicodeString:
        raise NotImplementedError

    @abstractmethod
    def createHLAvariableArray(self, factory: DataElementFactory, *elements: DataElement) -> HLAvariableArray:
        raise NotImplementedError


__all__ = [
    "ByteWrapper", "DataElement", "DataElementFactory", "EncoderFactory",
    "EncoderException", "DecoderException", "HLAASCIIchar", "HLAASCIIstring",
    "HLAboolean", "HLAbyte", "HLAfixedArray", "HLAfixedRecord", "HLAfloat32BE",
    "HLAfloat32LE", "HLAfloat64BE", "HLAfloat64LE", "HLAinteger16BE",
    "HLAinteger16LE", "HLAinteger32BE", "HLAinteger32LE", "HLAinteger64BE",
    "HLAinteger64LE", "HLAoctet", "HLAoctetPairBE", "HLAoctetPairLE",
    "HLAopaqueData", "HLAunicodeChar", "HLAunicodeString", "HLAvariableArray",
    "HLAvariantRecord",
]
