"""Lazy JPype mechanics for the standard IEEE 1516e Java package."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from threading import Lock
from typing import Any

from hla.rti1516e import (
    AttributeHandle,
    AttributeHandleSet,
    AttributeHandleValueMap,
    CallbackModel,
    DimensionHandle,
    DimensionHandleSet,
    FederateAmbassador,
    FederateHandle,
    FederateHandleSaveStatusPair,
    FederateHandleSet,
    FederateRestoreStatus,
    FederationExecutionInformation,
    FederationExecutionInformationSet,
    HLAfloat64Time,
    HLAinteger64Time,
    InteractionClassHandle,
    MessageRetractionHandle,
    MessageRetractionReturn,
    ObjectClassHandle,
    ObjectInstanceHandle,
    OrderType,
    ParameterHandle,
    ParameterHandleValueMap,
    RangeBounds,
    RegionHandle,
    RegionHandleSet,
    ResignAction,
    RestoreFailureReason,
    RestoreStatus,
    SaveFailureReason,
    SaveStatus,
    ServiceGroup,
    SupplementalReceiveInfo,
    SupplementalReflectInfo,
    SupplementalRemoveInfo,
    SynchronizationPointFailureReason,
    TimeQueryReturn,
    TransportationTypeHandle,
)
from hla.rti1516e.contracts import FEDERATE_AMBASSADOR_PARAMETER_TYPES
from hla.rti1516e.exceptions import RTIinternalError

from .config import Java2010ProviderConfiguration

_JVM_LOCK = Lock()
_STARTED_CONFIGURATION: Java2010ProviderConfiguration | None = None


@dataclass(frozen=True, slots=True)
class Java2010CallbackBinding:
    proxy: object
    target: object


_JAVA_HANDLE_TYPES: dict[str, type[object]] = {
    "FederateHandle": FederateHandle,
    "ObjectClassHandle": ObjectClassHandle,
    "ObjectInstanceHandle": ObjectInstanceHandle,
    "AttributeHandle": AttributeHandle,
    "InteractionClassHandle": InteractionClassHandle,
    "ParameterHandle": ParameterHandle,
    "TransportationTypeHandle": TransportationTypeHandle,
    "DimensionHandle": DimensionHandle,
    "RegionHandle": RegionHandle,
    "MessageRetractionHandle": MessageRetractionHandle,
}

_JAVA_ENUM_TYPES: dict[str, type[object]] = {
    "ResignAction": ResignAction,
    "OrderType": OrderType,
    "ServiceGroup": ServiceGroup,
    "SynchronizationPointFailureReason": SynchronizationPointFailureReason,
    "SaveFailureReason": SaveFailureReason,
    "SaveStatus": SaveStatus,
    "RestoreFailureReason": RestoreFailureReason,
    "RestoreStatus": RestoreStatus,
}


class _FederateAmbassadorProxy:
    """Forward every standard callback name without inventing Java methods."""

    def __init__(self, target: FederateAmbassador, runtime: JPype2010Runtime) -> None:
        self._target = target
        self._runtime = runtime

    def __getattr__(self, name: str):
        callback = getattr(self._target, name)

        def invoke(*args: object) -> object:
            overloads = FEDERATE_AMBASSADOR_PARAMETER_TYPES.get(name, ())
            expected = next(
                (candidate for candidate in overloads if len(candidate) == len(args)),
                None,
            )
            converted = tuple(
                self._runtime.from_java_value(
                    value, expected[index] if expected else None
                )
                for index, value in enumerate(args)
            )
            return callback(*converted)

        return invoke


class JPype2010Runtime:
    """Lazy production runtime; constructing it never starts a JVM."""

    def __init__(self) -> None:
        self._jpype: Any | None = None

    def get_rti_factory(self, configuration: Java2010ProviderConfiguration) -> object:
        jpype = self._ensure_jvm(configuration)
        factory_factory = jpype.JClass("hla.rti1516e.RtiFactoryFactory")
        try:
            if configuration.rti_factory_name is not None:
                return factory_factory.getRtiFactory(configuration.rti_factory_name)
            return factory_factory.getRtiFactory()
        except Exception as error:
            # The published 2010 helper calls javax.imageio.spi.ServiceRegistry,
            # whose modern JDK implementation rejects an HLA RtiFactory as a
            # non-ImageIO SPI category. Fall back only for that compatibility
            # defect; provider selection still uses the standard Java service
            # descriptor and the standard RtiFactory interface.
            if "not an ImageIO SPI class" not in str(error):
                raise RTIinternalError(
                    f"Java 1516e RtiFactoryFactory failed: {error}"
                ) from error
            try:
                service_loader = jpype.JClass("java.util.ServiceLoader")
                interface = jpype.JClass("hla.rti1516e.RtiFactory")
                iterator = service_loader.load(interface).iterator()
                while iterator.hasNext():
                    candidate = iterator.next()
                    if (
                        configuration.rti_factory_name is None
                        or str(candidate.rtiName()) == configuration.rti_factory_name
                    ):
                        return candidate
            except Exception as fallback_error:
                raise RTIinternalError(
                    f"Java 1516e factory compatibility fallback failed: {fallback_error}"
                ) from fallback_error
            raise RTIinternalError("Cannot find 2010 Java RtiFactory") from error

    def callback_model(self, callback_model: CallbackModel) -> object:
        if not isinstance(callback_model, CallbackModel):
            raise TypeError("callback_model must be hla.rti1516e.CallbackModel")
        return getattr(
            self._require_started_jvm().JClass("hla.rti1516e.CallbackModel"),
            callback_model.name,
        )

    def bind_federate_ambassador(
        self,
        federate_ambassador: FederateAmbassador,
    ) -> Java2010CallbackBinding:
        jpype = self._require_started_jvm()
        target = _FederateAmbassadorProxy(federate_ambassador, self)
        interface = jpype.JClass("hla.rti1516e.FederateAmbassador")
        return Java2010CallbackBinding(
            proxy=jpype.JProxy(interface, inst=target), target=target
        )

    def byte_array(self, value: bytes | bytearray | memoryview) -> object:
        jpype = self._require_started_jvm()
        return jpype.JArray(jpype.JByte)(bytes(value))

    def java_byte(self, value: object) -> object:
        jpype = self._require_started_jvm()
        return jpype.JByte(int(value))

    def java_short(self, value: object) -> object:
        jpype = self._require_started_jvm()
        return jpype.JShort(int(value))

    def java_int(self, value: object) -> object:
        jpype = self._require_started_jvm()
        return jpype.JInt(int(value))

    def java_long(self, value: object) -> object:
        jpype = self._require_started_jvm()
        return jpype.JLong(int(value))

    def java_float(self, value: object) -> object:
        jpype = self._require_started_jvm()
        return jpype.JFloat(float(value))

    def java_double(self, value: object) -> object:
        jpype = self._require_started_jvm()
        return jpype.JDouble(float(value))

    def java_byte_wrapper(self, value: object) -> object:
        """Convert the provider-neutral ByteWrapper to the exact Java helper."""

        if not hasattr(value, "array"):
            return value
        jpype = self._require_started_jvm()
        raw = jpype.JArray(jpype.JByte)(bytes(value.array()))
        return jpype.JClass("hla.rti1516e.encoding.ByteWrapper")(
            raw,
            int(value.getPos()),
            int(value.remaining()),
        )

    def copy_from_java_byte_wrapper(self, raw: object, target: object) -> None:
        if not hasattr(target, "array") or not hasattr(raw, "array"):
            return
        values = raw.array()
        target.array()[:] = bytes(int(item) & 0xFF for item in values)
        target.setPosition(int(raw.getPos()))

    def data_element_factory(self, factory: object) -> object:
        """Build the standard Java ``DataElementFactory`` proxy for Python code."""

        if hasattr(factory, "_implementation"):
            return factory._implementation  # type: ignore[attr-defined]
        jpype = self._require_started_jvm()
        interface = jpype.JClass("hla.rti1516e.encoding.DataElementFactory")

        class Target:
            def createElement(self, index: int) -> object:
                result = factory.createElement(int(index))  # type: ignore[attr-defined]
                return getattr(result, "_implementation", result)

        return jpype.JProxy(interface, inst=Target())

    def handle_bytes(self, handle: object) -> bytes:
        """Return a stable Python key for an opaque Java handle.

        Most HLA handles implement the encoded-handle operations.  The 1516e
        ``RegionHandle`` is deliberately different: it is an opaque value
        with only equality/hash/string operations and therefore cannot be
        serialized through ``encodedLength``/``encode``.  Keep the standard
        encoded path, but use the provider's Java ``toString`` identity for
        that opaque family so a handle issued by ``createRegion`` can be
        retained and passed back through later region-set calls.
        """

        encoded_length = getattr(handle, "encodedLength", None)
        encode = getattr(handle, "encode", None)
        if callable(encoded_length) and callable(encode):
            jpype = self._require_started_jvm()
            buffer = jpype.JArray(jpype.JByte)(int(encoded_length()))
            encode(buffer, 0)
            return bytes(int(value) & 0xFF for value in buffer)
        try:
            identity = str(handle)
        except Exception:  # noqa: BLE001  # pragma: no cover - defensive provider boundary
            identity = f"{type(handle).__name__}@{id(handle):x}"
        if not identity:
            identity = f"{type(handle).__name__}@{id(handle):x}"
        return ("opaque:" + identity).encode("utf-8")

    def exception_name(self, error: BaseException) -> str | None:
        current: object = error
        for _ in range(8):
            try:
                name = str(current.getClass().getSimpleName())  # type: ignore[attr-defined]
            except (AttributeError, TypeError):
                return None
            name = name.rsplit(".", 1)[-1].rsplit("$", 1)[-1]
            if name != "UndeclaredThrowableException":
                return name
            try:
                cause = current.getUndeclaredThrowable()  # type: ignore[attr-defined]
            except (AttributeError, TypeError):
                return None
            if cause is None:
                return None
            current = cause
        return None

    def java_enum(self, simple_name: str, member: str) -> object:
        return getattr(
            self._require_started_jvm().JClass(f"hla.rti1516e.{simple_name}"), member
        )

    def is_java_instance(self, value: object, class_name: str) -> bool:
        """Return whether a provider object implements a standard Java type.

        This is deliberately a runtime operation rather than a Python type
        check: Java providers commonly return ``Proxy`` instances whose
        generated class name contains no useful HLA interface information.
        """

        return isinstance(value, self._require_started_jvm().JClass(class_name))

    def decode_handle(
        self, ambassador: object, factory_name: str, encoded: bytes
    ) -> object:
        factory = getattr(ambassador, factory_name)()
        return factory.decode(self.byte_array(encoded), 0)

    def handle_set(
        self,
        ambassador: object,
        factory_name: str,
        values: object,
        set_factory_name: str | None = None,
    ) -> object:
        """Copy a Python handle set through the standard 1516e set factory."""

        if set_factory_name is None:
            java_set = self._require_started_jvm().JClass("java.util.HashSet")()
        else:
            java_set = getattr(ambassador, set_factory_name)().create()
        for value in values:  # type: ignore[operator]
            java_set.add(
                self.decode_handle(ambassador, factory_name, value.encodedValue)
            )  # type: ignore[attr-defined]
        return java_set

    def raw_handle_set(self, values: object) -> object:
        """Build a Java set from already-decoded provider handle objects."""

        java_set = self._require_started_jvm().JClass("java.util.HashSet")()
        for value in values:  # type: ignore[operator]
            java_set.add(value)
        return java_set

    def region_handle_set(self, ambassador: object, values: object) -> object:
        """Build the standard ``RegionHandleSet`` interface through its factory.

        A plain ``java.util.HashSet`` is not assignable to the 1516e
        ``RegionHandleSet`` interface, even though both extend ``Set``.  The
        standard API provides a dedicated factory, so use it for opaque region
        handles while retaining ``raw_handle_set`` for generic Java set uses.
        """

        java_set = ambassador.getRegionHandleSetFactory().create()  # type: ignore[attr-defined]
        for value in values:  # type: ignore[operator]
            java_set.add(value)
        return java_set

    def handle_value_map(
        self,
        ambassador: object,
        factory_name: str,
        values: object,
        map_factory_name: str | None = None,
    ) -> object:
        """Copy a Python handle/value map through the standard 1516e map factory."""

        if map_factory_name is None:
            java_map = self._require_started_jvm().JClass("java.util.HashMap")()
        else:
            java_map = getattr(ambassador, map_factory_name)().create(len(values))  # type: ignore[arg-type]
        for handle, encoded in values.items():  # type: ignore[union-attr]
            java_handle = self.decode_handle(
                ambassador, factory_name, handle.encodedValue
            )  # type: ignore[attr-defined]
            java_map.put(java_handle, self.byte_array(encoded))
        return java_map

    def string_set(self, values: object) -> object:
        java_set = self._require_started_jvm().JClass("java.util.HashSet")()
        for value in values:  # type: ignore[operator]
            java_set.add(str(value))
        return java_set

    def java_url(self, value: object) -> object:
        # The standard API consumes java.net.URL.  A pathlib.Path is the
        # portable Python spelling used by the TCK for a local FOM; convert it
        # to a file: URI before JPype constructs the authoritative Java URL.
        if isinstance(value, Path):
            value = value.resolve().as_uri()
        return self._require_started_jvm().JClass("java.net.URL")(str(value))

    def java_urls(self, values: object) -> object:
        jpype = self._require_started_jvm()
        url_type = jpype.JClass("java.net.URL")
        return jpype.JArray(url_type)([self.java_url(value) for value in values])  # type: ignore[operator]

    def range_bounds(self, value: object) -> object:
        return self._require_started_jvm().JClass("hla.rti1516e.RangeBounds")(
            int(value.lower),
            int(value.upper),  # type: ignore[attr-defined]
        )

    def decode_logical_time(self, ambassador: object, encoded: bytes) -> object:
        return ambassador.getTimeFactory().decodeTime(  # type: ignore[attr-defined]
            self.byte_array(encoded), 0
        )

    def decode_logical_interval(self, ambassador: object, encoded: bytes) -> object:
        return ambassador.getTimeFactory().decodeInterval(  # type: ignore[attr-defined]
            self.byte_array(encoded), 0
        )

    def from_java_value(
        self, value: object, expected_type: str | None = None
    ) -> object:
        """Convert callback carriers using the generated standard parameter type."""

        if expected_type is None:
            return value
        if expected_type == "byte[]":
            return bytes(value)
        if expected_type == "String":
            return str(value)
        if expected_type in _JAVA_HANDLE_TYPES:
            return _JAVA_HANDLE_TYPES[expected_type](self.handle_bytes(value))  # type: ignore[call-arg]
        if expected_type in _JAVA_ENUM_TYPES:
            name = getattr(value, "name", None)
            name = name() if callable(name) else name
            try:
                return _JAVA_ENUM_TYPES[expected_type][str(name)]  # type: ignore[index]
            except KeyError as error:
                raise RTIinternalError(
                    f"Java 1516e callback returned unknown {expected_type}: {name}"
                ) from error
        if expected_type == "AttributeHandleSet":
            return AttributeHandleSet(
                AttributeHandle(self.handle_bytes(item)) for item in value
            )  # type: ignore[arg-type]
        if expected_type == "DimensionHandleSet":
            return DimensionHandleSet(
                DimensionHandle(self.handle_bytes(item)) for item in value
            )  # type: ignore[arg-type]
        if expected_type == "FederateHandleSet":
            return FederateHandleSet(
                FederateHandle(self.handle_bytes(item)) for item in value
            )  # type: ignore[arg-type]
        if expected_type == "RegionHandleSet":
            return RegionHandleSet(
                RegionHandle(self.handle_bytes(item)) for item in value
            )  # type: ignore[arg-type]
        if expected_type == "Set<String>":
            return frozenset(str(item) for item in value)  # type: ignore[operator]
        if expected_type == "Set<DimensionHandle>":
            return DimensionHandleSet(
                DimensionHandle(self.handle_bytes(item))
                for item in value  # type: ignore[operator]
            )
        if expected_type == "FederationExecutionInformationSet":
            return FederationExecutionInformationSet(
                FederationExecutionInformation(
                    str(item.federationExecutionName),
                    str(item.logicalTimeImplementationName),
                )
                for item in value  # type: ignore[operator]
            )
        if expected_type == "FederationExecutionInformation":
            return FederationExecutionInformation(
                str(value.federationExecutionName),
                str(value.logicalTimeImplementationName),
            )
        if expected_type in {"SupplementalReflectInfo", "SupplementalReceiveInfo"}:
            producing = None
            has_producing = getattr(value, "hasProducingFederate", None)
            if callable(has_producing) and bool(has_producing()):
                producing = FederateHandle(
                    self.handle_bytes(value.getProducingFederate())
                )
            regions = None
            has_regions = getattr(value, "hasSentRegions", None)
            if callable(has_regions) and bool(has_regions()):
                regions = RegionHandleSet(
                    RegionHandle(self.handle_bytes(item))
                    for item in value.getSentRegions()
                )
            info_type = (
                SupplementalReflectInfo
                if expected_type == "SupplementalReflectInfo"
                else SupplementalReceiveInfo
            )
            return info_type(producing, regions)
        if expected_type == "SupplementalRemoveInfo":
            producing = None
            has_producing = getattr(value, "hasProducingFederate", None)
            if callable(has_producing) and bool(has_producing()):
                producing = FederateHandle(
                    self.handle_bytes(value.getProducingFederate())
                )
            return SupplementalRemoveInfo(producing)
        if expected_type == "FederateHandleSaveStatusPair[]":
            return tuple(
                FederateHandleSaveStatusPair(
                    FederateHandle(self.handle_bytes(item.handle)),
                    self.from_java_value(item.status, "SaveStatus"),  # type: ignore[arg-type]
                )
                for item in value  # type: ignore[operator]
            )
        if expected_type == "FederateRestoreStatus[]":
            return tuple(
                FederateRestoreStatus(
                    FederateHandle(self.handle_bytes(item.preRestoreHandle)),
                    FederateHandle(self.handle_bytes(item.postRestoreHandle)),
                    self.from_java_value(item.status, "RestoreStatus"),  # type: ignore[arg-type]
                )
                for item in value  # type: ignore[operator]
            )
        if expected_type in {"AttributeHandleValueMap", "ParameterHandleValueMap"}:
            handle_type = (
                AttributeHandle
                if expected_type.startswith("Attribute")
                else ParameterHandle
            )
            values = {
                handle_type(self.handle_bytes(handle)): bytes(encoded)
                for handle, encoded in value.entrySet()  # type: ignore[operator]
            }
            return (
                AttributeHandleValueMap(values)
                if expected_type.startswith("Attribute")
                else ParameterHandleValueMap(values)
            )
        if expected_type == "RangeBounds":
            return RangeBounds(int(value.lower), int(value.upper))  # type: ignore[attr-defined]
        if expected_type == "MessageRetractionReturn":
            handle = getattr(value, "handle", None)
            return MessageRetractionReturn(
                bool(value.retractionHandleIsValid),
                MessageRetractionHandle(self.handle_bytes(handle))
                if handle is not None
                else None,
            )
        if expected_type == "TimeQueryReturn":
            time = getattr(value, "time", None)
            return TimeQueryReturn(
                bool(value.timeIsValid),
                self.from_java_value(time, "LogicalTime") if time is not None else None,
            )
        if expected_type == "LogicalTime":
            implementation = str(value.getClass().getSimpleName())
            is_instance = getattr(self, "is_java_instance", None)
            standard_implementation = implementation in {
                "HLAinteger64Time",
                "HLAfloat64Time",
            }
            if callable(is_instance):
                try:
                    if is_instance(value, "hla.rti1516e.time.HLAinteger64Time"):
                        implementation = "HLAinteger64Time"
                        standard_implementation = True
                    elif is_instance(value, "hla.rti1516e.time.HLAfloat64Time"):
                        implementation = "HLAfloat64Time"
                        standard_implementation = True
                except Exception:  # noqa: BLE001, S110 - vendor proxy type probing
                    pass
            if not standard_implementation:
                # A vendor may expose a provider-specific LogicalTime
                # implementation.  Without a matching Python value type,
                # retaining the Java carrier is safer than guessing float64
                # and losing provider-owned arithmetic or wire semantics.
                return value
            encoded = self.handle_bytes(value)
            numeric = None
            for method_name in ("getTimeValue", "getTime", "getValue"):
                method = getattr(value, method_name, None)
                if callable(method):
                    numeric = method()
                    break
            time_type = (
                HLAinteger64Time
                if "integer" in implementation.lower()
                else HLAfloat64Time
            )
            return time_type(
                encodedValue=encoded,
                initial=bool(value.isInitial()),
                final=bool(value.isFinal()),
                value=numeric,
                text=str(value.toString()),
                implementationNameValue=(
                    "HLAinteger64Time"
                    if "integer" in implementation.lower()
                    else "HLAfloat64Time"
                ),
            )
        if expected_type == "LogicalTimeInterval":
            implementation = str(value.getClass().getSimpleName())
            is_instance = getattr(self, "is_java_instance", None)
            standard_implementation = implementation in {
                "HLAinteger64Interval",
                "HLAfloat64Interval",
            }
            if callable(is_instance):
                try:
                    if is_instance(value, "hla.rti1516e.time.HLAinteger64Interval"):
                        implementation = "HLAinteger64Interval"
                        standard_implementation = True
                    elif is_instance(value, "hla.rti1516e.time.HLAfloat64Interval"):
                        implementation = "HLAfloat64Interval"
                        standard_implementation = True
                except Exception:  # noqa: BLE001, S110 - vendor proxy type probing
                    pass
            if not standard_implementation:
                # Preserve an unknown vendor interval carrier for migration
                # code instead of coercing it to the float64 façade.
                return value
            encoded = self.handle_bytes(value)
            numeric = None
            for method_name in ("getIntervalValue", "getInterval", "getValue"):
                method = getattr(value, method_name, None)
                if callable(method):
                    numeric = method()
                    break
            from hla.rti1516e import HLAfloat64Interval, HLAinteger64Interval

            interval_type = (
                HLAinteger64Interval
                if "integer" in implementation.lower()
                else HLAfloat64Interval
            )
            return interval_type(
                encodedValue=encoded,
                zero=bool(value.isZero()),
                epsilon=bool(value.isEpsilon()),
                value=numeric,
                text=str(value.toString()),
                implementationNameValue=(
                    "HLAinteger64Interval"
                    if "integer" in implementation.lower()
                    else "HLAfloat64Interval"
                ),
            )
        # Unknown vendor-specific carriers remain raw rather than being
        # guessed into a 2025 or 2010 type.
        return value

    def attribute_region_pair_list(
        self,
        ambassador: object,
        values: object,
        raw_handle_resolver: object | None = None,
    ) -> object:
        """Build the standard list and association records through provider factories."""

        java_list = ambassador.getAttributeSetRegionSetPairListFactory().create(  # type: ignore[attr-defined,arg-type]
            len(values)
        )
        association_type = self._require_started_jvm().JClass(
            "hla.rti1516e.AttributeRegionAssociation"
        )
        for pair in values:  # type: ignore[operator]
            attributes = self.handle_set(
                ambassador,
                "getAttributeHandleFactory",
                pair.ahset,  # type: ignore[attr-defined]
                "getAttributeHandleSetFactory",
            )
            if raw_handle_resolver is None:
                raise RTIinternalError(
                    "1516e RegionHandle values require a live raw-handle resolver"
                )
            else:
                regions = self.region_handle_set(
                    ambassador,
                    (
                        raw_handle_resolver(RegionHandle, region)  # type: ignore[operator]
                        for region in pair.rhset  # type: ignore[attr-defined]
                    ),
                )
            java_list.add(association_type(attributes, regions))
        return java_list

    def _ensure_jvm(self, configuration: Java2010ProviderConfiguration) -> Any:
        global _STARTED_CONFIGURATION
        jpype = self._load_jpype()
        with _JVM_LOCK:
            if jpype.isJVMStarted():
                if _STARTED_CONFIGURATION == configuration:
                    return jpype
                if (
                    configuration.classpath
                    or configuration.jvm_path
                    or configuration.jvm_options
                ):
                    raise RTIinternalError(
                        "JVM already running with a different 1516e configuration"
                    )
                return jpype
            arguments = list(configuration.jvm_options)
            kwargs: dict[str, object] = {
                "classpath": list(configuration.classpath),
                "convertStrings": configuration.convert_strings,
            }
            if configuration.jvm_path is not None:
                kwargs["jvmpath"] = configuration.jvm_path
            try:
                jpype.startJVM(*arguments, **kwargs)
            except Exception as error:
                raise RTIinternalError(
                    f"Could not start the JVM for the 1516e RTI: {error}"
                ) from error
            _STARTED_CONFIGURATION = configuration
        return jpype

    def _require_started_jvm(self) -> Any:
        jpype = self._load_jpype()
        if not jpype.isJVMStarted():
            raise RTIinternalError("The 1516e Java RTI JVM has not been started")
        return jpype

    def _load_jpype(self) -> Any:
        if self._jpype is None:
            try:
                import jpype
            except ImportError as error:
                raise RTIinternalError(
                    "Install the optional dependency with 'umbra-rti-jpype[jpype]' to use a Java RTI"
                ) from error
            if not all(
                hasattr(jpype, name) for name in ("isJVMStarted", "startJVM", "JClass")
            ):
                raise RTIinternalError(
                    "The imported 'jpype' module is not JPype1; install 'umbra-rti-jpype[jpype]' "
                    "and remove any shadowing jpype package from PYTHONPATH"
                )
            self._jpype = jpype
        return self._jpype
