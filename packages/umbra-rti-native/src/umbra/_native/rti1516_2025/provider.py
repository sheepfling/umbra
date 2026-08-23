"""Umbra's private implementation of the public 2025 HLA Python API."""

from __future__ import annotations

from collections.abc import Iterable
from typing import Any, Callable, TypeVar

from hla.rti1516_2025 import (
    AdditionalSettingsResultCode,
    BytesLike,
    CallbackModel,
    ConfigurationResult,
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
    DimensionHandle,
    DimensionHandleFactory,
    DimensionHandleSet,
    DimensionHandleSetFactory,
    FederateAmbassador,
    FederateHandle,
    FederateHandleFactory,
    FederateHandleSet,
    FederateHandleSetFactory,
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
    HandleFactory,
    MessageRetractionHandle,
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
    RangeBounds,
    RegionHandle,
    RegionHandleFactory,
    MessageRetractionHandleFactory,
    RegionHandleSet,
    RegionHandleSetFactory,
    RtiConfiguration,
    ResignAction,
    RtiFactory,
    TransportationTypeHandle,
    TransportationTypeHandleFactory,
    MutableAttributeHandleSet,
    MutableAttributeHandleValueMap,
    MutableDimensionHandleSet,
    MutableFederateHandleSet,
    MutableParameterHandleValueMap,
    MutableRegionHandleSet,
    MutableInteractionClassHandleSet,
    ServiceGroup,
    TimeQueryResult,
)
from hla.rti1516_2025.auth import HLAnoCredentials
from hla.rti1516_2025.core import _require_callback_model, _resolve_connect_arguments
from hla.rti1516_2025.encoding import (
    DecoderException,
    DataElement,
    DataElementFactory,
    EncoderException,
    EncoderFactory,
    HLAfixedArray,
    HLAfixedRecord,
    HLAvariantRecord,
    HLAboolean,
    HLAASCIIchar,
    HLAASCIIstring,
    HLAbyte,
    HLAfloat32BE,
    HLAfloat32LE,
    HLAfloat64BE,
    HLAfloat64LE,
    HLAinteger16BE,
    HLAinteger16LE,
    HLAinteger32BE,
    HLAinteger32LE,
    HLAinteger64BE,
    HLAinteger64LE,
    HLAunsignedInteger16BE,
    HLAunsignedInteger16LE,
    HLAunsignedInteger32LE,
    HLAunsignedInteger64BE,
    HLAunsignedInteger64LE,
    HLAoctet,
    HLAoctetPairBE,
    HLAoctetPairLE,
    HLAopaqueData,
    HLAvariableArray,
    HLAunicodeChar,
    HLAunicodeString,
    HLAunsignedInteger32BE,
    _require_integer32,
    _require_integer16,
    _require_integer16_le,
    _require_integer32_le,
    _require_integer64,
    _require_integer64_le,
    _require_float64,
    _require_float64_le,
    _require_float32,
    _require_float32_le,
    _require_unsigned_integer16,
    _require_unsigned_integer16_le,
    _require_unsigned_integer32,
    _require_unsigned_integer32_le,
    _require_unsigned_integer64,
    _require_unsigned_integer64_le,
    _require_byte,
    _require_octet,
    _require_ascii_char,
    _require_ascii_string,
    _require_unicode_char,
    _require_octet_pair,
    _require_opaque_data,
    _require_opaque_index,
    _require_data_element_factory,
    _require_data_element_index,
    _require_data_element_size,
)
from hla.rti1516_2025.exceptions import (
    InvalidLogicalTime,
    InvalidLogicalTimeInterval,
    RTIinternalError,
    exceptionForName,
)

from . import _native

_Result = TypeVar("_Result")
_Handle = TypeVar("_Handle")


def _call_native(
    function: Callable[..., _Result],
    *args: object,
    encoding_error: type[EncoderException] | None = None,
) -> _Result:
    try:
        return function(*args)
    except _native.NativeRtiError as error:
        name, separator, message = str(error).partition(": ")
        if name == "EncoderException" and encoding_error is not None:
            raise encoding_error(message if separator else name) from error
        raise exceptionForName(name, message if separator else name) from error


def _encoded_handle(value: object, expected_type: type[_Handle]) -> bytes:
    """Require a public handle of the exact standard domain before native decode."""

    if not isinstance(value, expected_type):
        raise TypeError(f"handle must be {expected_type.__name__}")
    return value.encodedValue  # type: ignore[union-attr]


def _attribute_handle_bytes(attributes: object) -> tuple[bytes, ...]:
    """Copy an immutable public attribute set for a provider-native call."""

    if not isinstance(attributes, (AttributeHandleSet, MutableAttributeHandleSet)):
        raise TypeError("attributes must be AttributeHandleSet or its factory builder")
    return tuple(_encoded_handle(attribute, AttributeHandle) for attribute in attributes)


def _interaction_class_handle_bytes(interactions: object) -> tuple[bytes, ...]:
    """Copy an immutable public interaction-class set for a provider-native call."""

    if not isinstance(interactions, (InteractionClassHandleSet, MutableInteractionClassHandleSet)):
        raise TypeError("interactionClasses must be InteractionClassHandleSet or its factory builder")
    return tuple(
        _encoded_handle(interaction, InteractionClassHandle) for interaction in interactions
    )


def _federate_handle_bytes(handles: object) -> tuple[bytes, ...]:
    if not isinstance(handles, (FederateHandleSet, MutableFederateHandleSet)):
        raise TypeError("synchronizationSet must be FederateHandleSet or its factory builder")
    return tuple(_encoded_handle(handle, FederateHandle) for handle in handles)


def _fom_modules(value: str | Iterable[str]) -> tuple[str, ...]:
    if isinstance(value, str):
        return (value,)
    return tuple(str(module) for module in value)


def _attribute_value_pairs(values: object) -> tuple[tuple[bytes, bytes], ...]:
    """Copy a public attribute-value map for the provider-native call."""

    if not isinstance(values, (AttributeHandleValueMap, MutableAttributeHandleValueMap)):
        raise TypeError("attributeValues must be AttributeHandleValueMap or its factory builder")
    return tuple(
        (_encoded_handle(attribute, AttributeHandle), bytes(value))
        for attribute, value in values.items()
    )


def _attribute_region_pairs(values: object) -> tuple[tuple[tuple[bytes, ...], tuple[bytes, ...]], ...]:
    """Encode the Java-shaped attribute-set/region-set pair list."""

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


def _dimension_handle_bytes(dimensions: object) -> tuple[bytes, ...]:
    if not isinstance(dimensions, (DimensionHandleSet, MutableDimensionHandleSet)):
        raise TypeError("dimensions must be DimensionHandleSet or its factory builder")
    return tuple(_encoded_handle(dimension, DimensionHandle) for dimension in dimensions)


def _region_handle_bytes(regions: object) -> tuple[bytes, ...]:
    if not isinstance(regions, (RegionHandleSet, MutableRegionHandleSet)):
        raise TypeError("regions must be RegionHandleSet or its factory builder")
    return tuple(_encoded_handle(region, RegionHandle) for region in regions)


def _parameter_value_pairs(values: object) -> tuple[tuple[bytes, bytes], ...]:
    """Copy a public parameter-value map for the provider-native call."""

    if not isinstance(values, (ParameterHandleValueMap, MutableParameterHandleValueMap)):
        raise TypeError("parameterValues must be ParameterHandleValueMap or its factory builder")
    return tuple(
        (_encoded_handle(parameter, ParameterHandle), bytes(value))
        for parameter, value in values.items()
    )


def _object_instance_names(value: object) -> tuple[str, ...]:
    if not isinstance(value, ObjectInstanceNameSet):
        raise TypeError("objectInstanceNames must be ObjectInstanceNameSet")
    return tuple(str(name) for name in value)


def _logical_time(record: object, interval: bool = False) -> LogicalTime | LogicalTimeInterval:
    values = dict(record)  # type: ignore[arg-type]
    implementation = str(values["implementationName"])
    if interval:
        value_type = (
            HLAinteger64Interval
            if implementation == "HLAinteger64Time"
            else HLAfloat64Interval
        )
        return value_type(
            bytes(values["encodedValue"]),
            implementation,
            bool(values["zero"]),
            bool(values["epsilon"]),
            values["value"],
            str(values["text"]),
        )
    value_type = HLAinteger64Time if implementation == "HLAinteger64Time" else HLAfloat64Time
    return value_type(
        bytes(values["encodedValue"]),
        implementation,
        bool(values["initial"]),
        bool(values["final"]),
        values["value"],
        str(values["text"]),
    )


def _time_query(record: object) -> TimeQueryResult:
    valid = bool(record["valid"])
    return TimeQueryResult(
        valid,
        _logical_time(record["time"]) if valid else None,
    )


class _NativeTimeFactoryBase:
    def __init__(self, implementation: _native.NativeAmbassador) -> None:
        self._implementation = implementation

    def implementationName(self) -> str:
        return str(self._call("time_factory_name"))

    def makeInitial(self) -> LogicalTime:
        return _logical_time(self._call("make_initial_time"))  # type: ignore[return-value]

    def makeFinal(self) -> LogicalTime:
        return _logical_time(self._call("make_final_time"))  # type: ignore[return-value]

    def makeZero(self) -> LogicalTimeInterval:
        return _logical_time(self._call("make_zero_interval"), interval=True)  # type: ignore[return-value]

    def makeEpsilon(self) -> LogicalTimeInterval:
        return _logical_time(self._call("make_epsilon_interval"), interval=True)  # type: ignore[return-value]

    def decodeLogicalTime(self, encodedValue: BytesLike) -> LogicalTime:
        return _logical_time(self._call("decode_logical_time", bytes(encodedValue)))  # type: ignore[return-value]

    def decodeLogicalTimeInterval(self, encodedValue: BytesLike) -> LogicalTimeInterval:
        return _logical_time(
            self._call("decode_logical_interval", bytes(encodedValue)), interval=True
        )  # type: ignore[return-value]

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

    def add(self, time: LogicalTime, addend: LogicalTimeInterval) -> LogicalTime:
        if not isinstance(time, LogicalTime):
            raise TypeError("time must be LogicalTime")
        if not isinstance(addend, LogicalTimeInterval):
            raise TypeError("addend must be LogicalTimeInterval")
        self._require_time(time, "time")
        self._require_interval(addend, "addend")
        return _logical_time(
            self._call("add_logical_time", time.toByteArray(), addend.toByteArray())
        )  # type: ignore[return-value]

    def subtract(self, time: LogicalTime, subtrahend: LogicalTimeInterval) -> LogicalTime:
        if not isinstance(time, LogicalTime):
            raise TypeError("time must be LogicalTime")
        if not isinstance(subtrahend, LogicalTimeInterval):
            raise TypeError("subtrahend must be LogicalTimeInterval")
        self._require_time(time, "time")
        self._require_interval(subtrahend, "subtrahend")
        return _logical_time(
            self._call("subtract_logical_time", time.toByteArray(), subtrahend.toByteArray())
        )  # type: ignore[return-value]

    def difference(self, minuend: LogicalTime, subtrahend: LogicalTime) -> LogicalTimeInterval:
        if not isinstance(minuend, LogicalTime):
            raise TypeError("minuend must be LogicalTime")
        if not isinstance(subtrahend, LogicalTime):
            raise TypeError("subtrahend must be LogicalTime")
        self._require_time(minuend, "minuend")
        self._require_time(subtrahend, "subtrahend")
        return _logical_time(
            self._call("difference_logical_time", minuend.toByteArray(), subtrahend.toByteArray()),
            interval=True,
        )  # type: ignore[return-value]

    def _call(self, name: str, *args: object) -> object:
        return _call_native(getattr(self._implementation, name), *args)


class _NativeInteger64TimeFactory(_NativeTimeFactoryBase, HLAinteger64TimeFactory):
    def makeLogicalTime(self, value: int) -> HLAinteger64Time:
        return _logical_time(self._call("make_logical_time", int(value)))  # type: ignore[return-value]

    def makeLogicalTimeInterval(self, value: int) -> HLAinteger64Interval:
        return _logical_time(
            self._call("make_logical_interval", int(value)), interval=True
        )  # type: ignore[return-value]


class _NativeFloat64TimeFactory(_NativeTimeFactoryBase, HLAfloat64TimeFactory):
    def makeLogicalTime(self, value: float) -> HLAfloat64Time:
        return _logical_time(self._call("make_logical_time", float(value)))  # type: ignore[return-value]

    def makeLogicalTimeInterval(self, value: float) -> HLAfloat64Interval:
        return _logical_time(
            self._call("make_logical_interval", float(value)), interval=True
        )  # type: ignore[return-value]


class _NativeDataElement:
    """Common forwarding logic for values owned by the C++ binding."""

    def __init__(self, implementation: Any) -> None:
        self._implementation = implementation

    def getOctetBoundary(self) -> int:
        return int(_call_native(self._implementation.get_octet_boundary))

    def getEncodedLength(self) -> int:
        return int(
            _call_native(self._implementation.get_encoded_length, encoding_error=EncoderException)
        )

    def toByteArray(self) -> bytes:
        return bytes(_call_native(self._implementation.to_byte_array, encoding_error=EncoderException))

    def decode(self, bytes_: BytesLike):
        _call_native(
            self._implementation.decode,
            bytes(bytes_),
            encoding_error=DecoderException,
        )
        return self


class _NativeHLAinteger32BE(_NativeDataElement, HLAinteger32BE):
    def getValue(self) -> int:
        return int(_call_native(self._implementation.get_value))

    def setValue(self, value: int) -> _NativeHLAinteger32BE:
        _call_native(self._implementation.set_value, _require_integer32(value))
        return self


class _NativeHLAinteger16BE(_NativeDataElement, HLAinteger16BE):
    def getValue(self) -> int:
        return int(_call_native(self._implementation.get_value))

    def setValue(self, value: int) -> _NativeHLAinteger16BE:
        _call_native(self._implementation.set_value, _require_integer16(value))
        return self


class _NativeHLAinteger16LE(_NativeDataElement, HLAinteger16LE):
    def getValue(self) -> int:
        return int(_call_native(self._implementation.get_value))

    def setValue(self, value: int) -> _NativeHLAinteger16LE:
        _call_native(self._implementation.set_value, _require_integer16_le(value))
        return self


class _NativeHLAinteger32LE(_NativeDataElement, HLAinteger32LE):
    def getValue(self) -> int:
        return int(_call_native(self._implementation.get_value))

    def setValue(self, value: int) -> _NativeHLAinteger32LE:
        _call_native(self._implementation.set_value, _require_integer32_le(value))
        return self


class _NativeHLAinteger64BE(_NativeDataElement, HLAinteger64BE):
    def getValue(self) -> int:
        return int(_call_native(self._implementation.get_value))

    def setValue(self, value: int) -> _NativeHLAinteger64BE:
        _call_native(self._implementation.set_value, _require_integer64(value))
        return self


class _NativeHLAinteger64LE(_NativeDataElement, HLAinteger64LE):
    def getValue(self) -> int:
        return int(_call_native(self._implementation.get_value))

    def setValue(self, value: int) -> _NativeHLAinteger64LE:
        _call_native(self._implementation.set_value, _require_integer64_le(value))
        return self


class _NativeHLAfloat64BE(_NativeDataElement, HLAfloat64BE):
    def getValue(self) -> float:
        return float(_call_native(self._implementation.get_value))

    def setValue(self, value: float | int) -> _NativeHLAfloat64BE:
        _call_native(self._implementation.set_value, _require_float64(value))
        return self


class _NativeHLAfloat32BE(_NativeDataElement, HLAfloat32BE):
    def getValue(self) -> float:
        return float(_call_native(self._implementation.get_value))

    def setValue(self, value: float | int) -> _NativeHLAfloat32BE:
        _call_native(self._implementation.set_value, _require_float32(value))
        return self


class _NativeHLAfloat64LE(_NativeDataElement, HLAfloat64LE):
    def getValue(self) -> float:
        return float(_call_native(self._implementation.get_value))

    def setValue(self, value: float | int) -> _NativeHLAfloat64LE:
        _call_native(self._implementation.set_value, _require_float64_le(value))
        return self


class _NativeHLAfloat32LE(_NativeDataElement, HLAfloat32LE):
    def getValue(self) -> float:
        return float(_call_native(self._implementation.get_value))

    def setValue(self, value: float | int) -> _NativeHLAfloat32LE:
        _call_native(self._implementation.set_value, _require_float32_le(value))
        return self


class _NativeHLAunsignedInteger16BE(_NativeDataElement, HLAunsignedInteger16BE):
    def getValue(self) -> int:
        return int(_call_native(self._implementation.get_value))

    def setValue(self, value: int) -> _NativeHLAunsignedInteger16BE:
        _call_native(self._implementation.set_value, _require_unsigned_integer16(value))
        return self


class _NativeHLAunsignedInteger16LE(_NativeDataElement, HLAunsignedInteger16LE):
    def getValue(self) -> int:
        return int(_call_native(self._implementation.get_value))

    def setValue(self, value: int) -> _NativeHLAunsignedInteger16LE:
        _call_native(self._implementation.set_value, _require_unsigned_integer16_le(value))
        return self


class _NativeHLAunsignedInteger32LE(_NativeDataElement, HLAunsignedInteger32LE):
    def getValue(self) -> int:
        return int(_call_native(self._implementation.get_value))

    def setValue(self, value: int) -> _NativeHLAunsignedInteger32LE:
        _call_native(self._implementation.set_value, _require_unsigned_integer32_le(value))
        return self


class _NativeHLAunsignedInteger64BE(_NativeDataElement, HLAunsignedInteger64BE):
    def getValue(self) -> int:
        return int(_call_native(self._implementation.get_value))

    def setValue(self, value: int) -> _NativeHLAunsignedInteger64BE:
        _call_native(self._implementation.set_value, _require_unsigned_integer64(value))
        return self


class _NativeHLAunsignedInteger64LE(_NativeDataElement, HLAunsignedInteger64LE):
    def getValue(self) -> int:
        return int(_call_native(self._implementation.get_value))

    def setValue(self, value: int) -> _NativeHLAunsignedInteger64LE:
        _call_native(self._implementation.set_value, _require_unsigned_integer64_le(value))
        return self


class _NativeHLAbyte(_NativeDataElement, HLAbyte):
    def getValue(self) -> int:
        return int(_call_native(self._implementation.get_value))

    def setValue(self, value: int) -> _NativeHLAbyte:
        _call_native(self._implementation.set_value, _require_byte(value))
        return self


class _NativeHLAoctet(_NativeDataElement, HLAoctet):
    def getValue(self) -> int:
        return int(_call_native(self._implementation.get_value))

    def setValue(self, value: int) -> _NativeHLAoctet:
        _call_native(self._implementation.set_value, _require_octet(value))
        return self


class _NativeHLAASCIIchar(_NativeDataElement, HLAASCIIchar):
    def getValue(self) -> int:
        return int(_call_native(self._implementation.get_value))

    def setValue(self, value: int) -> _NativeHLAASCIIchar:
        _call_native(self._implementation.set_value, _require_ascii_char(value))
        return self


class _NativeHLAASCIIstring(_NativeDataElement, HLAASCIIstring):
    def getValue(self) -> str:
        return str(_call_native(self._implementation.get_value))

    def setValue(self, value: str) -> _NativeHLAASCIIstring:
        _call_native(self._implementation.set_value, _require_ascii_string(value))
        return self


class _NativeHLAunicodeChar(_NativeDataElement, HLAunicodeChar):
    def getValue(self) -> int:
        return int(_call_native(self._implementation.get_value))

    def setValue(self, value: int) -> _NativeHLAunicodeChar:
        _call_native(self._implementation.set_value, _require_unicode_char(value))
        return self


class _NativeHLAoctetPairBE(_NativeDataElement, HLAoctetPairBE):
    def getValue(self) -> int:
        return int(_call_native(self._implementation.get_value))

    def setValue(self, value: int) -> _NativeHLAoctetPairBE:
        _call_native(self._implementation.set_value, _require_octet_pair(value))
        return self


class _NativeHLAoctetPairLE(_NativeDataElement, HLAoctetPairLE):
    def getValue(self) -> int:
        return int(_call_native(self._implementation.get_value))

    def setValue(self, value: int) -> _NativeHLAoctetPairLE:
        _call_native(self._implementation.set_value, _require_octet_pair(value))
        return self


class _NativeHLAopaqueData(_NativeDataElement, HLAopaqueData):
    def size(self) -> int:
        return int(_call_native(self._implementation.data_length))

    def get(self, index: int) -> int:
        return self.getValue()[_require_opaque_index(index, self.size())]

    def getValue(self) -> bytes:
        return bytes(_call_native(self._implementation.get_value))

    def setValue(self, value: BytesLike) -> None:
        _call_native(self._implementation.set_value, _require_opaque_data(value))


class _NativeHLAvariableArray(_NativeDataElement, HLAvariableArray):
    def __init__(
        self,
        implementation: Any,
        factory: DataElementFactory,
        prototype: DataElement,
    ) -> None:
        super().__init__(implementation)
        self._factory = factory
        self._prototype = prototype

    def addElement(self, dataElement: DataElement) -> None:
        if not isinstance(dataElement, type(self._prototype)):
            raise TypeError("dataElement does not match the variable-array prototype")
        _call_native(
            self._implementation.add_element,
            dataElement.toByteArray(),
            encoding_error=EncoderException,
        )

    def size(self) -> int:
        return int(_call_native(self._implementation.size))

    def get(self, index: int) -> DataElement:
        index = _require_data_element_index(index, self.size())
        element = self._factory.createElement(index)
        if not isinstance(element, type(self._prototype)):
            raise TypeError("factory returned an element that does not match the prototype")
        element.decode(
            bytes(
                _call_native(
                    self._implementation.get_element_bytes,
                    index,
                    encoding_error=EncoderException,
                )
            )
        )
        return element


class _NativeHLAfixedArray(_NativeDataElement, HLAfixedArray):
    def __init__(
        self,
        implementation: Any,
        factory: DataElementFactory,
        prototype: DataElement,
    ) -> None:
        super().__init__(implementation)
        self._factory = factory
        self._prototype = prototype

    def size(self) -> int:
        return int(_call_native(self._implementation.size))

    def set(self, index: int, dataElement: DataElement) -> None:
        index = _require_data_element_index(index, self.size())
        if not isinstance(dataElement, type(self._prototype)):
            raise TypeError("dataElement does not match the fixed-array prototype")
        _call_native(
            self._implementation.set_element,
            index,
            dataElement.toByteArray(),
            encoding_error=EncoderException,
        )

    def get(self, index: int) -> DataElement:
        index = _require_data_element_index(index, self.size())
        element = self._factory.createElement(index)
        if not isinstance(element, type(self._prototype)):
            raise TypeError("factory returned an element that does not match the prototype")
        element.decode(
            bytes(
                _call_native(
                    self._implementation.get_element_bytes,
                    index,
                    encoding_error=EncoderException,
                )
            )
        )
        return element


class _NativeHLAfixedRecord(_NativeDataElement, HLAfixedRecord):
    """Native façade for a heterogeneous C++ ``HLAfixedRecord``.

    The C++ object owns copies of appended components.  Python keeps typed
    component façades so ``get`` can expose a normal public data element and
    synchronize it from the native record before returning it.
    """

    def __init__(self, implementation: Any) -> None:
        super().__init__(implementation)
        self._elements: list[DataElement] = []

    def appendElement(self, dataElement: DataElement) -> None:
        if not isinstance(dataElement, _NativeDataElement):
            raise TypeError("native fixed records require native DataElements")
        _call_native(
            self._implementation.append_element,
            dataElement._implementation,
            dataElement.toByteArray(),
            encoding_error=EncoderException,
        )
        self._elements.append(dataElement)

    def size(self) -> int:
        return int(_call_native(self._implementation.size))

    def _component(self, index: int) -> DataElement:
        index = _require_data_element_index(index, self.size())
        if index >= len(self._elements):
            raise EncoderException("native fixed record component state is unavailable")
        element = self._elements[index]
        element.decode(
            bytes(
                _call_native(
                    self._implementation.get_element_bytes,
                    index,
                    encoding_error=EncoderException,
                )
            )
        )
        return element

    def get(self, index: int) -> DataElement:
        return self._component(index)

    def set(self, index: int, dataElement: DataElement) -> None:
        index = _require_data_element_index(index, self.size())
        if index >= len(self._elements):
            raise EncoderException("native fixed record component state is unavailable")
        if not isinstance(dataElement, type(self._elements[index])):
            raise TypeError("dataElement does not match the fixed-record component type")
        _call_native(
            self._implementation.set_element,
            index,
            dataElement._implementation,
            dataElement.toByteArray(),
            encoding_error=EncoderException,
        )
        self._elements[index].decode(dataElement.toByteArray())


class _NativeHLAvariantRecord(_NativeDataElement, HLAvariantRecord):
    def __init__(self, implementation: Any, discriminant: DataElement) -> None:
        super().__init__(implementation)
        self._discriminant = discriminant
        self._variants: dict[bytes, DataElement] = {}

    def _require_discriminant(self, discriminant: DataElement) -> bytes:
        if not isinstance(discriminant, type(self._discriminant)):
            raise TypeError("discriminant does not match the variant-record prototype")
        return discriminant.toByteArray()

    def setVariant(self, discriminant: DataElement, dataElement: DataElement) -> None:
        key = self._require_discriminant(discriminant)
        if not isinstance(dataElement, _NativeDataElement):
            raise TypeError("native variant records require native DataElements")
        if key in self._variants and not isinstance(dataElement, type(self._variants[key])):
            raise TypeError("dataElement does not match the mapped variant type")
        method = self._implementation.set_variant if key in self._variants else self._implementation.add_variant
        _call_native(
            method,
            self._discriminant._implementation,
            key,
            dataElement._implementation,
            dataElement.toByteArray(),
            encoding_error=EncoderException,
        )
        self._variants[key] = dataElement

    def setDiscriminant(self, discriminant: DataElement) -> None:
        key = self._require_discriminant(discriminant)
        _call_native(
            self._implementation.set_discriminant,
            self._discriminant._implementation,
            key,
            encoding_error=EncoderException,
        )

    def getDiscriminant(self) -> DataElement:
        self._discriminant.decode(
            bytes(
                _call_native(
                    self._implementation.get_discriminant_bytes,
                    encoding_error=EncoderException,
                )
            )
        )
        return self._discriminant

    def getValue(self) -> DataElement | None:
        key = bytes(self.getDiscriminant().toByteArray())
        element = self._variants.get(key)
        if element is None:
            return None
        element.decode(
            bytes(
                _call_native(
                    self._implementation.get_variant_bytes,
                    encoding_error=EncoderException,
                )
            )
        )
        return element


class _NativeHLAextendableVariantRecord(_NativeDataElement, DataElement):
    """Native-only façade for C++ ``HLAextendableVariantRecord``.

    IEEE 1516.1-2025's C++ encoding helper has no matching Java
    ``EncoderFactory`` surface in the verified Java API, so this deliberately
    remains an edition/provider-specific method on the native factory rather
    than part of the shared abstract Python contract.
    """

    def __init__(self, implementation: Any, discriminant: DataElement) -> None:
        super().__init__(implementation)
        self._discriminant = discriminant
        self._variants: dict[bytes, DataElement] = {}

    def _require_discriminant(self, discriminant: DataElement) -> bytes:
        if not isinstance(discriminant, type(self._discriminant)):
            raise TypeError("discriminant does not match the extendable-variant prototype")
        return discriminant.toByteArray()

    def addVariant(self, discriminant: DataElement, dataElement: DataElement) -> None:
        key = self._require_discriminant(discriminant)
        if not isinstance(dataElement, _NativeDataElement):
            raise TypeError("native extendable variants require native DataElements")
        if key in self._variants:
            raise EncoderException("extendable-variant discriminant is already mapped")
        _call_native(
            self._implementation.add_variant,
            self._discriminant._implementation,
            key,
            dataElement._implementation,
            dataElement.toByteArray(),
            encoding_error=EncoderException,
        )
        self._variants[key] = dataElement

    def setVariant(self, discriminant: DataElement, dataElement: DataElement) -> None:
        key = self._require_discriminant(discriminant)
        if not isinstance(dataElement, _NativeDataElement):
            raise TypeError("native extendable variants require native DataElements")
        existing = self._variants.get(key)
        if existing is None:
            raise EncoderException("extendable-variant discriminant is not mapped")
        if not isinstance(dataElement, type(existing)):
            raise TypeError("dataElement does not match the mapped extendable variant type")
        _call_native(
            self._implementation.set_variant,
            self._discriminant._implementation,
            key,
            dataElement._implementation,
            dataElement.toByteArray(),
            encoding_error=EncoderException,
        )
        existing.decode(dataElement.toByteArray())

    def setDiscriminant(self, discriminant: DataElement) -> None:
        key = self._require_discriminant(discriminant)
        _call_native(
            self._implementation.set_discriminant,
            self._discriminant._implementation,
            key,
            encoding_error=EncoderException,
        )

    def getDiscriminant(self) -> DataElement:
        self._discriminant.decode(
            bytes(
                _call_native(
                    self._implementation.get_discriminant_bytes,
                    encoding_error=EncoderException,
                )
            )
        )
        return self._discriminant

    def getValue(self) -> DataElement | None:
        key = bytes(self.getDiscriminant().toByteArray())
        element = self._variants.get(key)
        if element is None:
            return None
        element.decode(
            bytes(
                _call_native(
                    self._implementation.get_variant_bytes,
                    encoding_error=EncoderException,
                )
            )
        )
        return element


class _NativeHLAunsignedInteger32BE(_NativeDataElement, HLAunsignedInteger32BE):
    def getValue(self) -> int:
        return int(_call_native(self._implementation.get_value))

    def setValue(self, value: int) -> _NativeHLAunsignedInteger32BE:
        _call_native(self._implementation.set_value, _require_unsigned_integer32(value))
        return self


class _NativeHLAboolean(_NativeDataElement, HLAboolean):
    def getValue(self) -> bool:
        return bool(_call_native(self._implementation.get_value))

    def setValue(self, value: bool) -> _NativeHLAboolean:
        _call_native(self._implementation.set_value, value)
        return self


class _NativeHLAunicodeString(_NativeDataElement, HLAunicodeString):
    def getValue(self) -> str:
        return str(_call_native(self._implementation.get_value))

    def setValue(self, value: str) -> _NativeHLAunicodeString:
        _call_native(self._implementation.set_value, str(value))
        return self


class _NativeEncoderFactory(EncoderFactory):
    """Java-shaped factory façade over Umbra's concrete C++ data elements."""

    def createHLAinteger32BE(self, value: int | None = None) -> HLAinteger32BE:
        implementation = (
            _call_native(_native.NativeHLAinteger32BE)
            if value is None
            else _call_native(_native.NativeHLAinteger32BE, _require_integer32(value))
        )
        return _NativeHLAinteger32BE(implementation)

    def createHLAinteger16BE(self, value: int | None = None) -> HLAinteger16BE:
        implementation = (
            _call_native(_native.NativeHLAinteger16BE)
            if value is None
            else _call_native(_native.NativeHLAinteger16BE, _require_integer16(value))
        )
        return _NativeHLAinteger16BE(implementation)

    def createHLAinteger16LE(self, value: int | None = None) -> HLAinteger16LE:
        implementation = (
            _call_native(_native.NativeHLAinteger16LE)
            if value is None
            else _call_native(_native.NativeHLAinteger16LE, _require_integer16_le(value))
        )
        return _NativeHLAinteger16LE(implementation)

    def createHLAinteger32LE(self, value: int | None = None) -> HLAinteger32LE:
        implementation = (
            _call_native(_native.NativeHLAinteger32LE)
            if value is None
            else _call_native(_native.NativeHLAinteger32LE, _require_integer32_le(value))
        )
        return _NativeHLAinteger32LE(implementation)

    def createHLAinteger64BE(self, value: int | None = None) -> HLAinteger64BE:
        implementation = (
            _call_native(_native.NativeHLAinteger64BE)
            if value is None
            else _call_native(_native.NativeHLAinteger64BE, _require_integer64(value))
        )
        return _NativeHLAinteger64BE(implementation)

    def createHLAinteger64LE(self, value: int | None = None) -> HLAinteger64LE:
        implementation = (
            _call_native(_native.NativeHLAinteger64LE)
            if value is None
            else _call_native(_native.NativeHLAinteger64LE, _require_integer64_le(value))
        )
        return _NativeHLAinteger64LE(implementation)

    def createHLAfloat64BE(self, value: float | int | None = None) -> HLAfloat64BE:
        implementation = (
            _call_native(_native.NativeHLAfloat64BE)
            if value is None
            else _call_native(_native.NativeHLAfloat64BE, _require_float64(value))
        )
        return _NativeHLAfloat64BE(implementation)

    def createHLAfloat32BE(self, value: float | int | None = None) -> HLAfloat32BE:
        implementation = (
            _call_native(_native.NativeHLAfloat32BE)
            if value is None
            else _call_native(_native.NativeHLAfloat32BE, _require_float32(value))
        )
        return _NativeHLAfloat32BE(implementation)

    def createHLAfloat64LE(self, value: float | int | None = None) -> HLAfloat64LE:
        implementation = (
            _call_native(_native.NativeHLAfloat64LE)
            if value is None
            else _call_native(_native.NativeHLAfloat64LE, _require_float64_le(value))
        )
        return _NativeHLAfloat64LE(implementation)

    def createHLAfloat32LE(self, value: float | int | None = None) -> HLAfloat32LE:
        implementation = (
            _call_native(_native.NativeHLAfloat32LE)
            if value is None
            else _call_native(_native.NativeHLAfloat32LE, _require_float32_le(value))
        )
        return _NativeHLAfloat32LE(implementation)

    def createHLAunsignedInteger16BE(
        self, value: int | None = None
    ) -> HLAunsignedInteger16BE:
        implementation = (
            _call_native(_native.NativeHLAunsignedInteger16BE)
            if value is None
            else _call_native(
                _native.NativeHLAunsignedInteger16BE,
                _require_unsigned_integer16(value),
            )
        )
        return _NativeHLAunsignedInteger16BE(implementation)

    def createHLAunsignedInteger16LE(
        self, value: int | None = None
    ) -> HLAunsignedInteger16LE:
        implementation = (
            _call_native(_native.NativeHLAunsignedInteger16LE)
            if value is None
            else _call_native(
                _native.NativeHLAunsignedInteger16LE,
                _require_unsigned_integer16_le(value),
            )
        )
        return _NativeHLAunsignedInteger16LE(implementation)

    def createHLAunsignedInteger32LE(
        self, value: int | None = None
    ) -> HLAunsignedInteger32LE:
        implementation = (
            _call_native(_native.NativeHLAunsignedInteger32LE)
            if value is None
            else _call_native(
                _native.NativeHLAunsignedInteger32LE,
                _require_unsigned_integer32_le(value),
            )
        )
        return _NativeHLAunsignedInteger32LE(implementation)

    def createHLAunsignedInteger32BE(self, value: int | None = None) -> HLAunsignedInteger32BE:
        implementation = (
            _call_native(_native.NativeHLAunsignedInteger32BE)
            if value is None
            else _call_native(
                _native.NativeHLAunsignedInteger32BE,
                _require_unsigned_integer32(value),
            )
        )
        return _NativeHLAunsignedInteger32BE(implementation)

    def createHLAunsignedInteger64BE(
        self, value: int | None = None
    ) -> HLAunsignedInteger64BE:
        implementation = (
            _call_native(_native.NativeHLAunsignedInteger64BE)
            if value is None
            else _call_native(
                _native.NativeHLAunsignedInteger64BE,
                _require_unsigned_integer64(value),
            )
        )
        return _NativeHLAunsignedInteger64BE(implementation)

    def createHLAunsignedInteger64LE(
        self, value: int | None = None
    ) -> HLAunsignedInteger64LE:
        implementation = (
            _call_native(_native.NativeHLAunsignedInteger64LE)
            if value is None
            else _call_native(
                _native.NativeHLAunsignedInteger64LE,
                _require_unsigned_integer64_le(value),
            )
        )
        return _NativeHLAunsignedInteger64LE(implementation)

    def createHLAbyte(self, value: int | None = None) -> HLAbyte:
        implementation = (
            _call_native(_native.NativeHLAbyte)
            if value is None
            else _call_native(_native.NativeHLAbyte, _require_byte(value))
        )
        return _NativeHLAbyte(implementation)

    def createHLAoctet(self, value: int | None = None) -> HLAoctet:
        implementation = (
            _call_native(_native.NativeHLAoctet)
            if value is None
            else _call_native(_native.NativeHLAoctet, _require_octet(value))
        )
        return _NativeHLAoctet(implementation)

    def createHLAASCIIchar(self, value: int | None = None) -> HLAASCIIchar:
        implementation = (
            _call_native(_native.NativeHLAASCIIchar)
            if value is None
            else _call_native(_native.NativeHLAASCIIchar, _require_ascii_char(value))
        )
        return _NativeHLAASCIIchar(implementation)

    def createHLAASCIIstring(self, value: str | None = None) -> HLAASCIIstring:
        implementation = (
            _call_native(_native.NativeHLAASCIIstring)
            if value is None
            else _call_native(_native.NativeHLAASCIIstring, _require_ascii_string(value))
        )
        return _NativeHLAASCIIstring(implementation)

    def createHLAunicodeChar(self, value: int | None = None) -> HLAunicodeChar:
        implementation = (
            _call_native(_native.NativeHLAunicodeChar)
            if value is None
            else _call_native(_native.NativeHLAunicodeChar, _require_unicode_char(value))
        )
        return _NativeHLAunicodeChar(implementation)

    def createHLAoctetPairBE(self, value: int | None = None) -> HLAoctetPairBE:
        implementation = (
            _call_native(_native.NativeHLAoctetPairBE)
            if value is None
            else _call_native(_native.NativeHLAoctetPairBE, _require_octet_pair(value))
        )
        return _NativeHLAoctetPairBE(implementation)

    def createHLAoctetPairLE(self, value: int | None = None) -> HLAoctetPairLE:
        implementation = (
            _call_native(_native.NativeHLAoctetPairLE)
            if value is None
            else _call_native(_native.NativeHLAoctetPairLE, _require_octet_pair(value))
        )
        return _NativeHLAoctetPairLE(implementation)

    def createHLAopaqueData(
        self, value: BytesLike | None = None
    ) -> HLAopaqueData:
        implementation = (
            _call_native(_native.NativeHLAopaqueData)
            if value is None
            else _call_native(_native.NativeHLAopaqueData, _require_opaque_data(value))
        )
        return _NativeHLAopaqueData(implementation)

    def createHLAvariableArray(
        self, factory: DataElementFactory, *elements: DataElement
    ) -> HLAvariableArray:
        factory = _require_data_element_factory(factory)
        prototype = factory.createElement(0)
        if not isinstance(prototype, _NativeDataElement):
            raise TypeError("native variable arrays require a native DataElement factory")
        implementation = _call_native(
            _native.NativeHLAvariableArray,
            prototype._implementation,
        )
        result = _NativeHLAvariableArray(implementation, factory, prototype)
        for element in elements:
            result.addElement(element)
        return result

    def createHLAfixedArray(self, factory: DataElementFactory, size: int) -> HLAfixedArray:
        factory = _require_data_element_factory(factory)
        size = _require_data_element_size(size)
        prototype = factory.createElement(0)
        if not isinstance(prototype, _NativeDataElement):
            raise TypeError("native fixed arrays require a native DataElement factory")
        implementation = _call_native(
            _native.NativeHLAfixedArray,
            prototype._implementation,
            size,
        )
        return _NativeHLAfixedArray(implementation, factory, prototype)

    def createHLAfixedRecord(self) -> HLAfixedRecord:
        return _NativeHLAfixedRecord(_call_native(_native.NativeHLAfixedRecord))

    def createHLAvariantRecord(self, discriminantPrototype: DataElement) -> HLAvariantRecord:
        if not isinstance(discriminantPrototype, _NativeDataElement):
            raise TypeError("native variant records require a native discriminant prototype")
        implementation = _call_native(
            _native.NativeHLAvariantRecord,
            discriminantPrototype._implementation,
        )
        return _NativeHLAvariantRecord(implementation, discriminantPrototype)

    def createHLAextendableVariantRecord(self, discriminantPrototype: DataElement) -> DataElement:
        """Create the native-only C++ extendable-variant encoding helper."""
        if not isinstance(discriminantPrototype, _NativeDataElement):
            raise TypeError("native extendable variants require a native discriminant prototype")
        implementation = _call_native(
            _native.NativeHLAextendableVariantRecord,
            discriminantPrototype._implementation,
        )
        return _NativeHLAextendableVariantRecord(implementation, discriminantPrototype)

    def createHLAboolean(self, value: bool | None = None) -> HLAboolean:
        implementation = (
            _call_native(_native.NativeHLAboolean)
            if value is None
            else _call_native(_native.NativeHLAboolean, value)
        )
        return _NativeHLAboolean(implementation)

    def createHLAunicodeString(self, value: str | None = None) -> HLAunicodeString:
        implementation = (
            _call_native(_native.NativeHLAunicodeString)
            if value is None
            else _call_native(_native.NativeHLAunicodeString, str(value))
        )
        return _NativeHLAunicodeString(implementation)


class _NativeHandleFactory(HandleFactory):
    """Provider-backed decoder façade over Umbra's C++ handle codecs."""

    def __init__(
        self,
        implementation: _native.NativeAmbassador,
        method_name: str,
        handle_type: type[_Handle],
    ) -> None:
        self._implementation = implementation
        self._method_name = method_name
        self._handle_type = handle_type

    def decode(self, encodedValue: BytesLike) -> _Handle:
        return self._handle_type(
            _call_native(getattr(self._implementation, self._method_name), bytes(encodedValue))
        )


class _NativeFederateHandleFactory(_NativeHandleFactory, FederateHandleFactory):
    pass


class _NativeObjectClassHandleFactory(_NativeHandleFactory, ObjectClassHandleFactory):
    pass


class _NativeObjectInstanceHandleFactory(_NativeHandleFactory, ObjectInstanceHandleFactory):
    pass


class _NativeAttributeHandleFactory(_NativeHandleFactory, AttributeHandleFactory):
    pass


class _NativeInteractionClassHandleFactory(_NativeHandleFactory, InteractionClassHandleFactory):
    pass


class _NativeParameterHandleFactory(_NativeHandleFactory, ParameterHandleFactory):
    pass


class _NativeTransportationTypeHandleFactory(
    _NativeHandleFactory, TransportationTypeHandleFactory
):
    pass


class _NativeDimensionHandleFactory(_NativeHandleFactory, DimensionHandleFactory):
    pass


class _NativeRegionHandleFactory(_NativeHandleFactory, RegionHandleFactory):
    pass


class _NativeMessageRetractionHandleFactory(
    _NativeHandleFactory, MessageRetractionHandleFactory
):
    pass


class _NativeAttributeHandleSetFactory(AttributeHandleSetFactory):
    def create(self) -> MutableAttributeHandleSet:
        return MutableAttributeHandleSet()


class _NativeDimensionHandleSetFactory(DimensionHandleSetFactory):
    def create(self) -> MutableDimensionHandleSet:
        return MutableDimensionHandleSet()


class _NativeFederateHandleSetFactory(FederateHandleSetFactory):
    def create(self) -> MutableFederateHandleSet:
        return MutableFederateHandleSet()


class _NativeRegionHandleSetFactory(RegionHandleSetFactory):
    def create(self) -> MutableRegionHandleSet:
        return MutableRegionHandleSet()


class _NativeInteractionClassHandleSetFactory(InteractionClassHandleSetFactory):
    def create(self) -> MutableInteractionClassHandleSet:
        return MutableInteractionClassHandleSet()


class _NativeAttributeSetRegionSetPairListFactory(AttributeSetRegionSetPairListFactory):
    def create(self, capacity: int = 0) -> MutableAttributeSetRegionSetPairList:
        return MutableAttributeSetRegionSetPairList(int(capacity))


class _NativeAttributeHandleValueMapFactory(AttributeHandleValueMapFactory):
    def create(self) -> MutableAttributeHandleValueMap:
        return MutableAttributeHandleValueMap()


class _NativeParameterHandleValueMapFactory(ParameterHandleValueMapFactory):
    def create(self) -> MutableParameterHandleValueMap:
        return MutableParameterHandleValueMap()


class _UmbraRTIambassador(RTIambassador):
    def __init__(self, implementation: _native.NativeAmbassador) -> None:
        self._implementation = implementation

    def getHLAversion(self) -> str:
        return str(self._call(self._implementation.hla_version))

    def connect(
        self,
        federateAmbassador: FederateAmbassador,
        callbackModel: CallbackModel,
        configuration: RtiConfiguration | object | None = None,
        credentials: object | None = None,
    ) -> ConfigurationResult:
        callbackModel = _require_callback_model(callbackModel)
        configuration, credentials = _resolve_connect_arguments(configuration, credentials)
        if credentials is not None and not isinstance(credentials, HLAnoCredentials):
            raise RTIinternalError("Umbra currently supports only HLAnoCredentials")
        result = self._call(
            self._implementation.connect,
            federateAmbassador,
            {
                CallbackModel.HLA_IMMEDIATE: "immediate",
                CallbackModel.HLA_EVOKED: "evoked",
            }[callbackModel],
            configuration.configurationName() if configuration is not None else "",
            configuration.rtiAddress() if configuration is not None else "",
            configuration.additionalSettings() if configuration is not None else "",
            configuration is not None,
            credentials is not None,
        )
        return ConfigurationResult(
            configurationUsed=result.configuration_used,
            addressUsed=result.address_used,
            additionalSettingsResultCode={
                "ignored": AdditionalSettingsResultCode.SETTINGS_IGNORED,
                "failed_to_parse": AdditionalSettingsResultCode.SETTINGS_FAILED_TO_PARSE,
                "applied": AdditionalSettingsResultCode.SETTINGS_APPLIED,
            }[result.additional_settings_result],
            message=result.message,
        )

    def disconnect(self) -> None:
        self._call(self._implementation.disconnect)

    def evokeCallback(self, approximateMinimumTimeInSeconds: float) -> bool:
        return self._call(self._implementation.evoke_callback, approximateMinimumTimeInSeconds)

    def evokeMultipleCallbacks(
        self,
        approximateMinimumTimeInSeconds: float,
        approximateMaximumTimeInSeconds: float,
    ) -> bool:
        return self._call(
            self._implementation.evoke_multiple_callbacks,
            approximateMinimumTimeInSeconds,
            approximateMaximumTimeInSeconds,
        )

    def enableCallbacks(self) -> None:
        self._call(self._implementation.enable_callbacks)

    def disableCallbacks(self) -> None:
        self._call(self._implementation.disable_callbacks)

    def listFederationExecutions(self) -> None:
        self._call(self._implementation.list_federation_executions)

    def listFederationExecutionMembers(self, federationExecutionName: str) -> None:
        self._call(self._implementation.list_federation_execution_members, federationExecutionName)

    def joinFederationExecution(
        self,
        federateType: str,
        federationExecutionName: str,
        *,
        federateName: str | None = None,
        additionalFomModules: Iterable[str] = (),
    ) -> FederateHandle:
        modules = tuple(str(module) for module in additionalFomModules)
        if modules:
            return FederateHandle(
                self._call(
                    self._implementation.join_federation_execution_with_modules,
                    federateType,
                    federationExecutionName,
                    federateName or "",
                    federateName is not None,
                    modules,
                )
            )
        return FederateHandle(
            self._call(
                self._implementation.join_federation_execution,
                federateType,
                federationExecutionName,
                federateName or "",
                federateName is not None,
            )
        )

    def resignFederationExecution(self, resignAction: ResignAction) -> None:
        if not isinstance(resignAction, ResignAction):
            raise TypeError("resignAction must be ResignAction")
        self._call(self._implementation.resign_federation_execution, resignAction.name)

    def registerFederationSynchronizationPoint(
        self,
        synchronizationPointLabel: str,
        userSuppliedTag: BytesLike = b"",
        *,
        synchronizationSet: FederateHandleSet | None = None,
    ) -> None:
        if synchronizationSet is not None:
            self._call(
                self._implementation.register_federation_synchronization_point_with_set,
                synchronizationPointLabel,
                bytes(userSuppliedTag),
                _federate_handle_bytes(synchronizationSet),
            )
            return
        self._call(
            self._implementation.register_federation_synchronization_point,
            synchronizationPointLabel,
            bytes(userSuppliedTag),
        )

    def synchronizationPointAchieved(
        self, synchronizationPointLabel: str, successfully: bool = True
    ) -> None:
        self._call(
            self._implementation.synchronization_point_achieved,
            synchronizationPointLabel,
            bool(successfully),
        )

    def queryFederationSaveStatus(self) -> None:
        self._call(self._implementation.query_federation_save_status)

    def requestFederationSave(self, label: str, time: LogicalTime | None = None) -> None:
        if time is None:
            self._call(self._implementation.request_federation_save, label)
            return
        if not isinstance(time, LogicalTime):
            raise TypeError("time must be LogicalTime")
        self._call(
            self._implementation.request_federation_save_with_time,
            label,
            time.encodedValue,
        )

    def federateSaveBegun(self) -> None:
        self._call(self._implementation.federate_save_begun)

    def federateSaveComplete(self) -> None:
        self._call(self._implementation.federate_save_complete)

    def federateSaveNotComplete(self) -> None:
        self._call(self._implementation.federate_save_not_complete)

    def abortFederationSave(self) -> None:
        self._call(self._implementation.abort_federation_save)

    def queryFederationRestoreStatus(self) -> None:
        self._call(self._implementation.query_federation_restore_status)

    def requestFederationRestore(self, label: str) -> None:
        self._call(self._implementation.request_federation_restore, label)

    def federateRestoreComplete(self) -> None:
        self._call(self._implementation.federate_restore_complete)

    def federateRestoreNotComplete(self) -> None:
        self._call(self._implementation.federate_restore_not_complete)

    def abortFederationRestore(self) -> None:
        self._call(self._implementation.abort_federation_restore)

    def publishObjectClassAttributes(
        self, objectClass: ObjectClassHandle, attributes: AttributeHandleSet
    ) -> None:
        self._call(
            self._implementation.publish_object_class_attributes,
            _encoded_handle(objectClass, ObjectClassHandle),
            _attribute_handle_bytes(attributes),
        )

    def unpublishObjectClass(self, objectClass: ObjectClassHandle) -> None:
        self._call(
            self._implementation.unpublish_object_class,
            _encoded_handle(objectClass, ObjectClassHandle),
        )

    def unpublishObjectClassAttributes(
        self, objectClass: ObjectClassHandle, attributes: AttributeHandleSet
    ) -> None:
        self._call(
            self._implementation.unpublish_object_class_attributes,
            _encoded_handle(objectClass, ObjectClassHandle),
            _attribute_handle_bytes(attributes),
        )

    def publishObjectClassDirectedInteractions(
        self,
        objectClass: ObjectClassHandle,
        interactionClasses: InteractionClassHandleSet,
    ) -> None:
        self._call(
            self._implementation.publish_object_class_directed_interactions,
            _encoded_handle(objectClass, ObjectClassHandle),
            _interaction_class_handle_bytes(interactionClasses),
        )

    def unpublishObjectClassDirectedInteractions(
        self,
        objectClass: ObjectClassHandle,
        interactionClasses: InteractionClassHandleSet | None = None,
    ) -> None:
        arguments: tuple[object, ...] = (
            _encoded_handle(objectClass, ObjectClassHandle),
            None
            if interactionClasses is None
            else _interaction_class_handle_bytes(interactionClasses),
        )
        self._call(self._implementation.unpublish_object_class_directed_interactions, *arguments)

    def subscribeObjectClassAttributes(
        self,
        objectClass: ObjectClassHandle,
        attributes: AttributeHandleSet,
        *,
        active: bool = True,
        updateRateDesignator: str = "",
    ) -> None:
        self._call(
            self._implementation.subscribe_object_class_attributes,
            _encoded_handle(objectClass, ObjectClassHandle),
            _attribute_handle_bytes(attributes),
            bool(active),
            str(updateRateDesignator),
        )

    def subscribeObjectClassAttributesPassively(
        self,
        objectClass: ObjectClassHandle,
        attributes: AttributeHandleSet,
        updateRateDesignator: str = "",
    ) -> None:
        self.subscribeObjectClassAttributes(
            objectClass,
            attributes,
            active=False,
            updateRateDesignator=updateRateDesignator,
        )

    def subscribeObjectClassDirectedInteractions(
        self,
        objectClass: ObjectClassHandle,
        interactionClasses: InteractionClassHandleSet,
        *,
        universally: bool = False,
    ) -> None:
        self._call(
            self._implementation.subscribe_object_class_directed_interactions,
            _encoded_handle(objectClass, ObjectClassHandle),
            _interaction_class_handle_bytes(interactionClasses),
            bool(universally),
        )

    def subscribeObjectClassDirectedInteractionsUniversally(
        self,
        objectClass: ObjectClassHandle,
        interactionClasses: InteractionClassHandleSet | MutableInteractionClassHandleSet,
    ) -> None:
        self.subscribeObjectClassDirectedInteractions(
            objectClass, interactionClasses, universally=True
        )

    def unsubscribeObjectClass(self, objectClass: ObjectClassHandle) -> None:
        self._call(
            self._implementation.unsubscribe_object_class,
            _encoded_handle(objectClass, ObjectClassHandle),
        )

    def unsubscribeObjectClassAttributes(
        self, objectClass: ObjectClassHandle, attributes: AttributeHandleSet
    ) -> None:
        self._call(
            self._implementation.unsubscribe_object_class_attributes,
            _encoded_handle(objectClass, ObjectClassHandle),
            _attribute_handle_bytes(attributes),
        )

    def unsubscribeObjectClassDirectedInteractions(
        self,
        objectClass: ObjectClassHandle,
        interactionClasses: InteractionClassHandleSet | None = None,
    ) -> None:
        arguments: tuple[object, ...] = (
            _encoded_handle(objectClass, ObjectClassHandle),
            None
            if interactionClasses is None
            else _interaction_class_handle_bytes(interactionClasses),
        )
        self._call(self._implementation.unsubscribe_object_class_directed_interactions, *arguments)

    def subscribeObjectClassAttributesWithRegions(
        self,
        objectClass: ObjectClassHandle,
        attributesAndRegions: AttributeSetRegionSetPairList,
        *,
        active: bool = True,
        updateRateDesignator: str = "",
    ) -> None:
        self._call(
            self._implementation.subscribe_object_class_attributes_with_regions,
            _encoded_handle(objectClass, ObjectClassHandle),
            _attribute_region_pairs(attributesAndRegions),
            bool(active),
            str(updateRateDesignator),
        )

    def subscribeObjectClassAttributesPassivelyWithRegions(
        self,
        objectClass: ObjectClassHandle,
        attributesAndRegions: AttributeSetRegionSetPairList | MutableAttributeSetRegionSetPairList,
        updateRateDesignator: str = "",
    ) -> None:
        self.subscribeObjectClassAttributesWithRegions(
            objectClass,
            attributesAndRegions,
            active=False,
            updateRateDesignator=updateRateDesignator,
        )

    def unsubscribeObjectClassAttributesWithRegions(
        self,
        objectClass: ObjectClassHandle,
        attributesAndRegions: AttributeSetRegionSetPairList,
    ) -> None:
        self._call(
            self._implementation.unsubscribe_object_class_attributes_with_regions,
            _encoded_handle(objectClass, ObjectClassHandle),
            _attribute_region_pairs(attributesAndRegions),
        )

    def publishInteractionClass(self, interactionClass: InteractionClassHandle) -> None:
        self._call(
            self._implementation.publish_interaction_class,
            _encoded_handle(interactionClass, InteractionClassHandle),
        )

    def unpublishInteractionClass(self, interactionClass: InteractionClassHandle) -> None:
        self._call(
            self._implementation.unpublish_interaction_class,
            _encoded_handle(interactionClass, InteractionClassHandle),
        )

    def subscribeInteractionClass(
        self, interactionClass: InteractionClassHandle, *, active: bool = True
    ) -> None:
        self._call(
            self._implementation.subscribe_interaction_class,
            _encoded_handle(interactionClass, InteractionClassHandle),
            bool(active),
        )

    def subscribeInteractionClassPassively(
        self, interactionClass: InteractionClassHandle
    ) -> None:
        self.subscribeInteractionClass(interactionClass, active=False)

    def unsubscribeInteractionClass(self, interactionClass: InteractionClassHandle) -> None:
        self._call(
            self._implementation.unsubscribe_interaction_class,
            _encoded_handle(interactionClass, InteractionClassHandle),
        )

    def reserveObjectInstanceName(self, objectInstanceName: str) -> None:
        self._call(self._implementation.reserve_object_instance_name, str(objectInstanceName))

    def releaseObjectInstanceName(self, objectInstanceName: str) -> None:
        self._call(self._implementation.release_object_instance_name, str(objectInstanceName))

    def reserveMultipleObjectInstanceNames(
        self, objectInstanceNames: ObjectInstanceNameSet
    ) -> None:
        self._call(
            self._implementation.reserve_multiple_object_instance_names,
            _object_instance_names(objectInstanceNames),
        )

    def releaseMultipleObjectInstanceNames(
        self, objectInstanceNames: ObjectInstanceNameSet
    ) -> None:
        self._call(
            self._implementation.release_multiple_object_instance_names,
            _object_instance_names(objectInstanceNames),
        )

    def registerObjectInstance(
        self,
        objectClass: ObjectClassHandle,
        *,
        objectInstanceName: str | None = None,
    ) -> ObjectInstanceHandle:
        return ObjectInstanceHandle(
            self._call(
                self._implementation.register_object_instance,
                _encoded_handle(objectClass, ObjectClassHandle),
                "" if objectInstanceName is None else str(objectInstanceName),
                objectInstanceName is not None,
            )
        )

    def registerObjectInstanceWithRegions(
        self,
        objectClass: ObjectClassHandle,
        attributesAndRegions: AttributeSetRegionSetPairList,
        *,
        objectInstanceName: str | None = None,
    ) -> ObjectInstanceHandle:
        return ObjectInstanceHandle(
            self._call(
                self._implementation.register_object_instance_with_regions,
                _encoded_handle(objectClass, ObjectClassHandle),
                _attribute_region_pairs(attributesAndRegions),
                "" if objectInstanceName is None else str(objectInstanceName),
                objectInstanceName is not None,
            )
        )

    def associateRegionsForUpdates(
        self,
        objectInstance: ObjectInstanceHandle,
        attributesAndRegions: AttributeSetRegionSetPairList,
    ) -> None:
        self._call(
            self._implementation.associate_regions_for_updates,
            _encoded_handle(objectInstance, ObjectInstanceHandle),
            _attribute_region_pairs(attributesAndRegions),
        )

    def unassociateRegionsForUpdates(
        self,
        objectInstance: ObjectInstanceHandle,
        attributesAndRegions: AttributeSetRegionSetPairList,
    ) -> None:
        self._call(
            self._implementation.unassociate_regions_for_updates,
            _encoded_handle(objectInstance, ObjectInstanceHandle),
            _attribute_region_pairs(attributesAndRegions),
        )

    def getObjectInstanceHandle(self, objectInstanceName: str) -> ObjectInstanceHandle:
        return ObjectInstanceHandle(
            self._call(self._implementation.get_object_instance_handle, str(objectInstanceName))
        )

    def getObjectInstanceName(self, objectInstance: ObjectInstanceHandle) -> str:
        return str(
            self._call(
                self._implementation.get_object_instance_name,
                _encoded_handle(objectInstance, ObjectInstanceHandle),
            )
        )

    def deleteObjectInstance(
        self, objectInstance: ObjectInstanceHandle, userSuppliedTag: BytesLike = b""
    ) -> None:
        self._call(
            self._implementation.delete_object_instance,
            _encoded_handle(objectInstance, ObjectInstanceHandle),
            bytes(userSuppliedTag),
        )

    def localDeleteObjectInstance(self, objectInstance: ObjectInstanceHandle) -> None:
        self._call(
            self._implementation.local_delete_object_instance,
            _encoded_handle(objectInstance, ObjectInstanceHandle),
        )

    def deleteObjectInstanceWithTime(
        self,
        objectInstance: ObjectInstanceHandle,
        time: LogicalTime,
        userSuppliedTag: BytesLike = b"",
    ) -> MessageRetractionHandle:
        if not isinstance(time, LogicalTime):
            raise TypeError("time must be LogicalTime")
        return MessageRetractionHandle(
            self._call(
                self._implementation.delete_object_instance_with_time,
                _encoded_handle(objectInstance, ObjectInstanceHandle),
                bytes(userSuppliedTag),
                time.toByteArray(),
            )
        )

    def updateAttributeValues(
        self,
        objectInstance: ObjectInstanceHandle,
        attributeValues: AttributeHandleValueMap,
        userSuppliedTag: BytesLike = b"",
    ) -> None:
        self._call(
            self._implementation.update_attribute_values,
            _encoded_handle(objectInstance, ObjectInstanceHandle),
            _attribute_value_pairs(attributeValues),
            bytes(userSuppliedTag),
        )

    def updateAttributeValuesWithTime(
        self,
        objectInstance: ObjectInstanceHandle,
        attributeValues: AttributeHandleValueMap,
        time: LogicalTime,
        userSuppliedTag: BytesLike = b"",
    ) -> MessageRetractionHandle:
        if not isinstance(time, LogicalTime):
            raise TypeError("time must be LogicalTime")
        return MessageRetractionHandle(
            self._call(
                self._implementation.update_attribute_values_with_time,
                _encoded_handle(objectInstance, ObjectInstanceHandle),
                _attribute_value_pairs(attributeValues),
                bytes(userSuppliedTag),
                time.toByteArray(),
            )
        )

    def requestAttributeValueUpdate(
        self,
        objectClassOrInstance: ObjectClassHandle | ObjectInstanceHandle,
        attributes: AttributeHandleSet,
        userSuppliedTag: BytesLike = b"",
    ) -> None:
        if isinstance(objectClassOrInstance, ObjectClassHandle):
            function = self._implementation.request_attribute_value_update_for_class
            encoded_handle = _encoded_handle(objectClassOrInstance, ObjectClassHandle)
        elif isinstance(objectClassOrInstance, ObjectInstanceHandle):
            function = self._implementation.request_attribute_value_update_for_instance
            encoded_handle = _encoded_handle(objectClassOrInstance, ObjectInstanceHandle)
        else:
            raise TypeError(
                "objectClassOrInstance must be ObjectClassHandle or ObjectInstanceHandle"
            )
        self._call(function, encoded_handle, _attribute_handle_bytes(attributes), bytes(userSuppliedTag))

    def requestAttributeValueUpdateWithRegions(
        self,
        objectClass: ObjectClassHandle,
        attributesAndRegions: AttributeSetRegionSetPairList,
        userSuppliedTag: BytesLike = b"",
    ) -> None:
        self._call(
            self._implementation.request_attribute_value_update_with_regions,
            _encoded_handle(objectClass, ObjectClassHandle),
            _attribute_region_pairs(attributesAndRegions),
            bytes(userSuppliedTag),
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
            self._implementation.change_attribute_order_type,
            _encoded_handle(objectInstance, ObjectInstanceHandle),
            _attribute_handle_bytes(attributes),
            orderType.name,
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
            self._implementation.change_default_attribute_order_type,
            _encoded_handle(objectClass, ObjectClassHandle),
            _attribute_handle_bytes(attributes),
            orderType.name,
        )

    def changeInteractionOrderType(
        self, interactionClass: InteractionClassHandle, orderType: OrderType
    ) -> None:
        if not isinstance(orderType, OrderType):
            raise TypeError("orderType must be OrderType")
        self._call(
            self._implementation.change_interaction_order_type,
            _encoded_handle(interactionClass, InteractionClassHandle),
            orderType.name,
        )

    def requestAttributeTransportationTypeChange(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
        transportationType: TransportationTypeHandle,
    ) -> None:
        self._call(
            self._implementation.request_attribute_transportation_type_change,
            _encoded_handle(objectInstance, ObjectInstanceHandle),
            _attribute_handle_bytes(attributes),
            _encoded_handle(transportationType, TransportationTypeHandle),
        )

    def changeDefaultAttributeTransportationType(
        self,
        objectClass: ObjectClassHandle,
        attributes: AttributeHandleSet,
        transportationType: TransportationTypeHandle,
    ) -> None:
        self._call(
            self._implementation.change_default_attribute_transportation_type,
            _encoded_handle(objectClass, ObjectClassHandle),
            _attribute_handle_bytes(attributes),
            _encoded_handle(transportationType, TransportationTypeHandle),
        )

    def queryAttributeTransportationType(
        self, objectInstance: ObjectInstanceHandle, attribute: AttributeHandle
    ) -> None:
        self._call(
            self._implementation.query_attribute_transportation_type,
            _encoded_handle(objectInstance, ObjectInstanceHandle),
            _encoded_handle(attribute, AttributeHandle),
        )

    def requestInteractionTransportationTypeChange(
        self,
        interactionClass: InteractionClassHandle,
        transportationType: TransportationTypeHandle,
    ) -> None:
        self._call(
            self._implementation.request_interaction_transportation_type_change,
            _encoded_handle(interactionClass, InteractionClassHandle),
            _encoded_handle(transportationType, TransportationTypeHandle),
        )

    def queryInteractionTransportationType(
        self, federate: FederateHandle, interactionClass: InteractionClassHandle
    ) -> None:
        self._call(
            self._implementation.query_interaction_transportation_type,
            _encoded_handle(federate, FederateHandle),
            _encoded_handle(interactionClass, InteractionClassHandle),
        )

    def queryAttributeOwnership(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
    ) -> None:
        self._call(
            self._implementation.query_attribute_ownership,
            _encoded_handle(objectInstance, ObjectInstanceHandle),
            _attribute_handle_bytes(attributes),
        )

    def isAttributeOwnedByFederate(
        self,
        objectInstance: ObjectInstanceHandle,
        attribute: AttributeHandle,
    ) -> bool:
        return bool(
            self._call(
                self._implementation.is_attribute_owned_by_federate,
                _encoded_handle(objectInstance, ObjectInstanceHandle),
                _encoded_handle(attribute, AttributeHandle),
            )
        )

    def unconditionalAttributeOwnershipDivestiture(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
        userSuppliedTag: BytesLike = b"",
    ) -> None:
        self._call(
            self._implementation.unconditional_attribute_ownership_divestiture,
            _encoded_handle(objectInstance, ObjectInstanceHandle),
            _attribute_handle_bytes(attributes),
            bytes(userSuppliedTag),
        )

    def negotiatedAttributeOwnershipDivestiture(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
        userSuppliedTag: BytesLike = b"",
    ) -> None:
        self._call(
            self._implementation.negotiated_attribute_ownership_divestiture,
            _encoded_handle(objectInstance, ObjectInstanceHandle),
            _attribute_handle_bytes(attributes),
            bytes(userSuppliedTag),
        )

    def confirmDivestiture(
        self,
        objectInstance: ObjectInstanceHandle,
        confirmedAttributes: AttributeHandleSet,
        userSuppliedTag: BytesLike = b"",
    ) -> None:
        self._call(
            self._implementation.confirm_divestiture,
            _encoded_handle(objectInstance, ObjectInstanceHandle),
            _attribute_handle_bytes(confirmedAttributes),
            bytes(userSuppliedTag),
        )

    def cancelNegotiatedAttributeOwnershipDivestiture(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
    ) -> None:
        self._call(
            self._implementation.cancel_negotiated_attribute_ownership_divestiture,
            _encoded_handle(objectInstance, ObjectInstanceHandle),
            _attribute_handle_bytes(attributes),
        )

    def attributeOwnershipAcquisition(
        self,
        objectInstance: ObjectInstanceHandle,
        desiredAttributes: AttributeHandleSet,
        userSuppliedTag: BytesLike = b"",
    ) -> None:
        self._call(
            self._implementation.attribute_ownership_acquisition,
            _encoded_handle(objectInstance, ObjectInstanceHandle),
            _attribute_handle_bytes(desiredAttributes),
            bytes(userSuppliedTag),
        )

    def attributeOwnershipAcquisitionIfAvailable(
        self,
        objectInstance: ObjectInstanceHandle,
        desiredAttributes: AttributeHandleSet,
        userSuppliedTag: BytesLike = b"",
    ) -> None:
        self._call(
            self._implementation.attribute_ownership_acquisition_if_available,
            _encoded_handle(objectInstance, ObjectInstanceHandle),
            _attribute_handle_bytes(desiredAttributes),
            bytes(userSuppliedTag),
        )

    def cancelAttributeOwnershipAcquisition(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
    ) -> None:
        self._call(
            self._implementation.cancel_attribute_ownership_acquisition,
            _encoded_handle(objectInstance, ObjectInstanceHandle),
            _attribute_handle_bytes(attributes),
        )

    def attributeOwnershipReleaseDenied(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
        userSuppliedTag: BytesLike = b"",
    ) -> None:
        self._call(
            self._implementation.attribute_ownership_release_denied,
            _encoded_handle(objectInstance, ObjectInstanceHandle),
            _attribute_handle_bytes(attributes),
            bytes(userSuppliedTag),
        )

    def attributeOwnershipDivestitureIfWanted(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
        userSuppliedTag: BytesLike = b"",
    ) -> AttributeHandleSet:
        return AttributeHandleSet(
            AttributeHandle(encoded)
            for encoded in self._call(
                self._implementation.attribute_ownership_divestiture_if_wanted,
                _encoded_handle(objectInstance, ObjectInstanceHandle),
                _attribute_handle_bytes(attributes),
                bytes(userSuppliedTag),
            )
        )

    def sendInteraction(
        self,
        interactionClass: InteractionClassHandle,
        parameterValues: ParameterHandleValueMap,
        userSuppliedTag: BytesLike = b"",
    ) -> None:
        self._call(
            self._implementation.send_interaction,
            _encoded_handle(interactionClass, InteractionClassHandle),
            _parameter_value_pairs(parameterValues),
            bytes(userSuppliedTag),
        )

    def sendInteractionWithTime(
        self,
        interactionClass: InteractionClassHandle,
        parameterValues: ParameterHandleValueMap,
        time: LogicalTime,
        userSuppliedTag: BytesLike = b"",
    ) -> MessageRetractionHandle:
        if not isinstance(time, LogicalTime):
            raise TypeError("time must be LogicalTime")
        return MessageRetractionHandle(
            self._call(
                self._implementation.send_interaction_with_time,
                _encoded_handle(interactionClass, InteractionClassHandle),
                _parameter_value_pairs(parameterValues),
                bytes(userSuppliedTag),
                time.toByteArray(),
            )
        )

    def sendDirectedInteraction(
        self,
        interactionClass: InteractionClassHandle,
        objectInstance: ObjectInstanceHandle,
        parameterValues: ParameterHandleValueMap,
        userSuppliedTag: BytesLike = b"",
    ) -> None:
        self._call(
            self._implementation.send_directed_interaction,
            _encoded_handle(interactionClass, InteractionClassHandle),
            _encoded_handle(objectInstance, ObjectInstanceHandle),
            _parameter_value_pairs(parameterValues),
            bytes(userSuppliedTag),
        )

    def sendDirectedInteractionWithTime(
        self,
        interactionClass: InteractionClassHandle,
        objectInstance: ObjectInstanceHandle,
        parameterValues: ParameterHandleValueMap,
        time: LogicalTime,
        userSuppliedTag: BytesLike = b"",
    ) -> MessageRetractionHandle:
        if not isinstance(time, LogicalTime):
            raise TypeError("time must be LogicalTime")
        return MessageRetractionHandle(
            self._call(
                self._implementation.send_directed_interaction_with_time,
                _encoded_handle(interactionClass, InteractionClassHandle),
                _encoded_handle(objectInstance, ObjectInstanceHandle),
                _parameter_value_pairs(parameterValues),
                bytes(userSuppliedTag),
                time.toByteArray(),
            )
        )

    def subscribeInteractionClassWithRegions(
        self,
        interactionClass: InteractionClassHandle,
        regions: RegionHandleSet,
        *,
        active: bool = True,
    ) -> None:
        self._call(
            self._implementation.subscribe_interaction_class_with_regions,
            _encoded_handle(interactionClass, InteractionClassHandle),
            _region_handle_bytes(regions),
            bool(active),
        )

    def subscribeInteractionClassPassivelyWithRegions(
        self, interactionClass: InteractionClassHandle, regions: RegionHandleSet
    ) -> None:
        self.subscribeInteractionClassWithRegions(interactionClass, regions, active=False)

    def unsubscribeInteractionClassWithRegions(
        self, interactionClass: InteractionClassHandle, regions: RegionHandleSet
    ) -> None:
        self._call(
            self._implementation.unsubscribe_interaction_class_with_regions,
            _encoded_handle(interactionClass, InteractionClassHandle),
            _region_handle_bytes(regions),
        )

    def sendInteractionWithRegions(
        self,
        interactionClass: InteractionClassHandle,
        parameterValues: ParameterHandleValueMap,
        regions: RegionHandleSet,
        userSuppliedTag: BytesLike = b"",
    ) -> None:
        self._call(
            self._implementation.send_interaction_with_regions,
            _encoded_handle(interactionClass, InteractionClassHandle),
            _parameter_value_pairs(parameterValues),
            _region_handle_bytes(regions),
            bytes(userSuppliedTag),
        )

    def sendInteractionWithRegionsWithTime(
        self,
        interactionClass: InteractionClassHandle,
        parameterValues: ParameterHandleValueMap,
        regions: RegionHandleSet,
        time: LogicalTime,
        userSuppliedTag: BytesLike = b"",
    ) -> MessageRetractionHandle:
        if not isinstance(time, LogicalTime):
            raise TypeError("time must be LogicalTime")
        return MessageRetractionHandle(
            self._call(
                self._implementation.send_interaction_with_regions_with_time,
                _encoded_handle(interactionClass, InteractionClassHandle),
                _parameter_value_pairs(parameterValues),
                _region_handle_bytes(regions),
                bytes(userSuppliedTag),
                time.toByteArray(),
            )
        )

    def retract(self, retraction: MessageRetractionHandle) -> None:
        self._call(
            self._implementation.retract,
            _encoded_handle(retraction, MessageRetractionHandle),
        )

    def getFederateHandleFactory(self) -> FederateHandleFactory:
        return _NativeFederateHandleFactory(
            self._implementation, "decode_federate_handle", FederateHandle
        )

    def getFederateHandleSetFactory(self) -> FederateHandleSetFactory:
        return _NativeFederateHandleSetFactory()

    def getObjectClassHandleFactory(self) -> ObjectClassHandleFactory:
        return _NativeObjectClassHandleFactory(
            self._implementation, "decode_object_class_handle", ObjectClassHandle
        )

    def getObjectInstanceHandleFactory(self) -> ObjectInstanceHandleFactory:
        return _NativeObjectInstanceHandleFactory(
            self._implementation,
            "decode_object_instance_handle",
            ObjectInstanceHandle,
        )

    def getAttributeHandleFactory(self) -> AttributeHandleFactory:
        return _NativeAttributeHandleFactory(
            self._implementation, "decode_attribute_handle", AttributeHandle
        )

    def getInteractionClassHandleFactory(self) -> InteractionClassHandleFactory:
        return _NativeInteractionClassHandleFactory(
            self._implementation,
            "decode_interaction_class_handle",
            InteractionClassHandle,
        )

    def getInteractionClassHandleSetFactory(self) -> InteractionClassHandleSetFactory:
        return _NativeInteractionClassHandleSetFactory()

    def getParameterHandleFactory(self) -> ParameterHandleFactory:
        return _NativeParameterHandleFactory(
            self._implementation, "decode_parameter_handle", ParameterHandle
        )

    def getTransportationTypeHandleFactory(self) -> TransportationTypeHandleFactory:
        return _NativeTransportationTypeHandleFactory(
            self._implementation,
            "decode_transportation_type_handle",
            TransportationTypeHandle,
        )

    def getDimensionHandleFactory(self) -> DimensionHandleFactory:
        return _NativeDimensionHandleFactory(
            self._implementation, "decode_dimension_handle", DimensionHandle
        )

    def getRegionHandleFactory(self) -> RegionHandleFactory:
        return _NativeRegionHandleFactory(
            self._implementation, "decode_region_handle", RegionHandle
        )

    def getMessageRetractionHandleFactory(self) -> MessageRetractionHandleFactory:
        return _NativeMessageRetractionHandleFactory(
            self._implementation,
            "decode_message_retraction_handle",
            MessageRetractionHandle,
        )

    def getDimensionHandleSetFactory(self) -> DimensionHandleSetFactory:
        return _NativeDimensionHandleSetFactory()

    def getRegionHandleSetFactory(self) -> RegionHandleSetFactory:
        return _NativeRegionHandleSetFactory()

    def getAttributeHandleSetFactory(self) -> AttributeHandleSetFactory:
        return _NativeAttributeHandleSetFactory()

    def getAttributeHandleValueMapFactory(self) -> AttributeHandleValueMapFactory:
        return _NativeAttributeHandleValueMapFactory()

    def getParameterHandleValueMapFactory(self) -> ParameterHandleValueMapFactory:
        return _NativeParameterHandleValueMapFactory()

    def getAttributeSetRegionSetPairListFactory(self) -> AttributeSetRegionSetPairListFactory:
        return _NativeAttributeSetRegionSetPairListFactory()

    def createRegion(self, dimensions: DimensionHandleSet) -> RegionHandle:
        return RegionHandle(
            self._call(self._implementation.create_region, _dimension_handle_bytes(dimensions))
        )

    def commitRegionModifications(self, regions: RegionHandleSet) -> None:
        self._call(
            self._implementation.commit_region_modifications,
            _region_handle_bytes(regions),
        )

    def deleteRegion(self, region: RegionHandle) -> None:
        self._call(
            self._implementation.delete_region,
            _encoded_handle(region, RegionHandle),
        )

    def getDimensionHandleSet(self, region: RegionHandle) -> DimensionHandleSet:
        return DimensionHandleSet(
            DimensionHandle(encoded)
            for encoded in self._call(
                self._implementation.get_dimension_handle_set,
                _encoded_handle(region, RegionHandle),
            )
        )

    def getRangeBounds(self, region: RegionHandle, dimension: DimensionHandle) -> RangeBounds:
        record = self._call(
            self._implementation.get_range_bounds,
            _encoded_handle(region, RegionHandle),
            _encoded_handle(dimension, DimensionHandle),
        )
        values = dict(record)
        return RangeBounds(int(values["lowerBound"]), int(values["upperBound"]))

    def setRangeBounds(
        self,
        region: RegionHandle,
        dimension: DimensionHandle,
        rangeBounds: RangeBounds,
    ) -> None:
        if not isinstance(rangeBounds, RangeBounds):
            raise TypeError("rangeBounds must be RangeBounds")
        self._call(
            self._implementation.set_range_bounds,
            _encoded_handle(region, RegionHandle),
            _encoded_handle(dimension, DimensionHandle),
            int(rangeBounds.getLowerBound()),
            int(rangeBounds.getUpperBound()),
        )

    def getConveyRegionDesignatorSetsSwitch(self) -> bool:
        return bool(
            self._call(self._implementation.get_convey_region_designator_sets_switch)
        )

    def setConveyRegionDesignatorSetsSwitch(self, switchValue: bool) -> None:
        self._call(
            self._implementation.set_convey_region_designator_sets_switch,
            bool(switchValue),
        )

    def getObjectClassRelevanceAdvisorySwitch(self) -> bool:
        return bool(
            self._call(self._implementation.get_object_class_relevance_advisory_switch)
        )

    def setObjectClassRelevanceAdvisorySwitch(self, switchValue: bool) -> None:
        self._call(
            self._implementation.set_object_class_relevance_advisory_switch,
            bool(switchValue),
        )

    def getAttributeRelevanceAdvisorySwitch(self) -> bool:
        return bool(
            self._call(self._implementation.get_attribute_relevance_advisory_switch)
        )

    def setAttributeRelevanceAdvisorySwitch(self, switchValue: bool) -> None:
        self._call(
            self._implementation.set_attribute_relevance_advisory_switch,
            bool(switchValue),
        )

    def getAttributeScopeAdvisorySwitch(self) -> bool:
        return bool(
            self._call(self._implementation.get_attribute_scope_advisory_switch)
        )

    def setAttributeScopeAdvisorySwitch(self, switchValue: bool) -> None:
        self._call(
            self._implementation.set_attribute_scope_advisory_switch,
            bool(switchValue),
        )

    def getInteractionRelevanceAdvisorySwitch(self) -> bool:
        return bool(
            self._call(self._implementation.get_interaction_relevance_advisory_switch)
        )

    def setInteractionRelevanceAdvisorySwitch(self, switchValue: bool) -> None:
        self._call(
            self._implementation.set_interaction_relevance_advisory_switch,
            bool(switchValue),
        )

    def getAutomaticResignDirective(self) -> ResignAction:
        try:
            return ResignAction[str(self._call(self._implementation.get_automatic_resign_directive))]
        except KeyError as error:
            raise RTIinternalError("Native RTI returned an unknown ResignAction") from error

    def setAutomaticResignDirective(self, resignAction: ResignAction) -> None:
        if not isinstance(resignAction, ResignAction):
            raise TypeError("resignAction must be ResignAction")
        self._call(
            self._implementation.set_automatic_resign_directive,
            resignAction.name,
        )

    def getServiceReportingSwitch(self) -> bool:
        return bool(self._call(self._implementation.get_service_reporting_switch))

    def setServiceReportingSwitch(self, switchValue: bool) -> None:
        self._call(self._implementation.set_service_reporting_switch, bool(switchValue))

    def getExceptionReportingSwitch(self) -> bool:
        return bool(self._call(self._implementation.get_exception_reporting_switch))

    def setExceptionReportingSwitch(self, switchValue: bool) -> None:
        self._call(self._implementation.set_exception_reporting_switch, bool(switchValue))

    def getSendServiceReportsToFileSwitch(self) -> bool:
        return bool(self._call(self._implementation.get_send_service_reports_to_file_switch))

    def setSendServiceReportsToFileSwitch(self, switchValue: bool) -> None:
        self._call(
            self._implementation.set_send_service_reports_to_file_switch,
            bool(switchValue),
        )

    def getAutoProvideSwitch(self) -> bool:
        return bool(self._call(self._implementation.get_auto_provide_switch))

    def getDelaySubscriptionEvaluationSwitch(self) -> bool:
        return bool(self._call(self._implementation.get_delay_subscription_evaluation_switch))

    def getAdvisoriesUseKnownClassSwitch(self) -> bool:
        return bool(self._call(self._implementation.get_advisories_use_known_class_switch))

    def getAllowRelaxedDDMSwitch(self) -> bool:
        return bool(self._call(self._implementation.get_allow_relaxed_ddm_switch))

    def getNonRegulatedGrantSwitch(self) -> bool:
        return bool(self._call(self._implementation.get_non_regulated_grant_switch))

    def getTimeFactory(self) -> LogicalTimeFactory:
        implementation_name = str(
            self._call(self._implementation.time_factory_name)
        )
        if implementation_name == "HLAinteger64Time":
            return _NativeInteger64TimeFactory(self._implementation)
        if implementation_name == "HLAfloat64Time":
            return _NativeFloat64TimeFactory(self._implementation)
        raise RTIinternalError(f"Unsupported logical-time implementation: {implementation_name}")

    def enableTimeRegulation(self, lookahead: LogicalTimeInterval) -> None:
        if not isinstance(lookahead, LogicalTimeInterval):
            raise TypeError("lookahead must be LogicalTimeInterval")
        self._call(self._implementation.enable_time_regulation, lookahead.encodedValue)

    def disableTimeRegulation(self) -> None:
        self._call(self._implementation.disable_time_regulation)

    def enableTimeConstrained(self) -> None:
        self._call(self._implementation.enable_time_constrained)

    def disableTimeConstrained(self) -> None:
        self._call(self._implementation.disable_time_constrained)

    def enableAsynchronousDelivery(self) -> None:
        self._call(self._implementation.enable_asynchronous_delivery)

    def disableAsynchronousDelivery(self) -> None:
        self._call(self._implementation.disable_asynchronous_delivery)

    def modifyLookahead(self, lookahead: LogicalTimeInterval) -> None:
        if not isinstance(lookahead, LogicalTimeInterval):
            raise TypeError("lookahead must be LogicalTimeInterval")
        self._call(self._implementation.modify_lookahead, lookahead.encodedValue)

    def queryLookahead(self) -> LogicalTimeInterval:
        return _logical_time(
            self._call(self._implementation.query_lookahead), interval=True
        )  # type: ignore[return-value]

    def timeAdvanceRequest(self, time: LogicalTime) -> None:
        if not isinstance(time, LogicalTime):
            raise TypeError("time must be LogicalTime")
        self._call(self._implementation.time_advance_request, time.encodedValue)

    def timeAdvanceRequestAvailable(self, time: LogicalTime) -> None:
        if not isinstance(time, LogicalTime):
            raise TypeError("time must be LogicalTime")
        self._call(self._implementation.time_advance_request_available, time.encodedValue)

    def nextMessageRequest(self, time: LogicalTime) -> None:
        if not isinstance(time, LogicalTime):
            raise TypeError("time must be LogicalTime")
        self._call(self._implementation.next_message_request, time.encodedValue)

    def nextMessageRequestAvailable(self, time: LogicalTime) -> None:
        if not isinstance(time, LogicalTime):
            raise TypeError("time must be LogicalTime")
        self._call(self._implementation.next_message_request_available, time.encodedValue)

    def flushQueueRequest(self, time: LogicalTime) -> None:
        if not isinstance(time, LogicalTime):
            raise TypeError("time must be LogicalTime")
        self._call(self._implementation.flush_queue_request, time.encodedValue)

    def queryLogicalTime(self) -> LogicalTime:
        return _logical_time(self._call(self._implementation.query_logical_time))  # type: ignore[return-value]

    def queryGALT(self) -> TimeQueryResult:
        return _time_query(self._call(self._implementation.query_galt))

    def queryLITS(self) -> TimeQueryResult:
        return _time_query(self._call(self._implementation.query_lits))

    def getObjectClassHandle(self, objectClassName: str) -> ObjectClassHandle:
        return ObjectClassHandle(
            self._call(self._implementation.get_object_class_handle, str(objectClassName))
        )

    def getObjectClassName(self, objectClass: ObjectClassHandle) -> str:
        return str(
            self._call(
                self._implementation.get_object_class_name,
                _encoded_handle(objectClass, ObjectClassHandle),
            )
        )

    def getFederateHandle(self, federateName: str) -> FederateHandle:
        return FederateHandle(
            self._call(self._implementation.get_federate_handle, str(federateName))
        )

    def getFederateName(self, federate: FederateHandle) -> str:
        return str(
            self._call(
                self._implementation.get_federate_name,
                _encoded_handle(federate, FederateHandle),
            )
        )

    def getKnownObjectClassHandle(
        self, objectInstance: ObjectInstanceHandle
    ) -> ObjectClassHandle:
        return ObjectClassHandle(
            self._call(
                self._implementation.get_known_object_class_handle,
                _encoded_handle(objectInstance, ObjectInstanceHandle),
            )
        )

    def getAttributeHandle(
        self, objectClass: ObjectClassHandle, attributeName: str
    ) -> AttributeHandle:
        return AttributeHandle(
            self._call(
                self._implementation.get_attribute_handle,
                _encoded_handle(objectClass, ObjectClassHandle),
                str(attributeName),
            )
        )

    def getAttributeName(
        self, objectClass: ObjectClassHandle, attribute: AttributeHandle
    ) -> str:
        return str(
            self._call(
                self._implementation.get_attribute_name,
                _encoded_handle(objectClass, ObjectClassHandle),
                _encoded_handle(attribute, AttributeHandle),
            )
        )

    def getUpdateRateValue(self, updateRateDesignator: str) -> float:
        return float(
            self._call(
                self._implementation.get_update_rate_value,
                str(updateRateDesignator),
            )
        )

    def getUpdateRateValueForAttribute(
        self, objectInstance: ObjectInstanceHandle, attribute: AttributeHandle
    ) -> float:
        return float(
            self._call(
                self._implementation.get_update_rate_value_for_attribute,
                _encoded_handle(objectInstance, ObjectInstanceHandle),
                _encoded_handle(attribute, AttributeHandle),
            )
        )

    def getInteractionClassHandle(self, interactionClassName: str) -> InteractionClassHandle:
        return InteractionClassHandle(
            self._call(self._implementation.get_interaction_class_handle, str(interactionClassName))
        )

    def getInteractionClassName(self, interactionClass: InteractionClassHandle) -> str:
        return str(
            self._call(
                self._implementation.get_interaction_class_name,
                _encoded_handle(interactionClass, InteractionClassHandle),
            )
        )

    def getParameterHandle(
        self, interactionClass: InteractionClassHandle, parameterName: str
    ) -> ParameterHandle:
        return ParameterHandle(
            self._call(
                self._implementation.get_parameter_handle,
                _encoded_handle(interactionClass, InteractionClassHandle),
                str(parameterName),
            )
        )

    def getParameterName(
        self, interactionClass: InteractionClassHandle, parameter: ParameterHandle
    ) -> str:
        return str(
            self._call(
                self._implementation.get_parameter_name,
                _encoded_handle(interactionClass, InteractionClassHandle),
                _encoded_handle(parameter, ParameterHandle),
            )
        )

    def getOrderType(self, orderTypeName: str) -> OrderType:
        value = str(self._call(self._implementation.get_order_type, str(orderTypeName)))
        if value == "Receive":
            return OrderType.RECEIVE
        if value == "TimeStamp":
            return OrderType.TIMESTAMP
        raise RTIinternalError(f"Native RTI returned an unknown OrderType name: {value}")

    def getOrderName(self, orderType: OrderType) -> str:
        if not isinstance(orderType, OrderType):
            raise TypeError("orderType must be OrderType")
        return str(self._call(self._implementation.get_order_name, orderType.name))

    def getTransportationTypeHandle(
        self, transportationTypeName: str
    ) -> TransportationTypeHandle:
        return TransportationTypeHandle(
            self._call(
                self._implementation.get_transportation_type_handle,
                str(transportationTypeName),
            )
        )

    def getTransportationTypeName(self, transportationType: TransportationTypeHandle) -> str:
        return str(
            self._call(
                self._implementation.get_transportation_type_name,
                _encoded_handle(transportationType, TransportationTypeHandle),
            )
        )

    def getDimensionHandle(self, dimensionName: str) -> DimensionHandle:
        return DimensionHandle(
            self._call(self._implementation.get_dimension_handle, str(dimensionName))
        )

    def getDimensionName(self, dimension: DimensionHandle) -> str:
        return str(
            self._call(
                self._implementation.get_dimension_name,
                _encoded_handle(dimension, DimensionHandle),
            )
        )

    def getAvailableDimensionsForObjectClass(
        self, objectClass: ObjectClassHandle
    ) -> DimensionHandleSet:
        return DimensionHandleSet(
            DimensionHandle(value)
            for value in self._call(
                self._implementation.get_available_dimensions_for_object_class,
                _encoded_handle(objectClass, ObjectClassHandle),
            )
        )

    def getAvailableDimensionsForInteractionClass(
        self, interactionClass: InteractionClassHandle
    ) -> DimensionHandleSet:
        return DimensionHandleSet(
            DimensionHandle(value)
            for value in self._call(
                self._implementation.get_available_dimensions_for_interaction_class,
                _encoded_handle(interactionClass, InteractionClassHandle),
            )
        )

    def getDimensionUpperBound(self, dimension: DimensionHandle) -> int:
        return int(
            self._call(
                self._implementation.get_dimension_upper_bound,
                _encoded_handle(dimension, DimensionHandle),
            )
        )

    def normalizeServiceGroup(self, serviceGroup: ServiceGroup) -> int:
        if not isinstance(serviceGroup, ServiceGroup):
            raise TypeError("serviceGroup must be a ServiceGroup")
        return int(self._call(self._implementation.normalize_service_group, serviceGroup.name))

    def normalizeFederateHandle(self, federate: FederateHandle) -> int:
        return int(
            self._call(
                self._implementation.normalize_federate_handle,
                _encoded_handle(federate, FederateHandle),
            )
        )

    def normalizeObjectClassHandle(self, objectClass: ObjectClassHandle) -> int:
        return int(
            self._call(
                self._implementation.normalize_object_class_handle,
                _encoded_handle(objectClass, ObjectClassHandle),
            )
        )

    def normalizeInteractionClassHandle(
        self, interactionClass: InteractionClassHandle
    ) -> int:
        return int(
            self._call(
                self._implementation.normalize_interaction_class_handle,
                _encoded_handle(interactionClass, InteractionClassHandle),
            )
        )

    def normalizeObjectInstanceHandle(self, objectInstance: ObjectInstanceHandle) -> int:
        return int(
            self._call(
                self._implementation.normalize_object_instance_handle,
                _encoded_handle(objectInstance, ObjectInstanceHandle),
            )
        )

    def createFederationExecution(
        self,
        federationName: str,
        fomModule: str | Iterable[str],
        logicalTimeImplementationName: str = "",
    ) -> None:
        modules = _fom_modules(fomModule)
        if isinstance(fomModule, str):
            self._call(
                self._implementation.create_federation_execution,
                federationName,
                fomModule,
                logicalTimeImplementationName,
            )
        else:
            self._call(
                self._implementation.create_federation_execution_with_modules,
                federationName,
                modules,
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
            self._implementation.create_federation_execution_with_mim,
            federationName,
            tuple(str(module) for module in fomModules),
            str(mimModule),
            logicalTimeImplementationName,
        )

    def destroyFederationExecution(self, federationName: str) -> None:
        self._call(self._implementation.destroy_federation_execution, federationName)

    @staticmethod
    def _call(function: Callable[..., _Result], *args: object) -> _Result:
        try:
            return function(*args)
        except _native.NativeRtiError as error:
            name, separator, message = str(error).partition(": ")
            raise exceptionForName(name, message if separator else name) from error


class UmbraRtiFactory(RtiFactory):
    """The discovered Umbra provider for ``hla.rti1516_2025``."""

    def getRtiAmbassador(self) -> RTIambassador:
        try:
            return _UmbraRTIambassador(_native.NativeAmbassador())
        except _native.NativeRtiError as error:
            name, separator, message = str(error).partition(": ")
            raise exceptionForName(name, message if separator else name) from error

    def getEncoderFactory(self) -> EncoderFactory:
        return _NativeEncoderFactory()

    def rtiName(self) -> str:
        return _native.rti_name()

    def rtiVersion(self) -> str:
        return _native.rti_version()
