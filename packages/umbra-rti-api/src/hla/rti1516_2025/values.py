"""Concrete value objects shared by the IEEE 1516.1-2025 contracts."""

from __future__ import annotations

from collections.abc import Iterable, Iterator, Mapping, MutableMapping, MutableSet
from dataclasses import dataclass
from enum import Enum
from typing import Self, TypeVar, cast

from .byte_types import BytesLike, WritableBytes, copy_bytes


class AdditionalSettingsResultCode(Enum):
    SETTINGS_IGNORED = "SETTINGS_IGNORED"
    SETTINGS_FAILED_TO_PARSE = "SETTINGS_FAILED_TO_PARSE"
    SETTINGS_APPLIED = "SETTINGS_APPLIED"


class CallbackModel(Enum):
    HLA_IMMEDIATE = "HLA_IMMEDIATE"
    HLA_EVOKED = "HLA_EVOKED"


class OrderType(Enum):
    RECEIVE = "RECEIVE"
    TIMESTAMP = "TIMESTAMP"


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
class ConfigurationResult:
    """The standard Java API result object, represented as an immutable value."""

    configurationUsed: bool
    addressUsed: bool
    additionalSettingsResultCode: AdditionalSettingsResultCode
    message: str = ""


@dataclass(frozen=True, slots=True)
class TimeQueryResult:
    """Provider-created result for the Java API's ``TimeQueryReturn``."""

    timeIsValid: bool
    time: LogicalTime | None

    def __post_init__(self) -> None:
        if self.timeIsValid != (self.time is not None):
            raise ValueError("timeIsValid must agree with whether time is present")


@dataclass(frozen=True, slots=True)
class FederationExecutionInformation:
    """The Java callback record identifying one federation execution."""

    federationExecutionName: str
    logicalTimeImplementationName: str


class FederationExecutionInformationSet(frozenset[FederationExecutionInformation]):
    """Immutable Python callback snapshot of the Java information set."""


@dataclass(frozen=True, slots=True)
class FederationExecutionMemberInformation:
    """The Java callback record identifying one joined federation member."""

    federateName: str
    federateType: str


class FederationExecutionMemberInformationSet(frozenset[FederationExecutionMemberInformation]):
    """Immutable Python callback snapshot of the Java member-information set."""


@dataclass(frozen=True, slots=True)
class EncodedHandle:
    """Portable immutable representation of an opaque standard handle.

    IEEE 1516.1 values are provider-owned.  The shared Python surface retains
    only their standard encoded form, so an application can pass a handle
    returned by a provider back to that same provider without leaking a C++
    pointer or Java proxy into its public types.  Concrete subclasses preserve
    the standard's distinct handle domains.
    """

    encodedValue: BytesLike

    def __post_init__(self) -> None:
        object.__setattr__(self, "encodedValue", copy_bytes(self.encodedValue, name="encodedValue"))

    def encodedLength(self) -> int:
        return len(self.encodedValue)

    def encode(self, buffer: WritableBytes, offset: int = 0) -> None:
        if offset < 0 or offset + len(self.encodedValue) > len(buffer):
            raise ValueError(f"buffer does not have room for this {type(self).__name__}")
        buffer[offset : offset + len(self.encodedValue)] = self.encodedValue


class FederateHandle(EncodedHandle):
    """Portable immutable form of the standard opaque federate handle."""


class ObjectClassHandle(EncodedHandle):
    """Portable immutable form of a standard object-class handle."""


class ObjectInstanceHandle(EncodedHandle):
    """Portable immutable form of a standard object-instance handle."""


class AttributeHandle(EncodedHandle):
    """Portable immutable form of a standard attribute handle."""


class InteractionClassHandle(EncodedHandle):
    """Portable immutable form of a standard interaction-class handle."""


class ParameterHandle(EncodedHandle):
    """Portable immutable form of a standard interaction-parameter handle."""


class TransportationTypeHandle(EncodedHandle):
    """Portable immutable form of a standard transportation-type handle."""


class DimensionHandle(EncodedHandle):
    """Portable immutable form of a standard routing-space dimension handle."""


class RegionHandle(EncodedHandle):
    """Portable immutable form of a standard DDM region handle."""


class MessageRetractionHandle(EncodedHandle):
    """Portable immutable form of a timestamped message-retraction handle."""

    def isValid(self) -> bool:
        return bool(self.encodedValue)


_HandleT = TypeVar("_HandleT", bound=EncodedHandle)


class _MutableHandleSet(MutableSet[_HandleT]):
    """Mutable Java-shaped builder whose values are copied at service calls."""

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

    def __repr__(self) -> str:
        return f"{type(self).__name__}({tuple(self._values)!r})"


class MutableAttributeHandleSet(_MutableHandleSet[AttributeHandle]):
    """Mutable builder returned by :class:`AttributeHandleSetFactory`."""

    _handle_type = AttributeHandle


class MutableDimensionHandleSet(_MutableHandleSet[DimensionHandle]):
    """Mutable builder returned by :class:`DimensionHandleSetFactory`."""

    _handle_type = DimensionHandle


class MutableFederateHandleSet(_MutableHandleSet[FederateHandle]):
    """Mutable builder returned by :class:`FederateHandleSetFactory`."""

    _handle_type = FederateHandle


class MutableRegionHandleSet(_MutableHandleSet[RegionHandle]):
    """Mutable builder returned by :class:`RegionHandleSetFactory`."""

    _handle_type = RegionHandle


class _MutableHandleValueMap(MutableMapping[_HandleT, bytes]):
    """Mutable Java-shaped map builder with copied byte values."""

    __slots__ = ("_values",)
    _handle_type: type[EncodedHandle] = EncodedHandle

    def __init__(
        self,
        values: Mapping[_HandleT, BytesLike]
        | Iterable[tuple[_HandleT, BytesLike]] = (),
    ) -> None:
        self._values: dict[_HandleT, bytes] = {}
        for handle, value in dict(values).items():
            self[handle] = value

    def __getitem__(self, handle: _HandleT) -> bytes:
        return self._values[handle]

    def __setitem__(self, handle: _HandleT, value: BytesLike) -> None:
        if not isinstance(handle, self._handle_type):
            raise TypeError(f"{type(self).__name__} keys must be {self._handle_type.__name__}")
        self._values[handle] = bytes(value)

    def __delitem__(self, handle: _HandleT) -> None:
        del self._values[handle]

    def __iter__(self) -> Iterator[_HandleT]:
        return iter(self._values)

    def __len__(self) -> int:
        return len(self._values)

    def __repr__(self) -> str:
        return f"{type(self).__name__}({self._values!r})"


class MutableAttributeHandleValueMap(_MutableHandleValueMap[AttributeHandle]):
    """Mutable builder returned by :class:`AttributeHandleValueMapFactory`."""

    _handle_type = AttributeHandle


class MutableParameterHandleValueMap(_MutableHandleValueMap[ParameterHandle]):
    """Mutable builder returned by :class:`ParameterHandleValueMapFactory`."""

    _handle_type = ParameterHandle


class DimensionHandleSet(frozenset[DimensionHandle]):
    """Immutable Python snapshot/input form of a dimension-handle set."""


class RegionHandleSet(frozenset[RegionHandle]):
    """Immutable Python snapshot/input form of a region-handle set."""


class InteractionClassHandleSet(frozenset[InteractionClassHandle]):
    """Immutable Python snapshot/input form of an interaction-class set."""


class MutableInteractionClassHandleSet(_MutableHandleSet[InteractionClassHandle]):
    """Mutable Java-shaped builder returned by the standard interaction-set factory."""

    _handle_type = InteractionClassHandle


class ObjectInstanceNameSet(frozenset[str]):
    """Immutable Python input form of a standard object-instance name set."""


@dataclass(frozen=True, slots=True)
class AttributeSetRegionSetPair:
    """One Java-shaped attribute-set to region-set association."""

    attributes: AttributeHandleSet
    regions: RegionHandleSet

    def __post_init__(self) -> None:
        if not isinstance(self.attributes, AttributeHandleSet):
            raise TypeError("attributes must be AttributeHandleSet")
        if not isinstance(self.regions, RegionHandleSet):
            raise TypeError("regions must be RegionHandleSet")


@dataclass(frozen=True, slots=True)
class AttributeRegionAssociation:
    """Exact IEEE Java ``AttributeRegionAssociation`` value shape.

    The Java standard names the fields ``ahset`` and ``rhset``.  The
    ``attributes``/``regions`` properties keep this value interoperable with
    Umbra's existing Python pair vocabulary.
    """

    ahset: AttributeHandleSet
    rhset: RegionHandleSet

    def __post_init__(self) -> None:
        if not isinstance(self.ahset, AttributeHandleSet):
            raise TypeError("ahset must be AttributeHandleSet")
        if not isinstance(self.rhset, RegionHandleSet):
            raise TypeError("rhset must be RegionHandleSet")

    @property
    def attributes(self) -> AttributeHandleSet:
        return self.ahset

    @property
    def regions(self) -> RegionHandleSet:
        return self.rhset


_AttributeSetRegionSetPairListValue = AttributeSetRegionSetPair | AttributeRegionAssociation


class AttributeSetRegionSetPairList(tuple[_AttributeSetRegionSetPairListValue, ...]):
    """Immutable Java-shaped list of attribute-set/region-set associations."""

    def __new__(
        cls,
        values: Iterable[_AttributeSetRegionSetPairListValue] = (),
    ) -> Self:
        copied = tuple(values)
        for value in copied:
            if not isinstance(value, (AttributeSetRegionSetPair, AttributeRegionAssociation)):
                raise TypeError(
                    "pair list values must be AttributeSetRegionSetPair or AttributeRegionAssociation"
                )
        # ``tuple.__new__`` constructs the requested subclass at runtime, but
        # typeshed exposes its static return type as ``tuple``.
        return cast(Self, tuple.__new__(cls, copied))


class MutableAttributeSetRegionSetPairList(
    list[_AttributeSetRegionSetPairListValue]
):
    """Mutable list returned by ``AttributeSetRegionSetPairListFactory``.

    The Java factory's integer argument is a capacity hint, not an initial
    element count, so it is intentionally ignored after validation.
    """

    def __init__(
        self,
        capacity: int | Iterable[_AttributeSetRegionSetPairListValue] = 0,
        values: Iterable[_AttributeSetRegionSetPairListValue] = (),
    ) -> None:
        if not isinstance(capacity, int):
            values = capacity
            capacity = 0
        if capacity < 0:
            raise ValueError("capacity must be non-negative")
        super().__init__()
        for value in values:
            self.add(value)

    def add(self, value: _AttributeSetRegionSetPairListValue) -> None:
        if not isinstance(value, (AttributeSetRegionSetPair, AttributeRegionAssociation)):
            raise TypeError(
                "pair list values must be AttributeSetRegionSetPair or AttributeRegionAssociation"
            )
        self.append(value)


# Keep the C++ typedef vocabulary available for code that maps directly to
# ``AttributeHandleSetRegionHandleSetPairVector`` while using the Java-shaped
# names as the canonical Python surface.
AttributeHandleSetRegionHandleSetPair = AttributeSetRegionSetPair
AttributeHandleSetRegionHandleSetPairVector = AttributeSetRegionSetPairList


@dataclass(slots=True)
class RangeBounds:
    """Provider-owned lower/upper bounds for one region dimension."""

    lowerBound: int
    upperBound: int

    def getLowerBound(self) -> int:
        return self.lowerBound

    def getUpperBound(self) -> int:
        return self.upperBound

    def setLowerBound(self, lowerBound: int) -> None:
        self.lowerBound = int(lowerBound)

    def setUpperBound(self, upperBound: int) -> None:
        self.upperBound = int(upperBound)


@dataclass(frozen=True, slots=True)
class LogicalTime:
    """Provider-created logical time value carried across the Python boundary."""

    encodedValue: BytesLike
    implementationNameValue: str
    initial: bool = False
    final: bool = False
    value: int | float | None = None
    text: str = ""

    def __post_init__(self) -> None:
        object.__setattr__(self, "encodedValue", copy_bytes(self.encodedValue, name="encodedValue"))

    def encodedLength(self) -> int:
        return len(self.encodedValue)

    def encode(self, buffer: WritableBytes, offset: int = 0) -> None:
        if offset < 0 or offset + len(self.encodedValue) > len(buffer):
            raise ValueError(f"buffer does not have room for {type(self).__name__}")
        buffer[offset : offset + len(self.encodedValue)] = self.encodedValue

    def toByteArray(self) -> bytes:
        return self.encodedValue

    def isInitial(self) -> bool:
        return self.initial

    def isFinal(self) -> bool:
        return self.final

    def implementationName(self) -> str:
        return self.implementationNameValue

    def toString(self) -> str:
        return self.text or repr(self.value)

    def getTime(self) -> int | float | None:
        return self.value


class HLAinteger64Time(LogicalTime):
    """Provider-created signed 64-bit reference logical time."""


class HLAfloat64Time(LogicalTime):
    """Provider-created IEEE-754 double reference logical time."""


@dataclass(frozen=True, slots=True)
class LogicalTimeInterval:
    """Provider-created logical-time interval value."""

    encodedValue: BytesLike
    implementationNameValue: str
    zero: bool = False
    epsilon: bool = False
    value: int | float | None = None
    text: str = ""

    def __post_init__(self) -> None:
        object.__setattr__(self, "encodedValue", copy_bytes(self.encodedValue, name="encodedValue"))

    def encodedLength(self) -> int:
        return len(self.encodedValue)

    def encode(self, buffer: WritableBytes, offset: int = 0) -> None:
        if offset < 0 or offset + len(self.encodedValue) > len(buffer):
            raise ValueError(f"buffer does not have room for {type(self).__name__}")
        buffer[offset : offset + len(self.encodedValue)] = self.encodedValue

    def toByteArray(self) -> bytes:
        return self.encodedValue

    def isZero(self) -> bool:
        return self.zero

    def isEpsilon(self) -> bool:
        return self.epsilon

    def implementationName(self) -> str:
        return self.implementationNameValue

    def toString(self) -> str:
        return self.text or repr(self.value)

    def getInterval(self) -> int | float | None:
        return self.value


class HLAinteger64Interval(LogicalTimeInterval):
    """Provider-created signed 64-bit reference logical-time interval."""


class HLAfloat64Interval(LogicalTimeInterval):
    """Provider-created IEEE-754 double reference logical-time interval."""


class AttributeHandleSet(frozenset[AttributeHandle]):
    """Immutable Python input/snapshot form of an attribute-handle set."""


class AttributeHandleValueMap(Mapping[AttributeHandle, bytes]):
    """Immutable, byte-copying form of the standard attribute-value map."""

    __slots__ = ("_values",)

    def __init__(
        self,
        values: (
            Mapping[AttributeHandle, BytesLike]
            | Iterable[tuple[AttributeHandle, BytesLike]]
        ) = (),
    ) -> None:
        copied: dict[AttributeHandle, bytes] = {}
        for attribute, value in dict(values).items():
            if not isinstance(attribute, AttributeHandle):
                raise TypeError("AttributeHandleValueMap keys must be AttributeHandle values")
            copied[attribute] = bytes(value)
        self._values = copied

    def __getitem__(self, attribute: AttributeHandle) -> bytes:
        return self._values[attribute]

    def __iter__(self) -> Iterator[AttributeHandle]:
        return iter(self._values)

    def __len__(self) -> int:
        return len(self._values)

    def __repr__(self) -> str:
        return f"{type(self).__name__}({self._values!r})"

    def __eq__(self, other: object) -> bool:
        if not isinstance(other, Mapping):
            return NotImplemented
        return self._values == dict(other.items())


class ParameterHandleValueMap(Mapping[ParameterHandle, bytes]):
    """Immutable, byte-copying form of the standard parameter-value map."""

    __slots__ = ("_values",)

    def __init__(
        self,
        values: (
            Mapping[ParameterHandle, BytesLike]
            | Iterable[tuple[ParameterHandle, BytesLike]]
        ) = (),
    ) -> None:
        copied: dict[ParameterHandle, bytes] = {}
        for parameter, value in dict(values).items():
            if not isinstance(parameter, ParameterHandle):
                raise TypeError("ParameterHandleValueMap keys must be ParameterHandle values")
            copied[parameter] = bytes(value)
        self._values = copied

    def __getitem__(self, parameter: ParameterHandle) -> bytes:
        return self._values[parameter]

    def __iter__(self) -> Iterator[ParameterHandle]:
        return iter(self._values)

    def __len__(self) -> int:
        return len(self._values)

    def __repr__(self) -> str:
        return f"{type(self).__name__}({self._values!r})"

    def __eq__(self, other: object) -> bool:
        if not isinstance(other, Mapping):
            return NotImplemented
        return self._values == dict(other.items())


class FederateHandleSet(frozenset[FederateHandle]):
    """Immutable Python snapshot of a standard federate-handle set."""


@dataclass(frozen=True, slots=True)
class FederateHandleSaveStatusPair:
    """One standard save-status response entry, preserving its federate handle."""

    federateHandle: FederateHandle
    saveStatus: SaveStatus


@dataclass(frozen=True, slots=True)
class FederateRestoreStatus:
    """One immutable standard federation-restore status record."""

    preRestoreHandle: FederateHandle
    postRestoreHandle: FederateHandle
    status: RestoreStatus


class RtiConfiguration:
    """Java-shaped mutable connection configuration value."""

    def __init__(self) -> None:
        self._configuration_name = ""
        self._rti_address = ""
        self._additional_settings = ""

    @classmethod
    def createConfiguration(cls) -> RtiConfiguration:
        return cls()

    def withConfigurationName(self, configurationName: str) -> RtiConfiguration:
        self._configuration_name = str(configurationName)
        return self

    def withRtiAddress(self, rtiAddress: str) -> RtiConfiguration:
        self._rti_address = str(rtiAddress)
        return self

    def withAdditionalSettings(self, additionalSettings: str) -> RtiConfiguration:
        self._additional_settings = str(additionalSettings)
        return self

    def configurationName(self) -> str:
        return self._configuration_name

    def rtiAddress(self) -> str:
        return self._rti_address

    def additionalSettings(self) -> str:
        return self._additional_settings


__all__ = [
    "AdditionalSettingsResultCode",
    "CallbackModel",
    "OrderType",
    "ServiceGroup",
    "ResignAction",
    "SynchronizationPointFailureReason",
    "SaveStatus",
    "SaveFailureReason",
    "RestoreFailureReason",
    "RestoreStatus",
    "ConfigurationResult",
    "TimeQueryResult",
    "FederationExecutionInformation",
    "FederationExecutionInformationSet",
    "FederationExecutionMemberInformation",
    "FederationExecutionMemberInformationSet",
    "EncodedHandle",
    "FederateHandle",
    "ObjectClassHandle",
    "ObjectInstanceHandle",
    "AttributeHandle",
    "InteractionClassHandle",
    "ParameterHandle",
    "TransportationTypeHandle",
    "DimensionHandle",
    "RegionHandle",
    "MessageRetractionHandle",
    "MutableAttributeHandleSet",
    "MutableDimensionHandleSet",
    "MutableFederateHandleSet",
    "MutableRegionHandleSet",
    "MutableAttributeHandleValueMap",
    "MutableParameterHandleValueMap",
    "DimensionHandleSet",
    "RegionHandleSet",
    "InteractionClassHandleSet",
    "MutableInteractionClassHandleSet",
    "ObjectInstanceNameSet",
    "AttributeSetRegionSetPair",
    "AttributeRegionAssociation",
    "AttributeSetRegionSetPairList",
    "MutableAttributeSetRegionSetPairList",
    "AttributeHandleSetRegionHandleSetPair",
    "AttributeHandleSetRegionHandleSetPairVector",
    "RangeBounds",
    "LogicalTime",
    "HLAinteger64Time",
    "HLAfloat64Time",
    "LogicalTimeInterval",
    "HLAinteger64Interval",
    "HLAfloat64Interval",
    "AttributeHandleSet",
    "AttributeHandleValueMap",
    "ParameterHandleValueMap",
    "FederateHandleSet",
    "FederateHandleSaveStatusPair",
    "FederateRestoreStatus",
    "RtiConfiguration",
]
