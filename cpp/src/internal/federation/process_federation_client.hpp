#pragma once

#include "internal/federation/process_federation_callback_bridge.hpp"
#include "internal/federation/process_transport_session.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <deque>
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

  void createFederationExecution(std::wstring federationName);

  [[nodiscard]] ProcessFederationJoinResult joinFederationExecution(
      std::wstring federationName,
      std::wstring federateType,
      std::optional<std::wstring> requestedFederateName = std::nullopt);

  void resignFederationExecution(
      std::wstring federationName,
      std::uint64_t federateId,
      rti1516_2025::ResignAction resignAction);

  [[nodiscard]] std::optional<std::uint64_t>
  lookupInteractionClassHandle(
      std::wstring federationName,
      std::uint64_t federateId,
      std::wstring interactionClassName);

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

  void publishInteractionClass(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t interactionClassHandle);
  void unpublishInteractionClass(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t interactionClassHandle);
  void subscribeInteractionClass(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t interactionClassHandle,
      bool active);
  void unsubscribeInteractionClass(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t interactionClassHandle);

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
  [[nodiscard]] ProcessFederationDimensionUpperBoundResult
  lookupDimensionUpperBound(
      std::wstring federationName,
      std::uint64_t federateId,
      std::uint64_t dimensionHandle);
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
  // Dispatch all event frames captured while the most recent service request
  // was in flight.  This is used by the request path before it returns so an
  // HLA_IMMEDIATE callback is not stranded behind a later explicit Evoke.
  void dispatchPendingPushedEvents();

  // Events captured while a request was in flight are drained by the public
  // ambassador before it returns from that service. This count never reads
  // the socket and therefore keeps the focused process slice non-blocking.
  [[nodiscard]] std::size_t pendingPushedEventCount() const noexcept;
  [[nodiscard]] std::size_t pendingPushedAttributeUpdateCount() const noexcept;
  [[nodiscard]] std::size_t pendingPushedObjectInstanceDiscoveryCount()
      const noexcept;
  [[nodiscard]] std::size_t pendingPushedObjectInstanceRemovalCount()
      const noexcept;
  [[nodiscard]] std::size_t pendingPushedObjectInstanceScopeChangeCount()
      const noexcept;
  [[nodiscard]] std::size_t pendingPushedAttributeRelevanceAdvisoryCount()
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
  std::deque<ProcessFederationObjectInstanceDiscoveryEvent>
      pendingObjectInstanceDiscoveryEvents_;
  std::deque<ProcessFederationObjectInstanceRemovalEvent>
      pendingObjectInstanceRemovalEvents_;
  std::deque<ProcessFederationObjectInstanceScopeChangeEvent>
      pendingObjectInstanceScopeChangeEvents_;
  std::deque<ProcessFederationAttributeRelevanceAdvisoryEvent>
      pendingAttributeRelevanceAdvisoryEvents_;
  std::uint64_t nextRequestId_ = 0U;
};

}  // namespace umbra::detail
