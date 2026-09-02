"""Focused evidence for the direct IEEE 1516e native Python boundary.

The extension is built by the package's CMake/scikit-build flow.  Keeping this
test separate from the 2025 provider suite makes the edition boundary explicit
and lets source-checkout CI skip it when no native extension has been built.
"""

import ast
import struct
import unittest
from importlib.metadata import EntryPoint
from pathlib import Path
from unittest.mock import patch

from hla.rti1516e import (
    RTIAMBASSADOR_METHODS,
    AttributeHandle,
    AttributeHandleSet,
    AttributeRegionAssociation,
    AttributeSetRegionSetPairList,
    CallbackModel,
    DimensionHandle,
    DimensionHandleSet,
    FederateHandle,
    FederateHandleSet,
    InteractionClassHandle,
    LogicalTimeFactoryFactory,
    NullFederateAmbassador,
    ObjectClassHandle,
    ObjectInstanceHandle,
    ParameterHandle,
    RegionHandle,
    RegionHandleSet,
    ResignAction,
    RTIambassador,
    RtiFactoryFactory,
    TransportationTypeHandle,
)
from hla.rti1516e.encoding import (
    ByteWrapper,
    DataElementFactory,
    DecoderException,
    EncoderException,
    EncoderFactory,
)
from hla.rti1516e.exceptions import (
    _EXCEPTION_TYPES,
    _STANDARD_EXCEPTION_NAMES,
    AlreadyConnected,
    CouldNotDecode,
    RTIinternalError,
)
from umbra_rti_test_support import (
    HLA_FOM,
    iter_data_element_value_matrix,
    iter_logical_time_arithmetic_matrix,
    iter_logical_time_wire_matrix,
)

try:
    from umbra._native.rti1516e import (
        Native2010Float64TimeFactory,
        Native2010RTIambassador,
        Native2010RtiFactory,
        _call_native,
        _native_2010,
    )
except ImportError as error:  # pragma: no cover - exercised only in source checkouts
    Native2010RTIambassador = None  # type: ignore[assignment]
    Native2010Float64TimeFactory = None  # type: ignore[assignment]
    Native2010RtiFactory = None  # type: ignore[assignment]
    _call_native = None  # type: ignore[assignment]
    _native_2010 = None  # type: ignore[assignment]
    _IMPORT_ERROR = error
else:
    _IMPORT_ERROR = None


@unittest.skipIf(
    Native2010RtiFactory is None,
    "the optional _native_2010 extension has not been built",
)
class Native2010ProviderTest(unittest.TestCase):
    def test_native_exception_matrix_preserves_identity_message_and_cause(self) -> None:
        assert _native_2010 is not None
        assert _call_native is not None
        for name in sorted(_STANDARD_EXCEPTION_NAMES):
            expected_type = _EXCEPTION_TYPES[name]
            message = f"native-2010:{name}: Ω 🚀\x00"
            with self.subTest(name=name), self.assertRaises(expected_type) as raised:
                _call_native(_native_2010.raise_standard_exception, name, message)
            self.assertIs(type(raised.exception), expected_type)
            self.assertEqual(str(raised.exception), message)
            self.assertIsNotNone(raised.exception.cause)
            self.assertIs(raised.exception.__cause__, raised.exception.cause)

        from umbra._native.rti1516e.encoding import _call_native as call_encoding_native

        with self.assertRaises(EncoderException) as encoder_error:
            call_encoding_native(
                _native_2010.raise_standard_exception,
                "EncoderException",
                "native-2010:EncoderException: Ω 🚀\x00",
                encoding_error=EncoderException,
            )
        self.assertEqual(
            str(encoder_error.exception), "native-2010:EncoderException: Ω 🚀\x00"
        )
        self.assertIsNotNone(encoder_error.exception.__cause__)

        with self.assertRaises(DecoderException) as decoder_error:
            call_encoding_native(
                _native_2010.raise_standard_exception,
                "DecoderException",
                "native-2010:DecoderException: Ω 🚀\x00",
                encoding_error=DecoderException,
            )
        self.assertEqual(
            str(decoder_error.exception), "native-2010:DecoderException: Ω 🚀\x00"
        )
        self.assertIsNotNone(decoder_error.exception.__cause__)

    def test_factory_and_lifecycle_use_the_1516e_contract(self) -> None:
        assert Native2010RtiFactory is not None
        factory = Native2010RtiFactory()
        self.assertIn("2010", factory.rtiName())
        self.assertTrue(factory.rtiVersion())
        ambassador = factory.getRtiAmbassador()
        self.assertIsInstance(ambassador, RTIambassador)
        self.assertIsInstance(ambassador, Native2010RTIambassador)

        callback = NullFederateAmbassador()
        ambassador.connect(callback, CallbackModel.HLA_EVOKED, "設定")
        with self.assertRaises(AlreadyConnected):
            ambassador.connect(callback, CallbackModel.HLA_EVOKED)

        ambassador.disconnect()
        with self.assertRaises(RTIinternalError):
            ambassador.disconnect()

        immediate = factory.getRtiAmbassador()
        immediate.connect(callback, CallbackModel.HLA_IMMEDIATE)
        immediate.disconnect()

    def test_native_provider_binds_every_2010_service_and_marks_gaps_explicitly(
        self,
    ) -> None:
        """Keep the bounded native route mechanically surface-complete.

        The direct 2010 provider intentionally implements only a reference
        slice.  The remaining standard methods are generated as explicit
        ``NotImplementedError`` gates, rather than being absent or silently
        falling through to an unrelated 2025 implementation.  This test
        checks the distinction for every generated service name while leaving
        stateful implementation work out of the surface gate.
        """

        assert Native2010RTIambassador is not None
        assert Native2010RtiFactory is not None
        ambassador = Native2010RtiFactory().getRtiAmbassador()
        self.assertEqual(Native2010RTIambassador.__abstractmethods__, frozenset())
        self.assertEqual(len(RTIAMBASSADOR_METHODS), 150)

        for name in RTIAMBASSADOR_METHODS:
            with self.subTest(service=name):
                method = getattr(ambassador, name, None)
                self.assertTrue(callable(method), name)
                class_method = getattr(type(ambassador), name)
                # Methods produced by _unsupported have a stable closure
                # name.  Do not invoke implemented methods without their
                # standard arguments just to prove that they are present.
                if (
                    getattr(getattr(class_method, "__code__", None), "co_name", "")
                    == "invoke"
                ):
                    with self.assertRaises(NotImplementedError) as raised:
                        method()
                    self.assertIn(name, str(raised.exception))

    def test_native_2010_encoder_surface_excludes_2025_extensions(self) -> None:
        """The direct 2010 provider must not leak newer encoding creators."""

        assert Native2010RtiFactory is not None
        encoder = Native2010RtiFactory().getEncoderFactory()
        current_only = (
            "createHLAunsignedInteger16BE",
            "createHLAunsignedInteger16LE",
            "createHLAunsignedInteger32BE",
            "createHLAunsignedInteger32LE",
            "createHLAunsignedInteger64BE",
            "createHLAunsignedInteger64LE",
            "createHLAextendableVariantRecord",
        )
        for method_name in current_only:
            with self.subTest(method=method_name):
                self.assertFalse(hasattr(encoder, method_name))

    def test_installed_style_entry_point_loading_selects_the_2010_native_factory(
        self,
    ) -> None:
        """Exercise the same metadata route used after wheel installation."""

        assert Native2010RtiFactory is not None
        entry_point = EntryPoint(
            name="UmbraNative2010",
            value="umbra._native.rti1516e:Native2010RtiFactory",
            group="hla.rti1516e.factories",
        )
        with patch("hla.rti1516e.core.entry_points", return_value=[entry_point]):
            factories = RtiFactoryFactory.getAvailableRtiFactories()
            selected = RtiFactoryFactory.getRtiFactory("UmbraNative2010")
        self.assertEqual(len(factories), 1)
        self.assertIsInstance(factories[0], Native2010RtiFactory)
        self.assertIsInstance(selected, Native2010RtiFactory)
        self.assertIn("2010", selected.rtiName())

    def test_native_handle_and_collection_factories_round_trip_at_python_boundary(
        self,
    ) -> None:
        """Exercise every bounded 2010 Python factory carrier directly."""

        assert Native2010RtiFactory is not None
        ambassador = Native2010RtiFactory().getRtiAmbassador()
        handle_specs = (
            ("getFederateHandleFactory", FederateHandle, b"federate"),
            ("getObjectClassHandleFactory", ObjectClassHandle, b"object-class"),
            (
                "getObjectInstanceHandleFactory",
                ObjectInstanceHandle,
                b"object-instance",
            ),
            ("getAttributeHandleFactory", AttributeHandle, b"attribute"),
            (
                "getInteractionClassHandleFactory",
                InteractionClassHandle,
                b"interaction",
            ),
            ("getParameterHandleFactory", ParameterHandle, b"parameter"),
            ("getDimensionHandleFactory", DimensionHandle, b"dimension"),
            (
                "getTransportationTypeHandleFactory",
                TransportationTypeHandle,
                b"transport",
            ),
        )
        decoded: dict[type[object], object] = {}
        for method_name, handle_type, encoded in handle_specs:
            with self.subTest(factory=method_name):
                value = getattr(ambassador, method_name)().decode(b"\x55" + encoded, 1)
                self.assertIsInstance(value, handle_type)
                self.assertEqual(value.encodedValue, encoded)
                decoded[handle_type] = value

        transportation = ambassador.getTransportationTypeHandleFactory()
        self.assertEqual(
            transportation.getHLAdefaultReliable(),
            TransportationTypeHandle(b"transport:HLAdefaultReliable"),
        )
        self.assertEqual(
            transportation.getHLAdefaultBestEffort(),
            TransportationTypeHandle(b"transport:HLAdefaultBestEffort"),
        )

        attribute = decoded[AttributeHandle]
        dimension = decoded[DimensionHandle]
        federate = decoded[FederateHandle]
        attribute_set = ambassador.getAttributeHandleSetFactory().create()
        attribute_set.add(attribute)
        dimension_set = ambassador.getDimensionHandleSetFactory().create()
        dimension_set.add(dimension)
        federate_set = ambassador.getFederateHandleSetFactory().create()
        federate_set.add(federate)
        region_set = ambassador.getRegionHandleSetFactory().create()
        region = RegionHandle(b"region")
        region_set.add(region)
        self.assertEqual(attribute_set, AttributeHandleSet((attribute,)))
        self.assertEqual(dimension_set, DimensionHandleSet((dimension,)))
        self.assertEqual(federate_set, FederateHandleSet((federate,)))
        self.assertEqual(region_set, RegionHandleSet((region,)))

        attribute_map = ambassador.getAttributeHandleValueMapFactory().create(1)
        attribute_map[attribute] = b"attribute-value"
        parameter = decoded[ParameterHandle]
        parameter_map = ambassador.getParameterHandleValueMapFactory().create(1)
        parameter_map[parameter] = b"parameter-value"
        self.assertEqual(attribute_map[attribute], b"attribute-value")
        self.assertEqual(parameter_map[parameter], b"parameter-value")

        pair_list = ambassador.getAttributeSetRegionSetPairListFactory().create(1)
        pair_list.append(AttributeRegionAssociation(attribute_set, region_set))
        self.assertEqual(pair_list, AttributeSetRegionSetPairList(pair_list))

    def test_logical_time_factory_factory_discovers_native_time_families(self) -> None:
        assert Native2010RtiFactory is not None

        entry_point = EntryPoint(
            name="UmbraNative2010",
            value="umbra._native.rti1516e:Native2010RtiFactory",
            group="hla.rti1516e.factories",
        )

        def entries(*, group: str):
            return (
                []
                if group == LogicalTimeFactoryFactory._ENTRY_POINT_GROUP
                else [entry_point]
            )

        with patch("hla.rti1516e.core.entry_points", side_effect=entries):
            available = LogicalTimeFactoryFactory.getAvailableLogicalTimeFactories()
            default = LogicalTimeFactoryFactory.getLogicalTimeFactory()
        self.assertEqual(
            {factory.getName() for factory in available},
            {"HLAinteger64Time", "HLAfloat64Time"},
        )
        self.assertIsNotNone(default)
        self.assertEqual(default.getName(), "HLAfloat64Time")

    def test_unsupported_service_is_explicit(self) -> None:
        assert Native2010RtiFactory is not None
        ambassador = Native2010RtiFactory().getRtiAmbassador()
        with self.assertRaises(NotImplementedError):
            ambassador.queryGALT()

    def test_reference_stateful_membership_declaration_and_object_callbacks(
        self,
    ) -> None:
        assert Native2010RtiFactory is not None

        class Recorder(NullFederateAmbassador):
            def __init__(self) -> None:
                self.discovered: tuple[object, ...] | None = None
                self.reflected: tuple[object, ...] | None = None

            def discoverObjectInstance(self, *values: object) -> None:
                self.discovered = values

            def reflectAttributeValues(self, *values: object) -> None:
                self.reflected = values

        factory = Native2010RtiFactory()
        publisher = factory.getRtiAmbassador()
        subscriber = factory.getRtiAmbassador()
        publisher_callback = Recorder()
        subscriber_callback = Recorder()
        publisher.connect(publisher_callback, CallbackModel.HLA_EVOKED)
        subscriber.connect(subscriber_callback, CallbackModel.HLA_EVOKED)
        federation = "umbra-2010-python-native-stateful"
        publisher.createFederationExecution(federation, "reference-fom")
        publisher_handle = publisher.joinFederationExecution(
            "publisher", "reference", federation
        )
        subscriber.joinFederationExecution("subscriber", "reference", federation)
        publisher_class = publisher.getObjectClassHandle(
            HLA_FOM.REFERENCE_2010_OBJECT_CLASS
        )
        subscriber_class = subscriber.getObjectClassHandle(
            HLA_FOM.REFERENCE_2010_OBJECT_CLASS
        )
        publisher_attribute = publisher.getAttributeHandle(
            publisher_class, HLA_FOM.REFERENCE_2010_ATTRIBUTE
        )
        subscriber_attribute = subscriber.getAttributeHandle(
            subscriber_class, HLA_FOM.REFERENCE_2010_ATTRIBUTE
        )
        published = publisher.getAttributeHandleSetFactory().create()
        published.add(publisher_attribute)
        subscribed = subscriber.getAttributeHandleSetFactory().create()
        subscribed.add(subscriber_attribute)
        publisher.publishObjectClassAttributes(publisher_class, published)
        subscriber.subscribeObjectClassAttributes(subscriber_class, subscribed)
        registered = publisher.registerObjectInstance(publisher_class)
        self.assertIsNotNone(subscriber_callback.discovered)
        assert subscriber_callback.discovered is not None
        self.assertEqual(subscriber_callback.discovered[0], registered)
        self.assertEqual(subscriber_callback.discovered[1], subscriber_class)
        self.assertEqual(subscriber_callback.discovered[3], publisher_handle)
        values = publisher.getAttributeHandleValueMapFactory().create()
        values[publisher_attribute] = b"native-2010"
        publisher.updateAttributeValues(registered, values, b"tag")
        self.assertIsNotNone(subscriber_callback.reflected)
        assert subscriber_callback.reflected is not None
        self.assertEqual(subscriber_callback.reflected[0], registered)
        self.assertEqual(
            subscriber_callback.reflected[1][subscriber_attribute], b"native-2010"
        )
        self.assertEqual(subscriber_callback.reflected[2], b"tag")
        self.assertEqual(
            publisher.getObjectInstanceName(registered),
            subscriber_callback.discovered[2],
        )
        subscriber.resignFederationExecution(ResignAction.NO_ACTION)
        publisher.resignFederationExecution(ResignAction.NO_ACTION)
        subscriber.disconnect()
        publisher.disconnect()
        publisher.connect(publisher_callback, CallbackModel.HLA_EVOKED)
        publisher.destroyFederationExecution(federation)
        publisher.disconnect()

    def test_reference_interaction_publish_subscribe_and_receive_callback(self) -> None:
        assert Native2010RtiFactory is not None

        class Recorder(NullFederateAmbassador):
            def __init__(self) -> None:
                self.received: tuple[object, ...] | None = None

            def receiveInteraction(self, *values: object) -> None:
                self.received = values

        factory = Native2010RtiFactory()
        publisher = factory.getRtiAmbassador()
        subscriber = factory.getRtiAmbassador()
        publisher_callback = Recorder()
        subscriber_callback = Recorder()
        federation = "umbra-2010-python-native-interaction"
        publisher.connect(publisher_callback, CallbackModel.HLA_EVOKED)
        subscriber.connect(subscriber_callback, CallbackModel.HLA_EVOKED)
        try:
            publisher.createFederationExecution(federation, "reference-fom")
            publisher_handle = publisher.joinFederationExecution(
                "publisher", "reference", federation
            )
            subscriber.joinFederationExecution("subscriber", "reference", federation)
            publisher_class = publisher.getInteractionClassHandle(
                HLA_FOM.REFERENCE_2010_INTERACTION_CLASS
            )
            subscriber_class = subscriber.getInteractionClassHandle(
                HLA_FOM.REFERENCE_2010_INTERACTION_CLASS
            )
            publisher_parameter = publisher.getParameterHandle(
                publisher_class, HLA_FOM.REFERENCE_2010_PARAMETER
            )
            subscriber_parameter = subscriber.getParameterHandle(
                subscriber_class, HLA_FOM.REFERENCE_2010_PARAMETER
            )
            self.assertEqual(publisher_class, subscriber_class)
            self.assertEqual(publisher_parameter, subscriber_parameter)
            self.assertEqual(
                publisher.getInteractionClassName(publisher_class),
                HLA_FOM.REFERENCE_2010_INTERACTION_CLASS,
            )
            self.assertEqual(
                publisher.getParameterName(publisher_class, publisher_parameter),
                HLA_FOM.REFERENCE_2010_PARAMETER,
            )
            publisher.publishInteractionClass(publisher_class)
            subscriber.subscribeInteractionClass(subscriber_class)
            values = publisher.getParameterHandleValueMapFactory().create()
            values[publisher_parameter] = b"native-interaction"
            publisher.sendInteraction(publisher_class, values, b"interaction-tag")
            self.assertIsNotNone(subscriber_callback.received)
            assert subscriber_callback.received is not None
            received = subscriber_callback.received
            self.assertEqual(received[0], subscriber_class)
            self.assertEqual(received[1][subscriber_parameter], b"native-interaction")
            self.assertEqual(received[2], b"interaction-tag")
            self.assertEqual(received[3].name, "RECEIVE")
            self.assertEqual(received[5].getProducingFederate(), publisher_handle)
            subscriber.unsubscribeInteractionClass(subscriber_class)
            publisher.unpublishInteractionClass(publisher_class)
        finally:
            try:
                subscriber.resignFederationExecution(ResignAction.NO_ACTION)
            except Exception:  # noqa: BLE001, S110 - cleanup is best effort
                pass
            try:
                publisher.resignFederationExecution(ResignAction.NO_ACTION)
            except Exception:  # noqa: BLE001, S110 - cleanup is best effort
                pass
            try:
                publisher.destroyFederationExecution(federation)
            except Exception:  # noqa: BLE001, S110 - cleanup is best effort
                pass
            try:
                subscriber.disconnect()
            except Exception:  # noqa: BLE001, S110 - cleanup is best effort
                pass
            try:
                publisher.disconnect()
            except Exception:  # noqa: BLE001, S110 - cleanup is best effort
                pass

    def test_reference_synchronization_registration_announcement_and_achievement(
        self,
    ) -> None:
        """Keep the standard callback names/carriers intact at the native boundary."""

        assert Native2010RtiFactory is not None

        class Recorder(NullFederateAmbassador):
            def __init__(self) -> None:
                self.registration: list[str] = []
                self.announcements: list[tuple[str, bytes]] = []
                self.completed = 0

            def synchronizationPointRegistrationSucceeded(self, label: str) -> None:
                self.registration.append(label)

            def announceSynchronizationPoint(self, label: str, tag: bytes) -> None:
                self.announcements.append((label, tag))

            def federationSynchronized(
                self, label: str, failed_to_sync: object
            ) -> None:
                if label != "umbra-2010-native-sync-point" or len(failed_to_sync) != 0:
                    raise AssertionError(
                        "unexpected synchronization completion payload"
                    )
                self.completed += 1

        factory = Native2010RtiFactory()
        registrar = factory.getRtiAmbassador()
        observer = factory.getRtiAmbassador()
        registrar_callback = Recorder()
        observer_callback = Recorder()
        federation = "umbra-2010-python-native-synchronization"
        registrar.connect(registrar_callback, CallbackModel.HLA_EVOKED)
        observer.connect(observer_callback, CallbackModel.HLA_EVOKED)
        try:
            registrar.createFederationExecution(federation, "reference-fom")
            registrar.joinFederationExecution("registrar", "reference", federation)
            observer.joinFederationExecution("observer", "reference", federation)
            label = "umbra-2010-native-sync-point"
            tag = b"sync-tag"
            registrar.registerFederationSynchronizationPoint(label, tag)
            self.assertEqual(registrar_callback.registration, [label])
            self.assertEqual(observer_callback.announcements, [(label, tag)])
            registrar.synchronizationPointAchieved(label)
            self.assertEqual(registrar_callback.completed, 0)
            observer.synchronizationPointAchieved(label)
            self.assertEqual(registrar_callback.completed, 1)
            self.assertEqual(observer_callback.completed, 1)
        finally:
            try:
                observer.resignFederationExecution(ResignAction.NO_ACTION)
            except Exception:  # noqa: BLE001, S110 - cleanup is best effort
                pass
            try:
                registrar.resignFederationExecution(ResignAction.NO_ACTION)
            except Exception:  # noqa: BLE001, S110 - cleanup is best effort
                pass
            try:
                registrar.destroyFederationExecution(federation)
            except Exception:  # noqa: BLE001, S110 - cleanup is best effort
                pass
            try:
                observer.disconnect()
            except Exception:  # noqa: BLE001, S110 - cleanup is best effort
                pass
            try:
                registrar.disconnect()
            except Exception:  # noqa: BLE001, S110 - cleanup is best effort
                pass

    def test_official_scalar_encoder_family_round_trips_through_cpp(self) -> None:
        assert Native2010RtiFactory is not None
        encoder = Native2010RtiFactory().getEncoderFactory()
        self.assertIsInstance(encoder, EncoderFactory)
        cases = (
            ("createHLAinteger16BE", -123, b"\xff\x85"),
            ("createHLAinteger16LE", -123, b"\x85\xff"),
            ("createHLAinteger32BE", 0x01020304, b"\x01\x02\x03\x04"),
            ("createHLAinteger32LE", 0x01020304, b"\x04\x03\x02\x01"),
            (
                "createHLAinteger64BE",
                0x0102030405060708,
                b"\x01\x02\x03\x04\x05\x06\x07\x08",
            ),
            (
                "createHLAinteger64LE",
                0x0102030405060708,
                b"\x08\x07\x06\x05\x04\x03\x02\x01",
            ),
            ("createHLAfloat32BE", 1.25, b"\x3f\xa0\x00\x00"),
            ("createHLAfloat32LE", 1.25, b"\x00\x00\xa0\x3f"),
            ("createHLAfloat64BE", 1.25, b"\x3f\xf4\x00\x00\x00\x00\x00\x00"),
            ("createHLAfloat64LE", 1.25, b"\x00\x00\x00\x00\x00\x00\xf4\x3f"),
            ("createHLAbyte", -1, b"\xff"),
            ("createHLAoctet", 0x5A, b"\x5a"),
            ("createHLAASCIIchar", 65, b"A"),
            ("createHLAunicodeChar", 0x03A9, b"\x03\xa9"),
            ("createHLAoctetPairBE", 0x1234, b"\x12\x34"),
            ("createHLAoctetPairLE", 0x1234, b"\x34\x12"),
            ("createHLAboolean", True, b"\x00\x00\x00\x01"),
            ("createHLAASCIIstring", "hello", b"\x00\x00\x00\x05hello"),
            ("createHLAunicodeString", "A", b"\x00\x00\x00\x01\x00A"),
        )
        for method_name, expected, encoded in cases:
            element = getattr(encoder, method_name)(expected)
            self.assertEqual(element.toByteArray(), encoded, method_name)
            decoded = getattr(encoder, method_name)().decode(encoded)
            self.assertEqual(decoded.getValue(), expected, method_name)
            wrapper = ByteWrapper(b"\x00" + encoded + b"\xff", 1, len(encoded))
            decoded.decode(wrapper)
            self.assertEqual(wrapper.getPos(), 1 + len(encoded), method_name)

    def test_native_basic_data_element_value_matrix_round_trips_through_cpp(
        self,
    ) -> None:
        """Exercise shared 2010 edge values through the direct C++ provider.

        The matrix carries values, not expected octets.  This keeps the test
        coupled to the standard factory surface while leaving byte order,
        padding, and length-prefix decisions to the official C++ encoder.
        """
        assert Native2010RtiFactory is not None
        encoder = Native2010RtiFactory().getEncoderFactory()
        signed_byte_kinds = {"HLAbyte", "HLAoctet", "HLAASCIIchar"}
        signed_short_kinds = {"HLAunicodeChar", "HLAoctetPairBE", "HLAoctetPairLE"}
        for vector in iter_data_element_value_matrix("2010"):
            with self.subTest(case=vector.case_id):
                element = getattr(encoder, "create" + vector.kind)(vector.value)
                encoded = bytes(element.toByteArray())
                decoded = getattr(encoder, "create" + vector.kind)()
                decoded.decode(encoded)
                self.assertEqual(bytes(decoded.toByteArray()), encoded)
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
                    self.assertEqual(bytes(actual), bytes(vector.value))
                else:
                    self.assertEqual(actual, vector.value)

    def test_native_encoder_rejects_malformed_input_and_composites_use_cpp_values(
        self,
    ) -> None:
        assert Native2010RtiFactory is not None
        encoder = Native2010RtiFactory().getEncoderFactory()
        with self.assertRaises(DecoderException):
            encoder.createHLAinteger32BE().decode(b"\x00")
        with self.assertRaises(DecoderException):
            encoder.createHLAboolean().decode(b"\x00\x00\x00\x02")
        opaque = encoder.createHLAopaqueData(b"\x01\x02\xff")
        self.assertEqual(opaque.size(), 3)
        self.assertEqual(opaque.get(2), 0xFF)
        self.assertEqual(opaque.getValue(), b"\x01\x02\xff")
        self.assertEqual(opaque.toByteArray(), b"\x00\x00\x00\x03\x01\x02\xff")
        decoded = encoder.createHLAopaqueData()
        decoded.decode(opaque.toByteArray())
        self.assertEqual(decoded.getValue(), b"\x01\x02\xff")
        with self.assertRaises(DecoderException):
            decoded.decode(b"\x00\x00\x00\x04\x01")

        class IntegerFactory(DataElementFactory):
            def createElement(self, index: int):
                del index
                return encoder.createHLAinteger32BE()

        integer_factory = IntegerFactory()
        one = encoder.createHLAinteger32BE(1)
        two = encoder.createHLAinteger32BE(2)
        variable = encoder.createHLAvariableArray(integer_factory, one, two)
        self.assertEqual(variable.size(), 2)
        self.assertEqual(
            variable.toByteArray(), b"\x00\x00\x00\x02\x00\x00\x00\x01\x00\x00\x00\x02"
        )
        self.assertEqual(variable.get(1).getValue(), 2)
        variable_decoded = encoder.createHLAvariableArray(integer_factory)
        variable_wrapper = ByteWrapper(
            b"\x00" + variable.toByteArray() + b"\xff", 1, len(variable.toByteArray())
        )
        variable_decoded.decode(variable_wrapper)
        self.assertEqual(variable_decoded.size(), 2)
        self.assertEqual(variable_wrapper.getPos(), 1 + len(variable.toByteArray()))

        fixed = encoder.createHLAfixedArray(integer_factory, 2)
        fixed.set(0, one)
        fixed.set(1, two)
        self.assertEqual(fixed.toByteArray(), b"\x00\x00\x00\x01\x00\x00\x00\x02")
        self.assertEqual(fixed.get(0).getValue(), 1)
        fixed_decoded = encoder.createHLAfixedArray(integer_factory, 2)
        fixed_wrapper = ByteWrapper(
            b"\x00" + fixed.toByteArray() + b"\xff", 1, len(fixed.toByteArray())
        )
        fixed_decoded.decode(fixed_wrapper)
        self.assertEqual(fixed_decoded.get(1).getValue(), 2)
        self.assertEqual(fixed_wrapper.getPos(), 1 + len(fixed.toByteArray()))

        record = encoder.createHLAfixedRecord()
        record.add(encoder.createHLAoctet(0x7F))
        record.add(one)
        self.assertEqual(record.toByteArray(), b"\x7f\x00\x00\x00\x00\x00\x00\x01")

        variant = encoder.createHLAvariantRecord(encoder.createHLAoctet())
        variant.setVariant(encoder.createHLAoctet(0x7F), one)
        self.assertEqual(variant.toByteArray(), b"\x7f\x00\x00\x00\x00\x00\x00\x01")
        variant.setDiscriminant(encoder.createHLAoctet(1))
        self.assertIsNone(variant.getValue())

    def test_integer_logical_time_factory_and_arithmetic_use_cpp_values(self) -> None:
        assert Native2010RtiFactory is not None
        ambassador = Native2010RtiFactory().getRtiAmbassador()
        factory = ambassador.getTimeFactory()
        self.assertEqual(factory.getName(), "HLAinteger64Time")
        initial = factory.makeInitial()
        final = factory.makeFinal()
        epsilon = factory.makeEpsilon()
        self.assertTrue(initial.isInitial())
        self.assertTrue(final.isFinal())
        self.assertTrue(epsilon.isEpsilon())
        time = factory.makeLogicalTime(5)
        interval = factory.makeLogicalTimeInterval(2)
        self.assertEqual(time.add(interval).getTime(), 7)
        self.assertEqual(time.subtract(interval).getTime(), 3)
        self.assertEqual(time.distance(factory.makeLogicalTime(2)).getInterval(), 3)
        self.assertEqual(time.compareTo(factory.makeLogicalTime(2)), 1)
        self.assertEqual(interval.compareTo(factory.makeLogicalTimeInterval(2)), 0)
        decoded = factory.decodeTime(time.toByteArray())
        self.assertEqual(decoded.getTime(), 5)

    def test_float_logical_time_factory_and_arithmetic_use_cpp_values(self) -> None:
        assert Native2010Float64TimeFactory is not None
        factory = Native2010Float64TimeFactory()
        self.assertEqual(factory.getName(), "HLAfloat64Time")
        self.assertTrue(factory.makeInitial().isInitial())
        self.assertTrue(factory.makeFinal().isFinal())
        self.assertTrue(factory.makeEpsilon().isEpsilon())
        time = factory.makeTime(1.25)
        interval = factory.makeInterval(2.5)
        self.assertAlmostEqual(time.add(interval).getTime(), 3.75)
        self.assertAlmostEqual(
            factory.makeTime(3.75).subtract(interval).getTime(), 1.25
        )
        self.assertAlmostEqual(time.distance(factory.makeTime(0.25)).getInterval(), 1.0)
        self.assertEqual(time.compareTo(factory.makeTime(0.25)), 1)
        self.assertEqual(time.toByteArray(), bytes.fromhex("3ff4000000000000"))
        decoded = factory.decodeTime(time.toByteArray())
        self.assertAlmostEqual(decoded.getTime(), 1.25)

    def test_logical_time_and_interval_marshal_round_trips_at_python_boundary(
        self,
    ) -> None:
        assert Native2010RtiFactory is not None
        assert Native2010Float64TimeFactory is not None
        integer_factory = Native2010RtiFactory().getRtiAmbassador().getTimeFactory()
        float_factory = Native2010Float64TimeFactory()
        cases = (
            (
                integer_factory,
                (0, 1, 123456789, 2**63 - 1),
                (0, 1, 1234, 2**63 - 1),
                lambda value: struct.pack(">q", value),
            ),
            (
                float_factory,
                (0.0, 1.25, 12.5, float.fromhex("0x1.fffffffffffffp+1023")),
                (0.0, 0.25, 2.5, float.fromhex("0x1.fffffffffffffp+1023")),
                lambda value: struct.pack(">d", value),
            ),
        )
        for factory, times, intervals, encode in cases:
            for expected in times:
                value = factory.makeTime(expected)
                encoded = encode(expected)
                destination = bytearray(len(encoded) + 2)
                value.encode(destination, 1)
                self.assertEqual(bytes(destination[1:-1]), encoded)
                decoded = factory.decodeTime(b"\x5a" + encoded + b"\xa5", 1)
                self.assertEqual(decoded.toByteArray(), encoded)
                self.assertEqual(decoded.getTime(), expected)
            for expected in intervals:
                value = factory.makeInterval(expected)
                encoded = encode(expected)
                destination = bytearray(len(encoded) + 2)
                value.encode(destination, 1)
                self.assertEqual(bytes(destination[1:-1]), encoded)
                decoded = factory.decodeInterval(b"\x5a" + encoded + b"\xa5", 1)
                self.assertEqual(decoded.toByteArray(), encoded)
                self.assertEqual(decoded.getInterval(), expected)
            with self.assertRaises(CouldNotDecode):
                factory.decodeTime(b"\x00" * 7)
            with self.assertRaises(CouldNotDecode):
                factory.decodeInterval(b"\x00" * 7)
        self.assertTrue(integer_factory.makeInitial().isInitial())
        self.assertTrue(integer_factory.makeFinal().isFinal())
        self.assertTrue(integer_factory.makeEpsilon().isEpsilon())
        self.assertTrue(float_factory.makeInitial().isInitial())
        self.assertTrue(float_factory.makeFinal().isFinal())
        self.assertTrue(float_factory.makeEpsilon().isEpsilon())

    def test_native_logical_time_wire_and_arithmetic_matrix_round_trips_through_cpp(
        self,
    ) -> None:
        """Exercise shared 1516e vectors at both pybind and façade boundaries.

        The raw pybind factories consume the complete fixed-width wire value,
        while the standard Python façade additionally exposes the 1516e
        offset overload.  Keeping both assertions here makes it explicit that
        Python does not silently change a C++ value while adapting the carrier.
        """

        assert Native2010RtiFactory is not None
        assert Native2010Float64TimeFactory is not None
        assert _native_2010 is not None
        factories = {
            "HLAinteger64Time": (
                _native_2010.Native2010Integer64TimeFactory(),
                Native2010RtiFactory().getRtiAmbassador().getTimeFactory(),
            ),
            "HLAfloat64Time": (
                _native_2010.Native2010Float64TimeFactory(),
                Native2010Float64TimeFactory(),
            ),
        }

        for implementation, (native_factory, factory) in factories.items():
            with self.subTest(implementation=implementation):
                for vector in iter_logical_time_wire_matrix((implementation,)):
                    with self.subTest(vector=vector.case_id):
                        if vector.valid:
                            native_time = _call_native(
                                native_factory.decode_time, vector.encoded
                            )
                            native_interval = _call_native(
                                native_factory.decode_interval, vector.encoded
                            )
                            # The C++ carrier canonicalizes equivalent values
                            # (for example, -0.0 becomes +0.0).  Compare the
                            # decoded wire with that provider-owned canonical
                            # representation rather than requiring bit-level
                            # preservation of a non-canonical input.
                            canonical_time = _call_native(
                                native_factory.make_time, vector.expected
                            )
                            canonical_interval = _call_native(
                                native_factory.make_interval, vector.expected
                            )
                            canonical_time_bytes = _call_native(
                                canonical_time.to_byte_array
                            )
                            canonical_interval_bytes = _call_native(
                                canonical_interval.to_byte_array
                            )
                            self.assertEqual(
                                _call_native(native_time.to_byte_array),
                                canonical_time_bytes,
                            )
                            self.assertEqual(
                                _call_native(native_interval.to_byte_array),
                                canonical_interval_bytes,
                            )
                            self.assertEqual(
                                _call_native(native_time.get_time), vector.expected
                            )
                            self.assertEqual(
                                _call_native(native_interval.get_interval),
                                vector.expected,
                            )

                            carrier = b"\x5a" + vector.encoded + b"\xa5"
                            decoded_time = factory.decodeTime(carrier, 1)
                            decoded_interval = factory.decodeInterval(carrier, 1)
                            self.assertEqual(decoded_time.getTime(), vector.expected)
                            self.assertEqual(
                                decoded_interval.getInterval(), vector.expected
                            )
                            self.assertEqual(
                                decoded_time.toByteArray(), canonical_time_bytes
                            )
                            self.assertEqual(
                                decoded_interval.toByteArray(), canonical_interval_bytes
                            )
                        elif vector.label == "trailing":
                            # Raw C++ decoding is exact-width; the standard
                            # Python overload decodes the selected eight-octet
                            # window and therefore permits a surrounding suffix.
                            with self.assertRaises(CouldNotDecode):
                                _call_native(native_factory.decode_time, vector.encoded)
                            with self.assertRaises(CouldNotDecode):
                                _call_native(
                                    native_factory.decode_interval, vector.encoded
                                )
                            decoded_time = factory.decodeTime(
                                b"\x5a" + vector.encoded[:8] + b"\xa5", 1
                            )
                            decoded_interval = factory.decodeInterval(
                                b"\x5a" + vector.encoded[:8] + b"\xa5", 1
                            )
                            self.assertEqual(decoded_time.getTime(), 0)
                            self.assertEqual(decoded_interval.getInterval(), 0)
                        else:
                            with self.assertRaises(CouldNotDecode):
                                _call_native(native_factory.decode_time, vector.encoded)
                            with self.assertRaises(CouldNotDecode):
                                _call_native(
                                    native_factory.decode_interval, vector.encoded
                                )

                for vector in iter_logical_time_arithmetic_matrix((implementation,)):
                    with self.subTest(arithmetic=vector.case_id):
                        base = factory.makeTime(vector.base)
                        interval = factory.makeInterval(vector.interval)
                        summed = base.add(interval)
                        self.assertEqual(summed.getTime(), vector.expected_sum)
                        difference = base.subtract(interval)
                        if implementation == "HLAfloat64Time" and vector.interval:
                            # The C++ HLAfloat64Time carrier retains IEEE
                            # round-to-nearest for subtracting the smallest
                            # subnormal from 1.0 (the result is still 1.0).
                            # Keep this provider-specific arithmetic edge
                            # visible while asserting that the Python façade
                            # preserves the C++ value exactly.
                            self.assertEqual(difference.getTime(), vector.base)
                        else:
                            self.assertEqual(
                                difference.getTime(), vector.expected_difference
                            )
                        self.assertEqual(
                            summed.distance(base).getInterval(),
                            vector.expected_distance,
                        )

                with self.assertRaises(CouldNotDecode):
                    factory.decodeTime(b"\x00" * 8, -1)
                with self.assertRaises(CouldNotDecode):
                    factory.decodeInterval(b"\x00" * 8, 1)

    def test_facade_calls_only_exported_native_methods(self) -> None:
        assert _native_2010 is not None
        source = (
            Path(__file__).parents[1]
            / "src"
            / "umbra"
            / "_native"
            / "rti1516e"
            / "__init__.py"
        ).read_text(encoding="utf-8")
        module = ast.parse(source)
        facade = next(
            node
            for node in ast.walk(module)
            if isinstance(node, ast.ClassDef) and node.name == "Native2010RTIambassador"
        )
        calls = {
            node.attr
            for node in ast.walk(facade)
            if isinstance(node, ast.Attribute)
            and isinstance(node.value, ast.Attribute)
            and node.value.attr == "_implementation"
        }
        exported = {
            name
            for name in dir(_native_2010.Native2010Ambassador)
            if not name.startswith("_")
        }
        self.assertEqual(sorted(calls - exported), [])


if __name__ == "__main__":
    unittest.main()
