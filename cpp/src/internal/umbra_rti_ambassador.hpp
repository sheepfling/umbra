#pragma once

#include "generated/rti_ambassador_shell.hpp"
#include "internal/callback_dispatcher.hpp"
#include "internal/callback_session.hpp"
#include "internal/federate_lifecycle.hpp"
#include "internal/federate_time_state.hpp"

#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace rti1516_2025::umbra_binding_detail {

// Private adapter for the embedded connection slice. It deliberately exposes
// no Umbra-specific public surface: callers use only RTIambassador.
class UmbraRtiAmbassador final : public RtiAmbassadorShell {
 public:
  ConfigurationResult connect(
      FederateAmbassador& federateAmbassador,
      CallbackModel callbackModel) override;

  ConfigurationResult connect(
      FederateAmbassador& federateAmbassador,
      CallbackModel callbackModel,
      RtiConfiguration const& configuration) override;

  ConfigurationResult connect(
      FederateAmbassador& federateAmbassador,
      CallbackModel callbackModel,
      Credentials const& credentials) override;

  ConfigurationResult connect(
      FederateAmbassador& federateAmbassador,
      CallbackModel callbackModel,
      RtiConfiguration const& configuration,
      Credentials const& credentials) override;

  void disconnect() override;

  bool evokeCallback(double approximateMinimumTimeInSeconds) override;

  bool evokeMultipleCallbacks(
      double approximateMinimumTimeInSeconds,
      double approximateMaximumTimeInSeconds) override;

  void enableCallbacks() override;
  void disableCallbacks() override;

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  void createFederationExecution(
      std::wstring const& federationName,
      std::wstring const& fomModule,
      std::wstring const& logicalTimeImplementationName = L"") override;

  void createFederationExecution(
      std::wstring const& federationName,
      std::vector<std::wstring> const& fomModules,
      std::wstring const& logicalTimeImplementationName = L"") override;

  void createFederationExecutionWithMIM(
      std::wstring const& federationName,
      std::vector<std::wstring> const& fomModules,
      std::wstring const& mimModule,
      std::wstring const& logicalTimeImplementationName = L"") override;

  void destroyFederationExecution(std::wstring const& federationName) override;

  void listFederationExecutions() override;

  void listFederationExecutionMembers(std::wstring const& federationName) override;

  FederateHandle joinFederationExecution(
      std::wstring const& federateType,
      std::wstring const& federationName,
      std::vector<std::wstring> const& additionalFomModules = std::vector<std::wstring>()) override;

  FederateHandle joinFederationExecution(
      std::wstring const& federateName,
      std::wstring const& federateType,
      std::wstring const& federationName,
      std::vector<std::wstring> const& additionalFomModules = std::vector<std::wstring>()) override;

  void resignFederationExecution(ResignAction resignAction) override;

  std::unique_ptr<LogicalTimeFactory> getTimeFactory() const override;

  FederateHandle getFederateHandle(std::wstring const& federateName) override;

  std::wstring getFederateName(FederateHandle const& federate) override;

  ObjectClassHandle getObjectClassHandle(std::wstring const& objectClassName) override;

  std::wstring getObjectClassName(ObjectClassHandle const& objectClass) override;

  AttributeHandle getAttributeHandle(
      ObjectClassHandle const& objectClass,
      std::wstring const& attributeName) override;

  std::wstring getAttributeName(
      ObjectClassHandle const& objectClass,
      AttributeHandle const& attribute) override;

  void publishObjectClassAttributes(
      ObjectClassHandle const& objectClass,
      AttributeHandleSet const& attributes) override;

  void unpublishObjectClassAttributes(
      ObjectClassHandle const& objectClass,
      AttributeHandleSet const& attributes) override;

  void subscribeObjectClassAttributes(
      ObjectClassHandle const& objectClass,
      AttributeHandleSet const& attributes,
      bool active = true,
      std::wstring const& updateRateDesignator = L"") override;

  void unsubscribeObjectClassAttributes(
      ObjectClassHandle const& objectClass,
      AttributeHandleSet const& attributes) override;

  void reserveObjectInstanceName(
      std::wstring const& objectInstanceName) override;

  void releaseObjectInstanceName(
      std::wstring const& objectInstanceName) override;

  void reserveMultipleObjectInstanceNames(
      std::set<std::wstring> const& objectInstanceNames) override;

  void releaseMultipleObjectInstanceNames(
      std::set<std::wstring> const& objectInstanceNames) override;

  ObjectInstanceHandle registerObjectInstanceWithRegions(
      ObjectClassHandle const& objectClass,
      AttributeHandleSetRegionHandleSetPairVector const& attributesAndRegions) override;

  void associateRegionsForUpdates(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSetRegionHandleSetPairVector const& attributesAndRegions) override;

  void unassociateRegionsForUpdates(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSetRegionHandleSetPairVector const& attributesAndRegions) override;

  void subscribeObjectClassAttributesWithRegions(
      ObjectClassHandle const& objectClass,
      AttributeHandleSetRegionHandleSetPairVector const& attributesAndRegions,
      bool active = true,
      std::wstring const& updateRateDesignator = L"") override;

  void unsubscribeObjectClassAttributesWithRegions(
      ObjectClassHandle const& objectClass,
      AttributeHandleSetRegionHandleSetPairVector const& attributesAndRegions) override;

  ObjectInstanceHandle registerObjectInstance(
      ObjectClassHandle const& objectClass) override;

  ObjectInstanceHandle registerObjectInstance(
      ObjectClassHandle const& objectClass,
      std::wstring const& objectInstanceName) override;

  ObjectInstanceHandle registerObjectInstanceWithRegions(
      ObjectClassHandle const& objectClass,
      AttributeHandleSetRegionHandleSetPairVector const& attributesAndRegions,
      std::wstring const& objectInstanceName) override;

  void deleteObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      VariableLengthData const& userSuppliedTag) override;

  MessageRetractionHandle deleteObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      VariableLengthData const& userSuppliedTag,
      LogicalTime const& time) override;

  void localDeleteObjectInstance(
      ObjectInstanceHandle const& objectInstance) override;

  void updateAttributeValues(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleValueMap const& attributeValues,
      VariableLengthData const& userSuppliedTag) override;

  MessageRetractionHandle updateAttributeValues(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleValueMap const& attributeValues,
      VariableLengthData const& userSuppliedTag,
      LogicalTime const& time) override;

  void requestAttributeValueUpdate(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes,
      VariableLengthData const& userSuppliedTag) override;

  void requestAttributeValueUpdate(
      ObjectClassHandle const& objectClass,
      AttributeHandleSet const& attributes,
      VariableLengthData const& userSuppliedTag) override;

  void requestAttributeValueUpdateWithRegions(
      ObjectClassHandle const& objectClass,
      AttributeHandleSetRegionHandleSetPairVector const& attributesAndRegions,
      VariableLengthData const& userSuppliedTag) override;

  void queryAttributeOwnership(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes) override;

  bool isAttributeOwnedByFederate(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandle const& attribute) override;

  void negotiatedAttributeOwnershipDivestiture(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes,
      VariableLengthData const& userSuppliedTag) override;

  void confirmDivestiture(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& confirmedAttributes,
      VariableLengthData const& userSuppliedTag) override;

  void cancelNegotiatedAttributeOwnershipDivestiture(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes) override;

  void unconditionalAttributeOwnershipDivestiture(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes,
      VariableLengthData const& userSuppliedTag) override;

  void attributeOwnershipAcquisition(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& desiredAttributes,
      VariableLengthData const& userSuppliedTag) override;

  void attributeOwnershipAcquisitionIfAvailable(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& desiredAttributes,
      VariableLengthData const& userSuppliedTag) override;

  void attributeOwnershipReleaseDenied(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes,
      VariableLengthData const& userSuppliedTag) override;

  void attributeOwnershipDivestitureIfWanted(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes,
      VariableLengthData const& userSuppliedTag,
      AttributeHandleSet& divestedAttributes) override;

  void cancelAttributeOwnershipAcquisition(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes) override;

  ObjectClassHandle getKnownObjectClassHandle(
      ObjectInstanceHandle const& objectInstance) override;

  ObjectInstanceHandle getObjectInstanceHandle(
      std::wstring const& objectInstanceName) override;

  std::wstring getObjectInstanceName(
      ObjectInstanceHandle const& objectInstance) override;

  InteractionClassHandle getInteractionClassHandle(
      std::wstring const& interactionClassName) override;

  std::wstring getInteractionClassName(
      InteractionClassHandle const& interactionClass) override;

  ParameterHandle getParameterHandle(
      InteractionClassHandle const& interactionClass,
      std::wstring const& parameterName) override;

  std::wstring getParameterName(
      InteractionClassHandle const& interactionClass,
      ParameterHandle const& parameter) override;

  void publishInteractionClass(InteractionClassHandle const& interactionClass) override;

  void unpublishInteractionClass(InteractionClassHandle const& interactionClass) override;

  void publishObjectClassDirectedInteractions(
      ObjectClassHandle const& objectClass,
      InteractionClassHandleSet const& interactionClasses) override;

  void unpublishObjectClassDirectedInteractions(
      ObjectClassHandle const& objectClass) override;

  void unpublishObjectClassDirectedInteractions(
      ObjectClassHandle const& objectClass,
      InteractionClassHandleSet const& interactionClasses) override;

  void subscribeInteractionClass(
      InteractionClassHandle const& interactionClass,
      bool active = true) override;

  void unsubscribeInteractionClass(InteractionClassHandle const& interactionClass) override;

  void subscribeObjectClassDirectedInteractions(
      ObjectClassHandle const& objectClass,
      InteractionClassHandleSet const& interactionClasses,
      bool universally = false) override;

  void unsubscribeObjectClassDirectedInteractions(
      ObjectClassHandle const& objectClass) override;

  void unsubscribeObjectClassDirectedInteractions(
      ObjectClassHandle const& objectClass,
      InteractionClassHandleSet const& interactionClasses) override;

  void subscribeInteractionClassWithRegions(
      InteractionClassHandle const& interactionClass,
      RegionHandleSet const& regions,
      bool active = true) override;

  void unsubscribeInteractionClassWithRegions(
      InteractionClassHandle const& interactionClass,
      RegionHandleSet const& regions) override;

  void sendInteraction(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag) override;

  MessageRetractionHandle sendInteraction(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag,
      LogicalTime const& time) override;

  void sendDirectedInteraction(
      InteractionClassHandle const& interactionClass,
      ObjectInstanceHandle const& objectInstance,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag) override;

  void sendInteractionWithRegions(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const& parameterValues,
      RegionHandleSet const& regions,
      VariableLengthData const& userSuppliedTag) override;

  TransportationTypeHandle getTransportationTypeHandle(
      std::wstring const& transportationTypeName) override;

  std::wstring getTransportationTypeName(
      TransportationTypeHandle const& transportationType) override;

  DimensionHandleSet getAvailableDimensionsForObjectClass(
      ObjectClassHandle const& objectClass) override;

  DimensionHandleSet getAvailableDimensionsForInteractionClass(
      InteractionClassHandle const& interactionClass) override;

  DimensionHandle getDimensionHandle(
      std::wstring const& dimensionName) override;

  std::wstring getDimensionName(
      DimensionHandle const& dimension) override;

  unsigned long getDimensionUpperBound(
      DimensionHandle const& dimension) override;

  RegionHandle createRegion(
      DimensionHandleSet const& dimensions) override;

  void commitRegionModifications(
      RegionHandleSet const& regions) override;

  void deleteRegion(
      RegionHandle const& region) override;

  DimensionHandleSet getDimensionHandleSet(
      RegionHandle const& region) override;

  RangeBounds getRangeBounds(
      RegionHandle const& region,
      DimensionHandle const& dimension) override;

  void setRangeBounds(
      RegionHandle const& region,
      DimensionHandle const& dimension,
      RangeBounds const& rangeBounds) override;

  bool getAttributeScopeAdvisorySwitch() const override;

  void setAttributeScopeAdvisorySwitch(bool switchValue) override;

  void enableTimeRegulation(LogicalTimeInterval const& lookahead) override;

  void disableTimeRegulation() override;

  void enableTimeConstrained() override;

  void disableTimeConstrained() override;

  void timeAdvanceRequest(LogicalTime const& time) override;

  bool queryGALT(LogicalTime& time) override;

  void queryLogicalTime(LogicalTime& time) override;

  bool queryLITS(LogicalTime& time) override;

  void queryLookahead(LogicalTimeInterval& interval) override;

  void retract(MessageRetractionHandle const& retraction) override;

  RegionHandle decodeRegionHandle(
      VariableLengthData const& encodedValue) const override;

  MessageRetractionHandle decodeMessageRetractionHandle(
      VariableLengthData const& encodedValue) const override;
#endif

 private:
  ConfigurationResult connectImpl(
      FederateAmbassador& federateAmbassador,
      CallbackModel callbackModel,
      RtiConfiguration const* configuration);

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  FederateHandle joinFederationExecutionImpl(
      std::optional<std::wstring> requestedFederateName,
      std::wstring const& federateType,
      std::wstring const& federationName,
      std::vector<std::wstring> const& additionalFomModules);
#endif

  mutable std::mutex mutex_;
  umbra::detail::FederateLifecycle lifecycle_;
  std::shared_ptr<umbra::detail::CallbackDispatcher> callbacks_ =
      std::make_shared<umbra::detail::CallbackDispatcher>();
  std::shared_ptr<CallbackSession> callbackSession_;
  CallbackModel callbackModel_ = HLA_EVOKED;
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  std::optional<std::wstring> joinedFederationName_;
  std::optional<std::uint64_t> joinedFederateId_;
  std::shared_ptr<umbra::detail::FederateTimeState> federateTimeState_;
#endif
};

}  // namespace rti1516_2025::umbra_binding_detail
