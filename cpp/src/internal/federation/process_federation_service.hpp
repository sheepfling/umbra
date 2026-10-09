#pragma once

#include "internal/federation/process_federation_service_protocol.hpp"

#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_transport_session.hpp"
#include "internal/observability/service_report_store.hpp"

#include <cstdint>
#include <deque>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <stdexcept>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace umbra::detail {

// Private service behavior switches used while the process endpoint is being
// carried toward the public RTI binding.  Polling remains the compatibility
// default for the existing service tests; the independently launched process
// slice enables event delivery so the client can exercise the official
// FederateAmbassador callback bridge over the process boundary.
struct ProcessFederationServiceOptions final {
  bool pushReceiveOrderEvents = false;
  // Create Federation Execution owns the caller's FOM/MIM/time designators at
  // the public boundary.  The process service accepts them only through the
  // same standards-derived coordinator seam used by the embedded profile;
  // without it, an explicit suffix is rejected rather than ignored.
  using CreateFomPreparation = std::function<std::optional<FederationDefinition>(
      std::vector<std::wstring> const& fomModules,
      std::optional<std::wstring> const& mimModule,
      std::wstring const& logicalTimeImplementationName)>;
  CreateFomPreparation createFomPreparation;
  // The process service is intentionally independent of the public FOM
  // coordinator. A configured server may inject the same standards-derived
  // preparation callback used by the embedded profile; without it, a Join
  // carrying additional modules is rejected rather than silently ignored.
  using AdditionalFomPreparation = std::function<std::optional<FederationDefinition>(
      FederationDefinition const& existing,
      std::vector<std::wstring> const& additionalFomModules)>;
  AdditionalFomPreparation additionalFomPreparation;
  // The process service exposes only the production storage decision: a
  // filesystem directory.  Test-only memory stores remain an internal
  // injection seam and are not selected by this runtime option.
  std::filesystem::path serviceReportDirectory;
};

// Private service binding for the first process-boundary federation path.  A
// validated base definition is supplied by the higher-level FOM coordinator;
// explicit Create FOM/MIM/time inputs and an optional Join suffix carry
// designators to injected preparation callbacks before the registry commits
// a definition or replacement definition.
// Each transport session gets a handler tied to its joined federate identity,
// while the registry remains the authority for publication, subscription, and
// receive-order routing.
class ProcessFederationService final {
 public:
  ProcessFederationService(
      EmbeddedFederationRegistry& registry,
      FederationDefinition federationDefinition,
      ProcessFederationServiceOptions options = {});

  [[nodiscard]] ProcessTransportServiceDispatcher::Handler handlerFor(
      ProcessTransportSession& session);

  // The transport owner calls detach after the session's serving loop exits.
  // Detaching does not resign a federate; lifecycle operations will be bound
  // to the registry in a later slice.
  void detach(ProcessTransportSession& session) noexcept;

  // Project registry work produced by an adapter-managed connection loss onto
  // the surviving process sessions. The registry mutation must happen before
  // this method is called; this method only crosses the process callback
  // boundary for the resulting standard events.
  [[nodiscard]] bool dispatchConnectionLossResult(
      std::wstring const& federationName,
      FederationRegistryResult result,
      std::optional<std::uint64_t> departedFederateId = std::nullopt,
      std::optional<FederateLostReportPlan> federateLostReport = std::nullopt,
      std::wstring faultDescription = {});

  // Apply an unexpected process-endpoint loss and project the resulting
  // standard callbacks, including HLAreportFederateLost when subscribed.
  // Capture the MOM routing plan before the registry removes the lost member.
  [[nodiscard]] bool connectionLost(
      std::wstring const& federationName,
      std::uint64_t departedFederateId,
      std::wstring faultDescription);

 private:
  struct SessionState final {
    std::optional<std::wstring> federationName;
    std::uint64_t federateId = 0U;
    // The process endpoint keeps the same federation-owned temporal state
    // object for the entire joined-federate lifetime.  Keeping the pointer in
    // the session as well as the registry makes the process service's
    // ownership boundary explicit and lets later role/grant operations use
    // the exact state established at Join.
    std::shared_ptr<FederateTimeState> timeState;
    // One writer and one immutable location belong to this joined-federate
    // lifetime.  Resign clears the state; rejoin allocates a new writer.
    std::unique_ptr<ServiceReportWriter> serviceReportWriter;
    std::filesystem::path serviceReportLocation;
    std::deque<ProcessFederationInteractionEvent> interactionEvents;
    std::deque<ProcessFederationAttributeUpdateEvent> attributeUpdateEvents;
    std::deque<ProcessFederationObjectInstanceDiscoveryEvent>
        objectInstanceDiscoveryEvents;
    std::deque<ProcessFederationObjectInstanceRemovalEvent>
        objectInstanceRemovalEvents;
    std::uint64_t nextObjectLifecycleCallbackOrderSequence = 1U;
    std::deque<ProcessFederationObjectInstanceScopeChangeEvent>
        objectInstanceScopeChangeEvents;
    std::deque<ProcessFederationAttributeRelevanceAdvisoryEvent>
        attributeRelevanceAdvisoryEvents;
    std::deque<ProcessFederationAttributeTransportationTypeChangeEvent>
        attributeTransportationTypeChangeEvents;
    std::deque<ProcessFederationAttributeTransportationTypeQueryEvent>
        attributeTransportationTypeQueryEvents;
    std::deque<ProcessFederationInteractionTransportationTypeChangeEvent>
        interactionTransportationTypeChangeEvents;
    std::deque<ProcessFederationInteractionTransportationTypeQueryEvent>
        interactionTransportationTypeQueryEvents;
    std::deque<ProcessFederationAttributeValueUpdateRequestEvent>
        attributeValueUpdateRequestEvents;
    std::deque<ProcessFederationAttributeOwnershipQueryEvent>
        attributeOwnershipQueryEvents;
    std::deque<
        ProcessFederationAttributeOwnershipAcquisitionIfAvailableEvent>
        attributeOwnershipAcquisitionIfAvailableEvents;
    std::deque<ProcessFederationAttributeOwnershipAcquisitionEvent>
        attributeOwnershipAcquisitionEvents;
    std::deque<ProcessFederationAttributeOwnershipUnavailableEvent>
        attributeOwnershipUnavailableEvents;
    std::deque<ProcessFederationSynchronizationPointAnnouncementEvent>
        synchronizationPointAnnouncementEvents;
    std::deque<ProcessFederationFederationSynchronizedEvent>
        federationSynchronizedEvents;
    std::deque<ProcessFederationSaveEvent> saveEvents;
    std::deque<ProcessFederationRestoreEvent> restoreEvents;
  };

  [[nodiscard]] TransportServiceMessage handle(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleCreate(
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleDestroy(
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleJoin(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleResign(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleRegisterFederationSynchronizationPoint(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleSynchronizationPointAchieved(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleRequestFederationSave(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleFederateSaveControl(
      ProcessTransportSession& session,
      TransportServiceMessage const& request,
      TransportServiceOperation operation);
  [[nodiscard]] TransportServiceMessage handleQueryFederationSaveStatus(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleAbortFederationSave(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleRequestFederationRestore(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleFederateRestoreComplete(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleFederateRestoreNotComplete(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleAbortFederationRestore(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleQueryFederationRestoreStatus(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleSendInteraction(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleSendInteractionWithRegions(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleSendDirectedInteraction(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleRetract(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleUpdateAttributeValues(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleRequestAttributeValueUpdate(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleRequestAttributeValueUpdateClass(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleRequestAttributeValueUpdateClassWithRegions(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleAttributeOwnershipCheck(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleQueryAttributeOwnership(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleAttributeOwnershipAcquisitionIfAvailable(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleAttributeOwnershipAcquisition(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleAttributeOwnershipReleaseDenied(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleAttributeOwnershipAcquisitionCancellation(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleCancelNegotiatedAttributeOwnershipDivestiture(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleNegotiatedAttributeOwnershipDivestiture(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleConfirmDivestiture(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleUnconditionalAttributeOwnershipDivestiture(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleReceiveInteraction(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleAcknowledgeTsoDelivery(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleQueryLogicalTime(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleQueryLookahead(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleModifyLookahead(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleQueryTimeBounds(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleEnableTimeRegulation(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleEnableTimeConstrained(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleDisableTimeRegulation(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleDisableTimeConstrained(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleDisableTimeRole(
      ProcessTransportSession& session,
      TransportServiceMessage const& request,
      bool regulation);
  [[nodiscard]] TransportServiceMessage handleTimeAdvanceRequest(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleTimeAdvanceRequestAvailable(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleNextMessageRequest(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleNextMessageRequestAvailable(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleFlushQueueRequest(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleTimeAdvanceRequestForMode(
      ProcessTransportSession& session,
      TransportServiceMessage const& request,
      FederateTimeAdvanceMode mode);
  [[nodiscard]] TransportServiceMessage handleReceiveAttributeUpdate(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleReceiveObjectInstanceDiscovery(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleGetInteractionClassHandle(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleGetFederateHandle(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleGetFederateName(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleNormalizeHandle(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleGetObjectClassHandle(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleGetParameterHandle(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleGetAttributeHandle(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleGetObjectInstanceHandle(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleGetObjectInstanceName(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleGetKnownObjectClassHandle(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleGetUpdateRateValue(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleGetUpdateRateValueForAttribute(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleGetObjectClassName(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleGetInteractionClassName(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleGetAttributeName(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleGetParameterName(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleInteractionClassDeclaration(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleChangeInteractionOrderType(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleChangeAttributeOrderType(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleChangeDefaultAttributeOrderType(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleChangeDefaultAttributeTransportationType(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleRequestAttributeTransportationTypeChange(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleQueryAttributeTransportationType(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleRequestInteractionTransportationTypeChange(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleQueryInteractionTransportationType(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleInteractionClassRegionalSubscription(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleObjectClassDirectedInteractionDeclaration(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleObjectClassAttributeDeclaration(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleObjectClassAttributeSubscription(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleObjectClassAttributeRegionalSubscription(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleRegisterObjectInstance(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleLocalDeleteObjectInstance(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleDeleteObjectInstance(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleReserveObjectInstanceName(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleReleaseObjectInstanceName(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleReserveMultipleObjectInstanceNames(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleReleaseMultipleObjectInstanceNames(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleGetDimensionHandle(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleGetDimensionName(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleGetTransportationTypeHandle(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleGetTransportationTypeName(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleGetDimensionUpperBound(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleGetAvailableDimensionsForObjectClass(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleGetAvailableDimensionsForInteractionClass(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleCreateRegion(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleCommitRegionModifications(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleDeleteRegion(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleGetDimensionHandleSet(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleGetRangeBounds(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleSetRangeBounds(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleGetAttributeScopeAdvisorySwitch(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleSetAttributeScopeAdvisorySwitch(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleGetObjectClassRelevanceAdvisorySwitch(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleGetAttributeRelevanceAdvisorySwitch(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleGetInteractionRelevanceAdvisorySwitch(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleSetAttributeRelevanceAdvisorySwitch(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleGetConveyRegionDesignatorSetsSwitch(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleGetAllowRelaxedDDMSwitch(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleSetConveyRegionDesignatorSetsSwitch(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleGetServiceReportingSwitch(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleSetServiceReportingSwitch(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleGetExceptionReportingSwitch(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleReportServiceException(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleReportFailedServiceInvocation(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleReportSuccessfulServiceInvocation(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleReportSuccessfulVoidServiceInvocation(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleServiceInvocationReport(
      ProcessTransportSession& session,
      TransportServiceMessage const& request,
      ProcessFederationServiceInvocationReport report);
  [[nodiscard]] TransportServiceMessage handleRecheckExceptionReport(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleSetExceptionReportingSwitch(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleGetSendServiceReportsToFileSwitch(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleSetSendServiceReportsToFileSwitch(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleGetAutomaticResignDirective(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleSetAutomaticResignDirective(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleRegisterObjectInstanceWithRegions(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleObjectInstanceRegionAssociation(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);

  // Project registry-reserved discoveries onto the receiving process.  The
  // public callback bridge is the only standards-facing consumer; the service
  // uses this private helper for both registration and later subscription.
  [[nodiscard]] bool enqueueObjectInstanceDiscoveries(
      std::wstring const& federationName,
      std::vector<ObjectInstanceDiscoveryRecipient> discoveries);

  [[nodiscard]] bool enqueueJoinedFederateMomConditionalAttributeUpdate(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::vector<std::string> const& attributeNames);

  [[nodiscard]] bool enqueueObjectInstanceRemovals(
      std::wstring const& federationName,
      std::vector<ObjectInstanceRemovalRecipient> removals,
      std::vector<std::uint8_t> userSuppliedTag,
      std::optional<ProcessFederationLogicalTime> timestamp = std::nullopt,
      std::uint64_t retractionMessageId = 0U,
      bool provideRetraction = false);

  [[nodiscard]] bool enqueueObjectInstanceScopeChanges(
      std::wstring const& federationName,
      std::vector<ObjectInstanceScopeChangeRecipient> changes);

  [[nodiscard]] bool enqueueAttributeRelevanceAdvisories(
      std::wstring const& federationName,
      std::vector<AttributeRelevanceAdvisoryRecipient> advisories);

  [[nodiscard]] bool enqueueAttributeOwnershipAcquisitionWorkItems(
      std::wstring const& federationName,
      std::vector<AttributeOwnershipAcquisitionWorkItem> workItems);

  [[nodiscard]] bool enqueueAttributeOwnershipAssumptionRecipients(
      std::wstring const& federationName,
      std::vector<AttributeOwnershipAssumptionRecipient> recipients,
      bool deferUntilCallbackEnabled = false);

  // Flush ownership-assumption frames that were intentionally retained while
  // the receiving federate had callbacks disabled.  The registry delivery
  // boundary is crossed only here, immediately before the frame is sent.
  [[nodiscard]] bool flushDeferredAttributeOwnershipAssumptionEvents(
      ProcessTransportSession& session,
      std::wstring const& federationName,
      std::uint64_t receivingFederateId);

  // Confirm Divestiture notifications use the same receive-order fence as
  // ownership acquisition callbacks, but retain a distinct registry delivery
  // boundary so the transfer commits immediately before callback projection.
  [[nodiscard]] bool enqueueConfirmDivestitureNotifications(
      std::wstring const& federationName,
      std::vector<ConfirmDivestitureNotification> notifications);

  [[nodiscard]] bool enqueueAttributeOwnershipUnavailableRecipients(
      std::wstring const& federationName,
      std::vector<AttributeOwnershipUnavailableRecipient> recipients,
      std::vector<std::uint8_t> userSuppliedTag);

  [[nodiscard]] bool enqueueSynchronizationPointAnnouncements(
      std::wstring const& federationName,
      std::vector<SynchronizationPointAnnouncement> announcements);

  [[nodiscard]] bool enqueueFederationSynchronizedNotifications(
      std::wstring const& federationName,
      std::vector<FederationSynchronizedNotification> notifications);
  [[nodiscard]] bool enqueueFederationSaveNotifications(
      std::wstring const& federationName,
      std::vector<FederationSaveNotification> notifications);
  [[nodiscard]] bool enqueueFederationRestoreNotifications(
      std::wstring const& federationName,
      std::vector<FederationRestoreNotification> notifications);

  // Append one selected service-report record for a process-owned joined
  // federate.  The registry remains authoritative for switch gating and
  // serial allocation; the session retains the immutable filesystem writer
  // allocated at Join.  Suppressed/interaction destinations are successful
  // no-ops, while a selected-file write failure is surfaced to the request.
  [[nodiscard]] bool appendSelectedServiceReportRecord(
      ProcessTransportSession& session,
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint16_t serviceGroup,
      FederateServiceReportRecordEncoder encodeRecord);

  // Execute registry-owned time-grant work only after the registry lock has
  // been released.  A grant may make another pending request eligible, so the
  // helper drains bounded re-evaluation rounds until the federation reaches a
  // fixed point.
  void dispatchTimeAdvanceGrants(
      std::wstring const& federationName,
      std::vector<FederationTimeGrantDispatch> dispatches);

  // Typed TSO events keep delivery ownership private to the process boundary.
  void dispatchTsoAttributeUpdateDelivery(std::wstring const& federationName,
      std::uint64_t receivingFederateId, TsoAttributeUpdateDelivery const& attribute);
  void dispatchTsoInteractionDelivery(std::wstring const& federationName,
      std::uint64_t receivingFederateId, TsoInteractionDelivery const& interaction);
  void dispatchTsoInteractionPayloads(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId,
      rti1516_2025::LogicalTime const& boundary);

  [[nodiscard]] TransportServiceMessage rejected(
      TransportServiceMessage const& request) const;
  [[nodiscard]] TransportServiceMessage invalid(
      TransportServiceMessage const& request) const;
  [[nodiscard]] TransportServiceMessage internalError(
      TransportServiceMessage const& request) const;

  EmbeddedFederationRegistry& registry_;
  std::shared_ptr<FederationDefinition const> federationDefinition_;
  ProcessFederationServiceOptions options_;
  std::unique_ptr<ServiceReportStore> serviceReportStore_;
  // Pushed directed events may be buffered in a receiver process client even
  // after they leave the service's queue. Retain the recipient sessions long
  // enough to send a matching retraction notification.
  std::map<std::uint64_t, std::vector<ProcessTransportSession*>>
      pendingPushedRetractionRecipients_;
  std::map<std::uint64_t, std::uint64_t> processTsoMessageProducers_;
  // Joining may replace a federation's composed definition. Serialize that
  // preparation/commit pair so two process clients cannot both compose from
  // the same stale definition and race the registry replacement.
  mutable std::mutex joinMutex_;
  mutable std::mutex mutex_;
  std::map<ProcessTransportSession*, SessionState> sessions_;
  std::map<std::uint64_t, ProcessTransportSession*> sessionsByFederateId_;
};

}  // namespace umbra::detail
