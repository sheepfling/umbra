"""Implemented foundation of the IEEE 1516.1-2025 Java-shaped API."""

from __future__ import annotations

from abc import ABC, abstractmethod
from collections.abc import Iterable, Iterator, Mapping, MutableMapping, MutableSet
from dataclasses import dataclass
from enum import Enum
from importlib.metadata import entry_points
import os
from typing import TYPE_CHECKING, Any, TypeVar

from .exceptions import RTIinternalError, UnsupportedCallbackModel

if TYPE_CHECKING:
    from .encoding import EncoderFactory


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


def _require_callback_model(value: object) -> CallbackModel:
    """Reject values outside the two standard callback-model enum members.

    Python does not enforce annotations at a call boundary. Providers therefore
    share this guard so an invalid value has the same observable outcome as the
    C++ binding's ``UnsupportedCallbackModel`` service error.
    """

    if not isinstance(value, CallbackModel):
        raise UnsupportedCallbackModel(f"Unsupported callback model: {value!r}")
    return value


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
    time: "LogicalTime | None"

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

    encodedValue: bytes

    def __post_init__(self) -> None:
        object.__setattr__(self, "encodedValue", bytes(self.encodedValue))

    def encodedLength(self) -> int:
        return len(self.encodedValue)

    def encode(self, buffer: bytearray, offset: int = 0) -> None:
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


class HandleFactory(ABC):
    """Provider-owned decoder for one opaque handle domain."""

    @abstractmethod
    def decode(self, encodedValue: bytes) -> EncodedHandle:
        """Decode and validate an encoded handle through the provider."""


class FederateHandleFactory(HandleFactory):
    """Factory for provider-owned :class:`FederateHandle` values."""


class ObjectClassHandleFactory(HandleFactory):
    """Factory for provider-owned :class:`ObjectClassHandle` values."""


class ObjectInstanceHandleFactory(HandleFactory):
    """Factory for provider-owned :class:`ObjectInstanceHandle` values."""


class AttributeHandleFactory(HandleFactory):
    """Factory for provider-owned :class:`AttributeHandle` values."""


class InteractionClassHandleFactory(HandleFactory):
    """Factory for provider-owned :class:`InteractionClassHandle` values."""


class ParameterHandleFactory(HandleFactory):
    """Factory for provider-owned :class:`ParameterHandle` values."""


class TransportationTypeHandleFactory(HandleFactory):
    """Factory for provider-owned :class:`TransportationTypeHandle` values."""


class DimensionHandleFactory(HandleFactory):
    """Factory for provider-owned :class:`DimensionHandle` values."""


class RegionHandleFactory(HandleFactory):
    """Factory for provider-owned :class:`RegionHandle` values."""


class MessageRetractionHandleFactory(HandleFactory):
    """Factory for provider-owned timestamped message-retraction handles."""


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
        values: Mapping[_HandleT, bytes | bytearray | memoryview]
        | Iterable[tuple[_HandleT, bytes | bytearray | memoryview]] = (),
    ) -> None:
        self._values: dict[_HandleT, bytes] = {}
        for handle, value in dict(values).items():
            self[handle] = value

    def __getitem__(self, handle: _HandleT) -> bytes:
        return self._values[handle]

    def __setitem__(self, handle: _HandleT, value: bytes | bytearray | memoryview) -> None:
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


class AttributeHandleSetFactory(ABC):
    """Provider-owned factory for mutable attribute-handle sets."""

    @abstractmethod
    def create(self) -> MutableAttributeHandleSet:
        """Create an empty mutable attribute-handle set."""


class DimensionHandleSetFactory(ABC):
    """Provider-owned factory for mutable dimension-handle sets."""

    @abstractmethod
    def create(self) -> MutableDimensionHandleSet:
        """Create an empty mutable dimension-handle set."""


class FederateHandleSetFactory(ABC):
    """Provider-owned factory for mutable federate-handle sets."""

    @abstractmethod
    def create(self) -> MutableFederateHandleSet:
        """Create an empty mutable federate-handle set."""


class RegionHandleSetFactory(ABC):
    """Provider-owned factory for mutable region-handle sets."""

    @abstractmethod
    def create(self) -> MutableRegionHandleSet:
        """Create an empty mutable region-handle set."""


class InteractionClassHandleSetFactory(ABC):
    """Provider-owned factory for mutable interaction-class handle sets."""

    @abstractmethod
    def create(self) -> MutableInteractionClassHandleSet:
        """Create an empty mutable interaction-class handle set."""


class AttributeHandleValueMapFactory(ABC):
    """Provider-owned factory for mutable attribute-value maps."""

    @abstractmethod
    def create(self) -> MutableAttributeHandleValueMap:
        """Create an empty mutable attribute-value map."""


class ParameterHandleValueMapFactory(ABC):
    """Provider-owned factory for mutable parameter-value maps."""

    @abstractmethod
    def create(self) -> MutableParameterHandleValueMap:
        """Create an empty mutable parameter-value map."""


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


class AttributeSetRegionSetPairList(tuple[AttributeSetRegionSetPair, ...]):
    """Immutable Java-shaped list of attribute-set/region-set associations."""

    def __new__(
        cls,
        values: Iterable[AttributeSetRegionSetPair | AttributeRegionAssociation] = (),
    ) -> "AttributeSetRegionSetPairList":
        copied = tuple(values)
        for value in copied:
            if not isinstance(value, (AttributeSetRegionSetPair, AttributeRegionAssociation)):
                raise TypeError(
                    "pair list values must be AttributeSetRegionSetPair or AttributeRegionAssociation"
                )
        return tuple.__new__(cls, copied)


class MutableAttributeSetRegionSetPairList(
    list[AttributeSetRegionSetPair | AttributeRegionAssociation]
):
    """Mutable list returned by ``AttributeSetRegionSetPairListFactory``.

    The Java factory's integer argument is a capacity hint, not an initial
    element count, so it is intentionally ignored after validation.
    """

    def __init__(
        self,
        capacity: int | Iterable[AttributeSetRegionSetPair | AttributeRegionAssociation] = 0,
        values: Iterable[AttributeSetRegionSetPair | AttributeRegionAssociation] = (),
    ) -> None:
        if not isinstance(capacity, int):
            values = capacity
            capacity = 0
        if capacity < 0:
            raise ValueError("capacity must be non-negative")
        super().__init__()
        for value in values:
            self.add(value)

    def add(self, value: AttributeSetRegionSetPair | AttributeRegionAssociation) -> None:
        if not isinstance(value, (AttributeSetRegionSetPair, AttributeRegionAssociation)):
            raise TypeError(
                "pair list values must be AttributeSetRegionSetPair or AttributeRegionAssociation"
            )
        self.append(value)


class AttributeSetRegionSetPairListFactory(ABC):
    """Provider-owned factory for Java-shaped attribute/region pair lists."""

    @abstractmethod
    def create(self, capacity: int = 0) -> MutableAttributeSetRegionSetPairList:
        """Create an empty list with the supplied Java capacity hint."""


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

    encodedValue: bytes
    implementationNameValue: str
    initial: bool = False
    final: bool = False
    value: int | float | None = None
    text: str = ""

    def __post_init__(self) -> None:
        object.__setattr__(self, "encodedValue", bytes(self.encodedValue))

    def encodedLength(self) -> int:
        return len(self.encodedValue)

    def encode(self, buffer: bytearray, offset: int = 0) -> None:
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

    encodedValue: bytes
    implementationNameValue: str
    zero: bool = False
    epsilon: bool = False
    value: int | float | None = None
    text: str = ""

    def __post_init__(self) -> None:
        object.__setattr__(self, "encodedValue", bytes(self.encodedValue))

    def encodedLength(self) -> int:
        return len(self.encodedValue)

    def encode(self, buffer: bytearray, offset: int = 0) -> None:
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


class LogicalTimeFactory(ABC):
    """Provider-owned factory for the selected federation logical-time type."""

    @abstractmethod
    def implementationName(self) -> str:
        """Return the selected implementation name."""

    @abstractmethod
    def makeInitial(self) -> LogicalTime:
        """Create the provider's initial logical time."""

    @abstractmethod
    def makeFinal(self) -> LogicalTime:
        """Create the provider's final logical time."""

    @abstractmethod
    def makeZero(self) -> LogicalTimeInterval:
        """Create the provider's zero interval."""

    @abstractmethod
    def makeEpsilon(self) -> LogicalTimeInterval:
        """Create the provider's epsilon interval."""

    @abstractmethod
    def decodeLogicalTime(self, encodedValue: bytes) -> LogicalTime:
        """Decode an encoded logical time through the provider."""

    @abstractmethod
    def decodeLogicalTimeInterval(self, encodedValue: bytes) -> LogicalTimeInterval:
        """Decode an encoded logical-time interval through the provider."""

    @abstractmethod
    def add(self, time: LogicalTime, addend: LogicalTimeInterval) -> LogicalTime:
        """Apply a provider's logical-time addition semantics and return a snapshot."""

    @abstractmethod
    def subtract(self, time: LogicalTime, subtrahend: LogicalTimeInterval) -> LogicalTime:
        """Apply a provider's logical-time subtraction semantics and return a snapshot."""

    @abstractmethod
    def difference(
        self, minuend: LogicalTime, subtrahend: LogicalTime
    ) -> LogicalTimeInterval:
        """Return a provider-computed nonnegative difference between two times."""


class HLAinteger64TimeFactory(LogicalTimeFactory, ABC):
    """Concrete factory contract for the standard integer reference time."""

    @abstractmethod
    def makeLogicalTime(self, value: int) -> HLAinteger64Time:
        """Create an integer reference time through the provider."""

    @abstractmethod
    def makeLogicalTimeInterval(self, value: int) -> HLAinteger64Interval:
        """Create an integer reference interval through the provider."""


class HLAfloat64TimeFactory(LogicalTimeFactory, ABC):
    """Concrete factory contract for the standard floating reference time."""

    @abstractmethod
    def makeLogicalTime(self, value: float) -> HLAfloat64Time:
        """Create a floating reference time through the provider."""

    @abstractmethod
    def makeLogicalTimeInterval(self, value: float) -> HLAfloat64Interval:
        """Create a floating reference interval through the provider."""


class AttributeHandleSet(frozenset[AttributeHandle]):
    """Immutable Python input/snapshot form of an attribute-handle set."""


class AttributeHandleValueMap(Mapping[AttributeHandle, bytes]):
    """Immutable, byte-copying form of the standard attribute-value map."""

    __slots__ = ("_values",)

    def __init__(
        self,
        values: (
            Mapping[AttributeHandle, bytes | bytearray | memoryview]
            | Iterable[tuple[AttributeHandle, bytes | bytearray | memoryview]]
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
            Mapping[ParameterHandle, bytes | bytearray | memoryview]
            | Iterable[tuple[ParameterHandle, bytes | bytearray | memoryview]]
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
    def createConfiguration(cls) -> "RtiConfiguration":
        return cls()

    def withConfigurationName(self, configurationName: str) -> "RtiConfiguration":
        self._configuration_name = str(configurationName)
        return self

    def withRtiAddress(self, rtiAddress: str) -> "RtiConfiguration":
        self._rti_address = str(rtiAddress)
        return self

    def withAdditionalSettings(self, additionalSettings: str) -> "RtiConfiguration":
        self._additional_settings = str(additionalSettings)
        return self

    def configurationName(self) -> str:
        return self._configuration_name

    def rtiAddress(self) -> str:
        return self._rti_address

    def additionalSettings(self) -> str:
        return self._additional_settings


def _resolve_connect_arguments(
    configuration: RtiConfiguration | object | None,
    credentials: object | None,
) -> tuple[RtiConfiguration | None, object | None]:
    """Adapt Java's connect overloads to Python's optional arguments."""

    from .auth import Credentials

    if isinstance(configuration, Credentials):
        if credentials is not None:
            raise TypeError("credentials were supplied twice")
        return None, configuration
    if configuration is not None and not isinstance(configuration, RtiConfiguration):
        raise TypeError("configuration must be RtiConfiguration or Credentials")
    if credentials is not None and not isinstance(credentials, Credentials):
        raise TypeError("credentials must be Credentials")
    return configuration, credentials


class FederateAmbassador(ABC):
    """Receives RTI callbacks; callback families arrive with native services."""

    def connectionLost(self, faultDescription: str) -> None:
        """Report connection loss when the provider supports that service."""

    def reportFederationExecutions(self, report: FederationExecutionInformationSet) -> None:
        """Report the federation executions requested through the RTI."""

    def reportFederationExecutionMembers(
        self,
        federationExecutionName: str,
        report: FederationExecutionMemberInformationSet,
    ) -> None:
        """Report federation members requested through the RTI."""

    def reportFederationExecutionDoesNotExist(self, federationExecutionName: str) -> None:
        """Report that a requested federation execution does not exist."""

    def federateResigned(self, reasonForResignDescription: str) -> None:
        """Report an RTI-originated removal from federation membership."""

    def synchronizationPointRegistrationSucceeded(self, synchronizationPointLabel: str) -> None:
        """Report successful synchronization-point registration."""

    def synchronizationPointRegistrationFailed(
        self,
        synchronizationPointLabel: str,
        reason: SynchronizationPointFailureReason,
    ) -> None:
        """Report registration failure through a typed standard reason."""

    def announceSynchronizationPoint(self, synchronizationPointLabel: str, userSuppliedTag: bytes) -> None:
        """Announce a synchronization point and its copied byte tag."""

    def federationSynchronized(
        self,
        synchronizationPointLabel: str,
        failedToSyncSet: FederateHandleSet,
    ) -> None:
        """Report that a synchronization point completed for this federate."""

    def federationSaveStatusResponse(
        self,
        response: tuple[FederateHandleSaveStatusPair, ...],
    ) -> None:
        """Report immutable ordered federation save-status entries."""

    def initiateFederateSave(self, label: str, time: LogicalTime | None = None) -> None:
        """Instruct this federate to begin a scalar or timestamped federation save."""

    def federationSaved(self) -> None:
        """Report successful completion of the current federation save."""

    def federationNotSaved(self, reason: SaveFailureReason) -> None:
        """Report failed or aborted federation save with a typed reason."""

    def federationRestoreStatusResponse(self, response: tuple[FederateRestoreStatus, ...]) -> None:
        """Report immutable ordered federation-restore status entries."""

    def requestFederationRestoreSucceeded(self, label: str) -> None:
        """Report acceptance of a federation restore request."""

    def requestFederationRestoreFailed(self, label: str) -> None:
        """Report that no matching federation restore snapshot is available."""

    def federationRestoreBegun(self) -> None:
        """Report that federation restore has begun."""

    def initiateFederateRestore(
        self, label: str, federateName: str, postRestoreFederateHandle: FederateHandle
    ) -> None:
        """Instruct this federate to restore with its post-restore handle."""

    def federationRestored(self) -> None:
        """Report successful completion of the current federation restore."""

    def federationNotRestored(self, reason: RestoreFailureReason) -> None:
        """Report failed or aborted federation restore with a typed reason."""

    def startRegistrationForObjectClass(self, objectClass: ObjectClassHandle) -> None:
        """Advise a publisher that object registration is now useful."""

    def stopRegistrationForObjectClass(self, objectClass: ObjectClassHandle) -> None:
        """Advise a publisher that object registration is no longer useful."""

    def turnInteractionsOn(self, interactionClass: InteractionClassHandle) -> None:
        """Advise a publisher that an interaction has an active subscriber."""

    def turnInteractionsOff(self, interactionClass: InteractionClassHandle) -> None:
        """Advise a publisher that an interaction has no active subscribers."""

    def discoverObjectInstance(
        self,
        objectInstance: ObjectInstanceHandle,
        objectClass: ObjectClassHandle,
        objectInstanceName: str,
        producingFederate: FederateHandle,
    ) -> None:
        """Report a newly known object instance with typed source handles."""

    def objectInstanceNameReservationSucceeded(self, objectInstanceName: str) -> None:
        """Report successful reservation of an object-instance name."""

    def objectInstanceNameReservationFailed(self, objectInstanceName: str) -> None:
        """Report that an object-instance name could not be reserved."""

    def multipleObjectInstanceNameReservationSucceeded(
        self, objectInstanceNames: ObjectInstanceNameSet
    ) -> None:
        """Report successful reservation of a batch of object-instance names."""

    def multipleObjectInstanceNameReservationFailed(
        self, objectInstanceNames: ObjectInstanceNameSet
    ) -> None:
        """Report that a batch of object-instance names could not be reserved."""

    def provideAttributeValueUpdate(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
        userSuppliedTag: bytes,
    ) -> None:
        """Request current values from the owner of an object instance."""

    def attributesInScope(
        self, objectInstance: ObjectInstanceHandle, attributes: AttributeHandleSet
    ) -> None:
        """Report attributes that became relevant for an object instance."""

    def attributesOutOfScope(
        self, objectInstance: ObjectInstanceHandle, attributes: AttributeHandleSet
    ) -> None:
        """Report attributes that are no longer relevant for an object instance."""

    def turnUpdatesOnForObjectInstance(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
        updateRateDesignator: str | None = None,
    ) -> None:
        """Report that updates are relevant, optionally at a named rate."""

    def turnUpdatesOffForObjectInstance(
        self, objectInstance: ObjectInstanceHandle, attributes: AttributeHandleSet
    ) -> None:
        """Report that updates are no longer relevant for an object instance."""

    def confirmAttributeTransportationTypeChange(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
        transportationType: TransportationTypeHandle,
    ) -> None:
        """Confirm an attribute transportation-type change."""

    def reportAttributeTransportationType(
        self,
        objectInstance: ObjectInstanceHandle,
        attribute: AttributeHandle,
        transportationType: TransportationTypeHandle,
    ) -> None:
        """Report the transportation type currently used by an attribute."""

    def confirmInteractionTransportationTypeChange(
        self,
        interactionClass: InteractionClassHandle,
        transportationType: TransportationTypeHandle,
    ) -> None:
        """Confirm an interaction transportation-type change."""

    def reportInteractionTransportationType(
        self,
        federate: FederateHandle,
        interactionClass: InteractionClassHandle,
        transportationType: TransportationTypeHandle,
    ) -> None:
        """Report the transportation type used by an interaction."""

    def removeObjectInstance(
        self,
        objectInstance: ObjectInstanceHandle,
        userSuppliedTag: bytes,
        producingFederate: FederateHandle,
        time: LogicalTime | None = None,
        sentOrderType: OrderType | None = None,
        receivedOrderType: OrderType | None = None,
        retraction: MessageRetractionHandle | None = None,
    ) -> None:
        """Report removal, optionally including timestamp/order metadata."""

    def reflectAttributeValues(
        self,
        objectInstance: ObjectInstanceHandle,
        attributeValues: AttributeHandleValueMap,
        userSuppliedTag: bytes,
        transportationType: TransportationTypeHandle,
        producingFederate: FederateHandle,
        sentRegions: RegionHandleSet | None = None,
        time: LogicalTime | None = None,
        sentOrderType: OrderType | None = None,
        receivedOrderType: OrderType | None = None,
        retraction: MessageRetractionHandle | None = None,
    ) -> None:
        """Report values and optional regions/timestamp ordering metadata."""

    def receiveInteraction(
        self,
        interactionClass: InteractionClassHandle,
        parameterValues: ParameterHandleValueMap,
        userSuppliedTag: bytes,
        transportationType: TransportationTypeHandle,
        producingFederate: FederateHandle,
        sentRegions: RegionHandleSet | None = None,
        time: LogicalTime | None = None,
        sentOrderType: OrderType | None = None,
        receivedOrderType: OrderType | None = None,
        retraction: MessageRetractionHandle | None = None,
    ) -> None:
        """Report an interaction and optional regions/timestamp metadata."""

    def receiveDirectedInteraction(
        self,
        interactionClass: InteractionClassHandle,
        objectInstance: ObjectInstanceHandle,
        parameterValues: ParameterHandleValueMap,
        userSuppliedTag: bytes,
        transportationType: TransportationTypeHandle,
        producingFederate: FederateHandle,
        time: LogicalTime | None = None,
        sentOrderType: OrderType | None = None,
        receivedOrderType: OrderType | None = None,
        retraction: MessageRetractionHandle | None = None,
    ) -> None:
        """Report a directed interaction and optional timestamp/order metadata."""

    def requestAttributeOwnershipAssumption(
        self,
        objectInstance: ObjectInstanceHandle,
        offeredAttributes: AttributeHandleSet,
        userSuppliedTag: bytes,
    ) -> None:
        """Request that this federate assume offered attribute ownership."""

    def requestDivestitureConfirmation(
        self,
        objectInstance: ObjectInstanceHandle,
        releasedAttributes: AttributeHandleSet,
        userSuppliedTag: bytes,
    ) -> None:
        """Confirm that a negotiated divestiture may complete."""

    def attributeOwnershipAcquisitionNotification(
        self,
        objectInstance: ObjectInstanceHandle,
        securedAttributes: AttributeHandleSet,
        userSuppliedTag: bytes,
    ) -> None:
        """Report attributes secured by an ownership acquisition."""

    def attributeOwnershipUnavailable(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
        userSuppliedTag: bytes,
    ) -> None:
        """Report attributes unavailable for acquisition."""

    def requestAttributeOwnershipRelease(
        self,
        objectInstance: ObjectInstanceHandle,
        candidateAttributes: AttributeHandleSet,
        userSuppliedTag: bytes,
    ) -> None:
        """Request release of candidate attributes."""

    def confirmAttributeOwnershipAcquisitionCancellation(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
    ) -> None:
        """Report cancellation of an ownership acquisition."""

    def informAttributeOwnership(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
        owner: FederateHandle,
    ) -> None:
        """Report the federate that owns the supplied attributes."""

    def attributeIsNotOwned(
        self, objectInstance: ObjectInstanceHandle, attributes: AttributeHandleSet
    ) -> None:
        """Report that the supplied attributes are unowned."""

    def attributeIsOwnedByRTI(
        self, objectInstance: ObjectInstanceHandle, attributes: AttributeHandleSet
    ) -> None:
        """Report that the RTI owns the supplied attributes."""

    def timeRegulationEnabled(self, time: LogicalTime) -> None:
        """Report that time regulation was enabled at a provider-created time."""

    def timeConstrainedEnabled(self, time: LogicalTime) -> None:
        """Report that time constrained mode was enabled at a provider-created time."""

    def flushQueueGrant(self, time: LogicalTime, optimisticTime: LogicalTime) -> None:
        """Report completion of a flush-queue request and its optimistic bound."""

    def timeAdvanceGrant(self, time: LogicalTime) -> None:
        """Report completion of a time-advance request."""

    def requestRetraction(self, retraction: MessageRetractionHandle) -> None:
        """Request that a previously delivered timestamped message be retracted."""


class RTIambassador(ABC):
    """The currently implemented connected/unjoined subset of the standard API."""

    @abstractmethod
    def getHLAversion(self) -> str:
        """Return the standard HLA API version reported by the provider."""

    @abstractmethod
    def connect(
        self,
        federateAmbassador: FederateAmbassador,
        callbackModel: CallbackModel,
        configuration: RtiConfiguration | object | None = None,
        credentials: object | None = None,
    ) -> ConfigurationResult:
        """Connect using the Java overload represented by the supplied values."""

    @abstractmethod
    def disconnect(self) -> None:
        """Disconnect this ambassador."""

    @abstractmethod
    def evokeCallback(self, approximateMinimumTimeInSeconds: float) -> bool:
        """Deliver at most one queued callback."""

    @abstractmethod
    def evokeMultipleCallbacks(
        self,
        approximateMinimumTimeInSeconds: float,
        approximateMaximumTimeInSeconds: float,
    ) -> bool:
        """Deliver queued callbacks during the requested interval."""

    @abstractmethod
    def enableCallbacks(self) -> None:
        """Enable callback delivery."""

    @abstractmethod
    def disableCallbacks(self) -> None:
        """Disable callback delivery without discarding queued callbacks."""

    @abstractmethod
    def listFederationExecutions(self) -> None:
        """Request ``reportFederationExecutions`` through the callback model."""

    @abstractmethod
    def listFederationExecutionMembers(self, federationExecutionName: str) -> None:
        """Request member reporting or the missing-federation callback."""

    @abstractmethod
    def joinFederationExecution(
        self,
        federateType: str,
        federationExecutionName: str,
        *,
        federateName: str | None = None,
        additionalFomModules: Iterable[str] = (),
    ) -> FederateHandle:
        """Join through either Java name overload, optionally adding FOM URLs."""

    @abstractmethod
    def resignFederationExecution(self, resignAction: ResignAction) -> None:
        """Resign from the currently joined federation execution."""

    @abstractmethod
    def registerFederationSynchronizationPoint(
        self,
        synchronizationPointLabel: str,
        userSuppliedTag: bytes = b"",
        *,
        synchronizationSet: FederateHandleSet | None = None,
    ) -> None:
        """Register a point for current members or an explicit federate-handle set."""

    @abstractmethod
    def synchronizationPointAchieved(
        self,
        synchronizationPointLabel: str,
        successfully: bool = True,
    ) -> None:
        """Report this federate's result for an announced synchronization point."""

    @abstractmethod
    def queryFederationSaveStatus(self) -> None:
        """Request ``federationSaveStatusResponse`` for the joined federation."""

    @abstractmethod
    def requestFederationSave(self, label: str, time: LogicalTime | None = None) -> None:
        """Request an untimed or timestamped federation save."""

    @abstractmethod
    def federateSaveBegun(self) -> None:
        """Report that this federate has begun the instructed save."""

    @abstractmethod
    def federateSaveComplete(self) -> None:
        """Report that this federate completed the instructed save."""

    @abstractmethod
    def federateSaveNotComplete(self) -> None:
        """Report that this federate cannot complete the instructed save."""

    @abstractmethod
    def abortFederationSave(self) -> None:
        """Abort the currently active federation save."""

    @abstractmethod
    def queryFederationRestoreStatus(self) -> None:
        """Request ``federationRestoreStatusResponse`` for the joined federation."""

    @abstractmethod
    def requestFederationRestore(self, label: str) -> None:
        """Request restore from a completed scalar federation save."""

    @abstractmethod
    def federateRestoreComplete(self) -> None:
        """Report that this federate completed the instructed restore."""

    @abstractmethod
    def federateRestoreNotComplete(self) -> None:
        """Report that this federate cannot complete the instructed restore."""

    @abstractmethod
    def abortFederationRestore(self) -> None:
        """Abort the currently active federation restore."""

    @abstractmethod
    def getObjectClassHandle(self, objectClassName: str) -> ObjectClassHandle:
        """Resolve a FOM object-class name through the joined RTI."""

    @abstractmethod
    def getFederateHandle(self, federateName: str) -> FederateHandle:
        """Resolve an active federate name through the joined RTI."""

    @abstractmethod
    def getFederateName(self, federate: FederateHandle) -> str:
        """Resolve a federate handle through the joined RTI."""

    @abstractmethod
    def getObjectClassName(self, objectClass: ObjectClassHandle) -> str:
        """Resolve a portable object-class handle through the joined RTI."""

    @abstractmethod
    def getKnownObjectClassHandle(self, objectInstance: ObjectInstanceHandle) -> ObjectClassHandle:
        """Resolve the known object class for an object instance."""

    @abstractmethod
    def getAttributeHandle(
        self, objectClass: ObjectClassHandle, attributeName: str
    ) -> AttributeHandle:
        """Resolve an attribute name in its object-class domain."""

    @abstractmethod
    def getAttributeName(
        self, objectClass: ObjectClassHandle, attribute: AttributeHandle
    ) -> str:
        """Resolve an attribute handle in its object-class domain."""

    @abstractmethod
    def getUpdateRateValue(self, updateRateDesignator: str) -> float:
        """Resolve an FDD update-rate designator to seconds per update."""

    @abstractmethod
    def getUpdateRateValueForAttribute(
        self, objectInstance: ObjectInstanceHandle, attribute: AttributeHandle
    ) -> float:
        """Resolve the effective update rate for a known object attribute."""

    @abstractmethod
    def getInteractionClassHandle(self, interactionClassName: str) -> InteractionClassHandle:
        """Resolve a FOM interaction-class name through the joined RTI."""

    @abstractmethod
    def getInteractionClassName(self, interactionClass: InteractionClassHandle) -> str:
        """Resolve a portable interaction-class handle through the joined RTI."""

    @abstractmethod
    def getParameterHandle(
        self, interactionClass: InteractionClassHandle, parameterName: str
    ) -> ParameterHandle:
        """Resolve a parameter name in its interaction-class domain."""

    @abstractmethod
    def getParameterName(
        self, interactionClass: InteractionClassHandle, parameter: ParameterHandle
    ) -> str:
        """Resolve a parameter handle in its interaction-class domain."""

    @abstractmethod
    def getOrderType(self, orderTypeName: str) -> OrderType:
        """Resolve the standard order-type name."""

    @abstractmethod
    def getOrderName(self, orderType: OrderType) -> str:
        """Resolve a standard order type to its FOM name."""

    @abstractmethod
    def getTransportationTypeHandle(
        self, transportationTypeName: str
    ) -> TransportationTypeHandle:
        """Resolve a provider-supported transportation-type name."""

    @abstractmethod
    def getTransportationTypeName(self, transportationType: TransportationTypeHandle) -> str:
        """Resolve a portable transportation-type handle through the RTI."""

    @abstractmethod
    def getDimensionHandle(self, dimensionName: str) -> DimensionHandle:
        """Resolve a FOM routing-space dimension name through the joined RTI."""

    @abstractmethod
    def getDimensionName(self, dimension: DimensionHandle) -> str:
        """Resolve a portable dimension handle through the joined RTI."""

    @abstractmethod
    def getAvailableDimensionsForObjectClass(
        self, objectClass: ObjectClassHandle
    ) -> DimensionHandleSet:
        """Return dimensions available to an object class."""

    @abstractmethod
    def getAvailableDimensionsForInteractionClass(
        self, interactionClass: InteractionClassHandle
    ) -> DimensionHandleSet:
        """Return dimensions available to an interaction class."""

    @abstractmethod
    def getDimensionUpperBound(self, dimension: DimensionHandle) -> int:
        """Return the provider-defined upper bound for a dimension."""

    @abstractmethod
    def normalizeServiceGroup(self, serviceGroup: ServiceGroup) -> int:
        """Return the standard normalized coordinate for a service group."""

    @abstractmethod
    def normalizeFederateHandle(self, federate: FederateHandle) -> int:
        """Return the provider normalized coordinate for a federate handle."""

    @abstractmethod
    def normalizeObjectClassHandle(self, objectClass: ObjectClassHandle) -> int:
        """Return the provider normalized coordinate for an object class."""

    @abstractmethod
    def normalizeInteractionClassHandle(self, interactionClass: InteractionClassHandle) -> int:
        """Return the provider normalized coordinate for an interaction class."""

    @abstractmethod
    def normalizeObjectInstanceHandle(self, objectInstance: ObjectInstanceHandle) -> int:
        """Return the provider normalized coordinate for an object instance."""

    @abstractmethod
    def publishObjectClassAttributes(
        self, objectClass: ObjectClassHandle, attributes: AttributeHandleSet
    ) -> None:
        """Publish a set of attributes for a FOM object class."""

    @abstractmethod
    def unpublishObjectClass(self, objectClass: ObjectClassHandle) -> None:
        """Stop publishing all attributes of a FOM object class."""

    @abstractmethod
    def unpublishObjectClassAttributes(
        self, objectClass: ObjectClassHandle, attributes: AttributeHandleSet
    ) -> None:
        """Stop publishing the supplied object-class attributes."""

    @abstractmethod
    def subscribeObjectClassAttributes(
        self,
        objectClass: ObjectClassHandle,
        attributes: AttributeHandleSet,
        *,
        active: bool = True,
        updateRateDesignator: str = "",
    ) -> None:
        """Subscribe actively or passively to an object-class attribute set."""

    @abstractmethod
    def subscribeObjectClassAttributesPassively(
        self,
        objectClass: ObjectClassHandle,
        attributes: AttributeHandleSet,
        updateRateDesignator: str = "",
    ) -> None:
        """Exact Java standard passive object-attribute subscription overload."""

    @abstractmethod
    def unsubscribeObjectClass(self, objectClass: ObjectClassHandle) -> None:
        """Remove all subscriptions for a FOM object class."""

    @abstractmethod
    def unsubscribeObjectClassAttributes(
        self, objectClass: ObjectClassHandle, attributes: AttributeHandleSet
    ) -> None:
        """Remove subscriptions for the supplied object-class attributes."""

    @abstractmethod
    def subscribeObjectClassAttributesWithRegions(
        self,
        objectClass: ObjectClassHandle,
        attributesAndRegions: AttributeSetRegionSetPairList,
        *,
        active: bool = True,
        updateRateDesignator: str = "",
    ) -> None:
        """Subscribe to object attributes using attribute-to-region associations."""

    @abstractmethod
    def subscribeObjectClassAttributesPassivelyWithRegions(
        self,
        objectClass: ObjectClassHandle,
        attributesAndRegions: AttributeSetRegionSetPairList
        | MutableAttributeSetRegionSetPairList,
        updateRateDesignator: str = "",
    ) -> None:
        """Exact Java standard passive regional object-attribute overload."""

    @abstractmethod
    def unsubscribeObjectClassAttributesWithRegions(
        self,
        objectClass: ObjectClassHandle,
        attributesAndRegions: AttributeSetRegionSetPairList,
    ) -> None:
        """Remove a regional object-class attribute subscription."""

    @abstractmethod
    def publishObjectClassDirectedInteractions(
        self,
        objectClass: ObjectClassHandle,
        interactionClasses: InteractionClassHandleSet,
    ) -> None:
        """Publish directed interactions associated with an object class."""

    @abstractmethod
    def unpublishObjectClassDirectedInteractions(
        self,
        objectClass: ObjectClassHandle,
        interactionClasses: InteractionClassHandleSet | None = None,
    ) -> None:
        """Remove all or a selected set of directed-interaction publications."""

    @abstractmethod
    def subscribeObjectClassDirectedInteractions(
        self,
        objectClass: ObjectClassHandle,
        interactionClasses: InteractionClassHandleSet,
        *,
        universally: bool = False,
    ) -> None:
        """Subscribe to directed interactions for an object class."""

    @abstractmethod
    def subscribeObjectClassDirectedInteractionsUniversally(
        self,
        objectClass: ObjectClassHandle,
        interactionClasses: InteractionClassHandleSet | MutableInteractionClassHandleSet,
    ) -> None:
        """Exact Java standard universal directed-interaction subscription overload."""

    @abstractmethod
    def unsubscribeObjectClassDirectedInteractions(
        self,
        objectClass: ObjectClassHandle,
        interactionClasses: InteractionClassHandleSet | None = None,
    ) -> None:
        """Remove all or a selected set of directed-interaction subscriptions."""

    @abstractmethod
    def publishInteractionClass(self, interactionClass: InteractionClassHandle) -> None:
        """Publish a FOM interaction class."""

    @abstractmethod
    def unpublishInteractionClass(self, interactionClass: InteractionClassHandle) -> None:
        """Stop publishing a FOM interaction class."""

    @abstractmethod
    def subscribeInteractionClass(
        self, interactionClass: InteractionClassHandle, *, active: bool = True
    ) -> None:
        """Subscribe actively or passively to a FOM interaction class."""

    @abstractmethod
    def subscribeInteractionClassPassively(
        self, interactionClass: InteractionClassHandle
    ) -> None:
        """Exact Java standard passive interaction subscription overload."""

    @abstractmethod
    def unsubscribeInteractionClass(self, interactionClass: InteractionClassHandle) -> None:
        """Remove a FOM interaction-class subscription."""

    @abstractmethod
    def reserveObjectInstanceName(self, objectInstanceName: str) -> None:
        """Asynchronously reserve a name for later named object registration."""

    @abstractmethod
    def releaseObjectInstanceName(self, objectInstanceName: str) -> None:
        """Release a name previously reserved by this federate."""

    @abstractmethod
    def reserveMultipleObjectInstanceNames(self, objectInstanceNames: ObjectInstanceNameSet) -> None:
        """Asynchronously reserve a batch of names for later object registration."""

    @abstractmethod
    def releaseMultipleObjectInstanceNames(self, objectInstanceNames: ObjectInstanceNameSet) -> None:
        """Release a batch of names previously reserved by this federate."""

    @abstractmethod
    def registerObjectInstance(
        self,
        objectClass: ObjectClassHandle,
        *,
        objectInstanceName: str | None = None,
    ) -> ObjectInstanceHandle:
        """Register a named or provider-named object instance."""

    @abstractmethod
    def registerObjectInstanceWithRegions(
        self,
        objectClass: ObjectClassHandle,
        attributesAndRegions: AttributeSetRegionSetPairList,
        *,
        objectInstanceName: str | None = None,
    ) -> ObjectInstanceHandle:
        """Register an object instance with attribute-to-region associations."""

    @abstractmethod
    def associateRegionsForUpdates(
        self,
        objectInstance: ObjectInstanceHandle,
        attributesAndRegions: AttributeSetRegionSetPairList,
    ) -> None:
        """Associate additional regions with an owned object instance."""

    @abstractmethod
    def unassociateRegionsForUpdates(
        self,
        objectInstance: ObjectInstanceHandle,
        attributesAndRegions: AttributeSetRegionSetPairList,
    ) -> None:
        """Remove regional associations from an owned object instance."""

    @abstractmethod
    def getObjectInstanceHandle(self, objectInstanceName: str) -> ObjectInstanceHandle:
        """Resolve a locally known object-instance name."""

    @abstractmethod
    def getObjectInstanceName(self, objectInstance: ObjectInstanceHandle) -> str:
        """Resolve a locally known object-instance handle."""

    @abstractmethod
    def deleteObjectInstance(
        self,
        objectInstance: ObjectInstanceHandle,
        userSuppliedTag: bytes = b"",
    ) -> None:
        """Delete a known object instance through the receive-order overload."""

    @abstractmethod
    def localDeleteObjectInstance(self, objectInstance: ObjectInstanceHandle) -> None:
        """Remove a locally owned object instance without sending a federation delete."""

    @abstractmethod
    def deleteObjectInstanceWithTime(
        self,
        objectInstance: ObjectInstanceHandle,
        time: LogicalTime,
        userSuppliedTag: bytes = b"",
    ) -> MessageRetractionHandle:
        """Delete an object instance with a provider timestamp."""

    @abstractmethod
    def updateAttributeValues(
        self,
        objectInstance: ObjectInstanceHandle,
        attributeValues: AttributeHandleValueMap,
        userSuppliedTag: bytes = b"",
    ) -> None:
        """Send receive-order values for published attributes of an object instance."""

    @abstractmethod
    def updateAttributeValuesWithTime(
        self,
        objectInstance: ObjectInstanceHandle,
        attributeValues: AttributeHandleValueMap,
        time: LogicalTime,
        userSuppliedTag: bytes = b"",
    ) -> MessageRetractionHandle:
        """Send timestamp-ordered values for an object instance."""

    @abstractmethod
    def requestAttributeValueUpdate(
        self,
        objectClassOrInstance: ObjectClassHandle | ObjectInstanceHandle,
        attributes: AttributeHandleSet,
        userSuppliedTag: bytes = b"",
    ) -> None:
        """Request current values by object class or by known instance.

        The Java API exposes these as overloads.  Python keeps one explicit
        entry point and dispatches from the type-distinct public handle.
        """

    @abstractmethod
    def requestAttributeValueUpdateWithRegions(
        self,
        objectClass: ObjectClassHandle,
        attributesAndRegions: AttributeSetRegionSetPairList,
        userSuppliedTag: bytes = b"",
    ) -> None:
        """Request current values for object attributes scoped by regions."""

    @abstractmethod
    def changeAttributeOrderType(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
        orderType: OrderType,
    ) -> None:
        """Change the order type used for updates of an object instance."""

    @abstractmethod
    def changeDefaultAttributeOrderType(
        self,
        objectClass: ObjectClassHandle,
        attributes: AttributeHandleSet,
        orderType: OrderType,
    ) -> None:
        """Change the default order type for an object-class attribute set."""

    @abstractmethod
    def changeInteractionOrderType(
        self, interactionClass: InteractionClassHandle, orderType: OrderType
    ) -> None:
        """Change the order type used for a published interaction class."""

    @abstractmethod
    def requestAttributeTransportationTypeChange(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
        transportationType: TransportationTypeHandle,
    ) -> None:
        """Request a change to an object's attribute transportation type."""

    @abstractmethod
    def changeDefaultAttributeTransportationType(
        self,
        objectClass: ObjectClassHandle,
        attributes: AttributeHandleSet,
        transportationType: TransportationTypeHandle,
    ) -> None:
        """Change the default transportation type for an object-class attribute set."""

    @abstractmethod
    def queryAttributeTransportationType(
        self, objectInstance: ObjectInstanceHandle, attribute: AttributeHandle
    ) -> None:
        """Request a report of an object's attribute transportation type."""

    @abstractmethod
    def requestInteractionTransportationTypeChange(
        self,
        interactionClass: InteractionClassHandle,
        transportationType: TransportationTypeHandle,
    ) -> None:
        """Request a change to an interaction's transportation type."""

    @abstractmethod
    def queryInteractionTransportationType(
        self, federate: FederateHandle, interactionClass: InteractionClassHandle
    ) -> None:
        """Request a report of a federate's interaction transportation type."""

    @abstractmethod
    def sendInteraction(
        self,
        interactionClass: InteractionClassHandle,
        parameterValues: ParameterHandleValueMap,
        userSuppliedTag: bytes = b"",
    ) -> None:
        """Send a receive-order interaction using published parameters."""

    @abstractmethod
    def sendInteractionWithTime(
        self,
        interactionClass: InteractionClassHandle,
        parameterValues: ParameterHandleValueMap,
        time: LogicalTime,
        userSuppliedTag: bytes = b"",
    ) -> MessageRetractionHandle:
        """Send a timestamp-ordered interaction."""

    @abstractmethod
    def sendDirectedInteraction(
        self,
        interactionClass: InteractionClassHandle,
        objectInstance: ObjectInstanceHandle,
        parameterValues: ParameterHandleValueMap,
        userSuppliedTag: bytes = b"",
    ) -> None:
        """Send a receive-order interaction directly to one object instance."""

    @abstractmethod
    def sendDirectedInteractionWithTime(
        self,
        interactionClass: InteractionClassHandle,
        objectInstance: ObjectInstanceHandle,
        parameterValues: ParameterHandleValueMap,
        time: LogicalTime,
        userSuppliedTag: bytes = b"",
    ) -> MessageRetractionHandle:
        """Send a timestamp-ordered directed interaction."""

    @abstractmethod
    def subscribeInteractionClassWithRegions(
        self,
        interactionClass: InteractionClassHandle,
        regions: RegionHandleSet,
        *,
        active: bool = True,
    ) -> None:
        """Subscribe to an interaction class using DDM regions."""

    @abstractmethod
    def subscribeInteractionClassPassivelyWithRegions(
        self, interactionClass: InteractionClassHandle, regions: RegionHandleSet
    ) -> None:
        """Exact Java standard passive regional interaction subscription overload."""

    @abstractmethod
    def unsubscribeInteractionClassWithRegions(
        self, interactionClass: InteractionClassHandle, regions: RegionHandleSet
    ) -> None:
        """Remove a regional interaction subscription."""

    @abstractmethod
    def sendInteractionWithRegions(
        self,
        interactionClass: InteractionClassHandle,
        parameterValues: ParameterHandleValueMap,
        regions: RegionHandleSet,
        userSuppliedTag: bytes = b"",
    ) -> None:
        """Send a receive-order interaction with DDM regions."""

    @abstractmethod
    def sendInteractionWithRegionsWithTime(
        self,
        interactionClass: InteractionClassHandle,
        parameterValues: ParameterHandleValueMap,
        regions: RegionHandleSet,
        time: LogicalTime,
        userSuppliedTag: bytes = b"",
    ) -> MessageRetractionHandle:
        """Send a timestamp-ordered interaction with DDM regions."""

    @abstractmethod
    def retract(self, retraction: MessageRetractionHandle) -> None:
        """Retract a pending timestamped message."""

    @abstractmethod
    def getFederateHandleFactory(self) -> FederateHandleFactory:
        """Return the provider-owned federate-handle decoder factory."""

    @abstractmethod
    def getFederateHandleSetFactory(self) -> FederateHandleSetFactory:
        """Return the provider-owned federate-handle set factory."""

    @abstractmethod
    def getObjectClassHandleFactory(self) -> ObjectClassHandleFactory:
        """Return the provider-owned object-class-handle decoder factory."""

    @abstractmethod
    def getObjectInstanceHandleFactory(self) -> ObjectInstanceHandleFactory:
        """Return the provider-owned object-instance-handle decoder factory."""

    @abstractmethod
    def getAttributeHandleFactory(self) -> AttributeHandleFactory:
        """Return the provider-owned attribute-handle decoder factory."""

    @abstractmethod
    def getInteractionClassHandleFactory(self) -> InteractionClassHandleFactory:
        """Return the provider-owned interaction-class-handle decoder factory."""

    @abstractmethod
    def getInteractionClassHandleSetFactory(self) -> InteractionClassHandleSetFactory:
        """Return the provider-owned mutable interaction-class-set factory."""

    @abstractmethod
    def getParameterHandleFactory(self) -> ParameterHandleFactory:
        """Return the provider-owned parameter-handle decoder factory."""

    @abstractmethod
    def getTransportationTypeHandleFactory(self) -> TransportationTypeHandleFactory:
        """Return the provider-owned transportation-handle decoder factory."""

    @abstractmethod
    def getDimensionHandleFactory(self) -> DimensionHandleFactory:
        """Return the provider-owned dimension-handle decoder factory."""

    @abstractmethod
    def getRegionHandleFactory(self) -> RegionHandleFactory:
        """Return the provider-owned region-handle decoder factory."""

    @abstractmethod
    def getMessageRetractionHandleFactory(self) -> MessageRetractionHandleFactory:
        """Return the provider-owned message-retraction decoder factory."""

    @abstractmethod
    def getDimensionHandleSetFactory(self) -> DimensionHandleSetFactory:
        """Return the provider-owned mutable dimension-set factory."""

    @abstractmethod
    def getRegionHandleSetFactory(self) -> RegionHandleSetFactory:
        """Return the provider-owned mutable region-set factory."""

    @abstractmethod
    def getAttributeHandleSetFactory(self) -> AttributeHandleSetFactory:
        """Return the provider-owned mutable attribute-set factory."""

    @abstractmethod
    def getAttributeHandleValueMapFactory(self) -> AttributeHandleValueMapFactory:
        """Return the provider-owned mutable attribute-map factory."""

    @abstractmethod
    def getParameterHandleValueMapFactory(self) -> ParameterHandleValueMapFactory:
        """Return the provider-owned mutable parameter-map factory."""

    @abstractmethod
    def getAttributeSetRegionSetPairListFactory(self) -> AttributeSetRegionSetPairListFactory:
        """Return the provider-owned Java-shaped attribute/region pair-list factory."""

    @abstractmethod
    def createRegion(self, dimensions: DimensionHandleSet) -> RegionHandle:
        """Create a provider-owned region over the supplied dimensions."""

    @abstractmethod
    def commitRegionModifications(self, regions: RegionHandleSet) -> None:
        """Commit pending range-bound changes for provider-owned regions."""

    @abstractmethod
    def deleteRegion(self, region: RegionHandle) -> None:
        """Delete a provider-owned region that is no longer in use."""

    @abstractmethod
    def getDimensionHandleSet(self, region: RegionHandle) -> DimensionHandleSet:
        """Return the dimensions associated with a region."""

    @abstractmethod
    def getRangeBounds(self, region: RegionHandle, dimension: DimensionHandle) -> RangeBounds:
        """Return the current bounds for a region dimension."""

    @abstractmethod
    def setRangeBounds(
        self, region: RegionHandle, dimension: DimensionHandle, rangeBounds: RangeBounds
    ) -> None:
        """Set pending bounds for one region dimension."""

    @abstractmethod
    def getConveyRegionDesignatorSetsSwitch(self) -> bool:
        """Return whether callbacks include provider region-designator sets."""

    @abstractmethod
    def setConveyRegionDesignatorSetsSwitch(self, switchValue: bool) -> None:
        """Enable or disable region-designator sets in receive callbacks."""

    @abstractmethod
    def getObjectClassRelevanceAdvisorySwitch(self) -> bool:
        """Return the object-class relevance advisory switch state."""

    @abstractmethod
    def setObjectClassRelevanceAdvisorySwitch(self, switchValue: bool) -> None:
        """Enable or disable object-class relevance advisories."""

    @abstractmethod
    def getAttributeRelevanceAdvisorySwitch(self) -> bool:
        """Return the attribute relevance advisory switch state."""

    @abstractmethod
    def setAttributeRelevanceAdvisorySwitch(self, switchValue: bool) -> None:
        """Enable or disable attribute relevance advisories."""

    @abstractmethod
    def getAttributeScopeAdvisorySwitch(self) -> bool:
        """Return the attribute scope advisory switch state."""

    @abstractmethod
    def setAttributeScopeAdvisorySwitch(self, switchValue: bool) -> None:
        """Enable or disable attribute scope advisories."""

    @abstractmethod
    def getInteractionRelevanceAdvisorySwitch(self) -> bool:
        """Return the interaction relevance advisory switch state."""

    @abstractmethod
    def setInteractionRelevanceAdvisorySwitch(self, switchValue: bool) -> None:
        """Enable or disable interaction relevance advisories."""

    @abstractmethod
    def getAutomaticResignDirective(self) -> ResignAction:
        """Return the provider's automatic-resign directive."""

    @abstractmethod
    def setAutomaticResignDirective(self, resignAction: ResignAction) -> None:
        """Set the provider's automatic-resign directive."""

    @abstractmethod
    def getServiceReportingSwitch(self) -> bool:
        """Return the service-reporting switch state."""

    @abstractmethod
    def setServiceReportingSwitch(self, switchValue: bool) -> None:
        """Enable or disable service reporting."""

    @abstractmethod
    def getExceptionReportingSwitch(self) -> bool:
        """Return the exception-reporting switch state."""

    @abstractmethod
    def setExceptionReportingSwitch(self, switchValue: bool) -> None:
        """Enable or disable exception reporting."""

    @abstractmethod
    def getSendServiceReportsToFileSwitch(self) -> bool:
        """Return whether service reports are sent to a file."""

    @abstractmethod
    def setSendServiceReportsToFileSwitch(self, switchValue: bool) -> None:
        """Enable or disable service reports being sent to a file."""

    @abstractmethod
    def getAutoProvideSwitch(self) -> bool:
        """Return the provider's automatic attribute-provision switch."""

    @abstractmethod
    def getDelaySubscriptionEvaluationSwitch(self) -> bool:
        """Return the delayed-subscription-evaluation switch."""

    @abstractmethod
    def getAdvisoriesUseKnownClassSwitch(self) -> bool:
        """Return the known-class advisory switch."""

    @abstractmethod
    def getAllowRelaxedDDMSwitch(self) -> bool:
        """Return whether relaxed DDM is enabled."""

    @abstractmethod
    def getNonRegulatedGrantSwitch(self) -> bool:
        """Return whether non-regulated grants are enabled."""

    @abstractmethod
    def queryAttributeOwnership(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
    ) -> None:
        """Request ownership reports for the supplied object attributes."""

    @abstractmethod
    def isAttributeOwnedByFederate(
        self,
        objectInstance: ObjectInstanceHandle,
        attribute: AttributeHandle,
    ) -> bool:
        """Return whether this federate currently owns an object attribute."""

    @abstractmethod
    def unconditionalAttributeOwnershipDivestiture(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
        userSuppliedTag: bytes = b"",
    ) -> None:
        """Immediately divest owned attributes."""

    @abstractmethod
    def negotiatedAttributeOwnershipDivestiture(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
        userSuppliedTag: bytes = b"",
    ) -> None:
        """Begin negotiated divestiture of owned attributes."""

    @abstractmethod
    def confirmDivestiture(
        self,
        objectInstance: ObjectInstanceHandle,
        confirmedAttributes: AttributeHandleSet,
        userSuppliedTag: bytes = b"",
    ) -> None:
        """Confirm a pending negotiated divestiture."""

    @abstractmethod
    def cancelNegotiatedAttributeOwnershipDivestiture(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
    ) -> None:
        """Cancel a pending negotiated divestiture."""

    @abstractmethod
    def attributeOwnershipAcquisition(
        self,
        objectInstance: ObjectInstanceHandle,
        desiredAttributes: AttributeHandleSet,
        userSuppliedTag: bytes = b"",
    ) -> None:
        """Request negotiated ownership acquisition."""

    @abstractmethod
    def attributeOwnershipAcquisitionIfAvailable(
        self,
        objectInstance: ObjectInstanceHandle,
        desiredAttributes: AttributeHandleSet,
        userSuppliedTag: bytes = b"",
    ) -> None:
        """Request ownership acquisition only for currently available attributes."""

    @abstractmethod
    def cancelAttributeOwnershipAcquisition(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
    ) -> None:
        """Cancel a pending ownership acquisition."""

    @abstractmethod
    def attributeOwnershipReleaseDenied(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
        userSuppliedTag: bytes = b"",
    ) -> None:
        """Deny release of owned attributes."""

    @abstractmethod
    def attributeOwnershipDivestitureIfWanted(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
        userSuppliedTag: bytes = b"",
    ) -> AttributeHandleSet:
        """Divest only attributes currently wanted by another federate."""

    @abstractmethod
    def getTimeFactory(self) -> LogicalTimeFactory:
        """Return the provider-created factory selected by the joined FOM."""

    @abstractmethod
    def enableTimeRegulation(self, lookahead: LogicalTimeInterval) -> None:
        """Enable time regulation with a provider-created lookahead interval."""

    @abstractmethod
    def disableTimeRegulation(self) -> None:
        """Disable time regulation for the joined federate."""

    @abstractmethod
    def enableTimeConstrained(self) -> None:
        """Enable time-constrained delivery."""

    @abstractmethod
    def disableTimeConstrained(self) -> None:
        """Disable time-constrained delivery."""

    @abstractmethod
    def timeAdvanceRequest(self, time: LogicalTime) -> None:
        """Request a receive-order time advance."""

    @abstractmethod
    def timeAdvanceRequestAvailable(self, time: LogicalTime) -> None:
        """Request an available receive-order time advance."""

    @abstractmethod
    def nextMessageRequest(self, time: LogicalTime) -> None:
        """Request the next-message time advance."""

    @abstractmethod
    def nextMessageRequestAvailable(self, time: LogicalTime) -> None:
        """Request an available next-message time advance."""

    @abstractmethod
    def flushQueueRequest(self, time: LogicalTime) -> None:
        """Flush queued timestamped messages through the requested time."""

    @abstractmethod
    def enableAsynchronousDelivery(self) -> None:
        """Enable asynchronous delivery of time-stamped interactions."""

    @abstractmethod
    def disableAsynchronousDelivery(self) -> None:
        """Disable asynchronous delivery of time-stamped interactions."""

    @abstractmethod
    def modifyLookahead(self, lookahead: LogicalTimeInterval) -> None:
        """Replace the current time-regulation lookahead interval."""

    @abstractmethod
    def queryLookahead(self) -> LogicalTimeInterval:
        """Return the provider-created current lookahead interval."""

    @abstractmethod
    def queryLogicalTime(self) -> LogicalTime:
        """Return the provider-created current logical time."""

    @abstractmethod
    def queryGALT(self) -> TimeQueryResult:
        """Return the Java ``TimeQueryReturn``-shaped federation GALT result."""

    @abstractmethod
    def queryLITS(self) -> TimeQueryResult:
        """Return the Java ``TimeQueryReturn``-shaped federation LITS result."""

    @abstractmethod
    def createFederationExecution(
        self,
        federationName: str,
        fomModule: str | Iterable[str],
        logicalTimeImplementationName: str = "",
    ) -> None:
        """Create a federation from one or more FOM modules."""

    @abstractmethod
    def createFederationExecutionWithMIM(
        self,
        federationName: str,
        fomModules: Iterable[str],
        mimModule: str,
        logicalTimeImplementationName: str = "",
    ) -> None:
        """Create a federation from FOM modules plus an explicit MIM URL."""

    @abstractmethod
    def destroyFederationExecution(self, federationName: str) -> None:
        """Destroy an unjoined federation execution."""


class RtiFactory(ABC):
    """Python analogue of the standard Java service-provider factory."""

    @abstractmethod
    def getRtiAmbassador(self) -> RTIambassador:
        """Return a new RTI ambassador."""

    @abstractmethod
    def getEncoderFactory(self) -> "EncoderFactory":
        """Return the provider-owned standard basic-data-element factory."""

    @abstractmethod
    def rtiName(self) -> str:
        """Return the provider name."""

    @abstractmethod
    def rtiVersion(self) -> str:
        """Return the provider version."""


class RtiFactoryFactory:
    """Discover providers like the Java API's ``ServiceLoader``-based helper."""

    _ENTRY_POINT_GROUP = "hla.rti1516_2025.factories"

    @classmethod
    def getRtiFactory(cls, name: str | None = None) -> RtiFactory:
        requested_name = name if name is not None else os.getenv("HLA_RTI_FACTORY_NAME")
        if requested_name is not None:
            aliased_factory = cls._factory_from_entry_point_alias(requested_name)
            if aliased_factory is not None:
                return aliased_factory
        factories = cls.getAvailableRtiFactories()
        if requested_name is None and factories:
            return factories[0]
        for factory in factories:
            if factory.rtiName() == requested_name:
                return factory
        if requested_name is None:
            raise RTIinternalError("Cannot find factory")
        raise RTIinternalError(f"Cannot find factory matching {requested_name}")

    @classmethod
    def getAvailableRtiFactories(cls) -> list[RtiFactory]:
        factories: list[RtiFactory] = []
        for entry_point in cls._entry_points():
            factory_type = entry_point.load()
            factories.append(factory_type())
        return factories

    @classmethod
    def _factory_from_entry_point_alias(cls, name: str) -> RtiFactory | None:
        """Select an installed transport before it needs to initialize itself.

        Most entry-point names equal ``RtiFactory.rtiName()``. A Java bridge is
        different: its actual standard factory name is learned only after its
        JVM and vendor JAR have been selected. An entry-point alias therefore
        lets callers choose the bridge (for example ``"java"``) without
        forcing every installed Java RTI to start during unrelated lookup.
        """

        matching_entry_points = [
            entry_point
            for entry_point in cls._entry_points()
            if entry_point.name == name
        ]
        if not matching_entry_points:
            return None
        if len(matching_entry_points) != 1:
            raise RTIinternalError(f"More than one Python RTI provider uses alias {name}")
        factory_type = matching_entry_points[0].load()
        return factory_type()

    @classmethod
    def _entry_points(cls) -> Any:
        return entry_points(group=cls._ENTRY_POINT_GROUP)
