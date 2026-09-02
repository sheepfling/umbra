"""Generic JPype adapter over the exact IEEE 1516.1-2010 Java surface."""

from __future__ import annotations

from abc import update_abstractmethods
from collections.abc import Sequence
from dataclasses import dataclass
from pathlib import Path
from enum import Enum
from typing import Any, Callable

from hla.rti1516e import (
    AttributeHandle,
    AttributeHandleSet,
    AttributeHandleValueMap,
    AttributeSetRegionSetPairList,
    MutableAttributeHandleSet,
    MutableAttributeHandleValueMap,
    CallbackModel,
    DimensionHandle,
    DimensionHandleSet,
    MutableDimensionHandleSet,
    FederateAmbassador,
    FederateHandle,
    FederateHandleSet,
    MutableFederateHandleSet,
    InteractionClassHandle,
    LogicalTime,
    LogicalTimeInterval,
    MessageRetractionHandle,
    ObjectClassHandle,
    ObjectInstanceHandle,
    ParameterHandle,
    ParameterHandleValueMap,
    MutableParameterHandleValueMap,
    RegionHandle,
    RegionHandleSet,
    RangeBounds,
    MutableRegionHandleSet,
    RTIAMBASSADOR_METHODS,
    RTIAMBASSADOR_PARAMETER_TYPES,
    RTIAMBASSADOR_RETURN_TYPES,
    RTIambassador,
    RtiFactory,
    AttributeHandleFactory,
    AttributeHandleSetFactory,
    AttributeHandleValueMapFactory,
    AttributeSetRegionSetPairListFactory,
    DimensionHandleFactory,
    DimensionHandleSetFactory,
    FederateHandleFactory,
    FederateHandleSetFactory,
    InteractionClassHandleFactory,
    ObjectClassHandleFactory,
    ObjectInstanceHandleFactory,
    ParameterHandleFactory,
    ParameterHandleValueMapFactory,
    RegionHandleSetFactory,
    TransportationTypeHandleFactory,
    ResignAction,
    TransportationTypeHandle,
)
from hla.rti1516e.byte_types import BytesLike
from hla.rti1516e.exceptions import RTIexception, RTIinternalError, exceptionForName

from .config import Java2010ProviderConfiguration
from .runtime import Java2010CallbackBinding, JPype2010Runtime
from .encoding import JavaEncoderFactory
from .time import Java2010TimeFactory


def _simple_name(value: str) -> str:
    return value.rsplit(".", 1)[-1].rsplit("$", 1)[-1]


def _overload_score(value: object, expected_type: str) -> int:
    """Score a Python argument against one generated Java parameter type.

    Arity alone is insufficient for the 2010 API: for example,
    ``createFederationExecution`` has same-arity ``URL``/``URL[]`` and
    ``URL``/``String`` alternatives.  A score keeps declaration order as the
    deterministic tie-breaker while selecting the most specific standard
    shape before JPype performs its final conversion.
    """

    if expected_type == "String":
        return 8 if isinstance(value, str) else 0
    if expected_type == "URL[]":
        return 9 if isinstance(value, Sequence) and not isinstance(value, (str, bytes, bytearray)) else 0
    if expected_type == "URL":
        return 6 if isinstance(value, (str, Path)) else 0
    if expected_type == "byte[]":
        return 9 if isinstance(value, (bytes, bytearray, memoryview)) else 0
    if expected_type == "Set<String>":
        return 9 if isinstance(value, (set, frozenset)) else 5 if isinstance(value, Sequence) and not isinstance(value, (str, bytes, bytearray)) else 0
    if expected_type == "boolean":
        return 9 if isinstance(value, bool) else 0
    if expected_type in {"double", "long"}:
        return 7 if isinstance(value, (int, float)) and not isinstance(value, bool) else 0
    if expected_type in {"CallbackModel", "OrderType", "ResignAction", "ServiceGroup", "SynchronizationPointFailureReason", "SaveFailureReason", "RestoreFailureReason"}:
        return 9 if isinstance(value, Enum) else 0
    if expected_type == "FederateAmbassador":
        return 9 if isinstance(value, FederateAmbassador) else 0
    if expected_type == "AttributeSetRegionSetPairList":
        return 9 if isinstance(value, AttributeSetRegionSetPairList) else 0
    if expected_type == "RangeBounds":
        return 9 if isinstance(value, RangeBounds) else 0
    if expected_type == "LogicalTime":
        return 9 if isinstance(value, LogicalTime) else 0
    if expected_type == "LogicalTimeInterval":
        return 9 if isinstance(value, LogicalTimeInterval) else 0
    expected_class = {
        "AttributeHandle": AttributeHandle,
        "AttributeHandleSet": AttributeHandleSet,
        "AttributeHandleValueMap": AttributeHandleValueMap,
        "DimensionHandle": DimensionHandle,
        "DimensionHandleSet": DimensionHandleSet,
        "FederateHandle": FederateHandle,
        "FederateHandleSet": FederateHandleSet,
        "InteractionClassHandle": InteractionClassHandle,
        "MessageRetractionHandle": MessageRetractionHandle,
        "ObjectClassHandle": ObjectClassHandle,
        "ObjectInstanceHandle": ObjectInstanceHandle,
        "ParameterHandle": ParameterHandle,
        "ParameterHandleValueMap": ParameterHandleValueMap,
        "RegionHandle": RegionHandle,
        "RegionHandleSet": RegionHandleSet,
        "TransportationTypeHandle": TransportationTypeHandle,
    }.get(expected_type)
    if expected_class is not None:
        return 9 if isinstance(value, expected_class) else 0
    # A generated type not yet given a richer Python classifier should not be
    # rejected here; declaration order remains a safe fallback.
    return 0


_HANDLE_FACTORIES: dict[type[object], str] = {
    FederateHandle: "getFederateHandleFactory",
    ObjectClassHandle: "getObjectClassHandleFactory",
    ObjectInstanceHandle: "getObjectInstanceHandleFactory",
    AttributeHandle: "getAttributeHandleFactory",
    InteractionClassHandle: "getInteractionClassHandleFactory",
    ParameterHandle: "getParameterHandleFactory",
    TransportationTypeHandle: "getTransportationTypeHandleFactory",
    DimensionHandle: "getDimensionHandleFactory",
}

_RETURN_HANDLES: dict[str, type[object]] = {
    "getFederateHandle": FederateHandle,
    "getObjectClassHandle": ObjectClassHandle,
    "getKnownObjectClassHandle": ObjectClassHandle,
    "getObjectInstanceHandle": ObjectInstanceHandle,
    "getAttributeHandle": AttributeHandle,
    "getInteractionClassHandle": InteractionClassHandle,
    "getParameterHandle": ParameterHandle,
    "getTransportationTypeHandle": TransportationTypeHandle,
    "getDimensionHandle": DimensionHandle,
    "createRegion": RegionHandle,
    "registerObjectInstance": ObjectInstanceHandle,
    "registerObjectInstanceWithRegions": ObjectInstanceHandle,
}

_HANDLE_SET_FACTORIES: dict[type[object], str] = {
    AttributeHandleSet: "getAttributeHandleFactory",
    MutableAttributeHandleSet: "getAttributeHandleFactory",
    DimensionHandleSet: "getDimensionHandleFactory",
    MutableDimensionHandleSet: "getDimensionHandleFactory",
    FederateHandleSet: "getFederateHandleFactory",
    MutableFederateHandleSet: "getFederateHandleFactory",
    RegionHandleSet: "",
    MutableRegionHandleSet: "",
}

_HANDLE_SET_FACTORY_NAMES: dict[str, str] = {
    "AttributeHandleSet": "getAttributeHandleSetFactory",
    "DimensionHandleSet": "getDimensionHandleSetFactory",
    "FederateHandleSet": "getFederateHandleSetFactory",
    "RegionHandleSet": "getRegionHandleSetFactory",
}

_HANDLE_SET_HANDLE_FACTORIES: dict[str, str] = {
    "AttributeHandleSet": "getAttributeHandleFactory",
    "DimensionHandleSet": "getDimensionHandleFactory",
    "FederateHandleSet": "getFederateHandleFactory",
    "RegionHandleSet": "",
}

_HANDLE_VALUE_MAP_FACTORIES: dict[type[object], str] = {
    AttributeHandleValueMap: "getAttributeHandleFactory",
    MutableAttributeHandleValueMap: "getAttributeHandleFactory",
    ParameterHandleValueMap: "getParameterHandleFactory",
    MutableParameterHandleValueMap: "getParameterHandleFactory",
}

_HANDLE_VALUE_MAP_FACTORY_NAMES: dict[str, str] = {
    "AttributeHandleValueMap": "getAttributeHandleValueMapFactory",
    "ParameterHandleValueMap": "getParameterHandleValueMapFactory",
}

_HANDLE_VALUE_MAP_HANDLE_FACTORIES: dict[str, str] = {
    "AttributeHandleValueMap": "getAttributeHandleFactory",
    "ParameterHandleValueMap": "getParameterHandleFactory",
}

_HANDLE_TYPE_NAMES: dict[str, tuple[type[object], str]] = {
    "FederateHandle": (FederateHandle, "getFederateHandleFactory"),
    "ObjectClassHandle": (ObjectClassHandle, "getObjectClassHandleFactory"),
    "ObjectInstanceHandle": (ObjectInstanceHandle, "getObjectInstanceHandleFactory"),
    "AttributeHandle": (AttributeHandle, "getAttributeHandleFactory"),
    "InteractionClassHandle": (InteractionClassHandle, "getInteractionClassHandleFactory"),
    "ParameterHandle": (ParameterHandle, "getParameterHandleFactory"),
    "TransportationTypeHandle": (TransportationTypeHandle, "getTransportationTypeHandleFactory"),
    "DimensionHandle": (DimensionHandle, "getDimensionHandleFactory"),
    # IEEE 1516e has no RegionHandleFactory.  Region handles are opaque
    # values issued by createRegion and are retained in the ambassador cache.
    "RegionHandle": (RegionHandle, ""),
    # Message-retraction handles are likewise issued by update/send/delete
    # services rather than a public handle factory.  Keep the raw Java carrier
    # in the ambassador cache so a Python handle can be passed to ``retract``
    # (or another standard service) without inventing a second encoding API.
    "MessageRetractionHandle": (MessageRetractionHandle, ""),
}


class _JavaHandleFactory:
    def __init__(self, owner: "Java2010RTIambassador", method_name: str, handle_type: type[object]) -> None:
        self._owner = owner
        self._method_name = method_name
        self._handle_type = handle_type

    def decode(self, buffer: BytesLike, offset: int = 0) -> object:
        raw = self._owner._runtime.decode_handle(
            self._owner._implementation,
            self._method_name,
            bytes(buffer)[offset:],
        )
        return self._owner._remember_raw_handle(self._handle_type, raw)


class _JavaTransportationTypeHandleFactory(_JavaHandleFactory, TransportationTypeHandleFactory):
    def getHLAdefaultReliable(self) -> TransportationTypeHandle:
        raw_factory = self._owner._call(getattr(self._owner._implementation, self._method_name))
        raw = self._owner._call(getattr(raw_factory, "getHLAdefaultReliable"))
        return self._owner._remember_raw_handle(TransportationTypeHandle, raw)

    def getHLAdefaultBestEffort(self) -> TransportationTypeHandle:
        raw_factory = self._owner._call(getattr(self._owner._implementation, self._method_name))
        raw = self._owner._call(getattr(raw_factory, "getHLAdefaultBestEffort"))
        return self._owner._remember_raw_handle(TransportationTypeHandle, raw)


class _JavaAttributeHandleFactory(_JavaHandleFactory, AttributeHandleFactory):
    pass


class _JavaDimensionHandleFactory(_JavaHandleFactory, DimensionHandleFactory):
    pass


class _JavaFederateHandleFactory(_JavaHandleFactory, FederateHandleFactory):
    pass


class _JavaInteractionClassHandleFactory(_JavaHandleFactory, InteractionClassHandleFactory):
    pass


class _JavaObjectClassHandleFactory(_JavaHandleFactory, ObjectClassHandleFactory):
    pass


class _JavaObjectInstanceHandleFactory(_JavaHandleFactory, ObjectInstanceHandleFactory):
    pass


class _JavaParameterHandleFactory(_JavaHandleFactory, ParameterHandleFactory):
    pass


class _JavaTransportationTypeFactory(_JavaTransportationTypeHandleFactory):
    pass


class _JavaSetFactory:
    def __init__(self, owner: "Java2010RTIambassador", method_name: str, builder_type: type[object]) -> None:
        self._owner = owner
        self._method_name = method_name
        self._builder_type = builder_type

    def create(self) -> object:
        factory = self._owner._call(getattr(self._owner._implementation, self._method_name))
        self._owner._call(getattr(factory, "create"))
        return self._builder_type()


class _JavaAttributeHandleSetFactory(_JavaSetFactory, AttributeHandleSetFactory):
    pass


class _JavaDimensionHandleSetFactory(_JavaSetFactory, DimensionHandleSetFactory):
    pass


class _JavaFederateHandleSetFactory(_JavaSetFactory, FederateHandleSetFactory):
    pass


class _JavaRegionHandleSetFactory(_JavaSetFactory, RegionHandleSetFactory):
    pass


class _JavaMapFactory:
    def __init__(self, owner: "Java2010RTIambassador", method_name: str, builder_type: type[object]) -> None:
        self._owner = owner
        self._method_name = method_name
        self._builder_type = builder_type

    def create(self, capacity: int = 0) -> object:
        factory = self._owner._call(getattr(self._owner._implementation, self._method_name))
        self._owner._call(getattr(factory, "create"), int(capacity))
        return self._builder_type()


class _JavaAttributeHandleValueMapFactory(_JavaMapFactory, AttributeHandleValueMapFactory):
    pass


class _JavaParameterHandleValueMapFactory(_JavaMapFactory, ParameterHandleValueMapFactory):
    pass


class _JavaAttributeSetRegionSetPairListFactory(AttributeSetRegionSetPairListFactory):
    def __init__(self, owner: "Java2010RTIambassador") -> None:
        self._owner = owner

    def create(self, capacity: int = 0) -> object:
        factory = self._owner._call(
            getattr(self._owner._implementation, "getAttributeSetRegionSetPairListFactory")
        )
        self._owner._call(getattr(factory, "create"), int(capacity))
        return AttributeSetRegionSetPairList()


class Java2010RTIambassador(RTIambassador):
    """Forwarding ambassador with a standard Java callback proxy.

    The adapter deliberately keeps the method names from the Java interface.
    Generated parameter metadata selects the standard overload arity and
    drives boundary conversion; JPype still performs Java's final overload
    dispatch.
    """

    def __init__(self, implementation: object, runtime: JPype2010Runtime) -> None:
        self._implementation = implementation
        self._runtime = runtime
        self._callback_binding: Java2010CallbackBinding | None = None
        self._raw_handles: dict[tuple[type[object], bytes], object] = {}

    def _remember_raw_handle(self, handle_type: type[object], raw: object) -> object:
        encoded = self._runtime.handle_bytes(raw)
        self._raw_handles[(handle_type, encoded)] = raw
        return handle_type(encoded)  # type: ignore[call-arg]

    def _raw_handle(self, handle_type: type[object], value: object) -> object:
        encoded = bytes(value.encodedValue)  # type: ignore[attr-defined]
        raw = self._raw_handles.get((handle_type, encoded))
        if raw is None:
            raise RTIinternalError(
                f"Java 1516e provider has no live {handle_type.__name__} for encoded value"
            )
        return raw

    def _call(self, function: Callable[..., Any], *args: object) -> Any:
        try:
            return function(*args)
        except RTIexception:
            raise
        except Exception as error:
            name = self._runtime.exception_name(error)
            if name is None:
                raise RTIinternalError(f"Java 1516e RTI call failed: {error}") from error
            raise exceptionForName(_simple_name(name), str(error), error) from error

    def _convert_argument(self, value: object, expected_type: str | None = None) -> object:
        if expected_type:
            if expected_type == "byte[]":
                return self._runtime.byte_array(value)  # type: ignore[arg-type]
            if expected_type == "URL[]":
                return self._runtime.java_urls(value)
            if expected_type == "URL":
                return self._runtime.java_url(value)
            if expected_type == "Set<String>":
                return self._runtime.string_set(value)
            if expected_type in _HANDLE_SET_FACTORY_NAMES:
                if expected_type == "RegionHandleSet":
                    raw_values = (
                        self._raw_handle(RegionHandle, item)
                        for item in value  # type: ignore[union-attr]
                    )
                    region_builder = getattr(self._runtime, "region_handle_set", None)
                    if callable(region_builder):
                        return region_builder(self._implementation, raw_values)
                    return self._runtime.raw_handle_set(raw_values)
                return self._runtime.handle_set(
                    self._implementation,
                    _HANDLE_SET_HANDLE_FACTORIES[expected_type],
                    value,
                    _HANDLE_SET_FACTORY_NAMES[expected_type],
                )
            if expected_type in _HANDLE_VALUE_MAP_FACTORY_NAMES:
                return self._runtime.handle_value_map(
                    self._implementation,
                    _HANDLE_VALUE_MAP_HANDLE_FACTORIES[expected_type],
                    value,
                    _HANDLE_VALUE_MAP_FACTORY_NAMES[expected_type],
                )
            if expected_type == "AttributeSetRegionSetPairList":
                return self._runtime.attribute_region_pair_list(
                    self._implementation, value, self._raw_handle
                )
            if expected_type == "RangeBounds":
                return self._runtime.range_bounds(value)
            if expected_type == "LogicalTime":
                encoded = getattr(value, "encodedValue", None)
                if encoded is None:
                    to_byte_array = getattr(value, "toByteArray", None)
                    encoded = (
                        to_byte_array()
                        if callable(to_byte_array)
                        else self._runtime.handle_bytes(value)
                    )
                return self._runtime.decode_logical_time(
                    self._implementation, encoded
                )
            if expected_type == "LogicalTimeInterval":
                encoded = getattr(value, "encodedValue", None)
                if encoded is None:
                    to_byte_array = getattr(value, "toByteArray", None)
                    encoded = (
                        to_byte_array()
                        if callable(to_byte_array)
                        else self._runtime.handle_bytes(value)
                    )
                return self._runtime.decode_logical_interval(
                    self._implementation, encoded
                )
            if expected_type in {
                "CallbackModel", "OrderType", "ResignAction", "ServiceGroup",
                "SynchronizationPointFailureReason", "SaveFailureReason", "RestoreFailureReason",
            } and isinstance(value, Enum):
                return self._runtime.java_enum(expected_type, value.name)
            handle_info = _HANDLE_TYPE_NAMES.get(expected_type)
            if handle_info is not None and isinstance(value, handle_info[0]):
                if not handle_info[1]:
                    return self._raw_handle(handle_info[0], value)
                return self._runtime.decode_handle(
                    self._implementation, handle_info[1], value.encodedValue  # type: ignore[attr-defined]
                )
        if isinstance(value, (bytes, bytearray, memoryview)):
            return self._runtime.byte_array(value)
        for handle_type, factory_name in _HANDLE_FACTORIES.items():
            if isinstance(value, handle_type):
                return self._runtime.decode_handle(self._implementation, factory_name, value.encodedValue)  # type: ignore[attr-defined]
        if isinstance(value, RegionHandle):
            return self._raw_handle(RegionHandle, value)
        if isinstance(value, CallbackModel):
            return self._runtime.callback_model(value)
        if isinstance(value, (ResignAction,)):
            return self._runtime.java_enum("ResignAction", value.name)
        if isinstance(value, Enum):
            return self._runtime.java_enum(type(value).__name__, value.name)
        for set_type, factory_name in _HANDLE_SET_FACTORIES.items():
            if isinstance(value, set_type):
                if not factory_name:
                    raw_values = (
                        self._raw_handle(RegionHandle, item)
                        for item in value  # type: ignore[union-attr]
                    )
                    region_builder = getattr(self._runtime, "region_handle_set", None)
                    if callable(region_builder):
                        return region_builder(self._implementation, raw_values)
                    return self._runtime.raw_handle_set(raw_values)
                return self._runtime.handle_set(self._implementation, factory_name, value)
        for map_type, factory_name in _HANDLE_VALUE_MAP_FACTORIES.items():
            if isinstance(value, map_type):
                return self._runtime.handle_value_map(self._implementation, factory_name, value)
        return value

    def _invoke(self, name: str, *args: object, **kwargs: object) -> Any:
        if kwargs:
            raise TypeError(f"Java 1516e method {name} does not accept Python keyword arguments")
        method = getattr(self._implementation, name)
        overloads = RTIAMBASSADOR_PARAMETER_TYPES.get(name, ())
        candidates = [
            (index, candidate)
            for index, candidate in enumerate(overloads)
            if len(candidate) == len(args)
        ]
        overload_index = None
        if candidates:
            overload_index = max(
                candidates,
                key=lambda item: sum(
                    _overload_score(value, expected)
                    for value, expected in zip(args, item[1])
                ),
            )[0]
        expected = overloads[overload_index] if overload_index is not None else None
        converted = (
            self._convert_argument(value, expected[index] if expected is not None else None)
            for index, value in enumerate(args)
        )
        result = self._call(method, *converted)
        result_type = _RETURN_HANDLES.get(name)
        if result_type is not None and result is not None:
            return self._remember_raw_handle(result_type, result)
        return_types = RTIAMBASSADOR_RETURN_TYPES.get(name, ())
        if overload_index is not None and overload_index < len(return_types):
            expected_return = return_types[overload_index]
            if expected_return != "void" and result is not None:
                # JPype may preserve primitive Java wrappers (notably under
                # ``convertStrings=False``).  The provider-neutral contract
                # exposes Python primitives, so normalize these scalar return
                # carriers before the richer record/handle conversion path.
                if expected_return == "boolean":
                    return bool(result)
                if expected_return == "double":
                    return float(result)
                if expected_return == "long":
                    return int(result)
                if expected_return in {"LogicalTime", "LogicalTimeInterval"}:
                    return self.getTimeFactory().from_java_value(result, expected_return)
                if expected_return in {"TimeQueryReturn"}:
                    return self.getTimeFactory().from_java_query_return(result)
                converted = self._runtime.from_java_value(result, expected_return)
                if expected_return == "MessageRetractionReturn":
                    raw_handle = getattr(result, "handle", None)
                    valid = getattr(result, "retractionHandleIsValid", False)
                    if raw_handle is not None and bool(valid):
                        # A retraction handle has no public Java factory.  A
                        # provider-issued return value is the authoritative
                        # raw carrier that must remain available when the
                        # Python caller later invokes ``retract``.
                        self._remember_raw_handle(
                            MessageRetractionHandle, raw_handle
                        )
                return converted
        return result

    def connect(
        self,
        federateAmbassador: FederateAmbassador,
        callbackModel: CallbackModel,
        localSettingsDesignator: str = "",
    ) -> None:
        if not isinstance(federateAmbassador, FederateAmbassador):
            raise TypeError("federateAmbassador must implement hla.rti1516e.FederateAmbassador")
        if not isinstance(callbackModel, CallbackModel):
            raise TypeError("callbackModel must be hla.rti1516e.CallbackModel")
        binding = self._runtime.bind_federate_ambassador(federateAmbassador)
        args: list[object] = [binding.proxy, self._runtime.callback_model(callbackModel)]
        if localSettingsDesignator:
            args.append(str(localSettingsDesignator))
        self._call(getattr(self._implementation, "connect"), *args)
        self._callback_binding = binding

    def disconnect(self) -> None:
        self._call(getattr(self._implementation, "disconnect"))
        self._callback_binding = None

    def getHLAversion(self) -> str:
        return str(self._call(getattr(self._implementation, "getHLAversion")))

    def unwrap_java_object(self) -> object:
        """Return the raw standard Java ambassador for migration code."""

        return self._implementation

    def getTimeFactory(self) -> Java2010TimeFactory:
        raw = self._call(getattr(self._implementation, "getTimeFactory"))
        return Java2010TimeFactory(self._implementation, self._runtime, raw)

    def getFederateHandleFactory(self) -> FederateHandleFactory:
        return _JavaFederateHandleFactory(self, "getFederateHandleFactory", FederateHandle)

    def getObjectClassHandleFactory(self) -> ObjectClassHandleFactory:
        return _JavaObjectClassHandleFactory(self, "getObjectClassHandleFactory", ObjectClassHandle)

    def getObjectInstanceHandleFactory(self) -> ObjectInstanceHandleFactory:
        return _JavaObjectInstanceHandleFactory(
            self, "getObjectInstanceHandleFactory", ObjectInstanceHandle
        )

    def getAttributeHandleFactory(self) -> AttributeHandleFactory:
        return _JavaAttributeHandleFactory(self, "getAttributeHandleFactory", AttributeHandle)

    def getInteractionClassHandleFactory(self) -> InteractionClassHandleFactory:
        return _JavaInteractionClassHandleFactory(
            self, "getInteractionClassHandleFactory", InteractionClassHandle
        )

    def getParameterHandleFactory(self) -> ParameterHandleFactory:
        return _JavaParameterHandleFactory(self, "getParameterHandleFactory", ParameterHandle)

    def getTransportationTypeHandleFactory(self) -> TransportationTypeHandleFactory:
        return _JavaTransportationTypeFactory(
            self, "getTransportationTypeHandleFactory", TransportationTypeHandle
        )

    def getDimensionHandleFactory(self) -> DimensionHandleFactory:
        return _JavaDimensionHandleFactory(self, "getDimensionHandleFactory", DimensionHandle)

    def getAttributeHandleSetFactory(self) -> AttributeHandleSetFactory:
        return _JavaAttributeHandleSetFactory(self, "getAttributeHandleSetFactory", MutableAttributeHandleSet)

    def getDimensionHandleSetFactory(self) -> DimensionHandleSetFactory:
        return _JavaDimensionHandleSetFactory(self, "getDimensionHandleSetFactory", MutableDimensionHandleSet)

    def getFederateHandleSetFactory(self) -> FederateHandleSetFactory:
        return _JavaFederateHandleSetFactory(self, "getFederateHandleSetFactory", MutableFederateHandleSet)

    def getRegionHandleSetFactory(self) -> RegionHandleSetFactory:
        return _JavaRegionHandleSetFactory(self, "getRegionHandleSetFactory", MutableRegionHandleSet)

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


def _forward(name: str):
    def invoke(self: Java2010RTIambassador, *args: object, **kwargs: object) -> object:
        return self._invoke(name, *args, **kwargs)

    invoke.__name__ = name
    invoke.__qualname__ = f"Java2010RTIambassador.{name}"
    return invoke


for _name in RTIAMBASSADOR_METHODS:
    if _name not in Java2010RTIambassador.__dict__:
        setattr(Java2010RTIambassador, _name, _forward(_name))
update_abstractmethods(Java2010RTIambassador)


@dataclass(frozen=True, slots=True)
class Java2010RtiProbe:
    factory: "Java2010RtiFactory"
    rti_name: str
    rti_version: str
    configuration: Java2010ProviderConfiguration


class Java2010RtiFactory(RtiFactory):
    """Load a standard 2010 Java RTI using ``RtiFactoryFactory``/ServiceRegistry."""

    def __init__(
        self,
        configuration: Java2010ProviderConfiguration | None = None,
        *,
        runtime: JPype2010Runtime | None = None,
    ) -> None:
        self._configuration = configuration or Java2010ProviderConfiguration.from_environment()
        self._runtime = runtime or JPype2010Runtime()
        self._implementation: object | None = None

    @classmethod
    def from_jar(
        cls,
        jar: str | Path,
        *,
        factory_name: str | None = None,
        dependencies: tuple[str | Path, ...] = (),
        jvm_path: str | None = None,
        jvm_options: tuple[str, ...] = (),
        convert_strings: bool = False,
        runtime: JPype2010Runtime | None = None,
    ) -> "Java2010RtiFactory":
        return cls(
            Java2010ProviderConfiguration(
                classpath=(str(jar), *(str(path) for path in dependencies)),
                rti_factory_name=factory_name,
                jvm_path=jvm_path,
                jvm_options=tuple(jvm_options),
                convert_strings=convert_strings,
            ),
            runtime=runtime,
        )

    @classmethod
    def probe_jar(cls, jar: str | Path, **kwargs: object) -> Java2010RtiProbe:
        return cls.from_jar(jar, **kwargs).probe()  # type: ignore[arg-type]

    def _java_factory(self) -> object:
        if self._implementation is None:
            self._implementation = self._runtime.get_rti_factory(self._configuration)
        return self._implementation

    def getRtiAmbassador(self) -> Java2010RTIambassador:
        return Java2010RTIambassador(
            self._call(getattr(self._java_factory(), "getRtiAmbassador")), self._runtime
        )

    def getEncoderFactory(self) -> JavaEncoderFactory:
        implementation = self._call(getattr(self._java_factory(), "getEncoderFactory"))
        return JavaEncoderFactory(implementation, self._runtime)

    def unwrap_java_encoder_factory(self) -> object:
        """Return the raw standard Java encoder for migration escape hatches."""

        return self._call(getattr(self._java_factory(), "getEncoderFactory"))

    def rtiName(self) -> str:
        return str(self._call(getattr(self._java_factory(), "rtiName")))

    def rtiVersion(self) -> str:
        return str(self._call(getattr(self._java_factory(), "rtiVersion")))

    def probe(self) -> Java2010RtiProbe:
        return Java2010RtiProbe(self, self.rtiName(), self.rtiVersion(), self._configuration)

    def unwrap_java_factory(self) -> object:
        return self._java_factory()

    def _call(self, function: Callable[..., Any], *args: object) -> Any:
        try:
            return function(*args)
        except Exception as error:
            name = self._runtime.exception_name(error)
            if name is None:
                raise RTIinternalError(f"Java 1516e factory call failed: {error}") from error
            raise exceptionForName(_simple_name(name), str(error), error) from error


__all__ = ["Java2010RTIambassador", "Java2010RtiFactory", "Java2010RtiProbe"]
