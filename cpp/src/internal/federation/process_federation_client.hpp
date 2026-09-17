#pragma once

#include "internal/federation/process_federation_callback_bridge.hpp"
#include "internal/federation/process_transport_session.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace rti1516_2025 {
class FederateAmbassador;
}

namespace umbra::detail {

// Private client-side seam for the first process-boundary federation slice.
// It owns request identities, response validation, unsolicited event
// buffering, and the official callback bridge.  The public RTI ambassador
// selects this seam only for the explicitly configured process endpoint and
// the currently implemented lifecycle slice; keeping the wire payloads
// private prevents them from becoming an accidental standards API.
class ProcessFederationClientError final : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

class ProcessFederationClient final {
 public:
  using CallbackCompletionHandler =
      ProcessFederationCallbackBridge::CallbackCompletionHandler;
  using FailureHandler = ProcessTransportConnection::FailureHandler;
  using ForcedResignationHandler =
      ProcessTransportConnection::ForcedResignationHandler;

  ProcessFederationClient(
      ProcessTransportAddress address,
      TransportEndpointIdentity localIdentity,
      std::shared_ptr<RuntimeInstrumentation> instrumentation = {},
      FailureHandler failureHandler = {},
      ForcedResignationHandler forcedResignationHandler = {});

  ProcessFederationClient(ProcessFederationClient const&) = delete;
  ProcessFederationClient& operator=(ProcessFederationClient const&) = delete;

  ~ProcessFederationClient();

  void createFederationExecution(
      std::wstring federationName,
      std::vector<std::wstring> fomModules = {},
      std::optional<std::wstring> mimModule = std::nullopt,
      std::wstring logicalTimeImplementationName = {});

  [[nodiscard]] ProcessFederationJoinResult joinFederationExecution(
      std::wstring federationName,
      std::wstring federateType,
      std::optional<std::wstring> requestedFederateName = std::nullopt,
      std::vector<std::wstring> additionalFomModules = {});

  void resignFederationExecution(
      std::wstring federationName,
      std::uint64_t federateId,
      rti1516_2025::ResignAction resignAction);

  [[nodiscard]] rti1516_2025::ResignAction getAutomaticResignDirective(
      std::wstring federationName,
      std::uint64_t federateId);

  void setAutomaticResignDirective(
      std::wstring federationName,
      std::uint64_t federateId,
      rti1516_2025::ResignAction resignAction);

  // Read-only temporal baseline for the process endpoint.  Grant and role
  // control are deliberately separate operations; this method only returns
  // the current logical-time representation owned by the endpoint.
  [[nodiscard]] ProcessFederationLogicalTime queryLogicalTime(
      std::wstring federationName,
      std::uint64_t federateId);

  [[nodiscard]] ProcessFederationQueryLookaheadResult queryLookahead(
      std::wstring federationName,
      std::uint64_t federateId);

  [[nodiscard]] ProcessFederationModifyLookaheadResult modifyLookahead(
      std::wstring federationName,
      std::uint64_t federateId,
      ProcessFederationLogicalTimeInterval lookahead);

  // Query GALT and Query LITS from one endpoint-owned temporal snapshot.  The
  // result keeps the two optional bounds independent, matching the official
  // undefined-GALT/defined-LITS distinction.
  [[nodiscard]] ProcessFederationQueryTimeBoundsResult queryTimeBounds(
      std::wstring federationName,
      std::uint64_t federateId);

  [[nodiscard]] ProcessFederationEnableTimeRegulationResult
  enableTimeRegulation(
      std::wstring federationName,
      std::uint64_t federateId,
      ProcessFederationLogicalTimeInterval lookahead);

  [[nodiscard]] ProcessFederationEnableTimeConstrainedResult
  enableTimeConstrained(
      std::wstring federationName,
      std::uint64_t federateId);

  [[nodiscard]] ProcessFederationTimeDisableResult disableTimeRegulation(
      std::wstring federationName,
      std::uint64_t federateId);

  [[nodiscard]] ProcessFederationTimeDisableResult disableTimeConstrained(
      std::wstring federationName,
      std::uint64_t federateId);

  [[nodiscard]] ProcessFederationTimeAdvanceResult timeAdvanceRequest(
      std::wstring federationName,
      std::uint64_t federateId,
      ProcessFederationLogicalTime requestedTime);

  [[nodiscard]] ProcessFederationTimeAdvanceResult
  timeAdvanceRequestAvailable(
      std::wstring federationName,
      std::uint64_t federateId,
      ProcessFederationLogicalTime requestedTime);

  [[nodiscard]] ProcessFederationTimeAdvanceResult nextMessageRequest(
      std::wstring federationName,
      std::uint64_t federateId,
      ProcessFederationLogicalTime requestedTime);

  [[nodiscard]] ProcessFederationTimeAdvanceResult
  nextMessageRequestAvailable(
      std::wstring federationName,
      std::uint64_t federateId,
      ProcessFederationLogicalTime requestedTime);

  [[nodiscard]] ProcessFederationTimeAdvanceResult flushQueueRequest(
      std::wstring federationName,
      std::uint64_t federateId,
      ProcessFederationLogicalTime requestedTime);

  [[nodiscard]] std::optional<std::uint64_t>
  lookupInteractionClassHandle(
      std::wstring federationName,
      std::uint64_t federateId,
      std::wstring interactionClassName);

  [[nodiscard]] std::optional<std::uint64_t> lookupFederateHandle(
      std::wstring federationName,
      std::uint64_t federateId,
      std::wstring federateName);

  [[nodiscard]] std::optional<std::wstring> lookupFederateName(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t targetFederateId);

  [[nodiscard]] std::optional<std::uint64_t> normalizeFederateHandle(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t handle);

  [[nodiscard]] std::optional<std::uint64_t> normalizeObjectClassHandle(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t handle);

  [[nodiscard]] std::optional<std::uint64_t> normalizeInteractionClassHandle(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t handle);

  [[nodiscard]] std::optional<std::uint64_t> normalizeObjectInstanceHandle(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t handle);

  [[nodiscard]] std::optional<std::uint64_t>
  lookupObjectClassHandle(
      std::wstring federationName,
      std::uint64_t federateId,
      std::wstring objectClassName);

  [[nodiscard]] std::optional<std::uint64_t> lookupParameterHandle(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t interactionClassHandle,
      std::wstring parameterName);

  [[nodiscard]] std::optional<std::uint64_t> lookupAttributeHandle(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle,
      std::wstring attributeName);

  [[nodiscard]] std::optional<std::uint64_t> lookupObjectInstanceHandle(
      std::wstring federationName,
      std::uint64_t federateId,
      std::wstring objectInstanceName);

  [[nodiscard]] std::optional<std::wstring> lookupObjectInstanceName(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t objectInstanceHandle);

  [[nodiscard]] std::optional<std::uint64_t>
  lookupKnownObjectClassHandle(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t objectInstanceHandle);

  [[nodiscard]] ProcessFederationAttributeOwnershipCheckResult
  attributeOwnershipCheck(
      std::wstring federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t attributeHandle);

  [[nodiscard]] ProcessFederationAttributeOwnershipQueryResult
  queryAttributeOwnership(
      std::wstring federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::vector<std::uint64_t> requestedAttributeHandles);

  [[nodiscard]]
  ProcessFederationAttributeOwnershipAcquisitionIfAvailableResult
  attributeOwnershipAcquisitionIfAvailable(
      std::wstring federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::vector<std::uint64_t> desiredAttributeHandles,
      std::vector<std::uint8_t> userSuppliedTag);

  [[nodiscard]] ProcessFederationAttributeOwnershipAcquisitionResult
  attributeOwnershipAcquisition(
      std::wstring federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::vector<std::uint64_t> desiredAttributeHandles,
      std::vector<std::uint8_t> userSuppliedTag);

  [[nodiscard]] ProcessFederationBooleanResult
  unconditionalAttributeOwnershipDivestiture(
      std::wstring federationName,
      std::uint64_t divestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::vector<std::uint64_t> attributeHandles,
      std::vector<std::uint8_t> userSuppliedTag);

  [[nodiscard]] ProcessFederationAttributeOwnershipReleaseDeniedResult
  attributeOwnershipReleaseDenied(
      std::wstring federationName,
      std::uint64_t owningFederateId,
      std::uint64_t objectInstanceHandle,
      std::vector<std::uint64_t> attributeHandles,
      std::vector<std::uint8_t> userSuppliedTag);

  [[nodiscard]]
  ProcessFederationAttributeOwnershipAcquisitionCancellationResult
  cancelAttributeOwnershipAcquisition(
      std::wstring federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::vector<std::uint64_t> attributeHandles);

  [[nodiscard]]
  ProcessFederationCancelNegotiatedAttributeOwnershipDivestitureResult
  cancelNegotiatedAttributeOwnershipDivestiture(
      std::wstring federationName,
      std::uint64_t divestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::vector<std::uint64_t> attributeHandles);

  [[nodiscard]]
  ProcessFederationNegotiatedAttributeOwnershipDivestitureResult
  negotiatedAttributeOwnershipDivestiture(
      std::wstring federationName,
      std::uint64_t divestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::vector<std::uint64_t> attributeHandles,
      std::vector<std::uint8_t> userSuppliedTag);

  [[nodiscard]] ProcessFederationConfirmDivestitureResult
  confirmDivestiture(
      std::wstring federationName,
      std::uint64_t divestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::vector<std::uint64_t> attributeHandles,
      std::vector<std::uint8_t> userSuppliedTag);

  [[nodiscard]] std::optional<std::wstring> lookupObjectClassName(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle);

  [[nodiscard]] std::optional<std::wstring> lookupInteractionClassName(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t interactionClassHandle);

  [[nodiscard]] std::optional<std::wstring> lookupAttributeName(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle,
      std::uint64_t attributeHandle);

  [[nodiscard]] std::optional<std::wstring> lookupParameterName(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t interactionClassHandle,
      std::uint64_t parameterHandle);

  void publishInteractionClass(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t interactionClassHandle);
  void unpublishInteractionClass(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t interactionClassHandle);
  [[nodiscard]] ProcessFederationInteractionOrderTypeChangeResult
  changeInteractionOrderType(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t interactionClassHandle,
      rti1516_2025::OrderType orderType);
  [[nodiscard]] ProcessFederationAttributeOrderTypeChangeResult
  changeAttributeOrderType(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t objectInstanceHandle,
      std::set<std::uint64_t> attributeHandles,
      rti1516_2025::OrderType orderType);
  [[nodiscard]] ProcessFederationAttributeOrderTypeDefaultResult
  changeDefaultAttributeOrderType(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle,
      std::set<std::uint64_t> attributeHandles,
      rti1516_2025::OrderType orderType);
  [[nodiscard]] ProcessFederationAttributeTransportationTypeDefaultResult
  changeDefaultAttributeTransportationType(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle,
      std::set<std::uint64_t> attributeHandles,
      std::uint64_t transportationTypeHandle);
  [[nodiscard]] ProcessFederationAttributeTransportationTypeChangeResult
  requestAttributeTransportationTypeChange(
      std::wstring federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::set<std::uint64_t> attributeHandles,
      std::uint64_t transportationTypeHandle);
  [[nodiscard]] ProcessFederationAttributeTransportationTypeQueryResult
  queryAttributeTransportationType(
      std::wstring federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t attributeHandle);
  void subscribeInteractionClass(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t interactionClassHandle,
      bool active);
  void unsubscribeInteractionClass(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t interactionClassHandle);
  void subscribeInteractionClassWithRegions(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t interactionClassHandle,
      std::set<std::uint64_t> regionHandles,
      bool active);
  void unsubscribeInteractionClassWithRegions(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t interactionClassHandle,
      std::set<std::uint64_t> regionHandles);

  void publishObjectClassDirectedInteractions(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle,
      std::vector<std::uint64_t> interactionClassHandles);
  void unpublishObjectClassDirectedInteractions(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle,
      std::optional<std::vector<std::uint64_t>> interactionClassHandles);
  void subscribeObjectClassDirectedInteractions(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle,
      std::vector<std::uint64_t> interactionClassHandles,
      bool universally);
  void unsubscribeObjectClassDirectedInteractions(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle,
      std::optional<std::vector<std::uint64_t>> interactionClassHandles);

  void publishObjectClassAttributes(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle,
      std::vector<std::uint64_t> attributeHandles);
  void subscribeObjectClassAttributes(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle,
      std::vector<std::uint64_t> attributeHandles,
      bool active,
      std::string updateRateDesignator);
  void unsubscribeObjectClassAttributes(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle,
      std::vector<std::uint64_t> attributeHandles);
  void subscribeObjectClassAttributesWithRegions(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle,
      std::map<std::uint64_t, std::set<std::uint64_t>> attributesAndRegions,
      bool active,
      std::string updateRateDesignator);
  void unsubscribeObjectClassAttributesWithRegions(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle,
      std::map<std::uint64_t, std::set<std::uint64_t>> attributesAndRegions);

  [[nodiscard]] ProcessFederationRegisterObjectInstanceResult
  registerObjectInstance(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle,
      std::optional<std::wstring> requestedObjectInstanceName = std::nullopt);

  [[nodiscard]] ProcessFederationLocalDeleteObjectInstanceResult
  localDeleteObjectInstance(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t objectInstanceHandle);

  [[nodiscard]] ProcessFederationDeleteObjectInstanceResult
  deleteObjectInstance(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t objectInstanceHandle,
      std::vector<std::uint8_t> userSuppliedTag,
      std::optional<ProcessFederationLogicalTime> timestamp = std::nullopt);

  [[nodiscard]] ProcessFederationReserveObjectInstanceNameResult
  reserveObjectInstanceName(
      std::wstring federationName,
      std::uint64_t federateId,
      std::wstring objectInstanceName);

  [[nodiscard]] std::optional<std::uint64_t> lookupDimensionHandle(
      std::wstring federationName,
      std::uint64_t federateId,
      std::string dimensionName);
  [[nodiscard]] std::optional<std::wstring> lookupDimensionName(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t dimensionHandle);
  [[nodiscard]] std::optional<std::uint64_t> lookupTransportationTypeHandle(
      std::wstring federationName,
      std::uint64_t federateId,
      std::string transportationTypeName);
  [[nodiscard]] std::optional<std::wstring> lookupTransportationTypeName(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t transportationTypeHandle);
  [[nodiscard]] ProcessFederationDimensionUpperBoundResult
  lookupDimensionUpperBound(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t dimensionHandle);
  [[nodiscard]] ProcessFederationAvailableDimensionsResult
  availableDimensionsForObjectClass(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle);
  [[nodiscard]] ProcessFederationAvailableDimensionsResult
  availableDimensionsForInteractionClass(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t interactionClassHandle);
  [[nodiscard]] ProcessFederationCreateRegionResult createRegion(
      std::wstring federationName,
      std::uint64_t federateId,
      std::vector<std::uint64_t> dimensionHandles);
  [[nodiscard]] ProcessFederationRegionStatusResult commitRegionModifications(
      std::wstring federationName,
      std::uint64_t federateId,
      std::vector<std::uint64_t> regionHandles);
  [[nodiscard]] ProcessFederationRegionStatusResult deleteRegion(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t regionHandle);
  [[nodiscard]] ProcessFederationDimensionSetResult dimensionHandleSetForRegion(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t regionHandle);
  [[nodiscard]] ProcessFederationRangeBoundsResult rangeBoundsForRegion(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t regionHandle,
      std::uint64_t dimensionHandle);
  [[nodiscard]] ProcessFederationRegionStatusResult setRangeBounds(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t regionHandle,
      std::uint64_t dimensionHandle,
      unsigned long lowerBound,
      unsigned long upperBound);

  [[nodiscard]] bool getAttributeScopeAdvisorySwitch(
      std::wstring federationName,
      std::uint64_t federateId);
  void setAttributeScopeAdvisorySwitch(
      std::wstring federationName,
      std::uint64_t federateId,
      bool switchValue);
  [[nodiscard]] bool getAttributeRelevanceAdvisorySwitch(
      std::wstring federationName,
      std::uint64_t federateId);
  void setAttributeRelevanceAdvisorySwitch(
      std::wstring federationName,
      std::uint64_t federateId,
      bool switchValue);
  [[nodiscard]] bool getConveyRegionDesignatorSetsSwitch(
      std::wstring federationName,
      std::uint64_t federateId);
  void setConveyRegionDesignatorSetsSwitch(
      std::wstring federationName,
      std::uint64_t federateId,
      bool switchValue);
  [[nodiscard]] ProcessFederationRegisterObjectInstanceResult
  registerObjectInstanceWithRegions(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle,
      std::map<std::uint64_t, std::set<std::uint64_t>> updateRegionsByAttribute,
      std::optional<std::wstring> requestedObjectInstanceName = std::nullopt);

  [[nodiscard]] ProcessFederationObjectInstanceRegionAssociationResult
  associateRegionsForUpdates(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t objectInstanceHandle,
      std::map<std::uint64_t, std::set<std::uint64_t>> attributesAndRegions);
  [[nodiscard]] ProcessFederationObjectInstanceRegionAssociationResult
  unassociateRegionsForUpdates(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t objectInstanceHandle,
      std::map<std::uint64_t, std::set<std::uint64_t>> attributesAndRegions);

  [[nodiscard]] ProcessFederationSendInteractionResult sendInteraction(
      std::wstring federationName,
      std::uint64_t producingFederateId,
      std::uint64_t interactionClassHandle,
      std::vector<std::uint64_t> sentParameterHandles,
      std::vector<std::uint8_t> payload,
      std::optional<ProcessFederationLogicalTime> timestamp = std::nullopt);

  [[nodiscard]] ProcessFederationSendInteractionResult sendInteractionWithRegions(
      std::wstring federationName,
      std::uint64_t producingFederateId,
      std::uint64_t interactionClassHandle,
      std::vector<std::uint64_t> sentParameterHandles,
      std::set<std::uint64_t> sentRegionHandles,
      std::vector<std::uint8_t> payload,
      std::optional<ProcessFederationLogicalTime> timestamp = std::nullopt);

  [[nodiscard]] ProcessFederationSendInteractionResult sendDirectedInteraction(
      std::wstring federationName,
      std::uint64_t producingFederateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t interactionClassHandle,
      std::vector<std::uint64_t> sentParameterHandles,
      std::vector<std::uint8_t> payload,
      std::optional<ProcessFederationLogicalTime> timestamp = std::nullopt);

  [[nodiscard]] ProcessFederationRetractResult retract(
      std::wstring federationName,
      std::uint64_t producingFederateId,
      std::uint64_t messageId);

  [[nodiscard]] ProcessFederationUpdateAttributeValuesResult
  updateAttributeValues(
      std::wstring federationName,
      std::uint64_t producingFederateId,
      std::uint64_t objectInstanceHandle,
      std::vector<ProcessFederationAttributeValue> attributeValues,
      std::vector<std::uint8_t> userSuppliedTag,
      std::optional<ProcessFederationLogicalTime> timestamp = std::nullopt);

  [[nodiscard]] ProcessFederationRequestAttributeValueUpdateResult
  requestAttributeValueUpdate(
      std::wstring federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::vector<std::uint64_t> requestedAttributeHandles,
      std::vector<std::uint8_t> userSuppliedTag);

  [[nodiscard]] ProcessFederationRequestAttributeValueUpdateResult
  requestAttributeValueUpdateClass(
      std::wstring federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectClassHandle,
      std::vector<std::uint64_t> requestedAttributeHandles,
      std::vector<std::uint8_t> userSuppliedTag);

  [[nodiscard]] ProcessFederationRequestAttributeValueUpdateResult
  requestAttributeValueUpdateClassWithRegions(
      std::wstring federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectClassHandle,
      std::vector<std::uint64_t> requestedAttributeHandles,
      std::map<std::uint64_t, std::set<std::uint64_t>> requestRegionsByAttribute,
      std::vector<std::uint8_t> userSuppliedTag);

  // Poll the private service queue.  A pushed event received while another
  // request is in flight is retained and returned before a new socket read.
  [[nodiscard]] std::optional<ProcessFederationInteractionEvent>
  receiveInteraction(
      std::wstring federationName,
      std::uint64_t receivingFederateId);

  [[nodiscard]] std::optional<ProcessFederationAttributeUpdateEvent>
  receiveAttributeUpdate(
      std::wstring federationName,
      std::uint64_t receivingFederateId);

  [[nodiscard]] std::optional<ProcessFederationObjectInstanceDiscoveryEvent>
  receiveObjectInstanceDiscovery(
      std::wstring federationName,
      std::uint64_t receivingFederateId);

  // Consume one pushed receive-order event and queue it for the official
  // FederateAmbassador callback bridge.  The caller can then use evokeOne or
  // evokeMultiple for HLA_EVOKED dispatch.
  void dispatchReceiveOrder(ProcessFederationInteractionEvent event);
  void dispatchPushedReceiveOrder();
  void dispatchAttributeUpdate(ProcessFederationAttributeUpdateEvent event);
  void dispatchPushedAttributeUpdate();
  void dispatchAttributeValueUpdateRequest(
      ProcessFederationAttributeValueUpdateRequestEvent event);
  void dispatchPushedAttributeValueUpdateRequest();
  void dispatchAttributeOwnershipQuery(
      ProcessFederationAttributeOwnershipQueryEvent event);
  void dispatchPushedAttributeOwnershipQuery();
  void dispatchAttributeOwnershipAcquisitionIfAvailable(
      ProcessFederationAttributeOwnershipAcquisitionIfAvailableEvent event);
  void dispatchPushedAttributeOwnershipAcquisitionIfAvailable();
  void dispatchAttributeOwnershipAcquisition(
      ProcessFederationAttributeOwnershipAcquisitionEvent event);
  void dispatchPushedAttributeOwnershipAcquisition();
  void dispatchAttributeOwnershipUnavailable(
      ProcessFederationAttributeOwnershipUnavailableEvent event);
  void dispatchPushedAttributeOwnershipUnavailable();
  void dispatchObjectInstanceDiscovery(
      ProcessFederationObjectInstanceDiscoveryEvent event);
  void dispatchPushedObjectInstanceDiscovery();
  void dispatchObjectInstanceRemoval(
      ProcessFederationObjectInstanceRemovalEvent event);
  void dispatchPushedObjectInstanceRemoval();
  void dispatchObjectInstanceScopeChange(
      ProcessFederationObjectInstanceScopeChangeEvent event);
  void dispatchPushedObjectInstanceScopeChange();
  void dispatchAttributeRelevanceAdvisory(
      ProcessFederationAttributeRelevanceAdvisoryEvent event);
  void dispatchPushedAttributeRelevanceAdvisory();
  void dispatchAttributeTransportationTypeChange(
      ProcessFederationAttributeTransportationTypeChangeEvent event);
  void dispatchPushedAttributeTransportationTypeChange();
  void dispatchAttributeTransportationTypeQuery(
      ProcessFederationAttributeTransportationTypeQueryEvent event);
  void dispatchPushedAttributeTransportationTypeQuery();
  // Queue the process endpoint's accepted role-enable consequence through the
  // same official callback dispatcher used by all other process callbacks.
  void dispatchTimeRegulationEnabled(ProcessFederationLogicalTime event);
  void dispatchTimeConstrainedEnabled(ProcessFederationLogicalTime event);
  void dispatchTimeAdvanceGrant(ProcessFederationLogicalTime event);
  void dispatchPushedTimeAdvanceGrant();
  void dispatchFlushQueueGrant(
      ProcessFederationLogicalTime grantedTime,
      ProcessFederationLogicalTime optimisticTime);
  void dispatchPushedFlushQueueGrant();
  void setTimeRoleEnableCompletionHandlers(
      CallbackCompletionHandler regulation,
      CallbackCompletionHandler constrained);
  // Dispatch all event frames captured while the most recent service request
  // was in flight.  This is used by the request path before it returns so an
  // HLA_IMMEDIATE callback is not stranded behind a later explicit Evoke.
  void dispatchPendingPushedEvents();

  // Events captured while a request was in flight are drained by the public
  // ambassador before it returns from that service. This count never reads
  // the socket and therefore keeps the focused process slice non-blocking.
  [[nodiscard]] std::size_t pendingPushedEventCount() const noexcept;
  [[nodiscard]] std::size_t pendingPushedAttributeUpdateCount() const noexcept;
  [[nodiscard]] std::size_t
  pendingPushedAttributeValueUpdateRequestCount() const noexcept;
  [[nodiscard]] std::size_t pendingPushedAttributeOwnershipQueryCount()
      const noexcept;
  [[nodiscard]] std::size_t
  pendingPushedAttributeOwnershipAcquisitionIfAvailableCount() const noexcept;
  [[nodiscard]] std::size_t pendingPushedAttributeOwnershipAcquisitionCount()
      const noexcept;
  [[nodiscard]] std::size_t pendingPushedAttributeOwnershipUnavailableCount()
      const noexcept;
  [[nodiscard]] std::size_t pendingPushedObjectInstanceDiscoveryCount()
      const noexcept;
  [[nodiscard]] std::size_t pendingPushedObjectInstanceRemovalCount()
      const noexcept;
  [[nodiscard]] std::size_t pendingPushedObjectInstanceScopeChangeCount()
      const noexcept;
  [[nodiscard]] std::size_t pendingPushedAttributeRelevanceAdvisoryCount()
      const noexcept;
  [[nodiscard]] std::size_t
  pendingPushedAttributeTransportationTypeChangeCount() const noexcept;
  [[nodiscard]] std::size_t
  pendingPushedAttributeTransportationTypeQueryCount() const noexcept;
  [[nodiscard]] std::size_t pendingPushedTimeAdvanceGrantCount()
      const noexcept;
  [[nodiscard]] std::size_t pendingPushedFlushQueueGrantCount()
      const noexcept;

  // Attach the borrowed official ambassador once for the client's lifetime.
  // This is intentionally a private adapter operation, not a public RTI
  // configuration choice.
  void attachCallbackBridge(
      rti1516_2025::FederateAmbassador& recipient,
      CallbackDispatchModel model = CallbackDispatchModel::evoked);

  void attachCallbackBridge(
      std::shared_ptr<CallbackDispatcher> dispatcher,
      std::shared_ptr<rti1516_2025::umbra_binding_detail::CallbackSession>
          callbackSession);

  [[nodiscard]] bool evokeOne(std::chrono::milliseconds minimumWait);
  [[nodiscard]] bool evokeMultiple(
      std::chrono::milliseconds minimumWait,
      std::chrono::milliseconds maximumWait);
  [[nodiscard]] std::size_t pendingCallbackCount() const;

  [[nodiscard]] std::shared_ptr<ProcessTransportConnection> connection() const
      noexcept;

  void close() noexcept;

 private:
  [[nodiscard]] TransportServiceMessage request(
      TransportServiceOperation operation,
      std::vector<std::uint8_t> payload);
  [[nodiscard]] ProcessFederationInteractionEvent receivePushedEvent();
  [[nodiscard]] ProcessFederationAttributeUpdateEvent
  receivePushedAttributeUpdate();
  [[nodiscard]] ProcessFederationObjectInstanceDiscoveryEvent
  receivePushedObjectInstanceDiscovery();
  [[nodiscard]] ProcessFederationObjectInstanceRemovalEvent
  receivePushedObjectInstanceRemoval();
  [[nodiscard]] ProcessFederationObjectInstanceScopeChangeEvent
  receivePushedObjectInstanceScopeChange();
  [[nodiscard]] ProcessFederationAttributeRelevanceAdvisoryEvent
  receivePushedAttributeRelevanceAdvisory();
  [[nodiscard]] ProcessFederationAttributeTransportationTypeChangeEvent
  receivePushedAttributeTransportationTypeChange();
  [[nodiscard]] ProcessFederationAttributeTransportationTypeQueryEvent
  receivePushedAttributeTransportationTypeQuery();
  [[nodiscard]] ProcessFederationLogicalTime receivePushedTimeAdvanceGrant();
  struct PendingFlushQueueGrant final {
    ProcessFederationLogicalTime grantedTime;
    ProcessFederationLogicalTime optimisticTime;
  };
  [[nodiscard]] PendingFlushQueueGrant receivePushedFlushQueueGrant();
  void acknowledgeTsoDelivery(std::uint64_t messageId);
  void deferTsoDeliveryAcknowledgement(std::uint64_t messageId);
  void flushDeferredTsoDeliveryAcknowledgements();
  void requireCallbackBridge() const;

  std::shared_ptr<ProcessTransportConnection> connection_;
  std::unique_ptr<ProcessTransportSession> session_;
  std::unique_ptr<ProcessFederationCallbackBridge> callbackBridge_;
  // Shared with queued advisory callback tasks so switch changes are checked
  // at callback entry, including the HLA_EVOKED case where a process frame
  // was admitted before the setter request completed.
  std::shared_ptr<std::atomic_bool> attributeRelevanceAdvisorySwitchState_;
  std::deque<ProcessFederationInteractionEvent> pendingEvents_;
  std::deque<ProcessFederationAttributeUpdateEvent> pendingAttributeUpdateEvents_;
  std::deque<ProcessFederationAttributeValueUpdateRequestEvent>
      pendingAttributeValueUpdateRequestEvents_;
  std::deque<ProcessFederationAttributeOwnershipQueryEvent>
      pendingAttributeOwnershipQueryEvents_;
  std::deque<ProcessFederationAttributeOwnershipAcquisitionIfAvailableEvent>
      pendingAttributeOwnershipAcquisitionIfAvailableEvents_;
  std::deque<ProcessFederationAttributeOwnershipAcquisitionEvent>
      pendingAttributeOwnershipAcquisitionEvents_;
  std::deque<ProcessFederationAttributeOwnershipUnavailableEvent>
      pendingAttributeOwnershipUnavailableEvents_;
  std::deque<ProcessFederationObjectInstanceDiscoveryEvent>
      pendingObjectInstanceDiscoveryEvents_;
  std::deque<ProcessFederationObjectInstanceRemovalEvent>
      pendingObjectInstanceRemovalEvents_;
  std::deque<ProcessFederationObjectInstanceScopeChangeEvent>
      pendingObjectInstanceScopeChangeEvents_;
  std::deque<ProcessFederationAttributeRelevanceAdvisoryEvent>
      pendingAttributeRelevanceAdvisoryEvents_;
  std::deque<ProcessFederationAttributeTransportationTypeChangeEvent>
      pendingAttributeTransportationTypeChangeEvents_;
  std::deque<ProcessFederationAttributeTransportationTypeQueryEvent>
      pendingAttributeTransportationTypeQueryEvents_;
  std::deque<ProcessFederationLogicalTime> pendingTimeAdvanceGrantEvents_;
  std::deque<PendingFlushQueueGrant> pendingFlushQueueGrantEvents_;
  // An unsolicited Request Retraction can arrive while a receive poll is
  // still awaiting its response.  Queue its delivery acknowledgement until
  // that outer response has been consumed so the single process stream never
  // carries nested request/response identities.
  std::deque<std::uint64_t> deferredTsoDeliveryAcknowledgements_;
  // The process profile currently owns one joined-federate lifetime per
  // client.  Retain its identity so callback completion can acknowledge the
  // federation-owned TSO entry without exposing a second public handle.
  std::optional<std::wstring> joinedFederationName_;
  std::uint64_t joinedFederateId_ = 0U;
  // A callback may complete while a service request is still consuming its
  // response stream.  Keep delivery acknowledgements deferred until the
  // outer request has received its response so the single process connection
  // never carries a nested request/response pair.
  std::size_t requestDepth_ = 0U;
  std::uint64_t nextRequestId_ = 0U;
};

}  // namespace umbra::detail
