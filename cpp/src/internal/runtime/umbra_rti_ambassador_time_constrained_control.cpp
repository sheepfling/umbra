#include "internal/runtime/umbra_rti_ambassador.hpp"

#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/ambassador_string_helpers.hpp"

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "internal/federation/federation_management_coordinator.hpp"
#include "internal/fom/hla_names.hpp"
#endif

#include <mutex>
#include <optional>
#include <utility>
#include <vector>

namespace rti1516_2025::umbra_binding_detail {
void UmbraRtiAmbassador::enableTimeConstrained() {
  auto instrumentationScope = beginRtiCall("enableTimeConstrained");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    std::wstring federationName;
    std::uint64_t federateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    {
      std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Enable Time Constrained requires membership in a federation execution.");
      }
      if (processTimeConstrainedCallbackPending_) {
        throw RequestForTimeConstrainedPending(
            L"The joined federate already has an Enable Time Constrained request awaiting its callback.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }

    umbra::detail::ProcessFederationEnableTimeConstrainedResult processResult;
    try {
      processResult = processClient->enableTimeConstrained(
          std::move(federationName), federateId);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }

    switch (processResult.status) {
      case umbra::detail::ProcessFederationTimeEnableStatus::applied:
        if (!processResult.enabledTime) {
          throw RTIinternalError(
              L"The process endpoint accepted Enable Time Constrained without a callback time.");
        }
        {
          std::scoped_lock lock(mutex_);
          processTimeConstrainedCallbackPending_ = true;
        }
        try {
          processClient->dispatchTimeConstrainedEnabled(
              std::move(*processResult.enabledTime));
        } catch (umbra::detail::ProcessFederationCallbackBridgeError const& error) {
          throw RTIinternalError(wideAscii(error.what()));
        }
        return;
      case umbra::detail::ProcessFederationTimeEnableStatus::time_advance_pending:
        throw InTimeAdvancingState(
            L"Enable Time Constrained cannot run while the joined federate has a time advance pending.");
      case umbra::detail::ProcessFederationTimeEnableStatus::request_pending:
        throw RequestForTimeConstrainedPending(
            L"The joined federate already has an Enable Time Constrained request awaiting its callback.");
      case umbra::detail::ProcessFederationTimeEnableStatus::already_enabled:
        throw TimeConstrainedAlreadyEnabled(
            L"Time constrained is already enabled for the joined federate.");
      case umbra::detail::ProcessFederationTimeEnableStatus::invalid_lookahead:
        throw RTIinternalError(
            L"The process time-constrained request returned an unexpected lookahead failure.");
      case umbra::detail::ProcessFederationTimeEnableStatus::inactive:
        throw FederateNotExecutionMember(
            L"The joined federate's logical-time state is no longer active.");
      case umbra::detail::ProcessFederationTimeEnableStatus::generation_exhausted:
        throw RTIinternalError(
            L"Umbra exhausted the process temporal-request generation space.");
    }
    throw RTIinternalError(
        L"The process endpoint returned an unknown Enable Time Constrained outcome.");
  }
#endif
  std::shared_ptr<umbra::detail::FederateTimeState> timeState;
  std::shared_ptr<CallbackSession> callbackSession;
  std::wstring federationName;
  std::uint64_t federateId = 0;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Enable Time Constrained");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          L"Enable Time Constrained requires a joined federate with initialized logical time.");
    }

    timeState = federateTimeState_;
    callbackSession = callbackSession_;
    federationName = *joinedFederationName_;
    federateId = *joinedFederateId_;
    if (!callbackSession) {
      throw RTIinternalError(
          L"The embedded connection has no federate ambassador callback recipient.");
    }
  }

  umbra::detail::FederateTimeEnableResult result;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Enable Time Constrained");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_ ||
        federateTimeState_ != timeState) {
      throw FederateNotExecutionMember(
          L"Enable Time Constrained requires an active joined federate with initialized logical time.");
    }
    result = timeState->requestTimeConstrained();
  }
  if (result.status == umbra::detail::FederateTimeEnableStatus::applied) {
    // The service invocation succeeds when the request is accepted; emit the
    // public report after releasing native locks and before the distinct Time
    // Constrained Enabled callback is queued below.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"EnableTimeConstrained",
        umbra::detail::MomServiceType::time_management,
        {},
        true);
  }
  switch (result.status) {
    case umbra::detail::FederateTimeEnableStatus::applied:
      break;
    case umbra::detail::FederateTimeEnableStatus::time_advance_pending:
      throw InTimeAdvancingState(
          L"Enable Time Constrained cannot run while the joined federate has a time advance pending.");
    case umbra::detail::FederateTimeEnableStatus::request_pending:
      throw RequestForTimeConstrainedPending(
          L"The joined federate already has an Enable Time Constrained request awaiting its callback.");
    case umbra::detail::FederateTimeEnableStatus::already_enabled:
      throw TimeConstrainedAlreadyEnabled(
          L"Time constrained is already enabled for the joined federate.");
    case umbra::detail::FederateTimeEnableStatus::invalid_lookahead:
      throw RTIinternalError(
          L"The embedded time-constrained request returned an unexpected lookahead failure.");
    case umbra::detail::FederateTimeEnableStatus::inactive:
      throw FederateNotExecutionMember(
          L"The joined federate's logical-time state is no longer active.");
    case umbra::detail::FederateTimeEnableStatus::generation_exhausted:
      throw RTIinternalError(
          L"Umbra exhausted the embedded temporal-request generation space.");
  }

  auto const callbackEpoch = timeState->callbackEpoch();
  callbacks_->submit([
      callbackSession = std::move(callbackSession),
      timeState = std::move(timeState),
      federationName = std::move(federationName),
      federateId,
      generation = result.generation,
      callbackEpoch] {
    bool granted = false;
    callbackSession->invoke([timeState, generation, callbackEpoch, &granted](FederateAmbassador& recipient) {
      auto enabledTime = timeState->grantTimeConstrainedIfCurrent(generation, callbackEpoch);
      if (!enabledTime) {
        return;
      }
      granted = true;
      recipient.timeConstrainedEnabled(*enabledTime);
    });
    if (granted) {
      queueAmbassadorJoinedFederateMomConditionalAttributeUpdateForJoinedFederate(
          federationName,
          federateId,
          {umbra::detail::hla::utf8::mom::time_constrained});
    }
  });
  } catch (Exception const& exception) {
    emitExceptionReport(L"Enable Time Constrained", exception);
    throw;
  }
}

void UmbraRtiAmbassador::disableTimeConstrained() {
  auto instrumentationScope = beginRtiCall("disableTimeConstrained");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    std::wstring federationName;
    std::uint64_t federateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    {
      std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Disable Time Constrained requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }

    umbra::detail::ProcessFederationTimeDisableResult processResult;
    try {
      processResult = processClient->disableTimeConstrained(
          std::move(federationName), federateId);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    switch (processResult.status) {
      case umbra::detail::ProcessFederationTimeDisableStatus::applied:
        return;
      case umbra::detail::ProcessFederationTimeDisableStatus::not_enabled:
        throw TimeConstrainedIsNotEnabled(
            L"Time constrained is not enabled for the joined federate.");
      case umbra::detail::ProcessFederationTimeDisableStatus::inactive:
        throw FederateNotExecutionMember(
            L"The joined federate's logical-time state is no longer active.");
    }
    throw RTIinternalError(
        L"The process endpoint returned an unknown Disable Time Constrained outcome.");
  }
#endif
  umbra::detail::FederateTimeDisableStatus result;
  std::shared_ptr<umbra::detail::FederateTimeState> timeState;
  std::vector<umbra::detail::FederationTimeGrantDispatch> newlyEligible;
  std::optional<AmbassadorJoinedFederateMomConditionalWork> momWork;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Disable Time Constrained");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          L"Disable Time Constrained requires a joined federate with initialized logical time.");
    }
    timeState = federateTimeState_;
    result = timeState->disableTimeConstrained();
    if (result == umbra::detail::FederateTimeDisableStatus::applied) {
      auto scheduled = embeddedFederationRegistry().reevaluateTimeAdvanceGrants(
          *joinedFederationName_);
      if (scheduled.status == umbra::detail::FederationTimeGrantStatus::applied) {
        newlyEligible = std::move(scheduled.dispatches);
      }
      momWork = ambassadorJoinedFederateMomConditionalWorkFor(
          embeddedFederationRegistry(),
          *joinedFederationName_,
          *joinedFederateId_,
          {umbra::detail::hla::utf8::mom::time_constrained});
    }
  }

  if (result == umbra::detail::FederateTimeDisableStatus::applied) {
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"DisableTimeConstrained",
        umbra::detail::MomServiceType::time_management,
        {},
        true);
  }

  if (momWork) {
    queueAmbassadorJoinedFederateMomConditionalAttributeUpdate(
        momWork->federationName,
        momWork->objectInstanceHandle,
        std::move(momWork->attributeHandles));
  }

  switch (result) {
    case umbra::detail::FederateTimeDisableStatus::applied:
      // A previously constrained TAR may no longer be GALT-bounded. Queue
      // any resulting grant only after the shared runtime locks are released.
      flushAmbassadorAsynchronousReceiveCallbacks(timeState);
      submitAmbassadorTimeAdvanceGrantDispatches(std::move(newlyEligible));
      return;
    case umbra::detail::FederateTimeDisableStatus::not_enabled:
      throw TimeConstrainedIsNotEnabled(
          L"Time constrained is not enabled for the joined federate.");
    case umbra::detail::FederateTimeDisableStatus::inactive:
      throw FederateNotExecutionMember(
          L"The joined federate's logical-time state is no longer active.");
  }
  throw RTIinternalError(L"Umbra encountered an unknown time-constrained disable outcome.");
  } catch (Exception const& exception) {
    emitExceptionReport(L"Disable Time Constrained", exception);
    throw;
  }
}

}  // namespace rti1516_2025::umbra_binding_detail
