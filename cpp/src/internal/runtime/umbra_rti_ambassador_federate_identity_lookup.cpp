#include "internal/runtime/umbra_rti_ambassador.hpp"

#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/ambassador_string_helpers.hpp"

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "internal/federation/federation_management_coordinator.hpp"
#include "internal/handles/federate_handle.hpp"
#include "internal/observability/mom_service_report_encoding.hpp"
#endif

#include <mutex>

namespace rti1516_2025::umbra_binding_detail {

FederateHandle UmbraRtiAmbassador::getFederateHandle(std::wstring const& federateName) {
  auto instrumentationScope = beginRtiCall("getFederateHandle");
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
            L"Get Federate Handle requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }

    std::optional<std::uint64_t> handle;
    try {
      handle = processClient->lookupFederateHandle(
          federationName, federateId, federateName);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    if (!handle || *handle == 0U) {
      throw NameNotFound(
          L"The supplied federate name is not active in this federation execution.");
    }
    return makeFederateHandle(*handle);
  }
#endif
  FederateHandle federateHandle;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Get Federate Handle requires membership in a federation execution.");
    }

    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }

    auto membership = registry.memberByName(*joinedFederationName_, federateName);
    if (!membership) {
      throw NameNotFound(L"The supplied federate name is not active in this federation execution.");
    }
    if (membership->id == 0) {
      throw RTIinternalError(L"The embedded federation returned an invalid federate identity.");
    }
    federateHandle = makeFederateHandle(membership->id);
  }
  appendSuccessfulServiceReportToFileIfSelected(
      L"GetFederateHandle",
      umbra::detail::MomServiceType::support_services,
      {{umbra::detail::MomArgumentType::string,
        L"Federate name",
        umbra::detail::formatMomString(federateName)}},
      {umbra::detail::MomArgumentType::federate_handle,
       L"Federate handle",
       umbra::detail::formatMomFederateHandle(federateHandle)});
  return federateHandle;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Federate Handle", exception);
    appendFailedServiceReportToFileIfSelected(
        L"GetFederateHandle",
        umbra::detail::MomServiceType::support_services,
        {{umbra::detail::MomArgumentType::string,
          L"Federate name",
          umbra::detail::formatMomString(federateName)}},
        describeAmbassadorException(exception));
    throw;
  }
}

std::wstring UmbraRtiAmbassador::getFederateName(FederateHandle const& federate) {
  auto instrumentationScope = beginRtiCall("getFederateName");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const federateId = federateHandleValue(federate);
    if (!federateId) {
      throw InvalidFederateHandle(
          L"Get Federate Name requires a valid FederateHandle.");
    }
    std::wstring federationName;
    std::uint64_t requestingFederateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Get Federate Name requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      requestingFederateId = *joinedFederateId_;
    }

    std::optional<std::wstring> federateName;
    try {
      federateName = processClient->lookupFederateName(
          federationName, requestingFederateId, *federateId);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    if (!federateName || federateName->empty()) {
      throw FederateHandleNotKnown(
          L"The supplied FederateHandle is not known in this federation execution.");
    }
    return *federateName;
  }
#endif
  std::wstring federateName;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Get Federate Name requires membership in a federation execution.");
    }

    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }

    auto const federateId = federateHandleValue(federate);
    if (!federateId) {
      throw InvalidFederateHandle(L"Get Federate Name requires a valid FederateHandle.");
    }

    auto result = registry.federateNameFor(*joinedFederationName_, *federateId);
    if (!result) {
      throw FederateHandleNotKnown(
          L"The supplied FederateHandle is not known in this federation execution.");
    }
    federateName = *result;
  }
  appendSuccessfulServiceReportToFileIfSelected(
      L"GetFederateName",
      umbra::detail::MomServiceType::support_services,
      {{umbra::detail::MomArgumentType::federate_handle,
        L"Federate handle",
        umbra::detail::formatMomFederateHandle(federate)}},
      {umbra::detail::MomArgumentType::string,
       L"Federate name",
       umbra::detail::formatMomString(federateName)});
  return federateName;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Federate Name", exception);
    appendFailedServiceReportToFileIfSelected(
        L"GetFederateName",
        umbra::detail::MomServiceType::support_services,
        {{umbra::detail::MomArgumentType::federate_handle,
          L"Federate handle",
          umbra::detail::formatMomFederateHandle(federate)}},
        describeAmbassadorException(exception));
    throw;
  }
}

}  // namespace rti1516_2025::umbra_binding_detail
