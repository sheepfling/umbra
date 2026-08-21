package hla.rti1516_2025;

import java.util.Set;
import hla.rti1516_2025.time.LogicalTime;

/** Test-fixture subset of the standard callback interface. */
public interface FederateAmbassador {
   void connectionLost(String faultDescription);

   void federateResigned(String reasonForResignDescription);

   void synchronizationPointRegistrationSucceeded(String synchronizationPointLabel);

   void synchronizationPointRegistrationFailed(
      String synchronizationPointLabel, SynchronizationPointFailureReason reason);

   void announceSynchronizationPoint(String synchronizationPointLabel, byte[] userSuppliedTag);

   void federationSynchronized(String synchronizationPointLabel, FederateHandleSet failedToSyncSet);

   void startRegistrationForObjectClass(ObjectClassHandle objectClass);

   void stopRegistrationForObjectClass(ObjectClassHandle objectClass);

   void turnInteractionsOn(InteractionClassHandle interactionClass);

   void turnInteractionsOff(InteractionClassHandle interactionClass);

   void discoverObjectInstance(
      ObjectInstanceHandle objectInstance,
      ObjectClassHandle objectClass,
      String objectInstanceName,
      FederateHandle producingFederate);

   void objectInstanceNameReservationSucceeded(String objectInstanceName);

   void objectInstanceNameReservationFailed(String objectInstanceName);

   void multipleObjectInstanceNameReservationSucceeded(Set<String> objectInstanceNames);

   void multipleObjectInstanceNameReservationFailed(Set<String> objectInstanceNames);

   void provideAttributeValueUpdate(
      ObjectInstanceHandle objectInstance,
      AttributeHandleSet attributes,
      byte[] userSuppliedTag);

   void attributesInScope(ObjectInstanceHandle objectInstance, AttributeHandleSet attributes);

   void attributesOutOfScope(ObjectInstanceHandle objectInstance, AttributeHandleSet attributes);

   void turnUpdatesOnForObjectInstance(
      ObjectInstanceHandle objectInstance, AttributeHandleSet attributes);

   void turnUpdatesOnForObjectInstance(
      ObjectInstanceHandle objectInstance, AttributeHandleSet attributes,
      String updateRateDesignator);

   void turnUpdatesOffForObjectInstance(
      ObjectInstanceHandle objectInstance, AttributeHandleSet attributes);

   void confirmAttributeTransportationTypeChange(
      ObjectInstanceHandle objectInstance,
      AttributeHandleSet attributes,
      TransportationTypeHandle transportationType);

   void reportAttributeTransportationType(
      ObjectInstanceHandle objectInstance,
      AttributeHandle attribute,
      TransportationTypeHandle transportationType);

   void confirmInteractionTransportationTypeChange(
      InteractionClassHandle interactionClass,
      TransportationTypeHandle transportationType);

   void reportInteractionTransportationType(
      FederateHandle federate,
      InteractionClassHandle interactionClass,
      TransportationTypeHandle transportationType);

   void removeObjectInstance(
      ObjectInstanceHandle objectInstance,
      byte[] userSuppliedTag,
      FederateHandle producingFederate);

   void removeObjectInstance(
      ObjectInstanceHandle objectInstance,
      byte[] userSuppliedTag,
      FederateHandle producingFederate,
      LogicalTime time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      MessageRetractionHandle optionalRetraction);

   void reflectAttributeValues(
      ObjectInstanceHandle objectInstance,
      AttributeHandleValueMap attributeValues,
      byte[] userSuppliedTag,
      TransportationTypeHandle transportationType,
      FederateHandle producingFederate,
      RegionHandleSet optionalSentRegions);

   void reflectAttributeValues(
      ObjectInstanceHandle objectInstance,
      AttributeHandleValueMap attributeValues,
      byte[] userSuppliedTag,
      TransportationTypeHandle transportationType,
      FederateHandle producingFederate,
      RegionHandleSet optionalSentRegions,
      LogicalTime time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      MessageRetractionHandle optionalRetraction);

   void receiveInteraction(
      InteractionClassHandle interactionClass,
      ParameterHandleValueMap parameterValues,
      byte[] userSuppliedTag,
      TransportationTypeHandle transportationType,
      FederateHandle producingFederate,
      RegionHandleSet optionalSentRegions);

   void receiveInteraction(
      InteractionClassHandle interactionClass,
      ParameterHandleValueMap parameterValues,
      byte[] userSuppliedTag,
      TransportationTypeHandle transportationType,
      FederateHandle producingFederate,
      RegionHandleSet optionalSentRegions,
      LogicalTime time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      MessageRetractionHandle optionalRetraction);

   void receiveDirectedInteraction(
      InteractionClassHandle interactionClass,
      ObjectInstanceHandle objectInstance,
      ParameterHandleValueMap parameterValues,
      byte[] userSuppliedTag,
      TransportationTypeHandle transportationType,
      FederateHandle producingFederate);

   void receiveDirectedInteraction(
      InteractionClassHandle interactionClass,
      ObjectInstanceHandle objectInstance,
      ParameterHandleValueMap parameterValues,
      byte[] userSuppliedTag,
      TransportationTypeHandle transportationType,
      FederateHandle producingFederate,
      LogicalTime time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      MessageRetractionHandle optionalRetraction);

   void requestAttributeOwnershipAssumption(
      ObjectInstanceHandle objectInstance,
      AttributeHandleSet offeredAttributes,
      byte[] userSuppliedTag);

   void requestDivestitureConfirmation(
      ObjectInstanceHandle objectInstance,
      AttributeHandleSet releasedAttributes,
      byte[] userSuppliedTag);

   void attributeOwnershipAcquisitionNotification(
      ObjectInstanceHandle objectInstance,
      AttributeHandleSet securedAttributes,
      byte[] userSuppliedTag);

   void attributeOwnershipUnavailable(
      ObjectInstanceHandle objectInstance,
      AttributeHandleSet attributes,
      byte[] userSuppliedTag);

   void requestAttributeOwnershipRelease(
      ObjectInstanceHandle objectInstance,
      AttributeHandleSet candidateAttributes,
      byte[] userSuppliedTag);

   void confirmAttributeOwnershipAcquisitionCancellation(
      ObjectInstanceHandle objectInstance,
      AttributeHandleSet attributes);

   void informAttributeOwnership(
      ObjectInstanceHandle objectInstance,
      AttributeHandleSet attributes,
      FederateHandle owner);

   void attributeIsNotOwned(ObjectInstanceHandle objectInstance, AttributeHandleSet attributes);

   void attributeIsOwnedByRTI(ObjectInstanceHandle objectInstance, AttributeHandleSet attributes);

   void timeRegulationEnabled(LogicalTime time);

   void timeConstrainedEnabled(LogicalTime time);

   void flushQueueGrant(LogicalTime time, LogicalTime optimisticTime);

   void timeAdvanceGrant(LogicalTime time);

   void requestRetraction(MessageRetractionHandle retraction);

   void reportFederationExecutions(FederationExecutionInformationSet report);

   void reportFederationExecutionMembers(
      String federationExecutionName,
      FederationExecutionMemberInformationSet report);

   void reportFederationExecutionDoesNotExist(String federationExecutionName);

   void federationSaveStatusResponse(FederateHandleSaveStatusPair[] response);

   void initiateFederateSave(String label);

   void initiateFederateSave(String label, LogicalTime time);

   void federationSaved();

   void federationNotSaved(SaveFailureReason reason);

   void federationRestoreStatusResponse(FederateRestoreStatus[] response);

   void requestFederationRestoreSucceeded(String label);

   void requestFederationRestoreFailed(String label);

   void federationRestoreBegun();

   void initiateFederateRestore(
      String label, String federateName, FederateHandle postRestoreFederateHandle);

   void federationRestored();

   void federationNotRestored(RestoreFailureReason reason);
}
