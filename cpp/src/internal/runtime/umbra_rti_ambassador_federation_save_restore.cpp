#include "internal/runtime/umbra_rti_ambassador.hpp"
#include "internal/runtime/ambassador_string_helpers.hpp"
#include "internal/runtime/ambassador_shared_utilities.hpp"

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "internal/encoding/variable_length_data_2025.hpp"
#include "internal/fom/hla_names.hpp"
#include "internal/observability/mom_service_report_encoding.hpp"
#include "internal/time/federation_time_bounds.hpp"
#endif

#include <algorithm>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace rti1516_2025::umbra_binding_detail {

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
namespace {
enum class FederationSaveServiceFailure {
  request,
  begun,
  completion,
  abort,
  query,
};

[[noreturn]] void throwFederationSaveServiceFailure(
    umbra::detail::FederationSaveControlStatus status,
    std::wstring const& operation,
    FederationSaveServiceFailure failure) {
  using Status = umbra::detail::FederationSaveControlStatus;
  switch (status) {
    case Status::federation_does_not_exist:
    case Status::federate_not_member:
      throw FederateNotExecutionMember(
          operation + L" requires membership in an active federation execution.");
    case Status::callback_route_missing:
      throw RTIinternalError(
          operation + L" could not find the joined federate's callback route.");
    case Status::invalid_timed_save:
      throw RTIinternalError(
          operation + L" could not commit a coherent timestamped save request.");
    case Status::inconsistent_temporal_state:
      throw RTIinternalError(
          operation + L" could not observe a coherent federation time state.");
    case Status::save_in_progress:
      if (failure == FederationSaveServiceFailure::request) {
        throw SaveInProgress(operation + L" cannot overlap an existing federation save.");
      }
      break;
    case Status::restore_in_progress:
      throw RestoreInProgress(operation + L" cannot overlap an active federation restore.");
    case Status::save_not_initiated:
      if (failure == FederationSaveServiceFailure::begun) {
        throw SaveNotInitiated(
            operation + L" was not preceded by an Initiate Federate Save callback.");
      }
      break;
    case Status::federate_has_not_begun_save:
      if (failure == FederationSaveServiceFailure::completion) {
        throw FederateHasNotBegunSave(
            operation + L" requires the federate to report Save Begun first.");
      }
      break;
    case Status::save_not_in_progress:
      if (failure == FederationSaveServiceFailure::abort) {
        throw SaveNotInProgress(operation + L" has no federation save to abort.");
      }
      break;
    case Status::applied:
      break;
  }
  throw RTIinternalError(operation + L" encountered an unknown save-control outcome.");
}


enum class FederationRestoreServiceFailure {
  request,
  completion,
  abort,
  query,
};

[[noreturn]] void throwFederationRestoreServiceFailure(
    umbra::detail::FederationRestoreControlStatus status,
    std::wstring const& operation,
    FederationRestoreServiceFailure failure) {
  using Status = umbra::detail::FederationRestoreControlStatus;
  switch (status) {
    case Status::federation_does_not_exist:
    case Status::federate_not_member:
      throw FederateNotExecutionMember(
          operation + L" requires membership in an active federation execution.");
    case Status::save_in_progress:
      throw SaveInProgress(operation + L" cannot overlap an active federation save.");
    case Status::restore_in_progress:
      if (failure == FederationRestoreServiceFailure::request) {
        throw RestoreInProgress(operation + L" cannot overlap an active federation restore.");
      }
      break;
    case Status::restore_not_requested:
      if (failure == FederationRestoreServiceFailure::completion) {
        throw RestoreNotRequested(
            operation + L" requires an accepted federation restore request.");
      }
      break;
    case Status::restore_not_in_progress:
      if (failure == FederationRestoreServiceFailure::abort) {
        throw RestoreNotInProgress(operation + L" has no federation restore to abort.");
      }
      break;
    case Status::callback_route_missing:
      throw RTIinternalError(
          operation + L" could not find the joined federate's callback route.");
    case Status::snapshot_not_found:
    case Status::membership_mismatch:
    case Status::applied:
      break;
  }
  throw RTIinternalError(operation + L" encountered an unknown restore-control outcome.");
}


}  // namespace
#endif
void UmbraRtiAmbassador::requestFederationSave(std::wstring const& label) {
  auto instrumentationScope = beginRtiCall("requestFederationSave");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    std::wstring federationName;
    std::uint64_t federateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Request Federation Save requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (!processClient) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    try {
      auto const result = processClient->requestFederationSave(
          federationName, federateId, label);
      if (result.status !=
          umbra::detail::FederationSaveControlStatus::applied) {
        throwFederationSaveServiceFailure(
            result.status,
            L"Request Federation Save",
            FederationSaveServiceFailure::request);
      }
      if (joinedServiceReport_) {
        appendSuccessfulVoidServiceReportToFileIfSelected(
            L"RequestFederationSave",
            umbra::detail::MomServiceType::federation_management,
            {{umbra::detail::MomArgumentType::string,
              L"Federation save label",
              umbra::detail::formatMomString(label)},
             {umbra::detail::MomArgumentType::null_value,
              L"Optional timestamp",
              umbra::detail::formatMomNull()}},
            true);
      }
      processClient->dispatchPendingPushedEvents();
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationCallbackBridgeError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif
  std::vector<umbra::detail::FederationSaveNotification> notifications;
  std::wstring federationName;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Request Federation Save requires membership in a federation execution.");
    }

    // Capture the joined-federate identity while lifecycle state is locked.
    // Notifications and MOM updates are dispatched after unlocking and must
    // not dereference mutable join state during that interval.
    federationName = *joinedFederationName_;
    auto result = embeddedFederationRegistry().requestFederationSave(
        *joinedFederationName_,
        *joinedFederateId_,
        label);
    if (result.status != umbra::detail::FederationSaveControlStatus::applied) {
      throwFederationSaveServiceFailure(
          result.status,
          L"Request Federation Save",
          FederationSaveServiceFailure::request);
    }
    notifications = std::move(result.notifications);
  }
  // The official C++ binding exposes §4.19's optional timestamp through two
  // overloads. This untimed entry point retains that slot as Null and emits
  // the public interaction only after releasing native locks, because an
  // immediate observer may synchronously enter Java/Python.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"RequestFederationSave",
      umbra::detail::MomServiceType::federation_management,
      {{umbra::detail::MomArgumentType::string,
        L"Federation save label",
        umbra::detail::formatMomString(label)},
       {umbra::detail::MomArgumentType::null_value,
        L"Optional timestamp",
        umbra::detail::formatMomNull()}},
      true);
  submitAmbassadorFederationSaveNotifications(
      federationName,
      std::move(notifications));
  queueAmbassadorFederationMomConditionalAttributeUpdate(
      federationName,
      {umbra::detail::hla::utf8::mom::next_save_name, umbra::detail::hla::utf8::mom::next_save_time});
  } catch (Exception const& exception) {
    emitExceptionReport(L"Request Federation Save", exception);
    throw;
  }
}

void UmbraRtiAmbassador::requestFederationSave(
    std::wstring const& label,
    LogicalTime const& time) {
  auto instrumentationScope = beginRtiCall("requestFederationSave");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    std::wstring federationName;
    std::uint64_t federateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    std::wstring processImplementationName;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_ ||
          !processLogicalTimeImplementationName_) {
        throw FederateNotExecutionMember(
            L"Request Federation Save with a timestamp requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (!processClient) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
      processImplementationName = *processLogicalTimeImplementationName_;
    }

    try {
      auto timestamp = cloneAmbassadorReferenceLogicalTime(processImplementationName, time);
      if (timestamp->isInitial() || timestamp->isFinal()) {
        throw InvalidLogicalTime(
            L"A timestamped service requires a finite logical timestamp.");
      }
      auto const result = processClient->requestFederationSave(
          federationName,
          federateId,
          label,
          umbra::detail::ProcessFederationLogicalTime{
              timestamp->implementationName(),
              umbra::detail::variable_length_data_2025::copyBytes(timestamp->encode())});
      if (result.status !=
          umbra::detail::FederationSaveControlStatus::applied) {
        throwFederationSaveServiceFailure(
            result.status,
            L"Request Federation Save",
            FederationSaveServiceFailure::request);
      }
      if (joinedServiceReport_) {
        appendSuccessfulVoidServiceReportToFileIfSelected(
            L"RequestFederationSave",
            umbra::detail::MomServiceType::federation_management,
            {{umbra::detail::MomArgumentType::string,
              L"Federation save label",
              umbra::detail::formatMomString(label)},
             {umbra::detail::MomArgumentType::logical_time,
              L"Optional timestamp",
              umbra::detail::formatMomLogicalTime(*timestamp)}},
            true);
      }
      processClient->dispatchPendingPushedEvents();
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationCallbackBridgeError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif
  std::shared_ptr<umbra::detail::FederateTimeState> timeState;
  std::wstring federationName;
  std::uint64_t federateId = 0;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          L"Request Federation Save with a timestamp requires membership in a federation execution.");
    }
    timeState = federateTimeState_;
    federationName = *joinedFederationName_;
    federateId = *joinedFederateId_;
  }

  // Decode the caller's polymorphic LogicalTime before taking runtime locks.
  // This both fences the selected IEEE implementation and gives the public
  // InvalidLogicalTime mapping for malformed or initial/final values.
  auto timestamp = cloneAmbassadorReferenceLogicalTime(timeState->implementationName(), time);
  validateAmbassadorTsoTimestamp(timeState->snapshot(), *timestamp);
  // Table 5 gives LogicalTime the quoted value from time.toString().  Format
  // the private clone before locking, then commit that exact supplied
  // timestamp only if §4.19 accepts the request below.
  auto const reportTimestampArgument = umbra::detail::MomServiceArgument{
      umbra::detail::MomArgumentType::logical_time,
      L"Optional timestamp",
      umbra::detail::formatMomLogicalTime(*timestamp),
  };

  umbra::detail::FederationTimeExecutionSnapshot execution;
  {
    std::scoped_lock lock(ambassadorFederationManagementMutex());
    auto snapshot = embeddedFederationRegistry().timeSnapshotFor(
        federationName);
    if (!snapshot) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    execution = std::move(*snapshot);
  }

  auto const requester = std::find_if(
      execution.federates.begin(),
      execution.federates.end(),
      [federateId](umbra::detail::FederationTimeFederateSnapshot const& candidate) {
        return candidate.membership.id == federateId;
      });
  if (requester == execution.federates.end()) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  if (requester->time.implementationName != timeState->implementationName() ||
      execution.definition.logicalTimeImplementationName != timeState->implementationName()) {
    throw RTIinternalError(
        L"The joined federation has inconsistent logical-time implementation state.");
  }

  auto equalOrEarlier = [](LogicalTime const& left, LogicalTime const& right) {
    try {
      return left <= right;
    } catch (Exception const&) {
      throw InvalidLogicalTime(
          L"The timestamped federation-save request cannot be compared in the selected implementation.");
    }
  };

  if (!requester->time.timeRegulating) {
    auto const bounds = umbra::detail::FederationTimeBoundsCalculator{}.calculate(
        execution,
        federateId);
    switch (bounds.status) {
      case umbra::detail::FederationTimeBoundStatus::available:
        if (!bounds.galt) {
          throw RTIinternalError(
              L"Umbra computed an available GALT without a logical-time value.");
        }
        if (equalOrEarlier(*timestamp, *bounds.galt)) {
          throw LogicalTimeAlreadyPassed(
              L"The timestamped federation-save request must be later than the joined federate's GALT.");
        }
        break;
      case umbra::detail::FederationTimeBoundStatus::undefined:
        throw FederateUnableToUseTime(
            L"A non-regulating federate may request a timestamped save only when GALT is defined.");
      case umbra::detail::FederationTimeBoundStatus::requesting_federate_not_registered:
        throw FederateNotExecutionMember(
            L"The embedded federation no longer records this RTI ambassador as a member.");
      case umbra::detail::FederationTimeBoundStatus::factory_unavailable:
      case umbra::detail::FederationTimeBoundStatus::inconsistent_temporal_state:
        throw RTIinternalError(
            L"Umbra could not calculate a coherent GALT for the timestamped federation-save request.");
    }
  } else if (requester->time.currentTime &&
             equalOrEarlier(*timestamp, *requester->time.currentTime)) {
    throw LogicalTimeAlreadyPassed(
        L"The timestamped federation-save request is not later than the regulating federate's current time.");
  }

  // A timestamped request must be strictly beyond every current position of a
  // constrained member.  The registry repeats this check at commit time so a
  // concurrent grant/resign cannot turn an admitted request into an invalid
  // one.
  for (auto const& federate : execution.federates) {
    if (!federate.time.timeConstrained) {
      continue;
    }
    if (!federate.time.currentTime) {
      throw RTIinternalError(
          L"A time-constrained federate has no current logical time for timed-save validation.");
    }
    if (equalOrEarlier(*timestamp, *federate.time.currentTime)) {
      throw LogicalTimeAlreadyPassed(
          L"The timestamped federation-save request is not later than every time-constrained federate.");
    }
  }

  std::vector<umbra::detail::FederationSaveNotification> notifications;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_ ||
        *joinedFederationName_ != federationName || *joinedFederateId_ != federateId ||
        federateTimeState_ != timeState) {
      throw FederateNotExecutionMember(
          L"Request Federation Save with a timestamp requires an active joined federate.");
    }

    auto result = embeddedFederationRegistry().requestFederationSave(
        federationName,
        federateId,
        label,
        std::move(timestamp));
    if (result.status != umbra::detail::FederationSaveControlStatus::applied) {
      throwFederationSaveServiceFailure(
          result.status,
          L"Request Federation Save",
          FederationSaveServiceFailure::request);
    }
    notifications = std::move(result.notifications);
  }
  // This is the supplied-timestamp counterpart to the overload above. A
  // successful request records the admitted §4.19 invocation after releasing
  // native locks and before time-bound Initiate Federate Save callbacks.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"RequestFederationSave",
      umbra::detail::MomServiceType::federation_management,
      {{umbra::detail::MomArgumentType::string,
        L"Federation save label",
        umbra::detail::formatMomString(label)},
       reportTimestampArgument},
      true);
  submitAmbassadorFederationSaveNotifications(
      federationName,
      std::move(notifications));
  queueAmbassadorFederationMomConditionalAttributeUpdate(
      federationName,
      {umbra::detail::hla::utf8::mom::next_save_name, umbra::detail::hla::utf8::mom::next_save_time});
  } catch (Exception const& exception) {
    emitExceptionReport(L"Request Federation Save", exception);
    throw;
  }
}

void UmbraRtiAmbassador::federateSaveBegun() {
  auto instrumentationScope = beginRtiCall("federateSaveBegun");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    std::wstring federationName;
    std::uint64_t federateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Federate Save Begun requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (!processClient) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    try {
      auto const result = processClient->federateSaveBegun(
          federationName, federateId);
      if (result.status !=
          umbra::detail::FederationSaveControlStatus::applied) {
        throwFederationSaveServiceFailure(
            result.status,
            L"Federate Save Begun",
            FederationSaveServiceFailure::begun);
      }
      if (joinedServiceReport_) {
        appendSuccessfulVoidServiceReportToFileIfSelected(
            L"FederateSaveBegun",
            umbra::detail::MomServiceType::federation_management,
            {},
            true);
      }
      processClient->dispatchPendingPushedEvents();
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationCallbackBridgeError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif
  std::wstring federationName;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Federate Save Begun requires membership in a federation execution.");
    }

    federationName = *joinedFederationName_;
    auto result = embeddedFederationRegistry().federateSaveBegun(
        federationName,
        *joinedFederateId_);
    if (result.status != umbra::detail::FederationSaveControlStatus::applied) {
      throwFederationSaveServiceFailure(
          result.status,
          L"Federate Save Begun",
          FederationSaveServiceFailure::begun);
    }
  }
  // Section 4.21 has no supplied or returned arguments. Its accepted state
  // transition is therefore the report boundary; save completion/failure and
  // the later Federation Saved callback remain separate service boundaries.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"FederateSaveBegun",
      umbra::detail::MomServiceType::federation_management,
      {},
      true);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Federate Save Begun", exception);
    throw;
  }
}

void UmbraRtiAmbassador::federateSaveComplete() {
  auto instrumentationScope = beginRtiCall("federateSaveComplete");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    std::wstring federationName;
    std::uint64_t federateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Federate Save Complete requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (!processClient) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    try {
      auto const result = processClient->federateSaveComplete(
          federationName, federateId);
      if (result.status !=
          umbra::detail::FederationSaveControlStatus::applied) {
        throwFederationSaveServiceFailure(
            result.status,
            L"Federate Save Complete",
            FederationSaveServiceFailure::completion);
      }
      if (joinedServiceReport_) {
        appendSuccessfulVoidServiceReportToFileIfSelected(
            L"FederateSaveComplete",
            umbra::detail::MomServiceType::federation_management,
            {{umbra::detail::MomArgumentType::boolean,
              L"Federate save-success indicator",
              umbra::detail::formatMomBoolean(true)}},
            true);
      }
      processClient->dispatchPendingPushedEvents();
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationCallbackBridgeError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif
  std::vector<umbra::detail::FederationSaveNotification> notifications;
  std::wstring federationName;
  bool saveCompletedSuccessfully = false;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Federate Save Complete requires membership in a federation execution.");
    }

    federationName = *joinedFederationName_;
    auto result = embeddedFederationRegistry().federateSaveComplete(
        federationName,
        *joinedFederateId_);
    if (result.status != umbra::detail::FederationSaveControlStatus::applied) {
      throwFederationSaveServiceFailure(
          result.status,
          L"Federate Save Complete",
          FederationSaveServiceFailure::completion);
    }
    saveCompletedSuccessfully = result.saveCompletedSuccessfully;
    notifications = std::move(result.notifications);
  }
  // The official C++ binding represents the §4.22 success selector through
  // separate Complete/Not Complete calls. Both are the one Federate Save
  // Complete service report, distinguished by its required Boolean argument.
  // Emit it after native locks are released and before Federation Saved work.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"FederateSaveComplete",
      umbra::detail::MomServiceType::federation_management,
      {{umbra::detail::MomArgumentType::boolean,
        L"Federate save-success indicator",
        umbra::detail::formatMomBoolean(true)}},
      true);
  submitAmbassadorFederationSaveNotifications(
      federationName,
      std::move(notifications));
  if (saveCompletedSuccessfully) {
    queueAmbassadorFederationMomConditionalAttributeUpdate(
        federationName,
        {umbra::detail::hla::utf8::mom::last_save_name, umbra::detail::hla::utf8::mom::last_save_time});
  }
  } catch (Exception const& exception) {
    emitExceptionReport(L"Federate Save Complete", exception);
    throw;
  }
}

void UmbraRtiAmbassador::federateSaveNotComplete() {
  auto instrumentationScope = beginRtiCall("federateSaveNotComplete");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    std::wstring federationName;
    std::uint64_t federateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Federate Save Not Complete requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (!processClient) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    try {
      auto const result = processClient->federateSaveNotComplete(
          federationName, federateId);
      if (result.status !=
          umbra::detail::FederationSaveControlStatus::applied) {
        throwFederationSaveServiceFailure(
            result.status,
            L"Federate Save Not Complete",
            FederationSaveServiceFailure::completion);
      }
      if (joinedServiceReport_) {
        appendSuccessfulVoidServiceReportToFileIfSelected(
            L"FederateSaveComplete",
            umbra::detail::MomServiceType::federation_management,
            {{umbra::detail::MomArgumentType::boolean,
              L"Federate save-success indicator",
              umbra::detail::formatMomBoolean(false)}},
            true);
      }
      processClient->dispatchPendingPushedEvents();
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationCallbackBridgeError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif
  std::vector<umbra::detail::FederationSaveNotification> notifications;
  std::wstring federationName;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Federate Save Not Complete requires membership in a federation execution.");
    }

    federationName = *joinedFederationName_;
    auto result = embeddedFederationRegistry().federateSaveNotComplete(
        federationName,
        *joinedFederateId_);
    if (result.status != umbra::detail::FederationSaveControlStatus::applied) {
      throwFederationSaveServiceFailure(
          result.status,
          L"Federate Save Not Complete",
          FederationSaveServiceFailure::completion);
    }
    notifications = std::move(result.notifications);
  }
  // §4.22 has one reportable service. The C++ failure spelling selects the
  // same service with its required save-success indicator set to false.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"FederateSaveComplete",
      umbra::detail::MomServiceType::federation_management,
      {{umbra::detail::MomArgumentType::boolean,
        L"Federate save-success indicator",
        umbra::detail::formatMomBoolean(false)}},
      true);
  submitAmbassadorFederationSaveNotifications(
      federationName,
      std::move(notifications));
  } catch (Exception const& exception) {
    emitExceptionReport(L"Federate Save Complete", exception);
    throw;
  }
}

void UmbraRtiAmbassador::abortFederationSave() {
  auto instrumentationScope = beginRtiCall("abortFederationSave");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    std::wstring federationName;
    std::uint64_t federateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Abort Federation Save requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (!processClient) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    try {
      auto const result = processClient->abortFederationSave(
          federationName, federateId);
      if (result.status !=
          umbra::detail::FederationSaveControlStatus::applied) {
        throwFederationSaveServiceFailure(
            result.status,
            L"Abort Federation Save",
            FederationSaveServiceFailure::abort);
      }
      if (joinedServiceReport_) {
        appendSuccessfulVoidServiceReportToFileIfSelected(
            L"AbortFederationSave",
            umbra::detail::MomServiceType::federation_management,
            {},
            true);
      }
      processClient->dispatchPendingPushedEvents();
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationCallbackBridgeError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif
  std::vector<umbra::detail::FederationSaveNotification> notifications;
  std::wstring federationName;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Abort Federation Save requires membership in a federation execution.");
    }

    federationName = *joinedFederationName_;
    auto result = embeddedFederationRegistry().abortFederationSave(
        federationName,
        *joinedFederateId_);
    if (result.status != umbra::detail::FederationSaveControlStatus::applied) {
      throwFederationSaveServiceFailure(
          result.status,
          L"Abort Federation Save",
          FederationSaveServiceFailure::abort);
    }
    notifications = std::move(result.notifications);
  }
  // Section 4.24 has no supplied or returned arguments. The accepted abort
  // request is the report boundary; the later Federation Saved/Not Saved
  // callback communicates the result of the aborted save operation.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"AbortFederationSave",
      umbra::detail::MomServiceType::federation_management,
      {},
      true);
  submitAmbassadorFederationSaveNotifications(
      federationName,
      std::move(notifications));
  } catch (Exception const& exception) {
    emitExceptionReport(L"Abort Federation Save", exception);
    throw;
  }
}

void UmbraRtiAmbassador::queryFederationSaveStatus() {
  auto instrumentationScope = beginRtiCall("queryFederationSaveStatus");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    std::wstring federationName;
    std::uint64_t federateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Query Federation Save Status requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (!processClient) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    try {
      auto const result = processClient->queryFederationSaveStatus(
          federationName, federateId);
      if (result.status !=
          umbra::detail::FederationSaveControlStatus::applied) {
        throwFederationSaveServiceFailure(
            result.status,
            L"Query Federation Save Status",
            FederationSaveServiceFailure::query);
      }
      if (joinedServiceReport_) {
        appendSuccessfulVoidServiceReportToFileIfSelected(
            L"QueryFederationSaveStatus",
            umbra::detail::MomServiceType::federation_management,
            {},
            true);
      }
      processClient->dispatchPendingPushedEvents();
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationCallbackBridgeError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif
  std::vector<umbra::detail::FederationSaveNotification> notifications;
  std::wstring federationName;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Query Federation Save Status requires membership in a federation execution.");
    }

    auto result = embeddedFederationRegistry().queryFederationSaveStatus(
        *joinedFederationName_,
        *joinedFederateId_);
    if (result.status != umbra::detail::FederationSaveControlStatus::applied) {
      throwFederationSaveServiceFailure(
          result.status,
          L"Query Federation Save Status",
          FederationSaveServiceFailure::query);
    }
    federationName = *joinedFederationName_;
    notifications = std::move(result.notifications);
  }
  // Section 4.25 has no supplied or returned arguments. Report the accepted
  // query before its separately queued Federation Save Status Response
  // callback provides the current member-status vector, after native locks
  // have been released so an immediate Java callback cannot re-enter them.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"QueryFederationSaveStatus",
      umbra::detail::MomServiceType::federation_management,
      {},
      true);
  submitAmbassadorFederationSaveNotifications(
      federationName,
      std::move(notifications));
  } catch (Exception const& exception) {
    emitExceptionReport(L"Query Federation Save Status", exception);
    throw;
  }
}

void UmbraRtiAmbassador::requestFederationRestore(std::wstring const& label) {
  auto instrumentationScope = beginRtiCall("requestFederationRestore");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    std::wstring federationName;
    std::uint64_t federateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Request Federation Restore requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (!processClient) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    try {
      auto const result = processClient->requestFederationRestore(
          federationName, federateId, label);
      if (result.status !=
              umbra::detail::FederationRestoreControlStatus::applied &&
          result.status !=
              umbra::detail::FederationRestoreControlStatus::snapshot_not_found &&
          result.status !=
              umbra::detail::FederationRestoreControlStatus::membership_mismatch) {
        throwFederationRestoreServiceFailure(
            result.status,
            L"Request Federation Restore",
            FederationRestoreServiceFailure::request);
      }
      if (joinedServiceReport_) {
        appendSuccessfulVoidServiceReportToFileIfSelected(
            L"RequestFederationRestore",
            umbra::detail::MomServiceType::federation_management,
            {{umbra::detail::MomArgumentType::string,
              L"Federation save label",
              umbra::detail::formatMomString(label)}},
            true);
      }
      processClient->dispatchPendingPushedEvents();
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationCallbackBridgeError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif
  std::vector<umbra::detail::FederationRestoreNotification> notifications;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Request Federation Restore requires membership in a federation execution.");
    }

    auto result = embeddedFederationRegistry().requestFederationRestore(
        *joinedFederationName_,
        *joinedFederateId_,
        label);
    if (result.status != umbra::detail::FederationRestoreControlStatus::applied &&
        result.status != umbra::detail::FederationRestoreControlStatus::snapshot_not_found &&
        result.status != umbra::detail::FederationRestoreControlStatus::membership_mismatch) {
      throwFederationRestoreServiceFailure(
          result.status,
          L"Request Federation Restore",
          FederationRestoreServiceFailure::request);
    }
    notifications = std::move(result.notifications);
  }
  // Section 4.27 has one supplied Federation save label and no returned
  // arguments. A missing snapshot or membership mismatch is communicated by
  // the separately queued confirmation callback, not by failure of this
  // public service invocation. Emit the accepted record after native locks
  // are released so an immediate observer may enter Java/Python safely.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"RequestFederationRestore",
      umbra::detail::MomServiceType::federation_management,
      {{umbra::detail::MomArgumentType::string,
        L"Federation save label",
        umbra::detail::formatMomString(label)}},
      true);
  submitAmbassadorFederationRestoreNotifications(
      *joinedFederationName_,
      std::move(notifications));
  } catch (Exception const& exception) {
    emitExceptionReport(L"Request Federation Restore", exception);
    throw;
  }
}

void UmbraRtiAmbassador::federateRestoreComplete() {
  auto instrumentationScope = beginRtiCall("federateRestoreComplete");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    std::wstring federationName;
    std::uint64_t federateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Federate Restore Complete requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (!processClient) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    try {
      auto const result = processClient->federateRestoreComplete(
          federationName, federateId, callbacks_->isEnabled());
      if (result.status !=
          umbra::detail::FederationRestoreControlStatus::applied) {
        throwFederationRestoreServiceFailure(
            result.status,
            L"Federate Restore Complete",
            FederationRestoreServiceFailure::completion);
      }
      // The first process restore-control slice transports the lifecycle
      // notifications. Rebinding ownership/time work items remains a
      // separate card; this branch never silently fabricates those payloads.
      if (joinedServiceReport_) {
        appendSuccessfulVoidServiceReportToFileIfSelected(
            L"FederateRestoreComplete",
            umbra::detail::MomServiceType::federation_management,
            {{umbra::detail::MomArgumentType::boolean,
              L"Federate restore-success indicator",
              umbra::detail::formatMomBoolean(true)}},
            true);
      }
      processClient->dispatchPendingPushedEvents();
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationCallbackBridgeError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif
  std::vector<umbra::detail::FederationRestoreNotification> notifications;
  std::vector<umbra::detail::FederationTimeGrantDispatch> timeAdvanceGrantDispatches;
  std::vector<umbra::detail::FederationTimeGrantDispatch> timeRoleEnableDispatches;
  std::vector<umbra::detail::AttributeOwnershipAcquisitionWorkItem>
      ownershipAcquisitionWorkItems;
  std::vector<umbra::detail::AttributeOwnershipAcquisitionCancellationWorkItem>
      ownershipAcquisitionCancellationWorkItems;
  std::vector<umbra::detail::AttributeOwnershipDivestitureIfWantedNotification>
      ownershipDivestitureIfWantedWorkItems;
  std::vector<umbra::detail::ConfirmDivestitureNotification>
      confirmDivestitureWorkItems;
  std::vector<umbra::detail::AttributeTransportationTypeChangeWorkItem>
      attributeTransportationTypeChangeWorkItems;
  std::vector<umbra::detail::InteractionTransportationTypeChangeWorkItem>
      interactionTransportationTypeChangeWorkItems;
  std::vector<umbra::detail::AttributeValueUpdateProvideWorkItem>
      attributeValueUpdateProvideWorkItems;
  std::vector<umbra::detail::AttributeValueUpdateClassProvideWorkItem>
      attributeValueUpdateClassProvideWorkItems;
  std::vector<umbra::detail::AttributeValueUpdateRegionalProvideWorkItem>
      attributeValueUpdateRegionalProvideWorkItems;
  std::vector<umbra::detail::AttributeOwnershipQueryRecipient>
      attributeOwnershipQueryWorkItems;
  std::vector<umbra::detail::AttributeOwnershipAssumptionRecipient>
      attributeOwnershipAssumptionWorkItems;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Federate Restore Complete requires membership in a federation execution.");
    }

    auto result = embeddedFederationRegistry().federateRestoreComplete(
        *joinedFederationName_,
        *joinedFederateId_);
    if (result.status != umbra::detail::FederationRestoreControlStatus::applied) {
      throwFederationRestoreServiceFailure(
          result.status,
          L"Federate Restore Complete",
          FederationRestoreServiceFailure::completion);
    }
    notifications = std::move(result.notifications);
    timeAdvanceGrantDispatches = std::move(result.timeAdvanceGrantDispatches);
    timeRoleEnableDispatches = std::move(result.timeRoleEnableDispatches);
    ownershipAcquisitionWorkItems = std::move(result.ownershipAcquisitionWorkItems);
    ownershipAcquisitionCancellationWorkItems =
        std::move(result.ownershipAcquisitionCancellationWorkItems);
    ownershipDivestitureIfWantedWorkItems =
        std::move(result.ownershipDivestitureIfWantedWorkItems);
    confirmDivestitureWorkItems =
        std::move(result.confirmDivestitureWorkItems);
    attributeTransportationTypeChangeWorkItems =
        std::move(result.attributeTransportationTypeChangeWorkItems);
    interactionTransportationTypeChangeWorkItems =
        std::move(result.interactionTransportationTypeChangeWorkItems);
    attributeValueUpdateProvideWorkItems =
        std::move(result.attributeValueUpdateProvideWorkItems);
    attributeValueUpdateClassProvideWorkItems =
        std::move(result.attributeValueUpdateClassProvideWorkItems);
    attributeValueUpdateRegionalProvideWorkItems =
        std::move(result.attributeValueUpdateRegionalProvideWorkItems);
    attributeOwnershipQueryWorkItems =
        std::move(result.attributeOwnershipQueryWorkItems);
    attributeOwnershipAssumptionWorkItems =
        std::move(result.attributeOwnershipAssumptionWorkItems);
  }
  // The official C++ binding represents §4.31's one required restore-success
  // indicator with the Complete/Not Complete selector pair. Emit the public
  // service record after releasing native locks and before the registry's
  // Federation Restored callback or restored time-grant work is submitted.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"FederateRestoreComplete",
      umbra::detail::MomServiceType::federation_management,
      {{umbra::detail::MomArgumentType::boolean,
        L"Federate restore-success indicator",
        umbra::detail::formatMomBoolean(true)}},
      true);
  submitAmbassadorFederationRestoreNotifications(
      *joinedFederationName_,
      std::move(notifications));
  // Queued Request Attribute Ownership Assumption callbacks are rebound only
  // after Federation Restored, preserving the same callback ordering boundary
  // as the other restored application-request families.
  queueAmbassadorAttributeOwnershipAssumptionRecipients(
      std::move(attributeOwnershipAssumptionWorkItems),
      *joinedFederationName_,
      VariableLengthData());
  // Fresh-registry restores rebind pending regular acquisition reservations
  // to the current callback routes. Submit this work only after the
  // Federation Restored callbacks so ownership callbacks observe restored
  // object state and preserve the normal callback ordering boundary.
  queueAmbassadorAttributeOwnershipAcquisitionWorkItems(
      std::move(ownershipAcquisitionWorkItems),
      *joinedFederationName_);
  // Cancellation confirmations are a separate requester callback family and
  // are submitted after the ordinary ownership work has been rebound.
  for (auto& cancellation : ownershipAcquisitionCancellationWorkItems) {
    queueAmbassadorAttributeOwnershipAcquisitionCancellationConfirmation(
        std::move(cancellation.callbackRoute),
        *joinedFederationName_,
        cancellation.requestingFederateId,
        cancellation.objectInstanceHandle,
        cancellation.cancellationId,
        std::move(cancellation.attributeHandles));
  }
  // Divestiture If Wanted transfers ownership synchronously at the service
  // boundary, but its Acquisition Notification remains callback-gated.  The
  // fresh-registry restore rebinds each notification to the live requester
  // route only after Federation Restored.
  queueAmbassadorAttributeOwnershipDivestitureIfWantedNotifications(
      std::move(ownershipDivestitureIfWantedWorkItems),
      *joinedFederationName_);
  // Confirm Divestiture likewise transfers ownership at the service
  // boundary, but its Acquisition Notification remains callback-gated.
  // Submit the rebound route only after Federation Restored so the callback
  // observes the restored ownership projection and consumes exactly once.
  queueAmbassadorConfirmDivestitureNotifications(
      std::move(confirmDivestitureWorkItems),
      *joinedFederationName_);
  // Pending attribute transportation-type changes are rebound after the
  // restore lifecycle callback and committed only when their confirmation
  // callback begins.
  for (auto& change : attributeTransportationTypeChangeWorkItems) {
    queueAmbassadorConfirmAttributeTransportationTypeChange(
        std::move(change.callbackRoute),
        *joinedFederationName_,
        change.requestingFederateId,
        change.requestId);
  }
  // Pending interaction transportation-type changes follow the same
  // restored-before-confirmation boundary and commit only when the public
  // confirmation callback begins.
  for (auto& change : interactionTransportationTypeChangeWorkItems) {
    queueAmbassadorConfirmInteractionTransportationTypeChange(
        std::move(change.callbackRoute),
        *joinedFederationName_,
        change.requestingFederateId,
        change.interactionClassHandle);
  }
  // Pending object-instance Request Attribute Value Update callbacks are
  // rebound to the provider route only after Federation Restored. The
  // callback-time registry boundary consumes the durable request identity and
  // suppresses stale ownership/member changes without replaying the request.
  for (auto& work : attributeValueUpdateProvideWorkItems) {
    queueAmbassadorAttributeValueUpdateProvide(
        std::move(work.callbackRoute),
        std::move(work.serviceReportRoute),
        *joinedFederationName_,
        work.requestingFederateId,
        work.providingFederateId,
        work.objectInstanceHandle,
        std::move(work.requestedAttributeHandles),
        makeAmbassadorVariableLengthData(work.userSuppliedTag),
        work.requestId);
  }
  for (auto& work : attributeValueUpdateClassProvideWorkItems) {
    queueAmbassadorAttributeValueUpdateClassProvide(
        std::move(work.callbackRoute),
        std::move(work.serviceReportRoute),
        *joinedFederationName_,
        work.requestingFederateId,
        work.providingFederateId,
        work.objectInstanceHandle,
        work.requestedObjectClassHandle,
        std::move(work.requestedAttributeHandles),
        makeAmbassadorVariableLengthData(work.userSuppliedTag),
        std::nullopt,
        work.requestId);
  }
  for (auto& work : attributeValueUpdateRegionalProvideWorkItems) {
    queueAmbassadorAttributeValueUpdateClassProvide(
        std::move(work.callbackRoute),
        std::move(work.serviceReportRoute),
        *joinedFederationName_,
        work.requestingFederateId,
        work.providingFederateId,
        work.objectInstanceHandle,
        work.requestedObjectClassHandle,
        std::move(work.requestedAttributeHandles),
        makeAmbassadorVariableLengthData(work.userSuppliedTag),
        std::move(work.requestRegionsByAttribute),
        work.requestId);
  }
  // Rebound Query Attribute Ownership results are submitted only after the
  // Federation Restored callbacks, so the callback observes the restored
  // object/ownership projection and consumes its durable request exactly once.
  for (auto& work : attributeOwnershipQueryWorkItems) {
    queueAmbassadorAttributeOwnershipQueryReport(
        std::move(work.callbackRoute),
        *joinedFederationName_,
        work.requestId,
        work.receivingFederateId,
        work.objectInstanceHandle,
        work.reportKind,
        work.owningFederateId,
        std::move(work.attributeHandles));
  }
  // Reconstructed role-enable callbacks are submitted after the restore
  // lifecycle notifications and before restored time grants. Their dispatch
  // factories are bound to the current ambassadors, so a final completion
  // call can wake every member in a multi-federate restore.
  submitAmbassadorTimeAdvanceGrantDispatches(std::move(timeRoleEnableDispatches));
  submitAmbassadorTimeAdvanceGrantDispatches(std::move(timeAdvanceGrantDispatches));
  } catch (Exception const& exception) {
    emitExceptionReport(L"Federate Restore Complete", exception);
    throw;
  }
}

void UmbraRtiAmbassador::federateRestoreNotComplete() {
  auto instrumentationScope = beginRtiCall("federateRestoreNotComplete");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    std::wstring federationName;
    std::uint64_t federateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Federate Restore Not Complete requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (!processClient) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    try {
      auto const result = processClient->federateRestoreNotComplete(
          federationName, federateId);
      if (result.status !=
          umbra::detail::FederationRestoreControlStatus::applied) {
        throwFederationRestoreServiceFailure(
            result.status,
            L"Federate Restore Not Complete",
            FederationRestoreServiceFailure::completion);
      }
      if (joinedServiceReport_) {
        appendSuccessfulVoidServiceReportToFileIfSelected(
            L"FederateRestoreComplete",
            umbra::detail::MomServiceType::federation_management,
            {{umbra::detail::MomArgumentType::boolean,
              L"Federate restore-success indicator",
              umbra::detail::formatMomBoolean(false)}},
            true);
      }
      processClient->dispatchPendingPushedEvents();
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationCallbackBridgeError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif
  std::vector<umbra::detail::FederationRestoreNotification> notifications;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Federate Restore Not Complete requires membership in a federation execution.");
    }

    auto result = embeddedFederationRegistry().federateRestoreNotComplete(
        *joinedFederationName_,
        *joinedFederateId_);
    if (result.status != umbra::detail::FederationRestoreControlStatus::applied) {
      throwFederationRestoreServiceFailure(
          result.status,
          L"Federate Restore Not Complete",
          FederationRestoreServiceFailure::completion);
    }
    notifications = std::move(result.notifications);
  }
  // §4.31 has the same reportable service for the C++ failure selector; its
  // required restore-success indicator is false rather than a distinct
  // service name. Emit it after releasing native locks.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"FederateRestoreComplete",
      umbra::detail::MomServiceType::federation_management,
      {{umbra::detail::MomArgumentType::boolean,
        L"Federate restore-success indicator",
        umbra::detail::formatMomBoolean(false)}},
      true);
  submitAmbassadorFederationRestoreNotifications(
      *joinedFederationName_,
      std::move(notifications));
  } catch (Exception const& exception) {
    emitExceptionReport(L"Federate Restore Complete", exception);
    throw;
  }
}

void UmbraRtiAmbassador::abortFederationRestore() {
  auto instrumentationScope = beginRtiCall("abortFederationRestore");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    std::wstring federationName;
    std::uint64_t federateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Abort Federation Restore requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (!processClient) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    try {
      auto const result = processClient->abortFederationRestore(
          federationName, federateId);
      if (result.status !=
          umbra::detail::FederationRestoreControlStatus::applied) {
        throwFederationRestoreServiceFailure(
            result.status,
            L"Abort Federation Restore",
            FederationRestoreServiceFailure::abort);
      }
      if (joinedServiceReport_) {
        appendSuccessfulVoidServiceReportToFileIfSelected(
            L"AbortFederationRestore",
            umbra::detail::MomServiceType::federation_management,
            {},
            true);
      }
      processClient->dispatchPendingPushedEvents();
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationCallbackBridgeError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif
  std::vector<umbra::detail::FederationRestoreNotification> notifications;
  std::wstring federationName;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Abort Federation Restore requires membership in a federation execution.");
    }

    auto result = embeddedFederationRegistry().abortFederationRestore(
        *joinedFederationName_,
        *joinedFederateId_);
    if (result.status != umbra::detail::FederationRestoreControlStatus::applied) {
      throwFederationRestoreServiceFailure(
          result.status,
          L"Abort Federation Restore",
          FederationRestoreServiceFailure::abort);
    }
    // Section 4.33 has no supplied or returned arguments. The accepted abort
    // request is the report boundary; the later restore-result callback
    // communicates the outcome of the aborted restore operation.
    federationName = *joinedFederationName_;
    notifications = std::move(result.notifications);
  }
  // Emit after releasing native locks so an immediate public MOM observer can
  // safely re-enter the Java/Python surface.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"AbortFederationRestore",
      umbra::detail::MomServiceType::federation_management,
      {},
      true);
  submitAmbassadorFederationRestoreNotifications(
      federationName,
      std::move(notifications));
  } catch (Exception const& exception) {
    emitExceptionReport(L"Abort Federation Restore", exception);
    throw;
  }
}

void UmbraRtiAmbassador::queryFederationRestoreStatus() {
  auto instrumentationScope = beginRtiCall("queryFederationRestoreStatus");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    std::wstring federationName;
    std::uint64_t federateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Query Federation Restore Status requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (!processClient) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    try {
      auto const result = processClient->queryFederationRestoreStatus(
          federationName, federateId);
      if (result.status !=
          umbra::detail::FederationRestoreControlStatus::applied) {
        throwFederationRestoreServiceFailure(
            result.status,
            L"Query Federation Restore Status",
            FederationRestoreServiceFailure::query);
      }
      if (joinedServiceReport_) {
        appendSuccessfulVoidServiceReportToFileIfSelected(
            L"QueryFederationRestoreStatus",
            umbra::detail::MomServiceType::federation_management,
            {},
            true);
      }
      processClient->dispatchPendingPushedEvents();
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationCallbackBridgeError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif
  std::vector<umbra::detail::FederationRestoreNotification> notifications;
  std::wstring federationName;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Query Federation Restore Status requires membership in a federation execution.");
    }

    auto result = embeddedFederationRegistry().queryFederationRestoreStatus(
        *joinedFederationName_,
        *joinedFederateId_);
    if (result.status != umbra::detail::FederationRestoreControlStatus::applied) {
      throwFederationRestoreServiceFailure(
          result.status,
          L"Query Federation Restore Status",
          FederationRestoreServiceFailure::query);
    }
    // Section 4.34 has no supplied or returned arguments. The accepted query
    // is the report boundary; the later Federation Restore Status Response
    // callback carries the restore-status descriptor vector.
    federationName = *joinedFederationName_;
    notifications = std::move(result.notifications);
  }
  // Keep the public interaction outside native locks for the same callback
  // re-entrancy guarantee as the other restore-control services.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"QueryFederationRestoreStatus",
      umbra::detail::MomServiceType::federation_management,
      {},
      true);
  submitAmbassadorFederationRestoreNotifications(
      federationName,
      std::move(notifications));
  } catch (Exception const& exception) {
    emitExceptionReport(L"Query Federation Restore Status", exception);
    throw;
  }
}


}  // namespace rti1516_2025::umbra_binding_detail
