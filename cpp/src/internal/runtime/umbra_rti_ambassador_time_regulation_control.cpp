#include "internal/runtime/umbra_rti_ambassador.hpp"

#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/ambassador_string_helpers.hpp"

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "internal/federation/federation_management_coordinator.hpp"
#include "internal/fom/hla_names.hpp"
#include "internal/time/federate_time_state.hpp"
#endif

#include <memory>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

namespace rti1516_2025::umbra_binding_detail {
void UmbraRtiAmbassador::enableTimeRegulation(LogicalTimeInterval const& lookahead) {
  auto instrumentationScope = beginRtiCall("enableTimeRegulation");
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
            L"Enable Time Regulation requires membership in a federation execution.");
      }
      if (processTimeRegulationCallbackPending_) {
        throw RequestForTimeRegulationPending(
            L"The joined federate already has an Enable Time Regulation request awaiting its callback.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }

    umbra::detail::ProcessFederationLogicalTimeInterval processLookahead;
    processLookahead.implementationName = lookahead.implementationName();
    try {
      auto const encoded = lookahead.encode();
      if (encoded.size() != 0U) {
        auto const* data = static_cast<std::uint8_t const*>(encoded.data());
        if (data == nullptr) {
          throw InvalidLookahead(
              L"The requested lookahead has no usable encoded value.");
        }
        processLookahead.encoding.assign(data, data + encoded.size());
      }
    } catch (InvalidLookahead const&) {
      throw;
    } catch (Exception const&) {
      throw InvalidLookahead(
          L"The requested lookahead cannot be encoded by the selected implementation.");
    }

    umbra::detail::ProcessFederationEnableTimeRegulationResult processResult;
    try {
      processResult = processClient->enableTimeRegulation(
          std::move(federationName), federateId, std::move(processLookahead));
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }

    switch (processResult.status) {
      case umbra::detail::ProcessFederationTimeEnableStatus::applied:
        if (!processResult.enabledTime) {
          throw RTIinternalError(
              L"The process endpoint accepted Enable Time Regulation without a callback time.");
        }
        {
          std::scoped_lock lock(mutex_);
          processTimeRegulationCallbackPending_ = true;
        }
        try {
          processClient->dispatchTimeRegulationEnabled(
              std::move(*processResult.enabledTime));
        } catch (umbra::detail::ProcessFederationCallbackBridgeError const& error) {
          throw RTIinternalError(wideAscii(error.what()));
        }
        return;
      case umbra::detail::ProcessFederationTimeEnableStatus::time_advance_pending:
        throw InTimeAdvancingState(
            L"Enable Time Regulation cannot run while the joined federate has a time advance pending.");
      case umbra::detail::ProcessFederationTimeEnableStatus::request_pending:
        throw RequestForTimeRegulationPending(
            L"The joined federate already has an Enable Time Regulation request awaiting its callback.");
      case umbra::detail::ProcessFederationTimeEnableStatus::already_enabled:
        throw TimeRegulationAlreadyEnabled(
            L"Time regulation is already enabled for the joined federate.");
      case umbra::detail::ProcessFederationTimeEnableStatus::invalid_lookahead:
        throw InvalidLookahead(
            L"The requested lookahead is invalid for the joined federation's implementation.");
      case umbra::detail::ProcessFederationTimeEnableStatus::inactive:
        throw FederateNotExecutionMember(
            L"The joined federate's logical-time state is no longer active.");
      case umbra::detail::ProcessFederationTimeEnableStatus::generation_exhausted:
        throw RTIinternalError(
            L"Umbra exhausted the process temporal-request generation space.");
    }
    throw RTIinternalError(
        L"The process endpoint returned an unknown Enable Time Regulation outcome.");
  }
#endif
  std::shared_ptr<umbra::detail::FederateTimeState> timeState;
  std::shared_ptr<CallbackSession> callbackSession;
  std::wstring federationName;
  std::uint64_t federateId = 0;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Enable Time Regulation");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          L"Enable Time Regulation requires a joined federate with initialized logical time.");
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

  // LogicalTimeInterval is caller-provided polymorphic state. Decode a private
  // reference interval outside Umbra locks, then retain only that copy while
  // the regulation request awaits its official callback.
  auto requestedLookahead = cloneAmbassadorReferenceLogicalTimeInterval(
      timeState->implementationName(),
      lookahead);
  // Section 8.2.1 names the one supplied argument Lookahead, while Table 5
  // gives LogicalTimeInterval the quoted interval.toString() form. Format the
  // private reference copy before locking rather than invoking a caller-owned
  // polymorphic object while the federation state is locked.
  auto const reportLookaheadArgument = umbra::detail::MomServiceArgument{
      umbra::detail::MomArgumentType::logical_time_interval,
      L"Lookahead",
      umbra::detail::formatMomLogicalTimeInterval(*requestedLookahead),
  };
  umbra::detail::FederateTimeEnableResult result;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Enable Time Regulation");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_ ||
        *joinedFederationName_ != federationName || federateTimeState_ != timeState) {
      throw FederateNotExecutionMember(
          L"Enable Time Regulation requires an active joined federate with initialized logical time.");
    }
    result = timeState->requestTimeRegulation(std::move(requestedLookahead));
  }
  if (result.status == umbra::detail::FederateTimeEnableStatus::applied) {
    // Acceptance makes the request reportable. Emit the public report after
    // releasing native locks; the distinct Time Regulation Enabled callback
    // remains the transition that establishes the role.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"EnableTimeRegulation",
        umbra::detail::MomServiceType::time_management,
        {reportLookaheadArgument},
        true);
  }
  switch (result.status) {
    case umbra::detail::FederateTimeEnableStatus::applied:
      break;
    case umbra::detail::FederateTimeEnableStatus::time_advance_pending:
      throw InTimeAdvancingState(
          L"Enable Time Regulation cannot run while the joined federate has a time advance pending.");
    case umbra::detail::FederateTimeEnableStatus::request_pending:
      throw RequestForTimeRegulationPending(
          L"The joined federate already has an Enable Time Regulation request awaiting its callback.");
    case umbra::detail::FederateTimeEnableStatus::already_enabled:
      throw TimeRegulationAlreadyEnabled(
          L"Time regulation is already enabled for the joined federate.");
    case umbra::detail::FederateTimeEnableStatus::invalid_lookahead:
      throw InvalidLookahead(
          L"The requested lookahead is invalid for the joined federation's implementation.");
    case umbra::detail::FederateTimeEnableStatus::inactive:
      throw FederateNotExecutionMember(
          L"The joined federate's logical-time state is no longer active.");
    case umbra::detail::FederateTimeEnableStatus::generation_exhausted:
      throw RTIinternalError(
          L"Umbra exhausted the embedded temporal-request generation space.");
  }

  // This process-local development profile completes the callback at the
  // current logical time. It has bounded local TSO routing, but a
  // transport-bearing coordinator must calculate the complete regulation
  // boundary before this behavior can be reused outside this scope.
  auto const callbackEpoch = timeState->callbackEpoch();
  callbacks_->submit([
      callbackSession = std::move(callbackSession),
      timeState = std::move(timeState),
      federationName = std::move(federationName),
      federateId,
      generation = result.generation,
      callbackEpoch] {
    std::vector<umbra::detail::FederationTimeGrantDispatch> newlyEligible;
    bool granted = false;
    callbackSession->invoke([timeState, federationName, generation, callbackEpoch, &newlyEligible, &granted](FederateAmbassador& recipient) {
      std::shared_ptr<LogicalTime const> enabledTime;
      {
        std::scoped_lock lock(ambassadorFederationManagementMutex());
        enabledTime = timeState->grantTimeRegulationIfCurrent(generation, callbackEpoch);
        if (!enabledTime) {
          return;
        }
        granted = true;
        auto scheduled = embeddedFederationRegistry().reevaluateTimeAdvanceGrants(
            federationName);
        if (scheduled.status == umbra::detail::FederationTimeGrantStatus::applied) {
          newlyEligible = std::move(scheduled.dispatches);
        }
      }
      recipient.timeRegulationEnabled(*enabledTime);
    });
    if (granted) {
      queueAmbassadorJoinedFederateMomConditionalAttributeUpdateForJoinedFederate(
          federationName,
          federateId,
          {umbra::detail::hla::utf8::mom::time_regulating});
    }
    submitAmbassadorTimeAdvanceGrantDispatches(std::move(newlyEligible));
  });
  } catch (Exception const& exception) {
    emitExceptionReport(L"Enable Time Regulation", exception);
    throw;
  }
}

void UmbraRtiAmbassador::disableTimeRegulation() {
  auto instrumentationScope = beginRtiCall("disableTimeRegulation");
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
            L"Disable Time Regulation requires membership in a federation execution.");
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
      processResult = processClient->disableTimeRegulation(
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
        throw TimeRegulationIsNotEnabled(
            L"Time regulation is not enabled for the joined federate.");
      case umbra::detail::ProcessFederationTimeDisableStatus::inactive:
        throw FederateNotExecutionMember(
            L"The joined federate's logical-time state is no longer active.");
    }
    throw RTIinternalError(
        L"The process endpoint returned an unknown Disable Time Regulation outcome.");
  }
#endif
  umbra::detail::FederateTimeDisableStatus result;
  std::vector<umbra::detail::FederationTimeGrantDispatch> newlyEligible;
  std::optional<AmbassadorJoinedFederateMomConditionalWork> momWork;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Disable Time Regulation");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          L"Disable Time Regulation requires a joined federate with initialized logical time.");
    }
    result = federateTimeState_->disableTimeRegulation();
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
          {umbra::detail::hla::utf8::mom::time_regulating});
    }
  }

  if (result == umbra::detail::FederateTimeDisableStatus::applied) {
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"DisableTimeRegulation",
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
      submitAmbassadorTimeAdvanceGrantDispatches(std::move(newlyEligible));
      return;
    case umbra::detail::FederateTimeDisableStatus::not_enabled:
      throw TimeRegulationIsNotEnabled(
          L"Time regulation is not enabled for the joined federate.");
    case umbra::detail::FederateTimeDisableStatus::inactive:
      throw FederateNotExecutionMember(
          L"The joined federate's logical-time state is no longer active.");
  }
  throw RTIinternalError(L"Umbra encountered an unknown time-regulation disable outcome.");
  } catch (Exception const& exception) {
    emitExceptionReport(L"Disable Time Regulation", exception);
    throw;
  }
}

}  // namespace rti1516_2025::umbra_binding_detail
