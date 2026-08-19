import unittest
import math
from pathlib import Path
from uuid import uuid4

from hla.rti1516_2025 import (
    AttributeHandle,
    AttributeHandleSet,
    AttributeHandleValueMap,
    AttributeSetRegionSetPair,
    AttributeSetRegionSetPairList,
    CallbackModel,
    DimensionHandle,
    DimensionHandleSet,
    EncoderFactory,
    FederateAmbassador,
    FederateHandle,
    FederateHandleSet,
    FederateHandleSaveStatusPair,
    FederateRestoreStatus,
    FederationExecutionInformation,
    FederationExecutionMemberInformationSet,
    FederationExecutionMemberInformation,
    ResignAction,
    SaveStatus,
    SaveFailureReason,
    RestoreFailureReason,
    RestoreStatus,
    SynchronizationPointFailureReason,
    HLAboolean,
    HLAinteger64Interval,
    HLAinteger64Time,
    HLAinteger64TimeFactory,
    HLAfloat64Interval,
    HLAfloat64Time,
    HLAfloat64TimeFactory,
    HLAinteger32BE,
    HLAunicodeString,
    HLAunsignedInteger32BE,
    InteractionClassHandle,
    InteractionClassHandleSet,
    MessageRetractionHandle,
    ObjectClassHandle,
    ObjectInstanceHandle,
    ObjectInstanceNameSet,
    OrderType,
    ParameterHandle,
    ParameterHandleValueMap,
    RtiFactoryFactory,
    RangeBounds,
    RegionHandle,
    RegionHandleSet,
    TransportationTypeHandle,
    ServiceGroup,
)
from hla.rti1516_2025.exceptions import (
    AlreadyConnected,
    FederateOwnsAttributes,
    InvalidLogicalTime,
    InvalidLogicalTimeInterval,
    InvalidRegion,
    MessageCanNoLongerBeRetracted,
    ObjectInstanceNotKnown,
)
from hla.rti1516_2025.encoding import DecoderException
from hla.rti1516_2025.testing import (
    ConnectionFoundationConformanceMixin,
    FederationExecutionDiscoveryConformanceMixin,
    FederationExecutionMemberDiscoveryConformanceMixin,
    ConnectionOverloadConformanceMixin,
)
from umbra._native.rti1516_2025 import UmbraRtiFactory


class NativeProviderTest(unittest.TestCase):
    def test_factory_is_discoverable_from_the_standard_api_namespace(self) -> None:
        self.assertEqual(RtiFactoryFactory.getRtiFactory("Umbra").rtiName(), "Umbra")

    def test_native_encoder_factory_uses_real_cpp_data_elements(self) -> None:
        encoder = UmbraRtiFactory().getEncoderFactory()
        integer = encoder.createHLAinteger32BE(-2)
        unsigned = encoder.createHLAunsignedInteger32BE(0x1234ABCD)
        unsigned_maximum = encoder.createHLAunsignedInteger32BE(0xFFFFFFFF)
        boolean = encoder.createHLAboolean(True)
        unicode = encoder.createHLAunicodeString("A😀")

        self.assertIsInstance(encoder, EncoderFactory)
        self.assertIsInstance(integer, HLAinteger32BE)
        self.assertIsInstance(unsigned, HLAunsignedInteger32BE)
        self.assertIsInstance(boolean, HLAboolean)
        self.assertIsInstance(unicode, HLAunicodeString)
        self.assertEqual(integer.toByteArray(), b"\xff\xff\xff\xfe")
        self.assertEqual(unsigned.toByteArray(), b"\x124\xab\xcd")
        self.assertEqual(unsigned_maximum.toByteArray(), b"\xff\xff\xff\xff")
        self.assertEqual(unsigned_maximum.getValue(), 0xFFFFFFFF)
        self.assertEqual(boolean.toByteArray(), b"\x00\x00\x00\x01")
        self.assertEqual(unicode.toByteArray(), b"\x00\x00\x00\x03\x00A\xd8=\xde\x00")
        self.assertEqual(integer.getEncodedLength(), 4)
        self.assertEqual(unicode.getOctetBoundary(), 4)
        self.assertIs(integer.decode(b"\x00\x00\x00\x07"), integer)
        self.assertEqual(integer.getValue(), 7)
        self.assertIs(boolean.setValue(False), boolean)
        self.assertEqual(boolean.toByteArray(), b"\x00\x00\x00\x00")
        with self.assertRaises(DecoderException):
            boolean.decode(b"\x00\x00\x00\x02")
        self.assertIs(unicode.setValue("reset"), unicode)
        self.assertEqual(unicode.getValue(), "reset")

    def test_native_connection_foundation(self) -> None:
        factory = UmbraRtiFactory()
        ambassador = factory.getRtiAmbassador()
        result = ambassador.connect(FederateAmbassador(), CallbackModel.HLA_EVOKED)

        self.assertEqual(factory.rtiName(), "Umbra")
        self.assertFalse(result.configurationUsed)
        ambassador.disconnect()

    def test_native_logical_time_factory_and_time_advance_callbacks(self) -> None:
        class RecordingFederateAmbassador(FederateAmbassador):
            def __init__(self) -> None:
                self.regulation: list[HLAinteger64Time] = []
                self.constrained: list[HLAinteger64Time] = []
                self.grants: list[HLAinteger64Time] = []
                self.reflections = []
                self.interactions = []
                self.removals = []
                self.discoveries = []

            def timeRegulationEnabled(self, time) -> None:
                self.regulation.append(time)

            def timeConstrainedEnabled(self, time) -> None:
                self.constrained.append(time)

            def timeAdvanceGrant(self, time) -> None:
                self.grants.append(time)

            def reflectAttributeValues(self, *arguments) -> None:
                self.reflections.append(arguments)

            def discoverObjectInstance(self, *arguments) -> None:
                self.discoveries.append(arguments)

            def receiveInteraction(self, *arguments) -> None:
                self.interactions.append(arguments)

            def removeObjectInstance(self, *arguments) -> None:
                self.removals.append(arguments)

        fom_module = (
            Path(__file__).parents[3]
            / "third_party"
            / "ieee1516.2-2025"
            / "resources"
            / "examples"
            / "RestaurantFOMmodule-2025.xml"
        )
        federation_name = f"python-native-time-{uuid4()}"
        callback = RecordingFederateAmbassador()
        regulator_callback = RecordingFederateAmbassador()
        ambassador = UmbraRtiFactory().getRtiAmbassador()
        regulator = UmbraRtiFactory().getRtiAmbassador()
        ambassador.connect(callback, CallbackModel.HLA_EVOKED)
        regulator.connect(regulator_callback, CallbackModel.HLA_EVOKED)
        created = joined = regulator_joined = False
        try:
            regulator.createFederationExecution(federation_name, str(fom_module), "HLAinteger64Time")
            created = True
            ambassador.joinFederationExecution("time-observer", federation_name)
            joined = True
            regulator.joinFederationExecution("time-regulator", federation_name)
            regulator_joined = True

            factory = ambassador.getTimeFactory()
            self.assertIsInstance(factory, HLAinteger64TimeFactory)
            self.assertEqual(factory.implementationName(), "HLAinteger64Time")
            initial = factory.makeInitial()
            final = factory.makeFinal()
            zero = factory.makeZero()
            epsilon = factory.makeEpsilon()
            time = factory.makeLogicalTime(5)
            interval = factory.makeLogicalTimeInterval(1)
            self.assertIsInstance(time, HLAinteger64Time)
            self.assertIsInstance(interval, HLAinteger64Interval)
            self.assertTrue(initial.isInitial())
            self.assertTrue(final.isFinal())
            self.assertTrue(zero.isZero())
            self.assertTrue(epsilon.isEpsilon())
            self.assertEqual(time.getTime(), 5)
            self.assertEqual(interval.getInterval(), 1)
            self.assertEqual(factory.decodeLogicalTime(time.toByteArray()).getTime(), 5)
            self.assertEqual(
                factory.decodeLogicalTimeInterval(interval.toByteArray()).getInterval(), 1
            )
            advanced = factory.add(time, interval)
            self.assertEqual(advanced.getTime(), 6)
            self.assertEqual(factory.subtract(advanced, interval).getTime(), 5)
            self.assertEqual(factory.difference(advanced, time).getInterval(), 1)

            ambassador.enableTimeConstrained()
            ambassador.evokeCallback(0.0)
            self.assertEqual([entry.getTime() for entry in callback.constrained], [0])
            regulator.enableTimeRegulation(interval)
            regulator.evokeCallback(0.0)
            self.assertEqual([entry.getTime() for entry in regulator_callback.regulation], [0])
            self.assertEqual(regulator.queryLookahead().getInterval(), 1)
            modified_interval = factory.makeLogicalTimeInterval(2)
            regulator.modifyLookahead(modified_interval)
            self.assertEqual(regulator.queryLookahead().getInterval(), 2)
            regulator.enableAsynchronousDelivery()
            regulator.disableAsynchronousDelivery()
            timestamp_class = regulator.getObjectClassHandle("HLAobjectRoot.Employee.Server")
            timestamp_attribute = regulator.getAttributeHandle(timestamp_class, "Efficiency")
            observer_class = ambassador.getObjectClassHandle("HLAobjectRoot.Employee.Server")
            observer_attribute = ambassador.getAttributeHandle(observer_class, "Efficiency")
            ambassador.subscribeObjectClassAttributes(
                observer_class, AttributeHandleSet([observer_attribute])
            )
            regulator.publishObjectClassAttributes(
                timestamp_class, AttributeHandleSet([timestamp_attribute])
            )
            timestamp_instance = regulator.registerObjectInstance(timestamp_class)
            # The native object-management path resolves the producer's
            # instance on the receiving ambassador before it can route
            # timestamped attribute updates/deletions.  Drain discovery first
            # so the timed callbacks exercise the same lifecycle as a real
            # federate.
            for _ in range(8):
                ambassador.evokeMultipleCallbacks(0.0, 0.0)
            self.assertTrue(callback.discoveries)
            timestamp_interaction = regulator.getInteractionClassHandle(
                "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed"
            )
            timestamp_parameter = regulator.getParameterHandle(
                timestamp_interaction, "TemperatureOk"
            )
            observer_interaction = ambassador.getInteractionClassHandle(
                "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed"
            )
            ambassador.subscribeInteractionClass(observer_interaction)
            regulator.publishInteractionClass(timestamp_interaction)
            timestamp_region_dimension = regulator.getDimensionHandle("ServerId")
            timestamp_region = regulator.createRegion(
                DimensionHandleSet([timestamp_region_dimension])
            )
            regulator.setRangeBounds(
                timestamp_region, timestamp_region_dimension, RangeBounds(0, 1)
            )
            regulator.commitRegionModifications(RegionHandleSet([timestamp_region]))
            timestamp = factory.makeLogicalTime(5)
            timestamped_update = regulator.updateAttributeValuesWithTime(
                timestamp_instance,
                AttributeHandleValueMap({timestamp_attribute: b"timestamped"}),
                timestamp,
                b"timestamped-update",
            )
            timestamped_interaction = regulator.sendInteractionWithTime(
                timestamp_interaction,
                ParameterHandleValueMap({timestamp_parameter: b"served"}),
                timestamp,
                b"timestamped-interaction",
            )
            timestamped_regional_interaction = regulator.sendInteractionWithRegionsWithTime(
                timestamp_interaction,
                ParameterHandleValueMap({timestamp_parameter: b"regional"}),
                RegionHandleSet([timestamp_region]),
                timestamp,
                b"timestamped-regional-interaction",
            )
            for retraction in (
                timestamped_update,
                timestamped_interaction,
                timestamped_regional_interaction,
            ):
                self.assertIsInstance(retraction, MessageRetractionHandle)
                self.assertTrue(retraction.isValid())
            ambassador.timeAdvanceRequest(time)
            regulator.timeAdvanceRequest(time)
            for _ in range(12):
                ambassador.evokeMultipleCallbacks(0.0, 0.0)
                regulator.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual([entry.getTime() for entry in callback.grants], [5])
            self.assertEqual(ambassador.queryLogicalTime().getTime(), 5)
            galt = regulator.queryGALT()
            lits = regulator.queryLITS()
            self.assertFalse(galt.timeIsValid)
            self.assertIsNone(galt.time)
            self.assertFalse(lits.timeIsValid)
            self.assertIsNone(lits.time)
            self.assertTrue(callback.reflections, (callback.reflections, callback.interactions, callback.removals))
            self.assertTrue(callback.interactions, (callback.reflections, callback.interactions, callback.removals))
            self.assertEqual(callback.reflections[-1][0], timestamp_instance)
            self.assertEqual(callback.reflections[-1][6].getTime(), 5)
            self.assertEqual(callback.reflections[-1][7], OrderType.TIMESTAMP)
            self.assertIsInstance(callback.reflections[-1][9], MessageRetractionHandle)
            self.assertEqual(callback.interactions[-1][0], timestamp_interaction)
            self.assertEqual(callback.interactions[-1][6].getTime(), 5)
            self.assertEqual(callback.interactions[-1][7], OrderType.TIMESTAMP)
            # The modified lookahead is two logical units, so the delete must
            # be at least two units beyond the sender's current time (5).
            delete_time = factory.makeLogicalTime(7)
            timestamped_delete = regulator.deleteObjectInstanceWithTime(
                timestamp_instance, delete_time, b"timestamped-delete"
            )
            self.assertIsInstance(timestamped_delete, MessageRetractionHandle)
            self.assertTrue(timestamped_delete.isValid())
            ambassador.timeAdvanceRequest(delete_time)
            regulator.timeAdvanceRequest(delete_time)
            for _ in range(8):
                ambassador.evokeMultipleCallbacks(0.0, 0.0)
                regulator.evokeMultipleCallbacks(0.0, 0.0)
            self.assertTrue(callback.removals)
            self.assertEqual(callback.removals[-1][3].getTime(), 7)
            for request, value in (
                (regulator.timeAdvanceRequestAvailable, 8),
                (regulator.nextMessageRequest, 9),
                (regulator.nextMessageRequestAvailable, 10),
                (regulator.flushQueueRequest, 11),
            ):
                request(factory.makeLogicalTime(value))
                regulator.evokeCallback(0.0)
            self.assertEqual(
                [entry.getTime() for entry in regulator_callback.grants], [5, 7, 8, 9, 10]
            )
            ambassador.disableTimeConstrained()
            regulator.disableTimeRegulation()
        finally:
            if joined:
                ambassador.resignFederationExecution(ResignAction.NO_ACTION)
            if regulator_joined:
                regulator.resignFederationExecution(ResignAction.NO_ACTION)
            if created:
                regulator.destroyFederationExecution(federation_name)
            ambassador.disconnect()
            regulator.disconnect()

    def test_native_floating_time_edges_match_reference_factory(self) -> None:
        class RecordingFederateAmbassador(FederateAmbassador):
            def __init__(self) -> None:
                self.grants: list[object] = []

            def timeAdvanceGrant(self, time) -> None:
                self.grants.append(time)

        fom_module = (
            Path(__file__).parents[3]
            / "cpp"
            / "tests"
            / "data"
            / "switch-support-enabled-fom.xml"
        )
        federation_name = f"python-native-floating-time-{uuid4()}"
        callback = RecordingFederateAmbassador()
        ambassador = UmbraRtiFactory().getRtiAmbassador()
        created = joined = False
        try:
            ambassador.connect(callback, CallbackModel.HLA_EVOKED)
            ambassador.createFederationExecution(
                federation_name, str(fom_module), "HLAfloat64Time"
            )
            created = True
            ambassador.joinFederationExecution("floating-time", federation_name)
            joined = True

            factory = ambassador.getTimeFactory()
            self.assertIsInstance(factory, HLAfloat64TimeFactory)
            epsilon = factory.makeEpsilon()
            self.assertIsInstance(epsilon, HLAfloat64Interval)
            self.assertEqual(epsilon.getInterval(), math.nextafter(0.0, 1.0))
            self.assertTrue(epsilon.isEpsilon())

            negative_zero = factory.makeLogicalTime(-0.0)
            self.assertIsInstance(negative_zero, HLAfloat64Time)
            self.assertEqual(negative_zero.getTime(), 0.0)
            self.assertEqual(negative_zero.toByteArray(), b"\x00" * 8)
            smallest = factory.makeLogicalTime(math.nextafter(0.0, 1.0))
            self.assertEqual(
                factory.decodeLogicalTime(smallest.toByteArray()).getTime(),
                math.nextafter(0.0, 1.0),
            )
            base = factory.makeLogicalTime(1.0)
            advanced = factory.add(base, epsilon)
            self.assertEqual(advanced.getTime(), math.nextafter(1.0, math.inf))
            self.assertEqual(factory.subtract(advanced, epsilon).getTime(), 1.0)
            ambassador.timeAdvanceRequest(smallest)
            ambassador.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(callback.grants[-1].getTime(), math.nextafter(0.0, 1.0))

            for value in (-1.0, math.inf, math.nan):
                with self.assertRaises(InvalidLogicalTime):
                    factory.makeLogicalTime(value)
                with self.assertRaises(InvalidLogicalTimeInterval):
                    factory.makeLogicalTimeInterval(value)
        finally:
            if joined:
                ambassador.resignFederationExecution(ResignAction.NO_ACTION)
            if created:
                ambassador.destroyFederationExecution(federation_name)
            ambassador.disconnect()

    def test_native_timestamped_retraction_and_flush_callbacks_cross_pybind(self) -> None:
        class RecordingFederateAmbassador(FederateAmbassador):
            def __init__(self) -> None:
                self.interactions: list[tuple[object, ...]] = []
                self.flush_grants: list[tuple[object, object]] = []
                self.request_retractions: list[MessageRetractionHandle] = []

            def receiveInteraction(self, *arguments) -> None:
                self.interactions.append(arguments)

            def flushQueueGrant(self, time, optimisticTime) -> None:
                self.flush_grants.append((time, optimisticTime))

            def requestRetraction(self, retraction) -> None:
                self.request_retractions.append(retraction)

        fom_module = (
            Path(__file__).parents[3]
            / "cpp"
            / "tests"
            / "data"
            / "parameter-handle-provider-fom.xml"
        )
        federation_name = f"python-native-retraction-callbacks-{uuid4()}"
        publisher_callback = RecordingFederateAmbassador()
        receiver_callback = RecordingFederateAmbassador()
        publisher = UmbraRtiFactory().getRtiAmbassador()
        receiver = UmbraRtiFactory().getRtiAmbassador()
        created = publisher_joined = receiver_joined = False
        try:
            publisher.connect(publisher_callback, CallbackModel.HLA_EVOKED)
            receiver.connect(receiver_callback, CallbackModel.HLA_EVOKED)
            publisher.createFederationExecution(
                federation_name, str(fom_module), "HLAinteger64Time"
            )
            created = True
            publisher.joinFederationExecution("timestamped-publisher", federation_name)
            publisher_joined = True
            receiver.joinFederationExecution("timestamped-receiver", federation_name)
            receiver_joined = True

            interaction = publisher.getInteractionClassHandle(
                "HLAinteractionRoot.UmbraParameterFixtureBase.UmbraParameterFixtureChild"
            )
            identifier = publisher.getParameterHandle(interaction, "Identifier")
            publisher.publishInteractionClass(interaction)
            publisher.changeInteractionOrderType(interaction, OrderType.TIMESTAMP)
            receiver.subscribeInteractionClass(interaction)
            receiver.enableTimeConstrained()
            receiver.evokeCallback(0.0)
            publisher.enableTimeRegulation(
                publisher.getTimeFactory().makeLogicalTimeInterval(5)
            )
            publisher.evokeCallback(0.0)

            timestamp = publisher.getTimeFactory().makeLogicalTime(9)
            retraction = publisher.sendInteractionWithTime(
                interaction,
                ParameterHandleValueMap({identifier: b"payload"}),
                timestamp,
                b"retract-me",
            )
            self.assertTrue(retraction.isValid())
            receiver.flushQueueRequest(timestamp)
            self.assertFalse(publisher.evokeCallback(0.0))
            self.assertFalse(receiver.evokeCallback(0.0))
            self.assertEqual(len(receiver_callback.flush_grants), 1)
            self.assertEqual(receiver_callback.flush_grants[0][0].getTime(), 5)
            self.assertEqual(receiver_callback.flush_grants[0][1].getTime(), 9)
            self.assertEqual(len(receiver_callback.interactions), 1)
            self.assertEqual(receiver_callback.interactions[0][6].getTime(), 9)

            publisher.retract(retraction)
            self.assertFalse(receiver.evokeCallback(0.0))
            self.assertEqual(len(receiver_callback.request_retractions), 1)
            self.assertEqual(receiver_callback.request_retractions[0].encodedValue, retraction.encodedValue)
        finally:
            if receiver_joined:
                receiver.resignFederationExecution(ResignAction.NO_ACTION)
            if publisher_joined:
                publisher.resignFederationExecution(ResignAction.NO_ACTION)
            if created:
                publisher.destroyFederationExecution(federation_name)
            receiver.disconnect()
            publisher.disconnect()

    def test_native_restore_reinstates_live_timestamped_retraction_record(self) -> None:
        class RecordingFederateAmbassador(FederateAmbassador):
            def __init__(self) -> None:
                self.interactions: list[tuple[object, ...]] = []
                self.save_initiations: list[str] = []
                self.save_completions = 0
                self.restore_acceptances: list[str] = []
                self.restore_begun = 0
                self.restore_initiations: list[tuple[object, ...]] = []
                self.restore_completions = 0
                self.request_retractions: list[MessageRetractionHandle] = []

            def receiveInteraction(self, *arguments) -> None:
                self.interactions.append(arguments)

            def initiateFederateSave(self, label: str) -> None:
                self.save_initiations.append(label)

            def federationSaved(self) -> None:
                self.save_completions += 1

            def requestFederationRestoreSucceeded(self, label: str) -> None:
                self.restore_acceptances.append(label)

            def federationRestoreBegun(self) -> None:
                self.restore_begun += 1

            def initiateFederateRestore(self, *arguments) -> None:
                self.restore_initiations.append(arguments)

            def federationRestored(self) -> None:
                self.restore_completions += 1

            def requestRetraction(self, retraction) -> None:
                self.request_retractions.append(retraction)

        fom_module = (
            Path(__file__).parents[3]
            / "cpp"
            / "tests"
            / "data"
            / "parameter-handle-provider-fom.xml"
        )
        federation_name = f"python-native-restore-live-retraction-{uuid4()}"
        publisher_callback = RecordingFederateAmbassador()
        receiver_callback = RecordingFederateAmbassador()
        publisher = UmbraRtiFactory().getRtiAmbassador()
        receiver = UmbraRtiFactory().getRtiAmbassador()
        created = publisher_joined = receiver_joined = False
        try:
            publisher.connect(publisher_callback, CallbackModel.HLA_EVOKED)
            receiver.connect(receiver_callback, CallbackModel.HLA_EVOKED)
            publisher.createFederationExecution(
                federation_name, str(fom_module), "HLAinteger64Time"
            )
            created = True
            publisher.joinFederationExecution("live-retraction-publisher", federation_name)
            publisher_joined = True
            receiver.joinFederationExecution("live-retraction-receiver", federation_name)
            receiver_joined = True

            interaction = publisher.getInteractionClassHandle(
                "HLAinteractionRoot.UmbraParameterFixtureBase.UmbraParameterFixtureChild"
            )
            identifier = publisher.getParameterHandle(interaction, "Identifier")
            publisher.publishInteractionClass(interaction)
            publisher.changeInteractionOrderType(interaction, OrderType.TIMESTAMP)
            receiver.subscribeInteractionClass(interaction)
            receiver.enableTimeConstrained()
            receiver.evokeCallback(0.0)
            publisher.enableTimeRegulation(
                publisher.getTimeFactory().makeLogicalTimeInterval(5)
            )
            publisher.evokeCallback(0.0)

            timestamp = publisher.getTimeFactory().makeLogicalTime(6)
            retraction = publisher.sendInteractionWithTime(
                interaction,
                ParameterHandleValueMap({identifier: b"restore-live-payload"}),
                timestamp,
                b"restore-live-tag",
            )
            self.assertTrue(retraction.isValid())
            self.assertEqual(receiver_callback.interactions, [])
            receiver.timeAdvanceRequest(receiver.getTimeFactory().makeLogicalTime(1))

            save_label = f"restore-live-retraction-{uuid4()}"
            publisher.requestFederationSave(save_label)
            for _ in range(8):
                receiver.evokeMultipleCallbacks(0.0, 0.0)
                publisher.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(publisher_callback.save_initiations, [save_label])
            self.assertEqual(receiver_callback.save_initiations, [save_label])
            publisher.federateSaveBegun()
            receiver.federateSaveBegun()
            publisher.federateSaveComplete()
            receiver.federateSaveComplete()
            for _ in range(4):
                publisher.evokeMultipleCallbacks(0.0, 0.0)
                receiver.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(publisher_callback.save_completions, 1)
            self.assertEqual(receiver_callback.save_completions, 1)

            publisher.retract(retraction)
            self.assertFalse(receiver.evokeCallback(0.0))
            self.assertEqual(receiver_callback.interactions, [])
            self.assertEqual(receiver_callback.request_retractions, [])
            with self.assertRaises(MessageCanNoLongerBeRetracted):
                publisher.retract(retraction)

            publisher.requestFederationRestore(save_label)
            for _ in range(8):
                publisher.evokeMultipleCallbacks(0.0, 0.0)
                receiver.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(publisher_callback.restore_acceptances, [save_label])
            self.assertEqual(receiver_callback.restore_acceptances, [])
            self.assertEqual(publisher_callback.restore_begun, 1)
            self.assertEqual(receiver_callback.restore_begun, 1)
            publisher.federateRestoreComplete()
            receiver.federateRestoreComplete()
            for _ in range(8):
                publisher.evokeMultipleCallbacks(0.0, 0.0)
                receiver.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(publisher_callback.restore_completions, 1)
            self.assertEqual(receiver_callback.restore_completions, 1)

            receiver.flushQueueRequest(receiver.getTimeFactory().makeLogicalTime(6))
            self.assertFalse(receiver.evokeCallback(0.0))
            self.assertEqual(len(receiver_callback.interactions), 1)
            restored = receiver_callback.interactions[0]
            self.assertEqual(restored[1][identifier], b"restore-live-payload")
            self.assertEqual(restored[2], b"restore-live-tag")
            self.assertEqual(restored[6].getTime(), 6)
            self.assertEqual(restored[7], OrderType.TIMESTAMP)
            self.assertTrue(restored[9].isValid())
            self.assertEqual(restored[9].encodedValue, retraction.encodedValue)

            publisher.retract(retraction)
            self.assertFalse(receiver.evokeCallback(0.0))
            self.assertEqual(len(receiver_callback.request_retractions), 1)
            self.assertEqual(
                receiver_callback.request_retractions[0].encodedValue,
                retraction.encodedValue,
            )
            with self.assertRaises(MessageCanNoLongerBeRetracted):
                publisher.retract(retraction)
        finally:
            if receiver_joined:
                receiver.resignFederationExecution(ResignAction.NO_ACTION)
            if publisher_joined:
                publisher.resignFederationExecution(ResignAction.NO_ACTION)
            if created:
                publisher.destroyFederationExecution(federation_name)
            receiver.disconnect()
            publisher.disconnect()

    def test_native_tso_delivery_precedes_timestamped_save_admission(self) -> None:
        class RecordingFederateAmbassador(FederateAmbassador):
            def __init__(self) -> None:
                self.events: list[tuple[str, object]] = []

            def receiveInteraction(self, *arguments) -> None:
                self.events.append(("receive", arguments))

            def initiateFederateSave(self, label: str) -> None:
                self.events.append(("initiate", label))

            def timeAdvanceGrant(self, time) -> None:
                self.events.append(("grant", time.getTime()))

            def federationSaved(self) -> None:
                self.events.append(("saved", None))

        fom_module = (
            Path(__file__).parents[3]
            / "cpp"
            / "tests"
            / "data"
            / "parameter-handle-provider-fom.xml"
        )
        federation_name = f"python-native-tso-save-boundary-{uuid4()}"
        publisher_callback = RecordingFederateAmbassador()
        receiver_callback = RecordingFederateAmbassador()
        publisher = UmbraRtiFactory().getRtiAmbassador()
        receiver = UmbraRtiFactory().getRtiAmbassador()
        created = publisher_joined = receiver_joined = False
        try:
            publisher.connect(publisher_callback, CallbackModel.HLA_EVOKED)
            receiver.connect(receiver_callback, CallbackModel.HLA_EVOKED)
            publisher.createFederationExecution(
                federation_name, str(fom_module), "HLAinteger64Time"
            )
            created = True
            publisher.joinFederationExecution("tso-save-publisher", federation_name)
            publisher_joined = True
            receiver.joinFederationExecution("tso-save-receiver", federation_name)
            receiver_joined = True

            interaction = publisher.getInteractionClassHandle(
                "HLAinteractionRoot.UmbraParameterFixtureBase.UmbraParameterFixtureChild"
            )
            identifier = publisher.getParameterHandle(interaction, "Identifier")
            publisher.publishInteractionClass(interaction)
            publisher.changeInteractionOrderType(interaction, OrderType.TIMESTAMP)
            receiver.subscribeInteractionClass(interaction)
            receiver.enableTimeConstrained()
            receiver.evokeCallback(0.0)
            time_factory = publisher.getTimeFactory()
            publisher.enableTimeRegulation(time_factory.makeLogicalTimeInterval(5))
            publisher.evokeCallback(0.0)

            boundary = time_factory.makeLogicalTime(9)
            message = publisher.sendInteractionWithTime(
                interaction,
                ParameterHandleValueMap({identifier: b"queued-before-save"}),
                boundary,
                b"queued-before-save-tag",
            )
            self.assertTrue(message.isValid())
            publisher.requestFederationSave("python-tso-save", boundary)
            self.assertFalse(
                any(event[0] == "initiate" for event in publisher_callback.events)
            )
            self.assertFalse(
                any(event[0] == "initiate" for event in receiver_callback.events)
            )

            # The requester can advance to the boundary, but the constrained
            # receiver still has an undelivered TSO payload at that time.
            publisher.timeAdvanceRequest(boundary)
            publisher.evokeMultipleCallbacks(0.0, 0.0)
            self.assertFalse(
                any(event[0] == "initiate" for event in receiver_callback.events)
            )

            receiver.timeAdvanceRequest(receiver.getTimeFactory().makeLogicalTime(9))
            for _ in range(8):
                publisher.evokeMultipleCallbacks(0.0, 0.0)
                receiver.evokeMultipleCallbacks(0.0, 0.0)

            receiver_events = receiver_callback.events
            receive_index = next(index for index, event in enumerate(receiver_events) if event[0] == "receive")
            initiate_index = next(index for index, event in enumerate(receiver_events) if event[0] == "initiate")
            grant_index = next(index for index, event in enumerate(receiver_events) if event == ("grant", 9))
            self.assertLess(receive_index, initiate_index)
            self.assertLess(initiate_index, grant_index)
            receive_arguments = receiver_events[receive_index][1]
            self.assertEqual(receive_arguments[1][identifier], b"queued-before-save")
            self.assertEqual(receive_arguments[2], b"queued-before-save-tag")
            self.assertEqual(receive_arguments[6].getTime(), 9)
            self.assertEqual(receive_arguments[7], OrderType.TIMESTAMP)
            self.assertIsInstance(receive_arguments[9], MessageRetractionHandle)
            self.assertIn(("initiate", "python-tso-save"), publisher_callback.events)

            publisher.federateSaveBegun()
            receiver.federateSaveBegun()
            publisher.federateSaveComplete()
            receiver.federateSaveComplete()
            for _ in range(4):
                publisher.evokeMultipleCallbacks(0.0, 0.0)
                receiver.evokeMultipleCallbacks(0.0, 0.0)
            self.assertIn(("saved", None), publisher_callback.events)
            self.assertIn(("saved", None), receiver_callback.events)
        finally:
            if receiver_joined:
                receiver.resignFederationExecution(ResignAction.NO_ACTION)
            if publisher_joined:
                publisher.resignFederationExecution(ResignAction.NO_ACTION)
            if created:
                publisher.destroyFederationExecution(federation_name)
            receiver.disconnect()
            publisher.disconnect()

    def test_native_reentrant_tso_callback_save_waits_for_callback_return(self) -> None:
        class RecordingFederateAmbassador(FederateAmbassador):
            def __init__(self) -> None:
                self.events: list[tuple[str, object]] = []
                self.owner = None
                self.save_time = None
                self.save_accepted = False
                self.save_seen_during_receive = False

            def receiveInteraction(self, *arguments) -> None:
                self.events.append(("receive", arguments))
                self.save_seen_during_receive = any(
                    event[0] == "initiate" for event in self.events
                )
                if self.owner is not None and self.save_time is not None:
                    try:
                        self.owner.requestFederationSave(
                            "python-reentrant-tso-save", self.save_time
                        )
                        self.save_accepted = True
                    except Exception:
                        self.save_accepted = False

            def initiateFederateSave(self, label: str) -> None:
                self.events.append(("initiate", label))

            def timeAdvanceGrant(self, time) -> None:
                self.events.append(("grant", time.getTime()))

            def federationSaved(self) -> None:
                self.events.append(("saved", None))

        fom_module = (
            Path(__file__).parents[3]
            / "cpp"
            / "tests"
            / "data"
            / "parameter-handle-provider-fom.xml"
        )
        federation_name = f"python-native-reentrant-tso-save-{uuid4()}"
        publisher_callback = RecordingFederateAmbassador()
        receiver_callback = RecordingFederateAmbassador()
        publisher = UmbraRtiFactory().getRtiAmbassador()
        receiver = UmbraRtiFactory().getRtiAmbassador()
        created = publisher_joined = receiver_joined = False
        try:
            publisher.connect(publisher_callback, CallbackModel.HLA_EVOKED)
            receiver.connect(receiver_callback, CallbackModel.HLA_EVOKED)
            publisher.createFederationExecution(
                federation_name, str(fom_module), "HLAinteger64Time"
            )
            created = True
            publisher.joinFederationExecution("reentrant-tso-publisher", federation_name)
            publisher_joined = True
            receiver.joinFederationExecution("reentrant-tso-receiver", federation_name)
            receiver_joined = True

            interaction = publisher.getInteractionClassHandle(
                "HLAinteractionRoot.UmbraParameterFixtureBase.UmbraParameterFixtureChild"
            )
            identifier = publisher.getParameterHandle(interaction, "Identifier")
            publisher.publishInteractionClass(interaction)
            publisher.changeInteractionOrderType(interaction, OrderType.TIMESTAMP)
            receiver.subscribeInteractionClass(interaction)
            receiver.enableTimeConstrained()
            receiver.evokeCallback(0.0)
            time_factory = publisher.getTimeFactory()
            publisher.enableTimeRegulation(time_factory.makeLogicalTimeInterval(1))
            publisher.evokeCallback(0.0)

            boundary = time_factory.makeLogicalTime(5)
            receiver_callback.owner = publisher
            receiver_callback.save_time = boundary
            message = publisher.sendInteractionWithTime(
                interaction,
                ParameterHandleValueMap({identifier: b"reentrant-payload"}),
                boundary,
                b"reentrant-tag",
            )
            self.assertTrue(message.isValid())
            receiver.timeAdvanceRequest(receiver.getTimeFactory().makeLogicalTime(5))
            publisher.timeAdvanceRequest(time_factory.makeLogicalTime(4))
            for _ in range(8):
                publisher.evokeMultipleCallbacks(0.0, 0.0)
                receiver.evokeMultipleCallbacks(0.0, 0.0)

            self.assertTrue(receiver_callback.save_accepted)
            self.assertFalse(receiver_callback.save_seen_during_receive)
            receiver_events = receiver_callback.events
            receive_index = next(index for index, event in enumerate(receiver_events) if event[0] == "receive")
            initiate_index = next(index for index, event in enumerate(receiver_events) if event[0] == "initiate")
            grant_index = next(index for index, event in enumerate(receiver_events) if event == ("grant", 5))
            self.assertLess(receive_index, initiate_index)
            self.assertLess(initiate_index, grant_index)
            receive_arguments = receiver_events[receive_index][1]
            self.assertEqual(receive_arguments[1][identifier], b"reentrant-payload")
            self.assertEqual(receive_arguments[2], b"reentrant-tag")
            self.assertEqual(receive_arguments[6].getTime(), 5)
            self.assertEqual(receive_arguments[7], OrderType.TIMESTAMP)
            self.assertTrue(receive_arguments[9].isValid())
            self.assertIn(("initiate", "python-reentrant-tso-save"), publisher_callback.events)

            publisher.federateSaveBegun()
            receiver.federateSaveBegun()
            publisher.federateSaveComplete()
            receiver.federateSaveComplete()
            for _ in range(4):
                publisher.evokeMultipleCallbacks(0.0, 0.0)
                receiver.evokeMultipleCallbacks(0.0, 0.0)
            self.assertIn(("saved", None), publisher_callback.events)
            self.assertIn(("saved", None), receiver_callback.events)
        finally:
            if receiver_joined:
                receiver.resignFederationExecution(ResignAction.NO_ACTION)
            if publisher_joined:
                publisher.resignFederationExecution(ResignAction.NO_ACTION)
            if created:
                publisher.destroyFederationExecution(federation_name)
            receiver.disconnect()
            publisher.disconnect()

    def test_native_region_range_bounds_round_trip_through_cpp(self) -> None:
        fom_module = (
            Path(__file__).parents[3]
            / "third_party"
            / "ieee1516.2-2025"
            / "resources"
            / "examples"
            / "RestaurantFOMmodule-2025.xml"
        )
        federation_name = f"python-native-region-{uuid4()}"
        ambassador = UmbraRtiFactory().getRtiAmbassador()
        ambassador.connect(FederateAmbassador(), CallbackModel.HLA_EVOKED)
        created = joined = False
        try:
            ambassador.createFederationExecution(federation_name, str(fom_module), "HLAinteger64Time")
            created = True
            ambassador.joinFederationExecution("region-owner", federation_name)
            joined = True
            bar_quantity = ambassador.getDimensionHandle("BarQuantity")
            soda_flavor = ambassador.getDimensionHandle("SodaFlavor")
            region = ambassador.createRegion(DimensionHandleSet([bar_quantity, soda_flavor]))
            self.assertIsInstance(region, RegionHandle)
            self.assertEqual(
                ambassador.getRegionHandleFactory().decode(region.encodedValue),
                region,
            )
            dimensions_from_factory = ambassador.getDimensionHandleSetFactory().create()
            dimensions_from_factory.add(bar_quantity)
            dimensions_from_factory.add(soda_flavor)
            self.assertEqual(
                ambassador.getDimensionHandleSet(region),
                DimensionHandleSet(dimensions_from_factory),
            )
            regions_from_factory = ambassador.getRegionHandleSetFactory().create()
            regions_from_factory.add(region)
            self.assertIn(region, regions_from_factory)
            self.assertEqual(
                ambassador.getDimensionHandleSet(region),
                DimensionHandleSet([bar_quantity, soda_flavor]),
            )
            ambassador.setRangeBounds(region, bar_quantity, RangeBounds(0, 10))
            ambassador.setRangeBounds(region, soda_flavor, RangeBounds(1, 3))
            self.assertEqual(
                (ambassador.getRangeBounds(region, bar_quantity).getLowerBound(),
                 ambassador.getRangeBounds(region, bar_quantity).getUpperBound()),
                (0, 10),
            )
            ambassador.commitRegionModifications(RegionHandleSet([region]))
            self.assertEqual(ambassador.getRangeBounds(region, soda_flavor).getUpperBound(), 3)
            ambassador.deleteRegion(region)
            with self.assertRaises(InvalidRegion):
                ambassador.getDimensionHandleSet(region)
        finally:
            if joined:
                ambassador.resignFederationExecution(ResignAction.NO_ACTION)
            if created:
                ambassador.destroyFederationExecution(federation_name)
            ambassador.disconnect()

    def test_native_regional_interaction_sends_region_designators(self) -> None:
        class RecordingFederateAmbassador(FederateAmbassador):
            def __init__(self) -> None:
                self.received: list[tuple[InteractionClassHandle, ParameterHandleValueMap, bytes, RegionHandleSet | None]] = []

            def receiveInteraction(
                self,
                interactionClass,
                parameterValues,
                userSuppliedTag,
                transportationType,
                producingFederate,
                sentRegions=None,
            ) -> None:
                self.received.append(
                    (interactionClass, parameterValues, userSuppliedTag, sentRegions)
                )

        fom_module = (
            Path(__file__).parents[3]
            / "third_party"
            / "ieee1516.2-2025"
            / "resources"
            / "examples"
            / "RestaurantFOMmodule-2025.xml"
        )
        federation_name = f"python-native-regional-interaction-{uuid4()}"
        publisher_callback = RecordingFederateAmbassador()
        subscriber_callback = RecordingFederateAmbassador()
        publisher = UmbraRtiFactory().getRtiAmbassador()
        subscriber = UmbraRtiFactory().getRtiAmbassador()
        publisher.connect(publisher_callback, CallbackModel.HLA_EVOKED)
        subscriber.connect(subscriber_callback, CallbackModel.HLA_EVOKED)
        created = publisher_joined = subscriber_joined = False
        try:
            publisher.createFederationExecution(federation_name, str(fom_module), "HLAinteger64Time")
            created = True
            publisher_federate = publisher.joinFederationExecution("regional-publisher", federation_name)
            publisher_joined = True
            subscriber.joinFederationExecution("regional-subscriber", federation_name)
            subscriber_joined = True
            interaction = publisher.getInteractionClassHandle(
                "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed"
            )
            parameter = publisher.getParameterHandle(interaction, "TemperatureOk")
            dimension = publisher.getDimensionHandle("ServerId")
            publisher.publishInteractionClass(interaction)
            publisher_region = publisher.createRegion(DimensionHandleSet([dimension]))
            publisher.setRangeBounds(publisher_region, dimension, RangeBounds(0, 10))
            publisher.commitRegionModifications(RegionHandleSet([publisher_region]))
            subscriber_region = subscriber.createRegion(DimensionHandleSet([dimension]))
            subscriber.setRangeBounds(subscriber_region, dimension, RangeBounds(5, 15))
            subscriber.commitRegionModifications(RegionHandleSet([subscriber_region]))
            subscriber.subscribeInteractionClassWithRegions(
                interaction, RegionHandleSet([subscriber_region])
            )
            subscriber.setConveyRegionDesignatorSetsSwitch(True)
            for getter, setter in (
                (
                    subscriber.getObjectClassRelevanceAdvisorySwitch,
                    subscriber.setObjectClassRelevanceAdvisorySwitch,
                ),
                (
                    subscriber.getAttributeRelevanceAdvisorySwitch,
                    subscriber.setAttributeRelevanceAdvisorySwitch,
                ),
                (
                    subscriber.getAttributeScopeAdvisorySwitch,
                    subscriber.setAttributeScopeAdvisorySwitch,
                ),
                (
                    subscriber.getInteractionRelevanceAdvisorySwitch,
                    subscriber.setInteractionRelevanceAdvisorySwitch,
                ),
            ):
                initial = getter()
                setter(not initial)
                self.assertEqual(getter(), not initial)
                setter(initial)
                self.assertEqual(getter(), initial)
            initial_resign = subscriber.getAutomaticResignDirective()
            alternate_resign = next(
                action for action in ResignAction if action is not initial_resign
            )
            subscriber.setAutomaticResignDirective(alternate_resign)
            self.assertEqual(subscriber.getAutomaticResignDirective(), alternate_resign)
            subscriber.setAutomaticResignDirective(initial_resign)
            self.assertEqual(subscriber.getAutomaticResignDirective(), initial_resign)
            for getter, setter in (
                (subscriber.getServiceReportingSwitch, subscriber.setServiceReportingSwitch),
                (subscriber.getExceptionReportingSwitch, subscriber.setExceptionReportingSwitch),
            ):
                initial = getter()
                setter(not initial)
                self.assertEqual(getter(), not initial)
                setter(initial)
                self.assertEqual(getter(), initial)
            for getter in (
                subscriber.getSendServiceReportsToFileSwitch,
                subscriber.getAutoProvideSwitch,
                subscriber.getDelaySubscriptionEvaluationSwitch,
                subscriber.getAdvisoriesUseKnownClassSwitch,
                subscriber.getAllowRelaxedDDMSwitch,
                subscriber.getNonRegulatedGrantSwitch,
            ):
                self.assertIsInstance(getter(), bool)
            publisher.sendInteractionWithRegions(
                interaction,
                ParameterHandleValueMap({parameter: b"regional"}),
                RegionHandleSet([publisher_region]),
                b"regional-tag",
            )
            subscriber.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(len(subscriber_callback.received), 1)
            self.assertEqual(subscriber_callback.received[0][0], interaction)
            self.assertEqual(subscriber_callback.received[0][1][parameter], b"regional")
            self.assertEqual(subscriber_callback.received[0][2], b"regional-tag")
            self.assertEqual(
                subscriber_callback.received[0][3], RegionHandleSet([publisher_region])
            )
        finally:
            if subscriber_joined:
                subscriber.resignFederationExecution(ResignAction.NO_ACTION)
            if publisher_joined:
                publisher.resignFederationExecution(ResignAction.NO_ACTION)
            if created:
                publisher.destroyFederationExecution(federation_name)
            publisher.disconnect()
            subscriber.disconnect()

    def test_native_regional_interaction_subscription_reprojects(self) -> None:
        class RecordingFederateAmbassador(FederateAmbassador):
            def __init__(self) -> None:
                self.received: list[tuple[object, ...]] = []

            def receiveInteraction(self, *arguments) -> None:
                self.received.append(arguments)

        fom_module = (
            Path(__file__).parents[3]
            / "third_party"
            / "ieee1516.2-2025"
            / "resources"
            / "examples"
            / "RestaurantFOMmodule-2025.xml"
        )
        federation_name = f"python-native-regional-interaction-reprojection-{uuid4()}"
        publisher_callback = RecordingFederateAmbassador()
        subscriber_callback = RecordingFederateAmbassador()
        publisher = UmbraRtiFactory().getRtiAmbassador()
        subscriber = UmbraRtiFactory().getRtiAmbassador()
        created = publisher_joined = subscriber_joined = False
        try:
            publisher.connect(publisher_callback, CallbackModel.HLA_EVOKED)
            subscriber.connect(subscriber_callback, CallbackModel.HLA_EVOKED)
            publisher.createFederationExecution(
                federation_name, str(fom_module), "HLAinteger64Time"
            )
            created = True
            publisher.joinFederationExecution("interaction-reprojection-publisher", federation_name)
            publisher_joined = True
            subscriber.joinFederationExecution("interaction-reprojection-subscriber", federation_name)
            subscriber_joined = True

            interaction = publisher.getInteractionClassHandle(
                "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed"
            )
            parameter = publisher.getParameterHandle(interaction, "TemperatureOk")
            publisher.publishInteractionClass(interaction)
            dimension = publisher.getDimensionHandle("ServerId")
            publisher_region = publisher.createRegion(DimensionHandleSet([dimension]))
            disjoint_region = subscriber.createRegion(DimensionHandleSet([dimension]))
            overlap_region = subscriber.createRegion(DimensionHandleSet([dimension]))
            publisher.setRangeBounds(publisher_region, dimension, RangeBounds(0, 5))
            subscriber.setRangeBounds(disjoint_region, dimension, RangeBounds(10, 15))
            subscriber.setRangeBounds(overlap_region, dimension, RangeBounds(2, 7))
            publisher.commitRegionModifications(RegionHandleSet([publisher_region]))
            subscriber.commitRegionModifications(
                RegionHandleSet([disjoint_region, overlap_region])
            )

            subscriber.subscribeInteractionClassWithRegions(
                interaction, RegionHandleSet([disjoint_region])
            )
            publisher.sendInteractionWithRegions(
                interaction,
                ParameterHandleValueMap({parameter: b"outside"}),
                RegionHandleSet([publisher_region]),
                b"outside-tag",
            )
            subscriber.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(subscriber_callback.received, [])

            subscriber.subscribeInteractionClassWithRegions(
                interaction, RegionHandleSet([overlap_region])
            )
            publisher.sendInteractionWithRegions(
                interaction,
                ParameterHandleValueMap({parameter: b"inside"}),
                RegionHandleSet([publisher_region]),
                b"inside-tag",
            )
            subscriber.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(len(subscriber_callback.received), 1)
            self.assertEqual(subscriber_callback.received[0][1][parameter], b"inside")

            subscriber.unsubscribeInteractionClassWithRegions(
                interaction, RegionHandleSet([overlap_region])
            )
            publisher.sendInteractionWithRegions(
                interaction,
                ParameterHandleValueMap({parameter: b"outside-again"}),
                RegionHandleSet([publisher_region]),
                b"outside-again-tag",
            )
            subscriber.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(len(subscriber_callback.received), 1)
        finally:
            if subscriber_joined:
                subscriber.resignFederationExecution(ResignAction.NO_ACTION)
            if publisher_joined:
                publisher.resignFederationExecution(ResignAction.NO_ACTION)
            if created:
                publisher.destroyFederationExecution(federation_name)
            subscriber.disconnect()
            publisher.disconnect()

    def test_native_relaxed_ddm_delivers_boundary_touching_regional_interaction(self) -> None:
        class RecordingFederateAmbassador(FederateAmbassador):
            def __init__(self) -> None:
                self.received: list[tuple[object, ...]] = []

            def receiveInteraction(self, *arguments) -> None:
                self.received.append(arguments)

        restaurant_fom = (
            Path(__file__).parents[3]
            / "third_party"
            / "ieee1516.2-2025"
            / "resources"
            / "examples"
            / "RestaurantFOMmodule-2025.xml"
        )
        switch_fom = (
            Path(__file__).parents[3]
            / "cpp"
            / "tests"
            / "data"
            / "switch-support-enabled-fom.xml"
        )
        federation_name = f"python-native-relaxed-ddm-{uuid4()}"
        publisher_callback = RecordingFederateAmbassador()
        subscriber_callback = RecordingFederateAmbassador()
        publisher = UmbraRtiFactory().getRtiAmbassador()
        subscriber = UmbraRtiFactory().getRtiAmbassador()
        created = publisher_joined = subscriber_joined = False
        try:
            publisher.connect(publisher_callback, CallbackModel.HLA_EVOKED)
            subscriber.connect(subscriber_callback, CallbackModel.HLA_EVOKED)
            publisher.createFederationExecution(
                federation_name,
                [str(restaurant_fom), str(switch_fom)],
                "HLAinteger64Time",
            )
            created = True
            publisher.joinFederationExecution("relaxed-ddm-publisher", federation_name)
            publisher_joined = True
            subscriber.joinFederationExecution("relaxed-ddm-subscriber", federation_name)
            subscriber_joined = True
            interaction = publisher.getInteractionClassHandle(
                "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed"
            )
            parameter = publisher.getParameterHandle(interaction, "TemperatureOk")
            dimension = publisher.getDimensionHandle("ServerId")
            publisher.publishInteractionClass(interaction)
            publisher_region = publisher.createRegion(DimensionHandleSet([dimension]))
            subscriber_region = subscriber.createRegion(DimensionHandleSet([dimension]))
            publisher.setRangeBounds(publisher_region, dimension, RangeBounds(0, 2))
            subscriber.setRangeBounds(subscriber_region, dimension, RangeBounds(2, 4))
            publisher.commitRegionModifications(RegionHandleSet([publisher_region]))
            subscriber.commitRegionModifications(RegionHandleSet([subscriber_region]))
            subscriber.subscribeInteractionClassWithRegions(
                interaction, RegionHandleSet([subscriber_region])
            )
            self.assertTrue(subscriber.getAllowRelaxedDDMSwitch())
            publisher.sendInteractionWithRegions(
                interaction,
                ParameterHandleValueMap({parameter: b"relaxed-boundary"}),
                RegionHandleSet([publisher_region]),
                b"relaxed-boundary-tag",
            )
            for _ in range(6):
                publisher.evokeMultipleCallbacks(0.0, 0.0)
                subscriber.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(len(subscriber_callback.received), 1)
            self.assertEqual(
                subscriber_callback.received[-1][1][parameter], b"relaxed-boundary"
            )
        finally:
            if subscriber_joined:
                subscriber.resignFederationExecution(ResignAction.NO_ACTION)
            if publisher_joined:
                publisher.resignFederationExecution(ResignAction.NO_ACTION)
            if created:
                publisher.destroyFederationExecution(federation_name)
            subscriber.disconnect()
            publisher.disconnect()

    def test_native_relaxed_ddm_delivers_boundary_touching_regional_object_update(self) -> None:
        class RecordingFederateAmbassador(FederateAmbassador):
            def __init__(self) -> None:
                self.discoveries: list[tuple[object, ...]] = []
                self.reflections: list[tuple[object, ...]] = []

            def discoverObjectInstance(self, *arguments) -> None:
                self.discoveries.append(arguments)

            def reflectAttributeValues(self, *arguments) -> None:
                self.reflections.append(arguments)

        restaurant_fom = (
            Path(__file__).parents[3]
            / "third_party"
            / "ieee1516.2-2025"
            / "resources"
            / "examples"
            / "RestaurantFOMmodule-2025.xml"
        )
        switch_fom = (
            Path(__file__).parents[3]
            / "cpp"
            / "tests"
            / "data"
            / "switch-support-enabled-fom.xml"
        )
        federation_name = f"python-native-relaxed-ddm-object-{uuid4()}"
        publisher_callback = RecordingFederateAmbassador()
        subscriber_callback = RecordingFederateAmbassador()
        publisher = UmbraRtiFactory().getRtiAmbassador()
        subscriber = UmbraRtiFactory().getRtiAmbassador()
        created = publisher_joined = subscriber_joined = False
        try:
            publisher.connect(publisher_callback, CallbackModel.HLA_EVOKED)
            subscriber.connect(subscriber_callback, CallbackModel.HLA_EVOKED)
            publisher.createFederationExecution(
                federation_name,
                [str(restaurant_fom), str(switch_fom)],
                "HLAinteger64Time",
            )
            created = True
            publisher.joinFederationExecution("relaxed-ddm-object-publisher", federation_name)
            publisher_joined = True
            subscriber.joinFederationExecution("relaxed-ddm-object-subscriber", federation_name)
            subscriber_joined = True
            publisher_class = publisher.getObjectClassHandle("HLAobjectRoot.Food.Drink.Soda")
            publisher_attribute = publisher.getAttributeHandle(publisher_class, "Flavor")
            subscriber_class = subscriber.getObjectClassHandle("HLAobjectRoot.Food.Drink.Soda")
            subscriber_attribute = subscriber.getAttributeHandle(subscriber_class, "Flavor")
            dimension = publisher.getDimensionHandle("SodaFlavor")
            publisher.publishObjectClassAttributes(
                publisher_class, AttributeHandleSet([publisher_attribute])
            )
            publisher_region = publisher.createRegion(DimensionHandleSet([dimension]))
            subscriber_region = subscriber.createRegion(DimensionHandleSet([dimension]))
            publisher.setRangeBounds(publisher_region, dimension, RangeBounds(0, 2))
            subscriber.setRangeBounds(subscriber_region, dimension, RangeBounds(2, 4))
            publisher.commitRegionModifications(RegionHandleSet([publisher_region]))
            subscriber.commitRegionModifications(RegionHandleSet([subscriber_region]))
            subscriber_pairs = AttributeSetRegionSetPairList([
                AttributeSetRegionSetPair(
                    AttributeHandleSet([subscriber_attribute]), RegionHandleSet([subscriber_region])
                )
            ])
            publisher_pairs = AttributeSetRegionSetPairList([
                AttributeSetRegionSetPair(
                    AttributeHandleSet([publisher_attribute]), RegionHandleSet([publisher_region])
                )
            ])
            subscriber.subscribeObjectClassAttributesWithRegions(subscriber_class, subscriber_pairs)
            self.assertTrue(subscriber.getAllowRelaxedDDMSwitch())
            object_instance = publisher.registerObjectInstanceWithRegions(
                publisher_class, publisher_pairs
            )
            for _ in range(6):
                publisher.evokeMultipleCallbacks(0.0, 0.0)
                subscriber.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(len(subscriber_callback.discoveries), 1)
            self.assertEqual(subscriber_callback.discoveries[-1][0], object_instance)
            publisher.updateAttributeValues(
                object_instance,
                AttributeHandleValueMap({publisher_attribute: b"relaxed-object-boundary"}),
                b"relaxed-object-boundary-tag",
            )
            for _ in range(6):
                publisher.evokeMultipleCallbacks(0.0, 0.0)
                subscriber.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(len(subscriber_callback.reflections), 1)
            self.assertEqual(
                subscriber_callback.reflections[-1][1][subscriber_attribute],
                b"relaxed-object-boundary",
            )
        finally:
            if subscriber_joined:
                subscriber.resignFederationExecution(ResignAction.NO_ACTION)
            if publisher_joined:
                publisher.resignFederationExecution(ResignAction.NO_ACTION)
            if created:
                publisher.destroyFederationExecution(federation_name)
            subscriber.disconnect()
            publisher.disconnect()

    def test_native_default_region_object_callbacks_convey_empty_region_set(self) -> None:
        class RecordingFederateAmbassador(FederateAmbassador):
            def __init__(self) -> None:
                self.discoveries: list[tuple[object, ...]] = []
                self.reflections: list[tuple[object, ...]] = []
                self.regulations: list[object] = []

            def discoverObjectInstance(self, *arguments) -> None:
                self.discoveries.append(arguments)

            def reflectAttributeValues(self, *arguments) -> None:
                self.reflections.append(arguments)

            def timeRegulationEnabled(self, time) -> None:
                self.regulations.append(time)

        fom_module = (
            Path(__file__).parents[3]
            / "third_party"
            / "ieee1516.2-2025"
            / "resources"
            / "examples"
            / "RestaurantFOMmodule-2025.xml"
        )
        federation_name = f"python-native-default-region-object-{uuid4()}"
        publisher_callback = RecordingFederateAmbassador()
        subscriber_callback = RecordingFederateAmbassador()
        publisher = UmbraRtiFactory().getRtiAmbassador()
        subscriber = UmbraRtiFactory().getRtiAmbassador()
        created = publisher_joined = subscriber_joined = False
        try:
            publisher.connect(publisher_callback, CallbackModel.HLA_EVOKED)
            subscriber.connect(subscriber_callback, CallbackModel.HLA_EVOKED)
            publisher.createFederationExecution(
                federation_name, str(fom_module), "HLAinteger64Time"
            )
            created = True
            publisher.joinFederationExecution("default-region-publisher", federation_name)
            publisher_joined = True
            subscriber.joinFederationExecution("default-region-subscriber", federation_name)
            subscriber_joined = True

            publisher_class = publisher.getObjectClassHandle("HLAobjectRoot.Food.Drink.Soda")
            publisher_attribute = publisher.getAttributeHandle(publisher_class, "Flavor")
            subscriber_class = subscriber.getObjectClassHandle("HLAobjectRoot.Food.Drink.Soda")
            subscriber_attribute = subscriber.getAttributeHandle(subscriber_class, "Flavor")
            dimension = publisher.getDimensionHandle("SodaFlavor")
            publisher.publishObjectClassAttributes(
                publisher_class, AttributeHandleSet([publisher_attribute])
            )
            subscriber_region = subscriber.createRegion(DimensionHandleSet([dimension]))
            subscriber.setRangeBounds(subscriber_region, dimension, RangeBounds(1, 3))
            subscriber.commitRegionModifications(RegionHandleSet([subscriber_region]))
            subscriber_pairs = AttributeSetRegionSetPairList([
                AttributeSetRegionSetPair(
                    AttributeHandleSet([subscriber_attribute]), RegionHandleSet([subscriber_region])
                )
            ])
            subscriber.subscribeObjectClassAttributesWithRegions(subscriber_class, subscriber_pairs)
            subscriber.setConveyRegionDesignatorSetsSwitch(True)
            object_instance = publisher.registerObjectInstance(publisher_class)
            for _ in range(6):
                publisher.evokeMultipleCallbacks(0.0, 0.0)
                subscriber.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(len(subscriber_callback.discoveries), 1)
            self.assertEqual(subscriber_callback.discoveries[-1][0], object_instance)

            publisher.updateAttributeValues(
                object_instance,
                AttributeHandleValueMap({publisher_attribute: b"default-receive"}),
                b"default-receive-tag",
            )
            for _ in range(6):
                publisher.evokeMultipleCallbacks(0.0, 0.0)
                subscriber.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(len(subscriber_callback.reflections), 1)
            receive_reflection = subscriber_callback.reflections[-1]
            self.assertEqual(receive_reflection[1][subscriber_attribute], b"default-receive")
            self.assertEqual(receive_reflection[5], RegionHandleSet())

            time_factory = publisher.getTimeFactory()
            publisher.enableTimeRegulation(time_factory.makeLogicalTimeInterval(1))
            for _ in range(6):
                publisher.evokeMultipleCallbacks(0.0, 0.0)
                if publisher_callback.regulations:
                    break
            self.assertEqual(len(publisher_callback.regulations), 1)
            retraction = publisher.updateAttributeValuesWithTime(
                object_instance,
                AttributeHandleValueMap({publisher_attribute: b"default-timestamped"}),
                time_factory.makeLogicalTime(5),
                b"default-timestamped-tag",
            )
            self.assertTrue(retraction.isValid())
            for _ in range(6):
                publisher.evokeMultipleCallbacks(0.0, 0.0)
                subscriber.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(len(subscriber_callback.reflections), 2)
            timed_reflection = subscriber_callback.reflections[-1]
            self.assertEqual(timed_reflection[1][subscriber_attribute], b"default-timestamped")
            self.assertEqual(timed_reflection[5], RegionHandleSet())
            self.assertEqual(timed_reflection[6].getTime(), 5)
        finally:
            if subscriber_joined:
                subscriber.resignFederationExecution(ResignAction.NO_ACTION)
            if publisher_joined:
                publisher.resignFederationExecution(ResignAction.NO_ACTION)
            if created:
                publisher.destroyFederationExecution(federation_name)
            subscriber.disconnect()
            publisher.disconnect()

    def test_native_regional_delayed_subscription_rechecks_at_callback_boundary(self) -> None:
        class RecordingFederateAmbassador(FederateAmbassador):
            def __init__(self) -> None:
                self.received: list[tuple[object, ...]] = []

            def receiveInteraction(
                self,
                interactionClass,
                parameterValues,
                userSuppliedTag,
                transportationType,
                producingFederate,
                sentRegions=None,
                *timed,
            ) -> None:
                self.received.append(
                    (
                        interactionClass,
                        parameterValues,
                        userSuppliedTag,
                        transportationType,
                        producingFederate,
                        sentRegions,
                        *timed,
                    )
                )

        restaurant_fom = (
            Path(__file__).parents[3]
            / "third_party"
            / "ieee1516.2-2025"
            / "resources"
            / "examples"
            / "RestaurantFOMmodule-2025.xml"
        )
        switch_fom = (
            Path(__file__).parents[3]
            / "cpp"
            / "tests"
            / "data"
            / "switch-support-enabled-fom.xml"
        )
        federation_name = f"python-native-regional-delayed-{uuid4()}"
        publisher_callback = RecordingFederateAmbassador()
        subscriber_callback = RecordingFederateAmbassador()
        publisher = UmbraRtiFactory().getRtiAmbassador()
        subscriber = UmbraRtiFactory().getRtiAmbassador()
        created = publisher_joined = subscriber_joined = False
        try:
            publisher.connect(publisher_callback, CallbackModel.HLA_EVOKED)
            subscriber.connect(subscriber_callback, CallbackModel.HLA_EVOKED)
            publisher.createFederationExecution(
                federation_name,
                [str(restaurant_fom), str(switch_fom)],
                "HLAinteger64Time",
            )
            created = True
            publisher.joinFederationExecution("regional-delayed-publisher", federation_name)
            publisher_joined = True
            subscriber.joinFederationExecution("regional-delayed-subscriber", federation_name)
            subscriber_joined = True

            interaction = publisher.getInteractionClassHandle(
                "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed"
            )
            parameter = publisher.getParameterHandle(interaction, "TemperatureOk")
            publisher_dimension = publisher.getDimensionHandle("ServerId")
            subscriber_dimension = subscriber.getDimensionHandle("ServerId")
            publisher.publishInteractionClass(interaction)
            publisher_region = publisher.createRegion(
                DimensionHandleSet([publisher_dimension])
            )
            subscriber_region = subscriber.createRegion(
                DimensionHandleSet([subscriber_dimension])
            )
            publisher.setRangeBounds(publisher_region, publisher_dimension, RangeBounds(0, 2))
            subscriber.setRangeBounds(
                subscriber_region, subscriber_dimension, RangeBounds(3, 4)
            )
            publisher.commitRegionModifications(RegionHandleSet([publisher_region]))
            subscriber.commitRegionModifications(RegionHandleSet([subscriber_region]))
            subscriber.subscribeInteractionClassWithRegions(
                interaction, RegionHandleSet([subscriber_region])
            )
            self.assertTrue(subscriber.getDelaySubscriptionEvaluationSwitch())

            publisher.sendInteractionWithRegions(
                interaction,
                ParameterHandleValueMap({parameter: b"delayed-regional"}),
                RegionHandleSet([publisher_region]),
                b"delayed-regional-tag",
            )
            self.assertEqual(subscriber_callback.received, [])

            subscriber.setRangeBounds(
                subscriber_region, subscriber_dimension, RangeBounds(1, 3)
            )
            subscriber.commitRegionModifications(RegionHandleSet([subscriber_region]))
            subscriber.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(len(subscriber_callback.received), 1)
            self.assertEqual(
                subscriber_callback.received[-1][1][parameter], b"delayed-regional"
            )
            self.assertEqual(subscriber_callback.received[-1][2], b"delayed-regional-tag")
        finally:
            if subscriber_joined:
                subscriber.resignFederationExecution(ResignAction.NO_ACTION)
            if publisher_joined:
                publisher.resignFederationExecution(ResignAction.NO_ACTION)
            if created:
                publisher.destroyFederationExecution(federation_name)
            subscriber.disconnect()
            publisher.disconnect()

    def test_native_regional_delayed_object_update_rechecks_at_callback_boundary(self) -> None:
        class RecordingFederateAmbassador(FederateAmbassador):
            def __init__(self) -> None:
                self.discoveries: list[tuple[object, ...]] = []
                self.reflections: list[tuple[object, ...]] = []

            def discoverObjectInstance(self, *arguments) -> None:
                self.discoveries.append(arguments)

            def reflectAttributeValues(self, *arguments) -> None:
                self.reflections.append(arguments)

        restaurant_fom = (
            Path(__file__).parents[3]
            / "third_party"
            / "ieee1516.2-2025"
            / "resources"
            / "examples"
            / "RestaurantFOMmodule-2025.xml"
        )
        switch_fom = (
            Path(__file__).parents[3]
            / "cpp"
            / "tests"
            / "data"
            / "switch-support-enabled-fom.xml"
        )
        federation_name = f"python-native-regional-delayed-object-{uuid4()}"
        publisher_callback = RecordingFederateAmbassador()
        subscriber_callback = RecordingFederateAmbassador()
        publisher = UmbraRtiFactory().getRtiAmbassador()
        subscriber = UmbraRtiFactory().getRtiAmbassador()
        created = publisher_joined = subscriber_joined = False
        try:
            publisher.connect(publisher_callback, CallbackModel.HLA_EVOKED)
            subscriber.connect(subscriber_callback, CallbackModel.HLA_EVOKED)
            publisher.createFederationExecution(
                federation_name,
                [str(restaurant_fom), str(switch_fom)],
                "HLAinteger64Time",
            )
            created = True
            publisher.joinFederationExecution("regional-delayed-object-publisher", federation_name)
            publisher_joined = True
            subscriber.joinFederationExecution("regional-delayed-object-subscriber", federation_name)
            subscriber_joined = True

            publisher_class = publisher.getObjectClassHandle(
                "HLAobjectRoot.Food.Drink.Soda"
            )
            subscriber_class = subscriber.getObjectClassHandle(
                "HLAobjectRoot.Food.Drink.Soda"
            )
            publisher_attribute = publisher.getAttributeHandle(publisher_class, "Flavor")
            subscriber_attribute = subscriber.getAttributeHandle(subscriber_class, "Flavor")
            publisher_dimension = publisher.getDimensionHandle("SodaFlavor")
            subscriber_dimension = subscriber.getDimensionHandle("SodaFlavor")
            attributes = AttributeHandleSet([publisher_attribute])
            subscriber_attributes = AttributeHandleSet([subscriber_attribute])
            publisher.publishObjectClassAttributes(publisher_class, attributes)

            publisher_region = publisher.createRegion(
                DimensionHandleSet([publisher_dimension])
            )
            subscriber_region = subscriber.createRegion(
                DimensionHandleSet([subscriber_dimension])
            )
            publisher.setRangeBounds(publisher_region, publisher_dimension, RangeBounds(0, 2))
            subscriber.setRangeBounds(
                subscriber_region, subscriber_dimension, RangeBounds(1, 3)
            )
            publisher.commitRegionModifications(RegionHandleSet([publisher_region]))
            subscriber.commitRegionModifications(RegionHandleSet([subscriber_region]))
            subscriber_pairs = AttributeSetRegionSetPairList(
                [
                    AttributeSetRegionSetPair(
                        subscriber_attributes, RegionHandleSet([subscriber_region])
                    )
                ]
            )
            publisher_pairs = AttributeSetRegionSetPairList(
                [
                    AttributeSetRegionSetPair(
                        attributes, RegionHandleSet([publisher_region])
                    )
                ]
            )
            subscriber.subscribeObjectClassAttributesWithRegions(
                subscriber_class, subscriber_pairs
            )
            object_instance = publisher.registerObjectInstanceWithRegions(
                publisher_class, publisher_pairs
            )
            subscriber.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(len(subscriber_callback.discoveries), 1)
            self.assertTrue(subscriber.getDelaySubscriptionEvaluationSwitch())

            subscriber.setRangeBounds(
                subscriber_region, subscriber_dimension, RangeBounds(3, 4)
            )
            subscriber.commitRegionModifications(RegionHandleSet([subscriber_region]))
            publisher.updateAttributeValues(
                object_instance,
                AttributeHandleValueMap({publisher_attribute: b"delayed-object"}),
                b"delayed-object-tag",
            )
            self.assertEqual(subscriber_callback.reflections, [])

            subscriber.setRangeBounds(
                subscriber_region, subscriber_dimension, RangeBounds(1, 3)
            )
            subscriber.commitRegionModifications(RegionHandleSet([subscriber_region]))
            subscriber.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(len(subscriber_callback.reflections), 1)
            self.assertEqual(
                subscriber_callback.reflections[-1][1][subscriber_attribute],
                b"delayed-object",
            )
            self.assertEqual(subscriber_callback.reflections[-1][2], b"delayed-object-tag")
        finally:
            if subscriber_joined:
                subscriber.resignFederationExecution(ResignAction.NO_ACTION)
            if publisher_joined:
                publisher.resignFederationExecution(ResignAction.NO_ACTION)
            if created:
                publisher.destroyFederationExecution(federation_name)
            subscriber.disconnect()
            publisher.disconnect()

    def test_native_timestamped_regional_interaction_callback_preserves_regions(self) -> None:
        class RecordingFederateAmbassador(FederateAmbassador):
            def __init__(self) -> None:
                self.received: list[tuple[object, ...]] = []

            def receiveInteraction(
                self,
                interactionClass,
                parameterValues,
                userSuppliedTag,
                transportationType,
                producingFederate,
                sentRegions=None,
                *timed,
            ) -> None:
                self.received.append(
                    (
                        interactionClass,
                        parameterValues,
                        userSuppliedTag,
                        transportationType,
                        producingFederate,
                        sentRegions,
                        *timed,
                    )
                )

        fom_module = (
            Path(__file__).parents[3]
            / "third_party"
            / "ieee1516.2-2025"
            / "resources"
            / "examples"
            / "RestaurantFOMmodule-2025.xml"
        )
        federation_name = f"python-native-timed-regional-interaction-{uuid4()}"
        publisher_callback = RecordingFederateAmbassador()
        subscriber_callback = RecordingFederateAmbassador()
        publisher = UmbraRtiFactory().getRtiAmbassador()
        subscriber = UmbraRtiFactory().getRtiAmbassador()
        created = publisher_joined = subscriber_joined = False
        try:
            publisher.connect(publisher_callback, CallbackModel.HLA_EVOKED)
            subscriber.connect(subscriber_callback, CallbackModel.HLA_EVOKED)
            publisher.createFederationExecution(
                federation_name, str(fom_module), "HLAinteger64Time"
            )
            created = True
            publisher.joinFederationExecution("timed-regional-publisher", federation_name)
            publisher_joined = True
            subscriber.joinFederationExecution("timed-regional-subscriber", federation_name)
            subscriber_joined = True

            interaction = publisher.getInteractionClassHandle(
                "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed"
            )
            parameter = publisher.getParameterHandle(interaction, "TemperatureOk")
            dimension = publisher.getDimensionHandle("ServerId")
            publisher.publishInteractionClass(interaction)
            publisher.changeInteractionOrderType(interaction, OrderType.TIMESTAMP)
            publisher_region = publisher.createRegion(DimensionHandleSet([dimension]))
            publisher.setRangeBounds(publisher_region, dimension, RangeBounds(0, 10))
            publisher.commitRegionModifications(RegionHandleSet([publisher_region]))
            subscriber_region = subscriber.createRegion(DimensionHandleSet([dimension]))
            subscriber.setRangeBounds(subscriber_region, dimension, RangeBounds(5, 15))
            subscriber.commitRegionModifications(RegionHandleSet([subscriber_region]))
            subscriber.subscribeInteractionClassWithRegions(
                interaction, RegionHandleSet([subscriber_region])
            )
            subscriber.setConveyRegionDesignatorSetsSwitch(True)

            time_factory = publisher.getTimeFactory()
            timestamp = time_factory.makeLogicalTime(5)
            subscriber.enableTimeConstrained()
            subscriber.evokeCallback(0.0)
            publisher.enableTimeRegulation(time_factory.makeLogicalTimeInterval(1))
            publisher.evokeCallback(0.0)
            retraction = publisher.sendInteractionWithRegionsWithTime(
                interaction,
                ParameterHandleValueMap({parameter: b"timed-regional"}),
                RegionHandleSet([publisher_region]),
                timestamp,
                b"timed-regional-tag",
            )
            self.assertTrue(retraction.isValid())
            publisher.timeAdvanceRequest(timestamp)
            subscriber.timeAdvanceRequest(subscriber.getTimeFactory().makeLogicalTime(5))
            for _ in range(8):
                publisher.evokeMultipleCallbacks(0.0, 0.0)
                subscriber.evokeMultipleCallbacks(0.0, 0.0)

            self.assertEqual(len(subscriber_callback.received), 1)
            received = subscriber_callback.received[0]
            self.assertEqual(received[0], interaction)
            self.assertEqual(received[1][parameter], b"timed-regional")
            self.assertEqual(received[2], b"timed-regional-tag")
            self.assertEqual(received[5], RegionHandleSet([publisher_region]))
            self.assertEqual(received[6].getTime(), 5)
            self.assertEqual(received[7], OrderType.TIMESTAMP)
            self.assertEqual(received[8], OrderType.TIMESTAMP)
            self.assertIsInstance(received[9], MessageRetractionHandle)
        finally:
            if subscriber_joined:
                subscriber.resignFederationExecution(ResignAction.NO_ACTION)
            if publisher_joined:
                publisher.resignFederationExecution(ResignAction.NO_ACTION)
            if created:
                publisher.destroyFederationExecution(federation_name)
            publisher.disconnect()
            subscriber.disconnect()

    def test_native_timestamped_regional_attribute_callback_preserves_regions(self) -> None:
        class RecordingFederateAmbassador(FederateAmbassador):
            def __init__(self) -> None:
                self.reflections: list[tuple[object, ...]] = []
                self.regulations: list[object] = []
                self.constraints: list[object] = []

            def timeRegulationEnabled(self, time) -> None:
                self.regulations.append(time)

            def timeConstrainedEnabled(self, time) -> None:
                self.constraints.append(time)

            def reflectAttributeValues(self, *arguments) -> None:
                self.reflections.append(arguments)

        fom_module = (
            Path(__file__).parents[3]
            / "third_party"
            / "ieee1516.2-2025"
            / "resources"
            / "examples"
            / "RestaurantFOMmodule-2025.xml"
        )
        federation_name = f"python-native-timed-regional-attribute-{uuid4()}"
        publisher_callback = RecordingFederateAmbassador()
        subscriber_callback = RecordingFederateAmbassador()
        publisher = UmbraRtiFactory().getRtiAmbassador()
        subscriber = UmbraRtiFactory().getRtiAmbassador()
        created = publisher_joined = subscriber_joined = False
        try:
            publisher.connect(publisher_callback, CallbackModel.HLA_EVOKED)
            subscriber.connect(subscriber_callback, CallbackModel.HLA_EVOKED)
            publisher.createFederationExecution(
                federation_name, str(fom_module), "HLAinteger64Time"
            )
            created = True
            publisher.joinFederationExecution("timed-regional-attribute-publisher", federation_name)
            publisher_joined = True
            subscriber.joinFederationExecution("timed-regional-attribute-subscriber", federation_name)
            subscriber_joined = True

            publisher_class = publisher.getObjectClassHandle("HLAobjectRoot.Food.Drink.Soda")
            publisher_attribute = publisher.getAttributeHandle(publisher_class, "Flavor")
            subscriber_class = subscriber.getObjectClassHandle("HLAobjectRoot.Food.Drink.Soda")
            subscriber_attribute = subscriber.getAttributeHandle(subscriber_class, "Flavor")
            dimension = publisher.getDimensionHandle("SodaFlavor")
            publisher.publishObjectClassAttributes(
                publisher_class, AttributeHandleSet([publisher_attribute])
            )
            publisher.changeDefaultAttributeOrderType(
                publisher_class, AttributeHandleSet([publisher_attribute]), OrderType.TIMESTAMP
            )

            publisher_region = publisher.createRegion(DimensionHandleSet([dimension]))
            publisher.setRangeBounds(publisher_region, dimension, RangeBounds(0, 2))
            publisher.commitRegionModifications(RegionHandleSet([publisher_region]))
            subscriber_region = subscriber.createRegion(
                DimensionHandleSet([subscriber.getDimensionHandle("SodaFlavor")])
            )
            subscriber_dimension = subscriber.getDimensionHandle("SodaFlavor")
            subscriber.setRangeBounds(subscriber_region, subscriber_dimension, RangeBounds(1, 3))
            subscriber.commitRegionModifications(RegionHandleSet([subscriber_region]))

            publisher_pair = AttributeSetRegionSetPairList(
                [
                    AttributeSetRegionSetPair(
                        AttributeHandleSet([publisher_attribute]),
                        RegionHandleSet([publisher_region]),
                    )
                ]
            )
            subscriber_pair = AttributeSetRegionSetPairList(
                [
                    AttributeSetRegionSetPair(
                        AttributeHandleSet([subscriber_attribute]),
                        RegionHandleSet([subscriber_region]),
                    )
                ]
            )
            object_instance = publisher.registerObjectInstanceWithRegions(
                publisher_class, publisher_pair
            )
            subscriber.subscribeObjectClassAttributesWithRegions(
                subscriber_class, subscriber_pair
            )
            subscriber.setConveyRegionDesignatorSetsSwitch(True)
            self.assertFalse(subscriber.evokeCallback(0.0))

            subscriber.enableTimeConstrained()
            subscriber.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(len(subscriber_callback.constraints), 1)
            publisher.enableTimeRegulation(
                publisher.getTimeFactory().makeLogicalTimeInterval(5)
            )
            for _ in range(8):
                publisher.evokeCallback(0.0)
                if publisher_callback.regulations:
                    break
            self.assertEqual(len(publisher_callback.regulations), 1)
            timestamp = publisher.getTimeFactory().makeLogicalTime(6)
            retraction = publisher.updateAttributeValuesWithTime(
                object_instance,
                AttributeHandleValueMap({publisher_attribute: b"timed-regional-attribute"}),
                timestamp,
                b"timed-regional-attribute-tag",
            )
            self.assertTrue(retraction.isValid())
            self.assertEqual(subscriber_callback.reflections, [])

            publisher.timeAdvanceRequest(publisher.getTimeFactory().makeLogicalTime(2))
            subscriber.timeAdvanceRequest(subscriber.getTimeFactory().makeLogicalTime(6))
            for _ in range(8):
                publisher.evokeMultipleCallbacks(0.0, 0.0)
                subscriber.evokeMultipleCallbacks(0.0, 0.0)

            self.assertEqual(len(subscriber_callback.reflections), 1)
            reflection = subscriber_callback.reflections[0]
            self.assertEqual(reflection[0], object_instance)
            self.assertEqual(reflection[1][subscriber_attribute], b"timed-regional-attribute")
            self.assertEqual(reflection[2], b"timed-regional-attribute-tag")
            self.assertEqual(reflection[5], RegionHandleSet([publisher_region]))
            self.assertEqual(reflection[6].getTime(), 6)
            self.assertEqual(reflection[7], OrderType.TIMESTAMP)
            self.assertEqual(reflection[8], OrderType.TIMESTAMP)
            self.assertIsInstance(reflection[9], MessageRetractionHandle)
        finally:
            if subscriber_joined:
                subscriber.resignFederationExecution(ResignAction.NO_ACTION)
            if publisher_joined:
                publisher.resignFederationExecution(ResignAction.NO_ACTION)
            if created:
                publisher.destroyFederationExecution(federation_name)
            subscriber.disconnect()
            publisher.disconnect()

    def test_native_timestamped_regional_attribute_mixed_fanout_honors_convey_switch(self) -> None:
        class RecordingFederateAmbassador(FederateAmbassador):
            def __init__(self) -> None:
                self.reflections: list[tuple[object, ...]] = []
                self.grants: list[object] = []
                self.regulations: list[object] = []
                self.constraints: list[object] = []

            def reflectAttributeValues(self, *arguments) -> None:
                self.reflections.append(arguments)

            def timeAdvanceGrant(self, time) -> None:
                self.grants.append(time)

            def timeRegulationEnabled(self, time) -> None:
                self.regulations.append(time)

            def timeConstrainedEnabled(self, time) -> None:
                self.constraints.append(time)

        fom_module = (
            Path(__file__).parents[3]
            / "third_party"
            / "ieee1516.2-2025"
            / "resources"
            / "examples"
            / "RestaurantFOMmodule-2025.xml"
        )
        federation_name = f"python-native-timed-regional-mixed-fanout-{uuid4()}"
        publisher_callback = RecordingFederateAmbassador()
        receiver_callback = RecordingFederateAmbassador()
        immediate_callback = RecordingFederateAmbassador()
        publisher = UmbraRtiFactory().getRtiAmbassador()
        receiver = UmbraRtiFactory().getRtiAmbassador()
        immediate = UmbraRtiFactory().getRtiAmbassador()
        created = publisher_joined = receiver_joined = immediate_joined = False
        try:
            publisher.connect(publisher_callback, CallbackModel.HLA_EVOKED)
            receiver.connect(receiver_callback, CallbackModel.HLA_EVOKED)
            immediate.connect(immediate_callback, CallbackModel.HLA_IMMEDIATE)
            publisher.createFederationExecution(
                federation_name, str(fom_module), "HLAinteger64Time"
            )
            created = True
            publisher.joinFederationExecution("mixed-fanout-publisher", federation_name)
            publisher_joined = True
            receiver.joinFederationExecution("mixed-fanout-receiver", federation_name)
            receiver_joined = True
            immediate.joinFederationExecution("mixed-fanout-immediate", federation_name)
            immediate_joined = True

            publisher_class = publisher.getObjectClassHandle("HLAobjectRoot.Food.Drink.Soda")
            publisher_attribute = publisher.getAttributeHandle(publisher_class, "Flavor")
            receiver_class = receiver.getObjectClassHandle("HLAobjectRoot.Food.Drink.Soda")
            receiver_attribute = receiver.getAttributeHandle(receiver_class, "Flavor")
            immediate_class = immediate.getObjectClassHandle("HLAobjectRoot.Food.Drink.Soda")
            immediate_attribute = immediate.getAttributeHandle(immediate_class, "Flavor")
            publisher_dimension = publisher.getDimensionHandle("SodaFlavor")
            receiver_dimension = receiver.getDimensionHandle("SodaFlavor")
            immediate_dimension = immediate.getDimensionHandle("SodaFlavor")
            publisher_attributes = AttributeHandleSet([publisher_attribute])
            receiver_attributes = AttributeHandleSet([receiver_attribute])
            immediate_attributes = AttributeHandleSet([immediate_attribute])
            publisher.publishObjectClassAttributes(publisher_class, publisher_attributes)
            publisher.changeDefaultAttributeOrderType(
                publisher_class, publisher_attributes, OrderType.TIMESTAMP
            )

            publisher_region = publisher.createRegion(DimensionHandleSet([publisher_dimension]))
            receiver_region = receiver.createRegion(DimensionHandleSet([receiver_dimension]))
            immediate_region = immediate.createRegion(DimensionHandleSet([immediate_dimension]))
            publisher.setRangeBounds(publisher_region, publisher_dimension, RangeBounds(0, 2))
            receiver.setRangeBounds(receiver_region, receiver_dimension, RangeBounds(1, 3))
            immediate.setRangeBounds(immediate_region, immediate_dimension, RangeBounds(1, 3))
            publisher.commitRegionModifications(RegionHandleSet([publisher_region]))
            receiver.commitRegionModifications(RegionHandleSet([receiver_region]))
            immediate.commitRegionModifications(RegionHandleSet([immediate_region]))

            publisher_pairs = AttributeSetRegionSetPairList(
                [AttributeSetRegionSetPair(publisher_attributes, RegionHandleSet([publisher_region]))]
            )
            receiver_pairs = AttributeSetRegionSetPairList(
                [AttributeSetRegionSetPair(receiver_attributes, RegionHandleSet([receiver_region]))]
            )
            immediate_pairs = AttributeSetRegionSetPairList(
                [AttributeSetRegionSetPair(immediate_attributes, RegionHandleSet([immediate_region]))]
            )
            object_instance = publisher.registerObjectInstanceWithRegions(
                publisher_class, publisher_pairs
            )
            receiver.subscribeObjectClassAttributesWithRegions(receiver_class, receiver_pairs)
            immediate.subscribeObjectClassAttributesWithRegions(immediate_class, immediate_pairs)
            for _ in range(4):
                receiver.evokeCallback(0.0)

            receiver.enableTimeConstrained()
            for _ in range(8):
                receiver.evokeCallback(0.0)
                if receiver_callback.constraints:
                    break
            publisher.enableTimeRegulation(
                publisher.getTimeFactory().makeLogicalTimeInterval(5)
            )
            for _ in range(8):
                publisher.evokeCallback(0.0)
                if publisher_callback.regulations:
                    break

            values = AttributeHandleValueMap({publisher_attribute: b"mixed-fanout-one"})
            first_time = publisher.getTimeFactory().makeLogicalTime(7)
            first_retraction = publisher.updateAttributeValuesWithTime(
                object_instance, values, first_time, b"mixed-fanout-one-tag"
            )
            self.assertTrue(first_retraction.isValid())
            self.assertEqual(len(immediate_callback.reflections), 1)
            immediate_first = immediate_callback.reflections[-1]
            self.assertIsNone(immediate_first[5])
            self.assertEqual(immediate_first[6].getTime(), 7)
            self.assertEqual(immediate_first[7], OrderType.TIMESTAMP)
            self.assertEqual(immediate_first[8], OrderType.RECEIVE)

            publisher.timeAdvanceRequest(publisher.getTimeFactory().makeLogicalTime(2))
            receiver.timeAdvanceRequest(publisher.getTimeFactory().makeLogicalTime(7))
            for _ in range(8):
                publisher.evokeMultipleCallbacks(0.0, 0.0)
                receiver.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(len(receiver_callback.reflections), 1)
            self.assertIsNone(receiver_callback.reflections[-1][5])
            self.assertEqual(receiver_callback.reflections[-1][6].getTime(), 7)
            self.assertEqual(receiver_callback.reflections[-1][8], OrderType.TIMESTAMP)

            receiver.setConveyRegionDesignatorSetsSwitch(True)
            second_time = publisher.getTimeFactory().makeLogicalTime(8)
            second_retraction = publisher.updateAttributeValuesWithTime(
                object_instance,
                AttributeHandleValueMap({publisher_attribute: b"mixed-fanout-two"}),
                second_time,
                b"mixed-fanout-two-tag",
            )
            self.assertTrue(second_retraction.isValid())
            publisher.timeAdvanceRequest(publisher.getTimeFactory().makeLogicalTime(3))
            receiver.timeAdvanceRequest(publisher.getTimeFactory().makeLogicalTime(8))
            for _ in range(8):
                publisher.evokeMultipleCallbacks(0.0, 0.0)
                receiver.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(len(receiver_callback.reflections), 2)
            conveyed = receiver_callback.reflections[-1]
            self.assertEqual(conveyed[1][receiver_attribute], b"mixed-fanout-two")
            self.assertEqual(conveyed[2], b"mixed-fanout-two-tag")
            self.assertEqual(conveyed[5], RegionHandleSet([publisher_region]))
            self.assertEqual(conveyed[6].getTime(), 8)
            self.assertEqual(conveyed[7], OrderType.TIMESTAMP)
            self.assertEqual(conveyed[8], OrderType.TIMESTAMP)
            self.assertTrue(conveyed[9].isValid())
        finally:
            if receiver_joined:
                receiver.resignFederationExecution(ResignAction.NO_ACTION)
            if immediate_joined:
                immediate.resignFederationExecution(ResignAction.NO_ACTION)
            if publisher_joined:
                publisher.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST)
            if created:
                publisher.destroyFederationExecution(federation_name)
            receiver.disconnect()
            immediate.disconnect()
            publisher.disconnect()

    def test_native_exception_is_translated_at_the_provider_edge(self) -> None:
        ambassador = UmbraRtiFactory().getRtiAmbassador()
        callback = FederateAmbassador()
        ambassador.connect(callback, CallbackModel.HLA_IMMEDIATE)

        with self.assertRaises(AlreadyConnected):
            ambassador.connect(callback, CallbackModel.HLA_IMMEDIATE)

        ambassador.disconnect()

    def test_native_federation_creation_populates_the_typed_listing_callback(self) -> None:
        class RecordingFederateAmbassador(FederateAmbassador):
            def reportFederationExecutions(self, report) -> None:
                self.reports.append(report)

            def __init__(self) -> None:
                self.reports = []
                self.member_reports: list[tuple[str, FederationExecutionMemberInformationSet]] = []
                self.missing_federations: list[str] = []

            def reportFederationExecutionMembers(self, federationExecutionName, report) -> None:
                self.member_reports.append((federationExecutionName, report))

            def reportFederationExecutionDoesNotExist(self, federationExecutionName) -> None:
                self.missing_federations.append(federationExecutionName)

        fom_module = (
            Path(__file__).parents[3]
            / "third_party"
            / "ieee1516.2-2025"
            / "resources"
            / "examples"
            / "RestaurantFOMmodule-2025.xml"
        )
        federation_name = f"python-native-{uuid4()}"
        callback = RecordingFederateAmbassador()
        ambassador = UmbraRtiFactory().getRtiAmbassador()
        ambassador.connect(callback, CallbackModel.HLA_EVOKED)
        created = False
        try:
            ambassador.createFederationExecution(
                federation_name,
                str(fom_module),
                "HLAinteger64Time",
            )
            created = True
            ambassador.listFederationExecutions()
            ambassador.evokeCallback(0.0)
            self.assertIn(
                FederationExecutionInformation(federation_name, "HLAinteger64Time"),
                callback.reports[-1],
            )
            ambassador.listFederationExecutionMembers(federation_name)
            ambassador.evokeMultipleCallbacks(0.0, 0.1)
            self.assertEqual(
                callback.member_reports,
                [(federation_name, FederationExecutionMemberInformationSet())],
            )
            ambassador.listFederationExecutionMembers("python-native-missing-federation")
            ambassador.evokeMultipleCallbacks(0.0, 0.1)
            self.assertEqual(callback.missing_federations, ["python-native-missing-federation"])
        finally:
            if created:
                ambassador.destroyFederationExecution(federation_name)
            ambassador.disconnect()

    def test_native_directed_interaction_declaration_and_delivery(self) -> None:
        class RecordingFederateAmbassador(FederateAmbassador):
            def __init__(self) -> None:
                self.directed: list[tuple[object, ...]] = []
                self.name_batches: list[ObjectInstanceNameSet] = []

            def receiveDirectedInteraction(self, *arguments) -> None:
                self.directed.append(arguments)

            def multipleObjectInstanceNameReservationSucceeded(self, names) -> None:
                self.name_batches.append(names)

        fom_module = (
            Path(__file__).parents[3]
            / "third_party"
            / "ieee1516.2-2025"
            / "resources"
            / "examples"
            / "RestaurantFOMmodule-2025.xml"
        )
        federation_name = f"python-native-directed-interaction-{uuid4()}"
        publisher_callback = RecordingFederateAmbassador()
        subscriber_callback = RecordingFederateAmbassador()
        publisher = UmbraRtiFactory().getRtiAmbassador()
        subscriber = UmbraRtiFactory().getRtiAmbassador()
        created = publisher_joined = subscriber_joined = False
        try:
            publisher.connect(publisher_callback, CallbackModel.HLA_EVOKED)
            subscriber.connect(subscriber_callback, CallbackModel.HLA_EVOKED)
            publisher.createFederationExecution(federation_name, str(fom_module), "HLAinteger64Time")
            created = True
            publisher.joinFederationExecution("directed-publisher", federation_name)
            publisher_joined = True
            subscriber.joinFederationExecution("directed-subscriber", federation_name)
            subscriber_joined = True
            object_class = publisher.getObjectClassHandle("HLAobjectRoot.Employee.Server")
            subscriber_object_class = subscriber.getObjectClassHandle("HLAobjectRoot.Employee.Server")
            attribute = publisher.getAttributeHandle(object_class, "Efficiency")
            subscriber_attribute = subscriber.getAttributeHandle(subscriber_object_class, "Efficiency")
            directed = publisher.getInteractionClassHandle("HLAinteractionRoot.ServerAction.TakeOrder")
            directed_for_subscriber = subscriber.getInteractionClassHandle("HLAinteractionRoot.ServerAction.TakeOrder")
            attributes = AttributeHandleSet([attribute])
            subscriber_attributes = AttributeHandleSet([subscriber_attribute])
            publisher.publishObjectClassAttributes(object_class, attributes)
            subscriber.subscribeObjectClassAttributes(subscriber_object_class, subscriber_attributes)
            publisher.evokeMultipleCallbacks(0.0, 0.0)
            publisher.reserveMultipleObjectInstanceNames(
                ObjectInstanceNameSet(["directed-batch-one", "directed-batch-two"])
            )
            publisher.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(
                publisher_callback.name_batches[-1],
                ObjectInstanceNameSet(["directed-batch-one", "directed-batch-two"]),
            )
            publisher.releaseMultipleObjectInstanceNames(
                ObjectInstanceNameSet(["directed-batch-one", "directed-batch-two"])
            )
            publisher.reserveObjectInstanceName("directed-server")
            instance = publisher.registerObjectInstance(object_class, objectInstanceName="directed-server")
            subscriber.evokeMultipleCallbacks(0.0, 0.0)
            self.assertTrue(subscriber_callback.directed == [])
            publisher.publishObjectClassDirectedInteractions(
                object_class, InteractionClassHandleSet([directed])
            )
            subscriber.subscribeObjectClassDirectedInteractions(
                subscriber_object_class,
                InteractionClassHandleSet([directed_for_subscriber]),
                universally=True,
            )
            publisher.sendDirectedInteraction(directed, instance, ParameterHandleValueMap(), b"directed-tag")
            subscriber.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(len(subscriber_callback.directed), 1)
            callback = subscriber_callback.directed[0]
            self.assertEqual(callback[0], directed_for_subscriber)
            self.assertEqual(callback[1], instance)
            self.assertEqual(callback[2], ParameterHandleValueMap())
            self.assertEqual(callback[3], b"directed-tag")
            self.assertIsInstance(callback[4], TransportationTypeHandle)
            self.assertIsInstance(callback[5], FederateHandle)
            publisher.unpublishObjectClassDirectedInteractions(object_class, InteractionClassHandleSet([directed]))
            subscriber.unsubscribeObjectClassDirectedInteractions(subscriber_object_class)
            with self.assertRaises(FederateOwnsAttributes):
                publisher.localDeleteObjectInstance(instance)
        finally:
            if subscriber_joined:
                subscriber.resignFederationExecution(ResignAction.NO_ACTION)
            if publisher_joined:
                publisher.resignFederationExecution(ResignAction.NO_ACTION)
            if created:
                publisher.destroyFederationExecution(federation_name)
            subscriber.disconnect()
            publisher.disconnect()

    def test_native_directed_tso_retraction_notifies_only_delivered_recipients(self) -> None:
        class RecordingFederateAmbassador(FederateAmbassador):
            def __init__(self) -> None:
                self.directed: list[tuple[object, ...]] = []
                self.retractions: list[MessageRetractionHandle] = []
                self.grants: list[object] = []

            def receiveDirectedInteraction(self, *arguments) -> None:
                self.directed.append(arguments)

            def requestRetraction(self, retraction) -> None:
                self.retractions.append(retraction)

            def timeAdvanceGrant(self, time) -> None:
                self.grants.append(time)

        object_consumer = (
            Path(__file__).parents[3]
            / "cpp"
            / "tests"
            / "data"
            / "directed-interaction-object-consumer-fom.xml"
        )
        interaction_provider = (
            Path(__file__).parents[3]
            / "cpp"
            / "tests"
            / "data"
            / "directed-interaction-interaction-provider-fom.xml"
        )
        federation_name = f"python-native-directed-retraction-{uuid4()}"
        publisher_callback = RecordingFederateAmbassador()
        immediate_callback = RecordingFederateAmbassador()
        constrained_callback = RecordingFederateAmbassador()
        publisher = UmbraRtiFactory().getRtiAmbassador()
        immediate = UmbraRtiFactory().getRtiAmbassador()
        constrained = UmbraRtiFactory().getRtiAmbassador()
        created = publisher_joined = immediate_joined = constrained_joined = False
        try:
            publisher.connect(publisher_callback, CallbackModel.HLA_EVOKED)
            immediate.connect(immediate_callback, CallbackModel.HLA_EVOKED)
            constrained.connect(constrained_callback, CallbackModel.HLA_EVOKED)
            publisher.createFederationExecution(
                federation_name,
                [str(object_consumer), str(interaction_provider)],
                "HLAinteger64Time",
            )
            created = True
            publisher.joinFederationExecution("directed-retraction-publisher", federation_name)
            publisher_joined = True
            immediate.joinFederationExecution("directed-retraction-immediate", federation_name)
            immediate_joined = True
            constrained.joinFederationExecution(
                "directed-retraction-constrained", federation_name
            )
            constrained_joined = True

            publisher_object_class = publisher.getObjectClassHandle(
                "HLAobjectRoot.UmbraDirectedFixtureObject"
            )
            immediate_object_class = immediate.getObjectClassHandle(
                "HLAobjectRoot.UmbraDirectedFixtureObject"
            )
            constrained_object_class = constrained.getObjectClassHandle(
                "HLAobjectRoot.UmbraDirectedFixtureObject"
            )
            marker = publisher.getAttributeHandle(publisher_object_class, "DirectedTargetMarker")
            immediate_marker = immediate.getAttributeHandle(
                immediate_object_class, "DirectedTargetMarker"
            )
            constrained_marker = constrained.getAttributeHandle(
                constrained_object_class, "DirectedTargetMarker"
            )
            interaction = publisher.getInteractionClassHandle(
                "HLAinteractionRoot.UmbraDirectedFixtureInteraction"
            )
            immediate_interaction = immediate.getInteractionClassHandle(
                "HLAinteractionRoot.UmbraDirectedFixtureInteraction"
            )
            constrained_interaction = constrained.getInteractionClassHandle(
                "HLAinteractionRoot.UmbraDirectedFixtureInteraction"
            )
            publisher.publishObjectClassAttributes(
                publisher_object_class, AttributeHandleSet([marker])
            )
            immediate.subscribeObjectClassAttributes(
                immediate_object_class, AttributeHandleSet([immediate_marker])
            )
            constrained.subscribeObjectClassAttributes(
                constrained_object_class, AttributeHandleSet([constrained_marker])
            )
            publisher.publishObjectClassDirectedInteractions(
                publisher_object_class, InteractionClassHandleSet([interaction])
            )
            publisher.changeInteractionOrderType(interaction, OrderType.TIMESTAMP)
            immediate.subscribeObjectClassDirectedInteractions(
                immediate_object_class,
                InteractionClassHandleSet([immediate_interaction]),
                universally=True,
            )
            constrained.subscribeObjectClassDirectedInteractions(
                constrained_object_class,
                InteractionClassHandleSet([constrained_interaction]),
                universally=True,
            )
            target = publisher.registerObjectInstance(publisher_object_class)
            for _ in range(8):
                immediate.evokeMultipleCallbacks(0.0, 0.0)
                constrained.evokeMultipleCallbacks(0.0, 0.0)

            constrained.enableTimeConstrained()
            for _ in range(4):
                constrained.evokeMultipleCallbacks(0.0, 0.0)
            publisher.enableTimeRegulation(
                publisher.getTimeFactory().makeLogicalTimeInterval(1)
            )
            for _ in range(4):
                publisher.evokeMultipleCallbacks(0.0, 0.0)

            retraction = publisher.sendDirectedInteractionWithTime(
                interaction,
                target,
                ParameterHandleValueMap(),
                publisher.getTimeFactory().makeLogicalTime(2),
                b"directed-retraction-tag",
            )
            self.assertTrue(retraction.isValid())
            for _ in range(4):
                immediate.evokeMultipleCallbacks(0.0, 0.0)
                constrained.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(len(immediate_callback.directed), 1)
            self.assertEqual(constrained_callback.directed, [])
            first = immediate_callback.directed[0]
            self.assertEqual(first[1], target)
            self.assertEqual(first[2], ParameterHandleValueMap())
            self.assertEqual(first[3], b"directed-retraction-tag")
            self.assertEqual(first[6].getTime(), 2)
            self.assertEqual(first[7], OrderType.TIMESTAMP)
            self.assertEqual(first[8], OrderType.RECEIVE)
            self.assertEqual(first[9].encodedValue, retraction.encodedValue)

            publisher.retract(retraction)
            for _ in range(4):
                immediate.evokeMultipleCallbacks(0.0, 0.0)
                constrained.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(len(immediate_callback.retractions), 1)
            self.assertEqual(immediate_callback.retractions[0].encodedValue, retraction.encodedValue)
            self.assertEqual(constrained_callback.retractions, [])

            constrained.timeAdvanceRequest(constrained.getTimeFactory().makeLogicalTime(2))
            publisher.timeAdvanceRequest(publisher.getTimeFactory().makeLogicalTime(2))
            for _ in range(8):
                constrained.evokeMultipleCallbacks(0.0, 0.0)
                publisher.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(constrained_callback.directed, [])
            self.assertEqual(constrained_callback.retractions, [])
            self.assertEqual(constrained_callback.grants[-1].getTime(), 2)

            constrained.resignFederationExecution(ResignAction.NO_ACTION)
            constrained_joined = False
            immediate_only = publisher.sendDirectedInteractionWithTime(
                interaction,
                target,
                ParameterHandleValueMap(),
                publisher.getTimeFactory().makeLogicalTime(4),
                b"directed-immediate-only",
            )
            self.assertTrue(immediate_only.isValid())
            for _ in range(4):
                immediate.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(len(immediate_callback.directed), 2)
            publisher.retract(immediate_only)
            for _ in range(4):
                immediate.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(len(immediate_callback.retractions), 2)
            self.assertEqual(
                immediate_callback.retractions[-1].encodedValue,
                immediate_only.encodedValue,
            )
        finally:
            if immediate_joined:
                immediate.resignFederationExecution(ResignAction.NO_ACTION)
            if constrained_joined:
                constrained.resignFederationExecution(ResignAction.NO_ACTION)
            if publisher_joined:
                publisher.resignFederationExecution(
                    ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST
                )
            if created:
                publisher.destroyFederationExecution(federation_name)
            constrained.disconnect()
            immediate.disconnect()
            publisher.disconnect()

    def test_native_join_returns_a_portable_handle_and_populates_member_reporting(self) -> None:
        class RecordingFederateAmbassador(FederateAmbassador):
            def __init__(self) -> None:
                self.member_reports: list[tuple[str, FederationExecutionMemberInformationSet]] = []
                self.registered: list[str] = []
                self.announcements: list[tuple[str, bytes]] = []

            def reportFederationExecutionMembers(self, federationExecutionName, report) -> None:
                self.member_reports.append((federationExecutionName, report))

            def synchronizationPointRegistrationSucceeded(self, synchronizationPointLabel) -> None:
                self.registered.append(synchronizationPointLabel)

            def announceSynchronizationPoint(self, synchronizationPointLabel, userSuppliedTag) -> None:
                self.announcements.append((synchronizationPointLabel, userSuppliedTag))

        fom_module = (
            Path(__file__).parents[3]
            / "third_party"
            / "ieee1516.2-2025"
            / "resources"
            / "examples"
            / "RestaurantFOMmodule-2025.xml"
        )
        federation_name = f"python-native-join-{uuid4()}"
        callback = RecordingFederateAmbassador()
        ambassador = UmbraRtiFactory().getRtiAmbassador()
        ambassador.connect(callback, CallbackModel.HLA_EVOKED)
        created = joined = False
        try:
            ambassador.createFederationExecution(federation_name, str(fom_module), "HLAinteger64Time")
            created = True
            handle = ambassador.joinFederationExecution(
                "observer", federation_name, federateName="python-member"
            )
            joined = True
            self.assertGreater(handle.encodedLength(), 0)
            encoded = bytearray(handle.encodedLength())
            handle.encode(encoded)
            self.assertEqual(bytes(encoded), handle.encodedValue)
            ambassador.listFederationExecutionMembers(federation_name)
            ambassador.evokeMultipleCallbacks(0.0, 0.1)
            self.assertEqual(callback.member_reports[-1][0], federation_name)
            self.assertEqual(
                callback.member_reports[-1][1],
                FederationExecutionMemberInformationSet(
                    [FederationExecutionMemberInformation("python-member", "observer")]
                ),
            )
            ambassador.resignFederationExecution(ResignAction.NO_ACTION)
            joined = False
            unnamed_handle = ambassador.joinFederationExecution("observer", federation_name)
            joined = True
            self.assertGreater(unnamed_handle.encodedLength(), 0)
            ambassador.registerFederationSynchronizationPoint("python-sync", b"python-tag")
            ambassador.evokeMultipleCallbacks(0.0, 0.1)
            self.assertEqual(callback.registered, ["python-sync"])
            self.assertEqual(callback.announcements, [("python-sync", b"python-tag")])
        finally:
            if joined:
                ambassador.resignFederationExecution(ResignAction.NO_ACTION)
            if created:
                ambassador.destroyFederationExecution(federation_name)
            ambassador.disconnect()

    def test_native_lookup_services_round_trip_typed_fom_handles(self) -> None:
        fom_module = (
            Path(__file__).parents[3]
            / "third_party"
            / "ieee1516.2-2025"
            / "resources"
            / "examples"
            / "RestaurantFOMmodule-2025.xml"
        )
        federation_name = f"python-native-lookups-{uuid4()}"
        ambassador = UmbraRtiFactory().getRtiAmbassador()
        ambassador.connect(FederateAmbassador(), CallbackModel.HLA_EVOKED)
        created = joined = False
        try:
            ambassador.createFederationExecution(federation_name, str(fom_module), "HLAinteger64Time")
            created = True
            ambassador.joinFederationExecution("observer", federation_name, federateName="observer")
            joined = True

            object_class = ambassador.getObjectClassHandle("HLAobjectRoot.Employee.Server")
            attribute = ambassador.getAttributeHandle(object_class, "Efficiency")
            interaction = ambassador.getInteractionClassHandle(
                "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed"
            )
            parameter = ambassador.getParameterHandle(interaction, "TemperatureOk")
            transportation = ambassador.getTransportationTypeHandle("HLAreliable")
            dimension = ambassador.getDimensionHandle("ServerId")

            self.assertIsInstance(object_class, ObjectClassHandle)
            self.assertIsInstance(attribute, AttributeHandle)
            self.assertIsInstance(interaction, InteractionClassHandle)
            self.assertIsInstance(parameter, ParameterHandle)
            self.assertIsInstance(transportation, TransportationTypeHandle)
            self.assertIsInstance(dimension, DimensionHandle)
            self.assertNotEqual(object_class, attribute)
            self.assertEqual(ambassador.getObjectClassName(object_class), "HLAobjectRoot.Employee.Server")
            self.assertEqual(ambassador.getAttributeName(object_class, attribute), "Efficiency")
            self.assertEqual(
                ambassador.getInteractionClassName(interaction),
                "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed",
            )
            self.assertEqual(ambassador.getParameterName(interaction, parameter), "TemperatureOk")
            self.assertEqual(ambassador.getTransportationTypeName(transportation), "HLAreliable")
            self.assertEqual(ambassador.getDimensionName(dimension), "ServerId")
            federate = ambassador.getFederateHandle("observer")
            self.assertEqual(ambassador.getFederateName(federate), "observer")
            soda_class = ambassador.getObjectClassHandle("HLAobjectRoot.Food.Drink.Soda")
            soda_dimension = ambassador.getDimensionHandle("SodaFlavor")
            bar_dimension = ambassador.getDimensionHandle("BarQuantity")
            self.assertEqual(
                ambassador.getAvailableDimensionsForObjectClass(soda_class),
                DimensionHandleSet([soda_dimension, bar_dimension]),
            )
            self.assertEqual(
                ambassador.getAvailableDimensionsForInteractionClass(interaction),
                DimensionHandleSet([dimension]),
            )
            self.assertEqual(ambassador.getDimensionUpperBound(dimension), 20)
            self.assertEqual(ambassador.getOrderType("Receive"), OrderType.RECEIVE)
            self.assertEqual(ambassador.getOrderName(OrderType.TIMESTAMP), "TimeStamp")
            self.assertEqual(ambassador.normalizeServiceGroup(ServiceGroup.OBJECT_MANAGEMENT), 2)
            self.assertGreaterEqual(ambassador.normalizeFederateHandle(federate), 0)
            self.assertGreaterEqual(ambassador.normalizeObjectClassHandle(object_class), 0)
            self.assertGreaterEqual(ambassador.normalizeInteractionClassHandle(interaction), 0)
            self.assertEqual(ambassador.getUpdateRateValue("High"), 30.0)
            ambassador.publishObjectClassAttributes(object_class, AttributeHandleSet([attribute]))
            ambassador.subscribeObjectClassAttributes(
                object_class,
                AttributeHandleSet([attribute]),
                active=True,
                updateRateDesignator="High",
            )
            registered = ambassador.registerObjectInstance(object_class)
            self.assertEqual(ambassador.getKnownObjectClassHandle(registered), object_class)
            self.assertGreaterEqual(ambassador.normalizeObjectInstanceHandle(registered), 0)
            self.assertEqual(ambassador.getUpdateRateValueForAttribute(registered, attribute), 30.0)
            self.assertEqual(
                ambassador.getObjectClassHandleFactory().decode(object_class.encodedValue),
                object_class,
            )
            self.assertEqual(
                ambassador.getObjectInstanceHandleFactory().decode(registered.encodedValue),
                registered,
            )
            self.assertEqual(
                ambassador.getAttributeHandleFactory().decode(attribute.encodedValue),
                attribute,
            )
            self.assertEqual(
                ambassador.getInteractionClassHandleFactory().decode(interaction.encodedValue),
                interaction,
            )
            self.assertEqual(
                ambassador.getParameterHandleFactory().decode(parameter.encodedValue),
                parameter,
            )
            self.assertEqual(
                ambassador.getTransportationTypeHandleFactory().decode(
                    transportation.encodedValue
                ),
                transportation,
            )
            self.assertEqual(
                ambassador.getDimensionHandleFactory().decode(dimension.encodedValue),
                dimension,
            )
            self.assertEqual(
                ambassador.getFederateHandleFactory().decode(
                    ambassador.getFederateHandle("observer").encodedValue
                ),
                ambassador.getFederateHandle("observer"),
            )
            attributes_from_factory = ambassador.getAttributeHandleSetFactory().create()
            attributes_from_factory.add(attribute)
            values_from_factory = ambassador.getAttributeHandleValueMapFactory().create()
            values_from_factory[attribute] = bytearray(b"factory-value")
            ambassador.updateAttributeValues(registered, values_from_factory)
            ambassador.deleteObjectInstance(registered)
            with self.assertRaises(TypeError):
                ambassador.getObjectClassName(attribute)  # type: ignore[arg-type]
        finally:
            if joined:
                ambassador.resignFederationExecution(ResignAction.NO_ACTION)
            if created:
                ambassador.destroyFederationExecution(federation_name)
            ambassador.disconnect()

    def test_native_federation_overloads_map_vectors_mim_and_handle_sets(self) -> None:
        fom_module = (
            Path(__file__).parents[3]
            / "third_party"
            / "ieee1516.2-2025"
            / "resources"
            / "examples"
            / "RestaurantFOMmodule-2025.xml"
        )
        mim_module = (
            Path(__file__).parents[3]
            / "third_party"
            / "ieee1516.2-2025"
            / "resources"
            / "mim"
            / "HLAstandardMIM-2025.xml"
        )
        federation_name = f"python-native-overloads-{uuid4()}"
        mim_federation_name = f"python-native-mim-{uuid4()}"
        ambassador = UmbraRtiFactory().getRtiAmbassador()
        ambassador.connect(FederateAmbassador(), CallbackModel.HLA_EVOKED)
        created = joined = mim_created = False
        try:
            ambassador.createFederationExecution(
                federation_name, [str(fom_module)], "HLAinteger64Time"
            )
            created = True
            handle = ambassador.joinFederationExecution(
                "observer", federation_name, additionalFomModules=[str(fom_module)]
            )
            joined = True
            ambassador.registerFederationSynchronizationPoint(
                "explicit-set", b"tag", synchronizationSet=FederateHandleSet([handle])
            )
            ambassador.resignFederationExecution(ResignAction.NO_ACTION)
            joined = False

            ambassador.createFederationExecutionWithMIM(
                mim_federation_name,
                [str(fom_module)],
                str(mim_module),
                "HLAinteger64Time",
            )
            mim_created = True
        finally:
            if joined:
                ambassador.resignFederationExecution(ResignAction.NO_ACTION)
            if mim_created:
                ambassador.destroyFederationExecution(mim_federation_name)
            if created:
                ambassador.destroyFederationExecution(federation_name)
            ambassador.disconnect()

    def test_native_declaration_services_emit_typed_relevance_advisories(self) -> None:
        class RecordingFederateAmbassador(FederateAmbassador):
            def __init__(self) -> None:
                self.start_registration: list[ObjectClassHandle] = []
                self.stop_registration: list[ObjectClassHandle] = []
                self.interactions_on: list[InteractionClassHandle] = []
                self.interactions_off: list[InteractionClassHandle] = []

            def startRegistrationForObjectClass(self, objectClass) -> None:
                self.start_registration.append(objectClass)

            def stopRegistrationForObjectClass(self, objectClass) -> None:
                self.stop_registration.append(objectClass)

            def turnInteractionsOn(self, interactionClass) -> None:
                self.interactions_on.append(interactionClass)

            def turnInteractionsOff(self, interactionClass) -> None:
                self.interactions_off.append(interactionClass)

        fom_module = (
            Path(__file__).parents[3]
            / "third_party"
            / "ieee1516.2-2025"
            / "resources"
            / "examples"
            / "RestaurantFOMmodule-2025.xml"
        )
        federation_name = f"python-native-declaration-{uuid4()}"
        publisher_callback = RecordingFederateAmbassador()
        subscriber_callback = RecordingFederateAmbassador()
        publisher = UmbraRtiFactory().getRtiAmbassador()
        subscriber = UmbraRtiFactory().getRtiAmbassador()
        publisher.connect(publisher_callback, CallbackModel.HLA_EVOKED)
        subscriber.connect(subscriber_callback, CallbackModel.HLA_EVOKED)
        created = publisher_joined = subscriber_joined = False
        try:
            publisher.createFederationExecution(federation_name, str(fom_module), "HLAinteger64Time")
            created = True
            publisher.joinFederationExecution("publisher", federation_name)
            publisher_joined = True
            subscriber.joinFederationExecution("subscriber", federation_name)
            subscriber_joined = True

            object_class = publisher.getObjectClassHandle("HLAobjectRoot.Employee.Server")
            attributes = AttributeHandleSet([publisher.getAttributeHandle(object_class, "Efficiency")])
            interaction = publisher.getInteractionClassHandle("HLAinteractionRoot.ServerAction.TakeOrder")
            subscriber_object_class = subscriber.getObjectClassHandle("HLAobjectRoot.Employee.Server")
            subscriber_attributes = AttributeHandleSet(
                [subscriber.getAttributeHandle(subscriber_object_class, "Efficiency")]
            )
            subscriber_interaction = subscriber.getInteractionClassHandle(
                "HLAinteractionRoot.ServerAction.TakeOrder"
            )

            subscriber.subscribeObjectClassAttributes(subscriber_object_class, subscriber_attributes)
            publisher.publishObjectClassAttributes(object_class, attributes)
            publisher.evokeMultipleCallbacks(0.0, 0.0)
            publisher.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(publisher_callback.start_registration, [object_class])
            subscriber.unsubscribeObjectClassAttributes(subscriber_object_class, subscriber_attributes)
            publisher.evokeMultipleCallbacks(0.0, 0.0)
            publisher.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(publisher_callback.stop_registration, [object_class])

            subscriber.subscribeInteractionClass(subscriber_interaction)
            publisher.publishInteractionClass(interaction)
            publisher.evokeMultipleCallbacks(0.0, 0.0)
            publisher.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(publisher_callback.interactions_on, [interaction])
            subscriber.unsubscribeInteractionClass(subscriber_interaction)
            publisher.evokeMultipleCallbacks(0.0, 0.0)
            publisher.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(publisher_callback.interactions_off, [interaction])

            subscriber.subscribeObjectClassAttributes(
                subscriber_object_class, subscriber_attributes, active=False, updateRateDesignator=""
            )
            subscriber.unsubscribeObjectClass(subscriber_object_class)
            publisher.unpublishInteractionClass(interaction)
            publisher.unpublishObjectClassAttributes(object_class, attributes)
            publisher.publishObjectClassAttributes(object_class, attributes)
            publisher.unpublishObjectClass(object_class)
            with self.assertRaises(TypeError):
                publisher.publishObjectClassAttributes(object_class, frozenset())  # type: ignore[arg-type]
        finally:
            if subscriber_joined:
                subscriber.resignFederationExecution(ResignAction.NO_ACTION)
            if publisher_joined:
                publisher.resignFederationExecution(ResignAction.NO_ACTION)
            if created:
                publisher.destroyFederationExecution(federation_name)
            publisher.disconnect()
            subscriber.disconnect()

    def test_native_registration_discovers_a_portable_object_instance(self) -> None:
        class RecordingFederateAmbassador(FederateAmbassador):
            def __init__(self) -> None:
                self.discoveries: list[
                    tuple[ObjectInstanceHandle, ObjectClassHandle, str, FederateHandle]
                ] = []
                self.reserved_names: list[str] = []
                self.rejected_reservations: list[str] = []
                self.removals: list[tuple[ObjectInstanceHandle, bytes, FederateHandle]] = []
                self.reflections: list[
                    tuple[
                        ObjectInstanceHandle,
                        AttributeHandleValueMap,
                        bytes,
                        TransportationTypeHandle,
                        FederateHandle,
                    ]
                ] = []
                self.interactions: list[
                    tuple[
                        InteractionClassHandle,
                        ParameterHandleValueMap,
                        bytes,
                        TransportationTypeHandle,
                        FederateHandle,
                    ]
                ] = []

            def discoverObjectInstance(
                self, objectInstance, objectClass, objectInstanceName, producingFederate
            ) -> None:
                self.discoveries.append(
                    (objectInstance, objectClass, objectInstanceName, producingFederate)
                )

            def objectInstanceNameReservationSucceeded(self, objectInstanceName) -> None:
                self.reserved_names.append(objectInstanceName)

            def objectInstanceNameReservationFailed(self, objectInstanceName) -> None:
                self.rejected_reservations.append(objectInstanceName)

            def removeObjectInstance(
                self, objectInstance, userSuppliedTag, producingFederate
            ) -> None:
                self.removals.append((objectInstance, userSuppliedTag, producingFederate))

            def reflectAttributeValues(
                self,
                objectInstance,
                attributeValues,
                userSuppliedTag,
                transportationType,
                producingFederate,
            ) -> None:
                self.reflections.append(
                    (
                        objectInstance,
                        attributeValues,
                        userSuppliedTag,
                        transportationType,
                        producingFederate,
                    )
                )

            def receiveInteraction(
                self,
                interactionClass,
                parameterValues,
                userSuppliedTag,
                transportationType,
                producingFederate,
            ) -> None:
                self.interactions.append(
                    (
                        interactionClass,
                        parameterValues,
                        userSuppliedTag,
                        transportationType,
                        producingFederate,
                    )
                )

        fom_module = (
            Path(__file__).parents[3]
            / "third_party"
            / "ieee1516.2-2025"
            / "resources"
            / "examples"
            / "RestaurantFOMmodule-2025.xml"
        )
        federation_name = f"python-native-registration-{uuid4()}"
        publisher_callback = RecordingFederateAmbassador()
        subscriber_callback = RecordingFederateAmbassador()
        publisher = UmbraRtiFactory().getRtiAmbassador()
        subscriber = UmbraRtiFactory().getRtiAmbassador()
        publisher.connect(publisher_callback, CallbackModel.HLA_EVOKED)
        subscriber.connect(subscriber_callback, CallbackModel.HLA_EVOKED)
        created = publisher_joined = subscriber_joined = False
        try:
            publisher.createFederationExecution(federation_name, str(fom_module), "HLAinteger64Time")
            created = True
            publisher.joinFederationExecution("publisher", federation_name)
            publisher_joined = True
            subscriber.joinFederationExecution("subscriber", federation_name)
            subscriber_joined = True

            publisher_class = publisher.getObjectClassHandle("HLAobjectRoot.Employee.Server")
            publisher_attributes = AttributeHandleSet(
                [publisher.getAttributeHandle(publisher_class, "Efficiency")]
            )
            subscriber_class = subscriber.getObjectClassHandle("HLAobjectRoot.Employee.Server")
            subscriber_attributes = AttributeHandleSet(
                [subscriber.getAttributeHandle(subscriber_class, "Efficiency")]
            )
            subscriber.subscribeObjectClassAttributes(subscriber_class, subscriber_attributes)
            publisher.publishObjectClassAttributes(publisher_class, publisher_attributes)
            publisher_interaction = publisher.getInteractionClassHandle(
                "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed"
            )
            publisher_parameter = publisher.getParameterHandle(
                publisher_interaction, "TemperatureOk"
            )
            subscriber_interaction = subscriber.getInteractionClassHandle(
                "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed"
            )
            subscriber_parameter = subscriber.getParameterHandle(
                subscriber_interaction, "TemperatureOk"
            )
            subscriber.subscribeInteractionClass(subscriber_interaction)
            publisher.publishInteractionClass(publisher_interaction)
            requested_name = "python-registered-server"
            publisher.reserveObjectInstanceName(requested_name)
            publisher.evokeMultipleCallbacks(0.0, 0.0)
            publisher.evokeMultipleCallbacks(0.0, 0.0)
            publisher.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(publisher_callback.reserved_names, [requested_name])
            registered = publisher.registerObjectInstance(
                publisher_class, objectInstanceName=requested_name
            )
            registered_name = publisher.getObjectInstanceName(registered)
            subscriber.disableCallbacks()
            subscriber.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(subscriber_callback.discoveries, [])
            subscriber.enableCallbacks()
            subscriber.evokeMultipleCallbacks(0.0, 0.0)
            subscriber.evokeMultipleCallbacks(0.0, 0.0)
            self.assertIsInstance(registered, ObjectInstanceHandle)
            self.assertEqual(len(subscriber_callback.discoveries), 1)
            discovered, object_class, name, producing_federate = subscriber_callback.discoveries[0]
            self.assertIsInstance(discovered, ObjectInstanceHandle)
            self.assertEqual(object_class, subscriber_class)
            self.assertEqual(name, registered_name)
            self.assertIsInstance(producing_federate, FederateHandle)
            self.assertGreater(producing_federate.encodedLength(), 0)
            self.assertEqual(subscriber.getObjectInstanceHandle(name), discovered)
            self.assertEqual(subscriber.getObjectInstanceName(discovered), name)
            update_tag = b"\x00python-update-tag\xff"
            values = AttributeHandleValueMap(
                {next(iter(publisher_attributes)): b"\x01\x02\xff"}
            )
            publisher.updateAttributeValues(registered, values, update_tag)
            subscriber.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(len(subscriber_callback.reflections), 1)
            reflected, reflected_values, reflected_tag, transportation, producer = (
                subscriber_callback.reflections[0]
            )
            self.assertEqual(reflected, discovered)
            self.assertEqual(
                reflected_values,
                {next(iter(subscriber_attributes)): b"\x01\x02\xff"},
            )
            self.assertEqual(reflected_tag, update_tag)
            self.assertIsInstance(transportation, TransportationTypeHandle)
            self.assertEqual(producer, producing_federate)
            interaction_tag = b"\x00python-interaction-tag\xff"
            parameter_values = ParameterHandleValueMap({publisher_parameter: b"\x01"})
            publisher.sendInteraction(publisher_interaction, parameter_values, interaction_tag)
            subscriber.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(len(subscriber_callback.interactions), 1)
            received_interaction, received_values, received_tag, received_transport, received_producer = (
                subscriber_callback.interactions[0]
            )
            self.assertEqual(received_interaction, subscriber_interaction)
            self.assertEqual(received_values, {subscriber_parameter: b"\x01"})
            self.assertEqual(received_tag, interaction_tag)
            self.assertIsInstance(received_transport, TransportationTypeHandle)
            self.assertEqual(received_producer, producing_federate)
            deletion_tag = b"\x00python-delete-tag\xff"
            publisher.deleteObjectInstance(registered, deletion_tag)
            subscriber.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(
                subscriber_callback.removals,
                [(discovered, deletion_tag, producing_federate)],
            )
            with self.assertRaises(ObjectInstanceNotKnown):
                subscriber.getObjectInstanceHandle(name)
        finally:
            if subscriber_joined:
                subscriber.resignFederationExecution(ResignAction.NO_ACTION)
            if publisher_joined:
                publisher.resignFederationExecution(ResignAction.NO_ACTION)
            if created:
                publisher.destroyFederationExecution(federation_name)
            publisher.disconnect()
            subscriber.disconnect()

    def test_native_regional_object_association_keeps_other_regions_active(self) -> None:
        class RecordingFederateAmbassador(FederateAmbassador):
            def __init__(self) -> None:
                self.discoveries: list[tuple[object, ...]] = []
                self.out_of_scope: list[tuple[object, ...]] = []
                self.reflections: list[tuple[object, ...]] = []

            def discoverObjectInstance(self, *arguments) -> None:
                self.discoveries.append(arguments)

            def attributesOutOfScope(self, *arguments) -> None:
                self.out_of_scope.append(arguments)

            def reflectAttributeValues(self, *arguments) -> None:
                self.reflections.append(arguments)

        fom_module = (
            Path(__file__).parents[3]
            / "third_party"
            / "ieee1516.2-2025"
            / "resources"
            / "examples"
            / "RestaurantFOMmodule-2025.xml"
        )
        federation_name = f"python-native-regional-association-{uuid4()}"
        publisher_callback = RecordingFederateAmbassador()
        subscriber_callback = RecordingFederateAmbassador()
        publisher = UmbraRtiFactory().getRtiAmbassador()
        subscriber = UmbraRtiFactory().getRtiAmbassador()
        created = publisher_joined = subscriber_joined = False
        try:
            publisher.connect(publisher_callback, CallbackModel.HLA_EVOKED)
            subscriber.connect(subscriber_callback, CallbackModel.HLA_EVOKED)
            publisher.createFederationExecution(
                federation_name, str(fom_module), "HLAinteger64Time"
            )
            created = True
            publisher.joinFederationExecution("regional-association-publisher", federation_name)
            publisher_joined = True
            subscriber.joinFederationExecution("regional-association-subscriber", federation_name)
            subscriber_joined = True

            publisher_class = publisher.getObjectClassHandle("HLAobjectRoot.Food.Drink.Soda")
            publisher_attribute = publisher.getAttributeHandle(publisher_class, "Flavor")
            subscriber_class = subscriber.getObjectClassHandle("HLAobjectRoot.Food.Drink.Soda")
            subscriber_attribute = subscriber.getAttributeHandle(subscriber_class, "Flavor")
            publisher_dimension = publisher.getDimensionHandle("SodaFlavor")
            subscriber_dimension = subscriber.getDimensionHandle("SodaFlavor")
            publisher.publishObjectClassAttributes(
                publisher_class, AttributeHandleSet([publisher_attribute])
            )

            first_region = publisher.createRegion(DimensionHandleSet([publisher_dimension]))
            second_region = publisher.createRegion(DimensionHandleSet([publisher_dimension]))
            subscriber_first_region = subscriber.createRegion(
                DimensionHandleSet([subscriber_dimension])
            )
            subscriber_second_region = subscriber.createRegion(
                DimensionHandleSet([subscriber_dimension])
            )
            publisher.setRangeBounds(first_region, publisher_dimension, RangeBounds(0, 1))
            publisher.setRangeBounds(second_region, publisher_dimension, RangeBounds(3, 4))
            subscriber.setRangeBounds(
                subscriber_first_region, subscriber_dimension, RangeBounds(0, 1)
            )
            subscriber.setRangeBounds(
                subscriber_second_region, subscriber_dimension, RangeBounds(3, 4)
            )
            publisher.commitRegionModifications(RegionHandleSet([first_region, second_region]))
            subscriber.commitRegionModifications(
                RegionHandleSet([subscriber_first_region, subscriber_second_region])
            )

            first_pair = AttributeSetRegionSetPairList([
                AttributeSetRegionSetPair(
                    AttributeHandleSet([publisher_attribute]), RegionHandleSet([first_region])
                )
            ])
            second_pair = AttributeSetRegionSetPairList([
                AttributeSetRegionSetPair(
                    AttributeHandleSet([publisher_attribute]), RegionHandleSet([second_region])
                )
            ])
            subscriber_pair = AttributeSetRegionSetPairList([
                AttributeSetRegionSetPair(
                    AttributeHandleSet([subscriber_attribute]),
                    RegionHandleSet([subscriber_first_region, subscriber_second_region]),
                )
            ])
            subscriber.subscribeObjectClassAttributesWithRegions(subscriber_class, subscriber_pair)
            object_instance = publisher.registerObjectInstanceWithRegions(
                publisher_class, first_pair
            )
            for _ in range(6):
                subscriber.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(len(subscriber_callback.discoveries), 1)

            publisher.associateRegionsForUpdates(object_instance, second_pair)
            for _ in range(8):
                publisher.evokeMultipleCallbacks(0.0, 0.0)
                subscriber.evokeMultipleCallbacks(0.0, 0.0)

            publisher.updateAttributeValues(
                object_instance,
                AttributeHandleValueMap({publisher_attribute: b"both-regions"}),
            )
            for _ in range(8):
                publisher.evokeMultipleCallbacks(0.0, 0.0)
                subscriber.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(len(subscriber_callback.reflections), 1)

            publisher.unassociateRegionsForUpdates(object_instance, first_pair)
            publisher.updateAttributeValues(
                object_instance,
                AttributeHandleValueMap({publisher_attribute: b"second-region-remains"}),
            )
            for _ in range(8):
                publisher.evokeMultipleCallbacks(0.0, 0.0)
                subscriber.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(len(subscriber_callback.reflections), 2)
            self.assertEqual(
                subscriber_callback.reflections[-1][1][subscriber_attribute],
                b"second-region-remains",
            )

            publisher.unassociateRegionsForUpdates(object_instance, second_pair)
            for _ in range(8):
                publisher.evokeMultipleCallbacks(0.0, 0.0)
                subscriber.evokeMultipleCallbacks(0.0, 0.0)
            publisher.updateAttributeValues(
                object_instance,
                AttributeHandleValueMap({publisher_attribute: b"default-region"}),
            )
            for _ in range(8):
                publisher.evokeMultipleCallbacks(0.0, 0.0)
                subscriber.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(len(subscriber_callback.reflections), 3)
            self.assertEqual(subscriber_callback.out_of_scope, [])
        finally:
            if subscriber_joined:
                subscriber.resignFederationExecution(ResignAction.NO_ACTION)
            if publisher_joined:
                publisher.resignFederationExecution(ResignAction.NO_ACTION)
            if created:
                publisher.destroyFederationExecution(federation_name)
            subscriber.disconnect()
            publisher.disconnect()

    def test_native_regional_subscription_reprojects_existing_object(self) -> None:
        class RecordingFederateAmbassador(FederateAmbassador):
            def __init__(self) -> None:
                self.discoveries: list[tuple[object, ...]] = []
                self.reflections: list[tuple[object, ...]] = []

            def discoverObjectInstance(self, *arguments) -> None:
                self.discoveries.append(arguments)

            def reflectAttributeValues(self, *arguments) -> None:
                self.reflections.append(arguments)

        fom_module = (
            Path(__file__).parents[3]
            / "third_party"
            / "ieee1516.2-2025"
            / "resources"
            / "examples"
            / "RestaurantFOMmodule-2025.xml"
        )
        federation_name = f"python-native-regional-reprojection-{uuid4()}"
        publisher_callback = RecordingFederateAmbassador()
        subscriber_callback = RecordingFederateAmbassador()
        publisher = UmbraRtiFactory().getRtiAmbassador()
        subscriber = UmbraRtiFactory().getRtiAmbassador()
        created = publisher_joined = subscriber_joined = False
        try:
            publisher.connect(publisher_callback, CallbackModel.HLA_EVOKED)
            subscriber.connect(subscriber_callback, CallbackModel.HLA_EVOKED)
            publisher.createFederationExecution(
                federation_name, str(fom_module), "HLAinteger64Time"
            )
            created = True
            publisher.joinFederationExecution("regional-reprojection-publisher", federation_name)
            publisher_joined = True
            subscriber.joinFederationExecution("regional-reprojection-subscriber", federation_name)
            subscriber_joined = True

            publisher_class = publisher.getObjectClassHandle("HLAobjectRoot.Food.Drink.Soda")
            subscriber_class = subscriber.getObjectClassHandle("HLAobjectRoot.Food.Drink.Soda")
            publisher_attribute = publisher.getAttributeHandle(publisher_class, "Flavor")
            subscriber_attribute = subscriber.getAttributeHandle(subscriber_class, "Flavor")
            publisher.publishObjectClassAttributes(
                publisher_class, AttributeHandleSet([publisher_attribute])
            )
            publisher_dimension = publisher.getDimensionHandle("SodaFlavor")
            subscriber_dimension = subscriber.getDimensionHandle("SodaFlavor")
            publisher_region = publisher.createRegion(DimensionHandleSet([publisher_dimension]))
            disjoint_region = subscriber.createRegion(DimensionHandleSet([subscriber_dimension]))
            overlap_region = subscriber.createRegion(DimensionHandleSet([subscriber_dimension]))
            publisher.setRangeBounds(publisher_region, publisher_dimension, RangeBounds(0, 1))
            subscriber.setRangeBounds(disjoint_region, subscriber_dimension, RangeBounds(3, 4))
            subscriber.setRangeBounds(overlap_region, subscriber_dimension, RangeBounds(0, 2))
            publisher.commitRegionModifications(RegionHandleSet([publisher_region]))
            subscriber.commitRegionModifications(
                RegionHandleSet([disjoint_region, overlap_region])
            )

            subscriber.subscribeObjectClassAttributesWithRegions(
                subscriber_class,
                AttributeSetRegionSetPairList(
                    [
                        AttributeSetRegionSetPair(
                            AttributeHandleSet([subscriber_attribute]),
                            RegionHandleSet([disjoint_region]),
                        )
                    ]
                ),
            )
            object_instance = publisher.registerObjectInstanceWithRegions(
                publisher_class,
                AttributeSetRegionSetPairList(
                    [
                        AttributeSetRegionSetPair(
                            AttributeHandleSet([publisher_attribute]),
                            RegionHandleSet([publisher_region]),
                        )
                    ]
                ),
            )
            subscriber.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(subscriber_callback.discoveries, [])

            subscriber.subscribeObjectClassAttributesWithRegions(
                subscriber_class,
                AttributeSetRegionSetPairList(
                    [
                        AttributeSetRegionSetPair(
                            AttributeHandleSet([subscriber_attribute]),
                            RegionHandleSet([overlap_region]),
                        )
                    ]
                ),
            )
            subscriber.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(len(subscriber_callback.discoveries), 1)
            self.assertEqual(subscriber_callback.discoveries[0][0], object_instance)

            publisher.updateAttributeValues(
                object_instance,
                AttributeHandleValueMap({publisher_attribute: b"reprojected"}),
                b"reprojected-tag",
            )
            subscriber.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(len(subscriber_callback.reflections), 1)
            self.assertEqual(
                subscriber_callback.reflections[0][1][subscriber_attribute], b"reprojected"
            )
            self.assertEqual(subscriber_callback.reflections[0][2], b"reprojected-tag")
        finally:
            if subscriber_joined:
                subscriber.resignFederationExecution(ResignAction.NO_ACTION)
            if publisher_joined:
                publisher.resignFederationExecution(ResignAction.NO_ACTION)
            if created:
                publisher.destroyFederationExecution(federation_name)
            subscriber.disconnect()
            publisher.disconnect()

    def test_native_regional_object_pair_vectors_register_and_update(self) -> None:
        class RecordingFederateAmbassador(FederateAmbassador):
            def __init__(self) -> None:
                self.discoveries = []
                self.reflections = []
                self.provided = []
                self.in_scope = []
                self.out_of_scope = []
                self.updates_on = []
                self.updates_off = []
                self.attribute_transport_confirmations = []
                self.attribute_transport_reports = []
                self.interaction_transport_confirmations = []
                self.interaction_transport_reports = []

            def discoverObjectInstance(self, objectInstance, objectClass, objectInstanceName, producingFederate) -> None:
                self.discoveries.append((objectInstance, objectClass, objectInstanceName, producingFederate))

            def reflectAttributeValues(
                self, objectInstance, attributeValues, userSuppliedTag, transportationType, producingFederate
            ) -> None:
                self.reflections.append((objectInstance, attributeValues, userSuppliedTag))

            def provideAttributeValueUpdate(self, objectInstance, attributes, userSuppliedTag) -> None:
                self.provided.append((objectInstance, attributes, userSuppliedTag))

            def attributesInScope(self, objectInstance, attributes) -> None:
                self.in_scope.append((objectInstance, attributes))

            def attributesOutOfScope(self, objectInstance, attributes) -> None:
                self.out_of_scope.append((objectInstance, attributes))

            def turnUpdatesOnForObjectInstance(
                self, objectInstance, attributes, updateRateDesignator=None
            ) -> None:
                self.updates_on.append((objectInstance, attributes, updateRateDesignator))

            def turnUpdatesOffForObjectInstance(self, objectInstance, attributes) -> None:
                self.updates_off.append((objectInstance, attributes))

            def confirmAttributeTransportationTypeChange(
                self, objectInstance, attributes, transportationType
            ) -> None:
                self.attribute_transport_confirmations.append(
                    (objectInstance, attributes, transportationType)
                )

            def reportAttributeTransportationType(
                self, objectInstance, attribute, transportationType
            ) -> None:
                self.attribute_transport_reports.append(
                    (objectInstance, attribute, transportationType)
                )

            def confirmInteractionTransportationTypeChange(
                self, interactionClass, transportationType
            ) -> None:
                self.interaction_transport_confirmations.append(
                    (interactionClass, transportationType)
                )

            def reportInteractionTransportationType(
                self, federate, interactionClass, transportationType
            ) -> None:
                self.interaction_transport_reports.append(
                    (federate, interactionClass, transportationType)
                )

        fom_module = (
            Path(__file__).parents[3]
            / "third_party"
            / "ieee1516.2-2025"
            / "resources"
            / "examples"
            / "RestaurantFOMmodule-2025.xml"
        )
        federation_name = f"python-native-regional-object-{uuid4()}"
        publisher_callback = RecordingFederateAmbassador()
        subscriber_callback = RecordingFederateAmbassador()
        publisher = UmbraRtiFactory().getRtiAmbassador()
        subscriber = UmbraRtiFactory().getRtiAmbassador()
        publisher.connect(publisher_callback, CallbackModel.HLA_EVOKED)
        subscriber.connect(subscriber_callback, CallbackModel.HLA_EVOKED)
        created = publisher_joined = subscriber_joined = False
        try:
            publisher.createFederationExecution(federation_name, str(fom_module), "HLAinteger64Time")
            created = True
            publisher_federate = publisher.joinFederationExecution("regional-publisher", federation_name)
            publisher_joined = True
            subscriber.joinFederationExecution("regional-subscriber", federation_name)
            subscriber_joined = True
            publisher_class = publisher.getObjectClassHandle("HLAobjectRoot.Food.Drink.Soda")
            publisher_attribute = publisher.getAttributeHandle(publisher_class, "Flavor")
            subscriber_class = subscriber.getObjectClassHandle("HLAobjectRoot.Food.Drink.Soda")
            subscriber_attribute = subscriber.getAttributeHandle(subscriber_class, "Flavor")
            dimension = publisher.getDimensionHandle("SodaFlavor")
            publisher_region = publisher.createRegion(DimensionHandleSet([dimension]))
            subscriber_region = subscriber.createRegion(DimensionHandleSet([dimension]))
            publisher.setRangeBounds(publisher_region, dimension, RangeBounds(0, 1))
            subscriber.setRangeBounds(subscriber_region, dimension, RangeBounds(0, 1))
            publisher.commitRegionModifications(RegionHandleSet([publisher_region]))
            subscriber.commitRegionModifications(RegionHandleSet([subscriber_region]))
            publisher_pair = AttributeSetRegionSetPairList([
                AttributeSetRegionSetPair(
                    AttributeHandleSet([publisher_attribute]), RegionHandleSet([publisher_region])
                )
            ])
            subscriber_pair = AttributeSetRegionSetPairList([
                AttributeSetRegionSetPair(
                    AttributeHandleSet([subscriber_attribute]), RegionHandleSet([subscriber_region])
                )
            ])
            publisher.publishObjectClassAttributes(
                publisher_class, AttributeHandleSet([publisher_attribute])
            )
            subscriber.setAttributeScopeAdvisorySwitch(True)
            subscriber.subscribeObjectClassAttributesWithRegions(subscriber_class, subscriber_pair)
            registered = publisher.registerObjectInstanceWithRegions(publisher_class, publisher_pair)
            subscriber.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(len(subscriber_callback.discoveries), 1)
            discovered = subscriber_callback.discoveries[0][0]
            self.assertIsInstance(registered, ObjectInstanceHandle)
            self.assertEqual(discovered, registered)
            subscriber.unsubscribeObjectClassAttributesWithRegions(subscriber_class, subscriber_pair)
            subscriber.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(subscriber_callback.out_of_scope[-1][0], registered)
            subscriber.subscribeObjectClassAttributesWithRegions(subscriber_class, subscriber_pair)
            subscriber.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(subscriber_callback.in_scope[-1][0], registered)
            publisher.updateAttributeValues(registered, AttributeHandleValueMap({publisher_attribute: b"regional"}))
            subscriber.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(subscriber_callback.reflections[0][1][subscriber_attribute], b"regional")
            subscriber.requestAttributeValueUpdate(registered, AttributeHandleSet([subscriber_attribute]), b"instance-request")
            for _ in range(4):
                publisher.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(publisher_callback.provided[-1][0], registered)
            self.assertIn(b"instance-request", [entry[2] for entry in publisher_callback.provided], publisher_callback.provided)
            subscriber.requestAttributeValueUpdate(subscriber_class, AttributeHandleSet([subscriber_attribute]), b"class-request")
            for _ in range(4):
                publisher.evokeMultipleCallbacks(0.0, 0.0)
            self.assertIn(b"class-request", [entry[2] for entry in publisher_callback.provided], publisher_callback.provided)
            subscriber.requestAttributeValueUpdateWithRegions(subscriber_class, subscriber_pair, b"request")
            publisher.associateRegionsForUpdates(registered, publisher_pair)
            publisher.unassociateRegionsForUpdates(registered, publisher_pair)
            reliable = publisher.getTransportationTypeHandle("HLAreliable")
            best_effort = publisher.getTransportationTypeHandle("HLAbestEffort")
            publisher.changeAttributeOrderType(
                registered, AttributeHandleSet([publisher_attribute]), OrderType.TIMESTAMP
            )
            publisher.changeDefaultAttributeOrderType(
                publisher_class, AttributeHandleSet([publisher_attribute]), OrderType.RECEIVE
            )
            interaction = publisher.getInteractionClassHandle(
                "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed"
            )
            publisher.publishInteractionClass(interaction)
            publisher.changeInteractionOrderType(interaction, OrderType.RECEIVE)
            publisher.requestAttributeTransportationTypeChange(
                registered, AttributeHandleSet([publisher_attribute]), best_effort
            )
            for _ in range(4):
                publisher.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(publisher_callback.attribute_transport_confirmations[-1][0], registered)
            publisher.changeDefaultAttributeTransportationType(
                publisher_class, AttributeHandleSet([publisher_attribute]), reliable
            )
            publisher.queryAttributeTransportationType(registered, publisher_attribute)
            for _ in range(4):
                publisher.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(publisher_callback.attribute_transport_reports[-1][2], best_effort)
            publisher.requestInteractionTransportationTypeChange(interaction, best_effort)
            for _ in range(4):
                publisher.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(publisher_callback.interaction_transport_confirmations[-1][0], interaction)
            publisher.queryInteractionTransportationType(
                publisher_federate, interaction
            )
            for _ in range(4):
                publisher.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(publisher_callback.interaction_transport_reports[-1][1], interaction)
            subscriber.unsubscribeObjectClassAttributesWithRegions(subscriber_class, subscriber_pair)
            publisher.deleteObjectInstance(registered)
            publisher.deleteRegion(publisher_region)
            subscriber.deleteRegion(subscriber_region)
        finally:
            if subscriber_joined:
                subscriber.resignFederationExecution(ResignAction.NO_ACTION)
            if publisher_joined:
                publisher.resignFederationExecution(ResignAction.NO_ACTION)
            if created:
                publisher.destroyFederationExecution(federation_name)
            publisher.disconnect()
            subscriber.disconnect()

    def test_native_attribute_ownership_query_and_status_are_typed(self) -> None:
        class RecordingFederateAmbassador(FederateAmbassador):
            def __init__(self) -> None:
                self.informed = []

            def informAttributeOwnership(self, objectInstance, attributes, owner) -> None:
                self.informed.append((objectInstance, attributes, owner))

        fom_module = (
            Path(__file__).parents[3]
            / "third_party"
            / "ieee1516.2-2025"
            / "resources"
            / "examples"
            / "RestaurantFOMmodule-2025.xml"
        )
        federation_name = f"python-native-ownership-query-{uuid4()}"
        callback = RecordingFederateAmbassador()
        ambassador = UmbraRtiFactory().getRtiAmbassador()
        ambassador.connect(callback, CallbackModel.HLA_EVOKED)
        created = joined = False
        try:
            ambassador.createFederationExecution(federation_name, str(fom_module), "HLAinteger64Time")
            created = True
            ambassador.joinFederationExecution("ownership-query", federation_name)
            joined = True
            object_class = ambassador.getObjectClassHandle("HLAobjectRoot.Employee.Server")
            attribute = ambassador.getAttributeHandle(object_class, "Efficiency")
            ambassador.publishObjectClassAttributes(object_class, AttributeHandleSet([attribute]))
            instance = ambassador.registerObjectInstance(object_class)
            self.assertTrue(ambassador.isAttributeOwnedByFederate(instance, attribute))
            ambassador.queryAttributeOwnership(instance, AttributeHandleSet([attribute]))
            ambassador.evokeCallback(0.0)
            self.assertEqual(len(callback.informed), 1)
            self.assertEqual(callback.informed[0][0], instance)
            self.assertEqual(callback.informed[0][1], AttributeHandleSet([attribute]))
            self.assertIsInstance(callback.informed[0][2], FederateHandle)
            ambassador.unconditionalAttributeOwnershipDivestiture(
                instance, AttributeHandleSet([attribute]), b"divest"
            )
            self.assertFalse(ambassador.isAttributeOwnedByFederate(instance, attribute))
            ambassador.attributeOwnershipAcquisitionIfAvailable(
                instance, AttributeHandleSet([attribute]), b"acquire-if-available"
            )
            ambassador.evokeMultipleCallbacks(0.0, 0.0)
            self.assertIsInstance(
                ambassador.attributeOwnershipDivestitureIfWanted(
                    instance, AttributeHandleSet([attribute]), b"wanted"
                ),
                AttributeHandleSet,
            )
            ambassador.negotiatedAttributeOwnershipDivestiture(
                instance, AttributeHandleSet([attribute]), b"negotiated"
            )
            ambassador.cancelNegotiatedAttributeOwnershipDivestiture(
                instance, AttributeHandleSet([attribute])
            )
            ambassador.unconditionalAttributeOwnershipDivestiture(
                instance, AttributeHandleSet([attribute]), b"divest"
            )
            self.assertFalse(ambassador.isAttributeOwnedByFederate(instance, attribute))
            ambassador.deleteObjectInstance(instance)
        finally:
            if joined:
                ambassador.resignFederationExecution(ResignAction.NO_ACTION)
            if created:
                ambassador.destroyFederationExecution(federation_name)
            ambassador.disconnect()

    def test_native_two_member_ownership_transfer_callbacks_are_sequenced(self) -> None:
        class RecordingFederateAmbassador(FederateAmbassador):
            def __init__(self) -> None:
                self.divestiture_requests = []
                self.acquisitions = []
                self.unavailable = []

            def requestDivestitureConfirmation(self, objectInstance, attributes, userSuppliedTag) -> None:
                self.divestiture_requests.append((objectInstance, attributes, userSuppliedTag))

            def attributeOwnershipAcquisitionNotification(
                self, objectInstance, attributes, userSuppliedTag
            ) -> None:
                self.acquisitions.append((objectInstance, attributes, userSuppliedTag))

            def attributeOwnershipUnavailable(self, objectInstance, attributes, userSuppliedTag) -> None:
                self.unavailable.append((objectInstance, attributes, userSuppliedTag))

        fom_module = (
            Path(__file__).parents[3]
            / "third_party"
            / "ieee1516.2-2025"
            / "resources"
            / "examples"
            / "RestaurantFOMmodule-2025.xml"
        )
        federation_name = f"python-native-ownership-transfer-{uuid4()}"
        owner_callback = RecordingFederateAmbassador()
        acquirer_callback = RecordingFederateAmbassador()
        owner = UmbraRtiFactory().getRtiAmbassador()
        acquirer = UmbraRtiFactory().getRtiAmbassador()
        owner.connect(owner_callback, CallbackModel.HLA_EVOKED)
        acquirer.connect(acquirer_callback, CallbackModel.HLA_EVOKED)
        created = owner_joined = acquirer_joined = False
        object_instance = None
        try:
            owner.createFederationExecution(federation_name, str(fom_module), "HLAinteger64Time")
            created = True
            owner.joinFederationExecution("ownership-owner", federation_name)
            owner_joined = True
            acquirer.joinFederationExecution("ownership-acquirer", federation_name)
            acquirer_joined = True
            owner_class = owner.getObjectClassHandle("HLAobjectRoot.Employee.Server")
            acquirer_class = acquirer.getObjectClassHandle("HLAobjectRoot.Employee.Server")
            owner_attribute = owner.getAttributeHandle(owner_class, "Efficiency")
            acquirer_attribute = acquirer.getAttributeHandle(acquirer_class, "Efficiency")
            owner_attributes = AttributeHandleSet([owner_attribute])
            acquirer_attributes = AttributeHandleSet([acquirer_attribute])
            owner.publishObjectClassAttributes(owner_class, owner_attributes)
            acquirer.publishObjectClassAttributes(acquirer_class, acquirer_attributes)
            acquirer.subscribeObjectClassAttributes(acquirer_class, acquirer_attributes)
            object_instance = owner.registerObjectInstance(owner_class)
            for _ in range(8):
                acquirer.evokeMultipleCallbacks(0.0, 0.0)

            owner.negotiatedAttributeOwnershipDivestiture(
                object_instance, owner_attributes, b"offer"
            )
            acquirer.attributeOwnershipAcquisition(
                object_instance, acquirer_attributes, b"acquire"
            )
            for _ in range(8):
                if owner_callback.divestiture_requests:
                    break
                owner.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(len(owner_callback.divestiture_requests), 1)
            self.assertEqual(owner_callback.divestiture_requests[0][2], b"acquire")
            self.assertFalse(acquirer_callback.acquisitions)

            owner.confirmDivestiture(
                object_instance, owner_attributes, b"confirm"
            )
            for _ in range(8):
                if acquirer_callback.acquisitions:
                    break
                acquirer.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(len(acquirer_callback.acquisitions), 1)
            self.assertEqual(acquirer_callback.acquisitions[0][2], b"confirm")
            self.assertFalse(owner.isAttributeOwnedByFederate(object_instance, owner_attribute))
            self.assertTrue(
                acquirer.isAttributeOwnedByFederate(object_instance, acquirer_attribute)
            )
        finally:
            if acquirer_joined and object_instance is not None:
                acquirer.unconditionalAttributeOwnershipDivestiture(
                    object_instance, acquirer_attributes, b"cleanup"
                )
            if acquirer_joined:
                acquirer.resignFederationExecution(ResignAction.NO_ACTION)
            if owner_joined:
                owner.resignFederationExecution(ResignAction.NO_ACTION)
            if created:
                owner.destroyFederationExecution(federation_name)
            acquirer.disconnect()
            owner.disconnect()

    def test_native_synchronization_registration_converts_the_tag_and_callbacks(self) -> None:
        class RecordingFederateAmbassador(FederateAmbassador):
            def __init__(self) -> None:
                self.registrations: list[str] = []
                self.announcements: list[tuple[str, bytes]] = []
                self.synchronized: list[tuple[str, FederateHandleSet]] = []
                self.registration_failures: list[tuple[str, SynchronizationPointFailureReason]] = []

            def synchronizationPointRegistrationSucceeded(self, synchronizationPointLabel) -> None:
                self.registrations.append(synchronizationPointLabel)

            def announceSynchronizationPoint(self, synchronizationPointLabel, userSuppliedTag) -> None:
                self.announcements.append((synchronizationPointLabel, userSuppliedTag))

            def federationSynchronized(self, synchronizationPointLabel, failedToSyncSet) -> None:
                self.synchronized.append((synchronizationPointLabel, failedToSyncSet))

            def synchronizationPointRegistrationFailed(self, synchronizationPointLabel, reason) -> None:
                self.registration_failures.append((synchronizationPointLabel, reason))

        fom_module = (
            Path(__file__).parents[3]
            / "third_party"
            / "ieee1516.2-2025"
            / "resources"
            / "examples"
            / "RestaurantFOMmodule-2025.xml"
        )
        federation_name = f"python-native-sync-{uuid4()}"
        label = f"python-sync-{uuid4()}"
        tag = b"\x00python-sync-tag\xff"
        callback = RecordingFederateAmbassador()
        ambassador = UmbraRtiFactory().getRtiAmbassador()
        ambassador.connect(callback, CallbackModel.HLA_EVOKED)
        created = joined = False
        try:
            ambassador.createFederationExecution(federation_name, str(fom_module), "HLAinteger64Time")
            created = True
            ambassador.joinFederationExecution("observer", federation_name)
            joined = True
            ambassador.registerFederationSynchronizationPoint(label, tag)
            ambassador.evokeMultipleCallbacks(0.0, 0.1)
            ambassador.evokeMultipleCallbacks(0.0, 0.1)
            self.assertEqual(callback.registrations, [label])
            self.assertEqual(callback.announcements, [(label, tag)])
            ambassador.registerFederationSynchronizationPoint(label, tag)
            ambassador.evokeMultipleCallbacks(0.0, 0.1)
            self.assertEqual(
                callback.registration_failures,
                [(label, SynchronizationPointFailureReason.SYNCHRONIZATION_POINT_LABEL_NOT_UNIQUE)],
            )
            ambassador.synchronizationPointAchieved(label)
            ambassador.evokeMultipleCallbacks(0.0, 0.1)
            self.assertEqual(len(callback.synchronized), 1)
            self.assertEqual(callback.synchronized[0][0], label)
            self.assertEqual(len(callback.synchronized[0][1]), 0)
            failed_label = f"{label}-failed"
            ambassador.registerFederationSynchronizationPoint(failed_label, b"failed-tag")
            ambassador.evokeMultipleCallbacks(0.0, 0.1)
            ambassador.evokeMultipleCallbacks(0.0, 0.1)
            ambassador.synchronizationPointAchieved(failed_label, successfully=False)
            ambassador.evokeMultipleCallbacks(0.0, 0.1)
            self.assertEqual(callback.synchronized[-1][0], failed_label)
            self.assertIsInstance(callback.synchronized[-1][1], FederateHandleSet)
            self.assertEqual(len(callback.synchronized[-1][1]), 1)
            self.assertGreater(next(iter(callback.synchronized[-1][1])).encodedLength(), 0)
        finally:
            if joined:
                ambassador.resignFederationExecution(ResignAction.NO_ACTION)
            if created:
                ambassador.destroyFederationExecution(federation_name)
            ambassador.disconnect()

    def test_native_query_federation_save_status_reports_typed_member_entries(self) -> None:
        class RecordingFederateAmbassador(FederateAmbassador):
            def __init__(self) -> None:
                self.save_status_reports: list[tuple[FederateHandleSaveStatusPair, ...]] = []
                self.save_initiations: list[str] = []
                self.save_completions = 0
                self.save_failures: list[SaveFailureReason] = []
                self.restore_status_reports: list[tuple[FederateRestoreStatus, ...]] = []
                self.restore_accepted: list[str] = []
                self.restore_rejected: list[str] = []
                self.restore_begun = 0
                self.restore_initiations: list[tuple[str, str, FederateHandle]] = []
                self.restore_completions = 0
                self.restore_failures: list[RestoreFailureReason] = []

            def federationSaveStatusResponse(self, response) -> None:
                self.save_status_reports.append(response)

            def initiateFederateSave(self, label) -> None:
                self.save_initiations.append(label)

            def federationSaved(self) -> None:
                self.save_completions += 1

            def federationNotSaved(self, reason) -> None:
                self.save_failures.append(reason)

            def federationRestoreStatusResponse(self, response) -> None:
                self.restore_status_reports.append(response)

            def requestFederationRestoreSucceeded(self, label) -> None:
                self.restore_accepted.append(label)

            def requestFederationRestoreFailed(self, label) -> None:
                self.restore_rejected.append(label)

            def federationRestoreBegun(self) -> None:
                self.restore_begun += 1

            def initiateFederateRestore(self, label, federateName, postRestoreFederateHandle) -> None:
                self.restore_initiations.append((label, federateName, postRestoreFederateHandle))

            def federationRestored(self) -> None:
                self.restore_completions += 1

            def federationNotRestored(self, reason) -> None:
                self.restore_failures.append(reason)

        fom_module = (
            Path(__file__).parents[3]
            / "third_party"
            / "ieee1516.2-2025"
            / "resources"
            / "examples"
            / "RestaurantFOMmodule-2025.xml"
        )
        federation_name = f"python-native-save-status-{uuid4()}"
        owner_callback = RecordingFederateAmbassador()
        peer_callback = RecordingFederateAmbassador()
        owner = UmbraRtiFactory().getRtiAmbassador()
        peer = UmbraRtiFactory().getRtiAmbassador()
        owner.connect(owner_callback, CallbackModel.HLA_IMMEDIATE)
        peer.connect(peer_callback, CallbackModel.HLA_IMMEDIATE)
        created = owner_joined = peer_joined = False
        try:
            owner.createFederationExecution(federation_name, str(fom_module), "HLAinteger64Time")
            created = True
            owner.joinFederationExecution("owner", federation_name, federateName="python-save-owner")
            owner_joined = True
            peer.joinFederationExecution("observer", federation_name, federateName="python-save-peer")
            peer_joined = True
            owner.queryFederationSaveStatus()
            self.assertEqual(len(owner_callback.save_status_reports), 1)
            report = owner_callback.save_status_reports[0]
            self.assertEqual(len(report), 2)
            self.assertTrue(all(isinstance(entry, FederateHandleSaveStatusPair) for entry in report))
            self.assertEqual({entry.saveStatus for entry in report}, {SaveStatus.NO_SAVE_IN_PROGRESS})
            self.assertTrue(all(entry.federateHandle.encodedLength() > 0 for entry in report))
            save_label = f"python-save-{uuid4()}"
            owner.requestFederationSave(save_label)
            self.assertEqual(owner_callback.save_initiations, [save_label])
            self.assertEqual(peer_callback.save_initiations, [save_label])
            owner.federateSaveBegun()
            peer.federateSaveBegun()
            owner.queryFederationSaveStatus()
            saving_report = owner_callback.save_status_reports[-1]
            self.assertEqual(
                {entry.saveStatus for entry in saving_report},
                {SaveStatus.FEDERATE_SAVING},
            )
            owner.federateSaveComplete()
            self.assertEqual(owner_callback.save_completions, 0)
            peer.federateSaveComplete()
            self.assertEqual(owner_callback.save_completions, 1)
            self.assertEqual(peer_callback.save_completions, 1)
            owner.queryFederationSaveStatus()
            active_save_report = owner_callback.save_status_reports[-1]
            self.assertEqual(
                {entry.saveStatus for entry in active_save_report},
                {SaveStatus.NO_SAVE_IN_PROGRESS},
            )
            failed_save_label = f"python-save-fail-{uuid4()}"
            owner.requestFederationSave(failed_save_label)
            owner.federateSaveBegun()
            peer.federateSaveBegun()
            peer.federateSaveNotComplete()
            self.assertEqual(
                owner_callback.save_failures,
                [SaveFailureReason.FEDERATE_REPORTED_FAILURE_DURING_SAVE],
            )
            self.assertEqual(
                peer_callback.save_failures,
                [SaveFailureReason.FEDERATE_REPORTED_FAILURE_DURING_SAVE],
            )
            aborted_save_label = f"python-save-abort-{uuid4()}"
            owner.requestFederationSave(aborted_save_label)
            owner.abortFederationSave()
            self.assertEqual(owner_callback.save_failures[-1], SaveFailureReason.SAVE_ABORTED)
            self.assertEqual(peer_callback.save_failures[-1], SaveFailureReason.SAVE_ABORTED)
            owner.queryFederationRestoreStatus()
            restore_report = owner_callback.restore_status_reports[-1]
            self.assertEqual(len(restore_report), 2)
            self.assertTrue(all(isinstance(entry, FederateRestoreStatus) for entry in restore_report))
            self.assertEqual({entry.status for entry in restore_report}, {RestoreStatus.NO_RESTORE_IN_PROGRESS})
            self.assertTrue(all(entry.preRestoreHandle.encodedLength() > 0 for entry in restore_report))
            self.assertTrue(
                all(
                    isinstance(entry.preRestoreHandle, FederateHandle)
                    and isinstance(entry.postRestoreHandle, FederateHandle)
                    for entry in restore_report
                )
            )
            missing_restore_label = f"python-missing-restore-{uuid4()}"
            owner.requestFederationRestore(missing_restore_label)
            self.assertEqual(owner_callback.restore_rejected, [missing_restore_label])
            owner.requestFederationRestore(save_label)
            self.assertEqual(owner_callback.restore_accepted, [save_label])
            self.assertEqual(owner_callback.restore_begun, 1)
            self.assertEqual(peer_callback.restore_begun, 1)
            self.assertEqual(owner_callback.restore_initiations[0][0], save_label)
            self.assertEqual(owner_callback.restore_initiations[0][1], "python-save-owner")
            self.assertGreater(owner_callback.restore_initiations[0][2].encodedLength(), 0)
            self.assertEqual(peer_callback.restore_initiations[0][1], "python-save-peer")
            owner.queryFederationRestoreStatus()
            restoring_report = owner_callback.restore_status_reports[-1]
            self.assertEqual(
                {entry.status for entry in restoring_report},
                {RestoreStatus.FEDERATE_RESTORING},
            )
            owner.federateRestoreComplete()
            self.assertEqual(owner_callback.restore_completions, 0)
            peer.federateRestoreComplete()
            self.assertEqual(owner_callback.restore_completions, 1)
            self.assertEqual(peer_callback.restore_completions, 1)
            owner.queryFederationRestoreStatus()
            completed_restore_report = owner_callback.restore_status_reports[-1]
            self.assertEqual(
                {entry.status for entry in completed_restore_report},
                {RestoreStatus.NO_RESTORE_IN_PROGRESS},
            )
            failed_restore_label = f"python-restore-fail-{uuid4()}"
            owner.requestFederationRestore(failed_restore_label)
            self.assertEqual(owner_callback.restore_rejected[-1], failed_restore_label)
            owner.requestFederationRestore(save_label)
            peer.federateRestoreNotComplete()
            self.assertEqual(
                owner_callback.restore_failures,
                [RestoreFailureReason.FEDERATE_REPORTED_FAILURE_DURING_RESTORE],
            )
            self.assertEqual(
                peer_callback.restore_failures,
                [RestoreFailureReason.FEDERATE_REPORTED_FAILURE_DURING_RESTORE],
            )
            owner.requestFederationRestore(save_label)
            owner.abortFederationRestore()
            self.assertEqual(owner_callback.restore_failures[-1], RestoreFailureReason.RESTORE_ABORTED)
            self.assertEqual(peer_callback.restore_failures[-1], RestoreFailureReason.RESTORE_ABORTED)
        finally:
            if peer_joined:
                peer.resignFederationExecution(ResignAction.NO_ACTION)
            if owner_joined:
                owner.resignFederationExecution(ResignAction.NO_ACTION)
            if created:
                owner.destroyFederationExecution(federation_name)
            peer.disconnect()
            owner.disconnect()

    def test_native_timestamped_federation_save_reaches_cpp_overload(self) -> None:
        class RecordingFederateAmbassador(FederateAmbassador):
            def __init__(self) -> None:
                self.save_initiations: list[str] = []

            def initiateFederateSave(self, label: str) -> None:
                self.save_initiations.append(label)

        fom_module = (
            Path(__file__).parents[3]
            / "third_party"
            / "ieee1516.2-2025"
            / "resources"
            / "examples"
            / "RestaurantFOMmodule-2025.xml"
        )
        federation_name = f"python-native-timed-save-{uuid4()}"
        callback = RecordingFederateAmbassador()
        ambassador = UmbraRtiFactory().getRtiAmbassador()
        connected = created = joined = False
        try:
            ambassador.connect(callback, CallbackModel.HLA_IMMEDIATE)
            connected = True
            ambassador.createFederationExecution(
                federation_name, str(fom_module), "HLAinteger64Time"
            )
            created = True
            ambassador.joinFederationExecution("timed-save-owner", federation_name)
            joined = True
            time_factory = ambassador.getTimeFactory()
            ambassador.enableTimeRegulation(time_factory.makeLogicalTimeInterval(1))
            ambassador.requestFederationSave(
                "python-timed-save", time_factory.makeLogicalTime(1)
            )
            self.assertEqual(callback.save_initiations, ["python-timed-save"])
        finally:
            if joined:
                ambassador.resignFederationExecution(ResignAction.NO_ACTION)
            if created:
                ambassador.destroyFederationExecution(federation_name)
            if connected:
                ambassador.disconnect()

    def test_native_restore_rewinds_saved_logical_time_and_lookahead(self) -> None:
        class RecordingFederateAmbassador(FederateAmbassador):
            def __init__(self) -> None:
                self.saved = 0
                self.restored = 0
                self.regulations: list[object] = []
                self.grants: list[object] = []

            def federationSaved(self) -> None:
                self.saved += 1

            def federationRestored(self) -> None:
                self.restored += 1

            def timeRegulationEnabled(self, time) -> None:
                self.regulations.append(time)

            def timeAdvanceGrant(self, time) -> None:
                self.grants.append(time)

        fom_module = (
            Path(__file__).parents[3]
            / "third_party"
            / "ieee1516.2-2025"
            / "resources"
            / "examples"
            / "RestaurantFOMmodule-2025.xml"
        )
        federation_name = f"python-native-restore-time-window-{uuid4()}"
        callback = RecordingFederateAmbassador()
        ambassador = UmbraRtiFactory().getRtiAmbassador()
        created = joined = False
        try:
            ambassador.connect(callback, CallbackModel.HLA_EVOKED)
            ambassador.createFederationExecution(
                federation_name, str(fom_module), "HLAinteger64Time"
            )
            created = True
            ambassador.joinFederationExecution("restore-time-window", federation_name)
            joined = True
            time_factory = ambassador.getTimeFactory()
            ambassador.enableTimeRegulation(time_factory.makeLogicalTimeInterval(2))
            for _ in range(4):
                ambassador.evokeMultipleCallbacks(0.0, 0.0)
            ambassador.timeAdvanceRequest(time_factory.makeLogicalTime(3))
            for _ in range(4):
                ambassador.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(ambassador.queryLogicalTime().getTime(), 3)
            self.assertEqual(ambassador.queryLookahead().getInterval(), 2)

            save_label = f"native-time-window-{uuid4()}"
            ambassador.requestFederationSave(save_label)
            for _ in range(4):
                ambassador.evokeMultipleCallbacks(0.0, 0.0)
            ambassador.federateSaveBegun()
            ambassador.federateSaveComplete()
            for _ in range(4):
                ambassador.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(callback.saved, 1)

            ambassador.modifyLookahead(time_factory.makeLogicalTimeInterval(5))
            ambassador.timeAdvanceRequest(time_factory.makeLogicalTime(5))
            for _ in range(4):
                ambassador.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(ambassador.queryLogicalTime().getTime(), 5)
            self.assertEqual(ambassador.queryLookahead().getInterval(), 5)

            ambassador.requestFederationRestore(save_label)
            for _ in range(8):
                ambassador.evokeMultipleCallbacks(0.0, 0.0)
            ambassador.federateRestoreComplete()
            for _ in range(4):
                ambassador.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(callback.restored, 1)
            self.assertEqual(ambassador.queryLogicalTime().getTime(), 3)
            self.assertEqual(ambassador.queryLookahead().getInterval(), 2)
        finally:
            if joined:
                ambassador.resignFederationExecution(ResignAction.NO_ACTION)
            if created:
                ambassador.destroyFederationExecution(federation_name)
            ambassador.disconnect()

    def test_native_restore_preserves_deferred_lookahead_decrease(self) -> None:
        class RecordingFederateAmbassador(FederateAmbassador):
            def __init__(self) -> None:
                self.saved = 0
                self.restored = 0
                self.regulations: list[object] = []
                self.grants: list[object] = []

            def federationSaved(self) -> None:
                self.saved += 1

            def federationRestored(self) -> None:
                self.restored += 1

            def timeRegulationEnabled(self, time) -> None:
                self.regulations.append(time)

            def timeAdvanceGrant(self, time) -> None:
                self.grants.append(time)

        fom_module = (
            Path(__file__).parents[3]
            / "third_party"
            / "ieee1516.2-2025"
            / "resources"
            / "examples"
            / "RestaurantFOMmodule-2025.xml"
        )
        federation_name = f"python-native-restore-deferred-lookahead-{uuid4()}"
        callback = RecordingFederateAmbassador()
        ambassador = UmbraRtiFactory().getRtiAmbassador()
        created = joined = False
        try:
            ambassador.connect(callback, CallbackModel.HLA_EVOKED)
            ambassador.createFederationExecution(
                federation_name, str(fom_module), "HLAinteger64Time"
            )
            created = True
            ambassador.joinFederationExecution("restore-deferred-lookahead", federation_name)
            joined = True
            time_factory = ambassador.getTimeFactory()
            ambassador.enableTimeRegulation(time_factory.makeLogicalTimeInterval(5))
            for _ in range(4):
                ambassador.evokeMultipleCallbacks(0.0, 0.0)

            ambassador.modifyLookahead(time_factory.makeLogicalTimeInterval(1))
            self.assertEqual(ambassador.queryLookahead().getInterval(), 5)
            save_label = f"native-deferred-lookahead-{uuid4()}"
            ambassador.requestFederationSave(save_label)
            for _ in range(4):
                ambassador.evokeMultipleCallbacks(0.0, 0.0)
            ambassador.federateSaveBegun()
            ambassador.federateSaveComplete()
            for _ in range(4):
                ambassador.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(callback.saved, 1)

            ambassador.timeAdvanceRequest(time_factory.makeLogicalTime(3))
            for _ in range(4):
                ambassador.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(ambassador.queryLogicalTime().getTime(), 3)
            self.assertEqual(ambassador.queryLookahead().getInterval(), 2)

            ambassador.requestFederationRestore(save_label)
            for _ in range(8):
                ambassador.evokeMultipleCallbacks(0.0, 0.0)
            ambassador.federateRestoreComplete()
            for _ in range(4):
                ambassador.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(callback.restored, 1)
            self.assertEqual(ambassador.queryLogicalTime().getTime(), 0)
            self.assertEqual(ambassador.queryLookahead().getInterval(), 5)

            ambassador.timeAdvanceRequest(time_factory.makeLogicalTime(3))
            for _ in range(4):
                ambassador.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(ambassador.queryLogicalTime().getTime(), 3)
            self.assertEqual(ambassador.queryLookahead().getInterval(), 2)
        finally:
            if joined:
                ambassador.resignFederationExecution(ResignAction.NO_ACTION)
            if created:
                ambassador.destroyFederationExecution(federation_name)
            ambassador.disconnect()

    def test_native_timestamped_save_waits_for_constrained_grant_boundary(self) -> None:
        class RecordingFederateAmbassador(FederateAmbassador):
            def __init__(self, name: str) -> None:
                self.name = name
                self.events: list[tuple[str, object]] = []

            def initiateFederateSave(self, label: str) -> None:
                self.events.append(("initiate", label))

            def timeAdvanceGrant(self, time) -> None:
                self.events.append(("grant", time.getTime()))

            def timeRegulationEnabled(self, time) -> None:
                self.events.append(("regulation", time.getTime()))

            def timeConstrainedEnabled(self, time) -> None:
                self.events.append(("constrained", time.getTime()))

            def federationSaved(self) -> None:
                self.events.append(("saved", self.name))

        fom_module = (
            Path(__file__).parents[3]
            / "third_party"
            / "ieee1516.2-2025"
            / "resources"
            / "examples"
            / "RestaurantFOMmodule-2025.xml"
        )
        federation_name = f"python-native-timed-save-boundary-{uuid4()}"
        owner_callback = RecordingFederateAmbassador("owner")
        peer_callback = RecordingFederateAmbassador("peer")
        owner = UmbraRtiFactory().getRtiAmbassador()
        peer = UmbraRtiFactory().getRtiAmbassador()
        created = owner_joined = peer_joined = False
        try:
            owner.connect(owner_callback, CallbackModel.HLA_EVOKED)
            peer.connect(peer_callback, CallbackModel.HLA_EVOKED)
            owner.createFederationExecution(
                federation_name, str(fom_module), "HLAinteger64Time"
            )
            created = True
            owner.joinFederationExecution("timed-save-owner", federation_name)
            owner_joined = True
            peer.joinFederationExecution("timed-save-peer", federation_name)
            peer_joined = True

            owner_time_factory = owner.getTimeFactory()
            peer_time_factory = peer.getTimeFactory()
            lookahead = owner_time_factory.makeLogicalTimeInterval(1)
            save_time = owner_time_factory.makeLogicalTime(5)
            owner.enableTimeRegulation(lookahead)
            peer.enableTimeConstrained()
            for ambassador in (owner, peer):
                ambassador.evokeMultipleCallbacks(0.0, 0.0)
            self.assertIn(("regulation", 0), owner_callback.events)
            self.assertIn(("constrained", 0), peer_callback.events)

            owner.requestFederationSave("python-timed-save-boundary", save_time)
            self.assertNotIn(
                ("initiate", "python-timed-save-boundary"), owner_callback.events
            )
            self.assertNotIn(
                ("initiate", "python-timed-save-boundary"), peer_callback.events
            )

            # The regulating federate can schedule the boundary, but the save
            # cannot begin until the constrained member has also requested a
            # qualifying grant.
            owner.timeAdvanceRequest(save_time)
            owner.evokeMultipleCallbacks(0.0, 0.0)
            peer.evokeMultipleCallbacks(0.0, 0.0)
            self.assertNotIn(
                ("initiate", "python-timed-save-boundary"), owner_callback.events
            )
            self.assertNotIn(
                ("initiate", "python-timed-save-boundary"), peer_callback.events
            )

            peer.timeAdvanceRequest(peer_time_factory.makeLogicalTime(5))
            for _ in range(4):
                owner.evokeMultipleCallbacks(0.0, 0.0)
                peer.evokeMultipleCallbacks(0.0, 0.0)

            self.assertIn(("initiate", "python-timed-save-boundary"), owner_callback.events)
            # IEEE's direct timed-save admission callback is made while the
            # constrained federate is still advancing.  The non-constrained
            # requester may receive its ordinary grant before the federation
            # broadcasts the final initiation to that requester.
            peer_initiate_index = peer_callback.events.index(
                ("initiate", "python-timed-save-boundary")
            )
            peer_grant_index = peer_callback.events.index(("grant", 5))
            self.assertLess(peer_initiate_index, peer_grant_index)

            owner.federateSaveBegun()
            peer.federateSaveBegun()
            owner.federateSaveComplete()
            peer.federateSaveComplete()
            for _ in range(2):
                owner.evokeMultipleCallbacks(0.0, 0.0)
                peer.evokeMultipleCallbacks(0.0, 0.0)
            self.assertIn(("saved", "owner"), owner_callback.events)
            self.assertIn(("saved", "peer"), peer_callback.events)
        finally:
            if peer_joined:
                peer.resignFederationExecution(ResignAction.NO_ACTION)
            if owner_joined:
                owner.resignFederationExecution(ResignAction.NO_ACTION)
            if created:
                owner.destroyFederationExecution(federation_name)
            peer.disconnect()
            owner.disconnect()


class NativeConnectionFoundationConformanceTest(
    ConnectionFoundationConformanceMixin,
    ConnectionOverloadConformanceMixin,
    FederationExecutionDiscoveryConformanceMixin,
    FederationExecutionMemberDiscoveryConformanceMixin,
    unittest.TestCase,
):
    def make_factory(self) -> UmbraRtiFactory:
        return UmbraRtiFactory()
