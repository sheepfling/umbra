from pathlib import Path
import re
import unittest
from array import array
from unittest.mock import patch

import hla.rti1516_2025.exceptions as exceptions
from hla.rti1516_2025 import (
    AdditionalSettingsResultCode,
    AttributeHandle,
    AttributeHandleSet,
    AttributeHandleValueMap,
    AttributeSetRegionSetPair,
    AttributeSetRegionSetPairList,
    AttributeRegionAssociation,
    MutableAttributeSetRegionSetPairList,
    CallbackModel,
    ConfigurationResult,
    DimensionHandle,
    DimensionHandleSet,
    FederateHandle,
    FederationExecutionMemberInformation,
    FederationExecutionMemberInformationSet,
    HLAinteger64Interval,
    HLAinteger64Time,
    InteractionClassHandle,
    MutableInteractionClassHandleSet,
    MutableAttributeHandleSet,
    MutableAttributeHandleValueMap,
    MutableDimensionHandleSet,
    MutableFederateHandleSet,
    MutableParameterHandleValueMap,
    MutableRegionHandleSet,
    LogicalTime,
    LogicalTimeInterval,
    RangeBounds,
    RegionHandle,
    RegionHandleSet,
    ObjectClassHandle,
    ObjectInstanceHandle,
    OrderType,
    ParameterHandle,
    ParameterHandleValueMap,
    RtiFactory,
    RtiFactoryFactory,
    RtiConfiguration,
    ResignAction,
    ServiceGroup,
    TransportationTypeHandle,
)
from hla.rti1516_2025.auth import Credentials, HLAnoCredentials
from hla.rti1516_2025.encoding import (
    DataElement,
    DataElementFactory,
    EncoderFactory,
    HLAfixedArray,
    HLAboolean,
    HLAASCIIchar,
    HLAASCIIstring,
    HLAbyte,
    HLAfloat64BE,
    HLAfloat64LE,
    HLAfloat32BE,
    HLAfloat32LE,
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
)
from hla.rti1516_2025.exceptions import RTIinternalError, UnsupportedCallbackModel
from hla.rti1516_2025.core import _require_callback_model
from hla.rti1516_2025.exceptions import (
    ConnectionFailed,
    FederateInternalError,
    RTIexception,
    exceptionForName,
)


class _AliasFactory(RtiFactory):
    def getRtiAmbassador(self):
        raise NotImplementedError

    def getEncoderFactory(self):
        raise NotImplementedError

    def rtiName(self) -> str:
        return "A dynamically discovered vendor name"

    def rtiVersion(self) -> str:
        return "test"


class _EntryPoint:
    name = "java"

    @staticmethod
    def load():
        return _AliasFactory


class ContractsTest(unittest.TestCase):
    def test_foundation_values_are_stable_and_pythonic(self) -> None:
        result = ConfigurationResult(
            configurationUsed=False,
            addressUsed=False,
            additionalSettingsResultCode=AdditionalSettingsResultCode.SETTINGS_IGNORED,
        )

        self.assertEqual(CallbackModel.HLA_EVOKED.name, "HLA_EVOKED")
        self.assertEqual(ServiceGroup.DATA_DISTRIBUTION_MANAGEMENT.name, "DATA_DISTRIBUTION_MANAGEMENT")
        self.assertFalse(result.configurationUsed)
        self.assertEqual(result.message, "")

    def test_entry_point_alias_can_select_a_lazy_transport(self) -> None:
        with patch("hla.rti1516_2025.core.entry_points", return_value=[_EntryPoint()]):
            factory = RtiFactoryFactory.getRtiFactory("java")

        self.assertEqual(factory.rtiName(), "A dynamically discovered vendor name")

    def test_available_factories_are_materialized_from_entry_points(self) -> None:
        with patch("hla.rti1516_2025.core.entry_points", return_value=[_EntryPoint()]):
            factories = RtiFactoryFactory.getAvailableRtiFactories()

        self.assertEqual(len(factories), 1)
        self.assertEqual(factories[0].rtiName(), "A dynamically discovered vendor name")

    def test_duplicate_entry_point_aliases_are_rejected(self) -> None:
        with patch(
            "hla.rti1516_2025.core.entry_points",
            return_value=[_EntryPoint(), _EntryPoint()],
        ):
            with self.assertRaises(RTIinternalError):
                RtiFactoryFactory.getRtiFactory("java")

    def test_invalid_callback_models_have_a_standard_provider_error(self) -> None:
        self.assertEqual(_require_callback_model(CallbackModel.HLA_IMMEDIATE), CallbackModel.HLA_IMMEDIATE)
        with self.assertRaises(UnsupportedCallbackModel):
            _require_callback_model("HLA_IMMEDIATE")

    def test_connection_value_objects_keep_the_java_builder_and_data_shape(self) -> None:
        configuration = (
            RtiConfiguration.createConfiguration()
            .withConfigurationName("test")
            .withRtiAddress("rti.example")
            .withAdditionalSettings("mode=test")
        )
        credentials = Credentials("token", bytearray(b"secret"))

        self.assertEqual(configuration.configurationName(), "test")
        self.assertEqual(configuration.rtiAddress(), "rti.example")
        self.assertEqual(configuration.additionalSettings(), "mode=test")
        self.assertEqual(credentials.getType(), "token")
        self.assertEqual(credentials.getData(), b"secret")
        self.assertEqual(HLAnoCredentials().getData(), b"")

    def test_federation_member_callback_values_are_immutable_snapshots(self) -> None:
        member = FederationExecutionMemberInformation("dish-service", "restaurant")
        report = FederationExecutionMemberInformationSet([member])

        self.assertEqual(member.federateName, "dish-service")
        self.assertEqual(member.federateType, "restaurant")
        self.assertEqual(report, FederationExecutionMemberInformationSet([member]))
        with self.assertRaises(AttributeError):
            member.federateName = "other"  # type: ignore[misc]

    def test_federate_handles_copy_encoded_bytes_and_resign_actions_keep_java_names(self) -> None:
        handle = FederateHandle(bytearray(b"handle"))
        destination = bytearray(b"xx------")

        handle.encode(destination, 2)
        self.assertEqual(handle.encodedValue, b"handle")
        self.assertEqual(handle.encodedLength(), 6)
        self.assertEqual(destination, b"xxhandle")
        self.assertEqual(ResignAction.NO_ACTION.name, "NO_ACTION")
        self.assertEqual(OrderType.RECEIVE.name, "RECEIVE")

    def test_read_only_binary_inputs_accept_buffer_protocol_values(self) -> None:
        source = array("B", b"handle")
        handle = FederateHandle(source)
        destination = memoryview(bytearray(8))

        source[0] = ord("H")
        handle.encode(destination, 1)

        self.assertEqual(handle.encodedValue, b"handle")
        self.assertEqual(destination.tobytes(), b"\x00handle\x00")
        self.assertEqual(Credentials("token", memoryview(b"secret")).getData(), b"secret")

    def test_portable_handle_domains_are_immutable_and_remain_distinct(self) -> None:
        encoded = bytearray(b"provider-owned-handle")
        handles = (
            FederateHandle(encoded),
            ObjectClassHandle(encoded),
            ObjectInstanceHandle(encoded),
            AttributeHandle(encoded),
            InteractionClassHandle(encoded),
            ParameterHandle(encoded),
            TransportationTypeHandle(encoded),
            DimensionHandle(encoded),
        )

        encoded[:] = b"x" * len(encoded)
        self.assertTrue(all(handle.encodedValue == b"provider-owned-handle" for handle in handles))
        self.assertEqual(len(set(handles)), len(handles))
        destination = bytearray(len(handles[1].encodedValue))
        handles[1].encode(destination)
        self.assertEqual(bytes(destination), handles[1].encodedValue)
        self.assertEqual(AttributeHandleSet([handles[2], handles[2]]), AttributeHandleSet([handles[2]]))
        with self.assertRaises(AttributeError):
            handles[1].encodedValue = b"other"  # type: ignore[misc]

    def test_attribute_handle_value_maps_copy_bytes_and_enforce_their_key_domain(self) -> None:
        attribute = AttributeHandle(b"attribute")
        source_value = bytearray(b"value")
        values = AttributeHandleValueMap({attribute: source_value})

        source_value[:] = b"other"
        self.assertEqual(values, {attribute: b"value"})
        self.assertEqual(values[attribute], b"value")
        with self.assertRaises(TypeError):
            AttributeHandleValueMap({ObjectClassHandle(b"object"): b"value"})
        with self.assertRaises(TypeError):
            values[attribute] = b"other"  # type: ignore[index]

    def test_factory_builders_are_mutable_but_keep_provider_handle_domains(self) -> None:
        attribute = AttributeHandle(b"attribute")
        dimension = DimensionHandle(b"dimension")
        federate = FederateHandle(b"federate")
        region = RegionHandle(b"region")

        attribute_set = MutableAttributeHandleSet()
        dimension_set = MutableDimensionHandleSet()
        federate_set = MutableFederateHandleSet()
        region_set = MutableRegionHandleSet()
        attribute_set.add(attribute)
        dimension_set.add(dimension)
        federate_set.add(federate)
        region_set.add(region)
        self.assertEqual(tuple(attribute_set), (attribute,))
        self.assertEqual(tuple(dimension_set), (dimension,))
        self.assertEqual(tuple(federate_set), (federate,))
        self.assertEqual(tuple(region_set), (region,))
        with self.assertRaises(TypeError):
            attribute_set.add(ObjectClassHandle(b"wrong"))  # type: ignore[arg-type]

        attribute_values = MutableAttributeHandleValueMap()
        parameter_values = MutableParameterHandleValueMap()
        source = bytearray(b"value")
        attribute_values[attribute] = source
        parameter_values[ParameterHandle(b"parameter")] = source
        source[:] = b"other"
        self.assertEqual(attribute_values[attribute], b"value")
        self.assertEqual(parameter_values[ParameterHandle(b"parameter")], b"value")
        with self.assertRaises(TypeError):
            attribute_values[ObjectClassHandle(b"wrong")] = b"value"  # type: ignore[index]

    def test_java_shaped_interaction_and_attribute_region_factories_are_portable(self) -> None:
        interaction = InteractionClassHandle(b"interaction")
        attribute = AttributeHandle(b"attribute")
        region = RegionHandle(b"region")

        interactions = MutableInteractionClassHandleSet()
        interactions.add(interaction)
        self.assertEqual(tuple(interactions), (interaction,))
        with self.assertRaises(TypeError):
            interactions.add(ObjectClassHandle(b"wrong"))  # type: ignore[arg-type]

        association = AttributeRegionAssociation(
            AttributeHandleSet([attribute]), RegionHandleSet([region])
        )
        self.assertEqual(association.attributes, association.ahset)
        self.assertEqual(association.regions, association.rhset)
        pairs = MutableAttributeSetRegionSetPairList(2)
        pairs.add(association)
        self.assertEqual(len(pairs), 1)
        self.assertEqual(AttributeSetRegionSetPairList(pairs)[0], association)

    def test_parameter_handle_value_maps_copy_bytes_and_enforce_their_key_domain(self) -> None:
        parameter = ParameterHandle(b"parameter")
        source_value = bytearray(b"value")
        values = ParameterHandleValueMap({parameter: source_value})

        source_value[:] = b"other"
        self.assertEqual(values, {parameter: b"value"})
        with self.assertRaises(TypeError):
            ParameterHandleValueMap({AttributeHandle(b"attribute"): b"value"})

    def test_encoding_contract_is_provider_owned_and_not_python_constructed(self) -> None:
        self.assertTrue(issubclass(HLAinteger32BE, DataElement))
        self.assertTrue(issubclass(HLAinteger16BE, DataElement))
        self.assertTrue(issubclass(HLAfloat64BE, DataElement))
        self.assertTrue(issubclass(HLAinteger32LE, DataElement))
        self.assertTrue(issubclass(HLAfloat64LE, DataElement))
        self.assertTrue(issubclass(HLAfloat32BE, DataElement))
        self.assertTrue(issubclass(HLAfloat32LE, DataElement))
        self.assertTrue(issubclass(HLAunsignedInteger16BE, DataElement))
        self.assertTrue(issubclass(HLAunsignedInteger16LE, DataElement))
        self.assertTrue(issubclass(HLAinteger64BE, DataElement))
        self.assertTrue(issubclass(HLAinteger16LE, DataElement))
        self.assertTrue(issubclass(HLAinteger64LE, DataElement))
        self.assertTrue(issubclass(HLAunsignedInteger64BE, DataElement))
        self.assertTrue(issubclass(HLAunsignedInteger32LE, DataElement))
        self.assertTrue(issubclass(HLAunsignedInteger64LE, DataElement))
        self.assertTrue(issubclass(HLAunsignedInteger32BE, DataElement))
        self.assertTrue(issubclass(HLAboolean, DataElement))
        self.assertTrue(issubclass(HLAASCIIchar, DataElement))
        self.assertTrue(issubclass(HLAASCIIstring, DataElement))
        self.assertTrue(issubclass(HLAunicodeChar, DataElement))
        self.assertTrue(issubclass(HLAbyte, DataElement))
        self.assertTrue(issubclass(HLAoctet, DataElement))
        self.assertTrue(issubclass(HLAoctetPairBE, DataElement))
        self.assertTrue(issubclass(HLAoctetPairLE, DataElement))
        self.assertTrue(issubclass(HLAopaqueData, DataElement))
        self.assertTrue(issubclass(HLAvariableArray, DataElement))
        self.assertTrue(issubclass(DataElementFactory, object))
        self.assertTrue(issubclass(HLAfixedArray, DataElement))
        self.assertTrue(issubclass(HLAunicodeString, DataElement))
        self.assertFalse(hasattr(EncoderFactory, "createHLAextendableVariantRecord"))
        with self.assertRaises(TypeError):
            EncoderFactory()
        with self.assertRaises(TypeError):
            HLAinteger32BE()

    def test_logical_time_snapshots_preserve_provider_encoding_and_java_shape(self) -> None:
        source = bytearray(b"\x00\x00\x00\x00\x00\x00\x00\x05")
        time = HLAinteger64Time(source, "HLAinteger64Time", value=5, text="5")
        interval = HLAinteger64Interval(b"\x00" * 8, "HLAinteger64Time", zero=True, value=0)
        source[:] = b"x" * 8

        self.assertIsInstance(time, LogicalTime)
        self.assertIsInstance(interval, LogicalTimeInterval)
        self.assertEqual(time.getTime(), 5)
        self.assertEqual(time.implementationName(), "HLAinteger64Time")
        self.assertEqual(time.toByteArray(), b"\x00\x00\x00\x00\x00\x00\x00\x05")
        self.assertEqual(interval.getInterval(), 0)
        self.assertTrue(interval.isZero())
        destination = bytearray(9)
        time.encode(destination, 1)
        self.assertEqual(bytes(destination), b"\x00\x00\x00\x00\x00\x00\x00\x00\x05")
        with self.assertRaises(AttributeError):
            time.value = 7  # type: ignore[misc]

    def test_region_values_keep_distinct_handle_sets_and_java_range_methods(self) -> None:
        dimension = DimensionHandle(b"dimension")
        region = RegionHandle(b"region")
        bounds = RangeBounds(1, 3)

        self.assertEqual(DimensionHandleSet([dimension]), DimensionHandleSet([dimension]))
        self.assertEqual(RegionHandleSet([region]), RegionHandleSet([region]))
        self.assertIsInstance(region, RegionHandle)
        self.assertEqual(bounds.getLowerBound(), 1)
        self.assertEqual(bounds.getUpperBound(), 3)
        bounds.setLowerBound(2)
        bounds.setUpperBound(4)
        self.assertEqual((bounds.getLowerBound(), bounds.getUpperBound()), (2, 4))

    def test_attribute_region_pair_vectors_are_immutable_and_domain_checked(self) -> None:
        attribute = AttributeHandle(b"attribute")
        region = RegionHandle(b"region")
        pair = AttributeSetRegionSetPair(
            AttributeHandleSet([attribute]), RegionHandleSet([region])
        )
        pairs = AttributeSetRegionSetPairList([pair])

        self.assertEqual(pairs[0], pair)
        self.assertEqual(pairs[0].attributes, AttributeHandleSet([attribute]))
        self.assertEqual(pairs[0].regions, RegionHandleSet([region]))
        with self.assertRaises(TypeError):
            AttributeSetRegionSetPair(AttributeHandleSet([attribute]), DimensionHandleSet())  # type: ignore[arg-type]
        with self.assertRaises(TypeError):
            AttributeSetRegionSetPairList([("not a pair",)])  # type: ignore[list-item]

    def test_all_provider_edges_map_official_exception_names_to_specific_types(self) -> None:
        exception_header = (
            Path(__file__).parents[3]
            / "third_party"
            / "ieee1516.1-2025"
            / "include"
            / "RTI"
            / "Exception.h"
        )
        expected_names = set(
            re.findall(
                r"RTI_EXCEPTION\(([A-Za-z_][A-Za-z0-9_]*)\)",
                exception_header.read_text(encoding="utf-8"),
            )
        ) - {"A"}
        self.assertEqual(set(exceptions._EXCEPTION_TYPES), expected_names)
        self.assertIsInstance(exceptionForName("ConnectionFailed", "offline"), ConnectionFailed)
        self.assertIsInstance(
            exceptionForName("FederateInternalError", "callback failed"),
            FederateInternalError,
        )
        self.assertIsInstance(exceptionForName("vendor-only-error", "detail"), RTIexception)
