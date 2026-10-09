#include "internal/runtime/umbra_rti_ambassador.hpp"
#include "internal/federation/federation_management_coordinator.hpp"
#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_client.hpp"
#include "internal/fom/hla_names.hpp"
#include "internal/observability/mom_service_report_encoding.hpp"
#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/ambassador_string_helpers.hpp"
#include "internal/runtime/umbra_rti_ambassador_service_failure_translation.hpp"
#include "internal/time/federate_time_state.hpp"
#include "internal/time/federation_time_bounds.hpp"
#include "internal/time/federation_time_grant_policy.hpp"

#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

namespace rti1516_2025::umbra_binding_detail {
using namespace service_failure_translation;

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
// Private 2025 process-endpoint time-advance translation.
void UmbraRtiAmbassador::requestProcessTimeAdvance(
    LogicalTime const &time,
    umbra::detail::FederateTimeAdvanceMode mode,
    std::wstring const &serviceName) {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  umbra::detail::ProcessFederationClient *processClient = nullptr;
  {
    std::scoped_lock lock(mutex_);
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          serviceName + L" requires membership in a federation execution.");
    }
    if (processTimeRegulationCallbackPending_) {
      throw RequestForTimeRegulationPending(
          L"The joined federate has an Enable Time Regulation request awaiting its callback.");
    }
    if (processTimeConstrainedCallbackPending_) {
      throw RequestForTimeConstrainedPending(
          L"The joined federate has an Enable Time Constrained request awaiting its callback.");
    }
    processClient = processFederationClient_.get();
    if (processClient == nullptr) {
      throw RTIinternalError(
          L"The configured process endpoint has no active federation client.");
    }
    federationName = *joinedFederationName_;
    federateId = *joinedFederateId_;
  }

  umbra::detail::ProcessFederationLogicalTime requestedTime;
  requestedTime.implementationName = time.implementationName();
  try {
    auto const encoded = time.encode();
    if (encoded.size() != 0U) {
      auto const *data = static_cast<std::uint8_t const*>(encoded.data());
      if (data == nullptr) {
        throw InvalidLogicalTime(
            L"The requested logical time has no usable encoded value.");
      }
      requestedTime.encoding.assign(data, data + encoded.size());
    }
  } catch (InvalidLogicalTime const&) {
    throw;
  } catch (Exception const&) {
    throw InvalidLogicalTime(
        L"The requested logical time cannot be encoded by the selected implementation.");
  }

  umbra::detail::ProcessFederationTimeAdvanceResult processResult;
  try {
    switch (mode) {
      case umbra::detail::FederateTimeAdvanceMode::time_advance_request:
        processResult = processClient->timeAdvanceRequest(
            std::move(federationName), federateId, std::move(requestedTime));
        break;
      case umbra::detail::FederateTimeAdvanceMode::time_advance_request_available:
        processResult = processClient->timeAdvanceRequestAvailable(
            std::move(federationName), federateId, std::move(requestedTime));
        break;
      case umbra::detail::FederateTimeAdvanceMode::next_message_request:
        processResult = processClient->nextMessageRequest(
            std::move(federationName), federateId, std::move(requestedTime));
        break;
      case umbra::detail::FederateTimeAdvanceMode::next_message_request_available:
        processResult = processClient->nextMessageRequestAvailable(
            std::move(federationName), federateId, std::move(requestedTime));
        break;
      case umbra::detail::FederateTimeAdvanceMode::flush_queue_request:
        processResult = processClient->flushQueueRequest(
            std::move(federationName), federateId, std::move(requestedTime));
        break;
      case umbra::detail::FederateTimeAdvanceMode::none:
        throw RTIinternalError(
            L"The process endpoint does not support this temporal request through the shared adapter.");
    }
  } catch (umbra::detail::ProcessFederationServiceProtocolError const &error) {
    throw RTIinternalError(wideAscii(error.what()));
  } catch (umbra::detail::ProcessFederationClientError const &error) {
    throw RTIinternalError(wideAscii(error.what()));
  }

  using Status = umbra::detail::ProcessFederationTimeAdvanceStatus;
  switch (processResult.status) {
      case Status::applied:
      if (processResult.optimisticTime && processResult.grantedTime) {
        try {
          processClient->dispatchFlushQueueGrant(
              std::move(*processResult.grantedTime),
              std::move(*processResult.optimisticTime));
        } catch (umbra::detail::ProcessFederationCallbackBridgeError const &error) {
          throw RTIinternalError(wideAscii(error.what()));
        }
      } else if (processResult.grantedTime) {
        try {
          processClient->dispatchTimeAdvanceGrant(
              std::move(*processResult.grantedTime));
        } catch (umbra::detail::ProcessFederationCallbackBridgeError const &error) {
          throw RTIinternalError(wideAscii(error.what()));
        }
      }
      return;
    case Status::logical_time_already_passed:
      throw LogicalTimeAlreadyPassed(
          serviceName + L" cannot move a joined federate backward in logical time.");
    case Status::time_advance_pending:
      throw InTimeAdvancingState(
          L"The joined federate already has a time-advance request awaiting a grant.");
    case Status::time_regulation_pending:
      throw RequestForTimeRegulationPending(
          L"The joined federate has an Enable Time Regulation request awaiting its callback.");
    case Status::time_constrained_pending:
      throw RequestForTimeConstrainedPending(
          L"The joined federate has an Enable Time Constrained request awaiting its callback.");
    case Status::invalid_logical_time:
      throw InvalidLogicalTime(
          serviceName + L" received a logical time invalid for the joined federation.");
    case Status::inactive:
      throw FederateNotExecutionMember(
          serviceName + L" found that the joined federate's logical-time state is inactive.");
    case Status::generation_exhausted:
      throw RTIinternalError(
          L"Umbra exhausted the process time-advance request generation space.");
  }
  throw RTIinternalError(
      L"The process endpoint returned an unknown temporal request outcome.");
}
#endif

void UmbraRtiAmbassador::timeAdvanceRequest(LogicalTime const &time) {
  auto instrumentationScope = beginRtiCall("timeAdvanceRequest");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    std::wstring federationName;
    std::uint64_t federateId = 0U;
    umbra::detail::ProcessFederationClient *processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Time Advance Request requires membership in a federation execution.");
      }
      if (processTimeRegulationCallbackPending_) {
        throw RequestForTimeRegulationPending(
            L"The joined federate has an Enable Time Regulation request awaiting its callback.");
      }
      if (processTimeConstrainedCallbackPending_) {
        throw RequestForTimeConstrainedPending(
            L"The joined federate has an Enable Time Constrained request awaiting its callback.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }

    umbra::detail::ProcessFederationLogicalTime requestedTime;
    requestedTime.implementationName = time.implementationName();
    try {
      auto const encoded = time.encode();
      if (encoded.size() != 0U) {
        auto const *data = static_cast<std::uint8_t const*>(encoded.data());
        if (data == nullptr) {
          throw InvalidLogicalTime(
              L"The requested logical time has no usable encoded value.");
        }
        requestedTime.encoding.assign(data, data + encoded.size());
      }
    } catch (InvalidLogicalTime const&) {
      throw;
    } catch (Exception const&) {
      throw InvalidLogicalTime(
          L"The requested logical time cannot be encoded by the selected implementation.");
    }

    umbra::detail::ProcessFederationTimeAdvanceResult processResult;
    try {
      processResult = processClient->timeAdvanceRequest(
          std::move(federationName), federateId, std::move(requestedTime));
    } catch (umbra::detail::ProcessFederationServiceProtocolError const &error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const &error) {
      throw RTIinternalError(wideAscii(error.what()));
    }

    using Status = umbra::detail::ProcessFederationTimeAdvanceStatus;
    switch (processResult.status) {
      case Status::applied:
        // An already-eligible request may carry its grant in the response.
        // Deferred/cross-federate grants arrive as unsolicited process events;
        // the client request path has already queued them through the same
        // callback bridge before returning this response.
        if (processResult.grantedTime) {
          try {
            processClient->dispatchTimeAdvanceGrant(
                std::move(*processResult.grantedTime));
          } catch (umbra::detail::ProcessFederationCallbackBridgeError const &error) {
            throw RTIinternalError(wideAscii(error.what()));
          }
        }
        return;
      case Status::logical_time_already_passed:
        throw LogicalTimeAlreadyPassed(
            L"Time Advance Request cannot move a joined federate backward in logical time.");
      case Status::time_advance_pending:
        throw InTimeAdvancingState(
            L"The joined federate already has a Time Advance Request awaiting a grant.");
      case Status::time_regulation_pending:
        throw RequestForTimeRegulationPending(
            L"The joined federate has an Enable Time Regulation request awaiting its callback.");
      case Status::time_constrained_pending:
        throw RequestForTimeConstrainedPending(
            L"The joined federate has an Enable Time Constrained request awaiting its callback.");
      case Status::invalid_logical_time:
        throw InvalidLogicalTime(
            L"The requested logical time is invalid for the joined federation's implementation.");
      case Status::inactive:
        throw FederateNotExecutionMember(
            L"The joined federate's logical-time state is no longer active.");
      case Status::generation_exhausted:
        throw RTIinternalError(
            L"Umbra exhausted the process time-advance request generation space.");
    }
    throw RTIinternalError(
        L"The process endpoint returned an unknown Time Advance Request outcome.");
  }
#endif
  std::shared_ptr<umbra::detail::FederateTimeState> timeState;
  std::wstring federationName;
  std::uint64_t federateId = 0;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Time Advance Request");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          L"Time Advance Request requires a joined federate with initialized logical time.");
    }

    timeState = federateTimeState_;
    federationName = *joinedFederationName_;
    federateId = *joinedFederateId_;
  }

  // LogicalTime is a caller-provided polymorphic object. Decode a reference
  // representation outside runtime locks, then commit only the private copy.
  auto requestedTime = cloneAmbassadorReferenceLogicalTime(timeState->implementationName(), time);
  // Section 8.8.1 names this supplied value Logical time. Table 5 gives
  // LogicalTime the quoted time.toString() form; obtain it from the private
  // reference copy outside federation locks.
  auto const reportTimeArgument = umbra::detail::MomServiceArgument{
      umbra::detail::MomArgumentType::logical_time,
      L"Logical time",
      umbra::detail::formatMomLogicalTime(*requestedTime),
  };
  umbra::detail::FederateTimeAdvanceResult result;
  umbra::detail::FederationTimeGrantDispatchResult scheduled;
  bool reportAccepted = false;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Time Advance Request");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_ ||
        *joinedFederationName_ != federationName || *joinedFederateId_ != federateId ||
        federateTimeState_ != timeState) {
      throw FederateNotExecutionMember(
          L"Time Advance Request requires an active joined federate with initialized logical time.");
    }

    result = timeState->requestAdvance(std::move(requestedTime));
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
        // Acceptance puts the federate in Time Advancing state and is
        // reportable now. The distinct Time Advance Grant callback later
        // completes the logical-time transition.
        reportAccepted = true;
      }
    }
  }
  if (reportAccepted) {
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"TimeAdvanceRequest",
        umbra::detail::MomServiceType::time_management,
        {reportTimeArgument},
        true);
  }
  switch (result.status) {
    case umbra::detail::FederateTimeAdvanceStatus::applied:
      break;
    case umbra::detail::FederateTimeAdvanceStatus::logical_time_already_passed:
      throw LogicalTimeAlreadyPassed(
          L"Time Advance Request cannot move a joined federate backward in logical time.");
    case umbra::detail::FederateTimeAdvanceStatus::time_advance_pending:
      throw InTimeAdvancingState(
          L"The joined federate already has a Time Advance Request awaiting a grant.");
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
        L"Umbra could not register the accepted Time Advance Request with its federation scheduler.");
  }

  // The request has now entered Time Advancing.  Reflect the current MOM
  // HLAtimeManagerState from the federation-owned temporal state; the grant
  // callback will publish the return to Time Granted separately.
  queueAmbassadorJoinedFederateMomConditionalAttributeUpdateForJoinedFederate(
      federationName,
      federateId,
      {umbra::detail::hla::utf8::mom::time_manager_state});

  // The registry admits nonconstrained requests immediately and holds a
  // constrained TAR until strict GALT/NRG policy allows delivery. Every action
  // is submitted only after the calling thread has released its runtime locks.
  flushAmbassadorAsynchronousReceiveCallbacks(timeState);
  submitAmbassadorTimeAdvanceGrantDispatches(std::move(scheduled.dispatches));
  } catch (Exception const &exception) {
    emitExceptionReport(L"Time Advance Request", exception);
    throw;
  }
}

void UmbraRtiAmbassador::requestAvailableTimeAdvance(
    LogicalTime const &time,
    umbra::detail::FederateTimeAdvanceMode mode,
    bool selectNextQueuedMessage,
    std::wstring const &serviceName) {
  auto instrumentationScope = beginRtiCall("requestAvailableTimeAdvance");
  std::shared_ptr<umbra::detail::FederateTimeState> timeState;
  std::wstring federationName;
  std::uint64_t federateId = 0;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(serviceName);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          serviceName + L" requires an active joined federate with initialized logical time.");
    }

    timeState = federateTimeState_;
    federationName = *joinedFederationName_;
    federateId = *joinedFederateId_;
  }

  auto requestedTime = cloneAmbassadorReferenceLogicalTime(timeState->implementationName(), time);
  // Sections 8.9.1 and 8.11.1 each name their supplied value Logical time.
  // Table 5 gives LogicalTime the quoted time.toString() form. The two
  // services share scheduling machinery but retain distinct report-service
  // identities.
  std::optional<umbra::detail::MomServiceArgument> reportTimeArgument;
  std::optional<std::wstring> reportService;
  if (mode == umbra::detail::FederateTimeAdvanceMode::time_advance_request_available) {
    reportService = L"TimeAdvanceRequestAvailable";
  } else if (mode == umbra::detail::FederateTimeAdvanceMode::next_message_request_available) {
    reportService = L"NextMessageRequestAvailable";
  }
  if (reportService) {
    reportTimeArgument = umbra::detail::MomServiceArgument{
        umbra::detail::MomArgumentType::logical_time,
        L"Logical time",
        umbra::detail::formatMomLogicalTime(*requestedTime),
    };
  }
  std::shared_ptr<LogicalTime> effectiveTime;
  std::shared_ptr<LogicalTime const> earliestQueuedTimestamp;
  if (selectNextQueuedMessage) {
    {
      std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
      requireConnectedForFederationManagement(lifecycle_);
      requireFederationServiceOperationAvailable(serviceName);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_ ||
          *joinedFederationName_ != federationName || *joinedFederateId_ != federateId ||
          federateTimeState_ != timeState) {
        throw FederateNotExecutionMember(
            serviceName + L" requires an active joined federate with initialized logical time.");
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
  }

  umbra::detail::FederateTimeAdvanceResult result;
  umbra::detail::FederationTimeGrantDispatchResult scheduled;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(serviceName);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_ ||
        *joinedFederationName_ != federationName || *joinedFederateId_ != federateId ||
        federateTimeState_ != timeState) {
      throw FederateNotExecutionMember(
          serviceName + L" requires an active joined federate with initialized logical time.");
    }

    if (mode == umbra::detail::FederateTimeAdvanceMode::time_advance_request_available) {
      result = timeState->requestAdvanceAvailable(std::move(requestedTime));
    } else if (mode == umbra::detail::FederateTimeAdvanceMode::next_message_request_available) {
      result = timeState->requestNextMessageAvailableAdvance(
          std::move(requestedTime),
          std::move(effectiveTime));
    } else {
      throw RTIinternalError(L"Umbra received an unsupported available time-advance mode.");
    }

    if (result.status == umbra::detail::FederateTimeAdvanceStatus::applied) {
      retireAmbassadorTsoMessagePayloadsAtRetractionBoundary(
          federationName,
          federateId,
          *timeState);
      scheduled = embeddedFederationRegistry().requestTimeAdvanceGrant(
          federationName,
          federateId,
          result.generation);
    }
  }
  if (reportService && reportTimeArgument &&
      scheduled.status == umbra::detail::FederationTimeGrantStatus::applied) {
    // Acceptance is reportable now; the distinct Time Advance Grant callback
    // later completes the logical-time transition.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        *reportService,
        umbra::detail::MomServiceType::time_management,
        {*reportTimeArgument},
        true);
  }

  switch (result.status) {
    case umbra::detail::FederateTimeAdvanceStatus::applied:
      break;
    case umbra::detail::FederateTimeAdvanceStatus::logical_time_already_passed:
      throw LogicalTimeAlreadyPassed(
          serviceName + L" cannot move a joined federate backward in logical time.");
    case umbra::detail::FederateTimeAdvanceStatus::time_advance_pending:
      throw InTimeAdvancingState(
          serviceName + L" cannot run while a time-advance request is awaiting a grant.");
    case umbra::detail::FederateTimeAdvanceStatus::time_regulation_pending:
      throw RequestForTimeRegulationPending(
          serviceName + L" cannot run while Enable Time Regulation is awaiting its callback.");
    case umbra::detail::FederateTimeAdvanceStatus::time_constrained_pending:
      throw RequestForTimeConstrainedPending(
          serviceName + L" cannot run while Enable Time Constrained is awaiting its callback.");
    case umbra::detail::FederateTimeAdvanceStatus::invalid_logical_time:
      throw InvalidLogicalTime(
          serviceName + L" received a logical time invalid for the joined federation.");
    case umbra::detail::FederateTimeAdvanceStatus::inactive:
      throw FederateNotExecutionMember(
          serviceName + L" found that the joined federate's logical-time state is inactive.");
    case umbra::detail::FederateTimeAdvanceStatus::generation_exhausted:
      throw RTIinternalError(
          L"Umbra exhausted the embedded time-advance request generation space.");
  }

  if (scheduled.status != umbra::detail::FederationTimeGrantStatus::applied) {
    throw RTIinternalError(
        serviceName + L" could not register the accepted request with its federation scheduler.");
  }

  queueAmbassadorJoinedFederateMomConditionalAttributeUpdateForJoinedFederate(
      federationName,
      federateId,
      {umbra::detail::hla::utf8::mom::time_manager_state});

  // The shared grant dispatch delivers any selected queued TSO cohort before
  // the ordinary Time Advance Grant callback. With no selected message, the
  // effective target remains the caller's supplied request.
  flushAmbassadorAsynchronousReceiveCallbacks(timeState);
  submitAmbassadorTimeAdvanceGrantDispatches(std::move(scheduled.dispatches));
}

void UmbraRtiAmbassador::timeAdvanceRequestAvailable(LogicalTime const &time) {
  auto instrumentationScope = beginRtiCall("timeAdvanceRequestAvailable");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    std::wstring federationName;
    std::uint64_t federateId = 0U;
    umbra::detail::ProcessFederationClient *processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Time Advance Request Available requires membership in a federation execution.");
      }
      if (processTimeRegulationCallbackPending_) {
        throw RequestForTimeRegulationPending(
            L"The joined federate has an Enable Time Regulation request awaiting its callback.");
      }
      if (processTimeConstrainedCallbackPending_) {
        throw RequestForTimeConstrainedPending(
            L"The joined federate has an Enable Time Constrained request awaiting its callback.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }

    umbra::detail::ProcessFederationLogicalTime requestedTime;
    requestedTime.implementationName = time.implementationName();
    try {
      auto const encoded = time.encode();
      if (encoded.size() != 0U) {
        auto const *data = static_cast<std::uint8_t const*>(encoded.data());
        if (data == nullptr) {
          throw InvalidLogicalTime(
              L"The requested logical time has no usable encoded value.");
        }
        requestedTime.encoding.assign(data, data + encoded.size());
      }
    } catch (InvalidLogicalTime const&) {
      throw;
    } catch (Exception const&) {
      throw InvalidLogicalTime(
          L"The requested logical time cannot be encoded by the selected implementation.");
    }

    umbra::detail::ProcessFederationTimeAdvanceResult processResult;
    try {
      processResult = processClient->timeAdvanceRequestAvailable(
          std::move(federationName), federateId, std::move(requestedTime));
    } catch (umbra::detail::ProcessFederationServiceProtocolError const &error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const &error) {
      throw RTIinternalError(wideAscii(error.what()));
    }

    using Status = umbra::detail::ProcessFederationTimeAdvanceStatus;
    switch (processResult.status) {
      case Status::applied:
        if (processResult.grantedTime) {
          try {
            processClient->dispatchTimeAdvanceGrant(
                std::move(*processResult.grantedTime));
          } catch (umbra::detail::ProcessFederationCallbackBridgeError const &error) {
            throw RTIinternalError(wideAscii(error.what()));
          }
        }
        return;
      case Status::logical_time_already_passed:
        throw LogicalTimeAlreadyPassed(
            L"Time Advance Request Available cannot move a joined federate backward in logical time.");
      case Status::time_advance_pending:
        throw InTimeAdvancingState(
            L"The joined federate already has a Time Advance Request awaiting a grant.");
      case Status::time_regulation_pending:
        throw RequestForTimeRegulationPending(
            L"The joined federate has an Enable Time Regulation request awaiting its callback.");
      case Status::time_constrained_pending:
        throw RequestForTimeConstrainedPending(
            L"The joined federate has an Enable Time Constrained request awaiting its callback.");
      case Status::invalid_logical_time:
        throw InvalidLogicalTime(
            L"The requested logical time is invalid for the joined federation's implementation.");
      case Status::inactive:
        throw FederateNotExecutionMember(
            L"The joined federate's logical-time state is no longer active.");
      case Status::generation_exhausted:
        throw RTIinternalError(
            L"Umbra exhausted the process time-advance request generation space.");
    }
    throw RTIinternalError(
        L"The process endpoint returned an unknown Time Advance Request Available outcome.");
  }
#endif
  requestAvailableTimeAdvance(
      time,
      umbra::detail::FederateTimeAdvanceMode::time_advance_request_available,
      false,
      L"Time Advance Request Available");
  } catch (Exception const &exception) {
    emitExceptionReport(L"Time Advance Request Available", exception);
    throw;
  }
}

void UmbraRtiAmbassador::flushQueueRequest(LogicalTime const &time) {
  auto instrumentationScope = beginRtiCall("flushQueueRequest");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    requestProcessTimeAdvance(
        time,
        umbra::detail::FederateTimeAdvanceMode::flush_queue_request,
        L"Flush Queue Request");
    return;
  }
#endif
  std::shared_ptr<umbra::detail::FederateTimeState> timeState;
  std::wstring federationName;
  std::uint64_t federateId = 0;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Flush Queue Request");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          L"Flush Queue Request requires an active joined federate with initialized logical time.");
    }

    timeState = federateTimeState_;
    federationName = *joinedFederationName_;
    federateId = *joinedFederateId_;
  }

  auto requestedTime = cloneAmbassadorReferenceLogicalTime(timeState->implementationName(), time);
  auto const reportTimeArgument = umbra::detail::MomServiceArgument{
      umbra::detail::MomArgumentType::logical_time,
      L"Logical time",
      umbra::detail::formatMomLogicalTime(*requestedTime),
  };
  umbra::detail::FederateTimeAdvanceResult result;
  umbra::detail::FederationTimeGrantDispatchResult scheduled;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Flush Queue Request");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_ ||
        *joinedFederationName_ != federationName || *joinedFederateId_ != federateId ||
        federateTimeState_ != timeState) {
      throw FederateNotExecutionMember(
          L"Flush Queue Request requires an active joined federate with initialized logical time.");
    }

    result = timeState->requestFlushQueueAdvance(std::move(requestedTime));
    if (result.status == umbra::detail::FederateTimeAdvanceStatus::applied) {
      retireAmbassadorTsoMessagePayloadsAtRetractionBoundary(
          federationName,
          federateId,
          *timeState);
      scheduled = embeddedFederationRegistry().requestTimeAdvanceGrant(
          federationName,
          federateId,
          result.generation);
    }
  }

  switch (result.status) {
    case umbra::detail::FederateTimeAdvanceStatus::applied:
      break;
    case umbra::detail::FederateTimeAdvanceStatus::logical_time_already_passed:
      throw LogicalTimeAlreadyPassed(
          L"Flush Queue Request cannot move a joined federate backward or below its optimistic logical time.");
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
        L"Umbra could not register the accepted Flush Queue Request with its federation scheduler.");
  }

  queueAmbassadorJoinedFederateMomConditionalAttributeUpdateForJoinedFederate(
      federationName,
      federateId,
      {umbra::detail::hla::utf8::mom::time_manager_state});

  // The successful invocation records its supplied boundary. Flush Queue Grant
  // later reports the actual and optimistic logical times selected at callback
  // delivery, which are separate values under §8.12.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"FlushQueueRequest",
      umbra::detail::MomServiceType::time_management,
      {reportTimeArgument},
      true);

  // Flush Queue Grant dispatch is callback-gated just like Time Advance Grant,
  // but computes the actual and optimistic times at the delivery boundary.
  flushAmbassadorAsynchronousReceiveCallbacks(timeState);
  submitAmbassadorTimeAdvanceGrantDispatches(std::move(scheduled.dispatches));
  } catch (Exception const &exception) {
    emitExceptionReport(L"Flush Queue Request", exception);
    throw;
  }
}

void UmbraRtiAmbassador::queryLogicalTime(LogicalTime &time) {
  auto instrumentationScope = beginRtiCall("queryLogicalTime");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    std::wstring federationName;
    std::uint64_t federateId = 0U;
    umbra::detail::ProcessFederationClient *processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Query Logical Time requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }

    umbra::detail::ProcessFederationLogicalTime processTime;
    try {
      processTime = processClient->queryLogicalTime(
          std::move(federationName), federateId);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const &error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const &error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    auto factory = HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(
        processTime.implementationName);
    if (!factory || factory->getName() != processTime.implementationName) {
      throw RTIinternalError(
          L"The process endpoint returned an unknown logical-time implementation.");
    }
    VariableLengthData encoded;
    if (!processTime.encoding.empty()) {
      encoded.setData(processTime.encoding.data(), processTime.encoding.size());
    }
    std::unique_ptr<LogicalTime> decoded;
    try {
      decoded = factory->decodeLogicalTime(encoded);
    } catch (Exception const&) {
      throw RTIinternalError(
          L"The process endpoint returned an invalid logical-time encoding.");
    }
    if (!decoded || decoded->implementationName() != processTime.implementationName) {
      throw RTIinternalError(
          L"The process endpoint returned an invalid logical-time value.");
    }
      copyAmbassadorQueriedLogicalTime(time, *decoded);
    return;
  }
#endif
  std::shared_ptr<umbra::detail::FederateTimeState> timeState;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Query Logical Time");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          L"Query Logical Time requires a joined federate with initialized logical time.");
    }
    timeState = federateTimeState_;
  }

  auto currentTime = timeState->currentTime();
  if (!currentTime) {
    throw FederateNotExecutionMember(
        L"The joined federate's logical-time state is no longer active.");
  }
  // Do not invoke the caller-provided LogicalTime assignment while holding an
  // Umbra state lock; a custom implementation may execute arbitrary code.
  copyAmbassadorQueriedLogicalTime(time, *currentTime);
  auto const returnedTime = umbra::detail::MomServiceArgument{
      umbra::detail::MomArgumentType::logical_time,
      L"Logical time",
      umbra::detail::formatMomLogicalTime(*currentTime),
  };
  static_cast<void>(emitSelectedMomServiceReportInteraction(
      L"QueryLogicalTime",
      umbra::detail::MomServiceType::time_management,
      {},
      returnedTime));
  } catch (Exception const &exception) {
    emitExceptionReport(L"Query Logical Time", exception);
    throw;
  }
}

}  // namespace rti1516_2025::umbra_binding_detail
