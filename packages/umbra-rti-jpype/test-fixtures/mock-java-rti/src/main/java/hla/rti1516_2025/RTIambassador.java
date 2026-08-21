package hla.rti1516_2025;

import java.util.Set;

import hla.rti1516_2025.exceptions.AlreadyConnected;
import hla.rti1516_2025.exceptions.FederateNotExecutionMember;
import hla.rti1516_2025.exceptions.NotConnected;
import hla.rti1516_2025.exceptions.RTIexception;
import hla.rti1516_2025.exceptions.ObjectInstanceNotKnown;
import hla.rti1516_2025.auth.Credentials;

/** Test-fixture subset of the standard ambassador interface. */
public interface RTIambassador {
   ConfigurationResult connect(FederateAmbassador federateAmbassador, CallbackModel callbackModel)
      throws AlreadyConnected;

   ConfigurationResult connect(
      FederateAmbassador federateAmbassador,
      CallbackModel callbackModel,
      RtiConfiguration configuration) throws AlreadyConnected;

   ConfigurationResult connect(
      FederateAmbassador federateAmbassador,
      CallbackModel callbackModel,
      Credentials credentials) throws AlreadyConnected;

   ConfigurationResult connect(
      FederateAmbassador federateAmbassador,
      CallbackModel callbackModel,
      RtiConfiguration configuration,
      Credentials credentials) throws AlreadyConnected;

   void disconnect() throws NotConnected;

   boolean evokeCallback(double approximateMinimumTimeInSeconds) throws NotConnected;

   boolean evokeMultipleCallbacks(
      double approximateMinimumTimeInSeconds,
      double approximateMaximumTimeInSeconds) throws NotConnected;

   void enableCallbacks() throws NotConnected;

   void disableCallbacks() throws NotConnected;

   void listFederationExecutions() throws NotConnected;

   void listFederationExecutionMembers(String federationExecutionName) throws NotConnected;

   FederateHandle joinFederationExecution(String federateType, String federationExecutionName)
      throws NotConnected;

   FederateHandle joinFederationExecution(
      String federateName, String federateType, String federationExecutionName) throws NotConnected;

   FederateHandle joinFederationExecution(
      String federateType, String federationExecutionName, String[] additionalFomModules)
      throws NotConnected;

   FederateHandle joinFederationExecution(
      String federateName,
      String federateType,
      String federationExecutionName,
      String[] additionalFomModules) throws NotConnected;

   void resignFederationExecution(ResignAction resignAction) throws NotConnected;

   void registerFederationSynchronizationPoint(String label, byte[] userSuppliedTag)
      throws NotConnected;

   void registerFederationSynchronizationPoint(
      String label, byte[] userSuppliedTag, FederateHandleSet synchronizationSet)
      throws NotConnected;

   void synchronizationPointAchieved(String label, boolean successfully) throws NotConnected;

   void queryFederationSaveStatus() throws NotConnected;

   void requestFederationSave(String label) throws NotConnected;

   void requestFederationSave(String label, LogicalTime time) throws RTIexception;

   void federateSaveBegun() throws NotConnected;

   void federateSaveComplete() throws NotConnected;

   void federateSaveNotComplete() throws NotConnected;

   void abortFederationSave() throws NotConnected;

   void queryFederationRestoreStatus() throws NotConnected;

   void requestFederationRestore(String label) throws NotConnected;

   void federateRestoreComplete() throws NotConnected;

   void federateRestoreNotComplete() throws NotConnected;

   void abortFederationRestore() throws NotConnected;

   ObjectClassHandle getObjectClassHandle(String objectClassName) throws NotConnected;

   String getObjectClassName(ObjectClassHandle objectClass) throws NotConnected;

   AttributeHandle getAttributeHandle(ObjectClassHandle objectClass, String attributeName)
      throws NotConnected;

   String getAttributeName(ObjectClassHandle objectClass, AttributeHandle attribute)
      throws NotConnected;

   InteractionClassHandle getInteractionClassHandle(String interactionClassName) throws NotConnected;

   String getInteractionClassName(InteractionClassHandle interactionClass) throws NotConnected;

   ParameterHandle getParameterHandle(InteractionClassHandle interactionClass, String parameterName)
      throws NotConnected;

   String getParameterName(InteractionClassHandle interactionClass, ParameterHandle parameter)
      throws NotConnected;

   TransportationTypeHandle getTransportationTypeHandle(String transportationTypeName)
      throws NotConnected;

   String getTransportationTypeName(TransportationTypeHandle transportationType) throws NotConnected;

   DimensionHandle getDimensionHandle(String dimensionName) throws NotConnected;

   String getDimensionName(DimensionHandle dimension) throws NotConnected;

   FederateHandle getFederateHandle(String federateName)
      throws FederateNotExecutionMember, NotConnected;

   String getFederateName(FederateHandle federate) throws NotConnected;

   ObjectClassHandle getKnownObjectClassHandle(ObjectInstanceHandle objectInstance)
      throws NotConnected;

   double getUpdateRateValue(String updateRateDesignator) throws NotConnected;

   double getUpdateRateValueForAttribute(ObjectInstanceHandle objectInstance, AttributeHandle attribute)
      throws NotConnected;

   OrderType getOrderType(String orderTypeName) throws NotConnected;

   String getOrderName(OrderType orderType) throws NotConnected;

   DimensionHandleSet getAvailableDimensionsForObjectClass(ObjectClassHandle objectClass)
      throws NotConnected;

   DimensionHandleSet getAvailableDimensionsForInteractionClass(InteractionClassHandle interactionClass)
      throws NotConnected;

   long getDimensionUpperBound(DimensionHandle dimension) throws NotConnected;

   long normalizeServiceGroup(ServiceGroup serviceGroup) throws NotConnected;

   long normalizeFederateHandle(FederateHandle federate) throws NotConnected;

   long normalizeObjectClassHandle(ObjectClassHandle objectClass) throws NotConnected;

   long normalizeInteractionClassHandle(InteractionClassHandle interactionClass)
      throws NotConnected;

   long normalizeObjectInstanceHandle(ObjectInstanceHandle objectInstance)
      throws FederateNotExecutionMember, NotConnected;

   MessageRetractionHandle deleteObjectInstanceWithTime(
      ObjectInstanceHandle objectInstance, byte[] userSuppliedTag, LogicalTime time)
      throws NotConnected;

   MessageRetractionHandle updateAttributeValuesWithTime(
      ObjectInstanceHandle objectInstance,
      AttributeHandleValueMap attributeValues,
      byte[] userSuppliedTag,
      LogicalTime time) throws NotConnected;

   MessageRetractionHandle sendInteractionWithTime(
      InteractionClassHandle interactionClass,
      ParameterHandleValueMap parameterValues,
      byte[] userSuppliedTag,
      LogicalTime time) throws NotConnected;

   MessageRetractionHandle sendInteractionWithRegionsWithTime(
      InteractionClassHandle interactionClass,
      ParameterHandleValueMap parameterValues,
      RegionHandleSet regions,
      byte[] userSuppliedTag,
      LogicalTime time) throws NotConnected;

   void localDeleteObjectInstance(ObjectInstanceHandle objectInstance) throws NotConnected;

   void retract(MessageRetractionHandle retraction) throws NotConnected;

   FederateHandleFactory getFederateHandleFactory() throws NotConnected;

   FederateHandleSetFactory getFederateHandleSetFactory() throws NotConnected;

   ObjectClassHandleFactory getObjectClassHandleFactory() throws NotConnected;

   AttributeHandleFactory getAttributeHandleFactory() throws NotConnected;

   InteractionClassHandleFactory getInteractionClassHandleFactory() throws NotConnected;

   InteractionClassHandleSetFactory getInteractionClassHandleSetFactory() throws NotConnected;

   ParameterHandleFactory getParameterHandleFactory() throws NotConnected;

   TransportationTypeHandleFactory getTransportationTypeHandleFactory() throws NotConnected;

   DimensionHandleFactory getDimensionHandleFactory() throws NotConnected;

   RegionHandleFactory getRegionHandleFactory() throws NotConnected;

   MessageRetractionHandleFactory getMessageRetractionHandleFactory() throws NotConnected;

   DimensionHandleSetFactory getDimensionHandleSetFactory() throws NotConnected;

   RegionHandleSetFactory getRegionHandleSetFactory() throws NotConnected;

   AttributeHandleSetFactory getAttributeHandleSetFactory() throws NotConnected;

   AttributeHandleValueMapFactory getAttributeHandleValueMapFactory() throws NotConnected;

   ParameterHandleValueMapFactory getParameterHandleValueMapFactory() throws NotConnected;

   void publishObjectClassAttributes(ObjectClassHandle objectClass, AttributeHandleSet attributes)
      throws NotConnected;

   void unpublishObjectClass(ObjectClassHandle objectClass) throws NotConnected;

   void unpublishObjectClassAttributes(ObjectClassHandle objectClass, AttributeHandleSet attributes)
      throws NotConnected;

   void publishObjectClassDirectedInteractions(
      ObjectClassHandle objectClass, InteractionClassHandleSet interactionClasses)
      throws NotConnected;

   void unpublishObjectClassDirectedInteractions(ObjectClassHandle objectClass)
      throws NotConnected;

   void unpublishObjectClassDirectedInteractions(
      ObjectClassHandle objectClass, InteractionClassHandleSet interactionClasses)
      throws NotConnected;

   void subscribeObjectClassAttributes(
      ObjectClassHandle objectClass,
      AttributeHandleSet attributes) throws NotConnected;

   void subscribeObjectClassAttributes(
      ObjectClassHandle objectClass,
      AttributeHandleSet attributes,
      String updateRateDesignator) throws NotConnected;

   void subscribeObjectClassAttributesPassively(
      ObjectClassHandle objectClass,
      AttributeHandleSet attributes) throws NotConnected;

   void subscribeObjectClassAttributesPassively(
      ObjectClassHandle objectClass,
      AttributeHandleSet attributes,
      String updateRateDesignator) throws NotConnected;

   void unsubscribeObjectClass(ObjectClassHandle objectClass) throws NotConnected;

   void unsubscribeObjectClassAttributes(ObjectClassHandle objectClass, AttributeHandleSet attributes)
      throws NotConnected;

   void subscribeObjectClassDirectedInteractions(
      ObjectClassHandle objectClass,
      InteractionClassHandleSet interactionClasses) throws NotConnected;

   void subscribeObjectClassDirectedInteractionsUniversally(
      ObjectClassHandle objectClass,
      InteractionClassHandleSet interactionClasses) throws NotConnected;

   void unsubscribeObjectClassDirectedInteractions(ObjectClassHandle objectClass)
      throws NotConnected;

   void unsubscribeObjectClassDirectedInteractions(
      ObjectClassHandle objectClass, InteractionClassHandleSet interactionClasses)
      throws NotConnected;

   void subscribeObjectClassAttributesWithRegions(
      ObjectClassHandle objectClass,
      AttributeSetRegionSetPairList attributesAndRegions) throws NotConnected;

   void subscribeObjectClassAttributesWithRegions(
      ObjectClassHandle objectClass,
      AttributeSetRegionSetPairList attributesAndRegions,
      String updateRateDesignator) throws NotConnected;

   void subscribeObjectClassAttributesPassivelyWithRegions(
      ObjectClassHandle objectClass,
      AttributeSetRegionSetPairList attributesAndRegions) throws NotConnected;

   void subscribeObjectClassAttributesPassivelyWithRegions(
      ObjectClassHandle objectClass,
      AttributeSetRegionSetPairList attributesAndRegions,
      String updateRateDesignator) throws NotConnected;

   void unsubscribeObjectClassAttributesWithRegions(
      ObjectClassHandle objectClass,
      AttributeSetRegionSetPairList attributesAndRegions) throws NotConnected;

   void publishInteractionClass(InteractionClassHandle interactionClass) throws NotConnected;

   void unpublishInteractionClass(InteractionClassHandle interactionClass) throws NotConnected;

   void subscribeInteractionClass(InteractionClassHandle interactionClass)
      throws NotConnected;

   void subscribeInteractionClassPassively(InteractionClassHandle interactionClass)
      throws NotConnected;

   void subscribeInteractionClassWithRegions(
      InteractionClassHandle interactionClass, RegionHandleSet regions)
      throws NotConnected;

   void subscribeInteractionClassPassivelyWithRegions(
      InteractionClassHandle interactionClass, RegionHandleSet regions)
      throws NotConnected;

   void unsubscribeInteractionClass(InteractionClassHandle interactionClass) throws NotConnected;

   void unsubscribeInteractionClassWithRegions(
      InteractionClassHandle interactionClass, RegionHandleSet regions) throws NotConnected;

   void reserveObjectInstanceName(String objectInstanceName) throws NotConnected;

   void releaseObjectInstanceName(String objectInstanceName) throws NotConnected;

   void reserveMultipleObjectInstanceNames(Set<String> objectInstanceNames) throws NotConnected;

   void releaseMultipleObjectInstanceNames(Set<String> objectInstanceNames) throws NotConnected;

   ObjectInstanceHandle registerObjectInstance(ObjectClassHandle objectClass) throws NotConnected;

   ObjectInstanceHandle registerObjectInstance(ObjectClassHandle objectClass, String objectInstanceName)
      throws NotConnected;

   ObjectInstanceHandle registerObjectInstanceWithRegions(
      ObjectClassHandle objectClass,
      AttributeSetRegionSetPairList attributesAndRegions) throws NotConnected;

   ObjectInstanceHandle registerObjectInstanceWithRegions(
      ObjectClassHandle objectClass,
      AttributeSetRegionSetPairList attributesAndRegions,
      String objectInstanceName) throws NotConnected;

   void associateRegionsForUpdates(
      ObjectInstanceHandle objectInstance,
      AttributeSetRegionSetPairList attributesAndRegions) throws NotConnected;

   void unassociateRegionsForUpdates(
      ObjectInstanceHandle objectInstance,
      AttributeSetRegionSetPairList attributesAndRegions) throws NotConnected;

   ObjectInstanceHandle getObjectInstanceHandle(String objectInstanceName)
      throws ObjectInstanceNotKnown, FederateNotExecutionMember, NotConnected;

   String getObjectInstanceName(ObjectInstanceHandle objectInstance)
      throws ObjectInstanceNotKnown, FederateNotExecutionMember, NotConnected;

   ObjectInstanceHandleFactory getObjectInstanceHandleFactory() throws NotConnected;

   void deleteObjectInstance(ObjectInstanceHandle objectInstance, byte[] userSuppliedTag)
      throws NotConnected;

   void updateAttributeValues(
      ObjectInstanceHandle objectInstance,
      AttributeHandleValueMap attributeValues,
      byte[] userSuppliedTag) throws NotConnected;

   void requestAttributeValueUpdate(
      ObjectInstanceHandle objectInstance,
      AttributeHandleSet attributes,
      byte[] userSuppliedTag) throws NotConnected;

   void requestAttributeValueUpdate(
      ObjectClassHandle objectClass,
      AttributeHandleSet attributes,
      byte[] userSuppliedTag) throws NotConnected;

   void requestAttributeValueUpdateWithRegions(
      ObjectClassHandle objectClass,
      AttributeSetRegionSetPairList attributesAndRegions,
      byte[] userSuppliedTag) throws NotConnected;

   void changeAttributeOrderType(
      ObjectInstanceHandle objectInstance,
      AttributeHandleSet attributes,
      OrderType orderType) throws NotConnected;

   void changeDefaultAttributeOrderType(
      ObjectClassHandle objectClass,
      AttributeHandleSet attributes,
      OrderType orderType) throws NotConnected;

   void changeInteractionOrderType(
      InteractionClassHandle interactionClass,
      OrderType orderType) throws NotConnected;

   void requestAttributeTransportationTypeChange(
      ObjectInstanceHandle objectInstance,
      AttributeHandleSet attributes,
      TransportationTypeHandle transportationType) throws NotConnected;

   void changeDefaultAttributeTransportationType(
      ObjectClassHandle objectClass,
      AttributeHandleSet attributes,
      TransportationTypeHandle transportationType) throws NotConnected;

   void queryAttributeTransportationType(
      ObjectInstanceHandle objectInstance,
      AttributeHandle attribute) throws NotConnected;

   void requestInteractionTransportationTypeChange(
      InteractionClassHandle interactionClass,
      TransportationTypeHandle transportationType) throws NotConnected;

   void queryInteractionTransportationType(
      FederateHandle federate,
      InteractionClassHandle interactionClass) throws NotConnected;

   void queryAttributeOwnership(
      ObjectInstanceHandle objectInstance,
      AttributeHandleSet attributes) throws NotConnected;

   boolean isAttributeOwnedByFederate(
      ObjectInstanceHandle objectInstance,
      AttributeHandle attribute) throws NotConnected;

   void unconditionalAttributeOwnershipDivestiture(
      ObjectInstanceHandle objectInstance,
      AttributeHandleSet attributes,
      byte[] userSuppliedTag) throws NotConnected;

   void negotiatedAttributeOwnershipDivestiture(
      ObjectInstanceHandle objectInstance,
      AttributeHandleSet attributes,
      byte[] userSuppliedTag) throws NotConnected;

   void confirmDivestiture(
      ObjectInstanceHandle objectInstance,
      AttributeHandleSet confirmedAttributes,
      byte[] userSuppliedTag) throws NotConnected;

   void cancelNegotiatedAttributeOwnershipDivestiture(
      ObjectInstanceHandle objectInstance,
      AttributeHandleSet attributes) throws NotConnected;

   void attributeOwnershipAcquisition(
      ObjectInstanceHandle objectInstance,
      AttributeHandleSet desiredAttributes,
      byte[] userSuppliedTag) throws NotConnected;

   void attributeOwnershipAcquisitionIfAvailable(
      ObjectInstanceHandle objectInstance,
      AttributeHandleSet desiredAttributes,
      byte[] userSuppliedTag) throws NotConnected;

   void cancelAttributeOwnershipAcquisition(
      ObjectInstanceHandle objectInstance,
      AttributeHandleSet attributes) throws NotConnected;

   void attributeOwnershipReleaseDenied(
      ObjectInstanceHandle objectInstance,
      AttributeHandleSet attributes,
      byte[] userSuppliedTag) throws NotConnected;

   AttributeHandleSet attributeOwnershipDivestitureIfWanted(
      ObjectInstanceHandle objectInstance,
      AttributeHandleSet attributes,
      byte[] userSuppliedTag) throws NotConnected;

   void sendInteraction(
      InteractionClassHandle interactionClass,
      ParameterHandleValueMap parameterValues,
      byte[] userSuppliedTag) throws NotConnected;

   void sendDirectedInteraction(
      InteractionClassHandle interactionClass,
      ObjectInstanceHandle objectInstance,
      ParameterHandleValueMap parameterValues,
      byte[] userSuppliedTag) throws NotConnected;

   MessageRetractionHandle sendDirectedInteraction(
      InteractionClassHandle interactionClass,
      ObjectInstanceHandle objectInstance,
      ParameterHandleValueMap parameterValues,
      byte[] userSuppliedTag,
      LogicalTime time) throws NotConnected;

   void sendInteractionWithRegions(
      InteractionClassHandle interactionClass,
      ParameterHandleValueMap parameterValues,
      RegionHandleSet regions,
      byte[] userSuppliedTag) throws NotConnected;

   RegionHandle createRegion(DimensionHandleSet dimensions) throws NotConnected;

   void commitRegionModifications(RegionHandleSet regions) throws NotConnected;

   void deleteRegion(RegionHandle region) throws NotConnected;

   DimensionHandleSet getDimensionHandleSet(RegionHandle region) throws NotConnected;

   RangeBounds getRangeBounds(RegionHandle region, DimensionHandle dimension) throws NotConnected;

   void setRangeBounds(RegionHandle region, DimensionHandle dimension, RangeBounds rangeBounds)
      throws NotConnected;

   boolean getConveyRegionDesignatorSetsSwitch() throws NotConnected;

   void setConveyRegionDesignatorSetsSwitch(boolean switchValue) throws NotConnected;

   boolean getObjectClassRelevanceAdvisorySwitch() throws NotConnected;

   void setObjectClassRelevanceAdvisorySwitch(boolean switchValue) throws NotConnected;

   boolean getAttributeRelevanceAdvisorySwitch() throws NotConnected;

   void setAttributeRelevanceAdvisorySwitch(boolean switchValue) throws NotConnected;

   boolean getAttributeScopeAdvisorySwitch() throws NotConnected;

   void setAttributeScopeAdvisorySwitch(boolean switchValue) throws NotConnected;

   boolean getInteractionRelevanceAdvisorySwitch() throws NotConnected;

   void setInteractionRelevanceAdvisorySwitch(boolean switchValue) throws NotConnected;

   ResignAction getAutomaticResignDirective() throws NotConnected;

   void setAutomaticResignDirective(ResignAction resignAction) throws NotConnected;

   boolean getServiceReportingSwitch() throws NotConnected;

   void setServiceReportingSwitch(boolean switchValue) throws NotConnected;

   boolean getExceptionReportingSwitch() throws NotConnected;

   void setExceptionReportingSwitch(boolean switchValue) throws NotConnected;

   boolean getSendServiceReportsToFileSwitch() throws NotConnected;

   void setSendServiceReportsToFileSwitch(boolean enabled) throws NotConnected;

   String getHLAversion();

   boolean getAutoProvideSwitch() throws NotConnected;

   boolean getDelaySubscriptionEvaluationSwitch() throws NotConnected;

   boolean getAdvisoriesUseKnownClassSwitch() throws NotConnected;

   boolean getAllowRelaxedDDMSwitch() throws NotConnected;

   boolean getNonRegulatedGrantSwitch() throws NotConnected;

   LogicalTimeFactory getTimeFactory() throws NotConnected;

   void enableTimeRegulation(LogicalTimeInterval lookahead) throws RTIexception;

   void disableTimeRegulation() throws NotConnected;

   void enableTimeConstrained() throws NotConnected;

   void disableTimeConstrained() throws NotConnected;

   void enableAsynchronousDelivery() throws NotConnected;

   void disableAsynchronousDelivery() throws NotConnected;

   void modifyLookahead(LogicalTimeInterval lookahead) throws NotConnected;

   LogicalTimeInterval queryLookahead() throws NotConnected;

   void timeAdvanceRequest(LogicalTime time) throws RTIexception;

   void timeAdvanceRequestAvailable(LogicalTime time) throws NotConnected;

   void nextMessageRequest(LogicalTime time) throws NotConnected;

   void nextMessageRequestAvailable(LogicalTime time) throws NotConnected;

   void flushQueueRequest(LogicalTime time) throws NotConnected;

   LogicalTime queryLogicalTime() throws NotConnected;

   TimeQueryReturn queryGALT() throws NotConnected;

   TimeQueryReturn queryLITS() throws NotConnected;

   void createFederationExecution(
      String federationName,
      String fomModule,
      String logicalTimeImplementationName) throws NotConnected;

   void createFederationExecution(
      String federationName,
      String[] fomModules,
      String logicalTimeImplementationName) throws NotConnected;

   void createFederationExecutionWithMIM(
      String federationName,
      String[] fomModules,
      String mimModule,
      String logicalTimeImplementationName) throws NotConnected;

   void destroyFederationExecution(String federationName) throws NotConnected;
}
