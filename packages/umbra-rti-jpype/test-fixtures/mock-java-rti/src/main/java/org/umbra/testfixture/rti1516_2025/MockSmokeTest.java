package org.umbra.testfixture.rti1516_2025;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.ConfigurationResult;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederationExecutionInformationSet;
import hla.rti1516_2025.FederationExecutionMemberInformationSet;
import hla.rti1516_2025.HLAfloat64Interval;
import hla.rti1516_2025.HLAfloat64Time;
import hla.rti1516_2025.HLAfloat64TimeFactory;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RestoreFailureReason;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.RtiFactoryFactory;
import hla.rti1516_2025.exceptions.AlreadyConnected;

public final class MockSmokeTest {
   private MockSmokeTest() {
   }

   public static void main(String[] arguments) throws Exception {
      RtiFactory factory = RtiFactoryFactory.getRtiFactory(MockRtiFactory.NAME);
      RTIambassador ambassador = factory.getRtiAmbassador();
      RecordingFederateAmbassador callbacks = new RecordingFederateAmbassador();
      ConfigurationResult result = ambassador.connect(callbacks, CallbackModel.HLA_EVOKED);
      if (result.configurationUsed || result.addressUsed || !"2025.mock".equals(factory.rtiVersion())) {
         throw new AssertionError("Mock factory did not return its expected configuration result");
      }
      try {
         ambassador.connect(callbacks, CallbackModel.HLA_EVOKED);
         throw new AssertionError("Expected AlreadyConnected");
      } catch (AlreadyConnected expected) {
         // Expected state-transition check.
      }
      MockRTIambassador mockAmbassador = (MockRTIambassador) ambassador;
      mockAmbassador.queueConnectionLost("mock connection loss");
      if (!ambassador.evokeCallback(0.0) || !"mock connection loss".equals(callbacks.description)) {
         throw new AssertionError("Mock callback was not delivered");
      }
      HLAfloat64TimeFactory floatFactory = new HLAfloat64TimeFactory();
      HLAfloat64Time floatTime = floatFactory.makeLogicalTime(12.5);
      HLAfloat64Interval floatInterval = floatFactory.makeLogicalTimeInterval(0.25);
      if (floatTime.getTime() != 12.5 || floatInterval.getInterval() != 0.25) {
         throw new AssertionError("Floating logical-time values were not preserved");
      }
      ambassador.createFederationExecution(
         "floating smoke federation", "fixture.xml", "HLAfloat64Time");
      ambassador.joinFederationExecution("observer", "floating smoke federation");
      if (!(mockAmbassador.getTimeFactory() instanceof HLAfloat64TimeFactory)) {
         throw new AssertionError("Mock ambassador did not select floating logical time");
      }
      ambassador.requestFederationSave("floating-timed-save", floatFactory.makeLogicalTime(1.0));
      if (callbacks.saveLabel != null) {
         throw new AssertionError("Timed save started before its time boundary");
      }
      ambassador.timeAdvanceRequest(floatFactory.makeLogicalTime(1.0));
      if (!ambassador.evokeCallback(0.0) || !"floating-timed-save".equals(callbacks.saveLabel)) {
         throw new AssertionError("Timed save initiation was not delivered at its boundary");
      }
      if (!ambassador.evokeCallback(0.0)) {
         throw new AssertionError("Timed save grant was not delivered");
      }
      ambassador.federateSaveBegun();
      ambassador.federateSaveComplete();
      if (!ambassador.evokeCallback(0.0)) {
         throw new AssertionError("Timed save completion was not delivered");
      }

      hla.rti1516_2025.InteractionClassHandle interaction =
         ambassador.getInteractionClassHandle("fixture-interaction");
      hla.rti1516_2025.ParameterHandle parameter =
         ambassador.getParameterHandle(interaction, "fixture-parameter");
      hla.rti1516_2025.ParameterHandleValueMap values =
         ambassador.getParameterHandleValueMapFactory().create(0);
      values.put(parameter, new byte[] { 4 });
      HLAfloat64Time tsoSaveTime = floatFactory.makeLogicalTime(2.0);
      mockAmbassador.enableTsoSaveBoundaryProbe();
      ambassador.sendInteractionWithTime(
         interaction, values, new byte[] { 5 }, tsoSaveTime);
      callbacks.saveLabel = null;
      ambassador.requestFederationSave("floating-tso-save", tsoSaveTime);
      int grantsBefore = callbacks.grants;
      ambassador.timeAdvanceRequest(tsoSaveTime);
      if (!ambassador.evokeCallback(0.0)
         || !callbacks.timestampedInteraction
         || callbacks.saveLabel != null) {
         throw new AssertionError("Timestamped interaction was not delivered before save initiation");
      }
      if (!ambassador.evokeCallback(0.0)
         || !"floating-tso-save".equals(callbacks.saveLabel)) {
         throw new AssertionError("TSO-boundary save initiation was not delivered");
      }
      if (!ambassador.evokeCallback(0.0) || callbacks.grants != grantsBefore + 1) {
         throw new AssertionError("TSO-boundary save grant was not delivered after initiation");
      }
      ambassador.federateSaveBegun();
      ambassador.federateSaveComplete();
      if (!ambassador.evokeCallback(0.0)) {
         throw new AssertionError("TSO-boundary save completion was not delivered");
      }
      ambassador.disconnect();
      System.out.println("Mock Java RTI smoke test: OK");
   }

   private static final class RecordingFederateAmbassador implements FederateAmbassador {
      private String description;
      private String saveLabel;
      private boolean timestampedInteraction;
      private int grants;

      @Override
      public void connectionLost(String faultDescription) {
         description = faultDescription;
      }

      @Override
      public void federateResigned(String reasonForResignDescription) {
      }

      @Override
      public void flushQueueGrant(
         LogicalTime time,
         LogicalTime optimisticTime) {
      }

      @Override
      public void requestRetraction(hla.rti1516_2025.MessageRetractionHandle retraction) {
      }

      @Override
      public void synchronizationPointRegistrationSucceeded(String synchronizationPointLabel) {
      }

      @Override
      public void synchronizationPointRegistrationFailed(
         String synchronizationPointLabel,
         hla.rti1516_2025.SynchronizationPointFailureReason reason) {
      }

      @Override
      public void announceSynchronizationPoint(
         String synchronizationPointLabel, byte[] userSuppliedTag) {
      }

      @Override
      public void federationSynchronized(
         String synchronizationPointLabel,
         hla.rti1516_2025.FederateHandleSet failedToSyncSet) {
      }

      @Override
      public void startRegistrationForObjectClass(
         hla.rti1516_2025.ObjectClassHandle objectClass) {
      }

      @Override
      public void stopRegistrationForObjectClass(
         hla.rti1516_2025.ObjectClassHandle objectClass) {
      }

      @Override
      public void turnInteractionsOn(
         hla.rti1516_2025.InteractionClassHandle interactionClass) {
      }

      @Override
      public void turnInteractionsOff(
         hla.rti1516_2025.InteractionClassHandle interactionClass) {
      }

      @Override
      public void discoverObjectInstance(
         hla.rti1516_2025.ObjectInstanceHandle objectInstance,
         hla.rti1516_2025.ObjectClassHandle objectClass,
         String objectInstanceName,
         hla.rti1516_2025.FederateHandle producingFederate) {
      }

      @Override
      public void objectInstanceNameReservationSucceeded(String objectInstanceName) {
      }

      @Override
      public void objectInstanceNameReservationFailed(String objectInstanceName) {
      }

      @Override
      public void multipleObjectInstanceNameReservationSucceeded(java.util.Set<String> objectInstanceNames) {
      }

      @Override
      public void multipleObjectInstanceNameReservationFailed(java.util.Set<String> objectInstanceNames) {
      }

      @Override
      public void provideAttributeValueUpdate(
         hla.rti1516_2025.ObjectInstanceHandle objectInstance,
         hla.rti1516_2025.AttributeHandleSet attributes,
         byte[] userSuppliedTag) {
      }

      @Override
      public void attributesInScope(
         hla.rti1516_2025.ObjectInstanceHandle objectInstance,
         hla.rti1516_2025.AttributeHandleSet attributes) {
      }

      @Override
      public void attributesOutOfScope(
         hla.rti1516_2025.ObjectInstanceHandle objectInstance,
         hla.rti1516_2025.AttributeHandleSet attributes) {
      }

      @Override
      public void turnUpdatesOnForObjectInstance(
         hla.rti1516_2025.ObjectInstanceHandle objectInstance,
         hla.rti1516_2025.AttributeHandleSet attributes) {
      }

      @Override
      public void turnUpdatesOnForObjectInstance(
         hla.rti1516_2025.ObjectInstanceHandle objectInstance,
         hla.rti1516_2025.AttributeHandleSet attributes,
         String updateRateDesignator) {
      }

      @Override
      public void turnUpdatesOffForObjectInstance(
         hla.rti1516_2025.ObjectInstanceHandle objectInstance,
         hla.rti1516_2025.AttributeHandleSet attributes) {
      }

      @Override
      public void confirmAttributeTransportationTypeChange(
         hla.rti1516_2025.ObjectInstanceHandle objectInstance,
         hla.rti1516_2025.AttributeHandleSet attributes,
         hla.rti1516_2025.TransportationTypeHandle transportationType) {
      }

      @Override
      public void reportAttributeTransportationType(
         hla.rti1516_2025.ObjectInstanceHandle objectInstance,
         hla.rti1516_2025.AttributeHandle attribute,
         hla.rti1516_2025.TransportationTypeHandle transportationType) {
      }

      @Override
      public void confirmInteractionTransportationTypeChange(
         hla.rti1516_2025.InteractionClassHandle interactionClass,
         hla.rti1516_2025.TransportationTypeHandle transportationType) {
      }

      @Override
      public void reportInteractionTransportationType(
         hla.rti1516_2025.FederateHandle federate,
         hla.rti1516_2025.InteractionClassHandle interactionClass,
         hla.rti1516_2025.TransportationTypeHandle transportationType) {
      }

      @Override
      public void removeObjectInstance(
         hla.rti1516_2025.ObjectInstanceHandle objectInstance,
         byte[] userSuppliedTag,
         hla.rti1516_2025.FederateHandle producingFederate) {
      }

      @Override
      public void removeObjectInstance(
         hla.rti1516_2025.ObjectInstanceHandle objectInstance,
         byte[] userSuppliedTag,
         hla.rti1516_2025.FederateHandle producingFederate,
         LogicalTime time,
         hla.rti1516_2025.OrderType sentOrderType,
         hla.rti1516_2025.OrderType receivedOrderType,
         hla.rti1516_2025.MessageRetractionHandle optionalRetraction) {
      }

      @Override
      public void reflectAttributeValues(
         hla.rti1516_2025.ObjectInstanceHandle objectInstance,
         hla.rti1516_2025.AttributeHandleValueMap attributeValues,
         byte[] userSuppliedTag,
         hla.rti1516_2025.TransportationTypeHandle transportationType,
         hla.rti1516_2025.FederateHandle producingFederate,
         hla.rti1516_2025.RegionHandleSet optionalSentRegions) {
      }

      @Override
      public void reflectAttributeValues(
         hla.rti1516_2025.ObjectInstanceHandle objectInstance,
         hla.rti1516_2025.AttributeHandleValueMap attributeValues,
         byte[] userSuppliedTag,
         hla.rti1516_2025.TransportationTypeHandle transportationType,
         hla.rti1516_2025.FederateHandle producingFederate,
         hla.rti1516_2025.RegionHandleSet optionalSentRegions,
         LogicalTime time,
         hla.rti1516_2025.OrderType sentOrderType,
         hla.rti1516_2025.OrderType receivedOrderType,
         hla.rti1516_2025.MessageRetractionHandle optionalRetraction) {
      }

      @Override
      public void receiveInteraction(
         hla.rti1516_2025.InteractionClassHandle interactionClass,
         hla.rti1516_2025.ParameterHandleValueMap parameterValues,
         byte[] userSuppliedTag,
         hla.rti1516_2025.TransportationTypeHandle transportationType,
         hla.rti1516_2025.FederateHandle producingFederate,
         hla.rti1516_2025.RegionHandleSet optionalSentRegions) {
      }

      @Override
      public void receiveInteraction(
         hla.rti1516_2025.InteractionClassHandle interactionClass,
         hla.rti1516_2025.ParameterHandleValueMap parameterValues,
         byte[] userSuppliedTag,
         hla.rti1516_2025.TransportationTypeHandle transportationType,
         hla.rti1516_2025.FederateHandle producingFederate,
         hla.rti1516_2025.RegionHandleSet optionalSentRegions,
         LogicalTime time,
         hla.rti1516_2025.OrderType sentOrderType,
         hla.rti1516_2025.OrderType receivedOrderType,
         hla.rti1516_2025.MessageRetractionHandle optionalRetraction) {
         timestampedInteraction = true;
      }

      @Override
      public void receiveDirectedInteraction(
         hla.rti1516_2025.InteractionClassHandle interactionClass,
         hla.rti1516_2025.ObjectInstanceHandle objectInstance,
         hla.rti1516_2025.ParameterHandleValueMap parameterValues,
         byte[] userSuppliedTag,
         hla.rti1516_2025.TransportationTypeHandle transportationType,
         hla.rti1516_2025.FederateHandle producingFederate) {
      }

      @Override
      public void receiveDirectedInteraction(
         hla.rti1516_2025.InteractionClassHandle interactionClass,
         hla.rti1516_2025.ObjectInstanceHandle objectInstance,
         hla.rti1516_2025.ParameterHandleValueMap parameterValues,
         byte[] userSuppliedTag,
         hla.rti1516_2025.TransportationTypeHandle transportationType,
         hla.rti1516_2025.FederateHandle producingFederate,
         LogicalTime time,
         hla.rti1516_2025.OrderType sentOrderType,
         hla.rti1516_2025.OrderType receivedOrderType,
         hla.rti1516_2025.MessageRetractionHandle optionalRetraction) {
      }

      @Override
      public void requestAttributeOwnershipAssumption(
         hla.rti1516_2025.ObjectInstanceHandle objectInstance,
         hla.rti1516_2025.AttributeHandleSet offeredAttributes,
         byte[] userSuppliedTag) {
      }

      @Override
      public void requestDivestitureConfirmation(
         hla.rti1516_2025.ObjectInstanceHandle objectInstance,
         hla.rti1516_2025.AttributeHandleSet releasedAttributes,
         byte[] userSuppliedTag) {
      }

      @Override
      public void attributeOwnershipAcquisitionNotification(
         hla.rti1516_2025.ObjectInstanceHandle objectInstance,
         hla.rti1516_2025.AttributeHandleSet securedAttributes,
         byte[] userSuppliedTag) {
      }

      @Override
      public void attributeOwnershipUnavailable(
         hla.rti1516_2025.ObjectInstanceHandle objectInstance,
         hla.rti1516_2025.AttributeHandleSet attributes,
         byte[] userSuppliedTag) {
      }

      @Override
      public void requestAttributeOwnershipRelease(
         hla.rti1516_2025.ObjectInstanceHandle objectInstance,
         hla.rti1516_2025.AttributeHandleSet candidateAttributes,
         byte[] userSuppliedTag) {
      }

      @Override
      public void confirmAttributeOwnershipAcquisitionCancellation(
         hla.rti1516_2025.ObjectInstanceHandle objectInstance,
         hla.rti1516_2025.AttributeHandleSet attributes) {
      }

      @Override
      public void informAttributeOwnership(
         hla.rti1516_2025.ObjectInstanceHandle objectInstance,
         hla.rti1516_2025.AttributeHandleSet attributes,
         hla.rti1516_2025.FederateHandle owner) {
      }

      @Override
      public void attributeIsNotOwned(
         hla.rti1516_2025.ObjectInstanceHandle objectInstance,
         hla.rti1516_2025.AttributeHandleSet attributes) {
      }

      @Override
      public void attributeIsOwnedByRTI(
         hla.rti1516_2025.ObjectInstanceHandle objectInstance,
         hla.rti1516_2025.AttributeHandleSet attributes) {
      }

      @Override
      public void timeRegulationEnabled(LogicalTime time) {
      }

      @Override
      public void timeConstrainedEnabled(LogicalTime time) {
      }

      @Override
      public void timeAdvanceGrant(LogicalTime time) {
         grants++;
      }

      @Override
      public void reportFederationExecutions(FederationExecutionInformationSet report) {
         // The Java-only smoke test covers the connection callback; Python
         // integration tests exercise the typed federation report callback.
      }

      @Override
      public void reportFederationExecutionMembers(
         String federationExecutionName,
         FederationExecutionMemberInformationSet report) {
         // Python integration tests exercise this callback conversion.
      }

      @Override
      public void reportFederationExecutionDoesNotExist(String federationExecutionName) {
         // Python integration tests exercise this callback conversion.
      }

      @Override
      public void federationSaveStatusResponse(
         hla.rti1516_2025.FederateHandleSaveStatusPair[] response) {
      }

      @Override
      public void initiateFederateSave(String label) {
         saveLabel = label;
      }

      @Override
      public void initiateFederateSave(String label, LogicalTime time) {
         saveLabel = label;
      }

      @Override
      public void federationSaved() {
      }

      @Override
      public void federationNotSaved(hla.rti1516_2025.SaveFailureReason reason) {
      }

      @Override
      public void federationRestoreStatusResponse(
         hla.rti1516_2025.FederateRestoreStatus[] response) {
      }

      @Override
      public void requestFederationRestoreSucceeded(String label) {
      }

      @Override
      public void requestFederationRestoreFailed(String label) {
      }

      @Override
      public void federationRestoreBegun() {
      }

      @Override
      public void initiateFederateRestore(
         String label,
         String federateName,
         hla.rti1516_2025.FederateHandle postRestoreFederateHandle) {
      }

      @Override
      public void federationRestored() {
      }

      @Override
      public void federationNotRestored(RestoreFailureReason reason) {
      }
   }
}
