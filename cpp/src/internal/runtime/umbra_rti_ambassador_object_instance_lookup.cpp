#include "internal/runtime/umbra_rti_ambassador.hpp"

#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/ambassador_string_helpers.hpp"

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "internal/federation/federation_management_coordinator.hpp"
#include "internal/handles/object_instance_handle.hpp"
#endif

#include <mutex>
#include <optional>

namespace rti1516_2025::umbra_binding_detail {

ObjectInstanceHandle UmbraRtiAmbassador::getObjectInstanceHandle(
    std::wstring const& objectInstanceName) {
  auto instrumentationScope = beginRtiCall("getObjectInstanceHandle");
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
            L"Get Object Instance Handle requires membership in a federation execution.");
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
      handle = processClient->lookupObjectInstanceHandle(
          std::move(federationName), federateId, objectInstanceName);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    if (!handle) {
      throw ObjectInstanceNotKnown(
          L"The supplied object instance name is not known to this federate.");
    }
    return makeObjectInstanceHandle(*handle);
  }
#endif
  ObjectInstanceHandle result;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Get Object Instance Handle requires membership in a federation execution.");
    }

    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const known = registry.knownObjectInstanceByNameFor(
        *joinedFederationName_,
        *joinedFederateId_,
        objectInstanceName);
    if (!known) {
      throw ObjectInstanceNotKnown(
          L"The supplied object instance name is not known to this federate.");
    }
    result = makeObjectInstanceHandle(known->objectInstanceHandle);
  }
  appendSuccessfulServiceReportToFileIfSelected(
      L"GetObjectInstanceHandle",
      umbra::detail::MomServiceType::support_services,
      {{umbra::detail::MomArgumentType::string,
        L"Object instance name",
        umbra::detail::formatMomString(objectInstanceName)}},
      {umbra::detail::MomArgumentType::object_instance_handle,
       L"Object instance handle",
       umbra::detail::formatMomObjectInstanceHandle(result)});
  return result;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Object Instance Handle", exception);
    appendFailedServiceReportToFileIfSelected(
        L"GetObjectInstanceHandle",
        umbra::detail::MomServiceType::support_services,
        {{umbra::detail::MomArgumentType::string,
          L"Object instance name",
          umbra::detail::formatMomString(objectInstanceName)}},
        describeAmbassadorException(exception));
    throw;
  }
}

std::wstring UmbraRtiAmbassador::getObjectInstanceName(
    ObjectInstanceHandle const& objectInstance) {
  auto instrumentationScope = beginRtiCall("getObjectInstanceName");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const objectInstanceHandleValueResult = objectInstanceHandleValue(objectInstance);
    if (!objectInstanceHandleValueResult) {
      throw ObjectInstanceNotKnown(
          L"The supplied ObjectInstanceHandle is not known to this federate.");
    }
    std::wstring federationName;
    std::uint64_t federateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Get Object Instance Name requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    std::optional<std::wstring> name;
    try {
      name = processClient->lookupObjectInstanceName(
          std::move(federationName), federateId, *objectInstanceHandleValueResult);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    if (!name) {
      throw ObjectInstanceNotKnown(
          L"The supplied ObjectInstanceHandle is not known to this federate.");
    }
    return *name;
  }
#endif
  std::wstring result;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Get Object Instance Name requires membership in a federation execution.");
    }

    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const objectInstanceHandleValueResult = objectInstanceHandleValue(objectInstance);
    if (!objectInstanceHandleValueResult) {
      throw ObjectInstanceNotKnown(
          L"The supplied ObjectInstanceHandle is not known to this federate.");
    }
    auto const known = registry.knownObjectInstanceFor(
        *joinedFederationName_,
        *joinedFederateId_,
        *objectInstanceHandleValueResult);
    if (!known) {
      throw ObjectInstanceNotKnown(
          L"The supplied ObjectInstanceHandle is not known to this federate.");
    }
    result = known->objectInstanceName;
  }
  appendSuccessfulServiceReportToFileIfSelected(
      L"GetObjectInstanceName",
      umbra::detail::MomServiceType::support_services,
      {{umbra::detail::MomArgumentType::object_instance_handle,
        L"Object instance handle",
        umbra::detail::formatMomObjectInstanceHandle(objectInstance)}},
      {umbra::detail::MomArgumentType::string,
       L"Object instance name",
       umbra::detail::formatMomString(result)});
  return result;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Object Instance Name", exception);
    appendFailedServiceReportToFileIfSelected(
        L"GetObjectInstanceName",
        umbra::detail::MomServiceType::support_services,
        {{umbra::detail::MomArgumentType::object_instance_handle,
          L"Object instance handle",
          umbra::detail::formatMomObjectInstanceHandle(objectInstance)}},
        describeAmbassadorException(exception));
    throw;
  }
}

}  // namespace rti1516_2025::umbra_binding_detail
