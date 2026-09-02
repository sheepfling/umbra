from __future__ import annotations

import unittest
from unittest.mock import patch

import hla.rti1516e as rti
from hla.rti1516e import contracts
from hla.rti1516e import exceptions
from hla.rti1516e.encoding import (
    ByteWrapper,
    EncoderFactory,
    HLAfixedArray,
    HLAfixedRecord,
    HLAinteger32BE,
    HLAvariableArray,
)
from hla.rti1516e.exceptions import CouldNotDecode, RTIinternalError, exceptionForName


class _Factory(rti.RtiFactory):
    def getRtiAmbassador(self):
        raise NotImplementedError

    def getEncoderFactory(self):
        raise NotImplementedError

    def rtiName(self) -> str:
        return "test-2010"

    def rtiVersion(self) -> str:
        return "2010"


class _EntryPoint:
    name = "test-2010"

    @staticmethod
    def load():
        return _Factory


class _AliasEntryPoint:
    name = "java-2010"

    @staticmethod
    def load():
        return _Factory


class Contract2010Tests(unittest.TestCase):
    def test_authoritative_method_counts_and_names_are_recorded(self) -> None:
        self.assertEqual(sum(contracts.RTIAMBASSADOR_OVERLOAD_COUNTS.values()), 172)
        self.assertEqual(sum(contracts.FEDERATE_AMBASSADOR_OVERLOAD_COUNTS.values()), 60)
        self.assertEqual(
            contracts.RTIAMBASSADOR_PARAMETER_TYPES["connect"][0],
            ("FederateAmbassador", "CallbackModel", "String"),
        )
        self.assertEqual(
            contracts.FEDERATE_AMBASSADOR_PARAMETER_TYPES["reflectAttributeValues"][0][-1],
            "SupplementalReflectInfo",
        )
        self.assertEqual(contracts.RTIAMBASSADOR_RETURN_TYPES["queryGALT"], ("TimeQueryReturn",))
        self.assertEqual(contracts.RTIAMBASSADOR_RETURN_TYPES["getOrderType"], ("OrderType",))
        self.assertEqual(
            set(contracts.RTIAMBASSADOR_PARAMETER_TYPES),
            set(contracts.RTIAMBASSADOR_OVERLOAD_COUNTS),
        )
        self.assertEqual(
            set(contracts.RTIAMBASSADOR_RETURN_TYPES),
            set(contracts.RTIAMBASSADOR_OVERLOAD_COUNTS),
        )
        self.assertTrue(
            all(
                len(overloads) == contracts.RTIAMBASSADOR_OVERLOAD_COUNTS[name]
                for name, overloads in contracts.RTIAMBASSADOR_PARAMETER_TYPES.items()
            )
        )
        self.assertTrue(
            all(
                len(overloads) == contracts.FEDERATE_AMBASSADOR_OVERLOAD_COUNTS[name]
                for name, overloads in contracts.FEDERATE_AMBASSADOR_PARAMETER_TYPES.items()
            )
        )
        self.assertIn("createFederationExecution", contracts.RTIAMBASSADOR_METHODS)
        self.assertIn("reflectAttributeValues", contracts.FEDERATE_AMBASSADOR_METHODS)
        self.assertEqual(rti.JAVA_PACKAGE, "hla.rti1516e")
        self.assertEqual(rti.CPP_NAMESPACE, "rti1516e")

    def test_standard_enum_members_are_edition_specific_and_complete(self) -> None:
        expected = {
            "CallbackModel": {"HLA_IMMEDIATE", "HLA_EVOKED"},
            "OrderType": {"RECEIVE", "TIMESTAMP"},
            "ServiceGroup": {
                "FEDERATION_MANAGEMENT",
                "DECLARATION_MANAGEMENT",
                "OBJECT_MANAGEMENT",
                "OWNERSHIP_MANAGEMENT",
                "TIME_MANAGEMENT",
                "DATA_DISTRIBUTION_MANAGEMENT",
                "SUPPORT_SERVICES",
            },
            "ResignAction": {
                "UNCONDITIONALLY_DIVEST_ATTRIBUTES",
                "DELETE_OBJECTS",
                "CANCEL_PENDING_OWNERSHIP_ACQUISITIONS",
                "DELETE_OBJECTS_THEN_DIVEST",
                "CANCEL_THEN_DELETE_THEN_DIVEST",
                "NO_ACTION",
            },
            "SynchronizationPointFailureReason": {
                "SYNCHRONIZATION_POINT_LABEL_NOT_UNIQUE",
                "SYNCHRONIZATION_SET_MEMBER_NOT_JOINED",
            },
            "SaveStatus": {
                "NO_SAVE_IN_PROGRESS",
                "FEDERATE_INSTRUCTED_TO_SAVE",
                "FEDERATE_SAVING",
                "FEDERATE_WAITING_FOR_FEDERATION_TO_SAVE",
            },
            "SaveFailureReason": {
                "RTI_UNABLE_TO_SAVE",
                "FEDERATE_REPORTED_FAILURE_DURING_SAVE",
                "FEDERATE_RESIGNED_DURING_SAVE",
                "RTI_DETECTED_FAILURE_DURING_SAVE",
                "SAVE_TIME_CANNOT_BE_HONORED",
                "SAVE_ABORTED",
            },
            "RestoreFailureReason": {
                "RTI_UNABLE_TO_RESTORE",
                "FEDERATE_REPORTED_FAILURE_DURING_RESTORE",
                "FEDERATE_RESIGNED_DURING_RESTORE",
                "RTI_DETECTED_FAILURE_DURING_RESTORE",
                "RESTORE_ABORTED",
            },
            "RestoreStatus": {
                "NO_RESTORE_IN_PROGRESS",
                "FEDERATE_RESTORE_REQUEST_PENDING",
                "FEDERATE_WAITING_FOR_RESTORE_TO_BEGIN",
                "FEDERATE_PREPARED_TO_RESTORE",
                "FEDERATE_RESTORING",
                "FEDERATE_WAITING_FOR_FEDERATION_TO_RESTORE",
            },
        }
        for class_name, names in expected.items():
            with self.subTest(class_name=class_name):
                self.assertEqual(
                    {member.name for member in getattr(rti, class_name)},
                    names,
                )

    def test_rti_contract_is_abstract_and_callback_sink_is_noop(self) -> None:
        with self.assertRaises(TypeError):
            rti.RTIambassador()
        with self.assertRaises(TypeError):
            rti.FederateHandleFactory()
        callback = rti.NullFederateAmbassador()
        self.assertIsNone(callback.connectionLost("test"))
        self.assertEqual(callback.__overload_counts__["reflectAttributeValues"], 3)

    def test_handles_are_immutable_and_edition_specific(self) -> None:
        source = bytearray(b"h")
        handle = rti.FederateHandle(source)
        source[:] = b"x"
        self.assertEqual(handle.encodedValue, b"h")
        self.assertEqual(handle.encodedLength(), 1)
        target = bytearray(2)
        handle.encode(target, 1)
        self.assertEqual(target, b"\x00h")
        self.assertTrue(handle.equals(rti.FederateHandle(b"h")))
        self.assertEqual(handle.hashCode(), hash(handle))

    def test_nested_callback_records_mirror_the_official_java_lookup(self) -> None:
        self.assertIs(rti.FederateAmbassador.SupplementalReflectInfo, rti.SupplementalReflectInfo)
        self.assertIs(rti.FederateAmbassador.SupplementalReceiveInfo, rti.SupplementalReceiveInfo)
        self.assertIs(rti.FederateAmbassador.SupplementalRemoveInfo, rti.SupplementalRemoveInfo)

    def test_parameter_value_map_has_both_standard_value_reference_overloads(self) -> None:
        handle = rti.ParameterHandle(b"p")
        values = rti.ParameterHandleValueMap({handle: b"payload"})
        self.assertEqual(values.getValueReference(handle), b"payload")
        wrapper = type("Wrapper", (), {})()
        wrapper.set = lambda value: setattr(wrapper, "value", value)
        self.assertIs(values.getValueReference(handle, wrapper), wrapper)
        self.assertEqual(wrapper.value, b"payload")

    def test_time_query_return_keeps_the_java_string_shape(self) -> None:
        value = rti.TimeQueryReturn(False, None)
        self.assertEqual(value.toString(), "false null")
        self.assertEqual(value, rti.TimeQueryReturn(False, rti.LogicalTime(value=9)))
        self.assertTrue(rti.RangeBounds(1, 2).equals(rti.RangeBounds(1, 2)))

    def test_factory_discovery_uses_2010_group_and_exact_names(self) -> None:
        with patch("hla.rti1516e.core.entry_points", return_value=[_EntryPoint()]):
            factory = rti.RtiFactoryFactory.getRtiFactory()
        self.assertEqual(factory.rtiName(), "test-2010")
        self.assertEqual(rti.RtiFactoryFactory._ENTRY_POINT_GROUP, "hla.rti1516e.factories")

        with patch("hla.rti1516e.core.entry_points", return_value=[_AliasEntryPoint()]):
            self.assertEqual(
                rti.RtiFactoryFactory.getRtiFactory("java-2010").rtiName(), "test-2010"
            )

        with patch("hla.rti1516e.core.entry_points", return_value=[]):
            with self.assertRaises(RTIinternalError):
                rti.RtiFactoryFactory.getRtiFactory()

    def test_logical_time_factory_discovery_keeps_the_java_class_overload(self) -> None:
        class TimeFactory(rti.LogicalTimeFactory):
            def decodeTime(self, buffer, offset=0):
                raise NotImplementedError

            def decodeInterval(self, buffer, offset=0):
                raise NotImplementedError

            def makeInitial(self):
                raise NotImplementedError

            def makeFinal(self):
                raise NotImplementedError

            def makeZero(self):
                raise NotImplementedError

            def makeEpsilon(self):
                raise NotImplementedError

            def getName(self):
                return "HLAinteger64Time"

        class EntryPoint:
            @staticmethod
            def load():
                return TimeFactory

        with patch("hla.rti1516e.core.entry_points", return_value=[EntryPoint()]):
            self.assertIsInstance(
                rti.LogicalTimeFactoryFactory.getLogicalTimeFactory(TimeFactory),
                TimeFactory,
            )

    def test_logical_time_factory_falls_back_to_edition_rti_providers(self) -> None:
        class TimeFactory(rti.LogicalTimeFactory):
            def __init__(self, name: str) -> None:
                self._name = name

            def decodeTime(self, buffer, offset=0):
                raise NotImplementedError

            def decodeInterval(self, buffer, offset=0):
                raise NotImplementedError

            def makeInitial(self):
                raise NotImplementedError

            def makeFinal(self):
                raise NotImplementedError

            def makeZero(self):
                raise NotImplementedError

            def makeEpsilon(self):
                raise NotImplementedError

            def getName(self):
                return self._name

        class Ambassador:
            def getTimeFactory(self):
                return TimeFactory("HLAinteger64Time")

            def getFloat64TimeFactory(self):
                return TimeFactory("HLAfloat64Time")

        class Provider(rti.RtiFactory):
            def getRtiAmbassador(self):
                return Ambassador()

            def getEncoderFactory(self):
                raise NotImplementedError

            def rtiName(self):
                return "provider-2010"

            def rtiVersion(self):
                return "test"

        class ProviderEntryPoint:
            @staticmethod
            def load():
                return Provider

        def entries(*, group: str):
            return [] if group == rti.LogicalTimeFactoryFactory._ENTRY_POINT_GROUP else [ProviderEntryPoint()]

        with patch("hla.rti1516e.core.entry_points", side_effect=entries):
            available = rti.LogicalTimeFactoryFactory.getAvailableLogicalTimeFactories()
            default = rti.LogicalTimeFactoryFactory.getLogicalTimeFactory()
        self.assertEqual({factory.getName() for factory in available}, {"HLAinteger64Time", "HLAfloat64Time"})
        self.assertIsNotNone(default)
        self.assertEqual(default.getName(), "HLAfloat64Time")

    def test_exception_names_are_constructible_and_unknowns_are_internal_errors(self) -> None:
        failure = exceptionForName("hla.rti1516e.exceptions.InconsistentFDD", "bad FDD")
        self.assertEqual(type(failure).__name__, "InconsistentFDD")
        self.assertEqual(str(failure), "bad FDD")
        unknown = exceptionForName("vendor.Unknown", "unknown")
        self.assertIsInstance(unknown, RTIinternalError)

    def test_every_standard_exception_name_round_trips_with_message_and_cause(self) -> None:
        cause = ValueError("provider detail")
        self.assertEqual(len(exceptions._STANDARD_EXCEPTION_NAMES), 110)
        for name in sorted(exceptions._STANDARD_EXCEPTION_NAMES):
            with self.subTest(name=name):
                failure = exceptionForName(
                    f"hla.rti1516e.exceptions.{name}",
                    f"message:{name}",
                    cause,
                )
                self.assertIs(type(failure), exceptions._EXCEPTION_TYPES[name])
                self.assertEqual(str(failure), f"message:{name}")
                self.assertIs(failure.cause, cause)
                self.assertIs(failure.__cause__, cause)

    def test_encoding_surface_is_provider_owned_and_abstract(self) -> None:
        with self.assertRaises(TypeError):
            EncoderFactory()
        with self.assertRaises(TypeError):
            HLAinteger32BE()
        self.assertEqual(HLAinteger32BE.__module__, "hla.rti1516e.encoding")
        wrapper = ByteWrapper(4)
        wrapper.put(b"abcd")
        wrapper.reset()
        self.assertEqual(wrapper.get(4), b"abcd")
        wrapper.reset()
        self.assertEqual(wrapper.get(), ord("a"))
        wrapper.reset()
        self.assertEqual(wrapper.getPos(), 0)
        wrapper.putInt(0x01020304)
        wrapper.reset()
        self.assertEqual(wrapper.getInt(), 0x01020304)
        self.assertEqual(str(wrapper), wrapper.toString())
        bounded = ByteWrapper(bytearray(b"abcdef"), 1, 2)
        self.assertEqual(bounded.slice().remaining(), 5)
        self.assertEqual(bounded.slice(2).remaining(), 2)
        self.assertIn("resize", HLAvariableArray.__abstractmethods__)
        self.assertIn("add", HLAfixedRecord.__abstractmethods__)
        self.assertNotIn("set", HLAfixedArray.__abstractmethods__)

    def test_order_type_uses_the_standard_one_octet_encoding(self) -> None:
        encoded = bytearray(1)
        rti.OrderType.RECEIVE.encode(encoded)
        self.assertEqual(encoded, b"\x01")
        self.assertIs(rti.OrderType.decode(encoded), rti.OrderType.RECEIVE)
        with self.assertRaises(CouldNotDecode):
            rti.OrderType.decode(b"\x03")


if __name__ == "__main__":
    unittest.main()
