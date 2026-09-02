"""Direct C++ IEEE 1516.1-2010 logical-time façades."""

from __future__ import annotations

from typing import Any

from hla.rti1516e import (
    HLAfloat64Interval,
    HLAfloat64Time,
    HLAinteger64Interval,
    HLAinteger64Time,
    LogicalTime,
    LogicalTimeFactory,
    LogicalTimeInterval,
)
from hla.rti1516e.exceptions import exceptionForName

from . import _native_2010


_LOGICAL_TIME_ENCODED_LENGTH = 8


def _call_native(function: Any, *args: object) -> Any:
    try:
        return function(*args)
    except _native_2010.Native2010RtiError as error:
        name, separator, message = str(error).partition(": ")
        raise exceptionForName(
            name, message if separator else name, error
        ) from error


def _logical_time_bytes(buffer: bytes, offset: int) -> bytes:
    encoded = bytes(buffer)
    if offset < 0 or offset > len(encoded) - _LOGICAL_TIME_ENCODED_LENGTH:
        raise exceptionForName(
            "CouldNotDecode",
            "An IEEE 1516.1-2010 logical-time encoding must contain eight octets.",
        )
    return encoded[offset : offset + _LOGICAL_TIME_ENCODED_LENGTH]


class _NativeTimeMixin:
    _implementation: Any
    _factory: "Native2010TimeFactory"

    def _require_time(self, value: object) -> LogicalTime:
        if not isinstance(value, LogicalTime):
            raise TypeError("time must be LogicalTime")
        if value.implementationName() != self.implementationName():
            raise ValueError(
                f"time uses {value.implementationName()}, expected {self.implementationName()}"
            )
        return value

    def _require_interval(self, value: object) -> LogicalTimeInterval:
        if not isinstance(value, LogicalTimeInterval):
            raise TypeError("interval must be LogicalTimeInterval")
        if value.implementationName() != self.implementationName():
            raise ValueError(
                f"interval uses {value.implementationName()}, expected {self.implementationName()}"
            )
        return value

    def add(self, value: object) -> "Native2010Integer64Time":
        interval = self._require_interval(value)
        result = self._factory.makeTime(self.getTime())
        _call_native(result._implementation.add_interval, interval.getInterval())
        return result

    def subtract(self, value: object) -> "Native2010Integer64Time":
        interval = self._require_interval(value)
        result = self._factory.makeTime(self.getTime())
        _call_native(result._implementation.subtract_interval, interval.getInterval())
        return result

    def distance(self, value: object) -> "Native2010Integer64Interval":
        other = self._require_time(value)
        return self._factory.makeInterval(
            self.getTime() - other.getTime()
        )

    def compareTo(self, value: object) -> int:
        other = self._require_time(value)
        return int(_call_native(self._implementation.compare, other.getTime()))


class _NativeIntervalMixin:
    _implementation: Any
    _factory: "Native2010TimeFactory"

    def _require_interval(self, value: object) -> "Native2010Integer64Interval":
        if not isinstance(value, LogicalTimeInterval):
            raise TypeError("interval must be LogicalTimeInterval")
        if value.implementationName() != self.implementationName():
            raise ValueError(
                f"interval uses {value.implementationName()}, expected {self.implementationName()}"
            )
        return value  # type: ignore[return-value]

    def add(self, value: object) -> "Native2010Integer64Interval":
        other = self._require_interval(value)
        result = self._factory.makeInterval(self.getInterval())
        _call_native(result._implementation.add_interval, other.getInterval())
        return result

    def subtract(self, value: object) -> "Native2010Integer64Interval":
        other = self._require_interval(value)
        result = self._factory.makeInterval(self.getInterval())
        _call_native(result._implementation.subtract_interval, other.getInterval())
        return result

    def compareTo(self, value: object) -> int:
        other = self._require_interval(value)
        left, right = int(self.getInterval()), int(other.getInterval())
        return -1 if left < right else 1 if left > right else 0


class Native2010Integer64Time(_NativeTimeMixin, HLAinteger64Time):
    __slots__ = ("_implementation", "_factory")

    def getTime(self) -> int:
        return int(_call_native(self._implementation.get_time))

    def encodedLength(self) -> int:
        return int(_call_native(self._implementation.encoded_length))


class Native2010Integer64Interval(_NativeIntervalMixin, HLAinteger64Interval):
    __slots__ = ("_implementation", "_factory")

    def getInterval(self) -> int:
        return int(_call_native(self._implementation.get_interval))

    def encodedLength(self) -> int:
        return int(_call_native(self._implementation.encoded_length))


class Native2010Float64Time(_NativeTimeMixin, HLAfloat64Time):
    __slots__ = ("_implementation", "_factory")

    def getTime(self) -> float:
        return float(_call_native(self._implementation.get_time))

    def encodedLength(self) -> int:
        return int(_call_native(self._implementation.encoded_length))


class Native2010Float64Interval(_NativeIntervalMixin, HLAfloat64Interval):
    __slots__ = ("_implementation", "_factory")

    def getInterval(self) -> float:
        return float(_call_native(self._implementation.get_interval))

    def encodedLength(self) -> int:
        return int(_call_native(self._implementation.encoded_length))


class Native2010TimeFactory(LogicalTimeFactory):
    """Provider-owned integer time factory backed by official C++ classes."""

    def __init__(self, implementation: Any | None = None) -> None:
        self._implementation = implementation or _native_2010.Native2010Integer64TimeFactory()
        self._name = str(_call_native(self._implementation.name))

    def _wrap_time(self, implementation: Any) -> Native2010Integer64Time:
        result = Native2010Integer64Time(
            encodedValue=bytes(_call_native(implementation.to_byte_array)),
            initial=bool(_call_native(implementation.is_initial)),
            final=bool(_call_native(implementation.is_final)),
            value=int(_call_native(implementation.get_time)),
            text=str(_call_native(implementation.get_time)),
            implementationNameValue=self._name,
        )
        object.__setattr__(result, "_implementation", implementation)
        object.__setattr__(result, "_factory", self)
        return result

    def _wrap_interval(self, implementation: Any) -> Native2010Integer64Interval:
        value = int(_call_native(implementation.get_interval))
        result = Native2010Integer64Interval(
            encodedValue=bytes(_call_native(implementation.to_byte_array)),
            zero=bool(_call_native(implementation.is_zero)),
            epsilon=bool(_call_native(implementation.is_epsilon)),
            value=value,
            text=str(value),
            implementationNameValue=self._name,
        )
        object.__setattr__(result, "_implementation", implementation)
        object.__setattr__(result, "_factory", self)
        return result

    def getName(self) -> str:
        return self._name

    def implementationName(self) -> str:
        return self._name

    def decodeTime(self, buffer: bytes, offset: int = 0) -> LogicalTime:
        return self._wrap_time(
            _call_native(self._implementation.decode_time, _logical_time_bytes(buffer, offset))
        )

    def decodeInterval(self, buffer: bytes, offset: int = 0) -> LogicalTimeInterval:
        return self._wrap_interval(
            _call_native(
                self._implementation.decode_interval,
                _logical_time_bytes(buffer, offset),
            )
        )

    def decodeLogicalTime(self, buffer: bytes, offset: int = 0) -> LogicalTime:
        return self.decodeTime(buffer, offset)

    def decodeLogicalTimeInterval(
        self, buffer: bytes, offset: int = 0
    ) -> LogicalTimeInterval:
        return self.decodeInterval(buffer, offset)

    def makeInitial(self) -> Native2010Integer64Time:
        return self._wrap_time(_call_native(self._implementation.make_initial))

    def makeFinal(self) -> Native2010Integer64Time:
        return self._wrap_time(_call_native(self._implementation.make_final))

    def makeZero(self) -> Native2010Integer64Interval:
        return self._wrap_interval(_call_native(self._implementation.make_zero))

    def makeEpsilon(self) -> Native2010Integer64Interval:
        return self._wrap_interval(_call_native(self._implementation.make_epsilon))

    def makeLogicalTime(self, value: int) -> Native2010Integer64Time:
        return self.makeTime(value)

    def makeTime(self, value: int) -> Native2010Integer64Time:
        return self._wrap_time(_call_native(self._implementation.make_time, int(value)))

    def makeLogicalTimeInterval(self, value: int) -> Native2010Integer64Interval:
        return self.makeInterval(value)

    def makeInterval(self, value: int) -> Native2010Integer64Interval:
        return self._wrap_interval(
            _call_native(self._implementation.make_interval, int(value))
        )


class Native2010Float64TimeFactory(LogicalTimeFactory):
    """Provider-owned floating-point time factory backed by official C++."""

    def __init__(self, implementation: Any | None = None) -> None:
        self._implementation = implementation or _native_2010.Native2010Float64TimeFactory()
        self._name = str(_call_native(self._implementation.name))

    def _wrap_time(self, implementation: Any) -> Native2010Float64Time:
        result = Native2010Float64Time(
            encodedValue=bytes(_call_native(implementation.to_byte_array)),
            initial=bool(_call_native(implementation.is_initial)),
            final=bool(_call_native(implementation.is_final)),
            value=float(_call_native(implementation.get_time)),
            text=str(_call_native(implementation.get_time)),
            implementationNameValue=self._name,
        )
        object.__setattr__(result, "_implementation", implementation)
        object.__setattr__(result, "_factory", self)
        return result

    def _wrap_interval(self, implementation: Any) -> Native2010Float64Interval:
        value = float(_call_native(implementation.get_interval))
        result = Native2010Float64Interval(
            encodedValue=bytes(_call_native(implementation.to_byte_array)),
            zero=bool(_call_native(implementation.is_zero)),
            epsilon=bool(_call_native(implementation.is_epsilon)),
            value=value,
            text=str(value),
            implementationNameValue=self._name,
        )
        object.__setattr__(result, "_implementation", implementation)
        object.__setattr__(result, "_factory", self)
        return result

    def getName(self) -> str:
        return self._name

    def implementationName(self) -> str:
        return self._name

    def decodeTime(self, buffer: bytes, offset: int = 0) -> LogicalTime:
        return self._wrap_time(
            _call_native(self._implementation.decode_time, _logical_time_bytes(buffer, offset))
        )

    def decodeInterval(self, buffer: bytes, offset: int = 0) -> LogicalTimeInterval:
        return self._wrap_interval(
            _call_native(
                self._implementation.decode_interval,
                _logical_time_bytes(buffer, offset),
            )
        )

    def decodeLogicalTime(self, buffer: bytes, offset: int = 0) -> LogicalTime:
        return self.decodeTime(buffer, offset)

    def decodeLogicalTimeInterval(
        self, buffer: bytes, offset: int = 0
    ) -> LogicalTimeInterval:
        return self.decodeInterval(buffer, offset)

    def makeInitial(self) -> Native2010Float64Time:
        return self._wrap_time(_call_native(self._implementation.make_initial))

    def makeFinal(self) -> Native2010Float64Time:
        return self._wrap_time(_call_native(self._implementation.make_final))

    def makeZero(self) -> Native2010Float64Interval:
        return self._wrap_interval(_call_native(self._implementation.make_zero))

    def makeEpsilon(self) -> Native2010Float64Interval:
        return self._wrap_interval(_call_native(self._implementation.make_epsilon))

    def makeLogicalTime(self, value: float) -> Native2010Float64Time:
        return self.makeTime(value)

    def makeTime(self, value: float) -> Native2010Float64Time:
        return self._wrap_time(_call_native(self._implementation.make_time, float(value)))

    def makeLogicalTimeInterval(self, value: float) -> Native2010Float64Interval:
        return self.makeInterval(value)

    def makeInterval(self, value: float) -> Native2010Float64Interval:
        return self._wrap_interval(
            _call_native(self._implementation.make_interval, float(value))
        )


__all__ = [
    "Native2010TimeFactory",
    "Native2010Integer64Time",
    "Native2010Integer64Interval",
    "Native2010Float64TimeFactory",
    "Native2010Float64Time",
    "Native2010Float64Interval",
]
