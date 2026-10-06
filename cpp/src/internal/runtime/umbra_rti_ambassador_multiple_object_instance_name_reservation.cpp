#include "internal/runtime/umbra_rti_ambassador.hpp"

#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/ambassador_string_helpers.hpp"

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "internal/federation/federation_management_coordinator.hpp"
#endif

#include <mutex>
#include <set>
#include <utility>

namespace rti1516_2025::umbra_binding_detail {
void UmbraRtiAmbassador::reserveMultipleObjectInstanceNames(
    std::set<std::wstring> const& objectInstanceNames) {
  auto instrumentationScope = beginRtiCall("reserveMultipleObjectInstanceNames");
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
            L"Reserve Multiple Object Instance Names requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }

    umbra::detail::ProcessFederationReserveMultipleObjectInstanceNamesResult result;
    try {
      result = processClient->reserveMultipleObjectInstanceNames(
          std::move(federationName), federateId, objectInstanceNames);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    if (result.status !=
        umbra::detail::ObjectInstanceNameReservationStatus::applied) {
      throwObjectInstanceNameReservationFailureForFederationManagement(
          result.status,
          L"Reserve Multiple Object Instance Names");
    }
    auto callbackSession = callbackSession_;
    if (!callbackSession) {
      throw RTIinternalError(
          L"The process endpoint has no federate ambassador callback recipient.");
    }
    // Process-endpoint joins do not install a local embedded service-report
    // route; keep this branch aligned with the existing single-name process
    // service and let the process service own its reporting boundary.
    if (!result.succeededNames.empty()) {
      callbacks_->submit([
          callbackSession,
          succeededNames = std::move(result.succeededNames)]() mutable {
        callbackSession->invoke([
            succeededNames = std::move(succeededNames)](
            FederateAmbassador& recipient) mutable {
          recipient.multipleObjectInstanceNameReservationSucceeded(
              succeededNames);
        });
      });
    }
    if (!result.failedNames.empty()) {
      callbacks_->submit([
          callbackSession,
          failedNames = std::move(result.failedNames)]() mutable {
        callbackSession->invoke([
            failedNames = std::move(failedNames)](
            FederateAmbassador& recipient) mutable {
          recipient.multipleObjectInstanceNameReservationFailed(failedNames);
        });
      });
    }
    return;
  }
#endif
  std::wstring federationName;
  std::uint64_t federateId = 0;
  umbra::detail::MultipleObjectInstanceNameReservationResult result;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Reserve Multiple Object Instance Names");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Reserve Multiple Object Instance Names requires membership in a federation execution.");
    }
    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    federationName = *joinedFederationName_;
    federateId = *joinedFederateId_;
    result = registry.reserveMultipleObjectInstanceNames(
        federationName,
        federateId,
        objectInstanceNames);
    if (result.status !=
        umbra::detail::ObjectInstanceNameReservationStatus::applied) {
      throwObjectInstanceNameReservationFailureForFederationManagement(
          result.status,
          L"Reserve Multiple Object Instance Names");
    }
  }

  // §6.5 supplies one non-empty StringSet and returns None. Emit the public
  // report after releasing the registry lock; its later per-name outcome
  // callbacks remain the typed response boundary.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"ReserveMultipleObjectInstanceNames",
      umbra::detail::MomServiceType::object_management,
      {{umbra::detail::MomArgumentType::string_set,
        L"Name Set",
        umbra::detail::formatMomStringSet(objectInstanceNames)}},
      true);

  if (!result.succeededNames.empty()) {
    queueAmbassadorMultipleObjectInstanceNameReservation(
        result.callbackRoute,
        federationName,
        federateId,
        true,
        result.succeededNames);
  }
  if (!result.failedNames.empty()) {
    queueAmbassadorMultipleObjectInstanceNameReservation(
        std::move(result.callbackRoute),
        std::move(federationName),
        federateId,
        false,
        std::move(result.failedNames));
  }
  } catch (Exception const& exception) {
    emitExceptionReport(L"Reserve Multiple Object Instance Names", exception);
    appendFailedServiceReportToFileIfSelected(
        L"ReserveMultipleObjectInstanceNames",
        umbra::detail::MomServiceType::object_management,
        {{umbra::detail::MomArgumentType::string_set,
          L"Name Set",
          umbra::detail::formatMomStringSet(objectInstanceNames)}},
        describeAmbassadorException(exception),
        true);
    throw;
  }
}

void UmbraRtiAmbassador::releaseMultipleObjectInstanceNames(
    std::set<std::wstring> const& objectInstanceNames) {
  auto instrumentationScope = beginRtiCall("releaseMultipleObjectInstanceNames");
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
            L"Release Multiple Object Instance Names requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }

    umbra::detail::ProcessFederationReleaseMultipleObjectInstanceNamesResult result;
    try {
      result = processClient->releaseMultipleObjectInstanceNames(
          std::move(federationName), federateId, objectInstanceNames);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    if (result.status !=
        umbra::detail::ObjectInstanceNameReservationStatus::applied) {
      throwObjectInstanceNameReservationFailureForFederationManagement(
          result.status,
          L"Release Multiple Object Instance Names");
    }
    return;
  }
#endif
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Release Multiple Object Instance Names");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Release Multiple Object Instance Names requires membership in a federation execution.");
    }
    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const status = registry.releaseMultipleObjectInstanceNames(
        *joinedFederationName_,
        *joinedFederateId_,
        objectInstanceNames);
    if (status != umbra::detail::ObjectInstanceNameReservationStatus::applied) {
      throwObjectInstanceNameReservationFailureForFederationManagement(
          status,
          L"Release Multiple Object Instance Names");
    }
  }
  // §6.7 supplies a StringSet and returns None.  Emit the accepted service
  // after releasing native locks so an HLA_IMMEDIATE MOM observer can safely
  // re-enter the RTI; no asynchronous callback follows this atomic release.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"ReleaseMultipleObjectInstanceNames",
      umbra::detail::MomServiceType::object_management,
      {{umbra::detail::MomArgumentType::string_set,
        L"Name set",
        umbra::detail::formatMomStringSet(objectInstanceNames)}},
      true);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Release Multiple Object Instance Names", exception);
    appendFailedServiceReportToFileIfSelected(
        L"ReleaseMultipleObjectInstanceNames",
        umbra::detail::MomServiceType::object_management,
        {{umbra::detail::MomArgumentType::string_set,
          L"Name set",
          umbra::detail::formatMomStringSet(objectInstanceNames)}},
        describeAmbassadorException(exception),
        true);
    throw;
  }
}


}  // namespace rti1516_2025::umbra_binding_detail
