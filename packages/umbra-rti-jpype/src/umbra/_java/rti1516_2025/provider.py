"""Transport-neutral Python adapters over the Java 2025 RTI binding."""

from __future__ import annotations

from collections.abc import Iterable
import math
from pathlib import Path
from typing import Any, Callable, TypeVar

from hla.rti1516_2025 import (
    AdditionalSettingsResultCode,
    AttributeHandle,
    AttributeHandleFactory,
    AttributeHandleSet,
    AttributeHandleSetFactory,
    AttributeHandleValueMap,
    AttributeHandleValueMapFactory,
    AttributeSetRegionSetPairList,
    AttributeRegionAssociation,
    AttributeSetRegionSetPairListFactory,
    MutableAttributeSetRegionSetPairList,
    CallbackModel,
    ConfigurationResult,
    DimensionHandle,
    DimensionHandleFactory,
    DimensionHandleSet,
    DimensionHandleSetFactory,
    FederateAmbassador,
    FederateHandle,
    FederateHandleFactory,
    FederateHandleSet,
    FederateHandleSetFactory,
    HandleFactory,
    InteractionClassHandle,
    InteractionClassHandleFactory,
    InteractionClassHandleSet,
    InteractionClassHandleSetFactory,
    HLAfloat64Interval,
    HLAfloat64Time,
    HLAfloat64TimeFactory,
    HLAinteger64Interval,
    HLAinteger64Time,
    HLAinteger64TimeFactory,
    LogicalTime,
    LogicalTimeFactory,
    LogicalTimeInterval,
    MessageRetractionHandle,
    MessageRetractionHandleFactory,
    ObjectClassHandle,
    ObjectClassHandleFactory,
    ObjectInstanceHandle,
    ObjectInstanceHandleFactory,
    ObjectInstanceNameSet,
    OrderType,
    ParameterHandle,
    ParameterHandleFactory,
    ParameterHandleValueMap,
    ParameterHandleValueMapFactory,
    RTIambassador,
    RtiConfiguration,
    RangeBounds,
    RegionHandle,
    RegionHandleFactory,
    RegionHandleSet,
    RegionHandleSetFactory,
    ResignAction,
    RtiFactory,
    ServiceGroup,
    TimeQueryResult,
    TransportationTypeHandle,
    TransportationTypeHandleFactory,
    MutableAttributeHandleSet,
    MutableAttributeHandleValueMap,
    MutableDimensionHandleSet,
    MutableFederateHandleSet,
    MutableParameterHandleValueMap,
    MutableRegionHandleSet,
    MutableInteractionClassHandleSet,
)
from hla.rti1516_2025.core import _require_callback_model, _resolve_connect_arguments
from hla.rti1516_2025.encoding import EncoderFactory
from hla.rti1516_2025.exceptions import (
    CouldNotDecode,
    InvalidLogicalTime,
    InvalidLogicalTimeInterval,
    IllegalTimeArithmetic,
    RTIexception,
    RTIinternalError,
    exceptionForName,
)

from ._runtime import JPypeJavaRuntime, JavaCallbackBinding, JavaRuntime
from .config import JavaProviderConfiguration
from .encoding import JavaEncoderFactory

_Result = TypeVar("_Result")


def _simple_java_exception_name(name: str) -> str:
    """Normalize simple, qualified, and nested Java exception names."""

    return name.rsplit(".", 1)[-1].rsplit("$", 1)[-1]


class _UnsupportedJavaArithmetic(Exception):
    """Internal marker for a legacy Java fixture's unimplemented distance."""


def _java_time_value(value: object) -> object:
    exact = getattr(value, "getTimeValue", None)
    if exact is not None:
        return exact()
    legacy = getattr(value, "getTime", None)
    if legacy is not None:
        return legacy()
    standard = getattr(value, "getValue", None)
    return standard() if standard is not None else None


def _java_time_implementation_name(value: object, numeric: object) -> str:
    """Identify standard time carriers without requiring vendor extensions."""

    legacy = getattr(value, "implementationName", None)
    if callable(legacy):
        return str(legacy())
    return "HLAfloat64Time" if isinstance(numeric, float) else "HLAinteger64Time"


def _java_interval_value(value: object) -> object:
    exact = getattr(value, "getIntervalValue", None)
    if exact is not None:
        return exact()
    legacy = getattr(value, "getInterval", None)
    if legacy is not None:
        return legacy()
    standard = getattr(value, "getValue", None)
    return standard() if standard is not None else None
_Handle = TypeVar("_Handle")


def _enum_name(value: object) -> str:
    """Read either a Java enum's ``name()`` or a test-double string value."""

    name = getattr(value, "name", None)
    if callable(name):
        return str(name())
    if name is not None:
        return str(name)
    return str(value)


def _configuration_result(java_result: object) -> ConfigurationResult:
    setting_name = _enum_name(getattr(java_result, "additionalSettingsResultCode"))
    try:
        setting_code = AdditionalSettingsResultCode[setting_name]
    except KeyError as error:
        raise RTIinternalError(
            f"Java RTI returned an unknown AdditionalSettingsResultCode: {setting_name}"
        ) from error
    return ConfigurationResult(
        configurationUsed=bool(getattr(java_result, "configurationUsed")),
        addressUsed=bool(getattr(java_result, "addressUsed")),
        additionalSettingsResultCode=setting_code,
        message=str(getattr(java_result, "message")),
    )


def _encoded_handle(value: object, expected_type: type[_Handle]) -> bytes:
    """Require a public handle in the expected standard domain."""

    if not isinstance(value, expected_type):
        raise TypeError(f"handle must be {expected_type.__name__}")
    return value.encodedValue  # type: ignore[union-attr]


def _attribute_handle_bytes(attributes: object) -> tuple[bytes, ...]:
    if not isinstance(attributes, (AttributeHandleSet, MutableAttributeHandleSet)):
        raise TypeError("attributes must be AttributeHandleSet or its factory builder")
    return tuple(_encoded_handle(attribute, AttributeHandle) for attribute in attributes)


def _interaction_class_handle_bytes(interactions: object) -> tuple[bytes, ...]:
    if not isinstance(interactions, (InteractionClassHandleSet, MutableInteractionClassHandleSet)):
        raise TypeError("interactionClasses must be InteractionClassHandleSet or its factory builder")
    return tuple(
        _encoded_handle(interaction, InteractionClassHandle) for interaction in interactions
    )


def _federate_handle_bytes(handles: object) -> tuple[bytes, ...]:
    if not isinstance(handles, (FederateHandleSet, MutableFederateHandleSet)):
        raise TypeError("synchronizationSet must be FederateHandleSet or its factory builder")
    return tuple(_encoded_handle(handle, FederateHandle) for handle in handles)


def _object_instance_names(value: object) -> tuple[str, ...]:
    if not isinstance(value, ObjectInstanceNameSet):
        raise TypeError("objectInstanceNames must be ObjectInstanceNameSet")
    return tuple(str(name) for name in value)


def _attribute_value_pairs(values: object) -> tuple[tuple[bytes, bytes], ...]:
    if not isinstance(values, (AttributeHandleValueMap, MutableAttributeHandleValueMap)):
        raise TypeError("attributeValues must be AttributeHandleValueMap or its factory builder")
    return tuple(
        (_encoded_handle(attribute, AttributeHandle), bytes(value))
        for attribute, value in values.items()
    )


def _attribute_region_pairs(
    values: object,
) -> tuple[tuple[tuple[bytes, ...], tuple[bytes, ...]], ...]:
    if not isinstance(values, (AttributeSetRegionSetPairList, MutableAttributeSetRegionSetPairList)):
        raise TypeError(
            "attributesAndRegions must be AttributeSetRegionSetPairList or its factory builder"
        )
    return tuple(
        (
            _attribute_handle_bytes(pair.attributes),
            _region_handle_bytes(pair.regions),
        )
        for pair in values
    )


def _parameter_value_pairs(values: object) -> tuple[tuple[bytes, bytes], ...]:
    if not isinstance(values, (ParameterHandleValueMap, MutableParameterHandleValueMap)):
        raise TypeError("parameterValues must be ParameterHandleValueMap or its factory builder")
    return tuple(
        (_encoded_handle(parameter, ParameterHandle), bytes(value))
        for parameter, value in values.items()
    )


def _dimension_handle_bytes(dimensions: object) -> tuple[bytes, ...]:
    if not isinstance(dimensions, (DimensionHandleSet, MutableDimensionHandleSet)):
        raise TypeError("dimensions must be DimensionHandleSet or its factory builder")
    return tuple(_encoded_handle(dimension, DimensionHandle) for dimension in dimensions)


def _region_handle_bytes(regions: object) -> tuple[bytes, ...]:
    if not isinstance(regions, (RegionHandleSet, MutableRegionHandleSet)):
        raise TypeError("regions must be RegionHandleSet or its factory builder")
    return tuple(_encoded_handle(region, RegionHandle) for region in regions)


class _JavaTimeFactoryBase:
    def __init__(self, ambassador: object, runtime: JavaRuntime, java_factory: object) -> None:
        self._ambassador = ambassador
        self._runtime = runtime
        self._java_factory = java_factory

    def _factory_call(self, function: Callable[..., _Result], *args: object) -> _Result:
        """Translate checked Java time-factory failures at the Python edge.

        The standard Java factory methods are called directly rather than
        through ``RTIambassador._call``.  JPype therefore exposes exceptions
        such as ``InvalidLogicalTime`` as Java proxy classes unless this edge
        performs the same name-based translation as the ambassador path.
        """
        try:
            return function(*args)
        except RTIexception:
            raise
        except Exception as error:
            name = self._runtime.exception_name(error)
            if name is None:
                raise RTIinternalError(f"Java logical-time factory call failed: {error}") from error
            raise exceptionForName(_simple_java_exception_name(name), str(error)) from error

    def implementationName(self) -> str:
        return str(self._factory_call(self._java_factory.getName))

    def _time(self, value: object) -> LogicalTime:
        implementation = self.implementationName()
        numeric = _java_time_value(value)
        try:
            invalid = numeric is None or float(numeric) < 0.0 or not math.isfinite(float(numeric))
        except (TypeError, ValueError, OverflowError):
            invalid = True
        if invalid:
            raise InvalidLogicalTime("Java provider returned an invalid logical-time value")
        value_type = HLAinteger64Time if implementation == "HLAinteger64Time" else HLAfloat64Time
        return value_type(
            self._runtime.handle_bytes(value),
            implementation,
            bool(value.isInitial()),
            bool(value.isFinal()),
            numeric,
            str(value.toString()),
        )

    def _interval(self, value: object) -> LogicalTimeInterval:
        implementation = self.implementationName()
        numeric = _java_interval_value(value)
        try:
            invalid = numeric is None or float(numeric) < 0.0 or not math.isfinite(float(numeric))
        except (TypeError, ValueError, OverflowError):
            invalid = True
        if invalid:
            raise InvalidLogicalTimeInterval(
                "Java provider returned an invalid logical-time interval value"
            )
        value_type = (
            HLAinteger64Interval if implementation == "HLAinteger64Time" else HLAfloat64Interval
        )
        return value_type(
            self._runtime.handle_bytes(value),
            implementation,
            bool(value.isZero()),
            bool(value.isEpsilon()),
            numeric,
            str(value.toString()),
        )

    def makeInitial(self) -> LogicalTime:
        return self._time(self._factory_call(self._java_factory.makeInitial))

    def makeFinal(self) -> LogicalTime:
        return self._time(self._factory_call(self._java_factory.makeFinal))

    def makeZero(self) -> LogicalTimeInterval:
        return self._interval(self._factory_call(self._java_factory.makeZero))

    def makeEpsilon(self) -> LogicalTimeInterval:
        return self._interval(self._factory_call(self._java_factory.makeEpsilon))

    def decodeLogicalTime(self, encodedValue: bytes) -> LogicalTime:
        encoded = bytes(encodedValue)
        if len(encoded) != 8:
            raise CouldNotDecode("Logical-time encoding must contain exactly eight bytes")
        try:
            return self._time(
                self._runtime.decode_logical_time(self._ambassador, encoded)
            )
        except (InvalidLogicalTime, CouldNotDecode) as error:
            if isinstance(error, CouldNotDecode):
                raise
            raise CouldNotDecode(f"Could not decode logical time: {error}") from error
        except RTIexception:
            raise
        except Exception as error:
            raise CouldNotDecode(f"Could not decode logical time: {error}") from error

    def decodeLogicalTimeInterval(self, encodedValue: bytes) -> LogicalTimeInterval:
        encoded = bytes(encodedValue)
        if len(encoded) != 8:
            raise CouldNotDecode("Logical-time interval encoding must contain exactly eight bytes")
        try:
            return self._interval(
                self._runtime.decode_logical_interval(self._ambassador, encoded)
            )
        except (InvalidLogicalTimeInterval, CouldNotDecode) as error:
            if isinstance(error, CouldNotDecode):
                raise
            raise CouldNotDecode(f"Could not decode logical-time interval: {error}") from error
        except RTIexception:
            raise
        except Exception as error:
            raise CouldNotDecode(f"Could not decode logical-time interval: {error}") from error

    def _require_time(self, value: LogicalTime, name: str) -> None:
        if value.implementationName() != self.implementationName():
            raise InvalidLogicalTime(
                f"{name} uses {value.implementationName()}, expected {self.implementationName()}"
            )

    def _require_interval(self, value: LogicalTimeInterval, name: str) -> None:
        if value.implementationName() != self.implementationName():
            raise InvalidLogicalTimeInterval(
                f"{name} uses {value.implementationName()}, expected {self.implementationName()}"
            )

    def _arithmetic_call(
        self,
        function: Callable[..., _Result],
        *args: object,
        allow_unsupported: bool = False,
    ) -> _Result:
        try:
            return function(*args)
        except RTIexception:
            raise
        except Exception as error:
            name = self._runtime.exception_name(error)
            if name is None:
                raise RTIinternalError(f"Java logical-time arithmetic failed: {error}") from error
            name = _simple_java_exception_name(name)
            if allow_unsupported and name == "UnsupportedOperationException":
                raise _UnsupportedJavaArithmetic(str(error)) from error
            if name in {"ArithmeticException", "IllegalArgumentException"}:
                raise IllegalTimeArithmetic(str(error)) from error
            raise exceptionForName(name, str(error)) from error

    def add(self, time: LogicalTime, addend: LogicalTimeInterval) -> LogicalTime:
        if not isinstance(time, LogicalTime):
            raise TypeError("time must be LogicalTime")
        if not isinstance(addend, LogicalTimeInterval):
            raise TypeError("addend must be LogicalTimeInterval")
        self._require_time(time, "time")
        self._require_interval(addend, "addend")
        java_time = self._runtime.decode_logical_time(self._ambassador, time.toByteArray())
        java_addend = self._runtime.decode_logical_interval(self._ambassador, addend.toByteArray())
        return self._time(self._arithmetic_call(java_time.add, java_addend))

    def subtract(self, time: LogicalTime, subtrahend: LogicalTimeInterval) -> LogicalTime:
        if not isinstance(time, LogicalTime):
            raise TypeError("time must be LogicalTime")
        if not isinstance(subtrahend, LogicalTimeInterval):
            raise TypeError("subtrahend must be LogicalTimeInterval")
        self._require_time(time, "time")
        self._require_interval(subtrahend, "subtrahend")
        java_time = self._runtime.decode_logical_time(self._ambassador, time.toByteArray())
        java_subtrahend = self._runtime.decode_logical_interval(
            self._ambassador, subtrahend.toByteArray()
        )
        return self._time(self._arithmetic_call(java_time.subtract, java_subtrahend))

    def difference(self, minuend: LogicalTime, subtrahend: LogicalTime) -> LogicalTimeInterval:
        if not isinstance(minuend, LogicalTime):
            raise TypeError("minuend must be LogicalTime")
        if not isinstance(subtrahend, LogicalTime):
            raise TypeError("subtrahend must be LogicalTime")
        self._require_time(minuend, "minuend")
        self._require_time(subtrahend, "subtrahend")
        java_minuend = self._runtime.decode_logical_time(self._ambassador, minuend.toByteArray())
        java_subtrahend = self._runtime.decode_logical_time(
            self._ambassador, subtrahend.toByteArray()
        )
        # IEEE 1516.1-2025 Java time carriers expose ``distance`` on
        # LogicalTime.  Older compact fixtures expose the C++-shaped
        # ``LogicalTimeInterval.setToDifference`` mutator as a compatibility
        # operation and may declare distance while deliberately leaving it
        # unsupported. Prefer the exact standard operation whenever it is
        # implemented, falling back only for that explicit legacy shape.
        distance = getattr(java_minuend, "distance", None)
        if distance is not None:
            java_difference = self._factory_call(self._java_factory.makeZero)
            set_to_difference = getattr(java_difference, "setToDifference", None)
            try:
                return self._interval(
                    self._arithmetic_call(
                        distance,
                        java_subtrahend,
                        allow_unsupported=set_to_difference is not None,
                    )
                )
            except _UnsupportedJavaArithmetic:
                if set_to_difference is None:
                    raise
            self._arithmetic_call(set_to_difference, java_minuend, java_subtrahend)
            return self._interval(java_difference)
        java_difference = self._factory_call(self._java_factory.makeZero)
        self._arithmetic_call(java_difference.setToDifference, java_minuend, java_subtrahend)
        return self._interval(java_difference)


class _JavaInteger64TimeFactory(_JavaTimeFactoryBase, HLAinteger64TimeFactory):
    def makeLogicalTime(self, value: int) -> HLAinteger64Time:
        method = getattr(self._java_factory, "makeTime", None)
        if method is None:
            method = self._java_factory.makeLogicalTime
        return self._time(self._factory_call(method, int(value)))  # type: ignore[return-value]

    def makeLogicalTimeInterval(self, value: int) -> HLAinteger64Interval:
        method = getattr(self._java_factory, "makeInterval", None)
        if method is None:
            method = self._java_factory.makeLogicalTimeInterval
        return self._interval(self._factory_call(method, int(value)))  # type: ignore[return-value]


class _JavaFloat64TimeFactory(_JavaTimeFactoryBase, HLAfloat64TimeFactory):
    def makeLogicalTime(self, value: float) -> HLAfloat64Time:
        method = getattr(self._java_factory, "makeTime", None)
        if method is None:
            method = self._java_factory.makeLogicalTime
        return self._time(self._factory_call(method, float(value)))  # type: ignore[return-value]

    def makeLogicalTimeInterval(self, value: float) -> HLAfloat64Interval:
        method = getattr(self._java_factory, "makeInterval", None)
        if method is None:
            method = self._java_factory.makeLogicalTimeInterval
        return self._interval(self._factory_call(method, float(value)))  # type: ignore[return-value]


class _JavaHandleFactory(HandleFactory):
    """Provider-backed handle decoder using the Java standard factory method."""

    def __init__(
        self,
        owner: "JavaRTIambassador",
        factory_method_name: str,
        handle_type: type[_Handle],
    ) -> None:
        self._owner = owner
        self._factory_method_name = factory_method_name
        self._handle_type = handle_type

    def decode(self, encodedValue: bytes) -> _Handle:
        raw_handle = self._owner._call(
            self._owner._runtime.decode_handle,
            self._owner._implementation,
            self._factory_method_name,
            bytes(encodedValue),
        )
        return self._handle_type(self._owner._runtime.handle_bytes(raw_handle))


class _JavaFederateHandleFactory(_JavaHandleFactory, FederateHandleFactory):
    pass


class _JavaObjectClassHandleFactory(_JavaHandleFactory, ObjectClassHandleFactory):
    pass


class _JavaObjectInstanceHandleFactory(_JavaHandleFactory, ObjectInstanceHandleFactory):
    pass


class _JavaAttributeHandleFactory(_JavaHandleFactory, AttributeHandleFactory):
    pass


class _JavaInteractionClassHandleFactory(_JavaHandleFactory, InteractionClassHandleFactory):
    pass


class _JavaParameterHandleFactory(_JavaHandleFactory, ParameterHandleFactory):
    pass


class _JavaTransportationTypeHandleFactory(
    _JavaHandleFactory, TransportationTypeHandleFactory
):
    pass


class _JavaDimensionHandleFactory(_JavaHandleFactory, DimensionHandleFactory):
    pass


class _JavaRegionHandleFactory(_JavaHandleFactory, RegionHandleFactory):
    pass


class _JavaMessageRetractionHandleFactory(
    _JavaHandleFactory, MessageRetractionHandleFactory
):
    pass


class _JavaSetFactory:
    """Common Java set-factory adapter returning Python mutable builders."""

    def __init__(self, owner: "JavaRTIambassador", method_name: str, builder_type: type[Any]) -> None:
        self._owner = owner
        self._method_name = method_name
        self._builder_type = builder_type

    def create(self) -> Any:
        factory = self._owner._call(getattr(self._owner._implementation, self._method_name))
        self._owner._call(getattr(factory, "create"))
        return self._builder_type()


class _JavaMapFactory:
    """Common Java map-factory adapter returning Python mutable builders."""

    def __init__(self, owner: "JavaRTIambassador", method_name: str, builder_type: type[Any]) -> None:
        self._owner = owner
        self._method_name = method_name
        self._builder_type = builder_type

    def create(self) -> Any:
        factory = self._owner._call(getattr(self._owner._implementation, self._method_name))
        self._owner._call(getattr(factory, "create"), 0)
        return self._builder_type()


class _JavaAttributeHandleSetFactory(_JavaSetFactory, AttributeHandleSetFactory):
    pass


class _JavaDimensionHandleSetFactory(_JavaSetFactory, DimensionHandleSetFactory):
    pass


class _JavaFederateHandleSetFactory(_JavaSetFactory, FederateHandleSetFactory):
    pass


class _JavaRegionHandleSetFactory(_JavaSetFactory, RegionHandleSetFactory):
    pass


class _JavaInteractionClassHandleSetFactory(_JavaSetFactory, InteractionClassHandleSetFactory):
    pass


class _JavaAttributeSetRegionSetPairListFactory(AttributeSetRegionSetPairListFactory):
    """Adapter for the 2025 Java pair-list factory.

    The compact in-repository fixture predates this factory.  In that lane we
    retain a provider-neutral Python builder; official 2025 Java providers
    still receive the exact ``create(int)`` invocation.
    """

    def __init__(self, owner: "JavaRTIambassador") -> None:
        self._owner = owner

    def create(self, capacity: int = 0) -> MutableAttributeSetRegionSetPairList:
        factory_method = getattr(self._owner._implementation, "getAttributeSetRegionSetPairListFactory", None)
        if factory_method is not None:
            factory = self._owner._call(factory_method)
            self._owner._call(getattr(factory, "create"), int(capacity))
        return MutableAttributeSetRegionSetPairList(int(capacity))


class _JavaAttributeHandleValueMapFactory(_JavaMapFactory, AttributeHandleValueMapFactory):
    pass


class _JavaParameterHandleValueMapFactory(_JavaMapFactory, ParameterHandleValueMapFactory):
    pass


class JavaRTIambassador(RTIambassador):
    """A public Python ambassador backed by a Java ``RTIambassador``."""

    def __init__(self, implementation: object, runtime: JavaRuntime) -> None:
        self._implementation = implementation
        self._runtime = runtime
        self._callback_binding: JavaCallbackBinding | None = None
        self._message_retractions: dict[bytes, object] = {}

    def getHLAversion(self) -> str:
        return str(self._call(getattr(self._implementation, "getHLAversion")))

    def connect(
        self,
        federateAmbassador: FederateAmbassador,
        callbackModel: CallbackModel,
        configuration: RtiConfiguration | object | None = None,
        credentials: object | None = None,
    ) -> ConfigurationResult:
        callbackModel = _require_callback_model(callbackModel)
        configuration, credentials = _resolve_connect_arguments(configuration, credentials)
        callback_binding = self._runtime.bind_federate_ambassador(federateAmbassador)
        arguments: list[object] = [
            callback_binding.proxy,
            self._runtime.callback_model(callbackModel),
        ]
        if configuration is not None:
            arguments.append(self._runtime.rti_configuration(configuration))
        if credentials is not None:
            arguments.append(self._runtime.credentials(credentials))
        result = self._call(
            getattr(self._implementation, "connect"),
            *arguments,
        )
        # A Java proxy must outlive the connection, even if Java keeps only a
        # weak reference to it.
        self._callback_binding = callback_binding
        return _configuration_result(result)

    def disconnect(self) -> None:
        self._call(getattr(self._implementation, "disconnect"))
        self._callback_binding = None
        self._message_retractions.clear()

    def evokeCallback(self, approximateMinimumTimeInSeconds: float) -> bool:
        return self._call(
            getattr(self._implementation, "evokeCallback"),
            approximateMinimumTimeInSeconds,
        )

    def evokeMultipleCallbacks(
        self,
        approximateMinimumTimeInSeconds: float,
        approximateMaximumTimeInSeconds: float,
    ) -> bool:
        return self._call(
            getattr(self._implementation, "evokeMultipleCallbacks"),
            approximateMinimumTimeInSeconds,
            approximateMaximumTimeInSeconds,
        )

    def enableCallbacks(self) -> None:
        self._call(getattr(self._implementation, "enableCallbacks"))

    def disableCallbacks(self) -> None:
        self._call(getattr(self._implementation, "disableCallbacks"))

    def listFederationExecutions(self) -> None:
        self._call(getattr(self._implementation, "listFederationExecutions"))

    def listFederationExecutionMembers(self, federationExecutionName: str) -> None:
        self._call(
            getattr(self._implementation, "listFederationExecutionMembers"),
            federationExecutionName,
        )

    def joinFederationExecution(
        self,
        federateType: str,
        federationExecutionName: str,
        *,
        federateName: str | None = None,
        additionalFomModules: Iterable[str] = (),
    ) -> FederateHandle:
        modules = tuple(str(module) for module in additionalFomModules)
        if federateName is None:
            if modules:
                raw_handle = self._call(
                    getattr(self._implementation, "joinFederationExecution"),
                    federateType,
                    federationExecutionName,
                    self._runtime.fom_module_urls(modules),
                )
            else:
                raw_handle = self._call(
                    getattr(self._implementation, "joinFederationExecution"),
                    federateType,
                    federationExecutionName,
                )
        else:
            if modules:
                raw_handle = self._call(
                    getattr(self._implementation, "joinFederationExecution"),
                    federateName,
                    federateType,
                    federationExecutionName,
                    self._runtime.fom_module_urls(modules),
                )
            else:
                raw_handle = self._call(
                    getattr(self._implementation, "joinFederationExecution"),
                    federateName,
                    federateType,
                    federationExecutionName,
                )
        return FederateHandle(self._runtime.federate_handle_bytes(raw_handle))

    def resignFederationExecution(self, resignAction: ResignAction) -> None:
        if not isinstance(resignAction, ResignAction):
            raise TypeError("resignAction must be ResignAction")
        self._call(
            getattr(self._implementation, "resignFederationExecution"),
            self._runtime.resign_action(resignAction.name),
        )

    def registerFederationSynchronizationPoint(
        self,
        synchronizationPointLabel: str,
        userSuppliedTag: bytes = b"",
        *,
        synchronizationSet: FederateHandleSet | None = None,
    ) -> None:
        arguments: tuple[object, ...] = (
            synchronizationPointLabel,
            self._runtime.byte_array(bytes(userSuppliedTag)),
        )
        if synchronizationSet is not None:
            arguments += (self._runtime.federate_handle_set(self._implementation, _federate_handle_bytes(synchronizationSet)),)
        self._call(getattr(self._implementation, "registerFederationSynchronizationPoint"), *arguments)

    def synchronizationPointAchieved(
        self, synchronizationPointLabel: str, successfully: bool = True
    ) -> None:
        self._call(
            getattr(self._implementation, "synchronizationPointAchieved"),
            synchronizationPointLabel,
            bool(successfully),
        )

    def queryFederationSaveStatus(self) -> None:
        self._call(getattr(self._implementation, "queryFederationSaveStatus"))

    def requestFederationSave(self, label: str, time: LogicalTime | None = None) -> None:
        method = getattr(self._implementation, "requestFederationSave")
        if time is None:
            self._call(method, label)
            return
        if not isinstance(time, LogicalTime):
            raise TypeError("time must be LogicalTime")
        raw_time = self._call(
            self._runtime.decode_logical_time,
            self._implementation,
            time.encodedValue,
        )
        self._call(method, label, raw_time)

    def federateSaveBegun(self) -> None:
        self._call(getattr(self._implementation, "federateSaveBegun"))

    def federateSaveComplete(self) -> None:
        self._call(getattr(self._implementation, "federateSaveComplete"))

    def federateSaveNotComplete(self) -> None:
        self._call(getattr(self._implementation, "federateSaveNotComplete"))

    def abortFederationSave(self) -> None:
        self._call(getattr(self._implementation, "abortFederationSave"))

    def queryFederationRestoreStatus(self) -> None:
        self._call(getattr(self._implementation, "queryFederationRestoreStatus"))

    def requestFederationRestore(self, label: str) -> None:
        self._call(getattr(self._implementation, "requestFederationRestore"), label)

    def federateRestoreComplete(self) -> None:
        self._call(getattr(self._implementation, "federateRestoreComplete"))

    def federateRestoreNotComplete(self) -> None:
        self._call(getattr(self._implementation, "federateRestoreNotComplete"))

    def abortFederationRestore(self) -> None:
        self._call(getattr(self._implementation, "abortFederationRestore"))

    def _attribute_handle_set(self, attributes: AttributeHandleSet) -> object:
        return self._call(
            self._runtime.attribute_handle_set,
            self._implementation,
            _attribute_handle_bytes(attributes),
        )

    def _interaction_class_set(
        self, interactions: InteractionClassHandleSet | MutableInteractionClassHandleSet
    ) -> object:
        return self._call(
            self._runtime.interaction_class_set,
            self._implementation,
            _interaction_class_handle_bytes(interactions),
        )

    def _attribute_region_pair_list(
        self,
        values: AttributeSetRegionSetPairList | MutableAttributeSetRegionSetPairList,
    ) -> object:
        return self._call(
            self._runtime.attribute_set_region_set_pair_list,
            self._implementation,
            _attribute_region_pairs(values),
        )

    def publishObjectClassAttributes(
        self, objectClass: ObjectClassHandle, attributes: AttributeHandleSet
    ) -> None:
        self._call(
            getattr(self._implementation, "publishObjectClassAttributes"),
            self._decode_handle(
                "getObjectClassHandleFactory",
                _encoded_handle(objectClass, ObjectClassHandle),
            ),
            self._attribute_handle_set(attributes),
        )

    def unpublishObjectClass(self, objectClass: ObjectClassHandle) -> None:
        self._call(
            getattr(self._implementation, "unpublishObjectClass"),
            self._decode_handle(
                "getObjectClassHandleFactory",
                _encoded_handle(objectClass, ObjectClassHandle),
            ),
        )

    def unpublishObjectClassAttributes(
        self, objectClass: ObjectClassHandle, attributes: AttributeHandleSet
    ) -> None:
        self._call(
            getattr(self._implementation, "unpublishObjectClassAttributes"),
            self._decode_handle(
                "getObjectClassHandleFactory",
                _encoded_handle(objectClass, ObjectClassHandle),
            ),
            self._attribute_handle_set(attributes),
        )

    def publishObjectClassDirectedInteractions(
        self,
        objectClass: ObjectClassHandle,
        interactionClasses: InteractionClassHandleSet,
    ) -> None:
        self._call(
            getattr(self._implementation, "publishObjectClassDirectedInteractions"),
            self._decode_handle(
                "getObjectClassHandleFactory",
                _encoded_handle(objectClass, ObjectClassHandle),
            ),
            self._interaction_class_set(interactionClasses),
        )

    def unpublishObjectClassDirectedInteractions(
        self,
        objectClass: ObjectClassHandle,
        interactionClasses: InteractionClassHandleSet | None = None,
    ) -> None:
        arguments: list[object] = [
            self._decode_handle(
                "getObjectClassHandleFactory",
                _encoded_handle(objectClass, ObjectClassHandle),
            )
        ]
        if interactionClasses is not None:
            arguments.append(self._interaction_class_set(interactionClasses))
        self._call(getattr(self._implementation, "unpublishObjectClassDirectedInteractions"), *arguments)

    def subscribeObjectClassAttributes(
        self,
        objectClass: ObjectClassHandle,
        attributes: AttributeHandleSet,
        *,
        active: bool = True,
        updateRateDesignator: str = "",
    ) -> None:
        arguments: list[object] = [
            self._decode_handle(
                "getObjectClassHandleFactory",
                _encoded_handle(objectClass, ObjectClassHandle),
            ),
            self._attribute_handle_set(attributes),
        ]
        if updateRateDesignator:
            arguments.append(str(updateRateDesignator))
        method = (
            "subscribeObjectClassAttributes"
            if active
            else "subscribeObjectClassAttributesPassively"
        )
        self._call(getattr(self._implementation, method), *arguments)

    def subscribeObjectClassAttributesPassively(
        self,
        objectClass: ObjectClassHandle,
        attributes: AttributeHandleSet,
        updateRateDesignator: str = "",
    ) -> None:
        self._call(
            getattr(self._implementation, "subscribeObjectClassAttributesPassively"),
            self._decode_handle("getObjectClassHandleFactory", _encoded_handle(objectClass, ObjectClassHandle)),
            self._attribute_handle_set(attributes),
            *([str(updateRateDesignator)] if updateRateDesignator else []),
        )

    def subscribeObjectClassDirectedInteractions(
        self,
        objectClass: ObjectClassHandle,
        interactionClasses: InteractionClassHandleSet,
        *,
        universally: bool = False,
    ) -> None:
        method = (
            "subscribeObjectClassDirectedInteractionsUniversally"
            if universally
            else "subscribeObjectClassDirectedInteractions"
        )
        self._call(
            getattr(self._implementation, method),
            self._decode_handle(
                "getObjectClassHandleFactory",
                _encoded_handle(objectClass, ObjectClassHandle),
            ),
            self._interaction_class_set(interactionClasses),
        )

    def subscribeObjectClassDirectedInteractionsUniversally(
        self,
        objectClass: ObjectClassHandle,
        interactionClasses: InteractionClassHandleSet | MutableInteractionClassHandleSet,
    ) -> None:
        self._call(
            getattr(self._implementation, "subscribeObjectClassDirectedInteractionsUniversally"),
            self._decode_handle("getObjectClassHandleFactory", _encoded_handle(objectClass, ObjectClassHandle)),
            self._interaction_class_set(interactionClasses),
        )

    def unsubscribeObjectClass(self, objectClass: ObjectClassHandle) -> None:
        self._call(
            getattr(self._implementation, "unsubscribeObjectClass"),
            self._decode_handle(
                "getObjectClassHandleFactory",
                _encoded_handle(objectClass, ObjectClassHandle),
            ),
        )

    def unsubscribeObjectClassAttributes(
        self, objectClass: ObjectClassHandle, attributes: AttributeHandleSet
    ) -> None:
        self._call(
            getattr(self._implementation, "unsubscribeObjectClassAttributes"),
            self._decode_handle(
                "getObjectClassHandleFactory",
                _encoded_handle(objectClass, ObjectClassHandle),
            ),
            self._attribute_handle_set(attributes),
        )

    def unsubscribeObjectClassDirectedInteractions(
        self,
        objectClass: ObjectClassHandle,
        interactionClasses: InteractionClassHandleSet | None = None,
    ) -> None:
        arguments: list[object] = [
            self._decode_handle(
                "getObjectClassHandleFactory",
                _encoded_handle(objectClass, ObjectClassHandle),
            )
        ]
        if interactionClasses is not None:
            arguments.append(self._interaction_class_set(interactionClasses))
        self._call(getattr(self._implementation, "unsubscribeObjectClassDirectedInteractions"), *arguments)

    def subscribeObjectClassAttributesWithRegions(
        self,
        objectClass: ObjectClassHandle,
        attributesAndRegions: AttributeSetRegionSetPairList,
        *,
        active: bool = True,
        updateRateDesignator: str = "",
    ) -> None:
        arguments: list[object] = [
            self._decode_handle(
                "getObjectClassHandleFactory",
                _encoded_handle(objectClass, ObjectClassHandle),
            ),
            self._attribute_region_pair_list(attributesAndRegions),
        ]
        if updateRateDesignator:
            arguments.append(str(updateRateDesignator))
        method = (
            "subscribeObjectClassAttributesWithRegions"
            if active
            else "subscribeObjectClassAttributesPassivelyWithRegions"
        )
        self._call(getattr(self._implementation, method), *arguments)

    def subscribeObjectClassAttributesPassivelyWithRegions(
        self,
        objectClass: ObjectClassHandle,
        attributesAndRegions: AttributeSetRegionSetPairList | MutableAttributeSetRegionSetPairList,
        updateRateDesignator: str = "",
    ) -> None:
        self._call(
            getattr(self._implementation, "subscribeObjectClassAttributesPassivelyWithRegions"),
            self._decode_handle("getObjectClassHandleFactory", _encoded_handle(objectClass, ObjectClassHandle)),
            self._attribute_region_pair_list(attributesAndRegions),
            *([str(updateRateDesignator)] if updateRateDesignator else []),
        )

    def unsubscribeObjectClassAttributesWithRegions(
        self,
        objectClass: ObjectClassHandle,
        attributesAndRegions: AttributeSetRegionSetPairList,
    ) -> None:
        self._call(
            getattr(self._implementation, "unsubscribeObjectClassAttributesWithRegions"),
            self._decode_handle(
                "getObjectClassHandleFactory",
                _encoded_handle(objectClass, ObjectClassHandle),
            ),
            self._attribute_region_pair_list(attributesAndRegions),
        )

    def publishInteractionClass(self, interactionClass: InteractionClassHandle) -> None:
        self._call(
            getattr(self._implementation, "publishInteractionClass"),
            self._decode_handle(
                "getInteractionClassHandleFactory",
                _encoded_handle(interactionClass, InteractionClassHandle),
            ),
        )

    def unpublishInteractionClass(self, interactionClass: InteractionClassHandle) -> None:
        self._call(
            getattr(self._implementation, "unpublishInteractionClass"),
            self._decode_handle(
                "getInteractionClassHandleFactory",
                _encoded_handle(interactionClass, InteractionClassHandle),
            ),
        )

    def subscribeInteractionClass(
        self, interactionClass: InteractionClassHandle, *, active: bool = True
    ) -> None:
        method = "subscribeInteractionClass" if active else "subscribeInteractionClassPassively"
        self._call(
            getattr(self._implementation, method),
            self._decode_handle(
                "getInteractionClassHandleFactory",
                _encoded_handle(interactionClass, InteractionClassHandle),
            ),
        )

    def subscribeInteractionClassPassively(
        self, interactionClass: InteractionClassHandle
    ) -> None:
        self._call(
            getattr(self._implementation, "subscribeInteractionClassPassively"),
            self._decode_handle("getInteractionClassHandleFactory", _encoded_handle(interactionClass, InteractionClassHandle)),
        )

    def unsubscribeInteractionClass(self, interactionClass: InteractionClassHandle) -> None:
        self._call(
            getattr(self._implementation, "unsubscribeInteractionClass"),
            self._decode_handle(
                "getInteractionClassHandleFactory",
                _encoded_handle(interactionClass, InteractionClassHandle),
            ),
        )

    def reserveObjectInstanceName(self, objectInstanceName: str) -> None:
        self._call(
            getattr(self._implementation, "reserveObjectInstanceName"), str(objectInstanceName)
        )

    def releaseObjectInstanceName(self, objectInstanceName: str) -> None:
        self._call(
            getattr(self._implementation, "releaseObjectInstanceName"), str(objectInstanceName)
        )

    def reserveMultipleObjectInstanceNames(
        self, objectInstanceNames: ObjectInstanceNameSet
    ) -> None:
        self._call(
            getattr(self._implementation, "reserveMultipleObjectInstanceNames"),
            self._runtime.object_instance_name_set(_object_instance_names(objectInstanceNames)),
        )

    def releaseMultipleObjectInstanceNames(
        self, objectInstanceNames: ObjectInstanceNameSet
    ) -> None:
        self._call(
            getattr(self._implementation, "releaseMultipleObjectInstanceNames"),
            self._runtime.object_instance_name_set(_object_instance_names(objectInstanceNames)),
        )

    def registerObjectInstance(
        self,
        objectClass: ObjectClassHandle,
        *,
        objectInstanceName: str | None = None,
    ) -> ObjectInstanceHandle:
        decoded_object_class = self._decode_handle(
            "getObjectClassHandleFactory",
            _encoded_handle(objectClass, ObjectClassHandle),
        )
        if objectInstanceName is None:
            raw_handle = self._call(
                getattr(self._implementation, "registerObjectInstance"),
                decoded_object_class,
            )
        else:
            raw_handle = self._call(
                getattr(self._implementation, "registerObjectInstance"),
                decoded_object_class,
                str(objectInstanceName),
            )
        return ObjectInstanceHandle(self._runtime.handle_bytes(raw_handle))

    def registerObjectInstanceWithRegions(
        self,
        objectClass: ObjectClassHandle,
        attributesAndRegions: AttributeSetRegionSetPairList,
        *,
        objectInstanceName: str | None = None,
    ) -> ObjectInstanceHandle:
        decoded_object_class = self._decode_handle(
            "getObjectClassHandleFactory",
            _encoded_handle(objectClass, ObjectClassHandle),
        )
        pair_list = self._attribute_region_pair_list(attributesAndRegions)
        if objectInstanceName is None:
            raw_handle = self._call(
                getattr(self._implementation, "registerObjectInstanceWithRegions"),
                decoded_object_class,
                pair_list,
            )
        else:
            raw_handle = self._call(
                getattr(self._implementation, "registerObjectInstanceWithRegions"),
                decoded_object_class,
                pair_list,
                str(objectInstanceName),
            )
        return ObjectInstanceHandle(self._runtime.handle_bytes(raw_handle))

    def associateRegionsForUpdates(
        self,
        objectInstance: ObjectInstanceHandle,
        attributesAndRegions: AttributeSetRegionSetPairList,
    ) -> None:
        self._call(
            getattr(self._implementation, "associateRegionsForUpdates"),
            self._decode_handle(
                "getObjectInstanceHandleFactory",
                _encoded_handle(objectInstance, ObjectInstanceHandle),
            ),
            self._attribute_region_pair_list(attributesAndRegions),
        )

    def unassociateRegionsForUpdates(
        self,
        objectInstance: ObjectInstanceHandle,
        attributesAndRegions: AttributeSetRegionSetPairList,
    ) -> None:
        self._call(
            getattr(self._implementation, "unassociateRegionsForUpdates"),
            self._decode_handle(
                "getObjectInstanceHandleFactory",
                _encoded_handle(objectInstance, ObjectInstanceHandle),
            ),
            self._attribute_region_pair_list(attributesAndRegions),
        )

    def getObjectInstanceHandle(self, objectInstanceName: str) -> ObjectInstanceHandle:
        raw_handle = self._call(
            getattr(self._implementation, "getObjectInstanceHandle"), str(objectInstanceName)
        )
        return ObjectInstanceHandle(self._runtime.handle_bytes(raw_handle))

    def getObjectInstanceName(self, objectInstance: ObjectInstanceHandle) -> str:
        return str(
            self._call(
                getattr(self._implementation, "getObjectInstanceName"),
                self._decode_handle(
                    "getObjectInstanceHandleFactory",
                    _encoded_handle(objectInstance, ObjectInstanceHandle),
                ),
            )
        )

    def deleteObjectInstance(
        self, objectInstance: ObjectInstanceHandle, userSuppliedTag: bytes = b""
    ) -> None:
        self._call(
            getattr(self._implementation, "deleteObjectInstance"),
            self._decode_handle(
                "getObjectInstanceHandleFactory",
                _encoded_handle(objectInstance, ObjectInstanceHandle),
            ),
            self._runtime.byte_array(bytes(userSuppliedTag)),
        )

    def localDeleteObjectInstance(self, objectInstance: ObjectInstanceHandle) -> None:
        self._call(
            getattr(self._implementation, "localDeleteObjectInstance"),
            self._decode_handle(
                "getObjectInstanceHandleFactory",
                _encoded_handle(objectInstance, ObjectInstanceHandle),
            ),
        )

    def deleteObjectInstanceWithTime(
        self,
        objectInstance: ObjectInstanceHandle,
        time: LogicalTime,
        userSuppliedTag: bytes = b"",
    ) -> MessageRetractionHandle:
        if not isinstance(time, LogicalTime):
            raise TypeError("time must be LogicalTime")
        implementation = getattr(self._implementation, "deleteObjectInstanceWithTime", None)
        if implementation is None:
            implementation = getattr(self._implementation, "deleteObjectInstance")
        raw_handle = self._call(
            implementation,
            self._decode_handle(
                "getObjectInstanceHandleFactory", _encoded_handle(objectInstance, ObjectInstanceHandle)
            ),
            self._runtime.byte_array(bytes(userSuppliedTag)),
            self._runtime.decode_logical_time(self._implementation, time.toByteArray()),
        )
        return self._remember_retraction(raw_handle)

    def updateAttributeValues(
        self,
        objectInstance: ObjectInstanceHandle,
        attributeValues: AttributeHandleValueMap,
        userSuppliedTag: bytes = b"",
    ) -> None:
        self._call(
            getattr(self._implementation, "updateAttributeValues"),
            self._decode_handle(
                "getObjectInstanceHandleFactory",
                _encoded_handle(objectInstance, ObjectInstanceHandle),
            ),
            self._call(
                self._runtime.attribute_handle_value_map,
                self._implementation,
                _attribute_value_pairs(attributeValues),
            ),
            self._runtime.byte_array(bytes(userSuppliedTag)),
        )

    def updateAttributeValuesWithTime(
        self,
        objectInstance: ObjectInstanceHandle,
        attributeValues: AttributeHandleValueMap,
        time: LogicalTime,
        userSuppliedTag: bytes = b"",
    ) -> MessageRetractionHandle:
        if not isinstance(time, LogicalTime):
            raise TypeError("time must be LogicalTime")
        implementation = getattr(self._implementation, "updateAttributeValuesWithTime", None)
        if implementation is None:
            implementation = getattr(self._implementation, "updateAttributeValues")
        raw_handle = self._call(
            implementation,
            self._decode_handle(
                "getObjectInstanceHandleFactory", _encoded_handle(objectInstance, ObjectInstanceHandle)
            ),
            self._call(
                self._runtime.attribute_handle_value_map,
                self._implementation,
                _attribute_value_pairs(attributeValues),
            ),
            self._runtime.byte_array(bytes(userSuppliedTag)),
            self._runtime.decode_logical_time(self._implementation, time.toByteArray()),
        )
        return self._remember_retraction(raw_handle)

    def requestAttributeValueUpdate(
        self,
        objectClassOrInstance: ObjectClassHandle | ObjectInstanceHandle,
        attributes: AttributeHandleSet,
        userSuppliedTag: bytes = b"",
    ) -> None:
        if isinstance(objectClassOrInstance, ObjectClassHandle):
            implementation = getattr(self._implementation, "requestAttributeValueUpdate")
            handle = self._decode_handle(
                "getObjectClassHandleFactory",
                _encoded_handle(objectClassOrInstance, ObjectClassHandle),
            )
            handle = self._runtime.cast_handle(handle, "hla.rti1516_2025.ObjectClassHandle")
        elif isinstance(objectClassOrInstance, ObjectInstanceHandle):
            implementation = getattr(self._implementation, "requestAttributeValueUpdate")
            handle = self._decode_handle(
                "getObjectInstanceHandleFactory",
                _encoded_handle(objectClassOrInstance, ObjectInstanceHandle),
            )
            handle = self._runtime.cast_handle(handle, "hla.rti1516_2025.ObjectInstanceHandle")
        else:
            raise TypeError(
                "objectClassOrInstance must be ObjectClassHandle or ObjectInstanceHandle"
            )
        self._call(
            implementation,
            handle,
            self._attribute_handle_set(attributes),
            self._runtime.byte_array(bytes(userSuppliedTag)),
        )

    def requestAttributeValueUpdateWithRegions(
        self,
        objectClass: ObjectClassHandle,
        attributesAndRegions: AttributeSetRegionSetPairList,
        userSuppliedTag: bytes = b"",
    ) -> None:
        self._call(
            getattr(self._implementation, "requestAttributeValueUpdateWithRegions"),
            self._decode_handle(
                "getObjectClassHandleFactory",
                _encoded_handle(objectClass, ObjectClassHandle),
            ),
            self._attribute_region_pair_list(attributesAndRegions),
            self._runtime.byte_array(bytes(userSuppliedTag)),
        )

    def changeAttributeOrderType(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
        orderType: OrderType,
    ) -> None:
        if not isinstance(orderType, OrderType):
            raise TypeError("orderType must be OrderType")
        self._call(
            getattr(self._implementation, "changeAttributeOrderType"),
            self._decode_handle(
                "getObjectInstanceHandleFactory",
                _encoded_handle(objectInstance, ObjectInstanceHandle),
            ),
            self._attribute_handle_set(attributes),
            self._runtime.order_type(orderType.name),
        )

    def changeDefaultAttributeOrderType(
        self,
        objectClass: ObjectClassHandle,
        attributes: AttributeHandleSet,
        orderType: OrderType,
    ) -> None:
        if not isinstance(orderType, OrderType):
            raise TypeError("orderType must be OrderType")
        self._call(
            getattr(self._implementation, "changeDefaultAttributeOrderType"),
            self._decode_handle(
                "getObjectClassHandleFactory",
                _encoded_handle(objectClass, ObjectClassHandle),
            ),
            self._attribute_handle_set(attributes),
            self._runtime.order_type(orderType.name),
        )

    def changeInteractionOrderType(
        self, interactionClass: InteractionClassHandle, orderType: OrderType
    ) -> None:
        if not isinstance(orderType, OrderType):
            raise TypeError("orderType must be OrderType")
        self._call(
            getattr(self._implementation, "changeInteractionOrderType"),
            self._decode_handle(
                "getInteractionClassHandleFactory",
                _encoded_handle(interactionClass, InteractionClassHandle),
            ),
            self._runtime.order_type(orderType.name),
        )

    def requestAttributeTransportationTypeChange(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
        transportationType: TransportationTypeHandle,
    ) -> None:
        self._call(
            getattr(self._implementation, "requestAttributeTransportationTypeChange"),
            self._decode_handle(
                "getObjectInstanceHandleFactory",
                _encoded_handle(objectInstance, ObjectInstanceHandle),
            ),
            self._attribute_handle_set(attributes),
            self._decode_handle(
                "getTransportationTypeHandleFactory",
                _encoded_handle(transportationType, TransportationTypeHandle),
            ),
        )

    def changeDefaultAttributeTransportationType(
        self,
        objectClass: ObjectClassHandle,
        attributes: AttributeHandleSet,
        transportationType: TransportationTypeHandle,
    ) -> None:
        self._call(
            getattr(self._implementation, "changeDefaultAttributeTransportationType"),
            self._decode_handle(
                "getObjectClassHandleFactory",
                _encoded_handle(objectClass, ObjectClassHandle),
            ),
            self._attribute_handle_set(attributes),
            self._decode_handle(
                "getTransportationTypeHandleFactory",
                _encoded_handle(transportationType, TransportationTypeHandle),
            ),
        )

    def queryAttributeTransportationType(
        self, objectInstance: ObjectInstanceHandle, attribute: AttributeHandle
    ) -> None:
        self._call(
            getattr(self._implementation, "queryAttributeTransportationType"),
            self._decode_handle(
                "getObjectInstanceHandleFactory",
                _encoded_handle(objectInstance, ObjectInstanceHandle),
            ),
            self._decode_handle(
                "getAttributeHandleFactory", _encoded_handle(attribute, AttributeHandle)
            ),
        )

    def requestInteractionTransportationTypeChange(
        self,
        interactionClass: InteractionClassHandle,
        transportationType: TransportationTypeHandle,
    ) -> None:
        self._call(
            getattr(self._implementation, "requestInteractionTransportationTypeChange"),
            self._decode_handle(
                "getInteractionClassHandleFactory",
                _encoded_handle(interactionClass, InteractionClassHandle),
            ),
            self._decode_handle(
                "getTransportationTypeHandleFactory",
                _encoded_handle(transportationType, TransportationTypeHandle),
            ),
        )

    def queryInteractionTransportationType(
        self, federate: FederateHandle, interactionClass: InteractionClassHandle
    ) -> None:
        self._call(
            getattr(self._implementation, "queryInteractionTransportationType"),
            self._decode_handle(
                "getFederateHandleFactory", _encoded_handle(federate, FederateHandle)
            ),
            self._decode_handle(
                "getInteractionClassHandleFactory",
                _encoded_handle(interactionClass, InteractionClassHandle),
            ),
        )

    def queryAttributeOwnership(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
    ) -> None:
        self._call(
            getattr(self._implementation, "queryAttributeOwnership"),
            self._decode_handle(
                "getObjectInstanceHandleFactory",
                _encoded_handle(objectInstance, ObjectInstanceHandle),
            ),
            self._attribute_handle_set(attributes),
        )

    def isAttributeOwnedByFederate(
        self,
        objectInstance: ObjectInstanceHandle,
        attribute: AttributeHandle,
    ) -> bool:
        return bool(
            self._call(
                getattr(self._implementation, "isAttributeOwnedByFederate"),
                self._decode_handle(
                    "getObjectInstanceHandleFactory",
                    _encoded_handle(objectInstance, ObjectInstanceHandle),
                ),
                self._decode_handle(
                    "getAttributeHandleFactory",
                    _encoded_handle(attribute, AttributeHandle),
                ),
            )
        )

    def unconditionalAttributeOwnershipDivestiture(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
        userSuppliedTag: bytes = b"",
    ) -> None:
        self._call(
            getattr(self._implementation, "unconditionalAttributeOwnershipDivestiture"),
            self._decode_handle(
                "getObjectInstanceHandleFactory",
                _encoded_handle(objectInstance, ObjectInstanceHandle),
            ),
            self._attribute_handle_set(attributes),
            self._runtime.byte_array(bytes(userSuppliedTag)),
        )

    def negotiatedAttributeOwnershipDivestiture(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
        userSuppliedTag: bytes = b"",
    ) -> None:
        self._call(
            getattr(self._implementation, "negotiatedAttributeOwnershipDivestiture"),
            self._decode_handle(
                "getObjectInstanceHandleFactory",
                _encoded_handle(objectInstance, ObjectInstanceHandle),
            ),
            self._attribute_handle_set(attributes),
            self._runtime.byte_array(bytes(userSuppliedTag)),
        )

    def confirmDivestiture(
        self,
        objectInstance: ObjectInstanceHandle,
        confirmedAttributes: AttributeHandleSet,
        userSuppliedTag: bytes = b"",
    ) -> None:
        self._call(
            getattr(self._implementation, "confirmDivestiture"),
            self._decode_handle(
                "getObjectInstanceHandleFactory",
                _encoded_handle(objectInstance, ObjectInstanceHandle),
            ),
            self._attribute_handle_set(confirmedAttributes),
            self._runtime.byte_array(bytes(userSuppliedTag)),
        )

    def cancelNegotiatedAttributeOwnershipDivestiture(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
    ) -> None:
        self._call(
            getattr(self._implementation, "cancelNegotiatedAttributeOwnershipDivestiture"),
            self._decode_handle(
                "getObjectInstanceHandleFactory",
                _encoded_handle(objectInstance, ObjectInstanceHandle),
            ),
            self._attribute_handle_set(attributes),
        )

    def attributeOwnershipAcquisition(
        self,
        objectInstance: ObjectInstanceHandle,
        desiredAttributes: AttributeHandleSet,
        userSuppliedTag: bytes = b"",
    ) -> None:
        self._call(
            getattr(self._implementation, "attributeOwnershipAcquisition"),
            self._decode_handle(
                "getObjectInstanceHandleFactory",
                _encoded_handle(objectInstance, ObjectInstanceHandle),
            ),
            self._attribute_handle_set(desiredAttributes),
            self._runtime.byte_array(bytes(userSuppliedTag)),
        )

    def attributeOwnershipAcquisitionIfAvailable(
        self,
        objectInstance: ObjectInstanceHandle,
        desiredAttributes: AttributeHandleSet,
        userSuppliedTag: bytes = b"",
    ) -> None:
        self._call(
            getattr(self._implementation, "attributeOwnershipAcquisitionIfAvailable"),
            self._decode_handle(
                "getObjectInstanceHandleFactory",
                _encoded_handle(objectInstance, ObjectInstanceHandle),
            ),
            self._attribute_handle_set(desiredAttributes),
            self._runtime.byte_array(bytes(userSuppliedTag)),
        )

    def cancelAttributeOwnershipAcquisition(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
    ) -> None:
        self._call(
            getattr(self._implementation, "cancelAttributeOwnershipAcquisition"),
            self._decode_handle(
                "getObjectInstanceHandleFactory",
                _encoded_handle(objectInstance, ObjectInstanceHandle),
            ),
            self._attribute_handle_set(attributes),
        )

    def attributeOwnershipReleaseDenied(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
        userSuppliedTag: bytes = b"",
    ) -> None:
        self._call(
            getattr(self._implementation, "attributeOwnershipReleaseDenied"),
            self._decode_handle(
                "getObjectInstanceHandleFactory",
                _encoded_handle(objectInstance, ObjectInstanceHandle),
            ),
            self._attribute_handle_set(attributes),
            self._runtime.byte_array(bytes(userSuppliedTag)),
        )

    def attributeOwnershipDivestitureIfWanted(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
        userSuppliedTag: bytes = b"",
    ) -> AttributeHandleSet:
        raw_attributes = self._call(
            getattr(self._implementation, "attributeOwnershipDivestitureIfWanted"),
            self._decode_handle(
                "getObjectInstanceHandleFactory",
                _encoded_handle(objectInstance, ObjectInstanceHandle),
            ),
            self._attribute_handle_set(attributes),
            self._runtime.byte_array(bytes(userSuppliedTag)),
        )
        return AttributeHandleSet(
            AttributeHandle(self._runtime.handle_bytes(attribute)) for attribute in raw_attributes
        )

    def sendInteraction(
        self,
        interactionClass: InteractionClassHandle,
        parameterValues: ParameterHandleValueMap,
        userSuppliedTag: bytes = b"",
    ) -> None:
        self._call(
            getattr(self._implementation, "sendInteraction"),
            self._decode_handle(
                "getInteractionClassHandleFactory",
                _encoded_handle(interactionClass, InteractionClassHandle),
            ),
            self._call(
                self._runtime.parameter_handle_value_map,
                self._implementation,
                _parameter_value_pairs(parameterValues),
            ),
            self._runtime.byte_array(bytes(userSuppliedTag)),
        )

    def sendInteractionWithTime(
        self,
        interactionClass: InteractionClassHandle,
        parameterValues: ParameterHandleValueMap,
        time: LogicalTime,
        userSuppliedTag: bytes = b"",
    ) -> MessageRetractionHandle:
        if not isinstance(time, LogicalTime):
            raise TypeError("time must be LogicalTime")
        implementation = getattr(self._implementation, "sendInteractionWithTime", None)
        if implementation is None:
            implementation = getattr(self._implementation, "sendInteraction")
        raw_handle = self._call(
            implementation,
            self._decode_handle(
                "getInteractionClassHandleFactory",
                _encoded_handle(interactionClass, InteractionClassHandle),
            ),
            self._call(
                self._runtime.parameter_handle_value_map,
                self._implementation,
                _parameter_value_pairs(parameterValues),
            ),
            self._runtime.byte_array(bytes(userSuppliedTag)),
            self._runtime.decode_logical_time(self._implementation, time.toByteArray()),
        )
        return self._remember_retraction(raw_handle)

    def sendDirectedInteraction(
        self,
        interactionClass: InteractionClassHandle,
        objectInstance: ObjectInstanceHandle,
        parameterValues: ParameterHandleValueMap,
        userSuppliedTag: bytes = b"",
    ) -> None:
        self._call(
            getattr(self._implementation, "sendDirectedInteraction"),
            self._decode_handle(
                "getInteractionClassHandleFactory",
                _encoded_handle(interactionClass, InteractionClassHandle),
            ),
            self._decode_handle(
                "getObjectInstanceHandleFactory",
                _encoded_handle(objectInstance, ObjectInstanceHandle),
            ),
            self._call(
                self._runtime.parameter_handle_value_map,
                self._implementation,
                _parameter_value_pairs(parameterValues),
            ),
            self._runtime.byte_array(bytes(userSuppliedTag)),
        )

    def sendDirectedInteractionWithTime(
        self,
        interactionClass: InteractionClassHandle,
        objectInstance: ObjectInstanceHandle,
        parameterValues: ParameterHandleValueMap,
        time: LogicalTime,
        userSuppliedTag: bytes = b"",
    ) -> MessageRetractionHandle:
        if not isinstance(time, LogicalTime):
            raise TypeError("time must be LogicalTime")
        raw_handle = self._call(
            getattr(self._implementation, "sendDirectedInteraction"),
            self._decode_handle(
                "getInteractionClassHandleFactory",
                _encoded_handle(interactionClass, InteractionClassHandle),
            ),
            self._decode_handle(
                "getObjectInstanceHandleFactory",
                _encoded_handle(objectInstance, ObjectInstanceHandle),
            ),
            self._call(
                self._runtime.parameter_handle_value_map,
                self._implementation,
                _parameter_value_pairs(parameterValues),
            ),
            self._runtime.byte_array(bytes(userSuppliedTag)),
            self._runtime.decode_logical_time(self._implementation, time.toByteArray()),
        )
        return self._remember_retraction(raw_handle)

    def subscribeInteractionClassWithRegions(
        self,
        interactionClass: InteractionClassHandle,
        regions: RegionHandleSet,
        *,
        active: bool = True,
    ) -> None:
        method = (
            "subscribeInteractionClassWithRegions"
            if active
            else "subscribeInteractionClassPassivelyWithRegions"
        )
        self._call(
            getattr(self._implementation, method),
            self._decode_handle(
                "getInteractionClassHandleFactory",
                _encoded_handle(interactionClass, InteractionClassHandle),
            ),
            self._call(
                self._runtime.region_handle_set,
                self._implementation,
                _region_handle_bytes(regions),
            ),
        )

    def subscribeInteractionClassPassivelyWithRegions(
        self, interactionClass: InteractionClassHandle, regions: RegionHandleSet
    ) -> None:
        self._call(
            getattr(self._implementation, "subscribeInteractionClassPassivelyWithRegions"),
            self._decode_handle("getInteractionClassHandleFactory", _encoded_handle(interactionClass, InteractionClassHandle)),
            self._call(
                self._runtime.region_handle_set,
                self._implementation,
                _region_handle_bytes(regions),
            ),
        )

    def unsubscribeInteractionClassWithRegions(
        self, interactionClass: InteractionClassHandle, regions: RegionHandleSet
    ) -> None:
        self._call(
            getattr(self._implementation, "unsubscribeInteractionClassWithRegions"),
            self._decode_handle(
                "getInteractionClassHandleFactory",
                _encoded_handle(interactionClass, InteractionClassHandle),
            ),
            self._call(
                self._runtime.region_handle_set,
                self._implementation,
                _region_handle_bytes(regions),
            ),
        )

    def sendInteractionWithRegions(
        self,
        interactionClass: InteractionClassHandle,
        parameterValues: ParameterHandleValueMap,
        regions: RegionHandleSet,
        userSuppliedTag: bytes = b"",
    ) -> None:
        self._call(
            getattr(self._implementation, "sendInteractionWithRegions"),
            self._decode_handle(
                "getInteractionClassHandleFactory",
                _encoded_handle(interactionClass, InteractionClassHandle),
            ),
            self._call(
                self._runtime.parameter_handle_value_map,
                self._implementation,
                _parameter_value_pairs(parameterValues),
            ),
            self._call(
                self._runtime.region_handle_set,
                self._implementation,
                _region_handle_bytes(regions),
            ),
            self._runtime.byte_array(bytes(userSuppliedTag)),
        )

    def sendInteractionWithRegionsWithTime(
        self,
        interactionClass: InteractionClassHandle,
        parameterValues: ParameterHandleValueMap,
        regions: RegionHandleSet,
        time: LogicalTime,
        userSuppliedTag: bytes = b"",
    ) -> MessageRetractionHandle:
        if not isinstance(time, LogicalTime):
            raise TypeError("time must be LogicalTime")
        implementation = getattr(self._implementation, "sendInteractionWithRegionsWithTime", None)
        if implementation is None:
            implementation = getattr(self._implementation, "sendInteractionWithRegions")
        raw_handle = self._call(
            implementation,
            self._decode_handle(
                "getInteractionClassHandleFactory",
                _encoded_handle(interactionClass, InteractionClassHandle),
            ),
            self._call(
                self._runtime.parameter_handle_value_map,
                self._implementation,
                _parameter_value_pairs(parameterValues),
            ),
            self._call(
                self._runtime.region_handle_set,
                self._implementation,
                _region_handle_bytes(regions),
            ),
            self._runtime.byte_array(bytes(userSuppliedTag)),
            self._runtime.decode_logical_time(self._implementation, time.toByteArray()),
        )
        return self._remember_retraction(raw_handle)

    def retract(self, retraction: MessageRetractionHandle) -> None:
        encoded = _encoded_handle(retraction, MessageRetractionHandle)
        raw_handle = self._message_retractions.get(encoded)
        if raw_handle is None:
            # The standard API accepts any provider-decoded handle here.  Do
            # not turn an unknown-but-well-formed value into a Python-side
            # RTIinternalError: the C++ RTI owns the handle ledger and must
            # report InvalidMessageRetractionHandle (or its other standard
            # failure) through the Java exception boundary.
            raw_handle = self._decode_handle(
                "getMessageRetractionHandleFactory", encoded
            )
        self._call(
            getattr(self._implementation, "retract"),
            raw_handle,
        )

    def getFederateHandleFactory(self) -> FederateHandleFactory:
        self._call(getattr(self._implementation, "getFederateHandleFactory"))
        return _JavaFederateHandleFactory(self, "getFederateHandleFactory", FederateHandle)

    def getFederateHandleSetFactory(self) -> FederateHandleSetFactory:
        return _JavaFederateHandleSetFactory(
            self, "getFederateHandleSetFactory", MutableFederateHandleSet
        )

    def getObjectClassHandleFactory(self) -> ObjectClassHandleFactory:
        self._call(getattr(self._implementation, "getObjectClassHandleFactory"))
        return _JavaObjectClassHandleFactory(self, "getObjectClassHandleFactory", ObjectClassHandle)

    def getAttributeHandleFactory(self) -> AttributeHandleFactory:
        self._call(getattr(self._implementation, "getAttributeHandleFactory"))
        return _JavaAttributeHandleFactory(self, "getAttributeHandleFactory", AttributeHandle)

    def getInteractionClassHandleFactory(self) -> InteractionClassHandleFactory:
        self._call(getattr(self._implementation, "getInteractionClassHandleFactory"))
        return _JavaInteractionClassHandleFactory(
            self, "getInteractionClassHandleFactory", InteractionClassHandle
        )

    def getInteractionClassHandleSetFactory(self) -> InteractionClassHandleSetFactory:
        return _JavaInteractionClassHandleSetFactory(
            self,
            "getInteractionClassHandleSetFactory",
            MutableInteractionClassHandleSet,
        )

    def getParameterHandleFactory(self) -> ParameterHandleFactory:
        self._call(getattr(self._implementation, "getParameterHandleFactory"))
        return _JavaParameterHandleFactory(self, "getParameterHandleFactory", ParameterHandle)

    def getTransportationTypeHandleFactory(self) -> TransportationTypeHandleFactory:
        self._call(getattr(self._implementation, "getTransportationTypeHandleFactory"))
        return _JavaTransportationTypeHandleFactory(
            self, "getTransportationTypeHandleFactory", TransportationTypeHandle
        )

    def getDimensionHandleFactory(self) -> DimensionHandleFactory:
        self._call(getattr(self._implementation, "getDimensionHandleFactory"))
        return _JavaDimensionHandleFactory(self, "getDimensionHandleFactory", DimensionHandle)

    def getRegionHandleFactory(self) -> RegionHandleFactory:
        self._call(getattr(self._implementation, "getRegionHandleFactory"))
        return _JavaRegionHandleFactory(self, "getRegionHandleFactory", RegionHandle)

    def getMessageRetractionHandleFactory(self) -> MessageRetractionHandleFactory:
        self._call(getattr(self._implementation, "getMessageRetractionHandleFactory"))
        return _JavaMessageRetractionHandleFactory(
            self,
            "getMessageRetractionHandleFactory",
            MessageRetractionHandle,
        )

    def getObjectInstanceHandleFactory(self) -> ObjectInstanceHandleFactory:
        self._call(getattr(self._implementation, "getObjectInstanceHandleFactory"))
        return _JavaObjectInstanceHandleFactory(
            self, "getObjectInstanceHandleFactory", ObjectInstanceHandle
        )

    def getDimensionHandleSetFactory(self) -> DimensionHandleSetFactory:
        return _JavaDimensionHandleSetFactory(
            self, "getDimensionHandleSetFactory", MutableDimensionHandleSet
        )

    def getRegionHandleSetFactory(self) -> RegionHandleSetFactory:
        return _JavaRegionHandleSetFactory(
            self, "getRegionHandleSetFactory", MutableRegionHandleSet
        )

    def getAttributeHandleSetFactory(self) -> AttributeHandleSetFactory:
        return _JavaAttributeHandleSetFactory(
            self, "getAttributeHandleSetFactory", MutableAttributeHandleSet
        )

    def getAttributeHandleValueMapFactory(self) -> AttributeHandleValueMapFactory:
        return _JavaAttributeHandleValueMapFactory(
            self, "getAttributeHandleValueMapFactory", MutableAttributeHandleValueMap
        )

    def getParameterHandleValueMapFactory(self) -> ParameterHandleValueMapFactory:
        return _JavaParameterHandleValueMapFactory(
            self, "getParameterHandleValueMapFactory", MutableParameterHandleValueMap
        )

    def getAttributeSetRegionSetPairListFactory(self) -> AttributeSetRegionSetPairListFactory:
        return _JavaAttributeSetRegionSetPairListFactory(self)

    def createRegion(self, dimensions: DimensionHandleSet) -> RegionHandle:
        raw_handle = self._call(
            getattr(self._implementation, "createRegion"),
            self._call(
                self._runtime.dimension_handle_set,
                self._implementation,
                _dimension_handle_bytes(dimensions),
            ),
        )
        return RegionHandle(self._runtime.handle_bytes(raw_handle))

    def commitRegionModifications(self, regions: RegionHandleSet) -> None:
        self._call(
            getattr(self._implementation, "commitRegionModifications"),
            self._call(
                self._runtime.region_handle_set,
                self._implementation,
                _region_handle_bytes(regions),
            ),
        )

    def deleteRegion(self, region: RegionHandle) -> None:
        self._call(
            getattr(self._implementation, "deleteRegion"),
            self._decode_handle("getRegionHandleFactory", _encoded_handle(region, RegionHandle)),
        )

    def getDimensionHandleSet(self, region: RegionHandle) -> DimensionHandleSet:
        raw_set = self._call(
            getattr(self._implementation, "getDimensionHandleSet"),
            self._decode_handle("getRegionHandleFactory", _encoded_handle(region, RegionHandle)),
        )
        return DimensionHandleSet(
            DimensionHandle(self._runtime.handle_bytes(value)) for value in raw_set
        )

    def getRangeBounds(self, region: RegionHandle, dimension: DimensionHandle) -> RangeBounds:
        raw_bounds = self._call(
            getattr(self._implementation, "getRangeBounds"),
            self._decode_handle("getRegionHandleFactory", _encoded_handle(region, RegionHandle)),
            self._decode_handle("getDimensionHandleFactory", _encoded_handle(dimension, DimensionHandle)),
        )
        lower = getattr(raw_bounds, "lower", None)
        upper = getattr(raw_bounds, "upper", None)
        if lower is None or upper is None:
            lower = raw_bounds.getLowerBound()
            upper = raw_bounds.getUpperBound()
        return RangeBounds(int(lower), int(upper))

    def setRangeBounds(
        self,
        region: RegionHandle,
        dimension: DimensionHandle,
        rangeBounds: RangeBounds,
    ) -> None:
        if not isinstance(rangeBounds, RangeBounds):
            raise TypeError("rangeBounds must be RangeBounds")
        java_bounds = self._runtime.range_bounds(
            rangeBounds.getLowerBound(), rangeBounds.getUpperBound()
        )
        self._call(
            getattr(self._implementation, "setRangeBounds"),
            self._decode_handle("getRegionHandleFactory", _encoded_handle(region, RegionHandle)),
            self._decode_handle("getDimensionHandleFactory", _encoded_handle(dimension, DimensionHandle)),
            java_bounds,
        )

    def getConveyRegionDesignatorSetsSwitch(self) -> bool:
        return bool(
            self._call(getattr(self._implementation, "getConveyRegionDesignatorSetsSwitch"))
        )

    def setConveyRegionDesignatorSetsSwitch(self, switchValue: bool) -> None:
        self._call(
            getattr(self._implementation, "setConveyRegionDesignatorSetsSwitch"),
            bool(switchValue),
        )

    def getObjectClassRelevanceAdvisorySwitch(self) -> bool:
        return bool(
            self._call(
                getattr(self._implementation, "getObjectClassRelevanceAdvisorySwitch")
            )
        )

    def setObjectClassRelevanceAdvisorySwitch(self, switchValue: bool) -> None:
        self._call(
            getattr(self._implementation, "setObjectClassRelevanceAdvisorySwitch"),
            bool(switchValue),
        )

    def getAttributeRelevanceAdvisorySwitch(self) -> bool:
        return bool(
            self._call(
                getattr(self._implementation, "getAttributeRelevanceAdvisorySwitch")
            )
        )

    def setAttributeRelevanceAdvisorySwitch(self, switchValue: bool) -> None:
        self._call(
            getattr(self._implementation, "setAttributeRelevanceAdvisorySwitch"),
            bool(switchValue),
        )

    def getAttributeScopeAdvisorySwitch(self) -> bool:
        return bool(
            self._call(
                getattr(self._implementation, "getAttributeScopeAdvisorySwitch")
            )
        )

    def setAttributeScopeAdvisorySwitch(self, switchValue: bool) -> None:
        self._call(
            getattr(self._implementation, "setAttributeScopeAdvisorySwitch"),
            bool(switchValue),
        )

    def getInteractionRelevanceAdvisorySwitch(self) -> bool:
        return bool(
            self._call(
                getattr(self._implementation, "getInteractionRelevanceAdvisorySwitch")
            )
        )

    def setInteractionRelevanceAdvisorySwitch(self, switchValue: bool) -> None:
        self._call(
            getattr(self._implementation, "setInteractionRelevanceAdvisorySwitch"),
            bool(switchValue),
        )

    def getAutomaticResignDirective(self) -> ResignAction:
        name = _enum_name(
            self._call(getattr(self._implementation, "getAutomaticResignDirective"))
        )
        try:
            return ResignAction[name]
        except KeyError as error:
            raise RTIinternalError(f"Java RTI returned an unknown ResignAction: {name}") from error

    def setAutomaticResignDirective(self, resignAction: ResignAction) -> None:
        if not isinstance(resignAction, ResignAction):
            raise TypeError("resignAction must be ResignAction")
        self._call(
            getattr(self._implementation, "setAutomaticResignDirective"),
            self._runtime.resign_action(resignAction.name),
        )

    def getServiceReportingSwitch(self) -> bool:
        return bool(
            self._call(getattr(self._implementation, "getServiceReportingSwitch"))
        )

    def setServiceReportingSwitch(self, switchValue: bool) -> None:
        self._call(
            getattr(self._implementation, "setServiceReportingSwitch"),
            bool(switchValue),
        )

    def getExceptionReportingSwitch(self) -> bool:
        return bool(
            self._call(getattr(self._implementation, "getExceptionReportingSwitch"))
        )

    def setExceptionReportingSwitch(self, switchValue: bool) -> None:
        self._call(
            getattr(self._implementation, "setExceptionReportingSwitch"),
            bool(switchValue),
        )

    def getSendServiceReportsToFileSwitch(self) -> bool:
        return bool(
            self._call(getattr(self._implementation, "getSendServiceReportsToFileSwitch"))
        )

    def setSendServiceReportsToFileSwitch(self, switchValue: bool) -> None:
        self._call(
            getattr(self._implementation, "setSendServiceReportsToFileSwitch"),
            bool(switchValue),
        )

    def getAutoProvideSwitch(self) -> bool:
        return bool(self._call(getattr(self._implementation, "getAutoProvideSwitch")))

    def getDelaySubscriptionEvaluationSwitch(self) -> bool:
        return bool(
            self._call(getattr(self._implementation, "getDelaySubscriptionEvaluationSwitch"))
        )

    def getAdvisoriesUseKnownClassSwitch(self) -> bool:
        return bool(
            self._call(getattr(self._implementation, "getAdvisoriesUseKnownClassSwitch"))
        )

    def getAllowRelaxedDDMSwitch(self) -> bool:
        return bool(
            self._call(getattr(self._implementation, "getAllowRelaxedDDMSwitch"))
        )

    def getNonRegulatedGrantSwitch(self) -> bool:
        return bool(
            self._call(getattr(self._implementation, "getNonRegulatedGrantSwitch"))
        )

    def getTimeFactory(self) -> LogicalTimeFactory:
        java_factory = self._call(self._runtime.logical_time_factory, self._implementation)
        name = str(java_factory.getName())
        if name == "HLAinteger64Time":
            return _JavaInteger64TimeFactory(self._implementation, self._runtime, java_factory)
        if name == "HLAfloat64Time":
            return _JavaFloat64TimeFactory(self._implementation, self._runtime, java_factory)
        raise RTIinternalError(f"Unsupported logical-time implementation: {name}")

    def enableTimeRegulation(self, lookahead: LogicalTimeInterval) -> None:
        if not isinstance(lookahead, LogicalTimeInterval):
            raise TypeError("lookahead must be LogicalTimeInterval")
        self._call(
            getattr(self._implementation, "enableTimeRegulation"),
            self._call(
                self._runtime.decode_logical_interval,
                self._implementation,
                lookahead.encodedValue,
            ),
        )

    def disableTimeRegulation(self) -> None:
        self._call(getattr(self._implementation, "disableTimeRegulation"))

    def enableTimeConstrained(self) -> None:
        self._call(getattr(self._implementation, "enableTimeConstrained"))

    def disableTimeConstrained(self) -> None:
        self._call(getattr(self._implementation, "disableTimeConstrained"))

    def enableAsynchronousDelivery(self) -> None:
        self._call(getattr(self._implementation, "enableAsynchronousDelivery"))

    def disableAsynchronousDelivery(self) -> None:
        self._call(getattr(self._implementation, "disableAsynchronousDelivery"))

    def modifyLookahead(self, lookahead: LogicalTimeInterval) -> None:
        if not isinstance(lookahead, LogicalTimeInterval):
            raise TypeError("lookahead must be LogicalTimeInterval")
        self._call(
            getattr(self._implementation, "modifyLookahead"),
            self._runtime.decode_logical_interval(self._implementation, lookahead.encodedValue),
        )

    def queryLookahead(self) -> LogicalTimeInterval:
        raw = self._call(getattr(self._implementation, "queryLookahead"))
        numeric = _java_interval_value(raw)
        implementation = _java_time_implementation_name(raw, numeric)
        interval_type = (
            HLAinteger64Interval if implementation == "HLAinteger64Time" else HLAfloat64Interval
        )
        return interval_type(
            self._runtime.handle_bytes(raw),
            implementation,
            bool(raw.isZero()),
            bool(raw.isEpsilon()),
            numeric,
            str(raw.toString()),
        )

    def timeAdvanceRequest(self, time: LogicalTime) -> None:
        if not isinstance(time, LogicalTime):
            raise TypeError("time must be LogicalTime")
        self._call(
            getattr(self._implementation, "timeAdvanceRequest"),
            self._call(
                self._runtime.decode_logical_time,
                self._implementation,
                time.encodedValue,
            ),
        )

    def timeAdvanceRequestAvailable(self, time: LogicalTime) -> None:
        if not isinstance(time, LogicalTime):
            raise TypeError("time must be LogicalTime")
        self._call(
            getattr(self._implementation, "timeAdvanceRequestAvailable"),
            self._call(
                self._runtime.decode_logical_time,
                self._implementation,
                time.encodedValue,
            ),
        )

    def nextMessageRequest(self, time: LogicalTime) -> None:
        if not isinstance(time, LogicalTime):
            raise TypeError("time must be LogicalTime")
        self._call(
            getattr(self._implementation, "nextMessageRequest"),
            self._call(
                self._runtime.decode_logical_time,
                self._implementation,
                time.encodedValue,
            ),
        )

    def nextMessageRequestAvailable(self, time: LogicalTime) -> None:
        if not isinstance(time, LogicalTime):
            raise TypeError("time must be LogicalTime")
        self._call(
            getattr(self._implementation, "nextMessageRequestAvailable"),
            self._call(
                self._runtime.decode_logical_time,
                self._implementation,
                time.encodedValue,
            ),
        )

    def flushQueueRequest(self, time: LogicalTime) -> None:
        if not isinstance(time, LogicalTime):
            raise TypeError("time must be LogicalTime")
        self._call(
            getattr(self._implementation, "flushQueueRequest"),
            self._call(
                self._runtime.decode_logical_time,
                self._implementation,
                time.encodedValue,
            ),
        )

    def queryLogicalTime(self) -> LogicalTime:
        raw = self._call(getattr(self._implementation, "queryLogicalTime"))
        numeric = _java_time_value(raw)
        implementation = _java_time_implementation_name(raw, numeric)
        value_type = HLAinteger64Time if implementation == "HLAinteger64Time" else HLAfloat64Time
        return value_type(
            self._runtime.handle_bytes(raw),
            implementation,
            bool(raw.isInitial()),
            bool(raw.isFinal()),
            numeric,
            str(raw.toString()),
        )

    def _timeQuery(self, method_name: str) -> TimeQueryResult:
        raw = self._call(getattr(self._implementation, method_name))
        valid = bool(getattr(raw, "timeIsValid"))
        if not valid:
            return TimeQueryResult(False, None)
        value = raw.time
        numeric = _java_time_value(value)
        implementation = _java_time_implementation_name(value, numeric)
        value_type = HLAinteger64Time if implementation == "HLAinteger64Time" else HLAfloat64Time
        return TimeQueryResult(
            True,
            value_type(
                self._runtime.handle_bytes(value),
                implementation,
                bool(value.isInitial()),
                bool(value.isFinal()),
                numeric,
                str(value.toString()),
            ),
        )

    def queryGALT(self) -> TimeQueryResult:
        return self._timeQuery("queryGALT")

    def queryLITS(self) -> TimeQueryResult:
        return self._timeQuery("queryLITS")

    def _decode_handle(
        self, factory_method_name: str, encoded_value: bytes
    ) -> object:
        return self._call(
            self._runtime.decode_handle,
            self._implementation,
            factory_method_name,
            encoded_value,
        )

    def _remember_retraction(self, raw_handle: object) -> MessageRetractionHandle:
        valid = getattr(raw_handle, "retractionHandleIsValid", True)
        if not bool(valid):
            raise RTIinternalError("Java RTI did not issue a valid message-retraction handle")
        raw_handle = getattr(raw_handle, "handle", raw_handle)
        if raw_handle is None:
            raise RTIinternalError("Java RTI returned no message-retraction handle")
        encoded = self._runtime.handle_bytes(raw_handle)
        self._message_retractions[encoded] = raw_handle
        return MessageRetractionHandle(encoded)

    def getObjectClassHandle(self, objectClassName: str) -> ObjectClassHandle:
        raw_handle = self._call(
            getattr(self._implementation, "getObjectClassHandle"), str(objectClassName)
        )
        return ObjectClassHandle(self._runtime.handle_bytes(raw_handle))

    def getFederateHandle(self, federateName: str) -> FederateHandle:
        raw_handle = self._call(
            getattr(self._implementation, "getFederateHandle"), str(federateName)
        )
        return FederateHandle(self._runtime.federate_handle_bytes(raw_handle))

    def getFederateName(self, federate: FederateHandle) -> str:
        return str(
            self._call(
                getattr(self._implementation, "getFederateName"),
                self._decode_handle(
                    "getFederateHandleFactory",
                    _encoded_handle(federate, FederateHandle),
                ),
            )
        )

    def getObjectClassName(self, objectClass: ObjectClassHandle) -> str:
        return str(
            self._call(
                getattr(self._implementation, "getObjectClassName"),
                self._decode_handle(
                    "getObjectClassHandleFactory",
                    _encoded_handle(objectClass, ObjectClassHandle),
                ),
            )
        )

    def getKnownObjectClassHandle(
        self, objectInstance: ObjectInstanceHandle
    ) -> ObjectClassHandle:
        raw_handle = self._call(
            getattr(self._implementation, "getKnownObjectClassHandle"),
            self._decode_handle(
                "getObjectInstanceHandleFactory",
                _encoded_handle(objectInstance, ObjectInstanceHandle),
            ),
        )
        return ObjectClassHandle(self._runtime.handle_bytes(raw_handle))

    def getAttributeHandle(
        self, objectClass: ObjectClassHandle, attributeName: str
    ) -> AttributeHandle:
        raw_handle = self._call(
            getattr(self._implementation, "getAttributeHandle"),
            self._decode_handle(
                "getObjectClassHandleFactory",
                _encoded_handle(objectClass, ObjectClassHandle),
            ),
            str(attributeName),
        )
        return AttributeHandle(self._runtime.handle_bytes(raw_handle))

    def getAttributeName(
        self, objectClass: ObjectClassHandle, attribute: AttributeHandle
    ) -> str:
        return str(
            self._call(
                getattr(self._implementation, "getAttributeName"),
                self._decode_handle(
                    "getObjectClassHandleFactory",
                    _encoded_handle(objectClass, ObjectClassHandle),
                ),
                self._decode_handle(
                    "getAttributeHandleFactory",
                    _encoded_handle(attribute, AttributeHandle),
                ),
            )
        )

    def getUpdateRateValue(self, updateRateDesignator: str) -> float:
        return float(
            self._call(
                getattr(self._implementation, "getUpdateRateValue"),
                str(updateRateDesignator),
            )
        )

    def getUpdateRateValueForAttribute(
        self, objectInstance: ObjectInstanceHandle, attribute: AttributeHandle
    ) -> float:
        return float(
            self._call(
                getattr(self._implementation, "getUpdateRateValueForAttribute"),
                self._decode_handle(
                    "getObjectInstanceHandleFactory",
                    _encoded_handle(objectInstance, ObjectInstanceHandle),
                ),
                self._decode_handle(
                    "getAttributeHandleFactory",
                    _encoded_handle(attribute, AttributeHandle),
                ),
            )
        )

    def getInteractionClassHandle(self, interactionClassName: str) -> InteractionClassHandle:
        raw_handle = self._call(
            getattr(self._implementation, "getInteractionClassHandle"), str(interactionClassName)
        )
        return InteractionClassHandle(self._runtime.handle_bytes(raw_handle))

    def getInteractionClassName(self, interactionClass: InteractionClassHandle) -> str:
        return str(
            self._call(
                getattr(self._implementation, "getInteractionClassName"),
                self._decode_handle(
                    "getInteractionClassHandleFactory",
                    _encoded_handle(interactionClass, InteractionClassHandle),
                ),
            )
        )

    def getParameterHandle(
        self, interactionClass: InteractionClassHandle, parameterName: str
    ) -> ParameterHandle:
        raw_handle = self._call(
            getattr(self._implementation, "getParameterHandle"),
            self._decode_handle(
                "getInteractionClassHandleFactory",
                _encoded_handle(interactionClass, InteractionClassHandle),
            ),
            str(parameterName),
        )
        return ParameterHandle(self._runtime.handle_bytes(raw_handle))

    def getParameterName(
        self, interactionClass: InteractionClassHandle, parameter: ParameterHandle
    ) -> str:
        return str(
            self._call(
                getattr(self._implementation, "getParameterName"),
                self._decode_handle(
                    "getInteractionClassHandleFactory",
                    _encoded_handle(interactionClass, InteractionClassHandle),
                ),
                self._decode_handle(
                    "getParameterHandleFactory",
                    _encoded_handle(parameter, ParameterHandle),
                ),
            )
        )

    def getOrderType(self, orderTypeName: str) -> OrderType:
        raw_order = self._call(
            getattr(self._implementation, "getOrderType"), str(orderTypeName)
        )
        name = _enum_name(raw_order)
        try:
            return OrderType[name]
        except KeyError as error:
            raise RTIinternalError(f"Java RTI returned an unknown OrderType: {name}") from error

    def getOrderName(self, orderType: OrderType) -> str:
        if not isinstance(orderType, OrderType):
            raise TypeError("orderType must be OrderType")
        return str(
            self._call(
                getattr(self._implementation, "getOrderName"),
                self._runtime.order_type(orderType.name),
            )
        )

    def getTransportationTypeHandle(
        self, transportationTypeName: str
    ) -> TransportationTypeHandle:
        raw_handle = self._call(
            getattr(self._implementation, "getTransportationTypeHandle"),
            str(transportationTypeName),
        )
        return TransportationTypeHandle(self._runtime.handle_bytes(raw_handle))

    def getTransportationTypeName(self, transportationType: TransportationTypeHandle) -> str:
        return str(
            self._call(
                getattr(self._implementation, "getTransportationTypeName"),
                self._decode_handle(
                    "getTransportationTypeHandleFactory",
                    _encoded_handle(transportationType, TransportationTypeHandle),
                ),
            )
        )

    def getDimensionHandle(self, dimensionName: str) -> DimensionHandle:
        raw_handle = self._call(
            getattr(self._implementation, "getDimensionHandle"), str(dimensionName)
        )
        return DimensionHandle(self._runtime.handle_bytes(raw_handle))

    def getDimensionName(self, dimension: DimensionHandle) -> str:
        return str(
            self._call(
                getattr(self._implementation, "getDimensionName"),
                self._decode_handle(
                    "getDimensionHandleFactory",
                    _encoded_handle(dimension, DimensionHandle),
                ),
            )
        )

    def getAvailableDimensionsForObjectClass(
        self, objectClass: ObjectClassHandle
    ) -> DimensionHandleSet:
        raw_set = self._call(
            getattr(self._implementation, "getAvailableDimensionsForObjectClass"),
            self._decode_handle(
                "getObjectClassHandleFactory",
                _encoded_handle(objectClass, ObjectClassHandle),
            ),
        )
        return DimensionHandleSet(
            DimensionHandle(self._runtime.handle_bytes(value)) for value in raw_set
        )

    def getAvailableDimensionsForInteractionClass(
        self, interactionClass: InteractionClassHandle
    ) -> DimensionHandleSet:
        raw_set = self._call(
            getattr(self._implementation, "getAvailableDimensionsForInteractionClass"),
            self._decode_handle(
                "getInteractionClassHandleFactory",
                _encoded_handle(interactionClass, InteractionClassHandle),
            ),
        )
        return DimensionHandleSet(
            DimensionHandle(self._runtime.handle_bytes(value)) for value in raw_set
        )

    def getDimensionUpperBound(self, dimension: DimensionHandle) -> int:
        return int(
            self._call(
                getattr(self._implementation, "getDimensionUpperBound"),
                self._decode_handle(
                    "getDimensionHandleFactory",
                    _encoded_handle(dimension, DimensionHandle),
                ),
            )
        )

    def normalizeServiceGroup(self, serviceGroup: ServiceGroup) -> int:
        if not isinstance(serviceGroup, ServiceGroup):
            raise TypeError("serviceGroup must be ServiceGroup")
        return int(
            self._call(
                getattr(self._implementation, "normalizeServiceGroup"),
                self._runtime.service_group(serviceGroup.name),
            )
        )

    def normalizeFederateHandle(self, federate: FederateHandle) -> int:
        return int(
            self._call(
                getattr(self._implementation, "normalizeFederateHandle"),
                self._decode_handle(
                    "getFederateHandleFactory", _encoded_handle(federate, FederateHandle)
                ),
            )
        )

    def normalizeObjectClassHandle(self, objectClass: ObjectClassHandle) -> int:
        return int(
            self._call(
                getattr(self._implementation, "normalizeObjectClassHandle"),
                self._decode_handle(
                    "getObjectClassHandleFactory", _encoded_handle(objectClass, ObjectClassHandle)
                ),
            )
        )

    def normalizeInteractionClassHandle(
        self, interactionClass: InteractionClassHandle
    ) -> int:
        return int(
            self._call(
                getattr(self._implementation, "normalizeInteractionClassHandle"),
                self._decode_handle(
                    "getInteractionClassHandleFactory",
                    _encoded_handle(interactionClass, InteractionClassHandle),
                ),
            )
        )

    def normalizeObjectInstanceHandle(self, objectInstance: ObjectInstanceHandle) -> int:
        return int(
            self._call(
                getattr(self._implementation, "normalizeObjectInstanceHandle"),
                self._decode_handle(
                    "getObjectInstanceHandleFactory",
                    _encoded_handle(objectInstance, ObjectInstanceHandle),
                ),
            )
        )

    def createFederationExecution(
        self,
        federationName: str,
        fomModule: str | Iterable[str],
        logicalTimeImplementationName: str = "",
    ) -> None:
        modules = (fomModule,) if isinstance(fomModule, str) else tuple(str(module) for module in fomModule)
        self._call(
            getattr(self._implementation, "createFederationExecution"),
            federationName,
            self._runtime.fom_module_urls(tuple(str(module) for module in modules)),
            logicalTimeImplementationName,
        )

    def createFederationExecutionWithMIM(
        self,
        federationName: str,
        fomModules: Iterable[str],
        mimModule: str,
        logicalTimeImplementationName: str = "",
    ) -> None:
        self._call(
            getattr(self._implementation, "createFederationExecutionWithMIM"),
            federationName,
            self._runtime.fom_module_urls(tuple(str(module) for module in fomModules)),
            self._runtime.fom_module_url(str(mimModule)),
            logicalTimeImplementationName,
        )

    def destroyFederationExecution(self, federationName: str) -> None:
        self._call(getattr(self._implementation, "destroyFederationExecution"), federationName)

    def unwrap_java_object(self) -> object:
        """Return the underlying Java object for explicit migration-only use.

        This method is deliberately absent from the public ``RTIambassador``
        contract. Code that calls it is coupled to Java and cannot be swapped
        to the pybind11 provider unchanged.
        """

        return self._implementation

    def _call(self, function: Callable[..., _Result], *args: object) -> _Result:
        try:
            return function(*args)
        except RTIexception:
            raise
        except Exception as error:
            name = self._runtime.exception_name(error)
            if name is None:
                raise RTIinternalError(f"Java RTI call failed: {error}") from error
            name = _simple_java_exception_name(name)
            raise exceptionForName(name, str(error)) from error


class JavaRtiFactory(RtiFactory):
    """A discovered provider that delegates to a selected Java RTI factory."""

    @classmethod
    def from_jar(
        cls,
        jar: str | Path,
        *,
        factory_name: str | None = None,
        dependencies: tuple[str | Path, ...] = (),
        jvm_path: str | None = None,
        jvm_options: tuple[str, ...] = (),
        native_library_path: str | Path | None = None,
        convert_strings: bool = False,
        runtime: JavaRuntime | None = None,
    ) -> "JavaRtiFactory":
        """Load one standard Java RTI through ``RtiFactoryFactory``.

        This is the concise onboarding form for a vendor JAR. It only builds
        the process-level JPype configuration; discovery still occurs through
        the exact standard Java ``RtiFactoryFactory`` and ``ServiceLoader``.
        """

        options = tuple(jvm_options)
        if native_library_path is not None and not any(
            option.startswith("-Djava.library.path=") for option in options
        ):
            options += (f"-Djava.library.path={Path(native_library_path).resolve()}",)
        return cls(
            JavaProviderConfiguration(
                classpath=(str(jar),) + tuple(str(path) for path in dependencies),
                rti_factory_name=factory_name,
                jvm_path=jvm_path,
                jvm_options=options,
                convert_strings=convert_strings,
            ),
            runtime=runtime,
        )

    def __init__(
        self,
        configuration: JavaProviderConfiguration | None = None,
        *,
        runtime: JavaRuntime | None = None,
    ) -> None:
        self._configuration = configuration or JavaProviderConfiguration.from_environment()
        self._runtime = runtime or JPypeJavaRuntime()
        self._implementation: object | None = None

    def getRtiAmbassador(self) -> RTIambassador:
        implementation = self._call(getattr(self._java_factory(), "getRtiAmbassador"))
        return JavaRTIambassador(implementation, self._runtime)

    def getEncoderFactory(self) -> EncoderFactory:
        implementation = self._call(getattr(self._java_factory(), "getEncoderFactory"))
        return JavaEncoderFactory(implementation, self._runtime)

    def rtiName(self) -> str:
        return str(self._call(getattr(self._java_factory(), "rtiName")))

    def rtiVersion(self) -> str:
        return str(self._call(getattr(self._java_factory(), "rtiVersion")))

    def unwrap_java_factory(self) -> object:
        """Return the underlying Java ``RtiFactory`` for explicit migration use."""

        return self._java_factory()

    def unwrap_java_encoder_factory(self) -> object:
        """Return the Java encoder only as an explicit non-portable escape hatch."""

        return self._call(getattr(self._java_factory(), "getEncoderFactory"))

    def _java_factory(self) -> object:
        if self._implementation is None:
            self._implementation = self._runtime.get_rti_factory(self._configuration)
        return self._implementation

    def _call(self, function: Callable[..., _Result], *args: object) -> _Result:
        try:
            return function(*args)
        except RTIexception:
            raise
        except Exception as error:
            name = self._runtime.exception_name(error)
            if name is None:
                raise RTIinternalError(f"Java RTI call failed: {error}") from error
            name = _simple_java_exception_name(name)
            raise exceptionForName(name, str(error)) from error
