"""Independent value objects for the IEEE 1516.1-2010 Python surface.

These classes deliberately do not reuse the 2025 value classes.  A handle or
time value from one edition is therefore never silently accepted by the other
edition's provider adapter.
"""

from __future__ import annotations

from collections.abc import Iterable, Iterator, Mapping, MutableMapping, MutableSet
from dataclasses import dataclass
from enum import Enum
from typing import Generic, TypeVar

from .byte_types import BytesLike, WritableBytes, copy_bytes


class CallbackModel(Enum):
    HLA_IMMEDIATE = "HLA_IMMEDIATE"
    HLA_EVOKED = "HLA_EVOKED"


class OrderType(Enum):
    RECEIVE = "RECEIVE"
    TIMESTAMP = "TIMESTAMP"

    def encodedLength(self) -> int:
        return 1

    def encode(self, buffer: WritableBytes, offset: int = 0) -> None:
        if offset < 0 or offset >= len(buffer):
            raise ValueError("buffer does not have room for OrderType")
        buffer[offset] = 1 if self is OrderType.RECEIVE else 2

    @staticmethod
    def decode(buffer: BytesLike, offset: int = 0) -> "OrderType":
        if offset < 0 or offset >= len(buffer):
            raise ValueError("buffer does not contain an OrderType")
        value = copy_bytes(buffer)[offset]
        if value == 1:
            return OrderType.RECEIVE
        if value == 2:
            return OrderType.TIMESTAMP
        from .exceptions import CouldNotDecode

        raise CouldNotDecode("Cannot decode OrderType")


class ServiceGroup(Enum):
    FEDERATION_MANAGEMENT = "FEDERATION_MANAGEMENT"
    DECLARATION_MANAGEMENT = "DECLARATION_MANAGEMENT"
    OBJECT_MANAGEMENT = "OBJECT_MANAGEMENT"
    OWNERSHIP_MANAGEMENT = "OWNERSHIP_MANAGEMENT"
    TIME_MANAGEMENT = "TIME_MANAGEMENT"
    DATA_DISTRIBUTION_MANAGEMENT = "DATA_DISTRIBUTION_MANAGEMENT"
    SUPPORT_SERVICES = "SUPPORT_SERVICES"


class ResignAction(Enum):
    UNCONDITIONALLY_DIVEST_ATTRIBUTES = "UNCONDITIONALLY_DIVEST_ATTRIBUTES"
    DELETE_OBJECTS = "DELETE_OBJECTS"
    CANCEL_PENDING_OWNERSHIP_ACQUISITIONS = "CANCEL_PENDING_OWNERSHIP_ACQUISITIONS"
    DELETE_OBJECTS_THEN_DIVEST = "DELETE_OBJECTS_THEN_DIVEST"
    CANCEL_THEN_DELETE_THEN_DIVEST = "CANCEL_THEN_DELETE_THEN_DIVEST"
    NO_ACTION = "NO_ACTION"


class SynchronizationPointFailureReason(Enum):
    SYNCHRONIZATION_POINT_LABEL_NOT_UNIQUE = "SYNCHRONIZATION_POINT_LABEL_NOT_UNIQUE"
    SYNCHRONIZATION_SET_MEMBER_NOT_JOINED = "SYNCHRONIZATION_SET_MEMBER_NOT_JOINED"


class SaveStatus(Enum):
    NO_SAVE_IN_PROGRESS = "NO_SAVE_IN_PROGRESS"
    FEDERATE_INSTRUCTED_TO_SAVE = "FEDERATE_INSTRUCTED_TO_SAVE"
    FEDERATE_SAVING = "FEDERATE_SAVING"
    FEDERATE_WAITING_FOR_FEDERATION_TO_SAVE = "FEDERATE_WAITING_FOR_FEDERATION_TO_SAVE"


class SaveFailureReason(Enum):
    RTI_UNABLE_TO_SAVE = "RTI_UNABLE_TO_SAVE"
    FEDERATE_REPORTED_FAILURE_DURING_SAVE = "FEDERATE_REPORTED_FAILURE_DURING_SAVE"
    FEDERATE_RESIGNED_DURING_SAVE = "FEDERATE_RESIGNED_DURING_SAVE"
    RTI_DETECTED_FAILURE_DURING_SAVE = "RTI_DETECTED_FAILURE_DURING_SAVE"
    SAVE_TIME_CANNOT_BE_HONORED = "SAVE_TIME_CANNOT_BE_HONORED"
    SAVE_ABORTED = "SAVE_ABORTED"


class RestoreFailureReason(Enum):
    RTI_UNABLE_TO_RESTORE = "RTI_UNABLE_TO_RESTORE"
    FEDERATE_REPORTED_FAILURE_DURING_RESTORE = "FEDERATE_REPORTED_FAILURE_DURING_RESTORE"
    FEDERATE_RESIGNED_DURING_RESTORE = "FEDERATE_RESIGNED_DURING_RESTORE"
    RTI_DETECTED_FAILURE_DURING_RESTORE = "RTI_DETECTED_FAILURE_DURING_RESTORE"
    RESTORE_ABORTED = "RESTORE_ABORTED"


class RestoreStatus(Enum):
    NO_RESTORE_IN_PROGRESS = "NO_RESTORE_IN_PROGRESS"
    FEDERATE_RESTORE_REQUEST_PENDING = "FEDERATE_RESTORE_REQUEST_PENDING"
    FEDERATE_WAITING_FOR_RESTORE_TO_BEGIN = "FEDERATE_WAITING_FOR_RESTORE_TO_BEGIN"
    FEDERATE_PREPARED_TO_RESTORE = "FEDERATE_PREPARED_TO_RESTORE"
    FEDERATE_RESTORING = "FEDERATE_RESTORING"
    FEDERATE_WAITING_FOR_FEDERATION_TO_RESTORE = "FEDERATE_WAITING_FOR_FEDERATION_TO_RESTORE"


@dataclass(frozen=True, slots=True)
class EncodedHandle:
    """Immutable provider handle snapshot represented by its encoded bytes."""

    encodedValue: BytesLike

    def __post_init__(self) -> None:
        object.__setattr__(self, "encodedValue", copy_bytes(self.encodedValue, name="encodedValue"))

    def encodedLength(self) -> int:
        return len(self.encodedValue)

    def encode(self, buffer: WritableBytes, offset: int = 0) -> None:
        if offset < 0 or offset + len(self.encodedValue) > len(buffer):
            raise ValueError(f"buffer does not have room for {type(self).__name__}")
        buffer[offset : offset + len(self.encodedValue)] = self.encodedValue

    def toString(self) -> str:
        return self.encodedValue.hex()

    def equals(self, other: object) -> bool:
        """Java-shaped equality operation retained alongside ``==``."""

        return self == other

    def hashCode(self) -> int:
        """Java-shaped hash operation retained alongside ``hash()``."""

        return hash(self)

    def __str__(self) -> str:
        return self.toString()


class FederateHandle(EncodedHandle):
    pass


class ObjectClassHandle(EncodedHandle):
    pass


class ObjectInstanceHandle(EncodedHandle):
    pass


class AttributeHandle(EncodedHandle):
    pass


class InteractionClassHandle(EncodedHandle):
    pass


class ParameterHandle(EncodedHandle):
    pass


class TransportationTypeHandle(EncodedHandle):
    pass


class DimensionHandle(EncodedHandle):
    pass


class RegionHandle(EncodedHandle):
    pass


class MessageRetractionHandle(EncodedHandle):
    """Opaque key for timestamped message retraction."""


_HandleT = TypeVar("_HandleT", bound=EncodedHandle)


class _MutableHandleSet(MutableSet[_HandleT], Generic[_HandleT]):
    __slots__ = ("_values",)
    _handle_type: type[EncodedHandle] = EncodedHandle

    def __init__(self, values: Iterable[_HandleT] = ()) -> None:
        self._values: set[_HandleT] = set()
        for value in values:
            self.add(value)

    def __contains__(self, value: object) -> bool:
        return value in self._values

    def __iter__(self) -> Iterator[_HandleT]:
        return iter(self._values)

    def __len__(self) -> int:
        return len(self._values)

    def add(self, value: _HandleT) -> None:
        if not isinstance(value, self._handle_type):
            raise TypeError(f"{type(self).__name__} values must be {self._handle_type.__name__}")
        self._values.add(value)

    def discard(self, value: object) -> None:
        self._values.discard(value)  # type: ignore[arg-type]


class MutableAttributeHandleSet(_MutableHandleSet[AttributeHandle]):
    _handle_type = AttributeHandle


class MutableDimensionHandleSet(_MutableHandleSet[DimensionHandle]):
    _handle_type = DimensionHandle


class MutableFederateHandleSet(_MutableHandleSet[FederateHandle]):
    _handle_type = FederateHandle


class MutableRegionHandleSet(_MutableHandleSet[RegionHandle]):
    _handle_type = RegionHandle


class _MutableHandleValueMap(MutableMapping[_HandleT, bytes], Generic[_HandleT]):
    __slots__ = ("_values",)
    _handle_type: type[EncodedHandle] = EncodedHandle

    def __init__(self, values: Mapping[_HandleT, BytesLike] | Iterable[tuple[_HandleT, BytesLike]] = ()) -> None:
        self._values: dict[_HandleT, bytes] = {}
        for handle, value in dict(values).items():
            self[handle] = value

    def __getitem__(self, handle: _HandleT) -> bytes:
        return self._values[handle]

    def __setitem__(self, handle: _HandleT, value: BytesLike) -> None:
        if not isinstance(handle, self._handle_type):
            raise TypeError(f"{type(self).__name__} keys must be {self._handle_type.__name__}")
        self._values[handle] = copy_bytes(value)

    def __delitem__(self, handle: _HandleT) -> None:
        del self._values[handle]

    def __iter__(self) -> Iterator[_HandleT]:
        return iter(self._values)

    def __len__(self) -> int:
        return len(self._values)

    def getValueReference(self, key: _HandleT, byteWrapper: object | None = None) -> object | None:
        value = self._values.get(key)
        if value is None:
            return None
        if byteWrapper is None:
            return value
        setter = getattr(byteWrapper, "set", None)
        if callable(setter):
            setter(value)
        return byteWrapper


class MutableAttributeHandleValueMap(_MutableHandleValueMap[AttributeHandle]):
    _handle_type = AttributeHandle


class MutableParameterHandleValueMap(_MutableHandleValueMap[ParameterHandle]):
    _handle_type = ParameterHandle


class AttributeHandleSet(frozenset[AttributeHandle]):
    def clone(self) -> "AttributeHandleSet":
        return type(self)(self)


class DimensionHandleSet(frozenset[DimensionHandle]):
    pass


class FederateHandleSet(frozenset[FederateHandle]):
    pass


class RegionHandleSet(frozenset[RegionHandle]):
    pass


class ObjectInstanceNameSet(frozenset[str]):
    pass


class AttributeHandleValueMap(Mapping[AttributeHandle, bytes]):
    __slots__ = ("_values",)

    def __init__(self, values: Mapping[AttributeHandle, BytesLike] | Iterable[tuple[AttributeHandle, BytesLike]] = ()) -> None:
        copied: dict[AttributeHandle, bytes] = {}
        for handle, value in dict(values).items():
            if not isinstance(handle, AttributeHandle):
                raise TypeError("AttributeHandleValueMap keys must be AttributeHandle")
            copied[handle] = copy_bytes(value)
        self._values = copied

    def __getitem__(self, key: AttributeHandle) -> bytes:
        return self._values[key]

    def __iter__(self) -> Iterator[AttributeHandle]:
        return iter(self._values)

    def __len__(self) -> int:
        return len(self._values)

    def getValueReference(self, key: AttributeHandle, byteWrapper: object | None = None) -> object | None:
        value = self._values.get(key)
        if value is None:
            return None
        if byteWrapper is None:
            return value
        setter = getattr(byteWrapper, "set", None)
        if callable(setter):
            setter(value)
        return byteWrapper


class ParameterHandleValueMap(Mapping[ParameterHandle, bytes]):
    __slots__ = ("_values",)

    def __init__(self, values: Mapping[ParameterHandle, BytesLike] | Iterable[tuple[ParameterHandle, BytesLike]] = ()) -> None:
        copied: dict[ParameterHandle, bytes] = {}
        for handle, value in dict(values).items():
            if not isinstance(handle, ParameterHandle):
                raise TypeError("ParameterHandleValueMap keys must be ParameterHandle")
            copied[handle] = copy_bytes(value)
        self._values = copied

    def __getitem__(self, key: ParameterHandle) -> bytes:
        return self._values[key]

    def __iter__(self) -> Iterator[ParameterHandle]:
        return iter(self._values)

    def __len__(self) -> int:
        return len(self._values)

    def getValueReference(self, key: ParameterHandle, byteWrapper: object | None = None) -> object | None:
        """Return or populate the Java-shaped value reference overload."""

        value = self._values.get(key)
        if value is None:
            return None
        if byteWrapper is None:
            return value
        setter = getattr(byteWrapper, "set", None)
        if callable(setter):
            setter(value)
        return byteWrapper


@dataclass(frozen=True, slots=True)
class AttributeRegionAssociation:
    ahset: AttributeHandleSet
    rhset: RegionHandleSet

    @property
    def attributes(self) -> AttributeHandleSet:
        return self.ahset

    @property
    def regions(self) -> RegionHandleSet:
        return self.rhset


class AttributeSetRegionSetPairList(list[AttributeRegionAssociation]):
    """Mutable Java-shaped list of attribute/region associations."""

    def __init__(self, values: Iterable[AttributeRegionAssociation] = ()) -> None:
        super().__init__()
        for value in values:
            self.append(value)

    def append(self, value: AttributeRegionAssociation) -> None:
        if not isinstance(value, AttributeRegionAssociation):
            raise TypeError("pair-list values must be AttributeRegionAssociation")
        super().append(value)


@dataclass(frozen=True, slots=True)
class FederationExecutionInformation:
    federationExecutionName: str
    logicalTimeImplementationName: str

    def equals(self, other: object) -> bool:
        return self == other

    def hashCode(self) -> int:
        return hash(self)


class FederationExecutionInformationSet(frozenset[FederationExecutionInformation]):
    pass


@dataclass(frozen=True, slots=True)
class SupplementalReflectInfo:
    """Provider-neutral snapshot of the 2010 reflect callback supplement."""

    producingFederate: FederateHandle | None = None
    sentRegions: RegionHandleSet | None = None

    def hasProducingFederate(self) -> bool:
        return self.producingFederate is not None

    def hasSentRegions(self) -> bool:
        return self.sentRegions is not None

    def getProducingFederate(self) -> FederateHandle | None:
        return self.producingFederate

    def getSentRegions(self) -> RegionHandleSet | None:
        return self.sentRegions


@dataclass(frozen=True, slots=True)
class SupplementalReceiveInfo:
    """Provider-neutral snapshot of the 2010 receive callback supplement."""

    producingFederate: FederateHandle | None = None
    sentRegions: RegionHandleSet | None = None

    def hasProducingFederate(self) -> bool:
        return self.producingFederate is not None

    def hasSentRegions(self) -> bool:
        return self.sentRegions is not None

    def getProducingFederate(self) -> FederateHandle | None:
        return self.producingFederate

    def getSentRegions(self) -> RegionHandleSet | None:
        return self.sentRegions


@dataclass(frozen=True, slots=True)
class SupplementalRemoveInfo:
    """Provider-neutral snapshot of the 2010 remove callback supplement."""

    producingFederate: FederateHandle | None = None

    def hasProducingFederate(self) -> bool:
        return self.producingFederate is not None

    def getProducingFederate(self) -> FederateHandle | None:
        return self.producingFederate


@dataclass(frozen=True, slots=True)
class FederateHandleSaveStatusPair:
    handle: FederateHandle
    status: SaveStatus

    @property
    def federateHandle(self) -> FederateHandle:
        return self.handle


@dataclass(frozen=True, slots=True)
class FederateRestoreStatus:
    preRestoreHandle: FederateHandle
    postRestoreHandle: FederateHandle
    status: RestoreStatus


@dataclass(frozen=True, slots=True)
class MessageRetractionReturn:
    retractionHandleIsValid: bool
    handle: MessageRetractionHandle | None


@dataclass(frozen=True, slots=True, eq=False)
class TimeQueryReturn:
    timeIsValid: bool
    time: "LogicalTime | None"

    def toString(self) -> str:
        return f"{str(self.timeIsValid).lower()} {'null' if self.time is None else self.time}"

    def __str__(self) -> str:
        return self.toString()

    def __eq__(self, other: object) -> bool:
        if not isinstance(other, TimeQueryReturn):
            return NotImplemented
        if not self.timeIsValid and not other.timeIsValid:
            return True
        return self.timeIsValid == other.timeIsValid and self.time == other.time

    def __hash__(self) -> int:
        return hash((self.timeIsValid, self.time if self.timeIsValid else None))

    def equals(self, other: object) -> bool:
        return self == other

    def hashCode(self) -> int:
        return hash(self)


@dataclass(frozen=True, slots=True)
class RangeBounds:
    lower: int
    upper: int

    def equals(self, other: object) -> bool:
        return self == other

    def hashCode(self) -> int:
        return hash(self)

    def getLowerBound(self) -> int:
        return self.lower

    def getUpperBound(self) -> int:
        return self.upper


@dataclass(frozen=True, slots=True)
class LogicalTime:
    encodedValue: BytesLike = b""
    initial: bool = False
    final: bool = False
    value: int | float | None = None
    text: str = ""
    # The 2010 Java interface does not require this accessor, but retaining
    # the selected factory name makes carrier identity explicit at the Python
    # boundary and lets adapters reject mixed integer/float values safely.
    implementationNameValue: str = ""

    def __post_init__(self) -> None:
        object.__setattr__(self, "encodedValue", copy_bytes(self.encodedValue, name="encodedValue"))

    def isInitial(self) -> bool:
        return self.initial

    def isFinal(self) -> bool:
        return self.final

    def add(self, val: "LogicalTimeInterval") -> "LogicalTime":
        raise NotImplementedError

    def subtract(self, val: "LogicalTimeInterval") -> "LogicalTime":
        raise NotImplementedError

    def distance(self, val: "LogicalTime") -> "LogicalTimeInterval":
        raise NotImplementedError

    def compareTo(self, other: "LogicalTime") -> int:
        """Compare two provider-owned logical times using the 2010 shape."""

        raise NotImplementedError

    def encodedLength(self) -> int:
        return len(self.encodedValue)

    def encode(self, buffer: WritableBytes, offset: int = 0) -> None:
        if offset < 0 or offset + len(self.encodedValue) > len(buffer):
            raise ValueError("buffer does not have room for logical time")
        buffer[offset : offset + len(self.encodedValue)] = self.encodedValue

    def toByteArray(self) -> bytes:
        return self.encodedValue

    def toString(self) -> str:
        return self.text or repr(self.value)

    def implementationName(self) -> str:
        return self.implementationNameValue or type(self).__name__

    def equals(self, other: object) -> bool:
        return self == other

    def hashCode(self) -> int:
        return hash(self)

    def getTime(self) -> int | float | None:
        return self.value


@dataclass(frozen=True, slots=True)
class LogicalTimeInterval:
    encodedValue: BytesLike = b""
    zero: bool = False
    epsilon: bool = False
    value: int | float | None = None
    text: str = ""
    implementationNameValue: str = ""

    def __post_init__(self) -> None:
        object.__setattr__(self, "encodedValue", copy_bytes(self.encodedValue, name="encodedValue"))

    def isZero(self) -> bool:
        return self.zero

    def isEpsilon(self) -> bool:
        return self.epsilon

    def add(self, addend: "LogicalTimeInterval") -> "LogicalTimeInterval":
        raise NotImplementedError

    def subtract(self, subtrahend: "LogicalTimeInterval") -> "LogicalTimeInterval":
        raise NotImplementedError

    def compareTo(self, other: "LogicalTimeInterval") -> int:
        """Compare two provider-owned logical intervals using the 2010 shape."""

        raise NotImplementedError

    def encodedLength(self) -> int:
        return len(self.encodedValue)

    def encode(self, buffer: WritableBytes, offset: int = 0) -> None:
        if offset < 0 or offset + len(self.encodedValue) > len(buffer):
            raise ValueError("buffer does not have room for logical interval")
        buffer[offset : offset + len(self.encodedValue)] = self.encodedValue

    def toByteArray(self) -> bytes:
        return self.encodedValue

    def toString(self) -> str:
        return self.text or repr(self.value)

    def implementationName(self) -> str:
        return self.implementationNameValue or type(self).__name__.removesuffix("Interval")

    def equals(self, other: object) -> bool:
        return self == other

    def hashCode(self) -> int:
        return hash(self)

    def getInterval(self) -> int | float | None:
        return self.value


class HLAinteger64Time(LogicalTime):
    pass


class HLAfloat64Time(LogicalTime):
    pass


class HLAinteger64Interval(LogicalTimeInterval):
    pass


class HLAfloat64Interval(LogicalTimeInterval):
    pass


__all__ = [
    "CallbackModel", "OrderType", "ServiceGroup", "ResignAction",
    "SynchronizationPointFailureReason", "SaveStatus", "SaveFailureReason",
    "RestoreFailureReason", "RestoreStatus", "EncodedHandle", "FederateHandle",
    "ObjectClassHandle", "ObjectInstanceHandle", "AttributeHandle",
    "InteractionClassHandle", "ParameterHandle", "TransportationTypeHandle",
    "DimensionHandle", "RegionHandle", "MessageRetractionHandle",
    "MutableAttributeHandleSet", "MutableDimensionHandleSet", "MutableFederateHandleSet",
    "MutableRegionHandleSet", "MutableAttributeHandleValueMap", "MutableParameterHandleValueMap",
    "AttributeHandleSet", "DimensionHandleSet", "FederateHandleSet", "RegionHandleSet",
    "ObjectInstanceNameSet", "AttributeHandleValueMap", "ParameterHandleValueMap",
    "AttributeRegionAssociation", "AttributeSetRegionSetPairList",
    "FederationExecutionInformation", "FederationExecutionInformationSet",
    "SupplementalReflectInfo", "SupplementalReceiveInfo", "SupplementalRemoveInfo",
    "FederateHandleSaveStatusPair", "FederateRestoreStatus", "MessageRetractionReturn",
    "TimeQueryReturn", "RangeBounds", "LogicalTime", "LogicalTimeInterval",
    "HLAinteger64Time", "HLAfloat64Time", "HLAinteger64Interval", "HLAfloat64Interval",
]
