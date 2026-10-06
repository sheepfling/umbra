#include "internal/runtime/umbra_rti_ambassador.hpp"

#include "internal/runtime/ambassador_shared_utilities.hpp"

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "internal/federation/federation_management_coordinator.hpp"
#include "internal/fom/hla_names.hpp"
#include "internal/time/federate_time_state.hpp"
#endif

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace rti1516_2025::umbra_binding_detail {

void UmbraRtiAmbassador::nextMessageRequest(LogicalTime const& time) {
  auto instrumentationScope = beginRtiCall("nextMessageRequest");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    requestProcessTimeAdvance(
        time,
        umbra::detail::FederateTimeAdvanceMode::next_message_request,
        L"Next Message Request");
    return;
  }
#endif
  std::shared_ptr<umbra::detail::FederateTimeState> timeState;
  std::wstring federationName;
  std::uint64_t federateId = 0;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Next Message Request");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          L"Next Message Request requires an active joined federate with initialized logical time.");
    }

    timeState = federateTimeState_;
    federationName = *joinedFederationName_;
    federateId = *joinedFederateId_;
  }

  // Keep the caller's requested boundary independently of the effective grant
  // target. NMR may grant at the earliest currently queued TSO timestamp when
  // that timestamp is no greater than the supplied request.
  auto requestedTime = cloneAmbassadorReferenceLogicalTime(
      timeState->implementationName(),
      time);
  // Section 8.10.1 names the supplied value Logical time. Table 5 uses the
  // quoted time.toString() form. Preserve this caller-supplied boundary for
  // reporting even when a queued TSO message later determines an earlier
  // effective Time Advance Grant target.
  auto const reportTimeArgument = umbra::detail::MomServiceArgument{
      umbra::detail::MomArgumentType::logical_time,
      L"Logical time",
      umbra::detail::formatMomLogicalTime(*requestedTime),
  };
  std::shared_ptr<LogicalTime> effectiveTime;
  std::shared_ptr<LogicalTime const> earliestQueuedTimestamp;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Next Message Request");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_ ||
        *joinedFederationName_ != federationName || *joinedFederateId_ != federateId ||
        federateTimeState_ != timeState) {
      throw FederateNotExecutionMember(
          L"Next Message Request requires an active joined federate with initialized logical time.");
    }

    auto const queued = embeddedFederationRegistry().earliestTsoTimestampFor(
        federationName,
        federateId);
    if (queued && *queued) {
      earliestQueuedTimestamp = *queued;
    }
  }

  if (earliestQueuedTimestamp) {
    auto candidate = cloneAmbassadorReferenceLogicalTime(
        timeState->implementationName(),
        *earliestQueuedTimestamp);
    auto const snapshot = timeState->snapshot();
    try {
      if (snapshot.currentTime && *candidate >= *snapshot.currentTime &&
          *candidate <= *requestedTime) {
        effectiveTime = std::move(candidate);
      }
    } catch (Exception const&) {
      throw InvalidLogicalTime(
          L"The next queued TSO timestamp cannot be compared with the requested logical time.");
    }
  }

  umbra::detail::FederateTimeAdvanceResult result;
  umbra::detail::FederationTimeGrantDispatchResult scheduled;
  bool reportAccepted = false;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Next Message Request");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_ ||
        *joinedFederationName_ != federationName || *joinedFederateId_ != federateId ||
        federateTimeState_ != timeState) {
      throw FederateNotExecutionMember(
          L"Next Message Request requires an active joined federate with initialized logical time.");
    }

    result = timeState->requestNextMessageAdvance(
        std::move(requestedTime),
        std::move(effectiveTime));
    if (result.status == umbra::detail::FederateTimeAdvanceStatus::applied) {
      retireAmbassadorTsoMessagePayloadsAtRetractionBoundary(
          federationName,
          federateId,
          *timeState);
      scheduled = embeddedFederationRegistry().requestTimeAdvanceGrant(
          federationName,
          federateId,
          result.generation);
      if (scheduled.status == umbra::detail::FederationTimeGrantStatus::applied) {
        // The accepted NMR records its supplied request now. The shared grant
        // dispatch later completes the request at either the selected queued
        // timestamp or the supplied boundary.
        reportAccepted = true;
      }
    }
  }
  if (reportAccepted) {
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"NextMessageRequest",
        umbra::detail::MomServiceType::time_management,
        {reportTimeArgument},
        true);
  }

  switch (result.status) {
    case umbra::detail::FederateTimeAdvanceStatus::applied:
      break;
    case umbra::detail::FederateTimeAdvanceStatus::logical_time_already_passed:
      throw LogicalTimeAlreadyPassed(
          L"Next Message Request cannot move a joined federate backward in logical time.");
    case umbra::detail::FederateTimeAdvanceStatus::time_advance_pending:
      throw InTimeAdvancingState(
          L"The joined federate already has a time-advance request awaiting a grant.");
    case umbra::detail::FederateTimeAdvanceStatus::time_regulation_pending:
      throw RequestForTimeRegulationPending(
          L"The joined federate has an Enable Time Regulation request awaiting its callback.");
    case umbra::detail::FederateTimeAdvanceStatus::time_constrained_pending:
      throw RequestForTimeConstrainedPending(
          L"The joined federate has an Enable Time Constrained request awaiting its callback.");
    case umbra::detail::FederateTimeAdvanceStatus::invalid_logical_time:
      throw InvalidLogicalTime(
          L"The requested logical time is invalid for the joined federation's implementation.");
    case umbra::detail::FederateTimeAdvanceStatus::inactive:
      throw FederateNotExecutionMember(
          L"The joined federate's logical-time state is no longer active.");
    case umbra::detail::FederateTimeAdvanceStatus::generation_exhausted:
      throw RTIinternalError(
          L"Umbra exhausted the embedded time-advance request generation space.");
  }

  if (scheduled.status != umbra::detail::FederationTimeGrantStatus::applied) {
    throw RTIinternalError(
        L"Umbra could not register the accepted Next Message Request with its federation scheduler.");
  }

  queueAmbassadorJoinedFederateMomConditionalAttributeUpdateForJoinedFederate(
      federationName,
      federateId,
      {umbra::detail::hla::utf8::mom::time_manager_state});

  // The shared grant dispatch delivers the selected TSO timestamp cohort
  // before the ordinary Time Advance Grant callback. If no queued timestamp
  // is eligible, the state target remains the caller's requested time.
  flushAmbassadorAsynchronousReceiveCallbacks(timeState);
  submitAmbassadorTimeAdvanceGrantDispatches(std::move(scheduled.dispatches));
  } catch (Exception const& exception) {
    emitExceptionReport(L"Next Message Request", exception);
    throw;
  }
}

void UmbraRtiAmbassador::nextMessageRequestAvailable(LogicalTime const& time) {
  auto instrumentationScope = beginRtiCall("nextMessageRequestAvailable");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    requestProcessTimeAdvance(
        time,
        umbra::detail::FederateTimeAdvanceMode::next_message_request_available,
        L"Next Message Request Available");
    return;
  }
#endif
  requestAvailableTimeAdvance(
      time,
      umbra::detail::FederateTimeAdvanceMode::next_message_request_available,
      true,
      L"Next Message Request Available");
  } catch (Exception const& exception) {
    emitExceptionReport(L"Next Message Request Available", exception);
    throw;
  }
}

}  // namespace rti1516_2025::umbra_binding_detail
