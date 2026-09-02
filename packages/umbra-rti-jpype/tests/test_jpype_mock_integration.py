from __future__ import annotations

import importlib.util
import math
import os
from pathlib import Path
import struct
import tempfile
import unittest

from hla.rti1516_2025 import (
    AttributeHandle,
    AttributeHandleSet,
    AttributeHandleValueMap,
    AttributeSetRegionSetPair,
    AttributeSetRegionSetPairList,
    CallbackModel,
    DecoderException,
    EncoderException,
    DataElementFactory,
    DimensionHandle,
    DimensionHandleSet,
    EncoderFactory,
    HLAfixedArray,
    HLAfixedRecord,
    HLAvariantRecord,
    FederateAmbassador,
    FederationExecutionInformation,
    FederationExecutionInformationSet,
    FederationExecutionMemberInformationSet,
    FederateHandle,
    FederateHandleSet,
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
    HLAinteger64Interval,
    HLAinteger64Time,
    HLAinteger64TimeFactory,
    HLAfloat64Interval,
    HLAfloat64Time,
    HLAfloat64TimeFactory,
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
    InteractionClassHandle,
    InteractionClassHandleSet,
    MessageRetractionHandle,
    MutableAttributeHandleSet,
    MutableAttributeHandleValueMap,
    MutableDimensionHandleSet,
    MutableFederateHandleSet,
    MutableParameterHandleValueMap,
    MutableRegionHandleSet,
    ObjectClassHandle,
    ObjectInstanceHandle,
    ObjectInstanceNameSet,
    OrderType,
    ParameterHandle,
    ParameterHandleValueMap,
    RangeBounds,
    RegionHandle,
    RegionHandleSet,
    ResignAction,
    SaveFailureReason,
    SaveStatus,
    RestoreFailureReason,
    ServiceGroup,
    TransportationTypeHandle,
)
from hla.rti1516_2025.exceptions import (
    AlreadyConnected,
    CouldNotDecode,
    IllegalTimeArithmetic,
    InvalidLogicalTime,
    InvalidLogicalTimeInterval,
)
from umbra_rti_test_support import (
    HLA_FIXTURES,
    HLA_FOM,
    HLA_MOM,
    HLA_TYPES,
    SurfaceEventTrace,
    assert_event_order,
    iter_surface_matrix,
)
from umbra._java.rti1516_2025 import JavaProviderConfiguration, JavaRtiFactory

from _mock_java_fixture import build_mock_java_rti, java_toolchain_available


JPYPE_AVAILABLE = importlib.util.find_spec("jpype") is not None


class _Integer32ElementFactory(DataElementFactory):
    def __init__(self, encoder: EncoderFactory) -> None:
        self.encoder = encoder

    def createElement(self, index: int):
        return self.encoder.createHLAinteger32BE()


class _RecordingFederateAmbassador(FederateAmbassador):
    def __init__(self) -> None:
        self.connection_losses: list[str] = []
        self.federation_execution_reports: list[FederationExecutionInformationSet] = []
        self.federation_execution_member_reports: list[
            tuple[str, FederationExecutionMemberInformationSet]
        ] = []
        self.missing_federation_executions: list[str] = []
        self.resignations: list[str] = []
        self.sync_registration: list[str] = []
        self.sync_announcements: list[tuple[str, bytes]] = []
        self.sync_completions: list[tuple[str, FederateHandleSet]] = []
        self.restore_accepted: list[str] = []
        self.restore_rejected: list[str] = []
        self.restore_begun = 0
        self.restore_initiations: list[tuple[str, str, FederateHandle]] = []
        self.restore_completions = 0
        self.restore_failures: list[RestoreFailureReason] = []
        self.save_status_reports: list[tuple[object, ...]] = []
        self.save_initiations: list[str] = []
        self.save_completions = 0
        self.save_failures: list[SaveFailureReason] = []
        self.restore_status_reports: list[tuple[object, ...]] = []
        self.start_registration: list[ObjectClassHandle] = []
        self.stop_registration: list[ObjectClassHandle] = []
        self.interactions_on: list[InteractionClassHandle] = []
        self.interactions_off: list[InteractionClassHandle] = []
        self.discoveries: list[
            tuple[ObjectInstanceHandle, ObjectClassHandle, str, FederateHandle]
        ] = []
        self.reserved_names: list[str] = []
        self.rejected_reservations: list[str] = []
        self.attribute_value_requests: list[tuple[ObjectInstanceHandle, AttributeHandleSet, bytes]] = []
        self.scope_entries: list[tuple[ObjectInstanceHandle, AttributeHandleSet]] = []
        self.scope_exits: list[tuple[ObjectInstanceHandle, AttributeHandleSet]] = []
        self.updates_on: list[tuple[ObjectInstanceHandle, AttributeHandleSet, str | None]] = []
        self.updates_off: list[tuple[ObjectInstanceHandle, AttributeHandleSet]] = []
        self.attribute_transport_confirmations: list[tuple[ObjectInstanceHandle, AttributeHandleSet, TransportationTypeHandle]] = []
        self.attribute_transport_reports: list[tuple[ObjectInstanceHandle, AttributeHandle, TransportationTypeHandle]] = []
        self.interaction_transport_confirmations: list[tuple[InteractionClassHandle, TransportationTypeHandle]] = []
        self.interaction_transport_reports: list[tuple[FederateHandle, InteractionClassHandle, TransportationTypeHandle]] = []
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
        self.timed_reflections: list[tuple[object, ...]] = []
        self.interactions: list[
            tuple[
                InteractionClassHandle,
                ParameterHandleValueMap,
                bytes,
                TransportationTypeHandle,
                FederateHandle,
            ]
        ] = []
        self.timed_interactions: list[tuple[object, ...]] = []
        self.timed_removals: list[tuple[object, ...]] = []
        self.sent_regions: list[RegionHandleSet] = []
        self.ownership_reports: list[tuple[ObjectInstanceHandle, AttributeHandleSet, FederateHandle]] = []
        self.ownership_acquisitions: list[
            tuple[ObjectInstanceHandle, AttributeHandleSet, bytes]
        ] = []
        self.ownership_acquisition_cancellations: list[
            tuple[ObjectInstanceHandle, AttributeHandleSet]
        ] = []
        self.ownership_divestiture_requests: list[
            tuple[ObjectInstanceHandle, AttributeHandleSet, bytes]
        ] = []
        self.ownership_unavailable: list[
            tuple[ObjectInstanceHandle, AttributeHandleSet, bytes]
        ] = []
        self.ownership_not_owned: list[tuple[ObjectInstanceHandle, AttributeHandleSet]] = []
        self.ownership_owned_by_rti: list[tuple[ObjectInstanceHandle, AttributeHandleSet]] = []
        self.time_regulation: list[HLAinteger64Time] = []
        self.time_constrained: list[HLAinteger64Time] = []
        self.time_grants: list[HLAinteger64Time] = []
        self.flush_grants: list[tuple[HLAinteger64Time, HLAinteger64Time]] = []
        self.request_retractions: list[MessageRetractionHandle] = []
        self.directed_interactions: list[tuple[object, ...]] = []
        self.timed_directed_interactions: list[tuple[object, ...]] = []
        self.name_batch_successes: list[ObjectInstanceNameSet] = []
        self.name_batch_failures: list[ObjectInstanceNameSet] = []

    def connectionLost(self, faultDescription: str) -> None:
        self.connection_losses.append(faultDescription)

    def startRegistrationForObjectClass(self, objectClass: ObjectClassHandle) -> None:
        self.start_registration.append(objectClass)

    def stopRegistrationForObjectClass(self, objectClass: ObjectClassHandle) -> None:
        self.stop_registration.append(objectClass)

    def turnInteractionsOn(self, interactionClass: InteractionClassHandle) -> None:
        self.interactions_on.append(interactionClass)

    def turnInteractionsOff(self, interactionClass: InteractionClassHandle) -> None:
        self.interactions_off.append(interactionClass)

    def discoverObjectInstance(
        self,
        objectInstance: ObjectInstanceHandle,
        objectClass: ObjectClassHandle,
        objectInstanceName: str,
        producingFederate: FederateHandle,
    ) -> None:
        self.discoveries.append(
            (objectInstance, objectClass, objectInstanceName, producingFederate)
        )

    def objectInstanceNameReservationSucceeded(self, objectInstanceName: str) -> None:
        self.reserved_names.append(objectInstanceName)

    def objectInstanceNameReservationFailed(self, objectInstanceName: str) -> None:
        self.rejected_reservations.append(objectInstanceName)

    def provideAttributeValueUpdate(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
        userSuppliedTag: bytes,
    ) -> None:
        self.attribute_value_requests.append((objectInstance, attributes, userSuppliedTag))

    def attributesInScope(self, objectInstance, attributes) -> None:
        self.scope_entries.append((objectInstance, attributes))

    def attributesOutOfScope(self, objectInstance, attributes) -> None:
        self.scope_exits.append((objectInstance, attributes))

    def turnUpdatesOnForObjectInstance(self, objectInstance, attributes, updateRateDesignator=None) -> None:
        self.updates_on.append((objectInstance, attributes, updateRateDesignator))

    def turnUpdatesOffForObjectInstance(self, objectInstance, attributes) -> None:
        self.updates_off.append((objectInstance, attributes))

    def confirmAttributeTransportationTypeChange(self, objectInstance, attributes, transportationType) -> None:
        self.attribute_transport_confirmations.append((objectInstance, attributes, transportationType))

    def reportAttributeTransportationType(self, objectInstance, attribute, transportationType) -> None:
        self.attribute_transport_reports.append((objectInstance, attribute, transportationType))

    def confirmInteractionTransportationTypeChange(self, interactionClass, transportationType) -> None:
        self.interaction_transport_confirmations.append((interactionClass, transportationType))

    def reportInteractionTransportationType(self, federate, interactionClass, transportationType) -> None:
        self.interaction_transport_reports.append((federate, interactionClass, transportationType))

    def removeObjectInstance(
        self,
        objectInstance: ObjectInstanceHandle,
        userSuppliedTag: bytes,
        producingFederate: FederateHandle,
        time: object | None = None,
        sentOrderType: OrderType | None = None,
        receivedOrderType: OrderType | None = None,
        retraction: MessageRetractionHandle | None = None,
    ) -> None:
        if time is not None:
            self.timed_removals.append(
                (objectInstance, userSuppliedTag, producingFederate, time, sentOrderType, receivedOrderType, retraction)
            )
        else:
            self.removals.append((objectInstance, userSuppliedTag, producingFederate))

    def reflectAttributeValues(
        self,
        objectInstance: ObjectInstanceHandle,
        attributeValues: AttributeHandleValueMap,
        userSuppliedTag: bytes,
        transportationType: TransportationTypeHandle,
        producingFederate: FederateHandle,
        sentRegions: RegionHandleSet | None = None,
        time: object | None = None,
        sentOrderType: OrderType | None = None,
        receivedOrderType: OrderType | None = None,
        retraction: MessageRetractionHandle | None = None,
    ) -> None:
        if time is not None:
            self.timed_reflections.append(
                (objectInstance, attributeValues, userSuppliedTag, transportationType,
                 producingFederate, sentRegions, time, sentOrderType, receivedOrderType, retraction)
            )
        else:
            self.reflections.append(
                (objectInstance, attributeValues, userSuppliedTag, transportationType, producingFederate)
            )

    def receiveInteraction(
        self,
        interactionClass: InteractionClassHandle,
        parameterValues: ParameterHandleValueMap,
        userSuppliedTag: bytes,
        transportationType: TransportationTypeHandle,
        producingFederate: FederateHandle,
        sentRegions: RegionHandleSet | None = None,
        time: object | None = None,
        sentOrderType: OrderType | None = None,
        receivedOrderType: OrderType | None = None,
        retraction: MessageRetractionHandle | None = None,
    ) -> None:
        if time is not None:
            self.timed_interactions.append(
                (interactionClass, parameterValues, userSuppliedTag, transportationType,
                 producingFederate, sentRegions, time, sentOrderType, receivedOrderType, retraction)
            )
        else:
            self.interactions.append(
                (interactionClass, parameterValues, userSuppliedTag, transportationType, producingFederate)
            )
        if sentRegions is not None:
            self.sent_regions.append(sentRegions)

    def receiveDirectedInteraction(
        self,
        interactionClass: InteractionClassHandle,
        objectInstance: ObjectInstanceHandle,
        parameterValues: ParameterHandleValueMap,
        userSuppliedTag: bytes,
        transportationType: TransportationTypeHandle,
        producingFederate: FederateHandle,
        time: object | None = None,
        sentOrderType: OrderType | None = None,
        receivedOrderType: OrderType | None = None,
        retraction: MessageRetractionHandle | None = None,
    ) -> None:
        entry = (
            interactionClass,
            objectInstance,
            parameterValues,
            userSuppliedTag,
            transportationType,
            producingFederate,
        )
        if time is None:
            self.directed_interactions.append(entry)
        else:
            self.timed_directed_interactions.append(
                entry + (time, sentOrderType, receivedOrderType, retraction)
            )

    def multipleObjectInstanceNameReservationSucceeded(self, objectInstanceNames) -> None:
        self.name_batch_successes.append(objectInstanceNames)

    def multipleObjectInstanceNameReservationFailed(self, objectInstanceNames) -> None:
        self.name_batch_failures.append(objectInstanceNames)

    def informAttributeOwnership(
        self, objectInstance: ObjectInstanceHandle, attributes: AttributeHandleSet, owner: FederateHandle
    ) -> None:
        self.ownership_reports.append((objectInstance, attributes, owner))

    def attributeOwnershipAcquisitionNotification(
        self,
        objectInstance: ObjectInstanceHandle,
        securedAttributes: AttributeHandleSet,
        userSuppliedTag: bytes,
    ) -> None:
        self.ownership_acquisitions.append((objectInstance, securedAttributes, userSuppliedTag))

    def confirmAttributeOwnershipAcquisitionCancellation(
        self, objectInstance: ObjectInstanceHandle, attributes: AttributeHandleSet
    ) -> None:
        self.ownership_acquisition_cancellations.append((objectInstance, attributes))

    def requestDivestitureConfirmation(
        self,
        objectInstance: ObjectInstanceHandle,
        releasedAttributes: AttributeHandleSet,
        userSuppliedTag: bytes,
    ) -> None:
        self.ownership_divestiture_requests.append(
            (objectInstance, releasedAttributes, userSuppliedTag)
        )

    def attributeOwnershipUnavailable(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
        userSuppliedTag: bytes,
    ) -> None:
        self.ownership_unavailable.append((objectInstance, attributes, userSuppliedTag))

    def attributeIsNotOwned(
        self, objectInstance: ObjectInstanceHandle, attributes: AttributeHandleSet
    ) -> None:
        self.ownership_not_owned.append((objectInstance, attributes))

    def attributeIsOwnedByRTI(
        self, objectInstance: ObjectInstanceHandle, attributes: AttributeHandleSet
    ) -> None:
        self.ownership_owned_by_rti.append((objectInstance, attributes))

    def timeRegulationEnabled(self, time: HLAinteger64Time) -> None:
        self.time_regulation.append(time)

    def timeConstrainedEnabled(self, time: HLAinteger64Time) -> None:
        self.time_constrained.append(time)

    def flushQueueGrant(self, time: HLAinteger64Time, optimisticTime: HLAinteger64Time) -> None:
        self.flush_grants.append((time, optimisticTime))

    def timeAdvanceGrant(self, time: HLAinteger64Time) -> None:
        self.time_grants.append(time)

    def requestRetraction(self, retraction: MessageRetractionHandle) -> None:
        self.request_retractions.append(retraction)

    def reportFederationExecutions(self, report: FederationExecutionInformationSet) -> None:
        self.federation_execution_reports.append(report)

    def reportFederationExecutionMembers(
        self,
        federationExecutionName: str,
        report: FederationExecutionMemberInformationSet,
    ) -> None:
        self.federation_execution_member_reports.append((federationExecutionName, report))

    def reportFederationExecutionDoesNotExist(self, federationExecutionName: str) -> None:
        self.missing_federation_executions.append(federationExecutionName)

    def federateResigned(self, reasonForResignDescription: str) -> None:
        self.resignations.append(reasonForResignDescription)

    def synchronizationPointRegistrationSucceeded(self, label: str) -> None:
        self.sync_registration.append(label)

    def announceSynchronizationPoint(self, label: str, tag: bytes) -> None:
        self.sync_announcements.append((label, tag))

    def federationSynchronized(self, label: str, failedToSyncSet: FederateHandleSet) -> None:
        self.sync_completions.append((label, failedToSyncSet))

    def federationSaveStatusResponse(self, response: tuple[object, ...]) -> None:
        self.save_status_reports.append(response)

    def initiateFederateSave(self, label: str, time: object | None = None) -> None:
        self.save_initiations.append(label)

    def federationSaved(self) -> None:
        self.save_completions += 1

    def federationNotSaved(self, reason: SaveFailureReason) -> None:
        self.save_failures.append(reason)

    def federationRestoreStatusResponse(self, response: tuple[object, ...]) -> None:
        self.restore_status_reports.append(response)

    def requestFederationRestoreSucceeded(self, label: str) -> None:
        self.restore_accepted.append(label)

    def requestFederationRestoreFailed(self, label: str) -> None:
        self.restore_rejected.append(label)

    def federationRestoreBegun(self) -> None:
        self.restore_begun += 1

    def initiateFederateRestore(
        self, label: str, federateName: str, postRestoreFederateHandle: FederateHandle
    ) -> None:
        self.restore_initiations.append((label, federateName, postRestoreFederateHandle))

    def federationRestored(self) -> None:
        self.restore_completions += 1

    def federationNotRestored(self, reason: RestoreFailureReason) -> None:
        self.restore_failures.append(reason)


class _MatrixFederateAmbassador(_RecordingFederateAmbassador):
    """Record the cross-federate callback sequence without narrowing types."""

    def __init__(self, trace: SurfaceEventTrace) -> None:
        super().__init__()
        self.trace = trace

    def timeRegulationEnabled(self, time: object) -> None:
        self.trace.record("timeRegulationEnabled")
        super().timeRegulationEnabled(time)  # type: ignore[arg-type]

    def timeConstrainedEnabled(self, time: object) -> None:
        self.trace.record("timeConstrainedEnabled")
        super().timeConstrainedEnabled(time)  # type: ignore[arg-type]

    def timeAdvanceGrant(self, time: object) -> None:
        self.trace.record("timeAdvanceGrant")
        super().timeAdvanceGrant(time)  # type: ignore[arg-type]

    def discoverObjectInstance(self, *args: object) -> None:
        self.trace.record("discoverObjectInstance")
        super().discoverObjectInstance(*args)  # type: ignore[arg-type]

    def reflectAttributeValues(self, *args: object) -> None:
        self.trace.record("reflectAttributeValues")
        super().reflectAttributeValues(*args)  # type: ignore[arg-type]

    def receiveInteraction(self, *args: object) -> None:
        self.trace.record("receiveInteraction")
        super().receiveInteraction(*args)  # type: ignore[arg-type]

    def initiateFederateSave(self, label: str, time: object | None = None) -> None:
        self.trace.record("initiateFederateSave")
        super().initiateFederateSave(label, time)  # type: ignore[arg-type]

    def federationSaved(self) -> None:
        self.trace.record("federationSaved")
        super().federationSaved()

    def federationNotSaved(self, reason: SaveFailureReason) -> None:
        self.trace.record("federationNotSaved")
        super().federationNotSaved(reason)

    def requestFederationRestoreSucceeded(self, label: str) -> None:
        self.trace.record("requestFederationRestoreSucceeded")
        super().requestFederationRestoreSucceeded(label)

    def federationRestoreBegun(self) -> None:
        self.trace.record("federationRestoreBegun")
        super().federationRestoreBegun()

    def initiateFederateRestore(
        self, label: str, federateName: str, postRestoreFederateHandle: FederateHandle
    ) -> None:
        self.trace.record("initiateFederateRestore")
        super().initiateFederateRestore(label, federateName, postRestoreFederateHandle)

    def federationRestored(self) -> None:
        self.trace.record("federationRestored")
        super().federationRestored()

    def federationNotRestored(self, reason: RestoreFailureReason) -> None:
        self.trace.record("federationNotRestored")
        super().federationNotRestored(reason)


@unittest.skipUnless(JPYPE_AVAILABLE and java_toolchain_available(), "requires JPype and a JDK")
class JPypeMockIntegrationTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        import jpype

        if jpype.isJVMStarted():
            raise unittest.SkipTest("the fixture classpath must be present before the JVM starts")
        # JPype does not permit a safe JVM restart, so leave this temporary
        # directory for process cleanup after the one integration JVM exits.
        cls._temporary_directory = Path(tempfile.mkdtemp(prefix="umbra-mock-java-rti-"))
        jar_path = build_mock_java_rti(cls._temporary_directory)
        cls._jar_path = jar_path
        cls.factory = JavaRtiFactory(
            JavaProviderConfiguration(
                classpath=(str(jar_path),),
                rti_factory_name="Umbra Mock Java RTI",
            )
        )

    def test_real_jvm_service_loader_and_java_to_python_callback(self) -> None:
        ambassador = self.factory.getRtiAmbassador()
        callbacks = _RecordingFederateAmbassador()

        result = ambassador.connect(callbacks, CallbackModel.HLA_EVOKED)

        self.assertEqual(self.factory.rtiName(), "Umbra Mock Java RTI")
        self.assertEqual(self.factory.rtiVersion(), "2025.mock")
        self.assertFalse(result.configurationUsed)
        self.assertEqual(result.message, "connected through the mock Java RTI using HLA_EVOKED")

        encoder = self.factory.getEncoderFactory()
        # The provider-scoped Java façade exposes the creator declared by the
        # standard 2025 EncoderFactory.  The mock implementation predates
        # that method, so this assertion checks the façade surface without
        # claiming the fixture implements the optional creator.
        self.assertTrue(hasattr(encoder, "createHLAextendableVariantRecord"))
        integer16 = encoder.createHLAinteger16BE(-1234)
        float64 = encoder.createHLAfloat64BE(math.pi)
        float32 = encoder.createHLAfloat32BE(math.pi)
        float32le = encoder.createHLAfloat32LE(math.pi)
        integer32le = encoder.createHLAinteger32LE(-0x1234567)
        float64le = encoder.createHLAfloat64LE(math.pi)
        unsigned16 = encoder.createHLAunsignedInteger16BE(0xABCD)
        unsigned16le = encoder.createHLAunsignedInteger16LE(0xABCD)
        integer16le = encoder.createHLAinteger16LE(-0x1234)
        unsigned32le = encoder.createHLAunsignedInteger32LE(0x89ABCDEF)
        integer64 = encoder.createHLAinteger64BE(-0x123456789AB)
        integer64le = encoder.createHLAinteger64LE(-0x123456789AB)
        unsigned64 = encoder.createHLAunsignedInteger64BE(0xFEDCBA9876543210)
        unsigned64le = encoder.createHLAunsignedInteger64LE(0xFEDCBA9876543210)
        integer = encoder.createHLAinteger32BE(-2)
        unsigned = encoder.createHLAunsignedInteger32BE(0x1234ABCD)
        unsigned_maximum = encoder.createHLAunsignedInteger32BE(0xFFFFFFFF)
        boolean = encoder.createHLAboolean(True)
        unicode = encoder.createHLAunicodeString("A😀")
        byte = encoder.createHLAbyte(0xA5)
        octet = encoder.createHLAoctet(0x5A)
        ascii_char = encoder.createHLAASCIIchar(0x41)
        ascii_string = encoder.createHLAASCIIstring("A!?")
        unicode_char = encoder.createHLAunicodeChar(0x03A9)
        pair_be = encoder.createHLAoctetPairBE(0x1234)
        pair_le = encoder.createHLAoctetPairLE(0x1234)
        opaque = encoder.createHLAopaqueData(b"\x00\xA5\xFF")
        self.assertIsInstance(encoder, EncoderFactory)
        self.assertIsInstance(integer, HLAinteger32BE)
        self.assertIsInstance(integer16, HLAinteger16BE)
        self.assertIsInstance(float64, HLAfloat64BE)
        self.assertIsInstance(float32, HLAfloat32BE)
        self.assertIsInstance(float32le, HLAfloat32LE)
        self.assertIsInstance(integer32le, HLAinteger32LE)
        self.assertIsInstance(float64le, HLAfloat64LE)
        self.assertIsInstance(unsigned16, HLAunsignedInteger16BE)
        self.assertIsInstance(unsigned16le, HLAunsignedInteger16LE)
        self.assertIsInstance(integer16le, HLAinteger16LE)
        self.assertIsInstance(unsigned32le, HLAunsignedInteger32LE)
        self.assertIsInstance(integer64, HLAinteger64BE)
        self.assertIsInstance(integer64le, HLAinteger64LE)
        self.assertIsInstance(unsigned64, HLAunsignedInteger64BE)
        self.assertIsInstance(unsigned64le, HLAunsignedInteger64LE)
        self.assertIsInstance(unsigned, HLAunsignedInteger32BE)
        self.assertIsInstance(boolean, HLAboolean)
        self.assertIsInstance(unicode, HLAunicodeString)
        self.assertIsInstance(byte, HLAbyte)
        self.assertIsInstance(octet, HLAoctet)
        self.assertIsInstance(ascii_char, HLAASCIIchar)
        self.assertIsInstance(ascii_string, HLAASCIIstring)
        self.assertIsInstance(unicode_char, HLAunicodeChar)
        self.assertIsInstance(pair_be, HLAoctetPairBE)
        self.assertIsInstance(pair_le, HLAoctetPairLE)
        self.assertIsInstance(opaque, HLAopaqueData)
        self.assertEqual(integer.toByteArray(), b"\xff\xff\xff\xfe")
        self.assertEqual(opaque.toByteArray(), b"\x00\x00\x00\x03\x00\xA5\xFF")
        self.assertEqual(opaque.getOctetBoundary(), 4)
        self.assertEqual(opaque.size(), 3)
        self.assertEqual([opaque.get(index) for index in range(3)], [0, 0xA5, 0xFF])
        self.assertEqual(opaque.getValue(), b"\x00\xA5\xFF")
        self.assertIs(opaque.decode(b"\x00\x00\x00\x02OK"), opaque)
        self.assertEqual(opaque.getValue(), b"OK")
        with self.assertRaises(DecoderException):
            opaque.decode(b"\x00\x00\x00\x02A")
        integer_factory = _Integer32ElementFactory(encoder)
        variable_array = encoder.createHLAvariableArray(integer_factory)
        variable_array.addElement(encoder.createHLAinteger32BE(1))
        variable_array.addElement(encoder.createHLAinteger32BE(-2))
        decoded_array = encoder.createHLAvariableArray(integer_factory)
        decoded_array.decode(variable_array.toByteArray())
        self.assertIsInstance(variable_array, HLAvariableArray)
        self.assertEqual(variable_array.toByteArray(), b"\x00\x00\x00\x02\x00\x00\x00\x01\xff\xff\xff\xfe")
        self.assertEqual(variable_array.getOctetBoundary(), 4)
        self.assertEqual(variable_array.size(), 2)
        self.assertEqual([element.getValue() for element in variable_array], [1, -2])
        self.assertEqual([decoded_array.get(index).getValue() for index in range(2)], [1, -2])
        with self.assertRaises(EncoderException):
            variable_array.addElement(encoder.createHLAoctet(1))
        with self.assertRaises(IndexError):
            variable_array.get(2)
        with self.assertRaises(DecoderException):
            encoder.createHLAvariableArray(integer_factory).decode(b"\x80\x00\x00\x00")
        initial_array = encoder.createHLAvariableArray(
            integer_factory,
            encoder.createHLAinteger32BE(3),
            encoder.createHLAinteger32BE(4),
        )
        self.assertEqual([element.getValue() for element in initial_array], [3, 4])
        fixed_array = encoder.createHLAfixedArray(integer_factory, 2)
        fixed_array.set(0, encoder.createHLAinteger32BE(1))
        fixed_array.set(1, encoder.createHLAinteger32BE(-2))
        decoded_fixed = encoder.createHLAfixedArray(integer_factory, 2)
        decoded_fixed.decode(fixed_array.toByteArray())
        self.assertIsInstance(fixed_array, HLAfixedArray)
        self.assertEqual(fixed_array.toByteArray(), b"\x00\x00\x00\x01\xff\xff\xff\xfe")
        self.assertEqual(fixed_array.size(), 2)
        self.assertEqual([element.getValue() for element in fixed_array], [1, -2])
        self.assertEqual([decoded_fixed.get(index).getValue() for index in range(2)], [1, -2])
        ascii_factory = type(
            "_AsciiStringFactory",
            (DataElementFactory,),
            {"createElement": lambda self, index: encoder.createHLAASCIIstring()},
        )()
        padded_fixed = encoder.createHLAfixedArray(ascii_factory, 2)
        padded_fixed.set(0, encoder.createHLAASCIIstring("A"))
        padded_fixed.set(1, encoder.createHLAASCIIstring("B"))
        padded_bytes = b"\x00\x00\x00\x01A" + b"\0\0\0" + b"\x00\x00\x00\x01B"
        self.assertEqual(padded_fixed.toByteArray(), padded_bytes)
        self.assertEqual(padded_fixed.getEncodedLength(), len(padded_bytes))
        bad_fixed_padding = bytearray(padded_bytes)
        bad_fixed_padding[5] = 1
        with self.assertRaises(DecoderException):
            encoder.createHLAfixedArray(ascii_factory, 2).decode(bad_fixed_padding)
        with self.assertRaises(EncoderException):
            fixed_array.set(0, encoder.createHLAoctet(1))
        with self.assertRaises(IndexError):
            fixed_array.get(2)
        with self.assertRaises(ValueError):
            encoder.createHLAfixedArray(integer_factory, -1)
        with self.assertRaises(DecoderException):
            encoder.createHLAfixedArray(integer_factory, 2).decode(
                b"\x00\x00\x00\x01\xff\xff\xff\xfe\x00"
            )
        record = encoder.createHLAfixedRecord()
        first = encoder.createHLAinteger32BE(0x10203040)
        middle = encoder.createHLAoctet(0xA5)
        last = encoder.createHLAinteger32BE(-2)
        record.appendElement(first)
        record.appendElement(middle)
        record.appendElement(last)
        first.setValue(99)
        middle.setValue(0x11)
        last.setValue(7)
        record_bytes = b"\x10\x20\x30\x40\xA5" + b"\0\0\0" + b"\xff\xff\xff\xfe"
        self.assertIsInstance(record, HLAfixedRecord)
        self.assertEqual(record.toByteArray(), record_bytes)
        self.assertEqual(record.getEncodedLength(), len(record_bytes))
        self.assertEqual(record.getOctetBoundary(), 4)
        self.assertEqual(record.size(), 3)
        self.assertEqual([element.getValue() for element in record], [0x10203040, 0xA5, -2])
        decoded_record = encoder.createHLAfixedRecord()
        decoded_record.appendElement(encoder.createHLAinteger32BE())
        decoded_record.appendElement(encoder.createHLAoctet())
        decoded_record.appendElement(encoder.createHLAinteger32BE())
        decoded_record.decode(record_bytes)
        self.assertEqual(
            [decoded_record.get(index).getValue() for index in range(3)],
            [0x10203040, 0xA5, -2],
        )
        record.set(1, encoder.createHLAoctet(0x5A))
        self.assertEqual(
            record.toByteArray(),
            b"\x10\x20\x30\x40\x5A" + b"\0\0\0" + b"\xff\xff\xff\xfe",
        )
        with self.assertRaises(EncoderException):
            record.set(1, encoder.createHLAinteger32BE(1))
        with self.assertRaises(IndexError):
            record.get(3)
        bad_record_padding = bytearray(record_bytes)
        bad_record_padding[5] = 1
        with self.assertRaises(DecoderException):
            decoded_record.decode(bad_record_padding)
        with self.assertRaises(DecoderException):
            decoded_record.decode(record_bytes + b"\0")
        nested = encoder.createHLAfixedRecord()
        nested.appendElement(encoder.createHLAinteger32BE(0x01020304))
        nested.appendElement(encoder.createHLAoctet(0xA5))
        outer = encoder.createHLAfixedRecord()
        outer.appendElement(nested)
        outer.appendElement(encoder.createHLAinteger32BE(-2))
        nested.set(0, encoder.createHLAinteger32BE(99))
        nested_bytes = b"\x01\x02\x03\x04\xA5"
        outer_bytes = nested_bytes + b"\0\0\0" + b"\xff\xff\xff\xfe"
        self.assertIsInstance(outer.get(0), HLAfixedRecord)
        self.assertEqual(outer.toByteArray(), outer_bytes)
        decoded_outer = encoder.createHLAfixedRecord()
        decoded_nested = encoder.createHLAfixedRecord()
        decoded_nested.appendElement(encoder.createHLAinteger32BE())
        decoded_nested.appendElement(encoder.createHLAoctet())
        decoded_outer.appendElement(decoded_nested)
        decoded_outer.appendElement(encoder.createHLAinteger32BE())
        decoded_outer.decode(outer_bytes)
        self.assertEqual(decoded_outer.get(0).get(0).getValue(), 0x01020304)
        self.assertEqual(decoded_outer.get(0).get(1).getValue(), 0xA5)
        self.assertEqual(decoded_outer.get(1).getValue(), -2)
        nested_array = encoder.createHLAfixedArray(integer_factory, 2)
        nested_array.set(0, encoder.createHLAinteger32BE(11))
        nested_array.set(1, encoder.createHLAinteger32BE(-12))
        array_outer = encoder.createHLAfixedRecord()
        array_outer.appendElement(nested_array)
        array_outer.appendElement(encoder.createHLAinteger32BE(-2))
        nested_array.set(0, encoder.createHLAinteger32BE(99))
        array_outer_bytes = b"\x00\x00\x00\x0b\xff\xff\xff\xf4" + b"\xff\xff\xff\xfe"
        self.assertIsInstance(array_outer.get(0), HLAfixedArray)
        self.assertEqual(array_outer.toByteArray(), array_outer_bytes)
        decoded_array_outer = encoder.createHLAfixedRecord()
        decoded_nested_array = encoder.createHLAfixedArray(integer_factory, 2)
        decoded_array_outer.appendElement(decoded_nested_array)
        decoded_array_outer.appendElement(encoder.createHLAinteger32BE())
        decoded_array_outer.decode(array_outer_bytes)
        self.assertEqual([decoded_array_outer.get(0).get(i).getValue() for i in range(2)], [11, -12])
        self.assertEqual(decoded_array_outer.get(1).getValue(), -2)
        nested_variable_factory = type(
            "_NestedIntegerFactory",
            (DataElementFactory,),
            {"createElement": lambda self, index: encoder.createHLAinteger32BE()},
        )()
        nested_variable = encoder.createHLAvariableArray(nested_variable_factory)
        nested_variable.addElement(encoder.createHLAinteger32BE(11))
        nested_variable.addElement(encoder.createHLAinteger32BE(-12))
        variable_outer = encoder.createHLAfixedRecord()
        variable_outer.appendElement(nested_variable)
        variable_outer.appendElement(encoder.createHLAinteger32BE(-2))
        nested_variable.addElement(encoder.createHLAinteger32BE(99))
        nested_variable_bytes = b"\0\0\0\x02\0\0\0\x0b\xff\xff\xff\xf4"
        variable_outer_bytes = nested_variable_bytes + b"\xff\xff\xff\xfe"
        self.assertIsInstance(variable_outer.get(0), HLAvariableArray)
        self.assertEqual(variable_outer.toByteArray(), variable_outer_bytes)
        decoded_variable_outer = encoder.createHLAfixedRecord()
        decoded_nested_variable = encoder.createHLAvariableArray(nested_variable_factory)
        decoded_variable_outer.appendElement(decoded_nested_variable)
        decoded_variable_outer.appendElement(encoder.createHLAinteger32BE())
        decoded_variable_outer.decode(variable_outer_bytes)
        self.assertEqual(decoded_variable_outer.get(0).size(), 2)
        self.assertEqual([decoded_variable_outer.get(0).get(i).getValue() for i in range(2)], [11, -12])
        self.assertEqual(decoded_variable_outer.get(1).getValue(), -2)
        nested_variant_value = encoder.createHLAfixedRecord()
        nested_variant_value.appendElement(encoder.createHLAinteger32BE(0x01020304))
        nested_variant_value.appendElement(encoder.createHLAoctet(0xA5))
        nested_variant = encoder.createHLAvariantRecord(encoder.createHLAoctet())
        nested_variant.setVariant(encoder.createHLAoctet(3), nested_variant_value)
        nested_variant_value.set(0, encoder.createHLAinteger32BE(99))
        nested_variant_bytes = b"\x03\0\0\0\x01\x02\x03\x04\xA5"
        self.assertIsInstance(nested_variant.getValue(), HLAfixedRecord)
        self.assertEqual(nested_variant.toByteArray(), nested_variant_bytes)
        decoded_nested_variant = encoder.createHLAvariantRecord(encoder.createHLAoctet())
        decoded_nested_variant_value = encoder.createHLAfixedRecord()
        decoded_nested_variant_value.appendElement(encoder.createHLAinteger32BE())
        decoded_nested_variant_value.appendElement(encoder.createHLAoctet())
        decoded_nested_variant.setVariant(encoder.createHLAoctet(3), decoded_nested_variant_value)
        decoded_nested_variant.decode(nested_variant_bytes)
        self.assertEqual(decoded_nested_variant.getValue().get(0).getValue(), 0x01020304)
        composite_discriminant = encoder.createHLAfixedRecord()
        composite_discriminant.appendElement(encoder.createHLAinteger32BE(0x01020304))
        composite_discriminant.appendElement(encoder.createHLAoctet(0xA5))
        composite_variant = encoder.createHLAvariantRecord(composite_discriminant)
        composite_variant.setVariant(composite_discriminant, encoder.createHLAinteger32BE(7))
        composite_discriminant.set(0, encoder.createHLAinteger32BE(99))
        composite_variant_bytes = b"\x01\x02\x03\x04\xA5" + b"\0\0\0" + b"\0\0\0\x07"
        self.assertIsInstance(composite_variant.getDiscriminant(), HLAfixedRecord)
        self.assertEqual(composite_variant.toByteArray(), composite_variant_bytes)
        decoded_composite_discriminant = encoder.createHLAfixedRecord()
        decoded_composite_discriminant.appendElement(encoder.createHLAinteger32BE())
        decoded_composite_discriminant.appendElement(encoder.createHLAoctet())
        decoded_composite_variant = encoder.createHLAvariantRecord(decoded_composite_discriminant)
        mapped_composite_discriminant = encoder.createHLAfixedRecord()
        mapped_composite_discriminant.appendElement(encoder.createHLAinteger32BE(0x01020304))
        mapped_composite_discriminant.appendElement(encoder.createHLAoctet(0xA5))
        decoded_composite_variant.setVariant(mapped_composite_discriminant, encoder.createHLAinteger32BE())
        decoded_composite_variant.decode(composite_variant_bytes)
        self.assertEqual(decoded_composite_variant.getValue().getValue(), 7)
        variant = encoder.createHLAvariantRecord(encoder.createHLAoctet())
        discriminant_one = encoder.createHLAoctet(1)
        discriminant_two = encoder.createHLAoctet(2)
        integer_value = encoder.createHLAinteger32BE(0x10203040)
        string_value = encoder.createHLAASCIIstring("A")
        variant.setVariant(discriminant_one, integer_value)
        variant.setVariant(discriminant_two, string_value)
        integer_value.setValue(7)
        string_value.setValue("B")
        variant_one_bytes = b"\x01" + b"\0\0\0" + b"\x10\x20\x30\x40"
        variant_two_bytes = b"\x02" + b"\0\0\0" + b"\x00\x00\x00\x01A"
        self.assertIsInstance(variant, HLAvariantRecord)
        self.assertEqual(variant.getOctetBoundary(), 4)
        self.assertEqual(variant.getEncodedLength(), len(variant_two_bytes))
        self.assertEqual(variant.toByteArray(), variant_two_bytes)
        self.assertEqual(variant.getDiscriminant().getValue(), 2)
        self.assertEqual(variant.getValue().getValue(), "A")  # type: ignore[union-attr]
        variant.setDiscriminant(discriminant_one)
        self.assertEqual(variant.toByteArray(), variant_one_bytes)
        self.assertEqual(variant.getValue().getValue(), 0x10203040)  # type: ignore[union-attr]
        variant.setDiscriminant(encoder.createHLAoctet(3))
        self.assertIsNone(variant.getValue())
        self.assertEqual(variant.toByteArray(), b"\x03")
        decoded_variant = encoder.createHLAvariantRecord(encoder.createHLAoctet())
        decoded_variant.setVariant(encoder.createHLAoctet(1), encoder.createHLAinteger32BE())
        decoded_variant.setVariant(encoder.createHLAoctet(2), encoder.createHLAASCIIstring())
        decoded_variant.decode(variant_two_bytes)
        self.assertEqual(decoded_variant.getValue().getValue(), "A")  # type: ignore[union-attr]
        bad_variant_padding = bytearray(variant_two_bytes)
        bad_variant_padding[1] = 1
        with self.assertRaises(DecoderException):
            decoded_variant.decode(bad_variant_padding)
        with self.assertRaises(DecoderException):
            decoded_variant.decode(variant_two_bytes + b"\0")
        with self.assertRaises(EncoderException):
            variant.setDiscriminant(encoder.createHLAinteger32BE(1))
        with self.assertRaises(EncoderException):
            variant.setVariant(discriminant_one, encoder.createHLAoctet(1))
        self.assertEqual(integer16.toByteArray(), struct.pack(">h", -1234))
        self.assertEqual(integer16.getOctetBoundary(), 2)
        self.assertIs(integer16.decode(b"\x7f\xff"), integer16)
        self.assertEqual(integer16.getValue(), 32767)
        self.assertEqual(integer32le.toByteArray(), struct.pack("<i", -0x1234567))
        self.assertEqual(integer32le.getOctetBoundary(), 4)
        self.assertIs(integer32le.decode(b"\x78\x56\x34\x12"), integer32le)
        self.assertEqual(integer32le.getValue(), 0x12345678)
        self.assertEqual(float64.toByteArray(), struct.pack(">d", math.pi))
        self.assertEqual(float64.getOctetBoundary(), 8)
        self.assertIs(float64.setValue(-0.5), float64)
        self.assertEqual(float64.getValue(), -0.5)
        self.assertEqual(float32.toByteArray(), struct.pack(">f", math.pi))
        self.assertEqual(float32.getOctetBoundary(), 4)
        self.assertAlmostEqual(float32.getValue(), math.pi, places=6)
        self.assertEqual(float32le.toByteArray(), struct.pack("<f", math.pi))
        self.assertEqual(float32le.getOctetBoundary(), 4)
        self.assertIs(float32le.setValue(-0.5), float32le)
        self.assertEqual(float32le.getValue(), -0.5)
        self.assertEqual(float64le.toByteArray(), struct.pack("<d", math.pi))
        self.assertEqual(float64le.getOctetBoundary(), 8)
        self.assertIs(float64le.setValue(-0.5), float64le)
        self.assertEqual(float64le.getValue(), -0.5)
        self.assertEqual(unsigned16.toByteArray(), b"\xab\xcd")
        self.assertEqual(unsigned16.getOctetBoundary(), 2)
        self.assertEqual(unsigned16.getValue(), 0xABCD)
        self.assertIs(unsigned16.decode(b"\x80\x01"), unsigned16)
        self.assertEqual(unsigned16.getValue(), 0x8001)
        self.assertEqual(unsigned16le.toByteArray(), b"\xcd\xab")
        self.assertEqual(unsigned16le.getOctetBoundary(), 2)
        self.assertIs(unsigned16le.decode(b"\x01\x80"), unsigned16le)
        self.assertEqual(unsigned16le.getValue(), 0x8001)
        self.assertEqual(integer16le.toByteArray(), struct.pack("<h", -0x1234))
        self.assertEqual(integer16le.getOctetBoundary(), 2)
        self.assertIs(integer16le.decode(b"\x34\x12"), integer16le)
        self.assertEqual(integer16le.getValue(), 0x1234)
        self.assertEqual(unsigned32le.toByteArray(), struct.pack("<I", 0x89ABCDEF))
        self.assertEqual(unsigned32le.getValue(), 0x89ABCDEF)
        self.assertEqual(integer64.toByteArray(), struct.pack(">q", -0x123456789AB))
        self.assertEqual(integer64.getOctetBoundary(), 8)
        self.assertIs(integer64.decode(b"\x01\x02\x03\x04\x05\x06\x07\x08"), integer64)
        self.assertEqual(integer64.getValue(), 0x0102030405060708)
        self.assertEqual(integer64le.toByteArray(), struct.pack("<q", -0x123456789AB))
        self.assertEqual(integer64le.getOctetBoundary(), 8)
        self.assertEqual(unsigned64.toByteArray(), struct.pack(">Q", 0xFEDCBA9876543210))
        self.assertEqual(unsigned64.getValue(), 0xFEDCBA9876543210)
        self.assertIs(unsigned64.decode(b"\xff\xff\xff\xff\xff\xff\xff\xff"), unsigned64)
        self.assertEqual(unsigned64.getValue(), 0xFFFFFFFFFFFFFFFF)
        self.assertEqual(unsigned64le.toByteArray(), struct.pack("<Q", 0xFEDCBA9876543210))
        self.assertEqual(unsigned64le.getValue(), 0xFEDCBA9876543210)
        self.assertEqual(unsigned.toByteArray(), b"\x124\xab\xcd")
        self.assertEqual(unsigned_maximum.toByteArray(), b"\xff\xff\xff\xff")
        self.assertEqual(unsigned_maximum.getValue(), 0xFFFFFFFF)
        self.assertEqual(boolean.toByteArray(), b"\x00\x00\x00\x01")
        self.assertEqual(unicode.toByteArray(), b"\x00\x00\x00\x03\x00A\xd8=\xde\x00")
        self.assertEqual(byte.toByteArray(), b"\xa5")
        self.assertEqual(byte.getOctetBoundary(), 1)
        self.assertIs(byte.decode(b"\xff"), byte)
        self.assertEqual(byte.getValue(), 0xFF)
        self.assertEqual(octet.toByteArray(), b"\x5a")
        self.assertEqual(octet.getOctetBoundary(), 1)
        self.assertIs(octet.decode(b"\x80"), octet)
        self.assertEqual(octet.getValue(), 0x80)
        self.assertEqual(ascii_char.toByteArray(), b"A")
        self.assertEqual(ascii_char.getOctetBoundary(), 1)
        self.assertIs(ascii_char.decode(b"Z"), ascii_char)
        self.assertEqual(ascii_char.getValue(), ord("Z"))
        self.assertEqual(ascii_string.toByteArray(), b"\x00\x00\x00\x03A!?")
        self.assertEqual(ascii_string.getOctetBoundary(), 4)
        self.assertIs(ascii_string.decode(b"\x00\x00\x00\x02OK"), ascii_string)
        self.assertEqual(ascii_string.getValue(), "OK")
        self.assertEqual(unicode_char.toByteArray(), b"\x03\xa9")
        self.assertEqual(unicode_char.getOctetBoundary(), 2)
        self.assertIs(unicode_char.decode(b"\xd8\x3d"), unicode_char)
        self.assertEqual(unicode_char.getValue(), 0xD83D)
        self.assertEqual(pair_be.toByteArray(), b"\x12\x34")
        self.assertEqual(pair_be.getOctetBoundary(), 2)
        self.assertIs(pair_be.decode(b"\xab\xcd"), pair_be)
        self.assertEqual(pair_be.getValue(), 0xABCD)
        self.assertEqual(pair_le.toByteArray(), b"\x34\x12")
        self.assertEqual(pair_le.getOctetBoundary(), 2)
        self.assertIs(pair_le.decode(b"\xab\xcd"), pair_le)
        self.assertEqual(pair_le.getValue(), 0xCDAB)
        self.assertEqual(integer.decode(b"\x00\x00\x00\x07").getValue(), 7)
        for element, malformed in (
            (integer, b"\0" * 5),
            (float64, b"\0" * 9),
            (ascii_string, b"\0\0\0\x02A"),
            (unicode, b"\0\0\0\x02\0A"),
        ):
            with self.assertRaises(DecoderException):
                element.decode(malformed)
        self.assertEqual(unicode.setValue("reset").getValue(), "reset")
        with self.assertRaises(DecoderException):
            encoder.createHLAboolean().decode(b"\x00\x00\x00\x02")

        java_ambassador = ambassador.unwrap_java_object()
        java_ambassador.queueConnectionLost("callback from the JVM")
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(callbacks.connection_losses, ["callback from the JVM"])
        java_ambassador.queueConnectionLost("callback held while disabled")
        ambassador.disableCallbacks()
        self.assertFalse(ambassador.evokeCallback(0.0))
        self.assertEqual(callbacks.connection_losses, ["callback from the JVM"])
        ambassador.enableCallbacks()
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(
            callbacks.connection_losses,
            ["callback from the JVM", "callback held while disabled"],
        )

        ambassador.listFederationExecutions()
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(
            callbacks.federation_execution_reports,
            [
                FederationExecutionInformationSet(
                    [FederationExecutionInformation("Umbra Mock Federation", HLA_TYPES.INTEGER64_TIME)]
                )
            ],
        )

        ambassador.createFederationExecution(
            "Python-created mock federation",
            "fixture-does-not-parse-fom.xml",
            HLA_TYPES.INTEGER64_TIME,
        )
        ambassador.listFederationExecutions()
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertIn(
            FederationExecutionInformation("Python-created mock federation", HLA_TYPES.INTEGER64_TIME),
            callbacks.federation_execution_reports[-1],
        )
        ambassador.destroyFederationExecution("Python-created mock federation")

        ambassador.createFederationExecution(
            "Python-created module federation",
            ["fixture-base.xml", "fixture-extension.xml"],
            HLA_TYPES.INTEGER64_TIME,
        )
        ambassador.createFederationExecutionWithMIM(
            "Python-created MIM federation",
            ["fixture-base.xml"],
            "fixture-mim.xml",
            HLA_TYPES.INTEGER64_TIME,
        )
        ambassador.destroyFederationExecution("Python-created module federation")
        ambassador.destroyFederationExecution("Python-created MIM federation")

        ambassador.listFederationExecutionMembers("Umbra Mock Federation")
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(
            callbacks.federation_execution_member_reports,
            [("Umbra Mock Federation", FederationExecutionMemberInformationSet())],
        )
        ambassador.listFederationExecutionMembers("Missing mock federation")
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(callbacks.missing_federation_executions, ["Missing mock federation"])

        handle = ambassador.joinFederationExecution(
            "observer", "Umbra Mock Federation", federateName="python-mock-member"
        )
        self.assertEqual(
            handle.encodedValue,
            b"Umbra Mock Federation:python-mock-member:observer",
        )
        additional_handle = ambassador.joinFederationExecution(
            "observer",
            "Umbra Mock Federation",
            federateName="python-module-member",
            additionalFomModules=["fixture-additional.xml"],
        )
        self.assertEqual(
            additional_handle.encodedValue,
            b"Umbra Mock Federation:python-module-member:observer",
        )
        for factory, handle_type in (
            (ambassador.getFederateHandleFactory(), FederateHandle),
            (ambassador.getObjectClassHandleFactory(), ObjectClassHandle),
            (ambassador.getObjectInstanceHandleFactory(), ObjectInstanceHandle),
            (ambassador.getAttributeHandleFactory(), AttributeHandle),
            (ambassador.getInteractionClassHandleFactory(), InteractionClassHandle),
            (ambassador.getParameterHandleFactory(), ParameterHandle),
            (ambassador.getTransportationTypeHandleFactory(), TransportationTypeHandle),
            (ambassador.getDimensionHandleFactory(), DimensionHandle),
            (ambassador.getRegionHandleFactory(), RegionHandle),
        ):
            decoded = factory.decode(b"factory-encoded")
            self.assertIsInstance(decoded, handle_type)
            self.assertEqual(decoded.encodedValue, b"factory-encoded")
        attribute_set = ambassador.getAttributeHandleSetFactory().create()
        dimension_set = ambassador.getDimensionHandleSetFactory().create()
        federate_set = ambassador.getFederateHandleSetFactory().create()
        region_set = ambassador.getRegionHandleSetFactory().create()
        self.assertIsInstance(attribute_set, MutableAttributeHandleSet)
        self.assertIsInstance(dimension_set, MutableDimensionHandleSet)
        self.assertIsInstance(federate_set, MutableFederateHandleSet)
        self.assertIsInstance(region_set, MutableRegionHandleSet)
        attribute_set.add(AttributeHandle(b"factory-attribute"))
        dimension_set.add(DimensionHandle(b"factory-dimension"))
        federate_set.add(FederateHandle(handle.encodedValue))
        region_set.add(RegionHandle(b"factory-region"))
        attribute_values = ambassador.getAttributeHandleValueMapFactory().create()
        parameter_values = ambassador.getParameterHandleValueMapFactory().create()
        self.assertIsInstance(attribute_values, MutableAttributeHandleValueMap)
        self.assertIsInstance(parameter_values, MutableParameterHandleValueMap)
        attribute_values[AttributeHandle(b"factory-attribute")] = b"value"
        parameter_values[ParameterHandle(b"factory-parameter")] = b"value"
        java_ambassador.emitAdditionalCallbacksForTest()
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(callbacks.resignations, ["fixture RTI removed this federate"])
        self.assertEqual([time.getTime() for time, _ in callbacks.flush_grants], [0])
        self.assertEqual([time.getTime() for _, time in callbacks.flush_grants], [0])
        self.assertEqual(len(callbacks.request_retractions), 1)
        self.assertTrue(callbacks.request_retractions[0].isValid())
        ambassador.registerFederationSynchronizationPoint("real-jvm-sync", b"sync-tag")
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(callbacks.sync_registration, ["real-jvm-sync"])
        self.assertEqual(callbacks.sync_announcements, [("real-jvm-sync", b"sync-tag")])
        ambassador.synchronizationPointAchieved("real-jvm-sync")
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(callbacks.sync_completions, [("real-jvm-sync", FederateHandleSet())])
        ambassador.registerFederationSynchronizationPoint("real-jvm-failed-sync", b"failed-tag")
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertTrue(ambassador.evokeCallback(0.0))
        ambassador.synchronizationPointAchieved("real-jvm-failed-sync", successfully=False)
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertIn(
            (
                "real-jvm-failed-sync",
                FederateHandleSet([FederateHandle(b"Umbra Mock Federation:python-module-member")]),
            ),
            callbacks.sync_completions,
        )
        ambassador.registerFederationSynchronizationPoint(
            "real-jvm-explicit-sync",
            b"explicit-tag",
            synchronizationSet=FederateHandleSet([handle]),
        )
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertTrue(ambassador.evokeCallback(0.0))
        time_factory = ambassador.getTimeFactory()
        self.assertIsInstance(time_factory, HLAinteger64TimeFactory)
        self.assertEqual(time_factory.implementationName(), HLA_TYPES.INTEGER64_TIME)
        initial = time_factory.makeInitial()
        final = time_factory.makeFinal()
        zero = time_factory.makeZero()
        epsilon = time_factory.makeEpsilon()
        requested_time = time_factory.makeLogicalTime(5)
        lookahead = time_factory.makeLogicalTimeInterval(1)
        self.assertTrue(initial.isInitial())
        self.assertTrue(final.isFinal())
        self.assertTrue(zero.isZero())
        self.assertTrue(epsilon.isEpsilon())
        self.assertEqual(requested_time.getTime(), 5)
        self.assertEqual(lookahead.getInterval(), 1)
        self.assertIsInstance(requested_time, HLAinteger64Time)
        self.assertIsInstance(lookahead, HLAinteger64Interval)
        self.assertEqual(time_factory.decodeLogicalTime(requested_time.toByteArray()).getTime(), 5)
        self.assertEqual(
            time_factory.decodeLogicalTimeInterval(lookahead.toByteArray()).getInterval(), 1
        )
        advanced = time_factory.add(requested_time, lookahead)
        self.assertEqual(advanced.getTime(), 6)
        self.assertEqual(time_factory.subtract(advanced, lookahead).getTime(), 5)
        self.assertEqual(time_factory.difference(advanced, requested_time).getInterval(), 1)
        malformed_time_values = (
            b"",
            b"\0" * 7,
            b"\0" * 9,
            struct.pack(">q", -1),
        )
        for encoded in malformed_time_values:
            with self.assertRaises(CouldNotDecode):
                time_factory.decodeLogicalTime(encoded)
            with self.assertRaises(CouldNotDecode):
                time_factory.decodeLogicalTimeInterval(encoded)
        foreign_time = HLAfloat64Time(
            struct.pack(">d", 1.0), HLA_TYPES.FLOAT64_TIME, False, False, 1.0, "1.0"
        )
        foreign_interval = HLAfloat64Interval(
            struct.pack(">d", 1.0), HLA_TYPES.FLOAT64_TIME, False, False, 1.0, "1.0"
        )
        with self.assertRaises(InvalidLogicalTime):
            time_factory.add(foreign_time, lookahead)
        with self.assertRaises(InvalidLogicalTimeInterval):
            time_factory.add(requested_time, foreign_interval)
        with self.assertRaises(IllegalTimeArithmetic):
            time_factory.add(time_factory.makeFinal(), time_factory.makeEpsilon())
        with self.assertRaises(IllegalTimeArithmetic):
            time_factory.subtract(time_factory.makeInitial(), time_factory.makeEpsilon())
        with self.assertRaises(IllegalTimeArithmetic):
            time_factory.difference(time_factory.makeInitial(), requested_time)
        ambassador.enableTimeRegulation(lookahead)
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual([value.getTime() for value in callbacks.time_regulation], [0])
        ambassador.enableTimeConstrained()
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual([value.getTime() for value in callbacks.time_constrained], [0])
        ambassador.timeAdvanceRequest(requested_time)
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual([value.getTime() for value in callbacks.time_grants], [5])
        self.assertEqual(ambassador.queryLogicalTime().getTime(), 5)
        for request, value in (
            (ambassador.timeAdvanceRequestAvailable, 6),
            (ambassador.nextMessageRequest, 7),
            (ambassador.nextMessageRequestAvailable, 8),
            (ambassador.flushQueueRequest, 9),
        ):
            request(time_factory.makeLogicalTime(value))
            self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(
            [value.getTime() for value in callbacks.time_grants], [5, 6, 7, 8, 9]
        )
        self.assertTrue(ambassador.queryGALT().timeIsValid)
        self.assertEqual(ambassador.queryGALT().time.getTime(), 9)  # type: ignore[union-attr]
        self.assertTrue(ambassador.queryLITS().timeIsValid)
        self.assertEqual(ambassador.queryLITS().time.getTime(), 9)  # type: ignore[union-attr]
        self.assertEqual(ambassador.queryLookahead().getInterval(), 1)
        modified_lookahead = time_factory.makeLogicalTimeInterval(2)
        ambassador.modifyLookahead(modified_lookahead)
        self.assertEqual(ambassador.queryLookahead().getInterval(), 2)
        ambassador.enableAsynchronousDelivery()
        ambassador.disableAsynchronousDelivery()
        ambassador.disableTimeConstrained()
        ambassador.disableTimeRegulation()
        dimension = ambassador.getDimensionHandle(HLA_FIXTURES.SODA_FLAVOR)
        region = ambassador.createRegion(DimensionHandleSet([dimension]))
        self.assertIsInstance(region, RegionHandle)
        self.assertEqual(ambassador.getDimensionHandleSet(region), DimensionHandleSet([dimension]))
        ambassador.setRangeBounds(region, dimension, RangeBounds(1, 3))
        bounds = ambassador.getRangeBounds(region, dimension)
        self.assertEqual((bounds.getLowerBound(), bounds.getUpperBound()), (1, 3))
        ambassador.commitRegionModifications(RegionHandleSet([region]))
        ambassador.deleteRegion(region)
        object_class = ambassador.getObjectClassHandle(HLA_FOM.EMPLOYEE_SERVER)
        attribute = ambassador.getAttributeHandle(object_class, HLA_FIXTURES.EFFICIENCY)
        interaction = ambassador.getInteractionClassHandle(
            HLA_FOM.MAIN_COURSE_SERVED
        )
        directed = ambassador.getInteractionClassHandle(HLA_FOM.TAKE_ORDER)
        parameter = ambassador.getParameterHandle(interaction, HLA_FIXTURES.TEMPERATURE_OK)
        transportation = ambassador.getTransportationTypeHandle(HLA_MOM.RELIABLE)
        dimension = ambassador.getDimensionHandle(HLA_FIXTURES.SERVER_ID)
        self.assertIsInstance(object_class, ObjectClassHandle)
        self.assertIsInstance(attribute, AttributeHandle)
        self.assertIsInstance(interaction, InteractionClassHandle)
        self.assertIsInstance(parameter, ParameterHandle)
        self.assertIsInstance(transportation, TransportationTypeHandle)
        self.assertIsInstance(dimension, DimensionHandle)
        self.assertEqual(ambassador.getObjectClassName(object_class), HLA_FOM.EMPLOYEE_SERVER)
        self.assertEqual(
            ambassador.getAttributeName(object_class, attribute),
            HLA_FIXTURES.EFFICIENCY,
        )
        self.assertEqual(
            ambassador.getInteractionClassName(interaction),
            HLA_FOM.MAIN_COURSE_SERVED,
        )
        self.assertEqual(
            ambassador.getParameterName(interaction, parameter),
            HLA_FIXTURES.TEMPERATURE_OK,
        )
        self.assertEqual(ambassador.getTransportationTypeName(transportation), HLA_MOM.RELIABLE)
        self.assertEqual(ambassador.getDimensionName(dimension), HLA_FIXTURES.SERVER_ID)
        batch_names = ObjectInstanceNameSet(["jvm-batch-one", "jvm-batch-two"])
        ambassador.reserveMultipleObjectInstanceNames(batch_names)
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(callbacks.name_batch_successes[-1], batch_names)
        ambassador.releaseMultipleObjectInstanceNames(batch_names)
        ambassador.reserveObjectInstanceName("mock-reserved-then-released")
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(callbacks.reserved_names[-1], "mock-reserved-then-released")
        ambassador.releaseObjectInstanceName("mock-reserved-then-released")
        attributes = AttributeHandleSet([attribute])
        ambassador.subscribeObjectClassAttributes(object_class, attributes, active=False)
        ambassador.publishObjectClassAttributes(object_class, attributes)
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(callbacks.start_registration, [object_class])
        ambassador.unpublishObjectClassAttributes(object_class, attributes)
        ambassador.unsubscribeObjectClassAttributes(object_class, attributes)
        ambassador.unsubscribeObjectClass(object_class)
        ambassador.unpublishObjectClass(object_class)
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(callbacks.stop_registration, [object_class])
        directed_set = InteractionClassHandleSet([directed])
        ambassador.publishObjectClassDirectedInteractions(object_class, directed_set)
        ambassador.subscribeObjectClassDirectedInteractions(
            object_class, directed_set, universally=True
        )
        ambassador.unpublishObjectClassDirectedInteractions(object_class, directed_set)
        ambassador.unsubscribeObjectClassDirectedInteractions(object_class)
        ambassador.subscribeInteractionClass(interaction, active=False)
        ambassador.publishInteractionClass(interaction)
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(callbacks.interactions_on, [interaction])
        ambassador.unsubscribeInteractionClass(interaction)
        ambassador.unpublishInteractionClass(interaction)
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(callbacks.interactions_off, [interaction])
        ambassador.reserveObjectInstanceName("mock-registered-server")
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(callbacks.reserved_names[-1], "mock-registered-server")
        registered = ambassador.registerObjectInstance(
            object_class, objectInstanceName="mock-registered-server"
        )
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertIsInstance(registered, ObjectInstanceHandle)
        self.assertEqual(ambassador.getObjectInstanceName(registered), "mock-registered-server")
        self.assertEqual(ambassador.getObjectInstanceHandle("mock-registered-server"), registered)
        ambassador.requestAttributeValueUpdate(registered, attributes, b"instance-request")
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(callbacks.attribute_value_requests[-1], (registered, attributes, b"instance-request"))
        ambassador.requestAttributeValueUpdate(object_class, attributes, b"class-request")
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(callbacks.attribute_value_requests[-1], (registered, attributes, b"class-request"))
        federate = ambassador.getFederateHandle("python-mock-member")
        self.assertEqual(ambassador.getFederateName(federate), "python-mock-member")
        self.assertEqual(ambassador.getKnownObjectClassHandle(registered), object_class)
        self.assertEqual(ambassador.getUpdateRateValue(HLA_MOM.DEFAULT_UPDATE_RATE), 1.0)
        self.assertEqual(ambassador.getUpdateRateValueForAttribute(registered, attribute), 1.0)
        self.assertEqual(ambassador.getOrderType("Receive"), OrderType.RECEIVE)
        self.assertEqual(ambassador.getOrderName(OrderType.TIMESTAMP), "TimeStamp")
        self.assertEqual(
            ambassador.getAvailableDimensionsForObjectClass(object_class),
            DimensionHandleSet([DimensionHandle(b"dimension:SodaFlavor")]),
        )
        self.assertEqual(
            ambassador.getAvailableDimensionsForInteractionClass(interaction),
            DimensionHandleSet([DimensionHandle(b"dimension:SodaFlavor")]),
        )
        self.assertEqual(ambassador.getDimensionUpperBound(dimension), 100)
        self.assertEqual(ambassador.normalizeServiceGroup(ServiceGroup.OBJECT_MANAGEMENT), 2)
        self.assertEqual(ambassador.normalizeFederateHandle(federate), 1)
        self.assertEqual(ambassador.normalizeObjectClassHandle(object_class), 2)
        self.assertEqual(ambassador.normalizeInteractionClassHandle(interaction), 3)
        self.assertEqual(ambassador.normalizeObjectInstanceHandle(registered), 4)
        timestamped_update = ambassador.updateAttributeValuesWithTime(
            registered,
            AttributeHandleValueMap({attribute: b"timestamped"}),
            requested_time,
            b"timestamped-update",
        )
        timestamped_interaction = ambassador.sendInteractionWithTime(
            interaction, ParameterHandleValueMap({parameter: b"timestamped"}), requested_time,
            b"timestamped-interaction"
        )
        timestamped_delete = ambassador.deleteObjectInstanceWithTime(
            registered, requested_time, b"timestamped-delete"
        )
        for retraction in (timestamped_update, timestamped_interaction, timestamped_delete):
            self.assertIsInstance(retraction, MessageRetractionHandle)
            self.assertTrue(retraction.isValid())
        ambassador.retract(timestamped_update)
        self.assertEqual(
            callbacks.discoveries,
            [
                (
                    registered,
                    object_class,
                    "mock-registered-server",
                    FederateHandle(b"mock-producing-federate"),
                )
            ],
        )
        reflection_values = AttributeHandleValueMap({attribute: b"\x01\x02\xff"})
        ambassador.updateAttributeValues(registered, reflection_values, b"mock-update-tag")
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(
            callbacks.reflections,
            [
                (
                    registered,
                    reflection_values,
                    b"mock-update-tag",
                    TransportationTypeHandle(b"transportation:HLAreliable"),
                    FederateHandle(b"mock-producing-federate"),
                )
            ],
        )
        interaction_values = ParameterHandleValueMap({parameter: b"served"})
        ambassador.sendInteraction(interaction, interaction_values, b"mock-interaction-tag")
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(
            callbacks.interactions,
            [
                (
                    interaction,
                    interaction_values,
                    b"mock-interaction-tag",
                    TransportationTypeHandle(b"transportation:HLAreliable"),
                    FederateHandle(b"mock-producing-federate"),
                )
            ],
        )
        ambassador.sendDirectedInteraction(
            directed, registered, ParameterHandleValueMap(), b"directed-tag"
        )
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(callbacks.directed_interactions[-1][0], directed)
        self.assertEqual(callbacks.directed_interactions[-1][1], registered)
        directed_retraction = ambassador.sendDirectedInteractionWithTime(
            directed,
            registered,
            ParameterHandleValueMap(),
            requested_time,
            b"timed-directed-tag",
        )
        self.assertIsInstance(directed_retraction, MessageRetractionHandle)
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(callbacks.timed_directed_interactions[-1][0], directed)
        self.assertEqual(callbacks.timed_directed_interactions[-1][1], registered)
        self.assertEqual(callbacks.timed_directed_interactions[-1][6].getTime(), 5)
        self.assertEqual(callbacks.timed_directed_interactions[-1][7], OrderType.TIMESTAMP)
        self.assertIsInstance(callbacks.timed_directed_interactions[-1][9], MessageRetractionHandle)
        ambassador._implementation.emitTimestampedCallbacksForEncoded(
            registered.encodedValue, interaction.encodedValue, 7
        )
        self.assertTrue(ambassador.evokeMultipleCallbacks(0.0, 0.0))
        self.assertTrue(ambassador.evokeMultipleCallbacks(0.0, 0.0))
        self.assertTrue(ambassador.evokeMultipleCallbacks(0.0, 0.0))
        self.assertEqual(callbacks.timed_reflections[-1][0], registered)
        self.assertEqual(
            callbacks.timed_reflections[-1][5],
            RegionHandleSet([RegionHandle(b"region:callback-region")]),
        )
        self.assertEqual(callbacks.timed_reflections[-1][6].getTime(), 7)
        self.assertEqual(callbacks.timed_reflections[-1][7], OrderType.TIMESTAMP)
        self.assertIsInstance(callbacks.timed_reflections[-1][9], MessageRetractionHandle)
        self.assertEqual(callbacks.timed_interactions[-1][0], interaction)
        self.assertEqual(
            callbacks.timed_interactions[-1][5],
            RegionHandleSet([RegionHandle(b"region:callback-region")]),
        )
        self.assertEqual(callbacks.timed_interactions[-1][6].getTime(), 7)
        self.assertEqual(callbacks.timed_interactions[-1][7], OrderType.TIMESTAMP)
        self.assertIsInstance(callbacks.timed_removals[-1][6], MessageRetractionHandle)
        ambassador._implementation.emitAdvisoryCallbacksForEncoded(
            registered.encodedValue, attribute.encodedValue
        )
        for _ in range(5):
            self.assertTrue(ambassador.evokeMultipleCallbacks(0.0, 0.0))
        self.assertEqual(callbacks.scope_entries[-1], (registered, AttributeHandleSet([attribute])))
        self.assertEqual(callbacks.scope_exits[-1], (registered, AttributeHandleSet([attribute])))
        self.assertEqual(callbacks.updates_on[-1][2], "High")
        self.assertEqual(callbacks.updates_off[-1], (registered, AttributeHandleSet([attribute])))
        ambassador.changeAttributeOrderType(registered, attributes, OrderType.TIMESTAMP)
        ambassador.changeDefaultAttributeOrderType(object_class, attributes, OrderType.RECEIVE)
        ambassador.changeInteractionOrderType(interaction, OrderType.RECEIVE)
        ambassador.requestAttributeTransportationTypeChange(registered, attributes, transportation)
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(callbacks.attribute_transport_confirmations[-1], (registered, attributes, transportation))
        ambassador.changeDefaultAttributeTransportationType(object_class, attributes, transportation)
        ambassador.queryAttributeTransportationType(registered, attribute)
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(callbacks.attribute_transport_reports[-1], (registered, attribute, transportation))
        ambassador.requestInteractionTransportationTypeChange(interaction, transportation)
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(callbacks.interaction_transport_confirmations[-1], (interaction, transportation))
        ambassador.queryInteractionTransportationType(federate, interaction)
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(callbacks.interaction_transport_reports[-1], (federate, interaction, transportation))
        regional_dimension = ambassador.getDimensionHandle(HLA_FIXTURES.SODA_FLAVOR)
        regional = ambassador.createRegion(DimensionHandleSet([regional_dimension]))
        ambassador.setRangeBounds(regional, regional_dimension, RangeBounds(1, 3))
        ambassador.commitRegionModifications(RegionHandleSet([regional]))
        timestamped_regional_interaction = ambassador.sendInteractionWithRegionsWithTime(
            interaction,
            ParameterHandleValueMap({parameter: b"timestamped-regional"}),
            RegionHandleSet([regional]),
            requested_time,
            b"timestamped-regional-interaction",
        )
        self.assertIsInstance(timestamped_regional_interaction, MessageRetractionHandle)
        self.assertTrue(timestamped_regional_interaction.isValid())
        ambassador.subscribeInteractionClassWithRegions(interaction, RegionHandleSet([regional]))
        ambassador.setConveyRegionDesignatorSetsSwitch(True)
        for getter, setter in (
            (
                ambassador.getObjectClassRelevanceAdvisorySwitch,
                ambassador.setObjectClassRelevanceAdvisorySwitch,
            ),
            (
                ambassador.getAttributeRelevanceAdvisorySwitch,
                ambassador.setAttributeRelevanceAdvisorySwitch,
            ),
            (
                ambassador.getAttributeScopeAdvisorySwitch,
                ambassador.setAttributeScopeAdvisorySwitch,
            ),
            (
                ambassador.getInteractionRelevanceAdvisorySwitch,
                ambassador.setInteractionRelevanceAdvisorySwitch,
            ),
        ):
            initial = getter()
            setter(not initial)
            self.assertEqual(getter(), not initial)
            setter(initial)
            self.assertEqual(getter(), initial)
        initial_resign = ambassador.getAutomaticResignDirective()
        alternate_resign = next(action for action in ResignAction if action is not initial_resign)
        ambassador.setAutomaticResignDirective(alternate_resign)
        self.assertEqual(ambassador.getAutomaticResignDirective(), alternate_resign)
        ambassador.setAutomaticResignDirective(initial_resign)
        self.assertEqual(ambassador.getAutomaticResignDirective(), initial_resign)
        for getter, setter in (
            (ambassador.getServiceReportingSwitch, ambassador.setServiceReportingSwitch),
            (ambassador.getExceptionReportingSwitch, ambassador.setExceptionReportingSwitch),
        ):
            initial = getter()
            setter(not initial)
            self.assertEqual(getter(), not initial)
            setter(initial)
            self.assertEqual(getter(), initial)
        for getter in (
            ambassador.getSendServiceReportsToFileSwitch,
            ambassador.getAutoProvideSwitch,
            ambassador.getDelaySubscriptionEvaluationSwitch,
            ambassador.getAdvisoriesUseKnownClassSwitch,
            ambassador.getAllowRelaxedDDMSwitch,
            ambassador.getNonRegulatedGrantSwitch,
        ):
            self.assertIsInstance(getter(), bool)
        ambassador.sendInteractionWithRegions(
            interaction, interaction_values, RegionHandleSet([regional]), b"regional-tag"
        )
        self.assertTrue(ambassador.evokeCallback(0.0))
        # The earlier timestamped callback probe also carries a non-null region
        # designator set; the ordinary regional send is the final entry here.
        self.assertEqual(callbacks.sent_regions[-1], RegionHandleSet([regional]))
        regional_pairs = AttributeSetRegionSetPairList(
            [AttributeSetRegionSetPair(attributes, RegionHandleSet([regional]))]
        )
        ambassador.subscribeObjectClassAttributesWithRegions(object_class, regional_pairs)
        regional_registered = ambassador.registerObjectInstanceWithRegions(object_class, regional_pairs)
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(callbacks.discoveries[-1][0], regional_registered)
        ambassador.unwrap_java_object().enableTimestampedRegionalAttributeProbe()
        regional_timed_update = ambassador.updateAttributeValuesWithTime(
            regional_registered,
            AttributeHandleValueMap({attribute: b"regional-timed-update"}),
            ambassador.getTimeFactory().makeLogicalTime(11),
            b"regional-timed-update-tag",
        )
        self.assertTrue(regional_timed_update.isValid())
        ambassador.timeAdvanceRequest(ambassador.getTimeFactory().makeLogicalTime(11))
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(callbacks.timed_reflections[-1][0], regional_registered)
        self.assertEqual(
            callbacks.timed_reflections[-1][1][attribute], b"regional-timed-update"
        )
        self.assertEqual(callbacks.timed_reflections[-1][2], b"regional-timed-update-tag")
        self.assertEqual(
            callbacks.timed_reflections[-1][5],
            RegionHandleSet([RegionHandle(b"region:regional-attribute")]),
        )
        self.assertEqual(callbacks.timed_reflections[-1][6].getTime(), 11)
        self.assertEqual(callbacks.timed_reflections[-1][7], OrderType.TIMESTAMP)
        self.assertEqual(callbacks.timed_reflections[-1][8], OrderType.TIMESTAMP)
        self.assertTrue(callbacks.timed_reflections[-1][9].isValid())
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(callbacks.time_grants[-1].getTime(), 11)
        ambassador.associateRegionsForUpdates(regional_registered, regional_pairs)
        ambassador.unassociateRegionsForUpdates(regional_registered, regional_pairs)
        ambassador.requestAttributeValueUpdateWithRegions(object_class, regional_pairs, b"regional-request")
        ambassador.queryAttributeOwnership(regional_registered, attributes)
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(callbacks.ownership_reports[-1][0], regional_registered)
        self.assertEqual(callbacks.ownership_reports[-1][1], attributes)
        self.assertIsInstance(callbacks.ownership_reports[-1][2], FederateHandle)
        self.assertTrue(ambassador.isAttributeOwnedByFederate(regional_registered, attribute))
        ambassador.unconditionalAttributeOwnershipDivestiture(
            regional_registered, attributes, b"regional-divest"
        )
        ambassador.attributeOwnershipAcquisition(
            regional_registered, attributes, b"regional-acquire"
        )
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(
            callbacks.ownership_acquisitions[-1],
            (regional_registered, attributes, b"regional-acquire"),
        )
        ambassador.attributeOwnershipAcquisitionIfAvailable(
            regional_registered, attributes, b"regional-acquire-if-available"
        )
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(
            callbacks.ownership_acquisitions[-1],
            (regional_registered, attributes, b"regional-acquire-if-available"),
        )
        ambassador.cancelAttributeOwnershipAcquisition(regional_registered, attributes)
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(
            callbacks.ownership_acquisition_cancellations[-1],
            (regional_registered, attributes),
        )
        ambassador.negotiatedAttributeOwnershipDivestiture(
            regional_registered, attributes, b"regional-negotiated"
        )
        ambassador.confirmDivestiture(regional_registered, attributes, b"regional-confirm")
        ambassador.cancelNegotiatedAttributeOwnershipDivestiture(regional_registered, attributes)
        ambassador.attributeOwnershipReleaseDenied(
            regional_registered, attributes, b"regional-release-denied"
        )
        self.assertEqual(
            ambassador.attributeOwnershipDivestitureIfWanted(
                regional_registered, attributes, b"regional-wanted"
            ),
            attributes,
        )
        ambassador.unsubscribeObjectClassAttributesWithRegions(object_class, regional_pairs)
        ambassador.deleteObjectInstance(regional_registered, b"regional-delete-tag")
        self.assertTrue(ambassador.evokeCallback(0.0))
        ambassador.deleteObjectInstance(registered, b"mock-delete-tag")
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(
            callbacks.removals[-1],
            (
                registered,
                b"mock-delete-tag",
                FederateHandle(b"mock-producing-federate"),
            ),
        )
        local_registered = ambassador.registerObjectInstance(object_class)
        self.assertTrue(ambassador.evokeCallback(0.0))
        ambassador.localDeleteObjectInstance(local_registered)

        ambassador.queryFederationSaveStatus()
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(len(callbacks.save_status_reports[-1]), 1)
        self.assertEqual(
            callbacks.save_status_reports[-1][0].federateHandle,
            FederateHandle(b"save:mock-federate"),
        )
        self.assertEqual(
            callbacks.save_status_reports[-1][0].saveStatus,
            SaveStatus.NO_SAVE_IN_PROGRESS,
        )

        ambassador.requestFederationSave("mock-save")
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(callbacks.save_initiations, ["mock-save"])
        ambassador.queryFederationSaveStatus()
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(
            callbacks.save_status_reports[-1][0].saveStatus,
            SaveStatus.FEDERATE_SAVING,
        )
        ambassador.federateSaveBegun()
        ambassador.federateSaveComplete()
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(callbacks.save_completions, 1)

        # The earlier time-management and regional callback probes advance the
        # fixture to 11, so the
        # standard timestamped-save contract requires a future save boundary.
        timed_save_time = ambassador.getTimeFactory().makeLogicalTime(12)
        ambassador.requestFederationSave("mock-timed-save", timed_save_time)
        ambassador.queryFederationSaveStatus()
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(
            callbacks.save_status_reports[-1][0].saveStatus,
            SaveStatus.FEDERATE_INSTRUCTED_TO_SAVE,
        )
        ambassador.timeAdvanceRequest(timed_save_time)
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(callbacks.save_initiations[-1], "mock-timed-save")
        self.assertTrue(ambassador.evokeCallback(0.0))
        raw_timed_save = ambassador.unwrap_java_object().getLastRequestedSaveTime()
        self.assertEqual(raw_timed_save.getTime(), 12)
        self.assertEqual(str(raw_timed_save.implementationName()), HLA_TYPES.INTEGER64_TIME)
        ambassador.federateSaveBegun()
        ambassador.federateSaveComplete()
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(callbacks.save_completions, 2)

        # The fixture can arm a small TSO probe so the standard timed send
        # remains queued until the save boundary.  This verifies that the
        # Java callback proxy observes receive -> initiate-save -> grant in
        # the same order as the native adapter.
        ambassador.unwrap_java_object().enableTsoSaveBoundaryProbe()
        tso_save_time = ambassador.getTimeFactory().makeLogicalTime(14)
        tso_retraction = ambassador.sendInteractionWithTime(
            interaction,
            ParameterHandleValueMap({parameter: b"queued-before-save"}),
            tso_save_time,
            b"queued-before-save-tag",
        )
        self.assertTrue(tso_retraction.isValid())
        ambassador.requestFederationSave("mock-tso-save", tso_save_time)
        ambassador.timeAdvanceRequest(tso_save_time)
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(callbacks.timed_interactions[-1][0], interaction)
        self.assertEqual(callbacks.timed_interactions[-1][1][parameter], b"queued-before-save")
        self.assertEqual(callbacks.timed_interactions[-1][2], b"queued-before-save-tag")
        self.assertEqual(callbacks.timed_interactions[-1][6].getTime(), 14)
        self.assertEqual(callbacks.timed_interactions[-1][7], OrderType.TIMESTAMP)
        self.assertTrue(callbacks.timed_interactions[-1][9].isValid())
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(callbacks.save_initiations[-1], "mock-tso-save")
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(callbacks.time_grants[-1].getTime(), 14)
        ambassador.federateSaveBegun()
        ambassador.federateSaveComplete()
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(callbacks.save_completions, 3)

        ambassador.requestFederationSave("mock-save-failure")
        self.assertTrue(ambassador.evokeCallback(0.0))
        ambassador.federateSaveNotComplete()
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(
            callbacks.save_failures,
            [SaveFailureReason.FEDERATE_REPORTED_FAILURE_DURING_SAVE],
        )

        ambassador.requestFederationSave("mock-save-abort")
        self.assertTrue(ambassador.evokeCallback(0.0))
        ambassador.abortFederationSave()
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(callbacks.save_failures[-1], SaveFailureReason.SAVE_ABORTED)

        ambassador.requestFederationSave("missing-save")
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(callbacks.save_failures[-1], SaveFailureReason.RTI_UNABLE_TO_SAVE)

        ambassador.queryFederationRestoreStatus()
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(
            callbacks.restore_status_reports[-1][0].status.name,
            "NO_RESTORE_IN_PROGRESS",
        )
        restore_label = "mock-restore"
        ambassador.requestFederationRestore(restore_label)
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(callbacks.restore_accepted, [restore_label])
        self.assertEqual(callbacks.restore_begun, 1)
        self.assertEqual(
            callbacks.restore_initiations,
            [(restore_label, "mock-federate", FederateHandle(b"restore:mock-restore"))],
        )
        ambassador.queryFederationRestoreStatus()
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(
            callbacks.restore_status_reports[-1][0].status.name,
            "FEDERATE_RESTORING",
        )
        ambassador.federateRestoreComplete()
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(callbacks.restore_completions, 1)
        ambassador.queryFederationRestoreStatus()
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(
            callbacks.restore_status_reports[-1][0].status.name,
            "NO_RESTORE_IN_PROGRESS",
        )

        ambassador.requestFederationRestore(restore_label)
        for _ in range(3):
            self.assertTrue(ambassador.evokeCallback(0.0))
        ambassador.federateRestoreNotComplete()
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(
            callbacks.restore_failures,
            [RestoreFailureReason.FEDERATE_REPORTED_FAILURE_DURING_RESTORE],
        )

        ambassador.requestFederationRestore(restore_label)
        for _ in range(3):
            self.assertTrue(ambassador.evokeCallback(0.0))
        ambassador.abortFederationRestore()
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(callbacks.restore_failures[-1], RestoreFailureReason.RESTORE_ABORTED)

        ambassador.requestFederationRestore("missing-restore")
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(callbacks.restore_rejected, ["missing-restore"])

        ambassador.resignFederationExecution(ResignAction.NO_ACTION)

        with self.assertRaises(AlreadyConnected):
            ambassador.connect(callbacks, CallbackModel.HLA_EVOKED)

        ambassador.disconnect()

    def test_real_jvm_multi_member_receive_order_fanout(self) -> None:
        publisher = self.factory.getRtiAmbassador()
        first_subscriber = self.factory.getRtiAmbassador()
        second_subscriber = self.factory.getRtiAmbassador()
        publisher_callbacks = _RecordingFederateAmbassador()
        first_callbacks = _RecordingFederateAmbassador()
        second_callbacks = _RecordingFederateAmbassador()
        federation_name = "Python multi-member fanout federation"
        created = publisher_joined = first_joined = second_joined = False
        try:
            publisher.connect(publisher_callbacks, CallbackModel.HLA_EVOKED)
            first_subscriber.connect(first_callbacks, CallbackModel.HLA_EVOKED)
            second_subscriber.connect(second_callbacks, CallbackModel.HLA_EVOKED)
            publisher.createFederationExecution(
                federation_name,
                "fixture-does-not-parse-fom.xml",
                HLA_TYPES.INTEGER64_TIME,
            )
            created = True
            publisher.joinFederationExecution("fanout-publisher", federation_name)
            publisher_joined = True
            first_subscriber.joinFederationExecution("fanout-first", federation_name)
            first_joined = True
            second_subscriber.joinFederationExecution("fanout-second", federation_name)
            second_joined = True

            publisher_object_class = publisher.getObjectClassHandle(
                HLA_FOM.FOOD_DRINK_SODA
            )
            first_object_class = first_subscriber.getObjectClassHandle(
                HLA_FOM.FOOD_DRINK_SODA
            )
            second_object_class = second_subscriber.getObjectClassHandle(
                HLA_FOM.FOOD_DRINK_SODA
            )
            publisher_attribute = publisher.getAttributeHandle(publisher_object_class, HLA_FIXTURES.FLAVOR)
            first_attribute = first_subscriber.getAttributeHandle(first_object_class, HLA_FIXTURES.FLAVOR)
            second_attribute = second_subscriber.getAttributeHandle(
                second_object_class, HLA_FIXTURES.FLAVOR
            )
            publisher.publishObjectClassAttributes(
                publisher_object_class, AttributeHandleSet([publisher_attribute])
            )
            first_subscriber.subscribeObjectClassAttributes(
                first_object_class, AttributeHandleSet([first_attribute])
            )
            second_subscriber.subscribeObjectClassAttributes(
                second_object_class, AttributeHandleSet([second_attribute])
            )

            publisher_interaction = publisher.getInteractionClassHandle(
                HLA_FOM.SODA_SERVED
            )
            first_interaction = first_subscriber.getInteractionClassHandle(
                HLA_FOM.SODA_SERVED
            )
            second_interaction = second_subscriber.getInteractionClassHandle(
                HLA_FOM.SODA_SERVED
            )
            publisher.publishInteractionClass(publisher_interaction)
            first_subscriber.subscribeInteractionClass(first_interaction)
            second_subscriber.subscribeInteractionClass(second_interaction)

            object_instance = publisher.registerObjectInstance(publisher_object_class)
            for _ in range(4):
                first_subscriber.evokeMultipleCallbacks(0.0, 0.0)
                second_subscriber.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(len(first_callbacks.discoveries), 1)
            self.assertEqual(len(second_callbacks.discoveries), 1)
            self.assertEqual(first_callbacks.discoveries[0][0], object_instance)
            self.assertEqual(second_callbacks.discoveries[0][0], object_instance)

            publisher.updateAttributeValues(
                object_instance,
                AttributeHandleValueMap({publisher_attribute: b"fanout-flavor"}),
                b"fanout-update-tag",
            )
            for _ in range(4):
                first_subscriber.evokeMultipleCallbacks(0.0, 0.0)
                second_subscriber.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(len(first_callbacks.reflections), 1)
            self.assertEqual(len(second_callbacks.reflections), 1)
            self.assertEqual(first_callbacks.reflections[0][1][first_attribute], b"fanout-flavor")
            self.assertEqual(second_callbacks.reflections[0][1][second_attribute], b"fanout-flavor")
            self.assertEqual(first_callbacks.reflections[0][2], b"fanout-update-tag")
            self.assertEqual(second_callbacks.reflections[0][2], b"fanout-update-tag")

            publisher.sendInteraction(
                publisher_interaction,
                ParameterHandleValueMap(),
                b"fanout-interaction-tag",
            )
            for _ in range(4):
                first_subscriber.evokeMultipleCallbacks(0.0, 0.0)
                second_subscriber.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(len(first_callbacks.interactions), 1)
            self.assertEqual(len(second_callbacks.interactions), 1)
            self.assertEqual(first_callbacks.interactions[0][0], first_interaction)
            self.assertEqual(second_callbacks.interactions[0][0], second_interaction)
            self.assertEqual(first_callbacks.interactions[0][2], b"fanout-interaction-tag")
            self.assertEqual(second_callbacks.interactions[0][2], b"fanout-interaction-tag")
        finally:
            if second_joined:
                second_subscriber.resignFederationExecution(ResignAction.NO_ACTION)
            if first_joined:
                first_subscriber.resignFederationExecution(ResignAction.NO_ACTION)
            if publisher_joined:
                publisher.resignFederationExecution(ResignAction.NO_ACTION)
            if created:
                publisher.destroyFederationExecution(federation_name)
            second_subscriber.disconnect()
            first_subscriber.disconnect()
            publisher.disconnect()

    def test_real_jvm_multi_member_ownership_transfer_and_cancellation(self) -> None:
        owner = self.factory.getRtiAmbassador()
        acquirer = self.factory.getRtiAmbassador()
        owner_callbacks = _RecordingFederateAmbassador()
        acquirer_callbacks = _RecordingFederateAmbassador()
        federation_name = "Python multi-member ownership federation"
        created = owner_joined = acquirer_joined = False
        try:
            owner.connect(owner_callbacks, CallbackModel.HLA_EVOKED)
            acquirer.connect(acquirer_callbacks, CallbackModel.HLA_EVOKED)
            owner.createFederationExecution(
                federation_name,
                "fixture-does-not-parse-fom.xml",
                HLA_TYPES.INTEGER64_TIME,
            )
            created = True
            owner.joinFederationExecution("ownership-owner", federation_name)
            owner_joined = True
            acquirer.joinFederationExecution("ownership-acquirer", federation_name)
            acquirer_joined = True

            owner_class = owner.getObjectClassHandle(HLA_FOM.FOOD_DRINK_SODA)
            acquirer_class = acquirer.getObjectClassHandle(HLA_FOM.FOOD_DRINK_SODA)
            owner_attribute = owner.getAttributeHandle(owner_class, HLA_FIXTURES.FLAVOR)
            acquirer_attribute = acquirer.getAttributeHandle(acquirer_class, HLA_FIXTURES.FLAVOR)
            attributes = AttributeHandleSet([owner_attribute])
            acquirer_attributes = AttributeHandleSet([acquirer_attribute])
            owner.publishObjectClassAttributes(owner_class, attributes)
            acquirer.subscribeObjectClassAttributes(acquirer_class, acquirer_attributes)

            object_instance = owner.registerObjectInstance(owner_class)
            self.assertTrue(acquirer.evokeCallback(0.0))
            self.assertEqual(acquirer_callbacks.discoveries[0][0], object_instance)
            self.assertTrue(owner.isAttributeOwnedByFederate(object_instance, owner_attribute))
            self.assertFalse(
                acquirer.isAttributeOwnedByFederate(object_instance, acquirer_attribute)
            )

            acquirer.queryAttributeOwnership(object_instance, acquirer_attributes)
            self.assertTrue(acquirer.evokeCallback(0.0))
            self.assertEqual(acquirer_callbacks.ownership_reports[-1][0], object_instance)
            self.assertEqual(acquirer_callbacks.ownership_reports[-1][1], acquirer_attributes)

            acquirer.attributeOwnershipAcquisition(
                object_instance, acquirer_attributes, b"acquire-request"
            )
            while not owner_callbacks.ownership_divestiture_requests:
                self.assertTrue(owner.evokeCallback(0.0))
            self.assertEqual(
                owner_callbacks.ownership_divestiture_requests[-1][0], object_instance
            )
            self.assertEqual(owner_callbacks.ownership_divestiture_requests[-1][1], attributes)
            self.assertEqual(owner_callbacks.ownership_divestiture_requests[-1][2], b"acquire-request")
            self.assertFalse(
                acquirer_callbacks.ownership_acquisitions,
                "the negotiated request must wait for owner confirmation",
            )

            owner.confirmDivestiture(object_instance, attributes, b"confirm-transfer")
            self.assertTrue(acquirer.evokeCallback(0.0))
            self.assertEqual(
                acquirer_callbacks.ownership_acquisitions[-1],
                (object_instance, acquirer_attributes, b"confirm-transfer"),
            )
            self.assertFalse(owner.isAttributeOwnedByFederate(object_instance, owner_attribute))
            self.assertTrue(
                acquirer.isAttributeOwnedByFederate(object_instance, acquirer_attribute)
            )

            acquirer.unconditionalAttributeOwnershipDivestiture(
                object_instance, acquirer_attributes, b"release-to-rti"
            )
            owner.attributeOwnershipAcquisitionIfAvailable(
                object_instance, attributes, b"immediate-acquire"
            )
            self.assertTrue(owner.evokeCallback(0.0))
            self.assertEqual(
                owner_callbacks.ownership_acquisitions[-1],
                (object_instance, attributes, b"immediate-acquire"),
            )

            acquirer.attributeOwnershipAcquisition(
                object_instance, acquirer_attributes, b"deny-me"
            )
            self.assertTrue(owner.evokeCallback(0.0))
            owner.attributeOwnershipReleaseDenied(
                object_instance, attributes, b"deny-tag"
            )
            self.assertTrue(acquirer.evokeCallback(0.0))
            self.assertEqual(
                acquirer_callbacks.ownership_unavailable[-1],
                (object_instance, acquirer_attributes, b"deny-tag"),
            )
            self.assertTrue(owner.isAttributeOwnedByFederate(object_instance, owner_attribute))

            acquirer.attributeOwnershipAcquisition(
                object_instance, acquirer_attributes, b"cancel-me"
            )
            self.assertTrue(owner.evokeCallback(0.0))
            acquirer.cancelAttributeOwnershipAcquisition(object_instance, acquirer_attributes)
            self.assertTrue(acquirer.evokeCallback(0.0))
            self.assertEqual(
                acquirer_callbacks.ownership_acquisition_cancellations[-1],
                (object_instance, acquirer_attributes),
            )
            self.assertTrue(owner.isAttributeOwnedByFederate(object_instance, owner_attribute))
        finally:
            if acquirer_joined:
                acquirer.resignFederationExecution(ResignAction.NO_ACTION)
            if owner_joined:
                owner.resignFederationExecution(ResignAction.NO_ACTION)
            if created:
                owner.destroyFederationExecution(federation_name)
            acquirer.disconnect()
            owner.disconnect()

    def test_real_jvm_multi_member_save_restore_barrier_and_time_rewind(self) -> None:
        first = self.factory.getRtiAmbassador()
        second = self.factory.getRtiAmbassador()
        first_callbacks = _RecordingFederateAmbassador()
        second_callbacks = _RecordingFederateAmbassador()
        federation_name = "Python multi-member save-restore federation"
        created = first_joined = second_joined = False
        try:
            first.connect(first_callbacks, CallbackModel.HLA_EVOKED)
            second.connect(second_callbacks, CallbackModel.HLA_EVOKED)
            first.createFederationExecution(
                federation_name,
                "fixture-does-not-parse-fom.xml",
                HLA_TYPES.INTEGER64_TIME,
            )
            created = True
            first.joinFederationExecution("save-first", federation_name)
            first_joined = True
            second.joinFederationExecution("save-second", federation_name)
            second_joined = True

            first_time_factory = first.getTimeFactory()
            second_time_factory = second.getTimeFactory()
            first_time = first_time_factory.makeLogicalTime(3)
            second_time = second_time_factory.makeLogicalTime(5)
            first.timeAdvanceRequest(first_time)
            second.timeAdvanceRequest(second_time)
            self.assertTrue(first.evokeCallback(0.0))
            self.assertTrue(second.evokeCallback(0.0))

            save_label = "jvm-multi-member-save"
            first.requestFederationSave(save_label)
            self.assertTrue(first.evokeCallback(0.0))
            self.assertTrue(second.evokeCallback(0.0))
            self.assertEqual(first_callbacks.save_initiations, [save_label])
            self.assertEqual(second_callbacks.save_initiations, [save_label])

            first.queryFederationSaveStatus()
            self.assertTrue(first.evokeCallback(0.0))
            self.assertEqual(len(first_callbacks.save_status_reports[-1]), 2)
            self.assertTrue(
                all(
                    entry.saveStatus.name == "FEDERATE_WAITING_FOR_FEDERATION_TO_SAVE"
                    for entry in first_callbacks.save_status_reports[-1]
                )
            )

            first.federateSaveBegun()
            second.federateSaveBegun()
            first.queryFederationSaveStatus()
            self.assertTrue(first.evokeCallback(0.0))
            self.assertTrue(
                all(
                    entry.saveStatus.name == "FEDERATE_SAVING"
                    for entry in first_callbacks.save_status_reports[-1]
                )
            )
            first.federateSaveComplete()
            self.assertEqual(first_callbacks.save_completions, 0)
            second.federateSaveComplete()
            self.assertTrue(first.evokeCallback(0.0))
            self.assertTrue(second.evokeCallback(0.0))
            self.assertEqual(first_callbacks.save_completions, 1)
            self.assertEqual(second_callbacks.save_completions, 1)

            first.timeAdvanceRequest(first_time_factory.makeLogicalTime(10))
            second.timeAdvanceRequest(second_time_factory.makeLogicalTime(10))
            self.assertTrue(first.evokeCallback(0.0))
            self.assertTrue(second.evokeCallback(0.0))

            first.requestFederationRestore(save_label)
            self.assertTrue(first.evokeCallback(0.0))
            self.assertEqual(first_callbacks.restore_accepted, [save_label])
            self.assertTrue(first.evokeCallback(0.0))
            self.assertTrue(first.evokeCallback(0.0))
            self.assertTrue(second.evokeCallback(0.0))
            self.assertTrue(second.evokeCallback(0.0))
            self.assertEqual(
                first_callbacks.restore_initiations[-1][1], "save-first"
            )
            self.assertEqual(
                second_callbacks.restore_initiations[-1][1], "save-second"
            )

            second.queryFederationRestoreStatus()
            self.assertTrue(second.evokeCallback(0.0))
            self.assertEqual(len(second_callbacks.restore_status_reports[-1]), 2)
            self.assertTrue(
                all(
                    entry.status.name == "FEDERATE_RESTORING"
                    for entry in second_callbacks.restore_status_reports[-1]
                )
            )

            first.federateRestoreComplete()
            self.assertEqual(first_callbacks.restore_completions, 0)
            second.federateRestoreComplete()
            self.assertTrue(first.evokeCallback(0.0))
            self.assertTrue(second.evokeCallback(0.0))
            self.assertEqual(first_callbacks.restore_completions, 1)
            self.assertEqual(second_callbacks.restore_completions, 1)
            self.assertEqual(first.queryLogicalTime().getTime(), 3)
            self.assertEqual(second.queryLogicalTime().getTime(), 5)

            failed_save = "jvm-multi-member-failed-save"
            first.requestFederationSave(failed_save)
            self.assertTrue(first.evokeCallback(0.0))
            self.assertTrue(second.evokeCallback(0.0))
            first.federateSaveBegun()
            second.federateSaveBegun()
            first.federateSaveNotComplete()
            self.assertTrue(first.evokeCallback(0.0))
            self.assertTrue(second.evokeCallback(0.0))
            self.assertEqual(
                first_callbacks.save_failures[-1],
                SaveFailureReason.FEDERATE_REPORTED_FAILURE_DURING_SAVE,
            )
            self.assertEqual(
                second_callbacks.save_failures[-1],
                SaveFailureReason.FEDERATE_REPORTED_FAILURE_DURING_SAVE,
            )

            aborted_save = "jvm-multi-member-aborted-save"
            first.requestFederationSave(aborted_save)
            self.assertTrue(first.evokeCallback(0.0))
            self.assertTrue(second.evokeCallback(0.0))
            first.federateSaveBegun()
            second.federateSaveBegun()
            first.abortFederationSave()
            self.assertTrue(first.evokeCallback(0.0))
            self.assertTrue(second.evokeCallback(0.0))
            self.assertEqual(first_callbacks.save_failures[-1], SaveFailureReason.SAVE_ABORTED)
            self.assertEqual(second_callbacks.save_failures[-1], SaveFailureReason.SAVE_ABORTED)

            first.requestFederationRestore(save_label)
            self.assertTrue(first.evokeCallback(0.0))
            self.assertTrue(first.evokeCallback(0.0))
            self.assertTrue(first.evokeCallback(0.0))
            self.assertTrue(second.evokeCallback(0.0))
            self.assertTrue(second.evokeCallback(0.0))
            first.federateRestoreNotComplete()
            self.assertTrue(first.evokeCallback(0.0))
            self.assertTrue(second.evokeCallback(0.0))
            self.assertEqual(
                first_callbacks.restore_failures[-1],
                RestoreFailureReason.FEDERATE_REPORTED_FAILURE_DURING_RESTORE,
            )
            self.assertEqual(
                second_callbacks.restore_failures[-1],
                RestoreFailureReason.FEDERATE_REPORTED_FAILURE_DURING_RESTORE,
            )

            first.requestFederationRestore(save_label)
            self.assertTrue(first.evokeCallback(0.0))
            self.assertTrue(first.evokeCallback(0.0))
            self.assertTrue(first.evokeCallback(0.0))
            self.assertTrue(second.evokeCallback(0.0))
            self.assertTrue(second.evokeCallback(0.0))
            first.abortFederationRestore()
            self.assertTrue(first.evokeCallback(0.0))
            self.assertTrue(second.evokeCallback(0.0))
            self.assertEqual(first_callbacks.restore_failures[-1], RestoreFailureReason.RESTORE_ABORTED)
            self.assertEqual(second_callbacks.restore_failures[-1], RestoreFailureReason.RESTORE_ABORTED)
        finally:
            if second_joined:
                second.resignFederationExecution(ResignAction.NO_ACTION)
            if first_joined:
                first.resignFederationExecution(ResignAction.NO_ACTION)
            if created:
                first.destroyFederationExecution(federation_name)
            second.disconnect()
            first.disconnect()

    def test_real_jvm_multi_member_timestamped_fanout_and_retraction(self) -> None:
        publisher = self.factory.getRtiAmbassador()
        immediate_subscriber = self.factory.getRtiAmbassador()
        constrained_subscriber = self.factory.getRtiAmbassador()
        publisher_callbacks = _RecordingFederateAmbassador()
        immediate_callbacks = _RecordingFederateAmbassador()
        constrained_callbacks = _RecordingFederateAmbassador()
        federation_name = "Python multi-member timestamped federation"
        created = publisher_joined = immediate_joined = constrained_joined = False
        try:
            publisher.connect(publisher_callbacks, CallbackModel.HLA_EVOKED)
            immediate_subscriber.connect(immediate_callbacks, CallbackModel.HLA_IMMEDIATE)
            constrained_subscriber.connect(constrained_callbacks, CallbackModel.HLA_EVOKED)
            publisher.createFederationExecution(
                federation_name,
                "fixture-does-not-parse-fom.xml",
                HLA_TYPES.INTEGER64_TIME,
            )
            created = True
            publisher.joinFederationExecution("timestamped-publisher", federation_name)
            publisher_joined = True
            immediate_subscriber.joinFederationExecution(
                "timestamped-immediate", federation_name
            )
            immediate_joined = True
            constrained_subscriber.joinFederationExecution(
                "timestamped-constrained", federation_name
            )
            constrained_joined = True

            publisher_object_class = publisher.getObjectClassHandle(
                HLA_FOM.FOOD_DRINK_SODA
            )
            immediate_object_class = immediate_subscriber.getObjectClassHandle(
                HLA_FOM.FOOD_DRINK_SODA
            )
            constrained_object_class = constrained_subscriber.getObjectClassHandle(
                HLA_FOM.FOOD_DRINK_SODA
            )
            publisher_attribute = publisher.getAttributeHandle(publisher_object_class, HLA_FIXTURES.FLAVOR)
            immediate_attribute = immediate_subscriber.getAttributeHandle(
                immediate_object_class, HLA_FIXTURES.FLAVOR
            )
            constrained_attribute = constrained_subscriber.getAttributeHandle(
                constrained_object_class, HLA_FIXTURES.FLAVOR
            )
            publisher.publishObjectClassAttributes(
                publisher_object_class, AttributeHandleSet([publisher_attribute])
            )
            immediate_subscriber.subscribeObjectClassAttributes(
                immediate_object_class, AttributeHandleSet([immediate_attribute])
            )
            constrained_subscriber.subscribeObjectClassAttributes(
                constrained_object_class, AttributeHandleSet([constrained_attribute])
            )

            publisher_interaction = publisher.getInteractionClassHandle(
                HLA_FOM.SODA_SERVED
            )
            immediate_interaction = immediate_subscriber.getInteractionClassHandle(
                HLA_FOM.SODA_SERVED
            )
            constrained_interaction = constrained_subscriber.getInteractionClassHandle(
                HLA_FOM.SODA_SERVED
            )
            publisher.publishInteractionClass(publisher_interaction)
            immediate_subscriber.subscribeInteractionClass(immediate_interaction)
            constrained_subscriber.subscribeInteractionClass(constrained_interaction)

            object_instance = publisher.registerObjectInstance(publisher_object_class)
            self.assertTrue(constrained_subscriber.evokeCallback(0.0))
            self.assertEqual(constrained_callbacks.discoveries[0][0], object_instance)

            constrained_subscriber.enableTimeConstrained()
            self.assertTrue(constrained_subscriber.evokeCallback(0.0))
            time_factory = publisher.getTimeFactory()
            time_five = time_factory.makeLogicalTime(5)
            time_six = time_factory.makeLogicalTime(6)

            interaction_retraction = publisher.sendInteractionWithTime(
                publisher_interaction,
                ParameterHandleValueMap(),
                time_five,
                b"timestamped-interaction-fanout",
            )
            self.assertEqual(len(immediate_callbacks.timed_interactions), 1)
            self.assertEqual(immediate_callbacks.timed_interactions[0][6].getTime(), 5)
            self.assertEqual(immediate_callbacks.timed_interactions[0][8], OrderType.RECEIVE)
            self.assertEqual(constrained_callbacks.timed_interactions, [])

            attribute_retraction = publisher.updateAttributeValuesWithTime(
                object_instance,
                AttributeHandleValueMap({publisher_attribute: b"timestamped-flavor"}),
                time_six,
                b"timestamped-attribute-fanout",
            )
            self.assertEqual(len(immediate_callbacks.timed_reflections), 1)
            self.assertEqual(immediate_callbacks.timed_reflections[0][6].getTime(), 6)
            self.assertEqual(immediate_callbacks.timed_reflections[0][8], OrderType.RECEIVE)
            self.assertEqual(constrained_callbacks.timed_reflections, [])

            constrained_subscriber.timeAdvanceRequest(time_five)
            self.assertTrue(constrained_subscriber.evokeCallback(0.0))
            self.assertTrue(constrained_subscriber.evokeCallback(0.0))
            self.assertEqual(len(constrained_callbacks.timed_interactions), 1)
            self.assertEqual(constrained_callbacks.timed_interactions[0][0], constrained_interaction)
            self.assertEqual(constrained_callbacks.timed_interactions[0][6].getTime(), 5)
            self.assertEqual(constrained_callbacks.timed_interactions[0][8], OrderType.TIMESTAMP)
            self.assertEqual(constrained_callbacks.timed_reflections, [])

            constrained_subscriber.timeAdvanceRequest(time_six)
            self.assertTrue(constrained_subscriber.evokeCallback(0.0))
            self.assertTrue(constrained_subscriber.evokeCallback(0.0))
            self.assertEqual(len(constrained_callbacks.timed_reflections), 1)
            self.assertEqual(constrained_callbacks.timed_reflections[0][0], object_instance)
            self.assertEqual(
                constrained_callbacks.timed_reflections[0][1][constrained_attribute],
                b"timestamped-flavor",
            )
            self.assertEqual(constrained_callbacks.timed_reflections[0][6].getTime(), 6)
            self.assertEqual(constrained_callbacks.timed_reflections[0][8], OrderType.TIMESTAMP)

            dimension = publisher.getDimensionHandle(HLA_FIXTURES.SODA_FLAVOR)
            region = publisher.createRegion(DimensionHandleSet([dimension]))
            immediate_subscriber.subscribeInteractionClassWithRegions(
                immediate_interaction, RegionHandleSet([region])
            )
            constrained_subscriber.subscribeInteractionClassWithRegions(
                constrained_interaction, RegionHandleSet([region])
            )
            immediate_subscriber.setConveyRegionDesignatorSetsSwitch(True)
            regional_retraction = publisher.sendInteractionWithRegionsWithTime(
                publisher_interaction,
                ParameterHandleValueMap(),
                RegionHandleSet([region]),
                time_factory.makeLogicalTime(8),
                b"timestamped-regional-fanout",
            )
            self.assertEqual(len(immediate_callbacks.timed_interactions), 2)
            self.assertEqual(
                immediate_callbacks.timed_interactions[-1][5],
                RegionHandleSet([RegionHandle(region.encodedValue)]),
            )
            self.assertEqual(len(constrained_callbacks.timed_interactions), 1)
            constrained_subscriber.timeAdvanceRequest(time_factory.makeLogicalTime(8))
            self.assertTrue(constrained_subscriber.evokeCallback(0.0))
            self.assertTrue(constrained_subscriber.evokeCallback(0.0))
            self.assertEqual(len(constrained_callbacks.timed_interactions), 2)
            self.assertIsNone(constrained_callbacks.timed_interactions[-1][5])

            regional_pairs = AttributeSetRegionSetPairList(
                [AttributeSetRegionSetPair(
                    AttributeHandleSet([publisher_attribute]), RegionHandleSet([region])
                )]
            )
            immediate_subscriber.subscribeObjectClassAttributesWithRegions(
                immediate_object_class, regional_pairs
            )
            constrained_subscriber.subscribeObjectClassAttributesWithRegions(
                constrained_object_class, regional_pairs
            )
            publisher.associateRegionsForUpdates(object_instance, regional_pairs)
            regional_attribute_retraction = publisher.updateAttributeValuesWithTime(
                object_instance,
                AttributeHandleValueMap({publisher_attribute: b"timestamped-regional-flavor"}),
                time_factory.makeLogicalTime(10),
                b"timestamped-regional-attribute-fanout",
            )
            self.assertEqual(len(immediate_callbacks.timed_reflections), 2)
            self.assertEqual(
                immediate_callbacks.timed_reflections[-1][5],
                RegionHandleSet([RegionHandle(region.encodedValue)]),
            )
            self.assertEqual(len(constrained_callbacks.timed_reflections), 1)
            constrained_subscriber.timeAdvanceRequest(time_factory.makeLogicalTime(10))
            self.assertTrue(constrained_subscriber.evokeCallback(0.0))
            self.assertTrue(constrained_subscriber.evokeCallback(0.0))
            self.assertEqual(len(constrained_callbacks.timed_reflections), 2)
            self.assertIsNone(constrained_callbacks.timed_reflections[-1][5])

            publisher.unassociateRegionsForUpdates(object_instance, regional_pairs)
            unassociated_retraction = publisher.updateAttributeValuesWithTime(
                object_instance,
                AttributeHandleValueMap({publisher_attribute: b"timestamped-unassociated-flavor"}),
                time_factory.makeLogicalTime(12),
                b"timestamped-unassociated-attribute-fanout",
            )
            self.assertEqual(len(immediate_callbacks.timed_reflections), 3)
            self.assertIsNone(immediate_callbacks.timed_reflections[-1][5])
            constrained_subscriber.timeAdvanceRequest(time_factory.makeLogicalTime(12))
            self.assertTrue(constrained_subscriber.evokeCallback(0.0))
            self.assertTrue(constrained_subscriber.evokeCallback(0.0))
            self.assertEqual(len(constrained_callbacks.timed_reflections), 3)
            self.assertIsNone(constrained_callbacks.timed_reflections[-1][5])

            publisher_secondary_attribute = publisher.getAttributeHandle(
                publisher_object_class, HLA_FIXTURES.COLOR
            )
            immediate_secondary_attribute = immediate_subscriber.getAttributeHandle(
                immediate_object_class, HLA_FIXTURES.COLOR
            )
            constrained_secondary_attribute = constrained_subscriber.getAttributeHandle(
                constrained_object_class, HLA_FIXTURES.COLOR
            )
            publisher.publishObjectClassAttributes(
                publisher_object_class, AttributeHandleSet([publisher_secondary_attribute])
            )
            immediate_subscriber.subscribeObjectClassAttributes(
                immediate_object_class, AttributeHandleSet([immediate_secondary_attribute])
            )
            constrained_subscriber.subscribeObjectClassAttributes(
                constrained_object_class, AttributeHandleSet([constrained_secondary_attribute])
            )
            secondary_region = publisher.createRegion(DimensionHandleSet([dimension]))
            secondary_pairs = AttributeSetRegionSetPairList(
                [AttributeSetRegionSetPair(
                    AttributeHandleSet([publisher_secondary_attribute]),
                    RegionHandleSet([region, secondary_region]),
                )]
            )
            immediate_subscriber.subscribeObjectClassAttributesWithRegions(
                immediate_object_class, secondary_pairs
            )
            constrained_subscriber.subscribeObjectClassAttributesWithRegions(
                constrained_object_class, secondary_pairs
            )
            publisher.associateRegionsForUpdates(object_instance, regional_pairs)
            publisher.associateRegionsForUpdates(object_instance, secondary_pairs)
            multi_region_values = AttributeHandleValueMap({
                publisher_attribute: b"multi-flavor",
                publisher_secondary_attribute: b"multi-color",
            })
            publisher.updateAttributeValuesWithTime(
                object_instance, multi_region_values, time_factory.makeLogicalTime(14), b"multi-14"
            )
            self.assertEqual(
                immediate_callbacks.timed_reflections[-1][5],
                RegionHandleSet([
                    RegionHandle(region.encodedValue), RegionHandle(secondary_region.encodedValue)
                ]),
            )
            constrained_subscriber.timeAdvanceRequest(time_factory.makeLogicalTime(14))
            self.assertTrue(constrained_subscriber.evokeCallback(0.0))
            self.assertTrue(constrained_subscriber.evokeCallback(0.0))
            self.assertIsNone(constrained_callbacks.timed_reflections[-1][5])

            publisher.unassociateRegionsForUpdates(object_instance, regional_pairs)
            publisher.updateAttributeValuesWithTime(
                object_instance, multi_region_values, time_factory.makeLogicalTime(16), b"multi-16"
            )
            self.assertEqual(
                immediate_callbacks.timed_reflections[-1][5],
                RegionHandleSet([
                    RegionHandle(region.encodedValue), RegionHandle(secondary_region.encodedValue)
                ]),
            )
            constrained_subscriber.timeAdvanceRequest(time_factory.makeLogicalTime(16))
            self.assertTrue(constrained_subscriber.evokeCallback(0.0))
            self.assertTrue(constrained_subscriber.evokeCallback(0.0))

            publisher.unassociateRegionsForUpdates(
               object_instance,
               AttributeSetRegionSetPairList([
                  AttributeSetRegionSetPair(
                     AttributeHandleSet([publisher_secondary_attribute]),
                     RegionHandleSet([region]),
                  )
               ]),
            )
            publisher.updateAttributeValuesWithTime(
                object_instance, multi_region_values, time_factory.makeLogicalTime(18), b"multi-18"
            )
            self.assertEqual(
                immediate_callbacks.timed_reflections[-1][5],
                RegionHandleSet([RegionHandle(secondary_region.encodedValue)]),
            )
            constrained_subscriber.timeAdvanceRequest(time_factory.makeLogicalTime(18))
            self.assertTrue(constrained_subscriber.evokeCallback(0.0))
            self.assertTrue(constrained_subscriber.evokeCallback(0.0))

            publisher.unassociateRegionsForUpdates(
                object_instance,
                AttributeSetRegionSetPairList([
                    AttributeSetRegionSetPair(
                     AttributeHandleSet([publisher_secondary_attribute]),
                     RegionHandleSet([secondary_region]),
                    )
                ]),
            )
            publisher.updateAttributeValuesWithTime(
                object_instance, multi_region_values, time_factory.makeLogicalTime(20), b"multi-20"
            )
            self.assertIsNone(immediate_callbacks.timed_reflections[-1][5])
            constrained_subscriber.timeAdvanceRequest(time_factory.makeLogicalTime(20))
            self.assertTrue(constrained_subscriber.evokeCallback(0.0))
            self.assertTrue(constrained_subscriber.evokeCallback(0.0))
            self.assertIsNone(constrained_callbacks.timed_reflections[-1][5])

            pending_retraction = publisher.sendInteractionWithTime(
                publisher_interaction,
                ParameterHandleValueMap(),
                time_factory.makeLogicalTime(21),
                b"timestamped-pending-retraction",
            )
            self.assertEqual(len(immediate_callbacks.timed_interactions), 3)
            self.assertEqual(constrained_callbacks.timed_interactions[0][6].getTime(), 5)
            self.assertEqual(len(constrained_callbacks.timed_interactions), 2)
            publisher.retract(pending_retraction)

            publisher.retract(interaction_retraction)
            publisher.retract(attribute_retraction)
            publisher.retract(regional_retraction)
            publisher.retract(regional_attribute_retraction)
            publisher.retract(unassociated_retraction)
            self.assertEqual(len(immediate_callbacks.request_retractions), 6)
            self.assertEqual(len(constrained_callbacks.request_retractions), 0)
            self.assertTrue(constrained_subscriber.evokeCallback(0.0))
            self.assertTrue(constrained_subscriber.evokeCallback(0.0))
            self.assertTrue(constrained_subscriber.evokeCallback(0.0))
            self.assertTrue(constrained_subscriber.evokeCallback(0.0))
            self.assertTrue(constrained_subscriber.evokeCallback(0.0))
            self.assertEqual(len(constrained_callbacks.request_retractions), 5)
            self.assertEqual(
                {
                    entry.encodedValue for entry in constrained_callbacks.request_retractions
                },
                {
                    entry.encodedValue
                    for entry in immediate_callbacks.request_retractions
                    if entry.encodedValue != pending_retraction.encodedValue
                },
            )
            self.assertNotIn(
                pending_retraction.encodedValue,
                {entry.encodedValue for entry in constrained_callbacks.request_retractions},
            )
        finally:
            if constrained_joined:
                constrained_subscriber.resignFederationExecution(ResignAction.NO_ACTION)
            if immediate_joined:
                immediate_subscriber.resignFederationExecution(ResignAction.NO_ACTION)
            if publisher_joined:
                publisher.resignFederationExecution(ResignAction.NO_ACTION)
            if created:
                publisher.destroyFederationExecution(federation_name)
            constrained_subscriber.disconnect()
            immediate_subscriber.disconnect()
            publisher.disconnect()

    def test_real_jvm_regional_interaction_overlap_filters_recipients(self) -> None:
        publisher = self.factory.getRtiAmbassador()
        overlap_subscriber = self.factory.getRtiAmbassador()
        disjoint_subscriber = self.factory.getRtiAmbassador()
        publisher_callbacks = _RecordingFederateAmbassador()
        overlap_callbacks = _RecordingFederateAmbassador()
        disjoint_callbacks = _RecordingFederateAmbassador()
        federation_name = "Python regional overlap federation"
        created = publisher_joined = overlap_joined = disjoint_joined = False
        try:
            publisher.connect(publisher_callbacks, CallbackModel.HLA_IMMEDIATE)
            overlap_subscriber.connect(overlap_callbacks, CallbackModel.HLA_IMMEDIATE)
            disjoint_subscriber.connect(disjoint_callbacks, CallbackModel.HLA_IMMEDIATE)
            publisher.createFederationExecution(
                federation_name,
                "fixture-does-not-parse-fom.xml",
                HLA_TYPES.INTEGER64_TIME,
            )
            created = True
            publisher.joinFederationExecution("overlap-publisher", federation_name)
            publisher_joined = True
            overlap_subscriber.joinFederationExecution("overlap-subscriber", federation_name)
            overlap_joined = True
            disjoint_subscriber.joinFederationExecution("disjoint-subscriber", federation_name)
            disjoint_joined = True

            publisher_interaction = publisher.getInteractionClassHandle(
                HLA_FOM.SODA_SERVED
            )
            overlap_interaction = overlap_subscriber.getInteractionClassHandle(
                HLA_FOM.SODA_SERVED
            )
            disjoint_interaction = disjoint_subscriber.getInteractionClassHandle(
                HLA_FOM.SODA_SERVED
            )
            publisher.publishInteractionClass(publisher_interaction)

            publisher_dimension = publisher.getDimensionHandle(HLA_FIXTURES.SODA_FLAVOR)
            overlap_dimension = overlap_subscriber.getDimensionHandle(HLA_FIXTURES.SODA_FLAVOR)
            disjoint_dimension = disjoint_subscriber.getDimensionHandle(HLA_FIXTURES.SODA_FLAVOR)
            publisher_region = publisher.createRegion(DimensionHandleSet([publisher_dimension]))
            overlap_region = overlap_subscriber.createRegion(DimensionHandleSet([overlap_dimension]))
            disjoint_region = disjoint_subscriber.createRegion(
                DimensionHandleSet([disjoint_dimension])
            )
            publisher.setRangeBounds(publisher_region, publisher_dimension, RangeBounds(0, 10))
            overlap_subscriber.setRangeBounds(overlap_region, overlap_dimension, RangeBounds(5, 15))
            disjoint_subscriber.setRangeBounds(
                disjoint_region, disjoint_dimension, RangeBounds(20, 30)
            )
            overlap_subscriber.subscribeInteractionClassWithRegions(
                overlap_interaction, RegionHandleSet([overlap_region])
            )
            disjoint_subscriber.subscribeInteractionClassWithRegions(
                disjoint_interaction, RegionHandleSet([disjoint_region])
            )
            overlap_subscriber.setConveyRegionDesignatorSetsSwitch(True)
            disjoint_subscriber.setConveyRegionDesignatorSetsSwitch(True)

            publisher.sendInteractionWithRegions(
                publisher_interaction,
                ParameterHandleValueMap(),
                RegionHandleSet([publisher_region]),
                b"regional-overlap-receive",
            )
            self.assertEqual(len(overlap_callbacks.interactions), 1)
            self.assertEqual(disjoint_callbacks.interactions, [])
            self.assertEqual(
                overlap_callbacks.sent_regions[-1],
                RegionHandleSet([RegionHandle(overlap_region.encodedValue)]),
            )

            timed_retraction = publisher.sendInteractionWithRegionsWithTime(
                publisher_interaction,
                ParameterHandleValueMap(),
                RegionHandleSet([publisher_region]),
                publisher.getTimeFactory().makeLogicalTime(5),
                b"regional-overlap-timed",
            )
            self.assertEqual(len(overlap_callbacks.timed_interactions), 1)
            self.assertEqual(disjoint_callbacks.timed_interactions, [])
            self.assertEqual(overlap_callbacks.timed_interactions[-1][6].getTime(), 5)
            publisher.retract(timed_retraction)
            self.assertEqual(len(overlap_callbacks.request_retractions), 1)
            self.assertEqual(disjoint_callbacks.request_retractions, [])

            publisher_object_class = publisher.getObjectClassHandle(
                HLA_FOM.FOOD_DRINK_SODA
            )
            overlap_object_class = overlap_subscriber.getObjectClassHandle(
                HLA_FOM.FOOD_DRINK_SODA
            )
            disjoint_object_class = disjoint_subscriber.getObjectClassHandle(
                HLA_FOM.FOOD_DRINK_SODA
            )
            publisher_attribute = publisher.getAttributeHandle(publisher_object_class, HLA_FIXTURES.FLAVOR)
            overlap_attribute = overlap_subscriber.getAttributeHandle(
                overlap_object_class, HLA_FIXTURES.FLAVOR
            )
            disjoint_attribute = disjoint_subscriber.getAttributeHandle(
                disjoint_object_class, HLA_FIXTURES.FLAVOR
            )
            publisher.publishObjectClassAttributes(
                publisher_object_class, AttributeHandleSet([publisher_attribute])
            )
            overlap_pairs = AttributeSetRegionSetPairList([
                AttributeSetRegionSetPair(
                    AttributeHandleSet([overlap_attribute]), RegionHandleSet([overlap_region])
                )
            ])
            disjoint_pairs = AttributeSetRegionSetPairList([
                AttributeSetRegionSetPair(
                    AttributeHandleSet([disjoint_attribute]), RegionHandleSet([disjoint_region])
                )
            ])
            overlap_subscriber.subscribeObjectClassAttributesWithRegions(
                overlap_object_class, overlap_pairs
            )
            disjoint_subscriber.subscribeObjectClassAttributesWithRegions(
                disjoint_object_class, disjoint_pairs
            )
            source_pairs = AttributeSetRegionSetPairList([
                AttributeSetRegionSetPair(
                    AttributeHandleSet([publisher_attribute]), RegionHandleSet([publisher_region])
                )
            ])
            object_instance = publisher.registerObjectInstanceWithRegions(
                publisher_object_class, source_pairs
            )
            self.assertEqual(len(overlap_callbacks.discoveries), 1)
            self.assertEqual(disjoint_callbacks.discoveries, [])
            publisher.updateAttributeValues(
                object_instance,
                AttributeHandleValueMap({publisher_attribute: b"regional-object-receive"}),
                b"regional-object-receive-tag",
            )
            self.assertEqual(len(overlap_callbacks.reflections), 1)
            self.assertEqual(disjoint_callbacks.reflections, [])
            self.assertEqual(
                overlap_callbacks.reflections[-1][1][overlap_attribute],
                b"regional-object-receive",
            )

            object_retraction = publisher.updateAttributeValuesWithTime(
                object_instance,
                AttributeHandleValueMap({publisher_attribute: b"regional-object-timed"}),
                publisher.getTimeFactory().makeLogicalTime(5),
                b"regional-object-timed-tag",
            )
            self.assertEqual(len(overlap_callbacks.timed_reflections), 1)
            self.assertEqual(disjoint_callbacks.timed_reflections, [])
            self.assertEqual(overlap_callbacks.timed_reflections[-1][6].getTime(), 5)
            publisher.retract(object_retraction)
            self.assertEqual(len(overlap_callbacks.request_retractions), 2)
            self.assertEqual(disjoint_callbacks.request_retractions, [])
        finally:
            if disjoint_joined:
                disjoint_subscriber.resignFederationExecution(ResignAction.NO_ACTION)
            if overlap_joined:
                overlap_subscriber.resignFederationExecution(ResignAction.NO_ACTION)
            if publisher_joined:
                publisher.resignFederationExecution(ResignAction.NO_ACTION)
            if created:
                publisher.destroyFederationExecution(federation_name)
            disjoint_subscriber.disconnect()
            overlap_subscriber.disconnect()
            publisher.disconnect()

    def test_real_jvm_regional_interaction_subscription_reprojects(self) -> None:
        publisher = self.factory.getRtiAmbassador()
        subscriber = self.factory.getRtiAmbassador()
        publisher_callbacks = _RecordingFederateAmbassador()
        subscriber_callbacks = _RecordingFederateAmbassador()
        federation_name = "Python regional interaction subscription reprojection federation"
        created = publisher_joined = subscriber_joined = False
        try:
            publisher.connect(publisher_callbacks, CallbackModel.HLA_EVOKED)
            subscriber.connect(subscriber_callbacks, CallbackModel.HLA_EVOKED)
            publisher.createFederationExecution(
                federation_name,
                "fixture-does-not-parse-fom.xml",
                HLA_TYPES.INTEGER64_TIME,
            )
            created = True
            publisher.joinFederationExecution("interaction-reprojection-publisher", federation_name)
            publisher_joined = True
            subscriber.joinFederationExecution("interaction-reprojection-subscriber", federation_name)
            subscriber_joined = True

            publisher_interaction = publisher.getInteractionClassHandle(
                HLA_FOM.MAIN_COURSE_SERVED
            )
            subscriber_interaction = subscriber.getInteractionClassHandle(
                HLA_FOM.MAIN_COURSE_SERVED
            )
            publisher_parameter = publisher.getParameterHandle(
                publisher_interaction, HLA_FIXTURES.TEMPERATURE_OK
            )
            publisher.publishInteractionClass(publisher_interaction)
            publisher_dimension = publisher.getDimensionHandle(HLA_FIXTURES.SERVER_ID)
            subscriber_dimension = subscriber.getDimensionHandle(HLA_FIXTURES.SERVER_ID)
            publisher_region = publisher.createRegion(DimensionHandleSet([publisher_dimension]))
            disjoint_region = subscriber.createRegion(DimensionHandleSet([subscriber_dimension]))
            overlap_region = subscriber.createRegion(DimensionHandleSet([subscriber_dimension]))
            overlap_region_two = subscriber.createRegion(
                DimensionHandleSet([subscriber_dimension])
            )
            publisher.setRangeBounds(publisher_region, publisher_dimension, RangeBounds(0, 5))
            subscriber.setRangeBounds(disjoint_region, subscriber_dimension, RangeBounds(10, 15))
            subscriber.setRangeBounds(overlap_region, subscriber_dimension, RangeBounds(2, 7))
            subscriber.setRangeBounds(
                overlap_region_two, subscriber_dimension, RangeBounds(2, 7)
            )
            publisher.commitRegionModifications(RegionHandleSet([publisher_region]))
            subscriber.commitRegionModifications(
                RegionHandleSet([disjoint_region, overlap_region, overlap_region_two])
            )

            subscriber.subscribeInteractionClassWithRegions(
                subscriber_interaction, RegionHandleSet([disjoint_region])
            )
            publisher.sendInteractionWithRegions(
                publisher_interaction,
                ParameterHandleValueMap({publisher_parameter: b"outside"}),
                RegionHandleSet([publisher_region]),
                b"outside-tag",
            )
            subscriber.evokeCallback(0.0)
            self.assertEqual(subscriber_callbacks.interactions, [])

            subscriber.subscribeInteractionClassWithRegions(
                subscriber_interaction, RegionHandleSet([overlap_region])
            )
            publisher.sendInteractionWithRegions(
                publisher_interaction,
                ParameterHandleValueMap({publisher_parameter: b"inside"}),
                RegionHandleSet([publisher_region]),
                b"inside-tag",
            )
            self.assertTrue(subscriber.evokeCallback(0.0))
            self.assertEqual(len(subscriber_callbacks.interactions), 1)
            self.assertEqual(
                subscriber_callbacks.interactions[0][1][publisher_parameter], b"inside"
            )

            subscriber.subscribeInteractionClassWithRegions(
                subscriber_interaction, RegionHandleSet([overlap_region_two])
            )
            publisher.sendInteractionWithRegions(
                publisher_interaction,
                ParameterHandleValueMap({publisher_parameter: b"inside-second"}),
                RegionHandleSet([publisher_region]),
                b"inside-second-tag",
            )
            self.assertTrue(subscriber.evokeCallback(0.0))
            self.assertEqual(len(subscriber_callbacks.interactions), 2)

            subscriber.unsubscribeInteractionClassWithRegions(
                subscriber_interaction, RegionHandleSet([overlap_region])
            )
            publisher.sendInteractionWithRegions(
                publisher_interaction,
                ParameterHandleValueMap({publisher_parameter: b"inside-still"}),
                RegionHandleSet([publisher_region]),
                b"inside-still-tag",
            )
            self.assertTrue(subscriber.evokeCallback(0.0))
            self.assertEqual(len(subscriber_callbacks.interactions), 3)

            subscriber.unsubscribeInteractionClassWithRegions(
                subscriber_interaction, RegionHandleSet([overlap_region_two])
            )
            publisher.sendInteractionWithRegions(
                publisher_interaction,
                ParameterHandleValueMap({publisher_parameter: b"outside-again"}),
                RegionHandleSet([publisher_region]),
                b"outside-again-tag",
            )
            subscriber.evokeCallback(0.0)
            self.assertEqual(len(subscriber_callbacks.interactions), 3)
        finally:
            if subscriber_joined:
                subscriber.resignFederationExecution(ResignAction.NO_ACTION)
            if publisher_joined:
                publisher.resignFederationExecution(ResignAction.NO_ACTION)
            if created:
                publisher.destroyFederationExecution(federation_name)
            subscriber.disconnect()
            publisher.disconnect()

    def test_real_jvm_relaxed_ddm_allows_boundary_touching_regional_delivery(self) -> None:
        federation_name = "Python relaxed DDM boundary federation"
        publisher = self.factory.getRtiAmbassador()
        subscriber = self.factory.getRtiAmbassador()
        publisher_callbacks = _RecordingFederateAmbassador()
        subscriber_callbacks = _RecordingFederateAmbassador()
        created = publisher_joined = subscriber_joined = False
        try:
            publisher.connect(publisher_callbacks, CallbackModel.HLA_EVOKED)
            subscriber.connect(subscriber_callbacks, CallbackModel.HLA_EVOKED)
            publisher.createFederationExecution(
                federation_name,
                "fixture-does-not-parse-fom.xml",
                HLA_TYPES.INTEGER64_TIME,
            )
            created = True
            publisher.joinFederationExecution("relaxed-ddm-publisher", federation_name)
            publisher_joined = True
            subscriber.joinFederationExecution("relaxed-ddm-subscriber", federation_name)
            subscriber_joined = True

            interaction = publisher.getInteractionClassHandle(
                HLA_FOM.MAIN_COURSE_SERVED
            )
            parameter = publisher.getParameterHandle(interaction, HLA_FIXTURES.TEMPERATURE_OK)
            dimension = publisher.getDimensionHandle(HLA_FIXTURES.SODA_FLAVOR)
            publisher.publishInteractionClass(interaction)
            publisher_region = publisher.createRegion(DimensionHandleSet([dimension]))
            subscriber_region = subscriber.createRegion(
                DimensionHandleSet([subscriber.getDimensionHandle(HLA_FIXTURES.SODA_FLAVOR)])
            )
            publisher.setRangeBounds(publisher_region, dimension, RangeBounds(0, 1))
            subscriber.setRangeBounds(
                subscriber_region,
                subscriber.getDimensionHandle(HLA_FIXTURES.SODA_FLAVOR),
                RangeBounds(1, 2),
            )
            publisher.commitRegionModifications(RegionHandleSet([publisher_region]))
            subscriber.commitRegionModifications(RegionHandleSet([subscriber_region]))
            subscriber.subscribeInteractionClassWithRegions(
                interaction, RegionHandleSet([subscriber_region])
            )
            self.assertFalse(subscriber.getAllowRelaxedDDMSwitch())

            publisher.sendInteractionWithRegions(
                interaction,
                ParameterHandleValueMap({parameter: b"strict-boundary"}),
                RegionHandleSet([publisher_region]),
                b"strict-boundary-tag",
            )
            self.assertFalse(subscriber.evokeCallback(0.0))
            self.assertEqual(subscriber_callbacks.interactions, [])

            publisher.unwrap_java_object().enableAllowRelaxedDDM()
            self.assertTrue(subscriber.getAllowRelaxedDDMSwitch())
            publisher.sendInteractionWithRegions(
                interaction,
                ParameterHandleValueMap({parameter: b"relaxed-boundary"}),
                RegionHandleSet([publisher_region]),
                b"relaxed-boundary-tag",
            )
            self.assertTrue(subscriber.evokeCallback(0.0))
            self.assertEqual(len(subscriber_callbacks.interactions), 1)
            self.assertEqual(
                subscriber_callbacks.interactions[-1][1][parameter], b"relaxed-boundary"
            )
            self.assertEqual(subscriber_callbacks.interactions[-1][2], b"relaxed-boundary-tag")

            publisher_class = publisher.getObjectClassHandle(HLA_FOM.FOOD_DRINK_SODA)
            subscriber_class = subscriber.getObjectClassHandle(HLA_FOM.FOOD_DRINK_SODA)
            publisher_attribute = publisher.getAttributeHandle(publisher_class, HLA_FIXTURES.FLAVOR)
            subscriber_attribute = subscriber.getAttributeHandle(subscriber_class, HLA_FIXTURES.FLAVOR)
            publisher.publishObjectClassAttributes(
                publisher_class, AttributeHandleSet([publisher_attribute])
            )
            subscriber_pairs = AttributeSetRegionSetPairList(
                [
                    AttributeSetRegionSetPair(
                        AttributeHandleSet([subscriber_attribute]),
                        RegionHandleSet([subscriber_region]),
                    )
                ]
            )
            source_pairs = AttributeSetRegionSetPairList(
                [
                    AttributeSetRegionSetPair(
                        AttributeHandleSet([publisher_attribute]),
                        RegionHandleSet([publisher_region]),
                    )
                ]
            )
            subscriber.subscribeObjectClassAttributesWithRegions(
                subscriber_class, subscriber_pairs
            )
            object_instance = publisher.registerObjectInstance(publisher_class)
            self.assertTrue(subscriber.evokeCallback(0.0))
            self.assertEqual(len(subscriber_callbacks.discoveries), 1)
            self.assertEqual(subscriber_callbacks.discoveries[-1][0], object_instance)
            publisher.associateRegionsForUpdates(object_instance, source_pairs)
            publisher.updateAttributeValues(
                object_instance,
                AttributeHandleValueMap({publisher_attribute: b"relaxed-object-boundary"}),
                b"relaxed-object-boundary-tag",
            )
            self.assertTrue(subscriber.evokeCallback(0.0))
            self.assertEqual(len(subscriber_callbacks.reflections), 1)
            self.assertEqual(
                subscriber_callbacks.reflections[-1][1][subscriber_attribute],
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

    def test_real_jvm_default_region_object_callbacks_convey_empty_region_set(self) -> None:
        class DefaultRegionCallbacks(FederateAmbassador):
            def __init__(self) -> None:
                self.discoveries: list[tuple[object, ...]] = []
                self.reflections: list[tuple[object, ...]] = []

            def discoverObjectInstance(self, *arguments) -> None:
                self.discoveries.append(arguments)

            def reflectAttributeValues(self, *arguments) -> None:
                self.reflections.append(arguments)

        federation_name = "Python default region object federation"
        publisher = self.factory.getRtiAmbassador()
        subscriber = self.factory.getRtiAmbassador()
        publisher_callbacks = _RecordingFederateAmbassador()
        subscriber_callbacks = DefaultRegionCallbacks()
        created = publisher_joined = subscriber_joined = False
        try:
            publisher.connect(publisher_callbacks, CallbackModel.HLA_EVOKED)
            subscriber.connect(subscriber_callbacks, CallbackModel.HLA_EVOKED)
            publisher.createFederationExecution(
                federation_name,
                "fixture-does-not-parse-fom.xml",
                HLA_TYPES.INTEGER64_TIME,
            )
            created = True
            publisher.joinFederationExecution("default-region-publisher", federation_name)
            publisher_joined = True
            subscriber.joinFederationExecution("default-region-subscriber", federation_name)
            subscriber_joined = True

            publisher_class = publisher.getObjectClassHandle(HLA_FOM.FOOD_DRINK_SODA)
            publisher_attribute = publisher.getAttributeHandle(publisher_class, HLA_FIXTURES.FLAVOR)
            subscriber_class = subscriber.getObjectClassHandle(HLA_FOM.FOOD_DRINK_SODA)
            subscriber_attribute = subscriber.getAttributeHandle(subscriber_class, HLA_FIXTURES.FLAVOR)
            dimension = publisher.getDimensionHandle(HLA_FIXTURES.SODA_FLAVOR)
            publisher.publishObjectClassAttributes(
                publisher_class, AttributeHandleSet([publisher_attribute])
            )
            subscriber_region = subscriber.createRegion(
                DimensionHandleSet([subscriber.getDimensionHandle(HLA_FIXTURES.SODA_FLAVOR)])
            )
            subscriber.setRangeBounds(
                subscriber_region,
                subscriber.getDimensionHandle(HLA_FIXTURES.SODA_FLAVOR),
                RangeBounds(1, 3),
            )
            subscriber.commitRegionModifications(RegionHandleSet([subscriber_region]))
            subscriber_pairs = AttributeSetRegionSetPairList([
                AttributeSetRegionSetPair(
                    AttributeHandleSet([subscriber_attribute]),
                    RegionHandleSet([subscriber_region]),
                )
            ])
            subscriber.subscribeObjectClassAttributesWithRegions(
                subscriber_class, subscriber_pairs
            )
            subscriber.setConveyRegionDesignatorSetsSwitch(True)
            object_instance = publisher.registerObjectInstance(publisher_class)
            self.assertTrue(subscriber.evokeCallback(0.0))
            self.assertEqual(len(subscriber_callbacks.discoveries), 1)
            self.assertEqual(subscriber_callbacks.discoveries[-1][0], object_instance)

            publisher.updateAttributeValues(
                object_instance,
                AttributeHandleValueMap({publisher_attribute: b"default-receive"}),
                b"default-receive-tag",
            )
            self.assertTrue(subscriber.evokeCallback(0.0))
            self.assertEqual(len(subscriber_callbacks.reflections), 1)
            receive_reflection = subscriber_callbacks.reflections[-1]
            self.assertEqual(receive_reflection[1][subscriber_attribute], b"default-receive")
            self.assertEqual(receive_reflection[5], RegionHandleSet())

            time_factory = publisher.getTimeFactory()
            publisher.enableTimeRegulation(time_factory.makeLogicalTimeInterval(1))
            self.assertTrue(publisher.evokeCallback(0.0))
            retraction = publisher.updateAttributeValuesWithTime(
                object_instance,
                AttributeHandleValueMap({publisher_attribute: b"default-timestamped"}),
                time_factory.makeLogicalTime(5),
                b"default-timestamped-tag",
            )
            self.assertTrue(retraction.isValid())
            self.assertTrue(subscriber.evokeCallback(0.0))
            self.assertEqual(len(subscriber_callbacks.reflections), 2)
            timed_reflection = subscriber_callbacks.reflections[-1]
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

    def test_real_jvm_regional_delayed_subscription_rechecks_at_callback_boundary(self) -> None:
        federation_name = "Python regional delayed subscription federation"
        publisher = self.factory.getRtiAmbassador()
        subscriber = self.factory.getRtiAmbassador()
        publisher_callbacks = _RecordingFederateAmbassador()
        subscriber_callbacks = _RecordingFederateAmbassador()
        created = publisher_joined = subscriber_joined = False
        try:
            publisher.connect(publisher_callbacks, CallbackModel.HLA_EVOKED)
            subscriber.connect(subscriber_callbacks, CallbackModel.HLA_EVOKED)
            publisher.createFederationExecution(
                federation_name,
                "fixture-does-not-parse-fom.xml",
                HLA_TYPES.INTEGER64_TIME,
            )
            created = True
            publisher.joinFederationExecution("regional-delayed-publisher", federation_name)
            publisher_joined = True
            subscriber.joinFederationExecution("regional-delayed-subscriber", federation_name)
            subscriber_joined = True

            interaction = publisher.getInteractionClassHandle(
                HLA_FOM.MAIN_COURSE_SERVED
            )
            parameter = publisher.getParameterHandle(interaction, HLA_FIXTURES.TEMPERATURE_OK)
            dimension = publisher.getDimensionHandle(HLA_FIXTURES.SODA_FLAVOR)
            publisher.publishInteractionClass(interaction)
            publisher_region = publisher.createRegion(DimensionHandleSet([dimension]))
            subscriber_region = subscriber.createRegion(
                DimensionHandleSet([subscriber.getDimensionHandle(HLA_FIXTURES.SODA_FLAVOR)])
            )
            publisher.setRangeBounds(publisher_region, dimension, RangeBounds(0, 2))
            subscriber.setRangeBounds(
                subscriber_region,
                subscriber.getDimensionHandle(HLA_FIXTURES.SODA_FLAVOR),
                RangeBounds(20, 30),
            )
            publisher.commitRegionModifications(RegionHandleSet([publisher_region]))
            subscriber.commitRegionModifications(RegionHandleSet([subscriber_region]))
            subscriber.subscribeInteractionClassWithRegions(
                interaction,
                RegionHandleSet([subscriber_region]),
            )
            subscriber.unwrap_java_object().enableDelayedSubscriptionEvaluation()
            self.assertTrue(subscriber.getDelaySubscriptionEvaluationSwitch())

            publisher.sendInteractionWithRegions(
                interaction,
                ParameterHandleValueMap({parameter: b"delayed-regional"}),
                RegionHandleSet([publisher_region]),
                b"delayed-regional-tag",
            )
            self.assertEqual(subscriber_callbacks.interactions, [])

            # The declaration was evaluated when the service was accepted, but
            # the delayed policy rechecks the committed region overlap at the
            # actual callback boundary.
            subscriber.setRangeBounds(
                subscriber_region,
                subscriber.getDimensionHandle(HLA_FIXTURES.SODA_FLAVOR),
                RangeBounds(1, 3),
            )
            subscriber.commitRegionModifications(RegionHandleSet([subscriber_region]))
            self.assertTrue(subscriber.evokeCallback(0.0))
            self.assertEqual(len(subscriber_callbacks.interactions), 1)
            self.assertEqual(
                subscriber_callbacks.interactions[-1][1][parameter], b"delayed-regional"
            )
            self.assertEqual(subscriber_callbacks.interactions[-1][2], b"delayed-regional-tag")
        finally:
            if subscriber_joined:
                subscriber.resignFederationExecution(ResignAction.NO_ACTION)
            if publisher_joined:
                publisher.resignFederationExecution(ResignAction.NO_ACTION)
            if created:
                publisher.destroyFederationExecution(federation_name)
            subscriber.disconnect()
            publisher.disconnect()

    def test_real_jvm_regional_delayed_object_update_rechecks_at_callback_boundary(self) -> None:
        federation_name = "Python regional delayed object federation"
        publisher = self.factory.getRtiAmbassador()
        subscriber = self.factory.getRtiAmbassador()
        publisher_callbacks = _RecordingFederateAmbassador()
        subscriber_callbacks = _RecordingFederateAmbassador()
        created = publisher_joined = subscriber_joined = False
        try:
            publisher.connect(publisher_callbacks, CallbackModel.HLA_EVOKED)
            subscriber.connect(subscriber_callbacks, CallbackModel.HLA_EVOKED)
            publisher.createFederationExecution(
                federation_name,
                "fixture-does-not-parse-fom.xml",
                HLA_TYPES.INTEGER64_TIME,
            )
            created = True
            publisher.joinFederationExecution("regional-delayed-object-publisher", federation_name)
            publisher_joined = True
            subscriber.joinFederationExecution("regional-delayed-object-subscriber", federation_name)
            subscriber_joined = True

            publisher_class = publisher.getObjectClassHandle(
                HLA_FOM.FOOD_DRINK_SODA
            )
            subscriber_class = subscriber.getObjectClassHandle(
                HLA_FOM.FOOD_DRINK_SODA
            )
            publisher_attribute = publisher.getAttributeHandle(publisher_class, HLA_FIXTURES.FLAVOR)
            subscriber_attribute = subscriber.getAttributeHandle(subscriber_class, HLA_FIXTURES.FLAVOR)
            publisher_dimension = publisher.getDimensionHandle(HLA_FIXTURES.SODA_FLAVOR)
            subscriber_dimension = subscriber.getDimensionHandle(HLA_FIXTURES.SODA_FLAVOR)
            publisher_attributes = AttributeHandleSet([publisher_attribute])
            subscriber_attributes = AttributeHandleSet([subscriber_attribute])
            publisher.publishObjectClassAttributes(publisher_class, publisher_attributes)
            publisher_region = publisher.createRegion(DimensionHandleSet([publisher_dimension]))
            subscriber_region = subscriber.createRegion(DimensionHandleSet([subscriber_dimension]))
            publisher.setRangeBounds(publisher_region, publisher_dimension, RangeBounds(0, 2))
            subscriber.setRangeBounds(subscriber_region, subscriber_dimension, RangeBounds(1, 3))
            publisher.commitRegionModifications(RegionHandleSet([publisher_region]))
            subscriber.commitRegionModifications(RegionHandleSet([subscriber_region]))
            publisher_pairs = AttributeSetRegionSetPairList(
                [AttributeSetRegionSetPair(publisher_attributes, RegionHandleSet([publisher_region]))]
            )
            subscriber_pairs = AttributeSetRegionSetPairList(
                [AttributeSetRegionSetPair(subscriber_attributes, RegionHandleSet([subscriber_region]))]
            )
            subscriber.subscribeObjectClassAttributesWithRegions(
                subscriber_class, subscriber_pairs
            )
            object_instance = publisher.registerObjectInstanceWithRegions(
                publisher_class, publisher_pairs
            )
            subscriber.evokeCallback(0.0)
            self.assertEqual(len(subscriber_callbacks.discoveries), 1)
            subscriber.unwrap_java_object().enableDelayedSubscriptionEvaluation()
            self.assertTrue(subscriber.getDelaySubscriptionEvaluationSwitch())

            subscriber.setRangeBounds(subscriber_region, subscriber_dimension, RangeBounds(20, 30))
            publisher.updateAttributeValues(
                object_instance,
                AttributeHandleValueMap({publisher_attribute: b"delayed-object"}),
                b"delayed-object-tag",
            )
            self.assertEqual(subscriber_callbacks.reflections, [])

            subscriber.setRangeBounds(subscriber_region, subscriber_dimension, RangeBounds(1, 3))
            subscriber.commitRegionModifications(RegionHandleSet([subscriber_region]))
            subscriber.evokeCallback(0.0)
            self.assertEqual(len(subscriber_callbacks.reflections), 1)
            self.assertEqual(
                subscriber_callbacks.reflections[-1][1][subscriber_attribute],
                b"delayed-object",
            )
            self.assertEqual(subscriber_callbacks.reflections[-1][2], b"delayed-object-tag")
        finally:
            if subscriber_joined:
                subscriber.resignFederationExecution(ResignAction.NO_ACTION)
            if publisher_joined:
                publisher.resignFederationExecution(ResignAction.NO_ACTION)
            if created:
                publisher.destroyFederationExecution(federation_name)
            subscriber.disconnect()
            publisher.disconnect()

    def test_real_jvm_timestamped_regional_association_rechecks_at_callback_boundary(self) -> None:
        """The Java adapter preserves delayed TSO association re-projection.

        This mirrors the native binding's association-change vector: the
        recipient is eligible when the timestamped update is accepted, loses
        overlap before its grant, and therefore must not receive a stale
        reflection.  A later re-association makes the next timed update
        eligible again.
        """

        federation_name = "Python timed regional association federation"
        publisher = self.factory.getRtiAmbassador()
        subscriber = self.factory.getRtiAmbassador()
        publisher_callbacks = _RecordingFederateAmbassador()
        subscriber_callbacks = _RecordingFederateAmbassador()
        created = publisher_joined = subscriber_joined = False

        def pump() -> None:
            for _ in range(6):
                publisher.evokeCallback(0.0)
                subscriber.evokeCallback(0.0)

        try:
            publisher.connect(publisher_callbacks, CallbackModel.HLA_EVOKED)
            subscriber.connect(subscriber_callbacks, CallbackModel.HLA_EVOKED)
            publisher.createFederationExecution(
                federation_name,
                "fixture-does-not-parse-fom.xml",
                HLA_TYPES.INTEGER64_TIME,
            )
            created = True
            publisher.joinFederationExecution("timed-association-publisher", federation_name)
            publisher_joined = True
            subscriber.joinFederationExecution("timed-association-subscriber", federation_name)
            subscriber_joined = True

            publisher_class = publisher.getObjectClassHandle(HLA_FOM.FOOD_DRINK_SODA)
            subscriber_class = subscriber.getObjectClassHandle(HLA_FOM.FOOD_DRINK_SODA)
            publisher_attribute = publisher.getAttributeHandle(publisher_class, HLA_FIXTURES.FLAVOR)
            subscriber_attribute = subscriber.getAttributeHandle(subscriber_class, HLA_FIXTURES.FLAVOR)
            publisher_attributes = AttributeHandleSet([publisher_attribute])
            subscriber_attributes = AttributeHandleSet([subscriber_attribute])
            publisher.publishObjectClassAttributes(publisher_class, publisher_attributes)
            publisher.changeDefaultAttributeOrderType(
                publisher_class, publisher_attributes, OrderType.TIMESTAMP
            )

            publisher_dimension = publisher.getDimensionHandle(HLA_FIXTURES.SODA_FLAVOR)
            subscriber_dimension = subscriber.getDimensionHandle(HLA_FIXTURES.SODA_FLAVOR)
            publisher_overlap_region = publisher.createRegion(
                DimensionHandleSet([publisher_dimension])
            )
            publisher_disjoint_region = publisher.createRegion(
                DimensionHandleSet([publisher_dimension])
            )
            subscriber_region = subscriber.createRegion(
                DimensionHandleSet([subscriber_dimension])
            )
            publisher.setRangeBounds(
                publisher_overlap_region, publisher_dimension, RangeBounds(0, 2)
            )
            publisher.setRangeBounds(
                publisher_disjoint_region, publisher_dimension, RangeBounds(20, 30)
            )
            subscriber.setRangeBounds(
                subscriber_region, subscriber_dimension, RangeBounds(1, 3)
            )
            publisher.commitRegionModifications(
                RegionHandleSet([publisher_overlap_region, publisher_disjoint_region])
            )
            subscriber.commitRegionModifications(RegionHandleSet([subscriber_region]))

            subscriber_pairs = AttributeSetRegionSetPairList(
                [
                    AttributeSetRegionSetPair(
                        subscriber_attributes, RegionHandleSet([subscriber_region])
                    )
                ]
            )
            source_both_regions = AttributeSetRegionSetPairList(
                [
                    AttributeSetRegionSetPair(
                        publisher_attributes,
                        RegionHandleSet([publisher_overlap_region, publisher_disjoint_region]),
                    )
                ]
            )
            source_overlap_region = AttributeSetRegionSetPairList(
                [
                    AttributeSetRegionSetPair(
                        publisher_attributes, RegionHandleSet([publisher_overlap_region])
                    )
                ]
            )
            subscriber.subscribeObjectClassAttributesWithRegions(
                subscriber_class, subscriber_pairs
            )
            object_instance = publisher.registerObjectInstanceWithRegions(
                publisher_class, source_both_regions
            )
            pump()
            self.assertEqual(len(subscriber_callbacks.discoveries), 1)
            publisher.unwrap_java_object().enableDelayedSubscriptionEvaluation()
            self.assertTrue(subscriber.getDelaySubscriptionEvaluationSwitch())

            time_factory = publisher.getTimeFactory()
            subscriber.enableTimeConstrained()
            publisher.enableTimeRegulation(time_factory.makeLogicalTimeInterval(1))
            pump()

            first_time = time_factory.makeLogicalTime(5)
            first_retraction = publisher.updateAttributeValuesWithTime(
                object_instance,
                AttributeHandleValueMap({publisher_attribute: b"stale-overlap"}),
                first_time,
                b"stale-overlap-tag",
            )
            self.assertTrue(first_retraction.isValid())
            publisher.unassociateRegionsForUpdates(object_instance, source_overlap_region)
            publisher.timeAdvanceRequest(first_time)
            subscriber.timeAdvanceRequest(subscriber.getTimeFactory().makeLogicalTime(5))
            pump()
            self.assertEqual(subscriber_callbacks.timed_reflections, [])

            publisher.associateRegionsForUpdates(object_instance, source_overlap_region)
            second_time = time_factory.makeLogicalTime(7)
            second_retraction = publisher.updateAttributeValuesWithTime(
                object_instance,
                AttributeHandleValueMap({publisher_attribute: b"restored-overlap"}),
                second_time,
                b"restored-overlap-tag",
            )
            self.assertTrue(second_retraction.isValid())
            publisher.timeAdvanceRequest(second_time)
            subscriber.timeAdvanceRequest(subscriber.getTimeFactory().makeLogicalTime(7))
            pump()
            self.assertEqual(len(subscriber_callbacks.timed_reflections), 1)
            reflection = subscriber_callbacks.timed_reflections[0]
            self.assertEqual(reflection[0], object_instance)
            self.assertEqual(reflection[1][subscriber_attribute], b"restored-overlap")
            self.assertEqual(reflection[2], b"restored-overlap-tag")
            self.assertEqual(reflection[6].getTime(), 7)
            self.assertEqual(reflection[7], OrderType.TIMESTAMP)
            self.assertEqual(reflection[8], OrderType.TIMESTAMP)
        finally:
            if subscriber_joined:
                subscriber.resignFederationExecution(ResignAction.NO_ACTION)
            if publisher_joined:
                publisher.resignFederationExecution(ResignAction.NO_ACTION)
            if created:
                publisher.destroyFederationExecution(federation_name)
            subscriber.disconnect()
            publisher.disconnect()

    def test_real_jvm_passive_regional_subscriptions_activate_before_delivery(self) -> None:
        publisher = self.factory.getRtiAmbassador()
        passive = self.factory.getRtiAmbassador()
        active = self.factory.getRtiAmbassador()
        publisher_callbacks = _RecordingFederateAmbassador()
        passive_callbacks = _RecordingFederateAmbassador()
        active_callbacks = _RecordingFederateAmbassador()
        federation_name = "Python passive regional subscription federation"
        created = publisher_joined = passive_joined = active_joined = False
        try:
            publisher.connect(publisher_callbacks, CallbackModel.HLA_EVOKED)
            passive.connect(passive_callbacks, CallbackModel.HLA_EVOKED)
            active.connect(active_callbacks, CallbackModel.HLA_EVOKED)
            publisher.createFederationExecution(
                federation_name,
                "fixture-does-not-parse-fom.xml",
                HLA_TYPES.INTEGER64_TIME,
            )
            created = True
            publisher.joinFederationExecution("passive-publisher", federation_name)
            publisher_joined = True
            passive.joinFederationExecution("passive-subscriber", federation_name)
            passive_joined = True
            active.joinFederationExecution("active-subscriber", federation_name)
            active_joined = True

            publisher_class = publisher.getObjectClassHandle(HLA_FOM.FOOD_DRINK_SODA)
            passive_class = passive.getObjectClassHandle(HLA_FOM.FOOD_DRINK_SODA)
            active_class = active.getObjectClassHandle(HLA_FOM.FOOD_DRINK_SODA)
            publisher_attribute = publisher.getAttributeHandle(publisher_class, HLA_FIXTURES.FLAVOR)
            passive_attribute = passive.getAttributeHandle(passive_class, HLA_FIXTURES.FLAVOR)
            active_attribute = active.getAttributeHandle(active_class, HLA_FIXTURES.FLAVOR)
            publisher.publishObjectClassAttributes(
                publisher_class, AttributeHandleSet([publisher_attribute])
            )
            passive_pair = AttributeSetRegionSetPairList(
                [
                    AttributeSetRegionSetPair(
                        AttributeHandleSet([passive_attribute]),
                        RegionHandleSet(),
                    )
                ]
            )
            active_pair = AttributeSetRegionSetPairList(
                [
                    AttributeSetRegionSetPair(
                        AttributeHandleSet([active_attribute]),
                        RegionHandleSet(),
                    )
                ]
            )
            publisher_dimension = publisher.getDimensionHandle(HLA_FIXTURES.SODA_FLAVOR)
            publisher_region = publisher.createRegion(DimensionHandleSet([publisher_dimension]))
            publisher.setRangeBounds(publisher_region, publisher_dimension, RangeBounds(0, 10))
            publisher.commitRegionModifications(RegionHandleSet([publisher_region]))
            passive_dimension = passive.getDimensionHandle(HLA_FIXTURES.SODA_FLAVOR)
            passive_region = passive.createRegion(DimensionHandleSet([passive_dimension]))
            passive.setRangeBounds(passive_region, passive_dimension, RangeBounds(5, 15))
            passive.commitRegionModifications(RegionHandleSet([passive_region]))
            active_dimension = active.getDimensionHandle(HLA_FIXTURES.SODA_FLAVOR)
            active_region = active.createRegion(DimensionHandleSet([active_dimension]))
            active.setRangeBounds(active_region, active_dimension, RangeBounds(5, 15))
            active.commitRegionModifications(RegionHandleSet([active_region]))
            passive_pair = AttributeSetRegionSetPairList(
                [
                    AttributeSetRegionSetPair(
                        AttributeHandleSet([passive_attribute]),
                        RegionHandleSet([passive_region]),
                    )
                ]
            )
            active_pair = AttributeSetRegionSetPairList(
                [
                    AttributeSetRegionSetPair(
                        AttributeHandleSet([active_attribute]),
                        RegionHandleSet([active_region]),
                    )
                ]
            )
            passive.subscribeObjectClassAttributesWithRegions(
                passive_class, passive_pair, active=False
            )
            active.subscribeObjectClassAttributesWithRegions(
                active_class, active_pair, active=True
            )
            interaction = publisher.getInteractionClassHandle(
                HLA_FOM.SODA_SERVED
            )
            passive_interaction = passive.getInteractionClassHandle(
                HLA_FOM.SODA_SERVED
            )
            active_interaction = active.getInteractionClassHandle(
                HLA_FOM.SODA_SERVED
            )
            publisher.publishInteractionClass(interaction)
            passive.subscribeInteractionClassWithRegions(
                passive_interaction, RegionHandleSet([passive_region]), active=False
            )
            active.subscribeInteractionClassWithRegions(
                active_interaction, RegionHandleSet([active_region]), active=True
            )

            first_object = publisher.registerObjectInstanceWithRegions(
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
            for _ in range(4):
                passive.evokeMultipleCallbacks(0.0, 0.0)
                active.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(passive_callbacks.discoveries, [])
            self.assertEqual(active_callbacks.discoveries[0][0], first_object)

            publisher.sendInteractionWithRegions(
                interaction, ParameterHandleValueMap(), RegionHandleSet([publisher_region]), b"passive"
            )
            for _ in range(3):
                passive.evokeMultipleCallbacks(0.0, 0.0)
                active.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(passive_callbacks.interactions, [])
            self.assertEqual(len(active_callbacks.interactions), 1)

            passive.subscribeObjectClassAttributesWithRegions(
                passive_class, passive_pair, active=True
            )
            passive.subscribeInteractionClassWithRegions(
                passive_interaction, RegionHandleSet([passive_region]), active=True
            )
            second_object = publisher.registerObjectInstanceWithRegions(
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
            for _ in range(4):
                passive.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(passive_callbacks.discoveries[-1][0], second_object)
            publisher.sendInteractionWithRegions(
                interaction, ParameterHandleValueMap(), RegionHandleSet([publisher_region]), b"active"
            )
            for _ in range(3):
                passive.evokeMultipleCallbacks(0.0, 0.0)
            self.assertEqual(len(passive_callbacks.interactions), 1)
        finally:
            if active_joined:
                active.resignFederationExecution(ResignAction.NO_ACTION)
            if passive_joined:
                passive.resignFederationExecution(ResignAction.NO_ACTION)
            if publisher_joined:
                publisher.resignFederationExecution(ResignAction.NO_ACTION)
            if created:
                publisher.destroyFederationExecution(federation_name)
            active.disconnect()
            passive.disconnect()
            publisher.disconnect()

    def test_real_jvm_regional_object_association_selectively_reprojects_attributes(self) -> None:
        publisher = self.factory.getRtiAmbassador()
        subscriber = self.factory.getRtiAmbassador()
        publisher_callbacks = _RecordingFederateAmbassador()
        subscriber_callbacks = _RecordingFederateAmbassador()
        federation_name = "Python regional association reprojection federation"
        created = publisher_joined = subscriber_joined = False
        try:
            publisher.connect(publisher_callbacks, CallbackModel.HLA_EVOKED)
            subscriber.connect(subscriber_callbacks, CallbackModel.HLA_EVOKED)
            publisher.createFederationExecution(
                federation_name,
                "fixture-does-not-parse-fom.xml",
                HLA_TYPES.INTEGER64_TIME,
            )
            created = True
            publisher.joinFederationExecution("association-publisher", federation_name)
            publisher_joined = True
            subscriber.joinFederationExecution("association-subscriber", federation_name)
            subscriber_joined = True

            publisher_class = publisher.getObjectClassHandle(HLA_FOM.FOOD_DRINK_SODA)
            subscriber_class = subscriber.getObjectClassHandle(HLA_FOM.FOOD_DRINK_SODA)
            publisher_flavor = publisher.getAttributeHandle(publisher_class, HLA_FIXTURES.FLAVOR)
            publisher_color = publisher.getAttributeHandle(publisher_class, HLA_FIXTURES.COLOR)
            subscriber_flavor = subscriber.getAttributeHandle(subscriber_class, HLA_FIXTURES.FLAVOR)
            subscriber_color = subscriber.getAttributeHandle(subscriber_class, HLA_FIXTURES.COLOR)
            publisher.publishObjectClassAttributes(
                publisher_class, AttributeHandleSet([publisher_flavor, publisher_color])
            )
            dimension = publisher.getDimensionHandle(HLA_FIXTURES.SODA_FLAVOR)
            publisher_overlap_region = publisher.createRegion(DimensionHandleSet([dimension]))
            publisher_disjoint_region = publisher.createRegion(DimensionHandleSet([dimension]))
            publisher.setRangeBounds(
                publisher_overlap_region, dimension, RangeBounds(0, 10)
            )
            publisher.setRangeBounds(
                publisher_disjoint_region, dimension, RangeBounds(20, 30)
            )
            publisher.commitRegionModifications(
                RegionHandleSet([publisher_overlap_region, publisher_disjoint_region])
            )
            subscriber_region = subscriber.createRegion(
                DimensionHandleSet([subscriber.getDimensionHandle(HLA_FIXTURES.SODA_FLAVOR)])
            )
            subscriber.setRangeBounds(
                subscriber_region, subscriber.getDimensionHandle(HLA_FIXTURES.SODA_FLAVOR), RangeBounds(5, 15)
            )
            subscriber.commitRegionModifications(RegionHandleSet([subscriber_region]))
            subscriber_pairs = AttributeSetRegionSetPairList(
                [
                    AttributeSetRegionSetPair(
                        AttributeHandleSet([subscriber_flavor, subscriber_color]),
                        RegionHandleSet([subscriber_region]),
                    )
                ]
            )
            subscriber.subscribeObjectClassAttributesWithRegions(
                subscriber_class, subscriber_pairs
            )
            source_pairs = AttributeSetRegionSetPairList(
                [
                    AttributeSetRegionSetPair(
                        AttributeHandleSet([publisher_flavor]),
                        RegionHandleSet([publisher_overlap_region, publisher_disjoint_region]),
                    ),
                    AttributeSetRegionSetPair(
                        AttributeHandleSet([publisher_color]),
                        RegionHandleSet([publisher_disjoint_region]),
                    ),
                ]
            )
            object_instance = publisher.registerObjectInstanceWithRegions(
                publisher_class, source_pairs
            )
            self.assertTrue(subscriber.evokeCallback(0.0))
            self.assertEqual(subscriber_callbacks.discoveries[-1][0], object_instance)

            publisher.updateAttributeValues(
                object_instance,
                AttributeHandleValueMap(
                    {publisher_flavor: b"overlap-1", publisher_color: b"disjoint-1"}
                ),
                b"initial-projection",
            )
            self.assertTrue(subscriber.evokeCallback(0.0))
            self.assertEqual(
                set(subscriber_callbacks.reflections[-1][1].keys()), {subscriber_flavor}
            )
            self.assertEqual(subscriber_callbacks.reflections[-1][1][subscriber_flavor], b"overlap-1")

            color_association = AttributeSetRegionSetPairList(
                [
                    AttributeSetRegionSetPair(
                        AttributeHandleSet([publisher_color]),
                        RegionHandleSet([publisher_overlap_region]),
                    )
                ]
            )
            publisher.associateRegionsForUpdates(object_instance, color_association)
            publisher.updateAttributeValues(
                object_instance,
                AttributeHandleValueMap(
                    {publisher_flavor: b"overlap-2", publisher_color: b"overlap-2"}
                ),
                b"associated-projection",
            )
            self.assertTrue(subscriber.evokeCallback(0.0))
            self.assertEqual(
                set(subscriber_callbacks.reflections[-1][1].keys()),
                {subscriber_flavor, subscriber_color},
            )

            flavor_unassociation = AttributeSetRegionSetPairList(
                [
                    AttributeSetRegionSetPair(
                        AttributeHandleSet([publisher_flavor]),
                        RegionHandleSet([publisher_overlap_region]),
                    )
                ]
            )
            publisher.unassociateRegionsForUpdates(object_instance, flavor_unassociation)
            publisher.updateAttributeValues(
                object_instance,
                AttributeHandleValueMap(
                    {publisher_flavor: b"disjoint-3", publisher_color: b"overlap-3"}
                ),
                b"selective-unassociation",
            )
            self.assertTrue(subscriber.evokeCallback(0.0))
            self.assertEqual(
                set(subscriber_callbacks.reflections[-1][1].keys()), {subscriber_color}
            )
            self.assertEqual(subscriber_callbacks.reflections[-1][1][subscriber_color], b"overlap-3")

            color_default_association = AttributeSetRegionSetPairList(
                [
                    AttributeSetRegionSetPair(
                        AttributeHandleSet([publisher_color]),
                        RegionHandleSet(
                            [publisher_overlap_region, publisher_disjoint_region]
                        ),
                    )
                ]
            )
            publisher.unassociateRegionsForUpdates(object_instance, color_default_association)
            publisher.updateAttributeValues(
                object_instance,
                AttributeHandleValueMap({publisher_color: b"default-region"}),
                b"default-region-fallback",
            )
            self.assertTrue(subscriber.evokeCallback(0.0))
            self.assertEqual(
                set(subscriber_callbacks.reflections[-1][1].keys()), {subscriber_color}
            )
            self.assertEqual(
                subscriber_callbacks.reflections[-1][1][subscriber_color], b"default-region"
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

    def test_real_jvm_regional_object_association_isolates_recipients(self) -> None:
        publisher = self.factory.getRtiAmbassador()
        overlap = self.factory.getRtiAmbassador()
        disjoint = self.factory.getRtiAmbassador()
        publisher_callbacks = _RecordingFederateAmbassador()
        overlap_callbacks = _RecordingFederateAmbassador()
        disjoint_callbacks = _RecordingFederateAmbassador()
        federation_name = "Python regional association recipient isolation federation"
        created = publisher_joined = overlap_joined = disjoint_joined = False

        def pump() -> None:
            for _ in range(4):
                publisher.evokeCallback(0.0)
                overlap.evokeCallback(0.0)
                disjoint.evokeCallback(0.0)

        try:
            publisher.connect(publisher_callbacks, CallbackModel.HLA_EVOKED)
            overlap.connect(overlap_callbacks, CallbackModel.HLA_EVOKED)
            disjoint.connect(disjoint_callbacks, CallbackModel.HLA_EVOKED)
            publisher.createFederationExecution(
                federation_name,
                "fixture-does-not-parse-fom.xml",
                HLA_TYPES.INTEGER64_TIME,
            )
            created = True
            publisher.joinFederationExecution("recipient-isolation-publisher", federation_name)
            publisher_joined = True
            overlap.joinFederationExecution("recipient-isolation-overlap", federation_name)
            overlap_joined = True
            disjoint.joinFederationExecution("recipient-isolation-disjoint", federation_name)
            disjoint_joined = True

            publisher_class = publisher.getObjectClassHandle(HLA_FOM.FOOD_DRINK_SODA)
            overlap_class = overlap.getObjectClassHandle(HLA_FOM.FOOD_DRINK_SODA)
            disjoint_class = disjoint.getObjectClassHandle(HLA_FOM.FOOD_DRINK_SODA)
            publisher_attribute = publisher.getAttributeHandle(publisher_class, HLA_FIXTURES.FLAVOR)
            overlap_attribute = overlap.getAttributeHandle(overlap_class, HLA_FIXTURES.FLAVOR)
            disjoint_attribute = disjoint.getAttributeHandle(disjoint_class, HLA_FIXTURES.FLAVOR)
            publisher.publishObjectClassAttributes(
                publisher_class, AttributeHandleSet([publisher_attribute])
            )

            publisher_dimension = publisher.getDimensionHandle(HLA_FIXTURES.SODA_FLAVOR)
            overlap_dimension = overlap.getDimensionHandle(HLA_FIXTURES.SODA_FLAVOR)
            disjoint_dimension = disjoint.getDimensionHandle(HLA_FIXTURES.SODA_FLAVOR)
            publisher_overlap_region = publisher.createRegion(
                DimensionHandleSet([publisher_dimension])
            )
            publisher_disjoint_region = publisher.createRegion(
                DimensionHandleSet([publisher_dimension])
            )
            disjoint_overlap_region = disjoint.createRegion(
                DimensionHandleSet([disjoint_dimension])
            )
            overlap_region = overlap.createRegion(DimensionHandleSet([overlap_dimension]))
            disjoint_region = disjoint.createRegion(DimensionHandleSet([disjoint_dimension]))
            publisher.setRangeBounds(
                publisher_overlap_region, publisher_dimension, RangeBounds(0, 10)
            )
            publisher.setRangeBounds(
                publisher_disjoint_region, publisher_dimension, RangeBounds(20, 30)
            )
            overlap.setRangeBounds(overlap_region, overlap_dimension, RangeBounds(0, 10))
            disjoint.setRangeBounds(
                disjoint_overlap_region, disjoint_dimension, RangeBounds(0, 10)
            )
            disjoint.setRangeBounds(disjoint_region, disjoint_dimension, RangeBounds(20, 30))
            publisher.commitRegionModifications(
                RegionHandleSet([publisher_overlap_region, publisher_disjoint_region])
            )
            overlap.commitRegionModifications(RegionHandleSet([overlap_region]))
            disjoint.commitRegionModifications(
                RegionHandleSet([disjoint_overlap_region, disjoint_region])
            )

            overlap_pairs = AttributeSetRegionSetPairList(
                [
                    AttributeSetRegionSetPair(
                        AttributeHandleSet([overlap_attribute]),
                        RegionHandleSet([overlap_region]),
                    )
                ]
            )
            disjoint_pairs = AttributeSetRegionSetPairList(
                [
                    AttributeSetRegionSetPair(
                        AttributeHandleSet([disjoint_attribute]),
                        RegionHandleSet([disjoint_region]),
                    )
                ]
            )
            disjoint_overlap_pairs = AttributeSetRegionSetPairList(
                [
                    AttributeSetRegionSetPair(
                        AttributeHandleSet([disjoint_attribute]),
                        RegionHandleSet([disjoint_overlap_region]),
                    )
                ]
            )
            overlap.subscribeObjectClassAttributesWithRegions(overlap_class, overlap_pairs)
            disjoint.setAttributeScopeAdvisorySwitch(True)
            disjoint.subscribeObjectClassAttributesWithRegions(
                disjoint_class, disjoint_overlap_pairs
            )
            source_overlap = AttributeSetRegionSetPairList(
                [
                    AttributeSetRegionSetPair(
                        AttributeHandleSet([publisher_attribute]),
                        RegionHandleSet([publisher_overlap_region]),
                    )
                ]
            )
            source_disjoint = AttributeSetRegionSetPairList(
                [
                    AttributeSetRegionSetPair(
                        AttributeHandleSet([publisher_attribute]),
                        RegionHandleSet([publisher_disjoint_region]),
                    )
                ]
            )
            object_instance = publisher.registerObjectInstanceWithRegions(
                publisher_class, source_overlap
            )
            pump()
            self.assertEqual(len(overlap_callbacks.discoveries), 1)
            self.assertEqual(len(disjoint_callbacks.discoveries), 1)

            disjoint.unsubscribeObjectClassAttributesWithRegions(
                disjoint_class, disjoint_overlap_pairs
            )
            pump()
            self.assertEqual(overlap_callbacks.scope_exits, [])
            self.assertEqual(len(disjoint_callbacks.scope_exits), 1)

            disjoint.subscribeObjectClassAttributesWithRegions(disjoint_class, disjoint_pairs)
            pump()
            self.assertEqual(len(disjoint_callbacks.scope_entries), 0)

            publisher.associateRegionsForUpdates(object_instance, source_disjoint)
            pump()
            self.assertEqual(overlap_callbacks.scope_entries, [])
            self.assertEqual(len(disjoint_callbacks.scope_entries), 1)

            publisher.updateAttributeValues(
                object_instance,
                AttributeHandleValueMap({publisher_attribute: b"both-recipients"}),
                b"both-recipients-tag",
            )
            pump()
            self.assertEqual(len(overlap_callbacks.reflections), 1)
            self.assertEqual(len(disjoint_callbacks.reflections), 1)
            self.assertEqual(
                overlap_callbacks.reflections[-1][1][overlap_attribute], b"both-recipients"
            )
            self.assertEqual(
                disjoint_callbacks.reflections[-1][1][disjoint_attribute], b"both-recipients"
            )

            publisher.unassociateRegionsForUpdates(object_instance, source_disjoint)
            pump()
            publisher.updateAttributeValues(
                object_instance,
                AttributeHandleValueMap({publisher_attribute: b"overlap-only"}),
                b"overlap-only-tag",
            )
            pump()
            self.assertEqual(len(overlap_callbacks.reflections), 2)
            self.assertEqual(len(disjoint_callbacks.reflections), 1)
            self.assertEqual(
                overlap_callbacks.reflections[-1][1][overlap_attribute], b"overlap-only"
            )
            self.assertEqual(len(disjoint_callbacks.scope_exits), 2)
        finally:
            if disjoint_joined:
                disjoint.resignFederationExecution(ResignAction.NO_ACTION)
            if overlap_joined:
                overlap.resignFederationExecution(ResignAction.NO_ACTION)
            if publisher_joined:
                publisher.resignFederationExecution(ResignAction.NO_ACTION)
            if created:
                publisher.destroyFederationExecution(federation_name)
            disjoint.disconnect()
            overlap.disconnect()
            publisher.disconnect()

    def test_real_jvm_regional_value_request_routes_by_overlap(self) -> None:
        publisher = self.factory.getRtiAmbassador()
        overlap = self.factory.getRtiAmbassador()
        disjoint = self.factory.getRtiAmbassador()
        publisher_callbacks = _RecordingFederateAmbassador()
        overlap_callbacks = _RecordingFederateAmbassador()
        disjoint_callbacks = _RecordingFederateAmbassador()
        federation_name = "Python regional value request federation"
        created = publisher_joined = overlap_joined = disjoint_joined = False
        try:
            publisher.connect(publisher_callbacks, CallbackModel.HLA_EVOKED)
            overlap.connect(overlap_callbacks, CallbackModel.HLA_EVOKED)
            disjoint.connect(disjoint_callbacks, CallbackModel.HLA_EVOKED)
            publisher.createFederationExecution(
                federation_name,
                "fixture-does-not-parse-fom.xml",
                HLA_TYPES.INTEGER64_TIME,
            )
            created = True
            publisher.joinFederationExecution("value-request-publisher", federation_name)
            publisher_joined = True
            overlap.joinFederationExecution("value-request-overlap", federation_name)
            overlap_joined = True
            disjoint.joinFederationExecution("value-request-disjoint", federation_name)
            disjoint_joined = True

            publisher_class = publisher.getObjectClassHandle(HLA_FOM.FOOD_DRINK_SODA)
            overlap_class = overlap.getObjectClassHandle(HLA_FOM.FOOD_DRINK_SODA)
            disjoint_class = disjoint.getObjectClassHandle(HLA_FOM.FOOD_DRINK_SODA)
            publisher_attribute = publisher.getAttributeHandle(publisher_class, HLA_FIXTURES.FLAVOR)
            overlap_attribute = overlap.getAttributeHandle(overlap_class, HLA_FIXTURES.FLAVOR)
            disjoint_attribute = disjoint.getAttributeHandle(disjoint_class, HLA_FIXTURES.FLAVOR)
            publisher.publishObjectClassAttributes(
                publisher_class, AttributeHandleSet([publisher_attribute])
            )
            # The publish operation queues the standard start-registration
            # advisory in HLA_EVOKED mode; drain it before asserting the
            # value-request callback below.
            self.assertTrue(publisher.evokeCallback(0.0))
            publisher_dimension = publisher.getDimensionHandle(HLA_FIXTURES.SODA_FLAVOR)
            overlap_dimension = overlap.getDimensionHandle(HLA_FIXTURES.SODA_FLAVOR)
            disjoint_dimension = disjoint.getDimensionHandle(HLA_FIXTURES.SODA_FLAVOR)
            publisher_region = publisher.createRegion(DimensionHandleSet([publisher_dimension]))
            overlap_region = overlap.createRegion(DimensionHandleSet([overlap_dimension]))
            disjoint_region = disjoint.createRegion(DimensionHandleSet([disjoint_dimension]))
            publisher.setRangeBounds(publisher_region, publisher_dimension, RangeBounds(0, 10))
            overlap.setRangeBounds(overlap_region, overlap_dimension, RangeBounds(5, 15))
            disjoint.setRangeBounds(disjoint_region, disjoint_dimension, RangeBounds(20, 30))
            publisher.commitRegionModifications(RegionHandleSet([publisher_region]))
            overlap.commitRegionModifications(RegionHandleSet([overlap_region]))
            disjoint.commitRegionModifications(RegionHandleSet([disjoint_region]))

            overlap_pairs = AttributeSetRegionSetPairList(
                [
                    AttributeSetRegionSetPair(
                        AttributeHandleSet([overlap_attribute]),
                        RegionHandleSet([overlap_region]),
                    )
                ]
            )
            overlap.subscribeObjectClassAttributesWithRegions(overlap_class, overlap_pairs)
            source_pairs = AttributeSetRegionSetPairList(
                [
                    AttributeSetRegionSetPair(
                        AttributeHandleSet([publisher_attribute]),
                        RegionHandleSet([publisher_region]),
                    )
                ]
            )
            object_instance = publisher.registerObjectInstanceWithRegions(
                publisher_class, source_pairs
            )
            self.assertTrue(overlap.evokeCallback(0.0))
            self.assertEqual(overlap_callbacks.discoveries[-1][0], object_instance)

            overlap.requestAttributeValueUpdateWithRegions(
                overlap_class, overlap_pairs, b"overlap-request"
            )
            self.assertTrue(publisher.evokeCallback(0.0))
            self.assertEqual(
                publisher_callbacks.attribute_value_requests[-1],
                (object_instance, AttributeHandleSet([publisher_attribute]), b"overlap-request"),
            )

            disjoint_pairs = AttributeSetRegionSetPairList(
                [
                    AttributeSetRegionSetPair(
                        AttributeHandleSet([disjoint_attribute]),
                        RegionHandleSet([disjoint_region]),
                    )
                ]
            )
            disjoint.requestAttributeValueUpdateWithRegions(
                disjoint_class, disjoint_pairs, b"disjoint-request"
            )
            self.assertFalse(publisher.evokeCallback(0.0))
            self.assertEqual(len(publisher_callbacks.attribute_value_requests), 1)
            self.assertEqual(disjoint_callbacks.attribute_value_requests, [])
        finally:
            if disjoint_joined:
                disjoint.resignFederationExecution(ResignAction.NO_ACTION)
            if overlap_joined:
                overlap.resignFederationExecution(ResignAction.NO_ACTION)
            if publisher_joined:
                publisher.resignFederationExecution(ResignAction.NO_ACTION)
            if created:
                publisher.destroyFederationExecution(federation_name)
            disjoint.disconnect()
            overlap.disconnect()
            publisher.disconnect()

    def test_real_jvm_regional_subscription_reprojects_existing_object(self) -> None:
        publisher = self.factory.getRtiAmbassador()
        subscriber = self.factory.getRtiAmbassador()
        publisher_callbacks = _RecordingFederateAmbassador()
        subscriber_callbacks = _RecordingFederateAmbassador()
        federation_name = "Python regional subscription reprojection federation"
        created = publisher_joined = subscriber_joined = False
        try:
            publisher.connect(publisher_callbacks, CallbackModel.HLA_EVOKED)
            subscriber.connect(subscriber_callbacks, CallbackModel.HLA_EVOKED)
            publisher.createFederationExecution(
                federation_name,
                "fixture-does-not-parse-fom.xml",
                HLA_TYPES.INTEGER64_TIME,
            )
            created = True
            publisher.joinFederationExecution("reprojection-publisher", federation_name)
            publisher_joined = True
            subscriber.joinFederationExecution("reprojection-subscriber", federation_name)
            subscriber_joined = True

            publisher_class = publisher.getObjectClassHandle(HLA_FOM.FOOD_DRINK_SODA)
            subscriber_class = subscriber.getObjectClassHandle(HLA_FOM.FOOD_DRINK_SODA)
            publisher_attribute = publisher.getAttributeHandle(publisher_class, HLA_FIXTURES.FLAVOR)
            subscriber_attribute = subscriber.getAttributeHandle(subscriber_class, HLA_FIXTURES.FLAVOR)
            publisher.publishObjectClassAttributes(
                publisher_class, AttributeHandleSet([publisher_attribute])
            )
            publisher_dimension = publisher.getDimensionHandle(HLA_FIXTURES.SODA_FLAVOR)
            subscriber_dimension = subscriber.getDimensionHandle(HLA_FIXTURES.SODA_FLAVOR)
            publisher_region = publisher.createRegion(DimensionHandleSet([publisher_dimension]))
            disjoint_region = subscriber.createRegion(DimensionHandleSet([subscriber_dimension]))
            overlap_region = subscriber.createRegion(DimensionHandleSet([subscriber_dimension]))
            overlap_region_two = subscriber.createRegion(
                DimensionHandleSet([subscriber_dimension])
            )
            publisher.setRangeBounds(publisher_region, publisher_dimension, RangeBounds(0, 10))
            subscriber.setRangeBounds(disjoint_region, subscriber_dimension, RangeBounds(20, 30))
            subscriber.setRangeBounds(overlap_region, subscriber_dimension, RangeBounds(5, 15))
            subscriber.setRangeBounds(
                overlap_region_two, subscriber_dimension, RangeBounds(5, 15)
            )
            publisher.commitRegionModifications(RegionHandleSet([publisher_region]))
            subscriber.commitRegionModifications(
                RegionHandleSet([disjoint_region, overlap_region, overlap_region_two])
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
            subscriber.evokeCallback(0.0)
            self.assertEqual(subscriber_callbacks.discoveries, [])

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
            self.assertTrue(subscriber.evokeCallback(0.0))
            self.assertEqual(subscriber_callbacks.discoveries[-1][0], object_instance)

            subscriber.subscribeObjectClassAttributesWithRegions(
                subscriber_class,
                AttributeSetRegionSetPairList(
                    [
                        AttributeSetRegionSetPair(
                            AttributeHandleSet([subscriber_attribute]),
                            RegionHandleSet([overlap_region_two]),
                        )
                    ]
                ),
            )
            self.assertFalse(subscriber.evokeCallback(0.0))
            self.assertEqual(len(subscriber_callbacks.discoveries), 1)

            first_region = AttributeSetRegionSetPairList(
                [
                    AttributeSetRegionSetPair(
                        AttributeHandleSet([subscriber_attribute]),
                        RegionHandleSet([overlap_region]),
                    )
                ]
            )
            subscriber.setAttributeScopeAdvisorySwitch(True)
            subscriber.unsubscribeObjectClassAttributesWithRegions(subscriber_class, first_region)
            self.assertFalse(subscriber.evokeCallback(0.0))
            self.assertEqual(subscriber_callbacks.scope_exits, [])
            publisher.updateAttributeValues(
                object_instance,
                AttributeHandleValueMap({publisher_attribute: b"second-region"}),
            )
            self.assertTrue(subscriber.evokeCallback(0.0))
            self.assertEqual(
                subscriber_callbacks.reflections[-1][1][subscriber_attribute], b"second-region"
            )
            remaining_regions = AttributeSetRegionSetPairList(
                [
                    AttributeSetRegionSetPair(
                        AttributeHandleSet([subscriber_attribute]),
                        RegionHandleSet([disjoint_region, overlap_region_two]),
                    )
                ]
            )
            subscriber.unsubscribeObjectClassAttributesWithRegions(
                subscriber_class, remaining_regions
            )
            self.assertTrue(subscriber.evokeCallback(0.0))
            self.assertEqual(subscriber_callbacks.scope_exits[-1][0], object_instance)
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
            self.assertTrue(subscriber.evokeCallback(0.0))
            self.assertEqual(subscriber_callbacks.scope_entries[-1][0], object_instance)

            publisher.updateAttributeValues(
                object_instance,
                AttributeHandleValueMap({publisher_attribute: b"reprojected"}),
                b"reprojected-tag",
            )
            self.assertTrue(subscriber.evokeCallback(0.0))
            self.assertEqual(
                subscriber_callbacks.reflections[-1][1][subscriber_attribute], b"reprojected"
            )
            self.assertEqual(subscriber_callbacks.reflections[-1][2], b"reprojected-tag")
        finally:
            if subscriber_joined:
                subscriber.resignFederationExecution(ResignAction.NO_ACTION)
            if publisher_joined:
                publisher.resignFederationExecution(ResignAction.NO_ACTION)
            if created:
                publisher.destroyFederationExecution(federation_name)
            subscriber.disconnect()
            publisher.disconnect()

    def test_real_jvm_directed_multi_recipient_tso_retraction(self) -> None:
        publisher = self.factory.getRtiAmbassador()
        immediate_subscriber = self.factory.getRtiAmbassador()
        constrained_subscriber = self.factory.getRtiAmbassador()
        publisher_callbacks = _RecordingFederateAmbassador()
        immediate_callbacks = _RecordingFederateAmbassador()
        constrained_callbacks = _RecordingFederateAmbassador()
        federation_name = "Python directed multi-member federation"
        created = publisher_joined = immediate_joined = constrained_joined = False
        try:
            publisher.connect(publisher_callbacks, CallbackModel.HLA_EVOKED)
            immediate_subscriber.connect(immediate_callbacks, CallbackModel.HLA_IMMEDIATE)
            constrained_subscriber.connect(constrained_callbacks, CallbackModel.HLA_EVOKED)
            publisher.createFederationExecution(
                federation_name,
                "fixture-does-not-parse-fom.xml",
                HLA_TYPES.INTEGER64_TIME,
            )
            created = True
            publisher.joinFederationExecution("directed-publisher", federation_name)
            publisher_joined = True
            immediate_subscriber.joinFederationExecution("directed-immediate", federation_name)
            immediate_joined = True
            constrained_subscriber.joinFederationExecution(
                "directed-constrained", federation_name
            )
            constrained_joined = True

            publisher_object_class = publisher.getObjectClassHandle(
                HLA_FOM.FOOD_DRINK_SODA
            )
            immediate_object_class = immediate_subscriber.getObjectClassHandle(
                HLA_FOM.FOOD_DRINK_SODA
            )
            constrained_object_class = constrained_subscriber.getObjectClassHandle(
                HLA_FOM.FOOD_DRINK_SODA
            )
            publisher_interaction = publisher.getInteractionClassHandle(
                HLA_FOM.SODA_SERVED
            )
            immediate_interaction = immediate_subscriber.getInteractionClassHandle(
                HLA_FOM.SODA_SERVED
            )
            constrained_interaction = constrained_subscriber.getInteractionClassHandle(
                HLA_FOM.SODA_SERVED
            )
            directed_set = InteractionClassHandleSet([publisher_interaction])
            publisher.publishObjectClassDirectedInteractions(
                publisher_object_class, directed_set
            )
            immediate_subscriber.subscribeObjectClassDirectedInteractions(
                immediate_object_class,
                InteractionClassHandleSet([immediate_interaction]),
                universally=True,
            )
            constrained_subscriber.subscribeObjectClassDirectedInteractions(
                constrained_object_class,
                InteractionClassHandleSet([constrained_interaction]),
                universally=True,
            )
            target = publisher.registerObjectInstance(publisher_object_class)
            constrained_subscriber.enableTimeConstrained()
            self.assertTrue(constrained_subscriber.evokeCallback(0.0))

            retraction = publisher.sendDirectedInteractionWithTime(
                publisher_interaction,
                target,
                ParameterHandleValueMap(),
                publisher.getTimeFactory().makeLogicalTime(2),
                b"directed-tso-tag",
            )
            self.assertEqual(len(immediate_callbacks.timed_directed_interactions), 1)
            self.assertEqual(constrained_callbacks.timed_directed_interactions, [])
            self.assertEqual(
                immediate_callbacks.timed_directed_interactions[-1][1], target
            )
            self.assertEqual(
                immediate_callbacks.timed_directed_interactions[-1][0], immediate_interaction
            )
            self.assertEqual(
                immediate_callbacks.timed_directed_interactions[-1][6].getTime(), 2
            )
            self.assertEqual(
                immediate_callbacks.timed_directed_interactions[-1][8], OrderType.RECEIVE
            )
            self.assertEqual(
                immediate_callbacks.timed_directed_interactions[-1][9].encodedValue,
                retraction.encodedValue,
            )

            publisher.retract(retraction)
            self.assertEqual(len(immediate_callbacks.request_retractions), 1)
            self.assertEqual(constrained_callbacks.request_retractions, [])
            constrained_subscriber.timeAdvanceRequest(
                constrained_subscriber.getTimeFactory().makeLogicalTime(2)
            )
            self.assertTrue(constrained_subscriber.evokeCallback(0.0))
            self.assertEqual(constrained_callbacks.timed_directed_interactions, [])
            self.assertEqual(constrained_callbacks.request_retractions, [])

            publisher.sendDirectedInteraction(
                publisher_interaction,
                target,
                ParameterHandleValueMap(),
                b"directed-receive-tag",
            )
            self.assertEqual(len(immediate_callbacks.directed_interactions), 1)
            self.assertEqual(len(constrained_callbacks.directed_interactions), 0)
            self.assertEqual(immediate_callbacks.directed_interactions[-1][1], target)
            self.assertEqual(immediate_callbacks.directed_interactions[-1][3], b"directed-receive-tag")
            self.assertTrue(constrained_subscriber.evokeCallback(0.0))
            self.assertEqual(len(constrained_callbacks.directed_interactions), 1)
            self.assertEqual(constrained_callbacks.directed_interactions[-1][1], target)
        finally:
            if constrained_joined:
                constrained_subscriber.resignFederationExecution(ResignAction.NO_ACTION)
            if immediate_joined:
                immediate_subscriber.resignFederationExecution(ResignAction.NO_ACTION)
            if publisher_joined:
                publisher.resignFederationExecution(ResignAction.NO_ACTION)
            if created:
                publisher.destroyFederationExecution(federation_name)
            constrained_subscriber.disconnect()
            immediate_subscriber.disconnect()
            publisher.disconnect()

    def test_real_jvm_floating_time_factory_and_callbacks(self) -> None:
        ambassador = self.factory.getRtiAmbassador()
        callbacks = _RecordingFederateAmbassador()
        ambassador.connect(callbacks, CallbackModel.HLA_EVOKED)
        ambassador.createFederationExecution(
            "Python-created floating federation",
            "fixture-does-not-parse-fom.xml",
            HLA_TYPES.FLOAT64_TIME,
        )
        ambassador.joinFederationExecution("observer", "Python-created floating federation")

        time_factory = ambassador.getTimeFactory()
        self.assertIsInstance(time_factory, HLAfloat64TimeFactory)
        self.assertEqual(time_factory.implementationName(), HLA_TYPES.FLOAT64_TIME)
        initial = time_factory.makeInitial()
        final = time_factory.makeFinal()
        zero = time_factory.makeZero()
        epsilon = time_factory.makeEpsilon()
        requested_time = time_factory.makeLogicalTime(12.5)
        lookahead = time_factory.makeLogicalTimeInterval(0.25)
        self.assertIsInstance(initial, HLAfloat64Time)
        self.assertIsInstance(final, HLAfloat64Time)
        self.assertIsInstance(zero, HLAfloat64Interval)
        self.assertIsInstance(epsilon, HLAfloat64Interval)
        self.assertTrue(initial.isInitial())
        self.assertTrue(final.isFinal())
        self.assertTrue(zero.isZero())
        self.assertTrue(epsilon.isEpsilon())
        self.assertEqual(epsilon.getInterval(), math.nextafter(0.0, 1.0))
        self.assertEqual(requested_time.getTime(), 12.5)
        self.assertEqual(lookahead.getInterval(), 0.25)
        negative_zero = time_factory.makeLogicalTime(-0.0)
        self.assertEqual(negative_zero.getTime(), 0.0)
        self.assertTrue(negative_zero.isInitial())
        for value in (-1.0, math.inf, math.nan):
            with self.assertRaises(InvalidLogicalTime):
                time_factory.makeLogicalTime(value)
            with self.assertRaises(InvalidLogicalTimeInterval):
                time_factory.makeLogicalTimeInterval(value)
        self.assertEqual(
            time_factory.decodeLogicalTime(requested_time.toByteArray()).getTime(), 12.5
        )
        self.assertEqual(
            time_factory.decodeLogicalTimeInterval(lookahead.toByteArray()).getInterval(), 0.25
        )
        base = time_factory.makeLogicalTime(1.0)
        advanced = time_factory.add(base, epsilon)
        self.assertEqual(advanced.getTime(), math.nextafter(1.0, math.inf))
        self.assertEqual(time_factory.subtract(advanced, epsilon).getTime(), 1.0)
        self.assertEqual(
            time_factory.difference(advanced, base).getInterval(),
            math.nextafter(1.0, math.inf) - 1.0,
        )
        final_time = time_factory.makeFinal()
        before_final = time_factory.subtract(final_time, epsilon)
        self.assertEqual(before_final.getTime(), math.nextafter(final_time.getTime(), 0.0))
        self.assertEqual(time_factory.add(before_final, epsilon).getTime(), final_time.getTime())
        smallest = time_factory.makeLogicalTime(math.nextafter(0.0, 1.0))
        self.assertEqual(
            time_factory.add(smallest, epsilon).getTime(), math.nextafter(smallest.getTime(), math.inf)
        )
        with self.assertRaises(IllegalTimeArithmetic):
            time_factory.add(final_time, epsilon)
        with self.assertRaises(IllegalTimeArithmetic):
            time_factory.subtract(time_factory.makeInitial(), epsilon)
        with self.assertRaises(IllegalTimeArithmetic):
            time_factory.difference(time_factory.makeInitial(), base)
        malformed_time_values = (
            b"",
            b"\0" * 7,
            b"\0" * 9,
            struct.pack(">d", -1.0),
            struct.pack(">d", math.inf),
        )
        for encoded in malformed_time_values:
            with self.assertRaises(CouldNotDecode):
                time_factory.decodeLogicalTime(encoded)
            with self.assertRaises(CouldNotDecode):
                time_factory.decodeLogicalTimeInterval(encoded)

        ambassador.enableTimeRegulation(lookahead)
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(callbacks.time_regulation[-1].getTime(), 0.0)
        tiny_time = time_factory.makeLogicalTime(math.nextafter(0.0, 1.0))
        ambassador.timeAdvanceRequest(tiny_time)
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(callbacks.time_grants[-1].getTime(), math.nextafter(0.0, 1.0))
        ambassador.timeAdvanceRequest(requested_time)
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(callbacks.time_grants[-1].getTime(), 12.5)
        self.assertEqual(ambassador.queryLogicalTime().getTime(), 12.5)
        self.assertEqual(ambassador.queryGALT().time.getTime(), 12.5)  # type: ignore[union-attr]
        self.assertEqual(ambassador.queryLITS().time.getTime(), 12.5)  # type: ignore[union-attr]

        java_ambassador = ambassador.unwrap_java_object()
        java_ambassador.emitTimestampedCallbacksForFloatEncoded(
            b"object-instance:float-object", b"interaction:float-interaction", 12.5
        )
        for _ in range(3):
            self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(callbacks.timed_reflections[-1][6].getTime(), 12.5)
        self.assertEqual(callbacks.timed_interactions[-1][6].getTime(), 12.5)
        self.assertEqual(callbacks.timed_removals[-1][3].getTime(), 12.5)

        ambassador.resignFederationExecution(ResignAction.NO_ACTION)
        ambassador.disconnect()

    def test_real_jvm_restore_rewinds_saved_logical_time_and_lookahead(self) -> None:
        ambassador = self.factory.getRtiAmbassador()
        callbacks = _RecordingFederateAmbassador()
        ambassador.connect(callbacks, CallbackModel.HLA_EVOKED)
        federation_name = "Python restore time-window federation"
        ambassador.createFederationExecution(
            federation_name,
            "fixture-does-not-parse-fom.xml",
            HLA_TYPES.INTEGER64_TIME,
        )
        ambassador.joinFederationExecution("restore-time-window", federation_name)
        time_factory = ambassador.getTimeFactory()
        ambassador.enableTimeRegulation(time_factory.makeLogicalTimeInterval(2))
        self.assertTrue(ambassador.evokeCallback(0.0))
        ambassador.timeAdvanceRequest(time_factory.makeLogicalTime(3))
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(ambassador.queryLogicalTime().getTime(), 3)
        self.assertEqual(ambassador.queryLookahead().getInterval(), 2)

        save_label = "jvm-time-window-baseline"
        ambassador.requestFederationSave(save_label)
        self.assertTrue(ambassador.evokeCallback(0.0))
        ambassador.federateSaveBegun()
        ambassador.federateSaveComplete()
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(callbacks.save_completions, 1)

        ambassador.modifyLookahead(time_factory.makeLogicalTimeInterval(5))
        ambassador.timeAdvanceRequest(time_factory.makeLogicalTime(5))
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(ambassador.queryLogicalTime().getTime(), 5)
        self.assertEqual(ambassador.queryLookahead().getInterval(), 5)

        ambassador.requestFederationRestore(save_label)
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertTrue(ambassador.evokeCallback(0.0))
        ambassador.federateRestoreComplete()
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(callbacks.restore_completions, 1)
        self.assertEqual(ambassador.queryLogicalTime().getTime(), 3)
        self.assertEqual(ambassador.queryLookahead().getInterval(), 2)
        ambassador.resignFederationExecution(ResignAction.NO_ACTION)
        ambassador.disconnect()

    def test_provider_neutral_multi_federate_callback_and_save_matrix(self) -> None:
        """Run the reusable matrix against the stateful Java mock federation."""

        if os.environ.get("UMBRA_ENABLE_JPYPE_STATE_SPACE_MATRIX") != "1":
            self.skipTest(
                "set UMBRA_ENABLE_JPYPE_STATE_SPACE_MATRIX=1 to run the opt-in JVM state-space lane"
            )
        # The source-checkout adapter matrix executes all 120 points without a
        # JVM.  Keep the live-JVM lane focused on successful save boundaries,
        # but include every standard time-advance service so a provider cannot
        # accidentally bind only the TAR spelling.
        cases = iter_surface_matrix()
        for case in cases:
            federation_name = f"python-surface-matrix-{case.case_id.replace(':', '-')}"
            ambassadors: list[object] = []
            callbacks: list[_MatrixFederateAmbassador] = []
            created = False

            def drain(ambassador: object) -> None:
                # JavaRTIambassador exposes the standard bool-returning evoke
                # operation; keep the helper provider-neutral for a transplant.
                while ambassador.evokeCallback(0.0):  # type: ignore[attr-defined]
                    pass

            try:
                for member in range(case.member_count):
                    ambassador = self.factory.getRtiAmbassador()
                    callback = _MatrixFederateAmbassador(SurfaceEventTrace())
                    ambassador.connect(callback, CallbackModel[case.callback_model])
                    if member == 0:
                        ambassador.createFederationExecution(
                            federation_name,
                            "fixture-does-not-parse-fom.xml",
                            case.time_implementation,
                        )
                        created = True
                    ambassador.joinFederationExecution(
                        f"matrix-{member}", federation_name
                    )
                    ambassadors.append(ambassador)
                    callbacks.append(callback)

                time_factory = ambassadors[0].getTimeFactory()
                if case.time_implementation == HLA_TYPES.FLOAT64_TIME:
                    target = time_factory.makeLogicalTime(5.5)
                    lookahead = time_factory.makeLogicalTimeInterval(0.25)
                else:
                    target = time_factory.makeLogicalTime(5)
                    lookahead = time_factory.makeLogicalTimeInterval(1)
                for ambassador in ambassadors:
                    ambassador.enableTimeRegulation(lookahead)
                    ambassador.enableTimeConstrained()
                drain(ambassadors[0])
                # Drain each member's setup callbacks before the save request;
                # this gives the matrix a stable per-member prefix.
                for ambassador in ambassadors[1:]:
                    drain(ambassador)

                label = f"save-{case.save_kind}-{case.member_count}"
                if case.save_kind == "timestamped":
                    ambassadors[0].requestFederationSave(label, target)
                else:
                    ambassadors[0].requestFederationSave(label)
                for ambassador in ambassadors:
                    getattr(ambassador, case.advance_service)(target)
                for ambassador in ambassadors:
                    drain(ambassador)
                for ambassador in ambassadors:
                    ambassador.federateSaveBegun()
                    ambassador.federateSaveComplete()
                for ambassador in ambassadors:
                    drain(ambassador)

                ambassadors[0].requestFederationRestore(label)
                for ambassador in ambassadors:
                    drain(ambassador)
                for ambassador in ambassadors:
                    ambassador.federateRestoreComplete()
                for ambassador in ambassadors:
                    drain(ambassador)

                for index, callback in enumerate(callbacks):
                    restore_events = (
                        "requestFederationRestoreSucceeded",
                        "federationRestoreBegun",
                        "initiateFederateRestore",
                        "federationRestored",
                    )
                    if index != 0:
                        restore_events = restore_events[1:]
                    assert_event_order(
                        callback.trace.events,
                        "timeRegulationEnabled",
                        "timeConstrainedEnabled",
                        "initiateFederateSave",
                        "timeAdvanceGrant",
                        "federationSaved",
                        *restore_events,
                    )
                    self.assertEqual(callback.trace.events.count("federationSaved"), 1)
                    self.assertEqual(callback.trace.events.count("federationRestored"), 1)
            finally:
                for ambassador in ambassadors:
                    try:
                        ambassador.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                    try:
                        ambassador.disconnect()
                    except Exception:
                        pass
                if created:
                    cleanup = self.factory.getRtiAmbassador()
                    cleanup.connect(_RecordingFederateAmbassador(), CallbackModel.HLA_IMMEDIATE)
                    try:
                        cleanup.destroyFederationExecution(federation_name)
                    finally:
                        cleanup.disconnect()

    def test_provider_neutral_multi_federate_object_and_interaction_matrix(self) -> None:
        """Exercise discover/reflect/receive ordering across provider members.

        This is deliberately opt-in for the same reason as the save matrix:
        it is a live JVM state-space lane, while the source-checkout provider
        tests cover every matrix point without requiring a JVM.  One publisher
        and one or two subscribers prove that the callback proxy keeps member
        identity, payloads, and ordering independent of callback model and
        logical-time family.
        """

        if os.environ.get("UMBRA_ENABLE_JPYPE_STATE_SPACE_MATRIX") != "1":
            self.skipTest(
                "set UMBRA_ENABLE_JPYPE_STATE_SPACE_MATRIX=1 to run the opt-in JVM state-space lane"
            )
        # Keep scalar object traffic bounded, but cover every standard
        # time-advance spelling so a provider cannot accidentally implement
        # only the ordinary TAR path.  The source-checkout matrix and the
        # companion 2010 JVM lane use the same defaults.
        cases = iter_surface_matrix(
            save_kinds=("scalar",),
            member_counts=(2, 3),
        )
        for case in cases:
            with self.subTest(case=case.case_id):
                federation_name = (
                    f"python-object-matrix-{case.case_id.replace(':', '-')}"
                )
                publisher = self.factory.getRtiAmbassador()
                subscribers: list[object] = []
                publisher_callback = _MatrixFederateAmbassador(SurfaceEventTrace())
                subscriber_callbacks: list[_MatrixFederateAmbassador] = []
                subscriber_attributes: list[AttributeHandle] = []
                created = False

                def drain(ambassador: object) -> None:
                    for _ in range(32):
                        if not ambassador.evokeCallback(0.0):  # type: ignore[attr-defined]
                            return
                    self.fail("callback queue did not drain")

                try:
                    callback_model = CallbackModel[case.callback_model]
                    publisher.connect(publisher_callback, callback_model)
                    publisher.createFederationExecution(
                        federation_name,
                        "fixture-does-not-parse-fom.xml",
                        case.time_implementation,
                    )
                    created = True
                    publisher.joinFederationExecution(
                        "matrix-publisher", federation_name
                    )
                    self.assertEqual(
                        publisher.getTimeFactory().implementationName(),
                        case.time_implementation,
                    )

                    publisher_class = publisher.getObjectClassHandle(
                        HLA_FOM.FOOD_DRINK_SODA
                    )
                    publisher_attribute = publisher.getAttributeHandle(
                        publisher_class, HLA_FIXTURES.FLAVOR
                    )
                    publisher_interaction = publisher.getInteractionClassHandle(
                        HLA_FOM.SODA_SERVED
                    )
                    publisher.publishObjectClassAttributes(
                        publisher_class, AttributeHandleSet([publisher_attribute])
                    )
                    publisher.publishInteractionClass(publisher_interaction)

                    for member in range(case.member_count - 1):
                        subscriber = self.factory.getRtiAmbassador()
                        callback = _MatrixFederateAmbassador(SurfaceEventTrace())
                        subscriber.connect(callback, callback_model)
                        subscriber.joinFederationExecution(
                            f"matrix-subscriber-{member}", federation_name
                        )
                        subscriber_class = subscriber.getObjectClassHandle(
                            HLA_FOM.FOOD_DRINK_SODA
                        )
                        subscriber_attribute = subscriber.getAttributeHandle(
                            subscriber_class, HLA_FIXTURES.FLAVOR
                        )
                        subscriber_interaction = subscriber.getInteractionClassHandle(
                            HLA_FOM.SODA_SERVED
                        )
                        subscriber.subscribeObjectClassAttributes(
                            subscriber_class,
                            AttributeHandleSet([subscriber_attribute]),
                        )
                        subscriber.subscribeInteractionClass(subscriber_interaction)
                        target = subscriber.getTimeFactory().makeLogicalTime(
                            5.5 if case.time_implementation == HLA_TYPES.FLOAT64_TIME else 5
                        )
                        getattr(subscriber, case.advance_service)(target)
                        drain(subscriber)
                        self.assertEqual(
                            subscriber.queryLogicalTime().getTime(), target.getTime()
                        )
                        subscribers.append(subscriber)
                        subscriber_callbacks.append(callback)
                        subscriber_attributes.append(subscriber_attribute)

                    target = publisher.getTimeFactory().makeLogicalTime(
                        5.5 if case.time_implementation == HLA_TYPES.FLOAT64_TIME else 5
                    )
                    getattr(publisher, case.advance_service)(target)
                    drain(publisher)
                    self.assertEqual(
                        publisher.queryLogicalTime().getTime(), target.getTime()
                    )
                    object_instance = publisher.registerObjectInstance(publisher_class)
                    for subscriber in subscribers:
                        drain(subscriber)
                    for callback in subscriber_callbacks:
                        self.assertEqual(len(callback.discoveries), 1)
                        self.assertEqual(callback.discoveries[0][0], object_instance)

                    publisher.updateAttributeValues(
                        object_instance,
                        AttributeHandleValueMap({publisher_attribute: b"matrix-value"}),
                        b"matrix-update-tag",
                    )
                    for subscriber in subscribers:
                        drain(subscriber)
                    for callback, attribute in zip(
                        subscriber_callbacks, subscriber_attributes, strict=True
                    ):
                        self.assertEqual(len(callback.reflections), 1)
                        self.assertEqual(
                            callback.reflections[0][1][attribute], b"matrix-value"
                        )
                        self.assertEqual(callback.reflections[0][2], b"matrix-update-tag")

                    publisher.sendInteraction(
                        publisher_interaction,
                        ParameterHandleValueMap(),
                        b"matrix-interaction-tag",
                    )
                    for subscriber in subscribers:
                        drain(subscriber)
                    for callback in subscriber_callbacks:
                        self.assertEqual(len(callback.interactions), 1)
                        self.assertEqual(
                            callback.interactions[0][2], b"matrix-interaction-tag"
                        )
                        assert_event_order(
                            callback.trace.events,
                            "timeAdvanceGrant",
                            "discoverObjectInstance",
                            "reflectAttributeValues",
                            "receiveInteraction",
                        )
                finally:
                    for subscriber in subscribers:
                        try:
                            subscriber.resignFederationExecution(ResignAction.NO_ACTION)
                        except Exception:
                            pass
                        try:
                            subscriber.disconnect()
                        except Exception:
                            pass
                    try:
                        publisher.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                    if created:
                        cleanup = self.factory.getRtiAmbassador()
                        cleanup.connect(
                            _RecordingFederateAmbassador(), CallbackModel.HLA_IMMEDIATE
                        )
                        try:
                            cleanup.destroyFederationExecution(federation_name)
                        finally:
                            cleanup.disconnect()
                    try:
                        publisher.disconnect()
                    except Exception:
                        pass

    def test_provider_neutral_live_save_restore_outcome_matrix(self) -> None:
        """Exercise lifecycle outcomes through a real Java callback proxy.

        The source-checkout provider test covers the complete 720-point
        forwarding matrix.  This opt-in lane keeps the JVM workload bounded
        while still proving every save outcome and every restore outcome for
        both callback models, both reference time families, both save forms,
        and one- or two-member federations.
        """

        if os.environ.get("UMBRA_ENABLE_JPYPE_STATE_SPACE_MATRIX") != "1":
            self.skipTest(
                "set UMBRA_ENABLE_JPYPE_STATE_SPACE_MATRIX=1 to run the opt-in JVM state-space lane"
            )
        outcomes = ("complete", "not-complete", "abort")
        expected_save_failure = {
            "not-complete": SaveFailureReason.FEDERATE_REPORTED_FAILURE_DURING_SAVE,
            "abort": SaveFailureReason.SAVE_ABORTED,
        }
        expected_restore_failure = {
            "not-complete": RestoreFailureReason.FEDERATE_REPORTED_FAILURE_DURING_RESTORE,
            "abort": RestoreFailureReason.RESTORE_ABORTED,
        }
        cases = iter_surface_matrix(advance_services=("timeAdvanceRequest",))
        for case in cases:
            for save_outcome in outcomes:
                with self.subTest(
                    case=case.case_id,
                    save_outcome=save_outcome,
                ):
                    federation_name = (
                        f"python-save-outcome-{case.case_id.replace(':', '-')}-"
                        f"{save_outcome}"
                    )
                    ambassadors: list[object] = []
                    callbacks: list[_MatrixFederateAmbassador] = []
                    created = False

                    def drain(ambassador: object) -> None:
                        for _ in range(64):
                            if not ambassador.evokeCallback(0.0):  # type: ignore[attr-defined]
                                return
                        self.fail("callback queue did not drain")

                    try:
                        callback_model = CallbackModel[case.callback_model]
                        for member in range(case.member_count):
                            ambassador = self.factory.getRtiAmbassador()
                            callback = _MatrixFederateAmbassador(SurfaceEventTrace())
                            ambassador.connect(callback, callback_model)
                            if member == 0:
                                ambassador.createFederationExecution(
                                    federation_name,
                                    "fixture-does-not-parse-fom.xml",
                                    case.time_implementation,
                                )
                                created = True
                            ambassador.joinFederationExecution(
                                f"save-outcome-{member}", federation_name
                            )
                            ambassadors.append(ambassador)
                            callbacks.append(callback)

                        time_factory = ambassadors[0].getTimeFactory()
                        if case.time_implementation == HLA_TYPES.FLOAT64_TIME:
                            target = time_factory.makeLogicalTime(5.5)
                            lookahead = time_factory.makeLogicalTimeInterval(0.25)
                        else:
                            target = time_factory.makeLogicalTime(5)
                            lookahead = time_factory.makeLogicalTimeInterval(1)
                        for ambassador in ambassadors:
                            ambassador.enableTimeRegulation(lookahead)
                            ambassador.enableTimeConstrained()
                            drain(ambassador)
                        for callback in callbacks:
                            callback.trace.events.clear()

                        save_label = f"save-{case.save_kind}-{save_outcome}"
                        if case.save_kind == "timestamped":
                            ambassadors[0].requestFederationSave(save_label, target)
                            for ambassador in ambassadors:
                                getattr(ambassador, case.advance_service)(target)
                                drain(ambassador)
                        else:
                            ambassadors[0].requestFederationSave(save_label)
                            for ambassador in ambassadors:
                                drain(ambassador)

                        if save_outcome == "complete":
                            for ambassador in ambassadors:
                                ambassador.federateSaveBegun()
                            for ambassador in ambassadors:
                                ambassador.federateSaveComplete()
                        else:
                            ambassadors[0].federateSaveBegun()
                            getattr(
                                ambassadors[0],
                                "federateSaveNotComplete"
                                if save_outcome == "not-complete"
                                else "abortFederationSave",
                            )()
                        for ambassador in ambassadors:
                            drain(ambassador)

                        for callback in callbacks:
                            self.assertEqual(callback.save_initiations, [save_label])
                            if save_outcome == "complete":
                                self.assertEqual(callback.save_completions, 1)
                                self.assertEqual(callback.save_failures, [])
                            else:
                                self.assertEqual(callback.save_completions, 0)
                                self.assertEqual(
                                    callback.save_failures,
                                    [expected_save_failure[save_outcome]],
                                )

                        if save_outcome != "complete":
                            continue

                        for restore_outcome in outcomes:
                            with self.subTest(restore_outcome=restore_outcome):
                                restore_label = (
                                    f"restore-{case.save_kind}-{restore_outcome}"
                                )
                                before_accepted = [
                                    len(callback.restore_accepted) for callback in callbacks
                                ]
                                before_begun = [
                                    callback.restore_begun for callback in callbacks
                                ]
                                before_initiated = [
                                    len(callback.restore_initiations)
                                    for callback in callbacks
                                ]
                                before_completed = [
                                    callback.restore_completions for callback in callbacks
                                ]
                                before_failed = [
                                    len(callback.restore_failures) for callback in callbacks
                                ]
                                ambassadors[0].requestFederationRestore(restore_label)
                                for ambassador in ambassadors:
                                    drain(ambassador)
                                for index, (callback, accepted, begun, initiated) in enumerate(zip(
                                    callbacks,
                                    before_accepted,
                                    before_begun,
                                    before_initiated,
                                    strict=True,
                                )):
                                    self.assertEqual(
                                        len(callback.restore_accepted),
                                        accepted + (1 if index == 0 else 0),
                                    )
                                    self.assertEqual(callback.restore_begun, begun + 1)
                                    self.assertEqual(
                                        len(callback.restore_initiations), initiated + 1
                                    )

                                if restore_outcome == "complete":
                                    for ambassador in ambassadors:
                                        ambassador.federateRestoreComplete()
                                else:
                                    getattr(
                                        ambassadors[0],
                                        "federateRestoreNotComplete"
                                        if restore_outcome == "not-complete"
                                        else "abortFederationRestore",
                                    )()
                                for ambassador in ambassadors:
                                    drain(ambassador)
                                for callback, completed, failed in zip(
                                    callbacks,
                                    before_completed,
                                    before_failed,
                                    strict=True,
                                ):
                                    if restore_outcome == "complete":
                                        self.assertEqual(
                                            callback.restore_completions, completed + 1
                                        )
                                        self.assertEqual(
                                            len(callback.restore_failures), failed
                                        )
                                    else:
                                        self.assertEqual(
                                            callback.restore_completions, completed
                                        )
                                        self.assertEqual(
                                            callback.restore_failures[failed:],
                                            [expected_restore_failure[restore_outcome]],
                                        )
                    finally:
                        for ambassador in ambassadors:
                            try:
                                ambassador.resignFederationExecution(ResignAction.NO_ACTION)
                            except Exception:
                                pass
                            try:
                                ambassador.disconnect()
                            except Exception:
                                pass
                        if created:
                            cleanup = self.factory.getRtiAmbassador()
                            cleanup.connect(
                                _RecordingFederateAmbassador(), CallbackModel.HLA_IMMEDIATE
                            )
                            try:
                                cleanup.destroyFederationExecution(federation_name)
                            finally:
                                cleanup.disconnect()

    def test_probe_jar_uses_standard_factory_factory_without_connecting(self) -> None:
        probe = JavaRtiFactory.probe_jar(
            self._jar_path,
            factory_name="Umbra Mock Java RTI",
        )

        self.assertEqual(probe.rti_name, "Umbra Mock Java RTI")
        self.assertEqual(probe.rti_version, "2025.mock")
        self.assertEqual(probe.configuration.classpath, (str(self._jar_path),))
        self.assertEqual(probe.factory.rtiName(), "Umbra Mock Java RTI")

    def test_real_jvm_restore_preserves_deferred_lookahead_decrease(self) -> None:
        ambassador = self.factory.getRtiAmbassador()
        callbacks = _RecordingFederateAmbassador()
        ambassador.connect(callbacks, CallbackModel.HLA_EVOKED)
        federation_name = "Python deferred lookahead federation"
        ambassador.createFederationExecution(
            federation_name,
            "fixture-does-not-parse-fom.xml",
            HLA_TYPES.INTEGER64_TIME,
        )
        ambassador.joinFederationExecution("deferred-lookahead", federation_name)
        time_factory = ambassador.getTimeFactory()
        ambassador.enableTimeRegulation(time_factory.makeLogicalTimeInterval(5))
        self.assertTrue(ambassador.evokeCallback(0.0))
        ambassador.modifyLookahead(time_factory.makeLogicalTimeInterval(1))
        self.assertEqual(ambassador.queryLookahead().getInterval(), 5)

        save_label = "jvm-deferred-lookahead-baseline"
        ambassador.requestFederationSave(save_label)
        self.assertTrue(ambassador.evokeCallback(0.0))
        ambassador.federateSaveBegun()
        ambassador.federateSaveComplete()
        self.assertTrue(ambassador.evokeCallback(0.0))

        ambassador.timeAdvanceRequest(time_factory.makeLogicalTime(3))
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(ambassador.queryLogicalTime().getTime(), 3)
        self.assertEqual(ambassador.queryLookahead().getInterval(), 2)

        ambassador.requestFederationRestore(save_label)
        for _ in range(3):
            self.assertTrue(ambassador.evokeCallback(0.0))
        ambassador.federateRestoreComplete()
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(ambassador.queryLogicalTime().getTime(), 0)
        self.assertEqual(ambassador.queryLookahead().getInterval(), 5)

        ambassador.timeAdvanceRequest(time_factory.makeLogicalTime(3))
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(ambassador.queryLogicalTime().getTime(), 3)
        self.assertEqual(ambassador.queryLookahead().getInterval(), 2)
        ambassador.resignFederationExecution(ResignAction.NO_ACTION)
        ambassador.disconnect()
