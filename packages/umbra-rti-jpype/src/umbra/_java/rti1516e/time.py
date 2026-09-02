"""Java-backed logical-time factories for the IEEE 1516e (2010) route.

The 2010 Java API keeps arithmetic on the logical-time carriers themselves.
This module preserves that ownership: Python values are immutable snapshots,
but values returned by this adapter retain the raw Java carrier so ``add``,
``subtract`` and ``distance`` still execute in the selected provider.
"""

from __future__ import annotations

import math
from typing import Any, Callable

from hla.rti1516e import (
    HLAfloat64Interval,
    HLAfloat64Time,
    HLAinteger64Interval,
    HLAinteger64Time,
    LogicalTime,
    LogicalTimeFactory,
    LogicalTimeInterval,
    TimeQueryReturn,
)
from hla.rti1516e.exceptions import (
    CouldNotDecode,
    InvalidLogicalTime,
    InvalidLogicalTimeInterval,
    RTIexception,
    RTIinternalError,
    exceptionForName,
)


_STANDARD_TIME_FACTORY_NAMES = {"HLAinteger64Time", "HLAfloat64Time"}


def _simple_name(value: str) -> str:
    return value.rsplit(".", 1)[-1].rsplit("$", 1)[-1]


def _logical_time_bytes(
    buffer: bytes, offset: int, *, fixed_width: bool = True
) -> bytes:
    encoded = bytes(buffer)
    if offset < 0 or offset > len(encoded) or (
        fixed_width and offset > len(encoded) - 8
    ):
        raise exceptionForName(
            "CouldNotDecode",
            (
                "An IEEE 1516.1-2010 logical-time encoding must contain eight octets."
                if fixed_width
                else "A logical-time encoding must contain data after its offset."
            ),
        )
    return encoded[offset : offset + 8] if fixed_width else encoded[offset:]


class _JavaTimeMixin:
    __slots__ = ()

    _java_value: object
    _factory: "Java2010TimeFactory"

    def _raw(self) -> object:
        return self._java_value

    def _require_same(self, value: object, expected: type[object], name: str) -> None:
        if not isinstance(value, expected):
            raise TypeError(f"{name} must be {expected.__name__}")
        if value.implementationName() != self.implementationName():  # type: ignore[attr-defined]
            raise ValueError(
                f"{name} uses {value.implementationName()}, expected {self.implementationName()}"  # type: ignore[attr-defined]
            )

    def add(self, value: object) -> object:
        self._require_same(value, LogicalTimeInterval, "interval")
        raw_interval = getattr(value, "_java_value", None)
        if raw_interval is None:
            raw_interval = self._factory._decode_interval(value)  # type: ignore[arg-type]
        return self._factory._wrap_time(
            self._factory._call(self._raw().add, raw_interval)
        )

    def subtract(self, value: object) -> object:
        self._require_same(value, LogicalTimeInterval, "interval")
        raw_interval = getattr(value, "_java_value", None)
        if raw_interval is None:
            raw_interval = self._factory._decode_interval(value)  # type: ignore[arg-type]
        return self._factory._wrap_time(
            self._factory._call(self._raw().subtract, raw_interval)
        )

    def distance(self, value: object) -> object:
        self._require_same(value, LogicalTime, "time")
        raw_time = getattr(value, "_java_value", None)
        if raw_time is None:
            raw_time = self._factory._decode_time(value)  # type: ignore[arg-type]
        return self._factory._wrap_interval(
            self._factory._call(self._raw().distance, raw_time)
        )

    def compareTo(self, value: object) -> int:
        self._require_same(value, LogicalTime, "time")
        raw_time = getattr(value, "_java_value", None)
        if raw_time is None:
            raw_time = self._factory._decode_time(value)  # type: ignore[arg-type]
        return int(self._factory._call(self._raw().compareTo, raw_time))


class _JavaIntervalMixin:
    __slots__ = ()

    _java_value: object
    _factory: "Java2010TimeFactory"

    def _raw(self) -> object:
        return self._java_value

    def _require_same(self, value: object) -> None:
        if not isinstance(value, LogicalTimeInterval):
            raise TypeError("interval must be LogicalTimeInterval")
        if value.implementationName() != self.implementationName():  # type: ignore[attr-defined]
            raise ValueError(
                f"interval uses {value.implementationName()}, expected {self.implementationName()}"  # type: ignore[attr-defined]
            )

    def add(self, value: object) -> object:
        self._require_same(value)
        raw_interval = getattr(value, "_java_value", None)
        if raw_interval is None:
            raw_interval = self._factory._decode_interval(value)  # type: ignore[arg-type]
        return self._factory._wrap_interval(
            self._factory._call(self._raw().add, raw_interval)
        )

    def subtract(self, value: object) -> object:
        self._require_same(value)
        raw_interval = getattr(value, "_java_value", None)
        if raw_interval is None:
            raw_interval = self._factory._decode_interval(value)  # type: ignore[arg-type]
        return self._factory._wrap_interval(
            self._factory._call(self._raw().subtract, raw_interval)
        )

    def compareTo(self, value: object) -> int:
        self._require_same(value)
        raw_interval = getattr(value, "_java_value", None)
        if raw_interval is None:
            raw_interval = self._factory._decode_interval(value)  # type: ignore[arg-type]
        return int(self._factory._call(self._raw().compareTo, raw_interval))


class Java2010Integer64Time(_JavaTimeMixin, HLAinteger64Time):
    __slots__ = ("_java_value", "_factory")


class Java2010Float64Time(_JavaTimeMixin, HLAfloat64Time):
    __slots__ = ("_java_value", "_factory")


class Java2010Integer64Interval(_JavaIntervalMixin, HLAinteger64Interval):
    __slots__ = ("_java_value", "_factory")


class Java2010Float64Interval(_JavaIntervalMixin, HLAfloat64Interval):
    __slots__ = ("_java_value", "_factory")


class Java2010TimeFactory(LogicalTimeFactory):
    """Provider-owned implementation of the 2010 ``LogicalTimeFactory``."""

    def __init__(self, ambassador: object, runtime: Any, java_factory: object) -> None:
        self._ambassador = ambassador
        self._runtime = runtime
        self._java_factory = java_factory
        self._name = str(self._call(getattr(java_factory, "getName")))
        if self._name == "HLAinteger64Time":
            self._time_type = Java2010Integer64Time
            self._interval_type = Java2010Integer64Interval
        elif self._name == "HLAfloat64Time":
            self._time_type = Java2010Float64Time
            self._interval_type = Java2010Float64Interval
        else:
            # A vendor is allowed to expose a custom logical-time factory.
            # Preserve it as the standard carrier shape instead of silently
            # converting it to integer or float.
            self._time_type = Java2010Float64Time
            self._interval_type = Java2010Float64Interval

    def _call(self, function: Callable[..., object], *args: object) -> object:
        try:
            return function(*args)
        except RTIexception:
            raise
        except Exception as error:
            name = self._runtime.exception_name(error)
            if name is None:
                raise RTIinternalError(
                    f"Java 1516e logical-time call failed: {error}"
                ) from error
            raise exceptionForName(_simple_name(name), str(error), error) from error

    def _numeric(self, value: object) -> int | float | None:
        for method_name in (
            "getTimeValue",
            "getIntervalValue",
            "getTime",
            "getInterval",
            "getValue",
        ):
            method = getattr(value, method_name, None)
            if callable(method):
                return method()  # type: ignore[no-any-return]
        for attribute_name in ("value", "timeValue", "intervalValue"):
            attribute = getattr(value, attribute_name, None)
            if isinstance(attribute, (int, float)):
                return attribute
        return None

    def _wrap_time(self, value: object) -> LogicalTime:
        numeric = self._numeric(value)
        try:
            invalid = (
                numeric is None
                or float(numeric) < 0.0
                or not math.isfinite(float(numeric))
            )
        except (TypeError, ValueError, OverflowError):
            invalid = True
        if invalid:
            raise InvalidLogicalTime(
                "Java provider returned an invalid logical-time value"
            )
        result = self._time_type(
            encodedValue=self._runtime.handle_bytes(value),
            initial=bool(value.isInitial()),
            final=bool(value.isFinal()),
            value=numeric,
            text=str(value.toString()),
            implementationNameValue=self._name,
        )
        object.__setattr__(result, "_java_value", value)
        object.__setattr__(result, "_factory", self)
        return result

    def _wrap_time_if_standard(self, value: object) -> object:
        return (
            self._wrap_time(value)
            if self._name in _STANDARD_TIME_FACTORY_NAMES
            else value
        )

    def _wrap_interval_if_standard(self, value: object) -> object:
        return (
            self._wrap_interval(value)
            if self._name in _STANDARD_TIME_FACTORY_NAMES
            else value
        )

    def _wrap_interval(self, value: object) -> LogicalTimeInterval:
        numeric = self._numeric(value)
        try:
            invalid = (
                numeric is None
                or float(numeric) < 0.0
                or not math.isfinite(float(numeric))
            )
        except (TypeError, ValueError, OverflowError):
            invalid = True
        if invalid:
            raise InvalidLogicalTimeInterval(
                "Java provider returned an invalid logical-time interval value"
            )
        result = self._interval_type(
            encodedValue=self._runtime.handle_bytes(value),
            zero=bool(value.isZero()),
            epsilon=bool(value.isEpsilon()),
            value=numeric,
            text=str(value.toString()),
            implementationNameValue=self._name,
        )
        object.__setattr__(result, "_java_value", value)
        object.__setattr__(result, "_factory", self)
        return result

    def _decode_time(self, value: LogicalTime) -> object:
        if value.implementationName() != self._name:
            raise ValueError(
                f"time uses {value.implementationName()}, expected {self._name}"
            )
        raw = getattr(value, "_java_value", None)
        if raw is not None:
            return raw
        return self._runtime.decode_logical_time(self._ambassador, value.encodedValue)

    def _decode_interval(self, value: LogicalTimeInterval) -> object:
        if value.implementationName() != self._name:
            raise ValueError(
                f"interval uses {value.implementationName()}, expected {self._name}"
            )
        raw = getattr(value, "_java_value", None)
        if raw is not None:
            return raw
        return self._runtime.decode_logical_interval(
            self._ambassador, value.encodedValue
        )

    def getName(self) -> str:
        return self._name

    def implementationName(self) -> str:
        """Compatibility spelling used by the sibling 2025 Python façade."""

        return self._name

    def from_java_value(self, value: object, expected_type: str) -> object:
        if expected_type == "LogicalTime":
            # The standard interface permits a provider-owned logical-time
            # factory. Its carrier may expose methods that look like the
            # float64 implementation while carrying different arithmetic or
            # wire semantics. Keep that object raw instead of silently
            # retyping it into one of Umbra's standard wrappers.
            if self._name not in _STANDARD_TIME_FACTORY_NAMES:
                return value
            return self._wrap_time(value)
        if expected_type == "LogicalTimeInterval":
            if self._name not in _STANDARD_TIME_FACTORY_NAMES:
                return value
            return self._wrap_interval(value)
        raise TypeError(f"unsupported 2010 time carrier: {expected_type}")

    def from_java_query_return(self, value: object) -> TimeQueryReturn:
        valid = bool(getattr(value, "timeIsValid"))
        raw_time = getattr(value, "time", None)
        return TimeQueryReturn(
            valid,
            self._wrap_time_if_standard(raw_time)
            if valid and raw_time is not None
            else None,
        )

    def decodeTime(self, buffer: bytes, offset: int = 0) -> LogicalTime:
        encoded = _logical_time_bytes(
            buffer,
            offset,
            fixed_width=self._name in _STANDARD_TIME_FACTORY_NAMES,
        )
        try:
            return self._wrap_time_if_standard(
                self._call(
                    self._java_factory.decodeTime, self._runtime.byte_array(encoded), 0
                )
            )
        except (InvalidLogicalTime, CouldNotDecode) as error:
            if isinstance(error, CouldNotDecode):
                raise
            raise CouldNotDecode(f"Could not decode logical time: {error}") from error

    def decodeInterval(self, buffer: bytes, offset: int = 0) -> LogicalTimeInterval:
        encoded = _logical_time_bytes(
            buffer,
            offset,
            fixed_width=self._name in _STANDARD_TIME_FACTORY_NAMES,
        )
        try:
            return self._wrap_interval_if_standard(
                self._call(
                    self._java_factory.decodeInterval,
                    self._runtime.byte_array(encoded),
                    0,
                )
            )
        except (InvalidLogicalTimeInterval, CouldNotDecode) as error:
            if isinstance(error, CouldNotDecode):
                raise
            raise CouldNotDecode(
                f"Could not decode logical-time interval: {error}"
            ) from error

    def decodeLogicalTime(self, buffer: bytes, offset: int = 0) -> LogicalTime:
        return self.decodeTime(buffer, offset)

    def decodeLogicalTimeInterval(
        self, buffer: bytes, offset: int = 0
    ) -> LogicalTimeInterval:
        return self.decodeInterval(buffer, offset)

    def makeInitial(self) -> LogicalTime:
        return self._wrap_time_if_standard(self._call(self._java_factory.makeInitial))

    def makeFinal(self) -> LogicalTime:
        return self._wrap_time_if_standard(self._call(self._java_factory.makeFinal))

    def makeZero(self) -> LogicalTimeInterval:
        return self._wrap_interval_if_standard(self._call(self._java_factory.makeZero))

    def makeEpsilon(self) -> LogicalTimeInterval:
        return self._wrap_interval_if_standard(self._call(self._java_factory.makeEpsilon))

    def makeLogicalTime(self, value: int | float) -> LogicalTime:
        return self.makeTime(value)

    def makeTime(self, value: int | float) -> LogicalTime:
        method = getattr(self._java_factory, "makeTime", None)
        if method is None:
            method = getattr(self._java_factory, "makeLogicalTime", None)
        if method is None:
            # ``LogicalTimeFactory`` itself only requires sentinel creators
            # and decode operations.  A provider-defined factory may therefore
            # intentionally omit the concrete numeric convenience method.
            # Keep that distinction explicit instead of leaking an
            # implementation-detail ``AttributeError`` from JPype.
            raise NotImplementedError(
                "2010 LogicalTimeFactory does not expose a numeric time creator"
            )
        return self._wrap_time_if_standard(self._call(method, value))

    def makeLogicalTimeInterval(self, value: int | float) -> LogicalTimeInterval:
        return self.makeInterval(value)

    def makeInterval(self, value: int | float) -> LogicalTimeInterval:
        method = getattr(self._java_factory, "makeInterval", None)
        if method is None:
            method = getattr(self._java_factory, "makeLogicalTimeInterval", None)
        if method is None:
            raise NotImplementedError(
                "2010 LogicalTimeFactory does not expose a numeric interval creator"
            )
        return self._wrap_interval_if_standard(self._call(method, value))


__all__ = [
    "Java2010TimeFactory",
    "Java2010Integer64Time",
    "Java2010Float64Time",
    "Java2010Integer64Interval",
    "Java2010Float64Interval",
]
