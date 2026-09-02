"""Provider-facing abstract factories for IEEE 1516.1-2010."""

from __future__ import annotations

from abc import ABC, abstractmethod

from .byte_types import BytesLike
from .contracts import FederateAmbassador, NullFederateAmbassador, RTIambassador
from .values import *


class _HandleFactory(ABC):
    @abstractmethod
    def decode(self, buffer: BytesLike, offset: int = 0) -> EncodedHandle:
        raise NotImplementedError


class FederateHandleFactory(_HandleFactory):
    @abstractmethod
    def decode(self, buffer: BytesLike, offset: int = 0) -> FederateHandle:
        raise NotImplementedError


class ObjectClassHandleFactory(_HandleFactory):
    @abstractmethod
    def decode(self, buffer: BytesLike, offset: int = 0) -> ObjectClassHandle:
        raise NotImplementedError


class ObjectInstanceHandleFactory(_HandleFactory):
    @abstractmethod
    def decode(self, buffer: BytesLike, offset: int = 0) -> ObjectInstanceHandle:
        raise NotImplementedError


class AttributeHandleFactory(_HandleFactory):
    @abstractmethod
    def decode(self, buffer: BytesLike, offset: int = 0) -> AttributeHandle:
        raise NotImplementedError


class InteractionClassHandleFactory(_HandleFactory):
    @abstractmethod
    def decode(self, buffer: BytesLike, offset: int = 0) -> InteractionClassHandle:
        raise NotImplementedError


class ParameterHandleFactory(_HandleFactory):
    @abstractmethod
    def decode(self, buffer: BytesLike, offset: int = 0) -> ParameterHandle:
        raise NotImplementedError


class TransportationTypeHandleFactory(_HandleFactory):
    @abstractmethod
    def decode(self, buffer: BytesLike, offset: int = 0) -> TransportationTypeHandle:
        raise NotImplementedError

    @abstractmethod
    def getHLAdefaultReliable(self) -> TransportationTypeHandle:
        raise NotImplementedError

    @abstractmethod
    def getHLAdefaultBestEffort(self) -> TransportationTypeHandle:
        raise NotImplementedError


class DimensionHandleFactory(_HandleFactory):
    @abstractmethod
    def decode(self, buffer: BytesLike, offset: int = 0) -> DimensionHandle:
        raise NotImplementedError


class AttributeHandleSetFactory(ABC):
    @abstractmethod
    def create(self) -> MutableAttributeHandleSet:
        raise NotImplementedError


class DimensionHandleSetFactory(ABC):
    @abstractmethod
    def create(self) -> MutableDimensionHandleSet:
        raise NotImplementedError


class FederateHandleSetFactory(ABC):
    @abstractmethod
    def create(self) -> MutableFederateHandleSet:
        raise NotImplementedError


class RegionHandleSetFactory(ABC):
    @abstractmethod
    def create(self) -> MutableRegionHandleSet:
        raise NotImplementedError


class AttributeHandleValueMapFactory(ABC):
    @abstractmethod
    def create(self, capacity: int = 0) -> MutableAttributeHandleValueMap:
        raise NotImplementedError


class ParameterHandleValueMapFactory(ABC):
    @abstractmethod
    def create(self, capacity: int = 0) -> MutableParameterHandleValueMap:
        raise NotImplementedError


class AttributeSetRegionSetPairListFactory(ABC):
    @abstractmethod
    def create(self, capacity: int = 0) -> AttributeSetRegionSetPairList:
        raise NotImplementedError


class LogicalTimeFactory(ABC):
    @abstractmethod
    def decodeTime(self, buffer: BytesLike, offset: int = 0) -> LogicalTime:
        raise NotImplementedError

    @abstractmethod
    def decodeInterval(self, buffer: BytesLike, offset: int = 0) -> LogicalTimeInterval:
        raise NotImplementedError

    @abstractmethod
    def makeInitial(self) -> LogicalTime:
        raise NotImplementedError

    @abstractmethod
    def makeFinal(self) -> LogicalTime:
        raise NotImplementedError

    @abstractmethod
    def makeZero(self) -> LogicalTimeInterval:
        raise NotImplementedError

    @abstractmethod
    def makeEpsilon(self) -> LogicalTimeInterval:
        raise NotImplementedError

    @abstractmethod
    def getName(self) -> str:
        raise NotImplementedError


class RtiFactory(ABC):
    @abstractmethod
    def getRtiAmbassador(self) -> RTIambassador:
        raise NotImplementedError

    @abstractmethod
    def getEncoderFactory(self) -> object:
        raise NotImplementedError

    @abstractmethod
    def rtiName(self) -> str:
        raise NotImplementedError

    @abstractmethod
    def rtiVersion(self) -> str:
        raise NotImplementedError


__all__ = [
    "FederateAmbassador", "NullFederateAmbassador", "RTIambassador", "RtiFactory",
    "FederateHandleFactory", "ObjectClassHandleFactory", "ObjectInstanceHandleFactory",
    "AttributeHandleFactory", "InteractionClassHandleFactory", "ParameterHandleFactory",
    "TransportationTypeHandleFactory", "DimensionHandleFactory", "AttributeHandleSetFactory",
    "DimensionHandleSetFactory", "FederateHandleSetFactory", "RegionHandleSetFactory",
    "AttributeHandleValueMapFactory", "ParameterHandleValueMapFactory",
    "AttributeSetRegionSetPairListFactory", "LogicalTimeFactory",
]
