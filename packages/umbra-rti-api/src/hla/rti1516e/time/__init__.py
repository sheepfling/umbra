"""Standard IEEE 1516.1-2010 logical-time value and factory names."""

from __future__ import annotations

from ..abstract import LogicalTimeFactory
from ..byte_types import BytesLike
from ..values import (
    HLAfloat64Interval,
    HLAfloat64Time,
    HLAinteger64Interval,
    HLAinteger64Time,
)


class HLAfloat64TimeFactory(LogicalTimeFactory):
    def getName(self) -> str:
        return "HLAfloat64Time"

    def decodeTime(self, buffer: BytesLike, offset: int = 0) -> HLAfloat64Time:
        raise NotImplementedError

    def decodeInterval(self, buffer: BytesLike, offset: int = 0) -> HLAfloat64Interval:
        raise NotImplementedError

    def makeInitial(self) -> HLAfloat64Time:
        raise NotImplementedError

    def makeFinal(self) -> HLAfloat64Time:
        raise NotImplementedError

    def makeZero(self) -> HLAfloat64Interval:
        raise NotImplementedError

    def makeEpsilon(self) -> HLAfloat64Interval:
        raise NotImplementedError

    def makeTime(self, value: float) -> HLAfloat64Time:
        raise NotImplementedError

    def makeInterval(self, value: float) -> HLAfloat64Interval:
        raise NotImplementedError


class HLAinteger64TimeFactory(LogicalTimeFactory):
    def getName(self) -> str:
        return "HLAinteger64Time"

    def decodeTime(self, buffer: BytesLike, offset: int = 0) -> HLAinteger64Time:
        raise NotImplementedError

    def decodeInterval(self, buffer: BytesLike, offset: int = 0) -> HLAinteger64Interval:
        raise NotImplementedError

    def makeInitial(self) -> HLAinteger64Time:
        raise NotImplementedError

    def makeFinal(self) -> HLAinteger64Time:
        raise NotImplementedError

    def makeZero(self) -> HLAinteger64Interval:
        raise NotImplementedError

    def makeEpsilon(self) -> HLAinteger64Interval:
        raise NotImplementedError

    def makeTime(self, value: int) -> HLAinteger64Time:
        raise NotImplementedError

    def makeInterval(self, value: int) -> HLAinteger64Interval:
        raise NotImplementedError


__all__ = [
    "HLAfloat64Time", "HLAfloat64Interval", "HLAinteger64Time", "HLAinteger64Interval",
    "HLAfloat64TimeFactory", "HLAinteger64TimeFactory",
]
