#include "internal/runtime/umbra_rti_ambassador.hpp"

#include "internal/federation/process_federation_client.hpp"
#include "internal/handles/message_retraction_handle.hpp"
#include "internal/observability/mom_service_report_encoding.hpp"
#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/ambassador_string_helpers.hpp"
#include "internal/runtime/umbra_rti_ambassador_service_failure_translation.hpp"
#include "internal/time/federate_time_state.hpp"

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace rti1516_2025::umbra_binding_detail {

using namespace service_failure_translation;

void UmbraRtiAmbassador::retract(MessageRetractionHandle const& retraction) {
  auto instrumentationScope = beginRtiCall("retract");
  try {
  auto const messageId = messageRetractionHandleValue(retraction);
  if (!messageId) {
    throw InvalidMessageRetractionHandle(
        L"Retract requires a valid MessageRetractionHandle returned by a timestamped service.");
  }
  auto const reportRetractionArgument = umbra::detail::MomServiceArgument{
      umbra::detail::MomArgumentType::message_retraction_handle,
      L"MessageRetractionDesignator",
      umbra::detail::formatMomMessageRetractionHandle(*messageId),
  };

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  // The process profile owns the same joined-federate time-regulation state
  // used by Enable/Disable Time Regulation, while its execution-wide
  // retraction ledger lives in the process service. Route Retract through that
  // service before the embedded-only lower-bound checks below.
  if (processEndpointActive_) {
    std::wstring federationName;
    std::uint64_t producingFederateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Retract requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      producingFederateId = *joinedFederateId_;
    }
    umbra::detail::ProcessFederationRetractResult processResult;
    try {
      processResult = processClient->retract(
          std::move(federationName), producingFederateId, *messageId);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    switch (processResult.status) {
      case umbra::detail::ProcessFederationRetractStatus::applied:
        return;
      case umbra::detail::ProcessFederationRetractStatus::message_no_longer_retractable:
        throw MessageCanNoLongerBeRetracted(
            L"The timestamped message is no longer retractable.");
      case umbra::detail::ProcessFederationRetractStatus::time_regulation_not_enabled:
        throw TimeRegulationIsNotEnabled(
            L"Retract requires time regulation to be enabled for the joined federate.");
      case umbra::detail::ProcessFederationRetractStatus::federate_not_member:
        throw FederateNotExecutionMember(
            L"The process federation no longer records this RTI ambassador as a member.");
      case umbra::detail::ProcessFederationRetractStatus::invalid_handle:
        throw InvalidMessageRetractionHandle(
            L"The MessageRetractionHandle is not owned by this joined federate.");
    }
    throw RTIinternalError(L"The process federation returned an unknown Retract outcome.");
  }
#endif

  std::wstring federationName;
  std::uint64_t producingFederateId = 0;
  std::shared_ptr<umbra::detail::FederateTimeState> timeState;
  std::shared_ptr<LogicalTime const> retractionLowerBound;
  umbra::detail::FederationTsoRetractionResult result;
  std::vector<umbra::detail::FederationTimeGrantDispatch> newlyEligibleTimeGrants;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Retract");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          L"Retract requires membership in a federation execution.");
    }
    federationName = *joinedFederationName_;
    producingFederateId = *joinedFederateId_;
    timeState = federateTimeState_;
    auto const timeSnapshot = timeState->snapshot();
    if (!timeSnapshot.timeRegulating) {
      throw TimeRegulationIsNotEnabled(
          L"Retract requires time regulation to be enabled for the joined federate.");
    }
    retractionLowerBound = makeAmbassadorTsoRetractionLowerBound(timeSnapshot);
    result = embeddedFederationRegistry().retractTsoMessageForProducer(
        federationName,
        producingFederateId,
        *messageId,
        retractionLowerBound);
  }
  if (result.status == umbra::detail::FederationTsoRegistryStatus::federation_does_not_exist ||
      result.status == umbra::detail::FederationTsoRegistryStatus::federate_not_member) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  if (result.status != umbra::detail::FederationTsoRegistryStatus::applied) {
    throw InvalidMessageRetractionHandle(
        L"The MessageRetractionHandle is not owned by this joined federate.");
  }
  if (!result.timestampEligible) {
    throw MessageCanNoLongerBeRetracted(
        L"The timestamped message is not later than the producer's current or requested time plus lookahead.");
  }
  switch (result.queueResult.status) {
    case umbra::detail::TsoMessageQueueStatus::applied:
      // Retraction can remove the last TSO frontier that was blocking another
      // joined federate's pending TARA/NMRA/FQR request. Re-evaluate the
      // federation scheduler now that the queue state is authoritative; the
      // accepted Retract itself must not leave an alternate request stranded.
      {
        auto const reevaluation = embeddedFederationRegistry().reevaluateTimeAdvanceGrants(
            federationName);
        if (reevaluation.status != umbra::detail::FederationTimeGrantStatus::applied) {
          throw RTIinternalError(
              L"Umbra could not re-evaluate pending time-advance requests after Retract.");
        }
        newlyEligibleTimeGrants = reevaluation.dispatches;
      }
      // The accepted Retract invocation records its original designator. Any
      // Request Retraction callbacks are a separate consequence and remain
      // callback-model gated below.
      appendSuccessfulVoidServiceReportToFileIfSelected(
          L"Retract",
          umbra::detail::MomServiceType::time_management,
          {reportRetractionArgument},
          true);
      for (auto const& notification : result.requestRetractionNotifications) {
        queueAmbassadorRequestRetraction(
            notification.callbackRoute,
            federationName,
          notification.receivingFederateId,
          notification.messageId);
      }
      submitAmbassadorTimeAdvanceGrantDispatches(std::move(newlyEligibleTimeGrants));
      return;
    case umbra::detail::TsoMessageQueueStatus::message_already_retracted:
    case umbra::detail::TsoMessageQueueStatus::message_already_delivered:
    case umbra::detail::TsoMessageQueueStatus::message_not_found:
      throw MessageCanNoLongerBeRetracted(
          L"The timestamped message is no longer retractable.");
    case umbra::detail::TsoMessageQueueStatus::invalid_message_id:
    case umbra::detail::TsoMessageQueueStatus::invalid_recipient:
    case umbra::detail::TsoMessageQueueStatus::invalid_timestamp:
    case umbra::detail::TsoMessageQueueStatus::logical_time_implementation_mismatch:
      throw InvalidMessageRetractionHandle(
          L"The MessageRetractionHandle does not identify a valid pending message.");
  }
  throw RTIinternalError(L"Umbra encountered an unknown Retract outcome.");
  } catch (Exception const& exception) {
    emitExceptionReport(L"Retract", exception);
    throw;
  }
}

}  // namespace rti1516_2025::umbra_binding_detail
