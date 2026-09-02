"""Opt-in Python -> JPype -> Java -> JNI -> C++ 2010 carrier evidence."""

from __future__ import annotations

import importlib.util
import os
import struct
import unittest
from pathlib import Path

from hla.rti1516e import (
    AttributeHandle,
    AttributeHandleSet,
    AttributeHandleValueMap,
    AttributeRegionAssociation,
    AttributeSetRegionSetPairList,
    CallbackModel,
    DimensionHandle,
    DimensionHandleSet,
    FederateHandle,
    FederateHandleSaveStatusPair,
    FederateHandleSet,
    FederateRestoreStatus,
    FederationExecutionInformation,
    FederationExecutionInformationSet,
    HLAfloat64Interval,
    HLAfloat64Time,
    HLAinteger64Interval,
    HLAinteger64Time,
    InteractionClassHandle,
    MessageRetractionHandle,
    MessageRetractionReturn,
    NullFederateAmbassador,
    ObjectClassHandle,
    ObjectInstanceHandle,
    OrderType,
    ParameterHandle,
    ParameterHandleValueMap,
    RangeBounds,
    RegionHandle,
    RegionHandleSet,
    ResignAction,
    RestoreStatus,
    SaveStatus,
    ServiceGroup,
    TimeQueryReturn,
    TransportationTypeHandle,
)
from hla.rti1516e.contracts import (
    FEDERATE_AMBASSADOR_METHODS,
    FEDERATE_AMBASSADOR_PARAMETER_TYPES,
    RTIAMBASSADOR_METHODS,
    RTIAMBASSADOR_PARAMETER_TYPES,
    RTIAMBASSADOR_RETURN_TYPES,
)
from hla.rti1516e.encoding import (
    ByteWrapper,
    DataElement,
    DecoderException,
    EncoderException,
)
from hla.rti1516e.exceptions import CouldNotDecode, RTIinternalError
from umbra._java.rti1516e import (
    Java2010ProviderConfiguration,
    Java2010RtiFactory,
    Java2010TimeFactory,
    JPype2010Runtime,
)
from umbra._java.rti1516e.encoding import JavaEncoderFactory, _wrap_element
from umbra._java.rti1516e.provider import Java2010RTIambassador
from umbra_rti_test_support import (
    CALLBACK_OVERLOAD_COUNTS,
    CallbackDeliveryObservation,
    assert_callback_delivery_parity,
    iter_callback_provenance_matrix,
    iter_data_element_value_matrix,
    iter_vendor_data_element_encode_matrix,
    iter_logical_time_wire_matrix,
    iter_vendor_data_element_wire_matrix,
    iter_vendor_time_arithmetic_matrix,
)

JPYPE_AVAILABLE = importlib.util.find_spec("jpype") is not None
ENABLED = os.environ.get("UMBRA_ENABLE_JNI_2010_TYPE_ROUNDTRIP_TESTS") == "1"


def _as_bytes(value: object) -> bytes:
    return bytes(int(item) & 0xFF for item in value)  # type: ignore[operator]


@unittest.skipUnless(
    JPYPE_AVAILABLE and ENABLED,
    "set UMBRA_ENABLE_JNI_2010_TYPE_ROUNDTRIP_TESTS=1 with JPype1 to run JNI evidence",
)
class Jpype2010JniTypeRoundTripTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        import jpype

        output = Path(
            os.environ.get(
                "UMBRA_JNI_2010_BRIDGE_ARTIFACT_DIRECTORY",
                "out/jni-2010",
            )
        ).resolve()
        api = Path(
            os.environ.get(
                "UMBRA_JNI_2010_JAVA_API_JAR",
                "out/java-tck-2010/ieee1516e-api-staged.jar",
            )
        ).resolve()
        bridge = output / "umbra-rti-jni-2010.jar"
        native = output / "umbra_rti_jni_2010.dll"
        if not api.is_file() or not bridge.is_file() or not native.is_file():
            raise unittest.SkipTest(
                "build the 2010 JNI artifacts or set UMBRA_JNI_2010_* artifact variables"
            )
        if not jpype.isJVMStarted():
            jpype.startJVM(
                classpath=[str(api), str(bridge)],
                convertStrings=False,
            )
        jpype.java.lang.System.setProperty("umbra.rti.jni.2010.library", str(native))
        cls.jpype = jpype
        cls.runtime = JPype2010Runtime()
        cls.runtime._jpype = jpype
        cls.probe = jpype.JClass("org.umbra.jni.rti1516e.NativeTypeRoundTrip")

    def test_python_discovers_jni_factory_and_binds_complete_2010_surface(self) -> None:
        """Resolve the standard JNI factory and expose every RTI method name."""

        factory = Java2010RtiFactory(
            Java2010ProviderConfiguration(
                rti_factory_name="Umbra JNI IEEE 1516e Null RTI",
            ),
            runtime=self.runtime,
        )
        self.assertEqual(factory.rtiName(), "Umbra JNI IEEE 1516e Null RTI")
        self.assertEqual(factory.rtiVersion(), "0.1.0-null")
        raw_factory = factory.unwrap_java_factory()
        for factory_method in ("rtiName", "rtiVersion", "getRtiAmbassador", "getEncoderFactory"):
            with self.subTest(factory_method=factory_method):
                self.assertTrue(callable(getattr(raw_factory, factory_method)))
        encoder = factory.getEncoderFactory()
        element = encoder.createHLAinteger32BE(7)
        self.assertEqual(element.getValue(), 7)
        ambassador = factory.getRtiAmbassador()
        raw_ambassador = ambassador.unwrap_java_object()
        self.assertTrue(callable(raw_ambassador.toString))
        try:
            for name in RTIAMBASSADOR_METHODS:
                with self.subTest(method=name):
                    self.assertTrue(callable(getattr(raw_ambassador, name)))
                    self.assertTrue(callable(getattr(ambassador, name)))
            self.assertEqual(len(RTIAMBASSADOR_METHODS), 150)
            self.assertEqual(
                sum(
                    len(RTIAMBASSADOR_PARAMETER_TYPES[name])
                    for name in RTIAMBASSADOR_METHODS
                ),
                172,
            )
            ambassador.connect(NullFederateAmbassador(), CallbackModel.HLA_IMMEDIATE)
            ambassador.disconnect()
        finally:
            close = getattr(raw_ambassador, "close", None)
            if callable(close):
                close()

    def test_python_encoder_facade_matches_reflected_2010_creator_surface(self) -> None:
        """Keep every Java 2010 EncoderFactory creator visible in Python.

        The provider-neutral Python factory collapses Java's primitive overload
        pairs into one optional-value method, but it must not lose a creator
        name or accidentally advertise a 2025-only creator.  Reflection keeps
        this check tied to the independently supplied standard Java API rather
        than to a duplicated list in the adapter.
        """

        import jpype

        standard_interface = jpype.JClass(
            "hla.rti1516e.encoding.EncoderFactory"
        ).class_
        java_signatures: dict[str, set[int]] = {}
        for method in standard_interface.getMethods():
            name = str(method.getName())
            if not name.startswith("createHLA"):
                continue
            java_signatures.setdefault(name, set()).add(
                int(method.getParameterCount())
            )

        scalar_names = {
            "createHLAASCIIchar",
            "createHLAASCIIstring",
            "createHLAboolean",
            "createHLAbyte",
            "createHLAfloat32BE",
            "createHLAfloat32LE",
            "createHLAfloat64BE",
            "createHLAfloat64LE",
            "createHLAinteger16BE",
            "createHLAinteger16LE",
            "createHLAinteger32BE",
            "createHLAinteger32LE",
            "createHLAinteger64BE",
            "createHLAinteger64LE",
            "createHLAoctet",
            "createHLAoctetPairBE",
            "createHLAoctetPairLE",
            "createHLAopaqueData",
            "createHLAunicodeChar",
            "createHLAunicodeString",
        }
        expected_signatures = {name: {0, 1} for name in scalar_names}
        expected_signatures.update(
            {
                "createHLAfixedArray": {1, 2},
                "createHLAfixedRecord": {0},
                "createHLAvariableArray": {2},
                "createHLAvariantRecord": {1},
            }
        )
        self.assertEqual(java_signatures, expected_signatures)

        encoder = JavaEncoderFactory(self.probe.encoderFactoryCarrier(), self.runtime)
        python_names = {
            name
            for name in dir(type(encoder))
            if name.startswith("createHLA")
        }
        self.assertEqual(python_names, set(java_signatures))
        for name in sorted(java_signatures):
            with self.subTest(creator=name):
                self.assertTrue(callable(getattr(encoder, name, None)))

    def test_python_jni_service_arguments_reach_standard_null_surface(self) -> None:
        """Feed every non-lifecycle overload through the real JNI ambassador.

        The null provider deliberately reports ``RTIinternalError`` for
        service state.  A Java proxy supplies only the standard factory
        carriers needed to convert provider-neutral Python arguments; all
        other calls delegate to the actual JNI ambassador.  Therefore an
        expected internal error proves that overload selection and argument
        conversion reached the standard Java/JNI surface without making a
        stateful-RTI claim.
        """

        factory = Java2010RtiFactory(
            Java2010ProviderConfiguration(
                rti_factory_name="Umbra JNI IEEE 1516e Null RTI",
            ),
            runtime=self.runtime,
        )

        handle_types = {
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
        raw_handles = {
            name: self.probe.handleCarrier(name, 140 + index)
            for index, name in enumerate(handle_types)
        }

        class DelegatingTarget:
            def __init__(self, raw: object, probe: object, time_kind: str) -> None:
                self.raw = raw
                self.probe = probe
                self.time_kind = time_kind

            def _delegate(self, name: str, *args: object) -> object:
                return getattr(self.raw, name)(*args)

            def connect(self, *args: object) -> object:
                return self.raw.connect(*args)

            def disconnect(self) -> object:
                return self.raw.disconnect()

            def getTimeFactory(self) -> object:
                return self.probe.logicalTimeFactoryCarrier(
                    self.time_kind + "Factory"
                )

            def getFederateHandleFactory(self) -> object:
                return self.probe.handleFactoryCarrier("FederateHandleFactory")

            def getObjectClassHandleFactory(self) -> object:
                return self.probe.handleFactoryCarrier("ObjectClassHandleFactory")

            def getObjectInstanceHandleFactory(self) -> object:
                return self.probe.handleFactoryCarrier(
                    "ObjectInstanceHandleFactory"
                )

            def getAttributeHandleFactory(self) -> object:
                return self.probe.handleFactoryCarrier("AttributeHandleFactory")

            def getInteractionClassHandleFactory(self) -> object:
                return self.probe.handleFactoryCarrier(
                    "InteractionClassHandleFactory"
                )

            def getParameterHandleFactory(self) -> object:
                return self.probe.handleFactoryCarrier("ParameterHandleFactory")

            def getTransportationTypeHandleFactory(self) -> object:
                return self.probe.handleFactoryCarrier(
                    "TransportationTypeHandleFactory"
                )

            def getDimensionHandleFactory(self) -> object:
                return self.probe.handleFactoryCarrier("DimensionHandleFactory")

            def getAttributeHandleSetFactory(self) -> object:
                return self.probe.collectionFactoryCarrier(
                    "AttributeHandleSetFactory"
                )

            def getDimensionHandleSetFactory(self) -> object:
                return self.probe.collectionFactoryCarrier(
                    "DimensionHandleSetFactory"
                )

            def getFederateHandleSetFactory(self) -> object:
                return self.probe.collectionFactoryCarrier(
                    "FederateHandleSetFactory"
                )

            def getRegionHandleSetFactory(self) -> object:
                return self.probe.collectionFactoryCarrier("RegionHandleSetFactory")

            def getAttributeHandleValueMapFactory(self) -> object:
                return self.probe.collectionFactoryCarrier(
                    "AttributeHandleValueMapFactory"
                )

            def getParameterHandleValueMapFactory(self) -> object:
                return self.probe.collectionFactoryCarrier(
                    "ParameterHandleValueMapFactory"
                )

            def getAttributeSetRegionSetPairListFactory(self) -> object:
                return self.probe.collectionFactoryCarrier(
                    "AttributeSetRegionSetPairListFactory"
                )

        for name in RTIAMBASSADOR_METHODS:
            if name in {
                "connect",
                "disconnect",
                "getTimeFactory",
                "getFederateHandleFactory",
                "getObjectClassHandleFactory",
                "getObjectInstanceHandleFactory",
                "getAttributeHandleFactory",
                "getInteractionClassHandleFactory",
                "getParameterHandleFactory",
                "getTransportationTypeHandleFactory",
                "getDimensionHandleFactory",
                "getAttributeHandleSetFactory",
                "getDimensionHandleSetFactory",
                "getFederateHandleSetFactory",
                "getRegionHandleSetFactory",
                "getAttributeHandleValueMapFactory",
                "getParameterHandleValueMapFactory",
                "getAttributeSetRegionSetPairListFactory",
            }:
                continue

            def invoke(self: DelegatingTarget, *args: object, _name=name) -> object:
                return self._delegate(_name, *args)

            setattr(DelegatingTarget, name, invoke)

        for time_kind in ("HLAinteger64Time", "HLAfloat64Time"):
            raw_ambassador = factory.getRtiAmbassador().unwrap_java_object()
            raw_close = getattr(raw_ambassador, "close", None)
            if callable(raw_close):
                self.addCleanup(raw_close)
            target = DelegatingTarget(raw_ambassador, self.probe, time_kind)
            ambassador = Java2010RTIambassador(target, self.runtime)
            for handle_name, handle_type in handle_types.items():
                ambassador._remember_raw_handle(handle_type, raw_handles[handle_name])

            def argument(expected_type: str, *, _time_kind=time_kind) -> object:
                if expected_type in handle_types:
                    return handle_types[expected_type](
                        self.runtime.handle_bytes(raw_handles[expected_type])
                    )
                if expected_type == "FederateAmbassador":
                    return NullFederateAmbassador()
                if expected_type == "CallbackModel":
                    return CallbackModel.HLA_IMMEDIATE
                if expected_type == "String":
                    return "jni-service-matrix"
                if expected_type == "URL":
                    return Path(__file__).resolve()
                if expected_type == "URL[]":
                    return (Path(__file__).resolve(),)
                if expected_type == "byte[]":
                    return b"jni-service-matrix"
                if expected_type == "Set<String>":
                    return frozenset(("jni-service-matrix",))
                if expected_type == "AttributeHandleSet":
                    return AttributeHandleSet(
                        (argument("AttributeHandle"),)
                    )
                if expected_type == "DimensionHandleSet":
                    return DimensionHandleSet((argument("DimensionHandle"),))
                if expected_type == "FederateHandleSet":
                    return FederateHandleSet((argument("FederateHandle"),))
                if expected_type == "RegionHandleSet":
                    return RegionHandleSet((argument("RegionHandle"),))
                if expected_type == "AttributeHandleValueMap":
                    return AttributeHandleValueMap(
                        {argument("AttributeHandle"): b"attribute"}
                    )
                if expected_type == "ParameterHandleValueMap":
                    return ParameterHandleValueMap(
                        {argument("ParameterHandle"): b"parameter"}
                    )
                if expected_type == "AttributeSetRegionSetPairList":
                    return AttributeSetRegionSetPairList(
                        [
                            AttributeRegionAssociation(
                                AttributeHandleSet((argument("AttributeHandle"),)),
                                RegionHandleSet((argument("RegionHandle"),)),
                            )
                        ]
                    )
                if expected_type == "LogicalTime":
                    if _time_kind == "HLAinteger64Time":
                        return HLAinteger64Time(
                            encodedValue=(89).to_bytes(8, "big", signed=True),
                            value=89,
                            implementationNameValue=_time_kind,
                        )
                    return HLAfloat64Time(
                        encodedValue=struct.pack(">d", 8.9),
                        value=8.9,
                        implementationNameValue=_time_kind,
                    )
                if expected_type == "LogicalTimeInterval":
                    if _time_kind == "HLAinteger64Time":
                        return HLAinteger64Interval(
                            encodedValue=(9).to_bytes(8, "big", signed=True),
                            value=9,
                            implementationNameValue="HLAinteger64Interval",
                        )
                    return HLAfloat64Interval(
                        encodedValue=struct.pack(">d", 0.9),
                        value=0.9,
                        implementationNameValue="HLAfloat64Interval",
                    )
                if expected_type == "RangeBounds":
                    return RangeBounds(81, 89)
                if expected_type == "OrderType":
                    return OrderType.RECEIVE
                if expected_type == "ResignAction":
                    return ResignAction.NO_ACTION
                if expected_type == "ServiceGroup":
                    return ServiceGroup.FEDERATION_MANAGEMENT
                if expected_type == "double":
                    return 2.25
                if expected_type == "boolean":
                    return True
                raise AssertionError(
                    "service argument matrix has no value for " + expected_type
                )

            for name in RTIAMBASSADOR_METHODS:
                if name in {"connect", "disconnect"}:
                    continue
                if name in {
                    "getTimeFactory",
                    "getFederateHandleFactory",
                    "getObjectClassHandleFactory",
                    "getObjectInstanceHandleFactory",
                    "getAttributeHandleFactory",
                    "getInteractionClassHandleFactory",
                    "getParameterHandleFactory",
                    "getTransportationTypeHandleFactory",
                    "getDimensionHandleFactory",
                    "getAttributeHandleSetFactory",
                    "getDimensionHandleSetFactory",
                    "getFederateHandleSetFactory",
                    "getRegionHandleSetFactory",
                    "getAttributeHandleValueMapFactory",
                    "getParameterHandleValueMapFactory",
                    "getAttributeSetRegionSetPairListFactory",
                }:
                    with self.subTest(method=name, time_kind=time_kind):
                        if name == "getTimeFactory":
                            self.assertIsInstance(ambassador.getTimeFactory(), Java2010TimeFactory)
                        else:
                            self.assertIsNotNone(getattr(ambassador, name)())
                    continue
                for overload_index, parameters in enumerate(
                    RTIAMBASSADOR_PARAMETER_TYPES[name]
                ):
                    arguments = tuple(argument(parameter) for parameter in parameters)
                    with self.subTest(
                        method=name,
                        overload=overload_index,
                        time_kind=time_kind,
                    ), self.assertRaises(RTIinternalError):
                        getattr(ambassador, name)(*arguments)

            ambassador.connect(NullFederateAmbassador(), CallbackModel.HLA_IMMEDIATE)
            ambassador.disconnect()

    def test_primitives_strings_bytes_and_bytewrapper(self) -> None:
        self.assertEqual(self.probe.roundTripByte(-7), -7)
        self.assertEqual(self.probe.roundTripShort(-1234), -1234)
        self.assertEqual(self.probe.roundTripInt(-123456789), -123456789)
        self.assertEqual(self.probe.roundTripLong(-123456789012345), -123456789012345)
        self.assertEqual(self.probe.roundTripFloat(1.25), 1.25)
        self.assertEqual(self.probe.roundTripDouble(-2.5), -2.5)
        self.assertTrue(self.probe.roundTripBoolean(True))
        self.assertEqual(str(self.probe.roundTripString("JNI Ω 🚀")), "JNI Ω 🚀")
        value = b"\x00\x01\xfe\x7f"
        java_value = self.runtime.byte_array(value)
        self.assertEqual(_as_bytes(self.probe.roundTripBytes(java_value)), value)
        self.assertEqual(_as_bytes(self.probe.roundTripByteWrapper(java_value)), value)
        callback_model = self.jpype.JClass("hla.rti1516e.CallbackModel").HLA_EVOKED
        self.assertEqual(self.probe.roundTripEnum(callback_model), callback_model)
        for enum_type in self.probe.enumTypes():
            enum_class = self.jpype.JClass("hla.rti1516e." + str(enum_type))
            first = enum_class.values()[0]
            self.assertEqual(self.probe.roundTripObject(first), first, str(enum_type))
        bounds = self.jpype.JClass("hla.rti1516e.RangeBounds")(4, 9)
        self.assertEqual(self.probe.roundTripObject(bounds), bounds)
        information = self.jpype.JClass("hla.rti1516e.FederationExecutionInformation")(
            "federation", "HLAinteger64Time"
        )
        self.assertEqual(self.probe.roundTripObject(information), information)
        federate_handle = self.probe.handleCarrier("FederateHandle", 31)
        save_status = self.jpype.JClass("hla.rti1516e.SaveStatus").values()[0]
        save_pair = self.jpype.JClass("hla.rti1516e.FederateHandleSaveStatusPair")(
            federate_handle, save_status
        )
        self.assertEqual(self.probe.roundTripObject(save_pair), save_pair)
        restore_status = self.jpype.JClass("hla.rti1516e.RestoreStatus").values()[0]
        restore_pair = self.jpype.JClass("hla.rti1516e.FederateRestoreStatus")(
            federate_handle,
            self.probe.handleCarrier("FederateHandle", 32),
            restore_status,
        )
        self.assertEqual(self.probe.roundTripObject(restore_pair), restore_pair)
        retraction = self.jpype.JClass("hla.rti1516e.MessageRetractionReturn")(
            True, self.probe.handleCarrier("MessageRetractionHandle", 33)
        )
        self.assertEqual(self.probe.roundTripObject(retraction), retraction)
        query = self.jpype.JClass("hla.rti1516e.TimeQueryReturn")(
            True, self.probe.logicalTimeCarrier("HLAinteger64Time")
        )
        self.assertEqual(self.probe.roundTripObject(query), query)
        callback = self.probe.callbackCarrier("SupplementalReflectInfo")
        returned_callback = self.probe.roundTripObject(callback)
        self.assertTrue(returned_callback.hasProducingFederate())
        self.assertFalse(returned_callback.hasSentRegions())
        for callback_type in ("SupplementalReceiveInfo", "SupplementalRemoveInfo"):
            self.assertIsNotNone(
                self.probe.roundTripObject(self.probe.callbackCarrier(callback_type)),
                callback_type,
            )
        exception = self.jpype.JClass("hla.rti1516e.exceptions.RTIinternalError")(
            "JNI exception carrier"
        )
        self.assertEqual(self.probe.roundTripObject(exception), exception)
        for exception_name in (
            "hla.rti1516e.encoding.EncoderException",
            "hla.rti1516e.encoding.DecoderException",
        ):
            carrier = self.jpype.JClass(exception_name)(
                "JNI encoding exception carrier"
            )
            self.assertEqual(
                self.probe.roundTripObject(carrier), carrier, exception_name
            )
        with self.assertRaises(
            self.jpype.JClass("hla.rti1516e.encoding.DecoderException")
        ):
            self.probe.roundTripDataElement(
                "HLAinteger32BE", self.runtime.byte_array(b"\x01")
            )

    def test_standard_exception_matrix_preserves_java_identity_and_message(
        self,
    ) -> None:
        rti_exception = self.jpype.JClass("hla.rti1516e.exceptions.RTIexception")
        for raw_name in self.probe.standardExceptionNames():
            name = str(raw_name)
            message = "jpype-2010:" + name + ": Ω 🚀\x00"
            try:
                self.probe.throwStandardException(name, message)
            except Exception as error:  # noqa: BLE001 - JPype raises Java Throwable wrappers
                self.assertIsInstance(error, rti_exception, name)
                self.assertEqual(self.runtime.exception_name(error), name)
                self.assertEqual(str(error.getMessage()), message)
                self.assertIsNone(error.getCause(), name + " cause")
            else:
                self.fail(name + " was not thrown")
        for name in ("EncoderException", "DecoderException"):
            message = "jpype-2010:" + name + ": Ω 🚀\x00"
            try:
                self.probe.throwStandardException(name, message)
            except Exception as error:  # noqa: BLE001 - JPype raises Java Throwable wrappers
                self.assertEqual(self.runtime.exception_name(error), name)
                self.assertEqual(str(error.getMessage()), message)
                self.assertIsNone(error.getCause(), name + " cause")
            else:
                self.fail(name + " was not thrown")

    def test_standard_data_elements_handles_collections_and_times(self) -> None:
        byte_wrapper_type = self.jpype.JClass(
            "hla.rti1516e.encoding.ByteWrapper"
        )
        for kind in self.probe.dataElementKinds():
            name = str(kind)
            seed = _as_bytes(self.probe.seedDataElement(name))
            self.assertTrue(seed, name)
            returned = self.probe.roundTripDataElement(
                name, self.runtime.byte_array(seed)
            )
            self.assertEqual(_as_bytes(returned), seed, name)
            carrier = self.probe.dataElementCarrier(name)
            self.assertEqual(
                _as_bytes(carrier.toByteArray()), seed, name + " standard carrier"
            )
            decoded_carrier = self.probe.dataElementCarrier(name)
            decoded_carrier.decode(self.runtime.byte_array(seed))
            self.assertEqual(
                _as_bytes(decoded_carrier.toByteArray()),
                seed,
                name + " byte-array decode",
            )
            wrapper = self.runtime.java_byte_wrapper(ByteWrapper(seed))
            carrier.encode(wrapper)
            self.assertEqual(
                _as_bytes(wrapper.array()), seed, name + " ByteWrapper carrier"
            )

            # Exercise the direct Java cursor overload in the opposite
            # direction as well.  Keep a non-zero offset and trailing
            # sentinel so a successful JNI encode must advance exactly one
            # carrier and leave the caller-owned suffix untouched.
            signed_seed = [
                value if value < 0x80 else value - 0x100 for value in seed
            ]
            encode_buffer = self.jpype.JArray(self.jpype.JByte)(
                [0, *signed_seed, -0x5B]
            )
            encode_wrapper = byte_wrapper_type(
                encode_buffer, 1, len(seed) + 1
            )
            carrier.encode(encode_wrapper)
            self.assertEqual(encode_wrapper.getPos(), len(seed) + 1, name)
            self.assertEqual(encode_wrapper.remaining(), 1, name)
            self.assertEqual(
                _as_bytes(encode_buffer)[1 : len(seed) + 1], seed, name
            )
            self.assertEqual(
                int(encode_buffer[encode_wrapper.getPos()]) & 0xFF,
                0xA5,
                name,
            )
            scalar = name.startswith(("HLAinteger", "HLAfloat")) or name in {
                "HLAbyte",
                "HLAoctet",
                "HLAASCIIchar",
                "HLAunicodeChar",
                "HLAoctetPairBE",
                "HLAoctetPairLE",
                "HLAboolean",
                "HLAASCIIstring",
                "HLAunicodeString",
            }
            if scalar:
                typed_value = carrier.getValue()
                self.assertIsNotNone(typed_value, name + " typed getValue")
                carrier.setValue(typed_value)
                self.assertEqual(
                    _as_bytes(carrier.toByteArray()), seed, name + " typed setValue"
                )
            elif name == "HLAopaqueData":
                payload = carrier.getValue()
                self.assertGreater(len(payload), 0, name + " typed payload")
                self.assertEqual(carrier.size(), len(payload), name + " typed size")
                self.assertEqual(int(carrier.get(0)) & 0xFF, int(payload[0]) & 0xFF)
                carrier.setValue(payload)
                self.assertEqual(
                    _as_bytes(carrier.toByteArray()), seed, name + " typed setValue"
                )
            elif name == "HLAvariantRecord":
                self.assertIsNotNone(
                    carrier.getDiscriminant(), name + " typed discriminant"
                )
                self.assertIsNotNone(carrier.getValue(), name + " typed variant value")
            else:
                self.assertGreater(carrier.size(), 0, name + " typed composite size")
                self.assertIsNotNone(carrier.get(0), name + " typed composite get")
        encoder_factory = self.probe.encoderFactoryCarrier()
        integer_encoder = encoder_factory.createHLAinteger32BE(7)
        self.assertEqual(
            _as_bytes(integer_encoder.toByteArray()),
            b"\x00\x00\x00\x07",
        )
        opaque_encoder = encoder_factory.createHLAopaqueData(
            self.runtime.byte_array(b"\x00\x01\x02\x03")
        )
        self.assertEqual(
            _as_bytes(opaque_encoder.toByteArray()),
            b"\x00\x00\x00\x04\x00\x01\x02\x03",
        )
        for element_kind in self.probe.dataElementFactoryKinds():
            name = str(element_kind)
            element = self.probe.dataElementFactoryCarrier(name).createElement(0)
            self.assertEqual(
                _as_bytes(element.toByteArray()),
                _as_bytes(self.probe.seedDataElement(name)),
                name + " DataElementFactory encoding",
            )

        byte_array = self.jpype.JArray(self.jpype.JByte)
        byte_arrays = self.jpype.JArray(byte_array)
        for kind in self.probe.handleKinds():
            name = str(kind)
            seed = _as_bytes(self.probe.seedHandle(name, 0x0123456789ABCDEF))
            self.assertEqual(
                _as_bytes(
                    self.probe.roundTripHandle(name, self.runtime.byte_array(seed))
                ),
                seed,
                name,
            )
            carrier = self.probe.handleCarrier(name, 0x0123456789ABCDEF)
            self.assertEqual(
                self.runtime.handle_bytes(carrier), seed, name + " standard carrier"
            )
            returned_carrier = self.probe.roundTripHandleCarrier(name, carrier)
            self.assertEqual(
                self.runtime.handle_bytes(returned_carrier),
                seed,
                name + " standard carrier round-trip",
            )
            values = byte_arrays(
                [
                    self.runtime.byte_array(seed),
                    self.runtime.byte_array(self.probe.seedHandle(name, 7)),
                ]
            )
            returned = self.probe.roundTripHandleCollection(name, values)
            self.assertEqual(len(returned), 2, name)
            self.assertEqual(_as_bytes(returned[0]), seed, name)

        for kind in self.probe.javaOnlyHandleKinds():
            name = str(kind)
            carrier = self.probe.handleCarrier(name, 7)
            returned_carrier = self.probe.roundTripHandleCarrier(name, carrier)
            self.assertEqual(
                self.runtime.handle_bytes(returned_carrier),
                self.runtime.handle_bytes(carrier),
                name + " Java-only standard carrier",
            )

        for kind in self.probe.logicalTimeKinds():
            name = str(kind)
            seed = _as_bytes(self.probe.seedLogicalTime(name))
            self.assertEqual(
                _as_bytes(
                    self.probe.roundTripLogicalTime(name, self.runtime.byte_array(seed))
                ),
                seed,
                name,
            )
            carrier = self.probe.logicalTimeCarrier(name)
            expected = {
                "HLAinteger64Time": 123456789,
                "HLAinteger64Interval": 1234,
                "HLAfloat64Time": 12.5,
                "HLAfloat64Interval": 0.25,
            }[name]
            actual = carrier.getValue()
            if isinstance(expected, int):
                self.assertEqual(int(actual), expected, name + " value")
            else:
                self.assertEqual(float(actual), expected, name + " value")
            destination = self.jpype.JArray(self.jpype.JByte)(len(seed) + 2)
            carrier.encode(destination, 1)
            self.assertEqual(_as_bytes(destination)[1:-1], seed, name + " encode")
            self.assertEqual(
                self.runtime.handle_bytes(carrier), seed, name + " standard carrier"
            )

        for factory_kind in self.probe.logicalTimeFactoryKinds():
            factory_name = str(factory_kind)
            time_name = factory_name.removesuffix("Factory")
            integer = time_name.startswith("HLAinteger64")
            factory = self.probe.logicalTimeFactoryCarrier(factory_name)
            self.assertEqual(str(factory.getName()), time_name)
            initial = factory.makeInitial()
            final = factory.makeFinal()
            zero = factory.makeZero()
            epsilon = factory.makeEpsilon()
            time_value = 123456789 if integer else 12.5
            interval_value = 1234 if integer else 0.25
            time = factory.makeTime(time_value)
            interval = factory.makeInterval(interval_value)
            self.assertTrue(initial.isInitial(), factory_name + " initial")
            self.assertTrue(final.isFinal(), factory_name + " final")
            self.assertTrue(zero.isZero(), factory_name + " zero")
            self.assertTrue(epsilon.isEpsilon(), factory_name + " epsilon")
            expected_values = (
                (
                    initial,
                    0,
                    struct.pack(">q", 0) if integer else struct.pack(">d", 0.0),
                    factory_name + " initial",
                ),
                (
                    final,
                    2**63 - 1 if integer else float.fromhex("0x1.fffffffffffffp+1023"),
                    struct.pack(">q", 2**63 - 1)
                    if integer
                    else struct.pack(">d", float.fromhex("0x1.fffffffffffffp+1023")),
                    factory_name + " final",
                ),
                (zero, 0, struct.pack(">q", 0), factory_name + " zero")
                if integer
                else (zero, 0.0, struct.pack(">d", 0.0), factory_name + " zero"),
                (
                    epsilon,
                    1 if integer else float.fromhex("0x0.0000000000001p-1022"),
                    struct.pack(">q", 1)
                    if integer
                    else struct.pack(">d", float.fromhex("0x0.0000000000001p-1022")),
                    factory_name + " epsilon",
                ),
                (
                    time,
                    time_value,
                    struct.pack(">q", time_value)
                    if integer
                    else struct.pack(">d", time_value),
                    factory_name + " makeTime",
                ),
                (
                    interval,
                    interval_value,
                    struct.pack(">q", interval_value)
                    if integer
                    else struct.pack(">d", interval_value),
                    factory_name + " makeInterval",
                ),
            )
            for carrier, expected, encoded, label in expected_values:
                self.assertEqual(
                    int(carrier.encodedLength()), len(encoded), label + " length"
                )
                destination = self.jpype.JArray(self.jpype.JByte)(len(encoded) + 2)
                carrier.encode(destination, 1)
                self.assertEqual(
                    _as_bytes(destination)[1:-1], encoded, label + " encode"
                )
                if integer:
                    self.assertEqual(
                        int(carrier.getValue()), expected, label + " value"
                    )
                else:
                    self.assertEqual(
                        float(carrier.getValue()), expected, label + " value"
                    )
            time_encoded = (
                struct.pack(">q", time_value)
                if integer
                else struct.pack(">d", time_value)
            )
            interval_encoded = (
                struct.pack(">q", interval_value)
                if integer
                else struct.pack(">d", interval_value)
            )
            decoded_time = factory.decodeTime(
                self.runtime.byte_array(b"\x5a" + time_encoded + b"\xa5"), 1
            )
            decoded_interval = factory.decodeInterval(
                self.runtime.byte_array(b"\x5a" + interval_encoded + b"\xa5"), 1
            )
            self.assertEqual(
                int(decoded_time.getValue())
                if integer
                else float(decoded_time.getValue()),
                time_value,
                factory_name + " decodeTime value",
            )
            self.assertEqual(
                int(decoded_interval.getValue())
                if integer
                else float(decoded_interval.getValue()),
                interval_value,
                factory_name + " decodeInterval value",
            )
            self.assertEqual(self.runtime.handle_bytes(decoded_time), time_encoded)
            self.assertEqual(
                self.runtime.handle_bytes(decoded_interval), interval_encoded
            )

        matrix = self.jpype.JArray(self.jpype.JByte)
        matrices = self.jpype.JArray(matrix)(
            [self.runtime.byte_array(b"abc"), self.runtime.byte_array(b"\x00\xff")]
        )
        returned_matrix = self.probe.roundTripByteMatrix(matrices)
        self.assertEqual(_as_bytes(returned_matrix[0]), b"abc")
        self.assertEqual(_as_bytes(returned_matrix[1]), b"\x00\xff")

        for kind in self.probe.collectionKinds():
            name = str(kind)
            carrier = self.probe.collectionCarrier(name)
            if name.endswith("Map"):
                handle_kind = (
                    "ParameterHandle"
                    if name.startswith("Parameter")
                    else "AttributeHandle"
                )
                key = self.probe.handleCarrier(handle_kind, 11)
                carrier.put(key, self.runtime.byte_array(b"\x03\x01\x04"))
                returned = self.probe.roundTripCollectionCarrier(name, carrier)
                self.assertEqual(returned.size(), 1, name)
                entry = returned.entrySet().iterator().next()
                self.assertEqual(_as_bytes(entry.getValue()), b"\x03\x01\x04", name)
                self.assertIsNotNone(
                    returned.getValueReference(key), name + " value reference"
                )
            elif name.endswith("List"):
                attributes = self.probe.collectionCarrier("AttributeHandleSet")
                regions = self.probe.collectionCarrier("RegionHandleSet")
                attributes.add(self.probe.handleCarrier("AttributeHandle", 1))
                regions.add(self.probe.handleCarrier("RegionHandle", 2))
                association_class = self.jpype.JClass(
                    "hla.rti1516e.AttributeRegionAssociation"
                )
                carrier.add(association_class(attributes, regions))
                returned = self.probe.roundTripCollectionCarrier(name, carrier)
                self.assertEqual(returned.size(), 1, name)
            else:
                if name.startswith("FederationExecution"):
                    information = self.jpype.JClass(
                        "hla.rti1516e.FederationExecutionInformation"
                    )("federation", "HLAinteger64Time")
                    carrier.add(information)
                else:
                    handle_kind = (
                        "AttributeHandle"
                        if name.startswith("Attribute")
                        else "DimensionHandle"
                        if name.startswith("Dimension")
                        else "FederateHandle"
                        if name.startswith("Federate")
                        else "RegionHandle"
                    )
                    carrier.add(self.probe.handleCarrier(handle_kind, 5))
                returned = self.probe.roundTripCollectionCarrier(name, carrier)
                self.assertEqual(returned.size(), 1, name)

        for factory_kind in self.probe.collectionFactoryKinds():
            name = str(factory_kind)
            factory = self.probe.collectionFactoryCarrier(name)
            created = (
                factory.create() if name.endswith("SetFactory") else factory.create(2)
            )
            self.assertIsNotNone(created, name)

        for factory_kind in self.probe.handleFactoryKinds():
            factory_name = str(factory_kind)
            handle_name = factory_name.removesuffix("Factory")
            factory = self.probe.handleFactoryCarrier(factory_name)
            carrier = self.probe.handleCarrier(handle_name, 19)
            decoded = factory.decode(carrier.encodedValue(), 0)
            self.assertEqual(
                self.runtime.handle_bytes(decoded),
                self.runtime.handle_bytes(carrier),
                factory_name + " decode",
            )
            if factory_name.endswith("TransportationTypeHandleFactory"):
                self.assertIsNotNone(factory.getHLAdefaultReliable())
                self.assertIsNotNone(factory.getHLAdefaultBestEffort())

    def test_malformed_standard_data_element_matrix_maps_to_decoder_exception(
        self,
    ) -> None:
        """Every standard 2010 carrier rejects a truncated wire value consistently.

        The JNI entry point is declared with the Java encoding contract's
        ``DecoderException``. This matrix keeps that direction-specific
        exception identity intact for every carrier exposed by the probe,
        instead of only checking one integer example.
        """
        decoder_exception = self.jpype.JClass("hla.rti1516e.encoding.DecoderException")
        for raw_kind in self.probe.dataElementKinds():
            name = str(raw_kind)
            seed = _as_bytes(self.probe.seedDataElement(name))
            self.assertTrue(seed, name)
            truncated = seed[:-1]
            with self.subTest(kind=name, malformed="truncated"):
                with self.assertRaises(decoder_exception) as raised:
                    self.probe.roundTripDataElement(
                        name, self.runtime.byte_array(truncated)
                    )
                self.assertEqual(
                    self.runtime.exception_name(raised.exception),
                    "DecoderException",
                    name + " exception identity",
                )
                self.assertTrue(str(raised.exception.getMessage()), name + " message")

            # Length-prefixed carriers also get a declared-length overrun;
            # this exercises malformed metadata independently of truncation.
            if name in {
                "HLAASCIIstring",
                "HLAunicodeString",
                "HLAopaqueData",
                "HLAvariableArray",
            }:
                overrun = bytearray(seed)
                declared = int.from_bytes(overrun[:4], "big")
                overrun[:4] = (declared + 1).to_bytes(4, "big")
                with self.subTest(kind=name, malformed="declared-overrun"):
                    with self.assertRaises(decoder_exception) as raised:
                        self.probe.roundTripDataElement(
                            name, self.runtime.byte_array(bytes(overrun))
                        )
                    self.assertEqual(
                        self.runtime.exception_name(raised.exception),
                        "DecoderException",
                        name + " overrun exception identity",
                    )

    def test_standard_data_element_bytewrapper_cursor_contract(self) -> None:
        """Decode one carrier from an offset while preserving trailing bytes."""
        byte_wrapper_type = self.jpype.JClass("hla.rti1516e.encoding.ByteWrapper")
        for raw_kind in self.probe.dataElementKinds():
            name = str(raw_kind)
            seed = _as_bytes(self.probe.seedDataElement(name))
            window = self.runtime.byte_array(b"\x55" + seed + b"\xa5")
            wrapper = byte_wrapper_type(window, 1, len(seed) + 1)
            carrier = self.probe.dataElementCarrier(name)
            with self.subTest(kind=name):
                carrier.decode(wrapper)
                self.assertEqual(_as_bytes(carrier.toByteArray()), seed, name)
                self.assertEqual(
                    wrapper.getPos(), len(seed) + 1, name + " consumed length"
                )
                self.assertEqual(wrapper.remaining(), 1, name + " trailing window")
                self.assertEqual(
                    int(wrapper.array()[wrapper.getPos()]) & 0xFF,
                    0xA5,
                    name + " trailing sentinel",
                )

    def test_python_encoder_facade_uses_real_jni_carriers(self) -> None:
        """Exercise the public Python encoder façade over the JNI factory proxy."""
        encoder = JavaEncoderFactory(self.probe.encoderFactoryCarrier(), self.runtime)
        scalar_cases = (
            ("HLAinteger16BE", -1234),
            ("HLAinteger16LE", -1234),
            ("HLAinteger32BE", -123456),
            ("HLAinteger32LE", -123456),
            ("HLAinteger64BE", -1234567890123),
            ("HLAinteger64LE", -1234567890123),
            ("HLAfloat32BE", 1.25),
            ("HLAfloat32LE", -2.5),
            ("HLAfloat64BE", 1.25),
            ("HLAfloat64LE", -2.5),
            ("HLAbyte", -7),
            ("HLAoctet", -2),
            ("HLAASCIIchar", 65),
            ("HLAunicodeChar", 0x03A9),
            ("HLAoctetPairBE", 0x1234),
            ("HLAoctetPairLE", 0x1234),
            ("HLAboolean", True),
            ("HLAASCIIstring", "ASCII"),
            ("HLAunicodeString", "Unicode-Ω"),
        )
        for kind, value in scalar_cases:
            method = getattr(encoder, "create" + kind)
            element = method(value)
            with self.subTest(kind=kind):
                self.assertEqual(element.getValue(), value, kind + " typed value")
                encoded = element.toByteArray()
                self.assertEqual(
                    _as_bytes(
                        self.probe.roundTripDataElement(
                            kind, self.runtime.byte_array(encoded)
                        )
                    ),
                    encoded,
                    kind + " JNI validation",
                )

        opaque = encoder.createHLAopaqueData(b"\x00\x11\xfe\x7f")
        self.assertEqual(_as_bytes(opaque.getValue()), b"\x00\x11\xfe\x7f")
        self.assertEqual(
            _as_bytes(
                self.probe.roundTripDataElement(
                    "HLAopaqueData", self.runtime.byte_array(opaque.toByteArray())
                )
            ),
            opaque.toByteArray(),
        )

        composites = {
            "HLAvariableArray": encoder.createHLAvariableArray(
                self.probe.dataElementFactoryCarrier("HLAoctet")
            ),
            "HLAfixedArray": encoder.createHLAfixedArray(),
            "HLAfixedRecord": encoder.createHLAfixedRecord(),
            "HLAvariantRecord": encoder.createHLAvariantRecord(
                encoder.createHLAinteger32BE(1)
            ),
        }
        for kind, element in composites.items():
            with self.subTest(kind=kind):
                encoded = element.toByteArray()
                self.assertEqual(
                    _as_bytes(self.probe.seedDataElement(kind)), encoded, kind
                )

        for kind, element in {
            **{
                kind: getattr(encoder, "create" + kind)(value)
                for kind, value in scalar_cases
            },
            "HLAopaqueData": encoder.createHLAopaqueData(b"\x00\x11\xfe\x7f"),
            **composites,
        }.items():
            encoded = element.toByteArray()
            window = ByteWrapper(b"\x55" + encoded + b"\xa5", 1, len(encoded) + 1)
            element.decode(window)
            with self.subTest(cursor_kind=kind):
                self.assertEqual(window.getPos(), len(encoded) + 1, kind)
                self.assertEqual(window.remaining(), 1, kind)
                self.assertEqual(window.array()[window.getPos()], 0xA5, kind)
            # Exercise the opposite Python -> JPype -> Java -> JNI direction
            # through the same provider-owned façade.  A raw Java carrier
            # test alone would not prove that the Python ByteWrapper cursor is
            # copied back after an encode operation.
            encoded_window = ByteWrapper(
                b"\x33" + encoded + b"\xa5", 1, len(encoded) + 1
            )
            with self.subTest(encode_cursor_kind=kind):
                self.assertIs(element.encode(encoded_window), element)
                self.assertEqual(encoded_window.getPos(), len(encoded) + 1, kind)
                self.assertEqual(encoded_window.remaining(), 1, kind)
                self.assertEqual(encoded_window.array()[1 : len(encoded) + 1], encoded, kind)
                self.assertEqual(encoded_window.array()[encoded_window.getPos()], 0xA5, kind)
            short_window = ByteWrapper(bytearray(max(len(encoded) - 1, 0)))
            with self.subTest(encode_short_kind=kind):
                with self.assertRaises(EncoderException):
                    element.encode(short_window)
                self.assertEqual(short_window.getPos(), 0, kind)
                self.assertEqual(bytes(short_window.array()), b"\0" * max(len(encoded) - 1, 0), kind)
            with self.subTest(malformed_kind=kind), self.assertRaises(DecoderException):
                element.decode(encoded[:-1])

    def test_python_encoder_facade_forwards_composite_creator_arguments(self) -> None:
        """Keep Java composite creator overloads visible at the Python boundary.

        Composite encodings are provider-owned, so this test intentionally
        checks the carrier shape rather than recomputing bytes in Python.  The
        JNI probe retains the Java factory/varargs arguments and exposes the
        resulting child carriers, which proves that JPype selected the
        standard overload and did not silently discard the values.
        """

        encoder = JavaEncoderFactory(self.probe.encoderFactoryCarrier(), self.runtime)
        element_factory = self.probe.dataElementFactoryCarrier("HLAinteger32BE")
        first = encoder.createHLAinteger32BE(17)
        second = encoder.createHLAinteger32BE(-23)

        fixed_from_values = encoder.createHLAfixedArray(first, second)
        self.assertEqual(fixed_from_values.size(), 2)
        self.assertEqual(fixed_from_values.get(0).getValue(), 17)
        self.assertEqual(fixed_from_values.get(1).getValue(), -23)

        fixed_from_factory = encoder.createHLAfixedArray(element_factory, 3)
        self.assertEqual(fixed_from_factory.size(), 3)
        for index in range(fixed_from_factory.size()):
            with self.subTest(fixed_factory_index=index):
                # The probe's DataElementFactory creates its native-seeded
                # HLAinteger32BE carrier for each requested index.
                self.assertEqual(fixed_from_factory.get(index).getValue(), -123456)

        variable = encoder.createHLAvariableArray(element_factory, first, second)
        self.assertEqual(variable.size(), 2)
        self.assertEqual(variable.get(0).getValue(), 17)
        self.assertEqual(variable.get(1).getValue(), -23)

        empty_variable = encoder.createHLAvariableArray(element_factory)
        self.assertEqual(empty_variable.size(), 0)

        record = encoder.createHLAfixedRecord()
        record.add(first)
        record.add(second)
        self.assertEqual(record.size(), 2)
        self.assertEqual(record.get(0).getValue(), 17)
        self.assertEqual(record.get(1).getValue(), -23)

        discriminant = encoder.createHLAinteger32BE(1)
        variant = encoder.createHLAvariantRecord(discriminant)
        self.assertEqual(variant.getDiscriminant().getValue(), 1)
        variant.setVariant(discriminant, second)
        self.assertEqual(variant.getValue().getValue(), -23)

    def test_python_basic_data_element_value_matrix_round_trips_through_jni(
        self,
    ) -> None:
        """Use the same provider-neutral edge values as the direct native route."""
        encoder = JavaEncoderFactory(self.probe.encoderFactoryCarrier(), self.runtime)
        signed_byte_kinds = {"HLAbyte", "HLAoctet", "HLAASCIIchar"}
        signed_short_kinds = {"HLAunicodeChar", "HLAoctetPairBE", "HLAoctetPairLE"}
        for vector in iter_data_element_value_matrix("2010"):
            with self.subTest(case=vector.case_id):
                method = getattr(encoder, "create" + vector.kind)
                element = method(vector.value)
                encoded = _as_bytes(element.toByteArray())
                self.assertEqual(
                    _as_bytes(
                        self.probe.roundTripDataElement(
                            vector.kind, self.runtime.byte_array(encoded)
                        )
                    ),
                    encoded,
                    vector.kind + " JNI validation",
                )
                decoded = method()
                decoded.decode(self.runtime.byte_array(encoded))
                self.assertEqual(_as_bytes(decoded.toByteArray()), encoded)
                actual = decoded.getValue()
                if vector.kind in signed_byte_kinds:
                    self.assertEqual(int(actual) & 0xFF, int(vector.value) & 0xFF)
                elif vector.kind in signed_short_kinds:
                    self.assertEqual(int(actual) & 0xFFFF, int(vector.value) & 0xFFFF)
                elif vector.kind.startswith("HLAfloat32"):
                    self.assertEqual(
                        struct.pack(">f", float(actual)),
                        struct.pack(">f", float(vector.value)),
                    )
                elif vector.kind.startswith("HLAfloat64"):
                    self.assertEqual(
                        struct.pack(">d", float(actual)),
                        struct.pack(">d", float(vector.value)),
                    )
                elif vector.kind == "HLAopaqueData":
                    self.assertEqual(_as_bytes(actual), bytes(vector.value))
                else:
                    self.assertEqual(actual, vector.value)

                window = ByteWrapper(b"\x55" + encoded + b"\xa5", 1, len(encoded) + 1)
                element.decode(window)
                self.assertEqual(window.getPos(), len(encoded) + 1)
                self.assertEqual(window.remaining(), 1)
                self.assertEqual(window.array()[window.getPos()], 0xA5)

    def test_python_logical_time_wire_matrix_round_trips_through_jni(
        self,
    ) -> None:
        """Exercise shared logical-time vectors through C++ JNI and JPype.

        ``NativeTypeRoundTrip`` exposes the standard Java factory returned by
        the C++ JNI null provider. Wrapping that exact factory in
        ``Java2010TimeFactory`` proves that Python decoding, offset handling,
        canonical encoding, and typed failures preserve the Java/C++ carrier
        instead of introducing a second Python wire format. Arithmetic remains
        an explicit deferred capability of this ABI/null-provider profile; the
        provider-neutral Java fixture and direct native profile cover that
        behavior separately.
        """

        for factory_kind in ("HLAinteger64TimeFactory", "HLAfloat64TimeFactory"):
            implementation = factory_kind.removesuffix("Factory")
            with self.subTest(implementation=implementation):
                raw_factory = self.probe.logicalTimeFactoryCarrier(factory_kind)
                factory = Java2010TimeFactory(self.probe, self.runtime, raw_factory)
                for vector in iter_logical_time_wire_matrix((implementation,)):
                    with self.subTest(vector=vector.case_id):
                        if vector.valid:
                            carrier = b"\x5a" + vector.encoded + b"\xa5"
                            decoded_time = factory.decodeTime(carrier, 1)
                            decoded_interval = factory.decodeInterval(carrier, 1)
                            self.assertEqual(decoded_time.getTime(), vector.expected)
                            self.assertEqual(
                                decoded_interval.getInterval(), vector.expected
                            )
                            # C++ owns canonical wire output. In particular,
                            # IEEE floating signed zero is normalized by the
                            # native decoder even though the input matrix keeps
                            # the raw negative-zero edge visible.
                            canonical = _as_bytes(
                                self.probe.roundTripLogicalTime(
                                    implementation,
                                    self.runtime.byte_array(vector.encoded),
                                )
                            )
                            self.assertEqual(decoded_time.encodedValue, canonical)
                            self.assertEqual(decoded_interval.encodedValue, canonical)
                        elif vector.label == "trailing":
                            # The 2010 offset overload selects exactly eight
                            # octets and leaves surrounding bytes to its caller.
                            window = b"\x5a" + vector.encoded[:8] + b"\xa5"
                            self.assertEqual(factory.decodeTime(window, 1).getTime(), 0)
                            self.assertEqual(
                                factory.decodeInterval(window, 1).getInterval(), 0
                            )
                            canonical = _as_bytes(
                                self.probe.roundTripLogicalTime(
                                    implementation,
                                    self.runtime.byte_array(vector.encoded[:8]),
                                )
                            )
                            self.assertEqual(
                                factory.decodeTime(window, 1).encodedValue,
                                canonical,
                            )
                            self.assertEqual(
                                factory.decodeInterval(window, 1).encodedValue,
                                canonical,
                            )
                        else:
                            with self.assertRaises(CouldNotDecode):
                                factory.decodeTime(vector.encoded)
                            with self.assertRaises(CouldNotDecode):
                                factory.decodeInterval(vector.encoded)

    def test_python_logical_time_operation_surface_is_callable_through_jni(
        self,
    ) -> None:
        """Keep 2010 carrier arithmetic calls on the Java/JNI object.

        The null-provider carrier intentionally does not claim arithmetic
        semantics.  This test is only a shape check: every standard time or
        interval operation is callable from Python, returns the expected
        standard carrier category, and therefore reaches the Java carrier
        instead of being silently reimplemented in Python.
        """

        for factory_kind in ("HLAinteger64TimeFactory", "HLAfloat64TimeFactory"):
            implementation = factory_kind.removesuffix("Factory")
            with self.subTest(implementation=implementation):
                raw_factory = self.probe.logicalTimeFactoryCarrier(factory_kind)
                factory = Java2010TimeFactory(self.probe, self.runtime, raw_factory)
                time = factory.makeTime(5 if implementation.startswith("HLAinteger") else 5.5)
                other_time = factory.makeTime(
                    3 if implementation.startswith("HLAinteger") else 3.5
                )
                interval = factory.makeInterval(
                    2 if implementation.startswith("HLAinteger") else 2.0
                )
                time_type = (
                    HLAinteger64Time
                    if implementation.startswith("HLAinteger")
                    else HLAfloat64Time
                )
                interval_type = (
                    HLAinteger64Interval
                    if implementation.startswith("HLAinteger")
                    else HLAfloat64Interval
                )
                for vector in iter_vendor_time_arithmetic_matrix():
                    with self.subTest(operation=vector.case_id):
                        receiver = time if vector.receiver == "time" else interval
                        argument = other_time if vector.argument == "time" else interval
                        result = getattr(receiver, vector.operation)(argument)
                        if vector.operation == "compareTo":
                            self.assertIsInstance(result, int)
                            continue
                        expected_type = (
                            interval_type
                            if vector.operation == "distance"
                            or vector.receiver == "interval"
                            else time_type
                        )
                        self.assertIsInstance(result, expected_type)
                        self.assertEqual(result.implementationName(), implementation)

    def test_python_handle_and_collection_facades_round_trip_through_jni(
        self,
    ) -> None:
        """Exercise Python handle/set/map carriers over the JNI probe.

        The null RTI ambassador intentionally has no service-state factory
        methods.  This small standard-shaped owner supplies the exact probe
        factories instead, allowing the public Python adapter conversion code
        to be tested without inventing a second provider API or claiming RTI
        service behavior.
        """

        class _CarrierOwner:
            def __init__(self, probe: object) -> None:
                self._probe = probe

            def _handle_factory(self, kind: str) -> object:
                return self._probe.handleFactoryCarrier(kind + "Factory")

            def _collection_factory(self, kind: str) -> object:
                return self._probe.collectionFactoryCarrier(kind + "Factory")

            def getFederateHandleFactory(self) -> object:
                return self._handle_factory("FederateHandle")

            def getObjectClassHandleFactory(self) -> object:
                return self._handle_factory("ObjectClassHandle")

            def getObjectInstanceHandleFactory(self) -> object:
                return self._handle_factory("ObjectInstanceHandle")

            def getAttributeHandleFactory(self) -> object:
                return self._handle_factory("AttributeHandle")

            def getInteractionClassHandleFactory(self) -> object:
                return self._handle_factory("InteractionClassHandle")

            def getParameterHandleFactory(self) -> object:
                return self._handle_factory("ParameterHandle")

            def getTransportationTypeHandleFactory(self) -> object:
                return self._handle_factory("TransportationTypeHandle")

            def getDimensionHandleFactory(self) -> object:
                return self._handle_factory("DimensionHandle")

            def getAttributeHandleSetFactory(self) -> object:
                return self._collection_factory("AttributeHandleSet")

            def getDimensionHandleSetFactory(self) -> object:
                return self._collection_factory("DimensionHandleSet")

            def getFederateHandleSetFactory(self) -> object:
                return self._collection_factory("FederateHandleSet")

            def getRegionHandleSetFactory(self) -> object:
                return self._collection_factory("RegionHandleSet")

            def getAttributeHandleValueMapFactory(self) -> object:
                return self._collection_factory("AttributeHandleValueMap")

            def getParameterHandleValueMapFactory(self) -> object:
                return self._collection_factory("ParameterHandleValueMap")

            def getAttributeSetRegionSetPairListFactory(self) -> object:
                return self._collection_factory("AttributeSetRegionSetPairList")

        owner = _CarrierOwner(self.probe)
        ambassador = Java2010RTIambassador(owner, self.runtime)
        handle_specs = (
            ("FederateHandle", FederateHandle, "getFederateHandleFactory"),
            ("ObjectClassHandle", ObjectClassHandle, "getObjectClassHandleFactory"),
            (
                "ObjectInstanceHandle",
                ObjectInstanceHandle,
                "getObjectInstanceHandleFactory",
            ),
            ("AttributeHandle", AttributeHandle, "getAttributeHandleFactory"),
            (
                "InteractionClassHandle",
                InteractionClassHandle,
                "getInteractionClassHandleFactory",
            ),
            ("ParameterHandle", ParameterHandle, "getParameterHandleFactory"),
            (
                "TransportationTypeHandle",
                TransportationTypeHandle,
                "getTransportationTypeHandleFactory",
            ),
            ("DimensionHandle", DimensionHandle, "getDimensionHandleFactory"),
        )
        decoded_handles: dict[str, object] = {}
        for kind, handle_type, factory_method in handle_specs:
            with self.subTest(handle=kind):
                raw = self.probe.handleCarrier(kind, 0x0123456789ABCDEF)
                encoded = self.runtime.handle_bytes(raw)
                factory = getattr(ambassador, factory_method)()
                decoded = factory.decode(encoded)
                self.assertIsInstance(decoded, handle_type)
                self.assertEqual(decoded.encodedValue, encoded)
                decoded_handles[kind] = decoded

        transportation_factory = ambassador.getTransportationTypeHandleFactory()
        self.assertEqual(
            self.runtime.handle_bytes(transportation_factory.getHLAdefaultReliable()),
            self.runtime.handle_bytes(
                self.probe.handleCarrier("TransportationTypeHandle", 1)
            ),
        )
        self.assertEqual(
            self.runtime.handle_bytes(transportation_factory.getHLAdefaultBestEffort()),
            self.runtime.handle_bytes(
                self.probe.handleCarrier("TransportationTypeHandle", 2)
            ),
        )

        region_raw = self.probe.handleCarrier("RegionHandle", 31)
        ambassador._remember_raw_handle(RegionHandle, region_raw)
        region = RegionHandle(self.runtime.handle_bytes(region_raw))
        returned_region_raw = ambassador._convert_argument(region, "RegionHandle")
        returned_region = self.probe.roundTripHandleCarrier(
            "RegionHandle", returned_region_raw
        )
        self.assertEqual(
            self.runtime.handle_bytes(returned_region),
            self.runtime.handle_bytes(region_raw),
            "RegionHandle opaque carrier round-trip",
        )

        retraction_raw = self.probe.handleCarrier("MessageRetractionHandle", 32)
        ambassador._remember_raw_handle(MessageRetractionHandle, retraction_raw)
        retraction = MessageRetractionHandle(
            self.runtime.handle_bytes(retraction_raw)
        )
        returned_retraction_raw = ambassador._convert_argument(
            retraction, "MessageRetractionHandle"
        )
        returned_retraction = self.probe.roundTripHandleCarrier(
            "MessageRetractionHandle", returned_retraction_raw
        )
        self.assertEqual(
            self.runtime.handle_bytes(returned_retraction),
            self.runtime.handle_bytes(retraction_raw),
            "MessageRetractionHandle opaque carrier round-trip",
        )

        attribute = decoded_handles["AttributeHandle"]
        attribute_set = AttributeHandleSet((attribute,))
        region_set = RegionHandleSet((region,))

        set_specs = (
            (
                "AttributeHandleSet",
                attribute_set,
                {attribute.encodedValue},
            ),
            (
                "DimensionHandleSet",
                DimensionHandleSet((decoded_handles["DimensionHandle"],)),
                {decoded_handles["DimensionHandle"].encodedValue},
            ),
            (
                "FederateHandleSet",
                FederateHandleSet((decoded_handles["FederateHandle"],)),
                {decoded_handles["FederateHandle"].encodedValue},
            ),
            ("RegionHandleSet", region_set, {region.encodedValue}),
        )
        for kind, value, expected in set_specs:
            with self.subTest(collection=kind):
                raw_set = ambassador._convert_argument(value, kind)
                returned_set = self.probe.roundTripCollectionCarrier(kind, raw_set)
                converted_set = self.runtime.from_java_value(returned_set, kind)
                self.assertEqual(
                    {item.encodedValue for item in converted_set},
                    expected,
                )

        information_type = self.jpype.JClass(
            "hla.rti1516e.FederationExecutionInformation"
        )
        raw_information_set = self.probe.collectionCarrier(
            "FederationExecutionInformationSet"
        )
        raw_information_set.add(information_type("jni-federation", "HLAinteger64Time"))
        returned_information_set = self.probe.roundTripCollectionCarrier(
            "FederationExecutionInformationSet", raw_information_set
        )
        converted_information_set = self.runtime.from_java_value(
            returned_information_set, "FederationExecutionInformationSet"
        )
        self.assertEqual(
            converted_information_set,
            FederationExecutionInformationSet(
                (FederationExecutionInformation("jni-federation", "HLAinteger64Time"),)
            ),
        )

        attribute_map = AttributeHandleValueMap({attribute: b"jni-2010-value"})
        raw_attribute_map = ambassador._convert_argument(
            attribute_map, "AttributeHandleValueMap"
        )
        returned_attribute_map = self.probe.roundTripCollectionCarrier(
            "AttributeHandleValueMap", raw_attribute_map
        )
        converted_attribute_map = self.runtime.from_java_value(
            returned_attribute_map, "AttributeHandleValueMap"
        )
        self.assertEqual(dict(converted_attribute_map), {attribute: b"jni-2010-value"})

        parameter = ParameterHandle(
            self.runtime.handle_bytes(self.probe.handleCarrier("ParameterHandle", 17))
        )
        parameter_map = ParameterHandleValueMap({parameter: b"jni-2010-parameter"})
        raw_parameter_map = ambassador._convert_argument(
            parameter_map, "ParameterHandleValueMap"
        )
        returned_parameter_map = self.probe.roundTripCollectionCarrier(
            "ParameterHandleValueMap", raw_parameter_map
        )
        converted_parameter_map = self.runtime.from_java_value(
            returned_parameter_map, "ParameterHandleValueMap"
        )
        self.assertEqual(
            dict(converted_parameter_map), {parameter: b"jni-2010-parameter"}
        )

        pair_list = AttributeSetRegionSetPairList(
            [AttributeRegionAssociation(attribute_set, region_set)]
        )
        raw_pair_list = ambassador._convert_argument(
            pair_list, "AttributeSetRegionSetPairList"
        )
        returned_pair_list = self.probe.roundTripCollectionCarrier(
            "AttributeSetRegionSetPairList", raw_pair_list
        )
        self.assertEqual(returned_pair_list.size(), 1)
        returned_pair = returned_pair_list.get(0)
        self.assertEqual(returned_pair.ahset.size(), 1)
        self.assertEqual(returned_pair.rhset.size(), 1)

    def test_python_callback_and_record_carriers_round_trip_through_jni(
        self,
    ) -> None:
        """Convert standard Java callback/record values into Python snapshots."""

        for kind, expected_type in (
            ("SupplementalReflectInfo", "SupplementalReflectInfo"),
            ("SupplementalReceiveInfo", "SupplementalReceiveInfo"),
            ("SupplementalRemoveInfo", "SupplementalRemoveInfo"),
        ):
            with self.subTest(callback=kind):
                raw = self.probe.callbackCarrier(kind)
                returned = self.probe.roundTripObject(raw)
                converted = self.runtime.from_java_value(returned, expected_type)
                self.assertTrue(converted.hasProducingFederate())
                self.assertIsInstance(converted.producingFederate, FederateHandle)
                self.assertEqual(
                    converted.producingFederate.encodedValue,
                    self.runtime.handle_bytes(returned.getProducingFederate()),
                )
                if expected_type != "SupplementalRemoveInfo":
                    self.assertFalse(converted.hasSentRegions())

        bounds = self.jpype.JClass("hla.rti1516e.RangeBounds")(4, 9)
        converted_bounds = self.runtime.from_java_value(
            self.probe.roundTripObject(bounds), "RangeBounds"
        )
        self.assertEqual(converted_bounds, RangeBounds(4, 9))

        information = self.jpype.JClass("hla.rti1516e.FederationExecutionInformation")(
            "jni-federation", "HLAinteger64Time"
        )
        converted_information = self.runtime.from_java_value(
            self.probe.roundTripObject(information),
            "FederationExecutionInformation",
        )
        self.assertEqual(
            converted_information,
            FederationExecutionInformation("jni-federation", "HLAinteger64Time"),
        )

        raw_federate = self.probe.handleCarrier("FederateHandle", 41)
        raw_retraction = self.probe.handleCarrier("MessageRetractionHandle", 42)
        raw_message = self.jpype.JClass("hla.rti1516e.MessageRetractionReturn")(
            True, raw_retraction
        )
        converted_message = self.runtime.from_java_value(
            self.probe.roundTripObject(raw_message), "MessageRetractionReturn"
        )
        self.assertIsInstance(converted_message, MessageRetractionReturn)
        self.assertTrue(converted_message.retractionHandleIsValid)
        self.assertEqual(
            converted_message.handle.encodedValue,
            self.runtime.handle_bytes(raw_retraction),
        )

        raw_time = self.probe.logicalTimeCarrier("HLAinteger64Time")
        raw_query = self.jpype.JClass("hla.rti1516e.TimeQueryReturn")(True, raw_time)
        converted_query = self.runtime.from_java_value(
            self.probe.roundTripObject(raw_query), "TimeQueryReturn"
        )
        self.assertIsInstance(converted_query, TimeQueryReturn)
        self.assertTrue(converted_query.timeIsValid)
        self.assertEqual(converted_query.time.getTime(), raw_time.getValue())

        save_status = self.jpype.JClass("hla.rti1516e.SaveStatus").values()[0]
        save_pair = self.jpype.JClass("hla.rti1516e.FederateHandleSaveStatusPair")(
            raw_federate, save_status
        )
        save_pair_type = self.jpype.JClass("hla.rti1516e.FederateHandleSaveStatusPair")
        save_array = self.jpype.JArray(save_pair_type)([save_pair])
        converted_save = self.runtime.from_java_value(
            save_array, "FederateHandleSaveStatusPair[]"
        )
        self.assertEqual(len(converted_save), 1)
        self.assertIsInstance(converted_save[0], FederateHandleSaveStatusPair)
        self.assertEqual(
            converted_save[0].handle.encodedValue,
            self.runtime.handle_bytes(raw_federate),
        )
        self.assertIsInstance(converted_save[0].status, SaveStatus)

        raw_post_restore = self.probe.handleCarrier("FederateHandle", 43)
        restore_status = self.jpype.JClass("hla.rti1516e.RestoreStatus").values()[0]
        restore_pair = self.jpype.JClass("hla.rti1516e.FederateRestoreStatus")(
            raw_federate, raw_post_restore, restore_status
        )
        restore_pair_type = self.jpype.JClass("hla.rti1516e.FederateRestoreStatus")
        restore_array = self.jpype.JArray(restore_pair_type)([restore_pair])
        converted_restore = self.runtime.from_java_value(
            restore_array, "FederateRestoreStatus[]"
        )
        self.assertEqual(len(converted_restore), 1)
        self.assertIsInstance(converted_restore[0], FederateRestoreStatus)
        self.assertEqual(
            converted_restore[0].postRestoreHandle.encodedValue,
            self.runtime.handle_bytes(raw_post_restore),
        )
        self.assertIsInstance(converted_restore[0].status, RestoreStatus)

    def test_python_unknown_vendor_time_carrier_remains_raw_through_jni(self) -> None:
        """Preserve a provider-specific time object without coercion."""

        for expected_type, text in (
            ("LogicalTime", "vendor-logical-time"),
            ("LogicalTimeInterval", "vendor-logical-interval"),
        ):
            with self.subTest(expected_type=expected_type):
                raw = self.jpype.JString(text)
                returned = self.probe.roundTripObject(raw)
                converted = self.runtime.from_java_value(returned, expected_type)
                self.assertEqual(str(converted), text)
                self.assertNotIsInstance(
                    converted,
                    (
                        HLAinteger64Time,
                        HLAfloat64Time,
                        HLAinteger64Interval,
                        HLAfloat64Interval,
                    ),
                )

    def test_python_unknown_vendor_data_element_preserves_jni_carrier(self) -> None:
        """Keep an unknown Java DataElement raw while preserving its cursor."""

        raw = self.probe.vendorDataElementCarrier()
        returned = self.probe.roundTripObject(raw)
        element = _wrap_element(returned, self.runtime)
        self.assertIsInstance(element, DataElement)
        self.assertEqual(type(element).__name__, "_JavaDataElement")
        self.assertEqual(element.getOctetBoundary(), 1)
        self.assertEqual(element.getEncodedLength(), 4)
        self.assertEqual(element.toByteArray(), b"abcd")
        self.assertEqual(element.encode(), b"abcd")

        encoded_window = ByteWrapper(b"\0" * 6, 1, 4)
        self.assertIs(element.encode(encoded_window), element)
        self.assertEqual(bytes(encoded_window.array()), b"\0abcd\0")
        self.assertEqual(encoded_window.getPos(), 5)
        self.assertEqual(encoded_window.remaining(), 0)

        for vector in iter_vendor_data_element_encode_matrix():
            with self.subTest(encode_vector=vector.case_id):
                target = _wrap_element(
                    self.probe.vendorDataElementCarrier(), self.runtime
                )
                wrapper = ByteWrapper(vector.backing, vector.offset, vector.length)
                before = bytes(wrapper.array())
                if vector.valid:
                    self.assertIs(target.encode(wrapper), target)
                    expected = bytearray(vector.backing)
                    expected[vector.offset : vector.offset + 4] = b"abcd"
                    self.assertEqual(bytes(wrapper.array()), bytes(expected))
                    self.assertEqual(wrapper.getPos(), vector.offset + 4)
                    self.assertEqual(wrapper.remaining(), vector.length - 4)
                else:
                    with self.assertRaises(EncoderException):
                        target.encode(wrapper)
                    self.assertEqual(bytes(wrapper.array()), before)
                    self.assertEqual(wrapper.getPos(), vector.offset)

        window = ByteWrapper(b"!wxyz?", 1, 5)
        element.decode(window)
        self.assertEqual(element.toByteArray(), b"wxyz")
        self.assertEqual(window.getPos(), 5)
        self.assertEqual(window.remaining(), 1)

        for vector in iter_vendor_data_element_wire_matrix():
            with self.subTest(vector=vector.case_id):
                target = _wrap_element(
                    self.probe.roundTripObject(
                        self.probe.vendorDataElementCarrier()
                    ),
                    self.runtime,
                )
                if vector.valid:
                    target.decode(vector.payload)
                    self.assertEqual(target.toByteArray(), vector.payload)
                else:
                    with self.assertRaises(DecoderException):
                        target.decode(vector.payload)
                    self.assertEqual(target.toByteArray(), b"abcd")

    def test_python_callback_retraction_handle_round_trips_through_jni(self) -> None:
        """Preserve an opaque retraction handle across the callback boundary."""

        class Callback(NullFederateAmbassador):
            received: MessageRetractionHandle | None = None

            def requestRetraction(self, retraction: MessageRetractionHandle) -> None:
                self.received = retraction

        callback = Callback()
        binding = self.runtime.bind_federate_ambassador(callback)
        raw_retraction = self.probe.handleCarrier("MessageRetractionHandle", 53)

        # Invoke the same target used by the real JPype JProxy.  The raw Java
        # carrier was produced by JNI/C++, so this exercises Java -> Python
        # conversion for the callback's only standalone opaque handle type.
        binding.target.requestRetraction(raw_retraction)
        self.assertIsInstance(callback.received, MessageRetractionHandle)
        self.assertEqual(
            callback.received.encodedValue,  # type: ignore[union-attr]
            self.runtime.handle_bytes(raw_retraction),
        )

    def test_python_callback_overload_carriers_round_trip_through_jni(self) -> None:
        """Exercise every 2010 object/interaction/remove callback carrier shape."""

        class Callback(NullFederateAmbassador):
            def __init__(self) -> None:
                self.calls: dict[str, tuple[object, ...]] = {}

            def reflectAttributeValues(self, *args: object) -> None:
                self.calls["reflect"] = args

            def receiveInteraction(self, *args: object) -> None:
                self.calls["receive"] = args

            def removeObjectInstance(self, *args: object) -> None:
                self.calls["remove"] = args

        callback = Callback()
        binding = self.runtime.bind_federate_ambassador(callback)
        raw_object = self.probe.handleCarrier("ObjectInstanceHandle", 61)
        raw_interaction = self.probe.handleCarrier("InteractionClassHandle", 62)
        raw_attribute = self.probe.handleCarrier("AttributeHandle", 63)
        raw_parameter = self.probe.handleCarrier("ParameterHandle", 64)
        raw_transport = self.probe.handleCarrier("TransportationTypeHandle", 1)
        raw_retraction = self.probe.handleCarrier("MessageRetractionHandle", 66)
        raw_time = self.probe.logicalTimeCarrier("HLAinteger64Time")
        order = self.jpype.JClass("hla.rti1516e.OrderType").RECEIVE
        raw_reflect_info = self.probe.callbackCarrier("SupplementalReflectInfo")
        raw_receive_info = self.probe.callbackCarrier("SupplementalReceiveInfo")
        raw_remove_info = self.probe.callbackCarrier("SupplementalRemoveInfo")

        raw_attributes = self.probe.collectionCarrier("AttributeHandleValueMap")
        raw_attributes.put(raw_attribute, self.runtime.byte_array(b"reflect"))
        raw_parameters = self.probe.collectionCarrier("ParameterHandleValueMap")
        raw_parameters.put(raw_parameter, self.runtime.byte_array(b"receive"))

        target = binding.target
        target.reflectAttributeValues(
            raw_object,
            raw_attributes,
            self.runtime.byte_array(b"reflect-tag"),
            order,
            raw_transport,
            raw_reflect_info,
        )
        target.reflectAttributeValues(
            raw_object,
            raw_attributes,
            self.runtime.byte_array(b"reflect-time"),
            order,
            raw_transport,
            raw_time,
            order,
            raw_reflect_info,
        )
        target.reflectAttributeValues(
            raw_object,
            raw_attributes,
            self.runtime.byte_array(b"reflect-retraction"),
            order,
            raw_transport,
            raw_time,
            order,
            raw_retraction,
            raw_reflect_info,
        )
        target.receiveInteraction(
            raw_interaction,
            raw_parameters,
            self.runtime.byte_array(b"receive-tag"),
            order,
            raw_transport,
            raw_receive_info,
        )
        target.receiveInteraction(
            raw_interaction,
            raw_parameters,
            self.runtime.byte_array(b"receive-time"),
            order,
            raw_transport,
            raw_time,
            order,
            raw_receive_info,
        )
        target.receiveInteraction(
            raw_interaction,
            raw_parameters,
            self.runtime.byte_array(b"receive-retraction"),
            order,
            raw_transport,
            raw_time,
            order,
            raw_retraction,
            raw_receive_info,
        )
        target.removeObjectInstance(
            raw_object,
            self.runtime.byte_array(b"remove-tag"),
            order,
            raw_remove_info,
        )
        target.removeObjectInstance(
            raw_object,
            self.runtime.byte_array(b"remove-time"),
            order,
            raw_time,
            order,
            raw_remove_info,
        )
        target.removeObjectInstance(
            raw_object,
            self.runtime.byte_array(b"remove-retraction"),
            order,
            raw_time,
            order,
            raw_retraction,
            raw_remove_info,
        )

        reflect = callback.calls["reflect"]
        self.assertEqual(len(reflect), 9)
        self.assertIsInstance(reflect[0], ObjectInstanceHandle)
        self.assertIsInstance(reflect[1], AttributeHandleValueMap)
        self.assertEqual(reflect[1][AttributeHandle(self.runtime.handle_bytes(raw_attribute))], b"reflect")
        self.assertEqual(reflect[2], b"reflect-retraction")
        self.assertIs(reflect[3], OrderType.RECEIVE)
        self.assertIsInstance(reflect[4], TransportationTypeHandle)
        self.assertEqual(reflect[5].getTime(), raw_time.getValue())
        self.assertIs(reflect[6], OrderType.RECEIVE)
        self.assertIsInstance(reflect[7], MessageRetractionHandle)
        self.assertIsInstance(reflect[8], type(self.runtime.from_java_value(raw_reflect_info, "SupplementalReflectInfo")))

        receive = callback.calls["receive"]
        self.assertEqual(len(receive), 9)
        self.assertIsInstance(receive[0], InteractionClassHandle)
        self.assertIsInstance(receive[1], ParameterHandleValueMap)
        self.assertEqual(receive[1][ParameterHandle(self.runtime.handle_bytes(raw_parameter))], b"receive")
        self.assertEqual(receive[2], b"receive-retraction")
        self.assertIsInstance(receive[4], TransportationTypeHandle)
        self.assertIsInstance(receive[7], MessageRetractionHandle)

        remove = callback.calls["remove"]
        self.assertEqual(len(remove), 7)
        self.assertIsInstance(remove[0], ObjectInstanceHandle)
        self.assertEqual(remove[1], b"remove-retraction")
        self.assertIs(remove[2], OrderType.RECEIVE)
        self.assertEqual(remove[3].getTime(), raw_time.getValue())
        self.assertIs(remove[4], OrderType.RECEIVE)
        self.assertIsInstance(remove[5], MessageRetractionHandle)

    def test_python_callback_surface_matrix_round_trips_through_jni(self) -> None:
        """Convert every standard 2010 callback overload through JPype.

        The focused object/interaction test above checks the most complicated
        overloads in detail.  This matrix closes the rest of the callback
        surface: every callback name and all 60 generated overloads receive
        standard Java carriers produced by the JNI probe, and the Python
        callback must receive the corresponding provider-neutral type.  The
        integer and float logical-time carriers are both selected for the
        overloads that contain ``LogicalTime``.
        """

        calls: list[tuple[str, tuple[object, ...]]] = []
        callback = NullFederateAmbassador()
        for name in FEDERATE_AMBASSADOR_METHODS:
            setattr(
                callback,
                name,
                lambda *args, _name=name: calls.append((_name, args)),
            )
        binding = self.runtime.bind_federate_ambassador(callback)

        handle_seeds = {
            "FederateHandle": 11,
            "ObjectClassHandle": 12,
            "ObjectInstanceHandle": 13,
            "AttributeHandle": 14,
            "InteractionClassHandle": 15,
            "ParameterHandle": 16,
            "TransportationTypeHandle": 17,
            "DimensionHandle": 18,
            "MessageRetractionHandle": 19,
            "RegionHandle": 20,
        }
        set_handle_kinds = {
            "AttributeHandleSet": "AttributeHandle",
            "DimensionHandleSet": "DimensionHandle",
            "FederateHandleSet": "FederateHandle",
            "RegionHandleSet": "RegionHandle",
        }

        def raw_value(expected_type: str, time_kind: str) -> object:
            if expected_type in handle_seeds:
                return self.probe.handleCarrier(
                    expected_type, handle_seeds[expected_type]
                )
            if expected_type == "LogicalTime":
                return self.probe.logicalTimeCarrier(time_kind)
            if expected_type == "LogicalTimeInterval":
                return self.probe.logicalTimeCarrier(
                    time_kind.replace("Time", "Interval")
                )
            if expected_type == "byte[]":
                return self.runtime.byte_array(b"callback-matrix")
            if expected_type == "String":
                return "callback-matrix"
            if expected_type == "Set<String>":
                values = self.jpype.JClass("java.util.LinkedHashSet")()
                values.add("callback-matrix")
                return values
            if expected_type in set_handle_kinds:
                values = self.probe.collectionCarrier(expected_type)
                handle_kind = set_handle_kinds[expected_type]
                values.add(self.probe.handleCarrier(handle_kind, 21))
                return values
            if expected_type in {
                "AttributeHandleValueMap",
                "ParameterHandleValueMap",
            }:
                values = self.probe.collectionCarrier(expected_type)
                handle_kind = (
                    "AttributeHandle"
                    if expected_type.startswith("Attribute")
                    else "ParameterHandle"
                )
                values.put(
                    self.probe.handleCarrier(handle_kind, 22),
                    self.runtime.byte_array(b"callback-value"),
                )
                return values
            if expected_type == "FederationExecutionInformationSet":
                values = self.probe.collectionCarrier(expected_type)
                information = self.jpype.JClass(
                    "hla.rti1516e.FederationExecutionInformation"
                )("callback-federation", time_kind)
                values.add(information)
                return values
            if expected_type in {
                "CallbackModel",
                "OrderType",
                "ResignAction",
                "ServiceGroup",
                "SynchronizationPointFailureReason",
                "SaveFailureReason",
                "SaveStatus",
                "RestoreFailureReason",
                "RestoreStatus",
            }:
                return self.jpype.JClass(
                    "hla.rti1516e." + expected_type
                ).values()[0]
            if expected_type == "FederateHandleSaveStatusPair[]":
                pair_type = self.jpype.JClass(
                    "hla.rti1516e.FederateHandleSaveStatusPair"
                )
                pair = pair_type(
                    self.probe.handleCarrier("FederateHandle", 23),
                    self.jpype.JClass("hla.rti1516e.SaveStatus").values()[0],
                )
                return self.jpype.JArray(pair_type)([pair])
            if expected_type == "FederateRestoreStatus[]":
                pair_type = self.jpype.JClass(
                    "hla.rti1516e.FederateRestoreStatus"
                )
                pair = pair_type(
                    self.probe.handleCarrier("FederateHandle", 24),
                    self.probe.handleCarrier("FederateHandle", 25),
                    self.jpype.JClass("hla.rti1516e.RestoreStatus").values()[0],
                )
                return self.jpype.JArray(pair_type)([pair])
            if expected_type in {
                "SupplementalReflectInfo",
                "SupplementalReceiveInfo",
                "SupplementalRemoveInfo",
            }:
                return self.probe.callbackCarrier(expected_type)
            raise AssertionError(
                "callback matrix has no JNI carrier for " + expected_type
            )

        def assert_python_type(expected_type: str, value: object) -> None:
            if expected_type == "LogicalTime":
                self.assertIsInstance(value, (HLAinteger64Time, HLAfloat64Time))
            elif expected_type == "LogicalTimeInterval":
                self.assertIn(
                    type(value).__name__,
                    {"HLAinteger64Interval", "HLAfloat64Interval"},
                )
            elif expected_type == "byte[]":
                self.assertIsInstance(value, bytes)
            elif expected_type == "String":
                self.assertIsInstance(value, str)
            elif expected_type == "Set<String>":
                self.assertIsInstance(value, frozenset)
            elif expected_type.endswith("[]"):
                self.assertIsInstance(value, tuple)
            else:
                self.assertEqual(type(value).__name__, expected_type)

        expected_calls = 0
        for time_kind in ("HLAinteger64Time", "HLAfloat64Time"):
            for name in FEDERATE_AMBASSADOR_METHODS:
                for overload in FEDERATE_AMBASSADOR_PARAMETER_TYPES[name]:
                    arguments = tuple(
                        raw_value(expected_type, time_kind)
                        for expected_type in overload
                    )
                    before = len(calls)
                    getattr(binding.target, name)(*arguments)
                    self.assertEqual(len(calls), before + 1, name)
                    returned_name, returned = calls[-1]
                    self.assertEqual(returned_name, name)
                    self.assertEqual(len(returned), len(overload), name)
                    for expected_type, value in zip(
                        overload, returned, strict=True
                    ):
                        with self.subTest(
                            callback=name,
                            overload=overload,
                            expected_type=expected_type,
                            time_kind=time_kind,
                        ):
                            assert_python_type(expected_type, value)
                    expected_calls += 1

        self.assertEqual(len(FEDERATE_AMBASSADOR_METHODS), 51)
        self.assertEqual(
            sum(
                len(FEDERATE_AMBASSADOR_PARAMETER_TYPES[name])
                for name in FEDERATE_AMBASSADOR_METHODS
            ),
            CALLBACK_OVERLOAD_COUNTS["2010"],
        )
        self.assertEqual(len(calls), expected_calls)

    def test_python_callback_provenance_vectors_round_trip_through_jni(self) -> None:
        """Use JNI-produced Java carriers for the shared callback vectors.

        The native probe cannot manufacture a region-bearing supplemental
        callback record, so that one vector is covered by the provider-fake
        parity test; the JNI lane consumes the four representable no-region
        vectors and still checks the same payload/tag/time/order semantics.
        """

        calls: list[tuple[str, tuple[object, ...]]] = []
        calls_secondary: list[tuple[str, tuple[object, ...]]] = []
        callback = NullFederateAmbassador()
        callback_secondary = NullFederateAmbassador()
        for name in (
            "discoverObjectInstance",
            "reflectAttributeValues",
            "receiveInteraction",
        ):
            setattr(
                callback,
                name,
                lambda *args, _name=name: calls.append((_name, args)),
            )
            setattr(
                callback_secondary,
                name,
                lambda *args, _name=name: calls_secondary.append((_name, args)),
            )
        binding = self.runtime.bind_federate_ambassador(callback)
        binding_secondary = self.runtime.bind_federate_ambassador(callback_secondary)
        actual: list[CallbackDeliveryObservation] = []

        def raw_handle(kind: str, seed: int) -> object:
            return self.probe.handleCarrier(kind, seed)

        def raw_map(kind: str, handle_kind: str, vector: object) -> object:
            values = self.probe.collectionCarrier(kind)
            values.put(
                raw_handle(handle_kind, 41),
                self.runtime.byte_array(vector.payload),  # type: ignore[attr-defined]
            )
            return values

        def raw_time(vector: object) -> object:
            factory_name = vector.time_implementation + "Factory"  # type: ignore[attr-defined]
            return self.probe.logicalTimeFactoryCarrier(factory_name).makeTime(  # type: ignore[attr-defined]
                vector.time_value  # type: ignore[attr-defined]
            )

        vectors = tuple(
            vector
            for vector in iter_callback_provenance_matrix()
            if vector.sent_region is None
        )
        for vector in vectors:
            with self.subTest(case=vector.case_id):
                if vector.callback == "discoverObjectInstance":
                    arguments = (
                        raw_handle("ObjectInstanceHandle", 31),
                        raw_handle("ObjectClassHandle", 32),
                        vector.instance_name,
                        raw_handle("FederateHandle", 33),
                    )
                else:
                    map_kind = (
                        "AttributeHandleValueMap"
                        if vector.callback == "reflectAttributeValues"
                        else "ParameterHandleValueMap"
                    )
                    handle_kind = (
                        "AttributeHandle"
                        if vector.callback == "reflectAttributeValues"
                        else "ParameterHandle"
                    )
                    common = (
                        raw_handle(
                            "ObjectInstanceHandle"
                            if vector.callback == "reflectAttributeValues"
                            else "InteractionClassHandle",
                            34,
                        ),
                        raw_map(map_kind, handle_kind, vector),
                        self.runtime.byte_array(vector.tag),  # type: ignore[attr-defined]
                        getattr(
                            self.jpype.JClass("hla.rti1516e.OrderType"),
                            vector.received_order
                            if not vector.timed
                            else vector.sent_order,  # type: ignore[arg-type]
                        ),
                        raw_handle("TransportationTypeHandle", 35),
                    )
                    if vector.timed:
                        arguments = common + (
                            raw_time(vector),
                            getattr(
                                self.jpype.JClass("hla.rti1516e.OrderType"),
                                vector.received_order,  # type: ignore[arg-type]
                            ),
                            raw_handle("MessageRetractionHandle", 36),
                            self.probe.callbackCarrier(
                                "SupplementalReflectInfo"
                                if vector.callback == "reflectAttributeValues"
                                else "SupplementalReceiveInfo"
                            ),
                        )
                    else:
                        arguments = common + (
                            self.probe.callbackCarrier(
                                "SupplementalReflectInfo"
                                if vector.callback == "reflectAttributeValues"
                                else "SupplementalReceiveInfo"
                            ),
                        )
                for recipient, target, recipient_calls in (
                    ("member-a", binding.target, calls),
                    ("member-b", binding_secondary.target, calls_secondary),
                ):
                    before = len(recipient_calls)
                    getattr(target, vector.callback)(*arguments)
                    self.assertEqual(len(recipient_calls), before + 1)
                    returned_name, result = recipient_calls[-1]
                    self.assertEqual(returned_name, vector.callback)
                    if vector.callback == "discoverObjectInstance":
                        self.assertEqual(result[2], vector.instance_name)
                        self.assertTrue(result[0].encodedValue)
                        self.assertTrue(result[1].encodedValue)
                        self.assertTrue(result[3].encodedValue)
                        actual.append(
                            CallbackDeliveryObservation(
                                recipient=recipient,
                                sequence=len(
                                    [
                                        item
                                        for item in actual
                                        if item.recipient == recipient
                                    ]
                                ),
                                vector_id=vector.case_id,
                                callback=vector.callback,
                                timed=False,
                                object_instance=vector.object_instance,
                                object_class=vector.object_class,
                                instance_name=result[2],
                                producing_federate=vector.producing_federate,
                            )
                        )
                        continue
                    self.assertEqual(result[2], vector.tag)
                    self.assertTrue(result[0].encodedValue)
                    self.assertEqual(len(result[1]), 1)
                    self.assertEqual(next(iter(result[1].values())), vector.payload)
                    self.assertTrue(result[4].encodedValue)
                    info = result[-1]
                    self.assertTrue(info.hasProducingFederate())
                    self.assertTrue(info.getProducingFederate().encodedValue)
                    self.assertFalse(info.hasSentRegions())
                    if vector.timed:
                        self.assertEqual(result[5].getTime(), vector.time_value)
                        self.assertEqual(result[3], OrderType[vector.sent_order])  # type: ignore[index]
                        self.assertEqual(result[6], OrderType[vector.received_order])  # type: ignore[index]
                        self.assertTrue(result[7].encodedValue)
                    else:
                        self.assertEqual(result[3], OrderType[vector.received_order])  # type: ignore[index]
                    payload_key = next(iter(result[1]))
                    actual.append(
                        CallbackDeliveryObservation(
                            recipient=recipient,
                            sequence=len(
                                [
                                    item
                                    for item in actual
                                    if item.recipient == recipient
                                ]
                            ),
                            vector_id=vector.case_id,
                            callback=vector.callback,
                            timed=vector.timed,
                            object_instance=(
                                vector.object_instance
                                if vector.callback == "reflectAttributeValues"
                                else None
                            ),
                            interaction_class=(
                                vector.interaction_class
                                if vector.callback == "receiveInteraction"
                                else None
                            ),
                            payload_handle=vector.payload_handle,
                            payload=result[1][payload_key],
                            tag=result[2],
                            transportation=vector.transportation,
                            producing_federate=vector.producing_federate,
                            sent_region=None,
                            time_implementation=(
                                result[5].implementationName() if vector.timed else None
                            ),
                            time_value=(result[5].getTime() if vector.timed else None),
                            sent_order=(result[3].name if vector.timed else None),
                            received_order=(
                                result[6].name if vector.timed else result[3].name
                            ),
                            retraction=(
                                vector.retraction if vector.timed else None
                            ),
                        )
                    )

        expected = tuple(
            CallbackDeliveryObservation.from_vector(
                vector,
                recipient=recipient,
                sequence=sequence,
            )
            for recipient in ("member-a", "member-b")
            for sequence, vector in enumerate(vectors)
        )
        assert_callback_delivery_parity(expected, actual)

    def test_python_returned_retraction_handle_can_round_trip_back_to_jni(self) -> None:
        """Cache every provider-issued return handle for a later ``retract`` call."""

        raw_retraction = self.probe.handleCarrier("MessageRetractionHandle", 71)
        raw_return = self.jpype.JClass("hla.rti1516e.MessageRetractionReturn")(
            True, raw_retraction
        )
        raw_return = self.probe.roundTripObject(raw_return)

        class Owner:
            def __init__(self, probe: object) -> None:
                self.probe = probe
                self.retracted: object | None = None

            def getParameterHandleFactory(self) -> object:
                return self.probe.handleFactoryCarrier("ParameterHandleFactory")

            def getInteractionClassHandleFactory(self) -> object:
                return self.probe.handleFactoryCarrier(
                    "InteractionClassHandleFactory"
                )

            def getParameterHandleValueMapFactory(self) -> object:
                return self.probe.collectionFactoryCarrier(
                    "ParameterHandleValueMapFactory"
                )

            def getAttributeHandleValueMapFactory(self) -> object:
                return self.probe.collectionFactoryCarrier(
                    "AttributeHandleValueMapFactory"
                )

            def getAttributeHandleFactory(self) -> object:
                return self.probe.handleFactoryCarrier("AttributeHandleFactory")

            def getObjectInstanceHandleFactory(self) -> object:
                return self.probe.handleFactoryCarrier(
                    "ObjectInstanceHandleFactory"
                )

            def getRegionHandleSetFactory(self) -> object:
                return self.probe.collectionFactoryCarrier("RegionHandleSetFactory")

            def getTimeFactory(self) -> object:
                return self.probe.logicalTimeFactoryCarrier(
                    "HLAinteger64TimeFactory"
                )

            def sendInteraction(self, *args: object) -> object:
                return raw_return if len(args) == 4 else None

            def updateAttributeValues(self, *args: object) -> object:
                return raw_return if len(args) == 4 else None

            def deleteObjectInstance(self, *args: object) -> object:
                return raw_return if len(args) == 3 else None

            def sendInteractionWithRegions(self, *args: object) -> object:
                return raw_return if len(args) == 5 else None

            def retract(self, value: object) -> None:
                self.retracted = self.probe.roundTripHandleCarrier(
                    "MessageRetractionHandle", value
                )

        owner = Owner(self.probe)
        ambassador = Java2010RTIambassador(owner, self.runtime)
        interaction = self.probe.handleCarrier("InteractionClassHandle", 72)
        interaction_value = InteractionClassHandle(
            self.runtime.handle_bytes(interaction)
        )
        object_raw = self.probe.handleCarrier("ObjectInstanceHandle", 73)
        attribute_raw = self.probe.handleCarrier("AttributeHandle", 74)
        region_raw = self.probe.handleCarrier("RegionHandle", 75)
        ambassador._remember_raw_handle(ObjectInstanceHandle, object_raw)
        ambassador._remember_raw_handle(RegionHandle, region_raw)
        object_instance = ObjectInstanceHandle(self.runtime.handle_bytes(object_raw))
        attribute = AttributeHandle(self.runtime.handle_bytes(attribute_raw))
        regions = RegionHandleSet((RegionHandle(self.runtime.handle_bytes(region_raw)),))
        attribute_values = AttributeHandleValueMap({attribute: b"attribute"})
        parameter_values = ParameterHandleValueMap()
        time = HLAinteger64Time(
            encodedValue=(72).to_bytes(8, "big", signed=True),
            value=72,
            implementationNameValue="HLAinteger64Time",
        )
        calls = (
            (
                "updateAttributeValues",
                (object_instance, attribute_values, b"return-update"),
                (object_instance, attribute_values, b"return-update", time),
            ),
            (
                "sendInteraction",
                (interaction_value, parameter_values, b"return-interaction"),
                (interaction_value, parameter_values, b"return-interaction", time),
            ),
            (
                "deleteObjectInstance",
                (object_instance, b"return-delete"),
                (object_instance, b"return-delete", time),
            ),
            (
                "sendInteractionWithRegions",
                (interaction_value, parameter_values, regions, b"return-regional"),
                (
                    interaction_value,
                    parameter_values,
                    regions,
                    b"return-regional",
                    time,
                ),
            ),
        )
        for method_name, immediate, timestamped in calls:
            for arguments in (immediate, timestamped):
                with self.subTest(method=method_name, arity=len(arguments)):
                    result = getattr(ambassador, method_name)(*arguments)
                    if len(arguments) == len(immediate):
                        self.assertIsNone(result)
                        continue
                    self.assertIsInstance(result, MessageRetractionReturn)
                    self.assertTrue(result.retractionHandleIsValid)
                    self.assertIsInstance(result.handle, MessageRetractionHandle)
                    ambassador.retract(result.handle)
                    self.assertIsNotNone(owner.retracted)
                    self.assertEqual(
                        self.runtime.handle_bytes(owner.retracted),  # type: ignore[arg-type]
                        self.runtime.handle_bytes(raw_retraction),
                    )

    def test_python_rti_return_surface_matrix_round_trips_jni_carriers(self) -> None:
        """Convert every non-void 2010 RTI return shape through the provider.

        The JNI probe produces each returned Java carrier first.  A small
        standard-shaped implementation then exposes those carriers through
        the generated RTI method names, allowing the public Python adapter to
        exercise every non-void overload without pretending that the bounded
        JNI ambassador implements RTI service state.
        """

        handle_types = {
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
        handle_seeds = {
            name: self.probe.handleCarrier(name, 80 + index)
            for index, name in enumerate(handle_types)
        }

        class Owner:
            def __init__(self, probe: object, time_kind: str) -> None:
                self.probe = probe
                self.time_kind = time_kind
                self.calls: list[tuple[str, tuple[object, ...]]] = []
                self.results: dict[str, object] = {}

            def _handle_factory(self, kind: str) -> object:
                return self.probe.handleFactoryCarrier(kind + "Factory")

            def _collection_factory(self, kind: str) -> object:
                return self.probe.collectionFactoryCarrier(kind + "Factory")

            def getFederateHandleFactory(self) -> object:
                return self._handle_factory("FederateHandle")

            def getObjectClassHandleFactory(self) -> object:
                return self._handle_factory("ObjectClassHandle")

            def getObjectInstanceHandleFactory(self) -> object:
                return self._handle_factory("ObjectInstanceHandle")

            def getAttributeHandleFactory(self) -> object:
                return self._handle_factory("AttributeHandle")

            def getInteractionClassHandleFactory(self) -> object:
                return self._handle_factory("InteractionClassHandle")

            def getParameterHandleFactory(self) -> object:
                return self._handle_factory("ParameterHandle")

            def getTransportationTypeHandleFactory(self) -> object:
                return self._handle_factory("TransportationTypeHandle")

            def getDimensionHandleFactory(self) -> object:
                return self._handle_factory("DimensionHandle")

            def getAttributeHandleSetFactory(self) -> object:
                return self._collection_factory("AttributeHandleSet")

            def getDimensionHandleSetFactory(self) -> object:
                return self._collection_factory("DimensionHandleSet")

            def getFederateHandleSetFactory(self) -> object:
                return self._collection_factory("FederateHandleSet")

            def getRegionHandleSetFactory(self) -> object:
                return self._collection_factory("RegionHandleSet")

            def getAttributeHandleValueMapFactory(self) -> object:
                return self._collection_factory("AttributeHandleValueMap")

            def getParameterHandleValueMapFactory(self) -> object:
                return self._collection_factory("ParameterHandleValueMap")

            def getAttributeSetRegionSetPairListFactory(self) -> object:
                return self._collection_factory("AttributeSetRegionSetPairList")

            def getTimeFactory(self) -> object:
                return self.probe.logicalTimeFactoryCarrier(self.time_kind + "Factory")

            def getHLAversion(self) -> object:
                return self.probe.roundTripString("jni-2010-return-matrix")

            def __getattr__(self, name: str) -> object:
                if name not in self.results:
                    raise AttributeError(name)

                def invoke(*args: object) -> object:
                    self.calls.append((name, args))
                    return self.results[name]

                return invoke

        enum_types = {
            "OrderType": OrderType,
            "ResignAction": ResignAction,
        }
        explicit_provider_methods = {
            "connect",
            "disconnect",
            "getHLAversion",
            "getTimeFactory",
            "getAttributeHandleFactory",
            "getAttributeHandleSetFactory",
            "getAttributeHandleValueMapFactory",
            "getAttributeSetRegionSetPairListFactory",
            "getDimensionHandleFactory",
            "getDimensionHandleSetFactory",
            "getFederateHandleFactory",
            "getFederateHandleSetFactory",
            "getInteractionClassHandleFactory",
            "getObjectClassHandleFactory",
            "getObjectInstanceHandleFactory",
            "getParameterHandleFactory",
            "getParameterHandleValueMapFactory",
            "getRegionHandleSetFactory",
            "getTransportationTypeHandleFactory",
        }

        def raw_result(return_type: str, time_kind: str) -> object:
            if return_type in handle_types:
                return self.probe.roundTripHandleCarrier(
                    return_type, handle_seeds[return_type]
                )
            if return_type == "AttributeHandleSet":
                carrier = self.probe.collectionCarrier(return_type)
                carrier.add(handle_seeds["AttributeHandle"])
                return self.probe.roundTripCollectionCarrier(return_type, carrier)
            if return_type == "DimensionHandleSet":
                carrier = self.probe.collectionCarrier(return_type)
                carrier.add(handle_seeds["DimensionHandle"])
                return self.probe.roundTripCollectionCarrier(return_type, carrier)
            if return_type == "LogicalTime":
                return self.probe.logicalTimeCarrier(time_kind)
            if return_type == "LogicalTimeInterval":
                return self.probe.logicalTimeCarrier(
                    time_kind.replace("Time", "Interval")
                )
            if return_type == "TimeQueryReturn":
                return self.probe.roundTripObject(
                    self.jpype.JClass("hla.rti1516e.TimeQueryReturn")(
                        True, self.probe.logicalTimeCarrier(time_kind)
                    )
                )
            if return_type == "MessageRetractionReturn":
                return self.probe.roundTripObject(
                    self.jpype.JClass("hla.rti1516e.MessageRetractionReturn")(
                        True, handle_seeds["MessageRetractionHandle"]
                    )
                )
            if return_type == "RangeBounds":
                return self.probe.roundTripObject(
                    self.jpype.JClass("hla.rti1516e.RangeBounds")(81, 89)
                )
            if return_type == "String":
                return self.probe.roundTripString("jni-2010-return-matrix")
            if return_type in enum_types:
                enum_class = self.jpype.JClass("hla.rti1516e." + return_type)
                return self.probe.roundTripEnum(enum_class.values()[0])
            if return_type == "boolean":
                return self.probe.roundTripBoolean(True)
            if return_type == "double":
                return self.probe.roundTripDouble(3.5)
            if return_type == "long":
                return self.probe.roundTripLong(89)
            raise AssertionError("return matrix has no JNI carrier for " + return_type)

        def python_value(
            expected_type: str,
            time_kind: str,
            ambassador: Java2010RTIambassador,
        ) -> object:
            if expected_type in handle_types:
                return handle_types[expected_type](
                    self.runtime.handle_bytes(handle_seeds[expected_type])
                )
            if expected_type == "FederateAmbassador":
                return NullFederateAmbassador()
            if expected_type == "CallbackModel":
                return CallbackModel.HLA_EVOKED
            if expected_type == "String":
                return "python-return-matrix"
            if expected_type == "URL":
                return Path(__file__).resolve()
            if expected_type == "URL[]":
                return (Path(__file__).resolve(),)
            if expected_type == "byte[]":
                return b"python-return-matrix"
            if expected_type == "Set<String>":
                return frozenset(("python-return-matrix",))
            if expected_type == "AttributeHandleSet":
                return AttributeHandleSet((python_value("AttributeHandle", time_kind, ambassador),))
            if expected_type == "DimensionHandleSet":
                return DimensionHandleSet((python_value("DimensionHandle", time_kind, ambassador),))
            if expected_type == "FederateHandleSet":
                return FederateHandleSet((python_value("FederateHandle", time_kind, ambassador),))
            if expected_type == "RegionHandleSet":
                return RegionHandleSet((python_value("RegionHandle", time_kind, ambassador),))
            if expected_type == "AttributeHandleValueMap":
                return AttributeHandleValueMap(
                    {python_value("AttributeHandle", time_kind, ambassador): b"attribute"}
                )
            if expected_type == "ParameterHandleValueMap":
                return ParameterHandleValueMap(
                    {python_value("ParameterHandle", time_kind, ambassador): b"parameter"}
                )
            if expected_type == "AttributeSetRegionSetPairList":
                attributes = AttributeHandleSet(
                    (python_value("AttributeHandle", time_kind, ambassador),)
                )
                regions = RegionHandleSet(
                    (python_value("RegionHandle", time_kind, ambassador),)
                )
                return AttributeSetRegionSetPairList(
                    [AttributeRegionAssociation(attributes, regions)]
                )
            if expected_type == "RangeBounds":
                return RangeBounds(81, 89)
            if expected_type == "LogicalTime":
                encoded = (89).to_bytes(8, "big", signed=True)
                if time_kind == "HLAinteger64Time":
                    return HLAinteger64Time(
                        encodedValue=encoded,
                        value=89,
                        implementationNameValue=time_kind,
                    )
                return HLAfloat64Time(
                    encodedValue=struct.pack(">d", 8.9),
                    value=8.9,
                    implementationNameValue=time_kind,
                )
            if expected_type == "LogicalTimeInterval":
                if time_kind == "HLAinteger64Time":
                    return HLAinteger64Interval(
                        encodedValue=(9).to_bytes(8, "big", signed=True),
                        value=9,
                        implementationNameValue="HLAinteger64Interval",
                    )
                return HLAfloat64Interval(
                    encodedValue=struct.pack(">d", 0.9),
                    value=0.9,
                    implementationNameValue="HLAfloat64Interval",
                )
            if expected_type == "OrderType":
                return OrderType.RECEIVE
            if expected_type == "ResignAction":
                return ResignAction.NO_ACTION
            if expected_type == "ServiceGroup":
                return ServiceGroup.FEDERATION_MANAGEMENT
            if expected_type == "double":
                return 2.25
            if expected_type == "boolean":
                return True
            raise AssertionError("return matrix has no Python value for " + expected_type)

        expected_cases = 0
        for time_kind in ("HLAinteger64Time", "HLAfloat64Time"):
            owner = Owner(self.probe, time_kind)
            ambassador = Java2010RTIambassador(owner, self.runtime)
            for handle_name, handle_type in handle_types.items():
                ambassador._remember_raw_handle(handle_type, handle_seeds[handle_name])
            for name in RTIAMBASSADOR_METHODS:
                if name in explicit_provider_methods:
                    continue
                for overload_index, return_type in enumerate(
                    RTIAMBASSADOR_RETURN_TYPES[name]
                ):
                    if return_type == "void":
                        continue
                    parameters = RTIAMBASSADOR_PARAMETER_TYPES[name][overload_index]
                    owner.results[name] = raw_result(return_type, time_kind)
                    arguments = tuple(
                        python_value(parameter, time_kind, ambassador)
                        for parameter in parameters
                    )
                    with self.subTest(
                        method=name,
                        overload=overload_index,
                        return_type=return_type,
                        time_kind=time_kind,
                    ):
                        result = getattr(ambassador, name)(*arguments)
                        if return_type in handle_types:
                            self.assertIsInstance(result, handle_types[return_type])
                            self.assertEqual(
                                result.encodedValue,
                                self.runtime.handle_bytes(handle_seeds[return_type]),
                            )
                        elif return_type == "AttributeHandleSet":
                            self.assertIsInstance(result, AttributeHandleSet)
                            self.assertEqual(len(result), 1)
                        elif return_type == "DimensionHandleSet":
                            self.assertIsInstance(result, DimensionHandleSet)
                            self.assertEqual(len(result), 1)
                        elif return_type == "LogicalTime":
                            expected_class = (
                                HLAinteger64Time
                                if time_kind == "HLAinteger64Time"
                                else HLAfloat64Time
                            )
                            self.assertIsInstance(result, expected_class)
                        elif return_type == "LogicalTimeInterval":
                            expected_class = (
                                HLAinteger64Interval
                                if time_kind == "HLAinteger64Time"
                                else HLAfloat64Interval
                            )
                            self.assertIsInstance(result, expected_class)
                        elif return_type == "MessageRetractionReturn":
                            self.assertIsInstance(result, MessageRetractionReturn)
                            self.assertTrue(result.retractionHandleIsValid)
                            self.assertIsInstance(result.handle, MessageRetractionHandle)
                        elif return_type == "TimeQueryReturn":
                            self.assertIsInstance(result, TimeQueryReturn)
                            self.assertTrue(result.timeIsValid)
                            self.assertIsInstance(
                                result.time,
                                HLAinteger64Time
                                if time_kind == "HLAinteger64Time"
                                else HLAfloat64Time,
                            )
                        elif return_type == "RangeBounds":
                            self.assertEqual(result, RangeBounds(81, 89))
                        elif return_type == "String":
                            self.assertIsInstance(result, str)
                        elif return_type in enum_types or return_type == "ServiceGroup":
                            self.assertIsInstance(result, type(python_value(return_type, time_kind, ambassador)))
                        elif return_type == "boolean":
                            self.assertIs(type(result), bool)
                        elif return_type == "double":
                            self.assertIs(type(result), float)
                        elif return_type == "long":
                            self.assertIs(type(result), int)
                        else:
                            self.fail("unhandled return type " + return_type)
                    expected_cases += 1
            self.assertEqual(len(owner.calls), 50)

        self.assertEqual(expected_cases, 100)
