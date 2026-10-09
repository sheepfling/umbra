#include "internal/runtime/umbra_rti_ambassador.hpp"
#include "internal/federation/process_federation_client.hpp"
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
#include <utility>
#include <vector>

namespace rti1516_2025::umbra_binding_detail {
using namespace service_failure_translation;

bool UmbraRtiAmbassador::queryGALT(LogicalTime &time) {
  auto instrumentationScope = beginRtiCall("queryGALT");
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
            L"Query GALT requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    auto const bounds = processClient->queryTimeBounds(
        std::move(federationName), federateId);
    using Status = umbra::detail::ProcessFederationTimeBoundStatus;
    if (bounds.status == Status::requesting_federate_not_registered) {
      throw FederateNotExecutionMember(
          L"The process federation no longer records this RTI ambassador's time state.");
    }
    if (bounds.status == Status::factory_unavailable ||
        bounds.status == Status::inconsistent_temporal_state) {
      throw RTIinternalError(
          L"The process federation could not calculate a coherent GALT.");
    }
    if (bounds.status == Status::available) {
      if (!bounds.galt) {
        throw RTIinternalError(
            L"The process federation returned an available GALT without a value.");
      }
      auto galt = decodeAmbassadorProcessLogicalTimeValue(*bounds.galt);
      copyAmbassadorQueriedLogicalTime(time, *galt);
      return true;
    }
    return false;
  }
#endif
  std::uint64_t federateId = 0;
  umbra::detail::FederationTimeExecutionSnapshot snapshot;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Query GALT");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          L"Query GALT requires a joined federate with initialized logical time.");
    }
    auto currentSnapshot = embeddedFederationRegistry().timeSnapshotFor(
        *joinedFederationName_);
    if (!currentSnapshot) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador's time state.");
    }
    federateId = *joinedFederateId_;
    snapshot = std::move(*currentSnapshot);
  }

  auto const bounds = umbra::detail::FederationTimeBoundsCalculator{}.calculate(snapshot, federateId);
  switch (bounds.status) {
    case umbra::detail::FederationTimeBoundStatus::available: {
      if (!bounds.galt) {
        throw RTIinternalError(L"Umbra computed an available GALT without a logical-time value.");
      }
      copyAmbassadorQueriedLogicalTime(time, *bounds.galt);
      auto const returnedGalt = umbra::detail::MomServiceArgument{
          umbra::detail::MomArgumentType::logical_time,
          L"GALT",
          umbra::detail::formatMomLogicalTime(*bounds.galt),
      };
      static_cast<void>(emitSelectedMomServiceReportInteraction(
          L"QueryGALT",
          umbra::detail::MomServiceType::time_management,
          {},
          returnedGalt));
      return true;
    }
    case umbra::detail::FederationTimeBoundStatus::undefined: {
      static_cast<void>(emitSelectedMomServiceReportInteraction(
          L"QueryGALT",
          umbra::detail::MomServiceType::time_management,
          {},
          {umbra::detail::MomArgumentType::null_value, L"", umbra::detail::formatMomNull()}));
      return false;
    }
    case umbra::detail::FederationTimeBoundStatus::requesting_federate_not_registered:
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador's time state.");
    case umbra::detail::FederationTimeBoundStatus::factory_unavailable:
    case umbra::detail::FederationTimeBoundStatus::inconsistent_temporal_state:
      throw RTIinternalError(L"Umbra could not calculate a coherent federation GALT.");
  }
  throw RTIinternalError(L"Umbra encountered an unknown Query GALT outcome.");
  } catch (Exception const &exception) {
    emitExceptionReport(L"Query GALT", exception);
    throw;
  }
}

bool UmbraRtiAmbassador::queryLITS(LogicalTime &time) {
  auto instrumentationScope = beginRtiCall("queryLITS");
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
            L"Query LITS requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    auto const bounds = processClient->queryTimeBounds(
        std::move(federationName), federateId);
    using Status = umbra::detail::ProcessFederationTimeBoundStatus;
    if (bounds.status == Status::requesting_federate_not_registered) {
      throw FederateNotExecutionMember(
          L"The process federation no longer records this RTI ambassador's time state.");
    }
    if (bounds.status == Status::factory_unavailable ||
        bounds.status == Status::inconsistent_temporal_state) {
      throw RTIinternalError(
          L"The process federation could not calculate a coherent LITS.");
    }
    if (bounds.lits) {
      auto lits = decodeAmbassadorProcessLogicalTimeValue(*bounds.lits);
      copyAmbassadorQueriedLogicalTime(time, *lits);
      return true;
    }
    return false;
  }
#endif
  std::uint64_t federateId = 0;
  umbra::detail::FederationTimeExecutionSnapshot snapshot;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Query LITS");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          L"Query LITS requires a joined federate with initialized logical time.");
    }
    auto currentSnapshot = embeddedFederationRegistry().timeSnapshotFor(
        *joinedFederationName_);
    if (!currentSnapshot) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador's time state.");
    }
    federateId = *joinedFederateId_;
    snapshot = std::move(*currentSnapshot);
  }

  auto const bounds = umbra::detail::FederationTimeBoundsCalculator{}.calculate(snapshot, federateId);
  switch (bounds.status) {
    case umbra::detail::FederationTimeBoundStatus::available: {
      if (!bounds.lits) {
        throw RTIinternalError(L"Umbra computed an available LITS without a logical-time value.");
      }
      copyAmbassadorQueriedLogicalTime(time, *bounds.lits);
      auto const returnedLits = umbra::detail::MomServiceArgument{
          umbra::detail::MomArgumentType::logical_time,
          L"LITS",
          umbra::detail::formatMomLogicalTime(*bounds.lits),
      };
      static_cast<void>(emitSelectedMomServiceReportInteraction(
          L"QueryLITS",
          umbra::detail::MomServiceType::time_management,
          {},
          returnedLits));
      return true;
    }
    case umbra::detail::FederationTimeBoundStatus::undefined: {
      if (bounds.lits) {
        copyAmbassadorQueriedLogicalTime(time, *bounds.lits);
        auto const returnedLits = umbra::detail::MomServiceArgument{
            umbra::detail::MomArgumentType::logical_time,
            L"LITS",
            umbra::detail::formatMomLogicalTime(*bounds.lits),
        };
        static_cast<void>(emitSelectedMomServiceReportInteraction(
            L"QueryLITS",
            umbra::detail::MomServiceType::time_management,
            {},
            returnedLits));
        return true;
      }
      static_cast<void>(emitSelectedMomServiceReportInteraction(
          L"QueryLITS",
          umbra::detail::MomServiceType::time_management,
          {},
          {umbra::detail::MomArgumentType::null_value, L"", umbra::detail::formatMomNull()}));
      return false;
    }
    case umbra::detail::FederationTimeBoundStatus::requesting_federate_not_registered:
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador's time state.");
    case umbra::detail::FederationTimeBoundStatus::factory_unavailable:
    case umbra::detail::FederationTimeBoundStatus::inconsistent_temporal_state:
      throw RTIinternalError(L"Umbra could not calculate a coherent federation LITS.");
  }
  throw RTIinternalError(L"Umbra encountered an unknown Query LITS outcome.");
  } catch (Exception const &exception) {
    emitExceptionReport(L"Query LITS", exception);
    throw;
  }
}

void UmbraRtiAmbassador::queryLookahead(LogicalTimeInterval &interval) {
  auto instrumentationScope = beginRtiCall("queryLookahead");
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
            L"Query Lookahead requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }

    umbra::detail::ProcessFederationQueryLookaheadResult processResult;
    try {
      processResult = processClient->queryLookahead(
          std::move(federationName), federateId);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const &error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const &error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    switch (processResult.status) {
      case umbra::detail::ProcessFederationLookaheadStatus::applied:
        if (!processResult.lookahead) {
          throw RTIinternalError(
              L"The process endpoint returned a successful lookahead query without an interval.");
        }
        {
          auto decoded = decodeAmbassadorProcessLogicalTimeIntervalValue(
              *processResult.lookahead);
          copyAmbassadorQueriedLogicalTimeInterval(interval, *decoded);
        }
        return;
      case umbra::detail::ProcessFederationLookaheadStatus::not_enabled:
        throw TimeRegulationIsNotEnabled(
            L"Time regulation is not enabled for the joined federate.");
      case umbra::detail::ProcessFederationLookaheadStatus::inactive:
        throw FederateNotExecutionMember(
            L"The joined federate's logical-time state is no longer active.");
    }
    throw RTIinternalError(
        L"The process endpoint returned an unknown Query Lookahead outcome.");
  }
#endif
  std::shared_ptr<umbra::detail::FederateTimeState> timeState;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Query Lookahead");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          L"Query Lookahead requires a joined federate with initialized logical time.");
    }
    timeState = federateTimeState_;
  }

  auto result = timeState->currentLookahead();
  switch (result.status) {
    case umbra::detail::FederateTimeLookaheadStatus::applied: {
      if (!result.lookahead) {
        throw RTIinternalError(
            L"Umbra received a successful lookahead query without an interval value.");
      }
      copyAmbassadorQueriedLogicalTimeInterval(interval, *result.lookahead);
      auto const returnedLookahead = umbra::detail::MomServiceArgument{
          umbra::detail::MomArgumentType::logical_time_interval,
          L"Lookahead",
          umbra::detail::formatMomLogicalTimeInterval(*result.lookahead),
      };
      static_cast<void>(emitSelectedMomServiceReportInteraction(
          L"QueryLookahead",
          umbra::detail::MomServiceType::time_management,
          {},
          returnedLookahead));
      return;
    }
    case umbra::detail::FederateTimeLookaheadStatus::not_enabled:
      throw TimeRegulationIsNotEnabled(
          L"Time regulation is not enabled for the joined federate.");
    case umbra::detail::FederateTimeLookaheadStatus::inactive:
      throw FederateNotExecutionMember(
          L"The joined federate's logical-time state is no longer active.");
  }
  throw RTIinternalError(L"Umbra encountered an unknown Query Lookahead outcome.");
  } catch (Exception const &exception) {
    emitExceptionReport(L"Query Lookahead", exception);
    throw;
  }
}

void UmbraRtiAmbassador::modifyLookahead(LogicalTimeInterval const &lookahead) {
  auto instrumentationScope = beginRtiCall("modifyLookahead");
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
            L"Modify Lookahead requires membership in a federation execution.");
      }
      if (processTimeRegulationCallbackPending_) {
        throw TimeRegulationIsNotEnabled(
            L"Modify Lookahead requires the Time Regulation Enabled callback to complete.");
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
        auto const *data = static_cast<std::uint8_t const*>(encoded.data());
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

    umbra::detail::ProcessFederationModifyLookaheadResult processResult;
    try {
      processResult = processClient->modifyLookahead(
          std::move(federationName), federateId, std::move(processLookahead));
    } catch (umbra::detail::ProcessFederationServiceProtocolError const &error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const &error) {
      throw RTIinternalError(wideAscii(error.what()));
    }

    using Status = umbra::detail::ProcessFederationModifyLookaheadStatus;
    switch (processResult.status) {
      case Status::applied:
        return;
      case Status::time_advance_pending:
        throw InTimeAdvancingState(
            L"Modify Lookahead cannot run while the joined federate has a time advance pending.");
      case Status::not_enabled:
        throw TimeRegulationIsNotEnabled(
            L"Modify Lookahead requires time regulation to be enabled for the joined federate.");
      case Status::inactive:
        throw FederateNotExecutionMember(
            L"The joined federate's logical-time state is no longer active.");
      case Status::invalid_lookahead:
        throw InvalidLookahead(
            L"The requested lookahead is invalid for the joined federation's implementation.");
    }
    throw RTIinternalError(
        L"The process endpoint returned an unknown Modify Lookahead outcome.");
  }
#endif
  std::shared_ptr<umbra::detail::FederateTimeState> timeState;
  std::wstring federationName;
  std::uint64_t federateId = 0;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Modify Lookahead");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          L"Modify Lookahead requires a joined federate with initialized logical time.");
    }
    timeState = federateTimeState_;
    federationName = *joinedFederationName_;
    federateId = *joinedFederateId_;
  }

 auto requestedLookahead = cloneAmbassadorReferenceLogicalTimeInterval(
     timeState->implementationName(), lookahead);
  // Section 8.20.1 identifies this supplied value as Requested lookahead.
  // Table 5 gives LogicalTimeInterval the quoted interval.toString() form;
  // obtain it from the private reference copy outside federation locks.
  auto const reportLookaheadArgument = umbra::detail::MomServiceArgument{
      umbra::detail::MomArgumentType::logical_time_interval,
      L"Requested lookahead",
      umbra::detail::formatMomLogicalTimeInterval(*requestedLookahead),
  };
 umbra::detail::FederateTimeModifyLookaheadStatus result;
  std::vector<umbra::detail::FederationTimeGrantDispatch> newlyEligible;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Modify Lookahead");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_ ||
        *joinedFederationName_ != federationName || *joinedFederateId_ != federateId ||
        federateTimeState_ != timeState) {
      throw FederateNotExecutionMember(
          L"Modify Lookahead requires an active joined federate with initialized logical time.");
   }
   result = timeState->modifyLookahead(std::move(requestedLookahead));
   if (result == umbra::detail::FederateTimeModifyLookaheadStatus::applied) {
     retireAmbassadorTsoMessagePayloadsAtRetractionBoundary(
         federationName,
          federateId,
          *timeState);
      auto scheduled = embeddedFederationRegistry().reevaluateTimeAdvanceGrants(
          federationName);
      if (scheduled.status == umbra::detail::FederationTimeGrantStatus::applied) {
        newlyEligible = std::move(scheduled.dispatches);
      }
    }
  }

  if (result == umbra::detail::FederateTimeModifyLookaheadStatus::applied) {
    // A lower lookahead remains a pending state transition, but this
    // successful service invocation is reportable immediately.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"ModifyLookahead",
        umbra::detail::MomServiceType::time_management,
        {reportLookaheadArgument},
        true);
  }

  switch (result) {
    case umbra::detail::FederateTimeModifyLookaheadStatus::applied:
      submitAmbassadorTimeAdvanceGrantDispatches(std::move(newlyEligible));
      return;
    case umbra::detail::FederateTimeModifyLookaheadStatus::time_advance_pending:
      throw InTimeAdvancingState(
          L"Modify Lookahead cannot run while the joined federate has a time advance pending.");
    case umbra::detail::FederateTimeModifyLookaheadStatus::not_enabled:
      throw TimeRegulationIsNotEnabled(
          L"Modify Lookahead requires time regulation to be enabled for the joined federate.");
    case umbra::detail::FederateTimeModifyLookaheadStatus::inactive:
      throw FederateNotExecutionMember(
          L"The joined federate's logical-time state is no longer active.");
  }
  throw RTIinternalError(L"Umbra encountered an unknown Modify Lookahead outcome.");
  } catch (Exception const &exception) {
    emitExceptionReport(L"Modify Lookahead", exception);
    throw;
  }
}

}  // namespace rti1516_2025::umbra_binding_detail
