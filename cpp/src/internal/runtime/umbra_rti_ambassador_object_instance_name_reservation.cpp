#include "internal/runtime/umbra_rti_ambassador.hpp"

#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/ambassador_string_helpers.hpp"

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "internal/federation/federation_management_coordinator.hpp"
#endif

#include <mutex>
#include <utility>

namespace rti1516_2025::umbra_binding_detail {
void UmbraRtiAmbassador::reserveObjectInstanceName(
    std::wstring const& objectInstanceName) {
  auto instrumentationScope = beginRtiCall("reserveObjectInstanceName");
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
            L"Reserve Object Instance Name requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }

    umbra::detail::ProcessFederationReserveObjectInstanceNameResult result;
    try {
      result = processClient->reserveObjectInstanceName(
          std::move(federationName), federateId, objectInstanceName);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    if (result.status !=
        umbra::detail::ObjectInstanceNameReservationStatus::applied) {
      throwObjectInstanceNameReservationFailureForFederationManagement(
          result.status,
          L"Reserve Object Instance Name");
    }
    if (result.objectInstanceName.empty()) {
      throw RTIinternalError(
          L"The private process endpoint returned an invalid object-instance name reservation.");
    }

    auto callbackSession = callbackSession_;
    if (!callbackSession) {
      throw RTIinternalError(
          L"The process endpoint has no federate ambassador callback recipient.");
    }
    callbacks_->submit([
        callbackSession = std::move(callbackSession),
        succeeded = result.succeeded,
        reservedName = std::move(result.objectInstanceName)]() mutable {
      callbackSession->invoke([
          succeeded,
          reservedName = std::move(reservedName)](FederateAmbassador& recipient) mutable {
        if (succeeded) {
          recipient.objectInstanceNameReservationSucceeded(reservedName);
        } else {
          recipient.objectInstanceNameReservationFailed(reservedName);
        }
      });
    });
    return;
  }
#endif
  std::wstring federationName;
  std::uint64_t federateId = 0;
  umbra::detail::ObjectInstanceNameReservationResult result;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Reserve Object Instance Name");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Reserve Object Instance Name requires membership in a federation execution.");
    }
    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    federationName = *joinedFederationName_;
    federateId = *joinedFederateId_;
    result = registry.reserveObjectInstanceName(
        federationName,
        federateId,
        objectInstanceName);
    if (result.status !=
        umbra::detail::ObjectInstanceNameReservationStatus::applied) {
      throwObjectInstanceNameReservationFailureForFederationManagement(
          result.status,
          L"Reserve Object Instance Name");
    }
  }
  // §6.2 has no returned arguments. The accepted reservation is the public
  // service boundary; emit its report after releasing the registry lock and
  // before the asynchronous reservation callback is queued.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"ReserveObjectInstanceName",
      umbra::detail::MomServiceType::object_management,
      {{umbra::detail::MomArgumentType::string,
        L"Name",
        umbra::detail::formatMomString(objectInstanceName)}},
      true);
  queueAmbassadorObjectInstanceNameReservation(
      std::move(result.callbackRoute),
      std::move(federationName),
      federateId,
      result.succeeded,
      std::move(result.objectInstanceName));
  } catch (Exception const& exception) {
    emitExceptionReport(L"Reserve Object Instance Name", exception);
    appendFailedServiceReportToFileIfSelected(
        L"ReserveObjectInstanceName",
        umbra::detail::MomServiceType::object_management,
        {{umbra::detail::MomArgumentType::string,
          L"Name",
          umbra::detail::formatMomString(objectInstanceName)}},
        describeAmbassadorException(exception),
        true);
    throw;
  }
}

void UmbraRtiAmbassador::releaseObjectInstanceName(
    std::wstring const& objectInstanceName) {
  auto instrumentationScope = beginRtiCall("releaseObjectInstanceName");
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
            L"Release Object Instance Name requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }

    umbra::detail::ProcessFederationObjectInstanceNameReleaseResult result;
    try {
      result = processClient->releaseObjectInstanceName(
          std::move(federationName), federateId, objectInstanceName);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    if (result.status !=
        umbra::detail::ObjectInstanceNameReservationStatus::applied) {
      throwObjectInstanceNameReservationFailureForFederationManagement(
          result.status,
          L"Release Object Instance Name");
    }
    return;
  }
#endif
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Release Object Instance Name");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Release Object Instance Name requires membership in a federation execution.");
    }
    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const status = registry.releaseObjectInstanceName(
        *joinedFederationName_,
        *joinedFederateId_,
        objectInstanceName);
    if (status != umbra::detail::ObjectInstanceNameReservationStatus::applied) {
      throwObjectInstanceNameReservationFailureForFederationManagement(
          status,
          L"Release Object Instance Name");
    }
  }
  // §6.4 is a successful-void service with one String/Name argument. Emit
  // outside native locks so an HLA_IMMEDIATE observer can safely re-enter.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"ReleaseObjectInstanceName",
      umbra::detail::MomServiceType::object_management,
      {{umbra::detail::MomArgumentType::string,
        L"Name",
        umbra::detail::formatMomString(objectInstanceName)}},
      true);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Release Object Instance Name", exception);
    appendFailedServiceReportToFileIfSelected(
        L"ReleaseObjectInstanceName",
        umbra::detail::MomServiceType::object_management,
        {{umbra::detail::MomArgumentType::string,
          L"Name",
          umbra::detail::formatMomString(objectInstanceName)}},
        describeAmbassadorException(exception),
        true);
    throw;
  }
}


}  // namespace rti1516_2025::umbra_binding_detail
