#include "internal/runtime/umbra_rti_ambassador.hpp"

#include "internal/runtime/ambassador_string_helpers.hpp"
#include "internal/runtime/ambassador_shared_utilities.hpp"

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "internal/federation/federation_management_coordinator.hpp"
#include "internal/handles/object_class_handle.hpp"
#include "internal/runtime/utf8_string.hpp"
#endif

#include <mutex>
#include <optional>

namespace rti1516_2025::umbra_binding_detail {

ObjectClassHandle UmbraRtiAmbassador::getObjectClassHandle(
    std::wstring const& objectClassName) {
  auto instrumentationScope = beginRtiCall("getObjectClassHandle");
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
            L"Get Object Class Handle requires membership in a federation execution.");
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
      handle = processClient->lookupObjectClassHandle(
          federationName,
          federateId,
          objectClassName,
          callbacks_->isEnabled());
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    if (!handle) {
      throw NameNotFound(
          L"The supplied object class name is not defined in this federation execution.");
    }
    return makeObjectClassHandle(*handle);
  }
#endif
  ObjectClassHandle objectClassHandle;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Get Object Class Handle requires membership in a federation execution.");
    }

    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }

    auto const encodedName = umbra::detail::utf8FromWide(objectClassName);
    if (!encodedName) {
      throw NameNotFound(L"The supplied object class name is not valid UTF-8 text.");
    }
    auto const handle = registry.objectClassHandleFor(*joinedFederationName_, *encodedName);
    if (!handle) {
      throw NameNotFound(L"The supplied object class name is not defined in this federation execution.");
    }
    objectClassHandle = makeObjectClassHandle(*handle);
  }
  appendSuccessfulServiceReportToFileIfSelected(
      L"GetObjectClassHandle",
      umbra::detail::MomServiceType::support_services,
      {{umbra::detail::MomArgumentType::string,
        L"Object class name",
        umbra::detail::formatMomString(objectClassName)}},
      {umbra::detail::MomArgumentType::object_class_handle,
       L"Object class handle",
       umbra::detail::formatMomObjectClassHandle(objectClassHandle)});
  return objectClassHandle;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Object Class Handle", exception);
    appendFailedServiceReportToFileIfSelected(
        L"GetObjectClassHandle",
        umbra::detail::MomServiceType::support_services,
        {{umbra::detail::MomArgumentType::string,
          L"Object class name",
          umbra::detail::formatMomString(objectClassName)}},
        describeAmbassadorException(exception));
    throw;
  }
}

std::wstring UmbraRtiAmbassador::getObjectClassName(ObjectClassHandle const& objectClass) {
  auto instrumentationScope = beginRtiCall("getObjectClassName");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const objectClassHandle = objectClassHandleValue(objectClass);
    if (!objectClassHandle) {
      throw InvalidObjectClassHandle(
          L"Get Object Class Name requires a valid ObjectClassHandle.");
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
            L"Get Object Class Name requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }

    std::optional<std::wstring> objectClassName;
    try {
      objectClassName = processClient->lookupObjectClassName(
          federationName, federateId, *objectClassHandle);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    if (!objectClassName) {
      throw InvalidObjectClassHandle(
          L"The supplied ObjectClassHandle is not known in this federation execution.");
    }
    return *objectClassName;
  }
#endif
  std::wstring objectClassName;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Get Object Class Name requires membership in a federation execution.");
    }

    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }

    auto const handle = objectClassHandleValue(objectClass);
    if (!handle) {
      throw InvalidObjectClassHandle(L"Get Object Class Name requires a valid ObjectClassHandle.");
    }
    auto const encodedName = registry.objectClassNameFor(*joinedFederationName_, *handle);
    if (!encodedName) {
      throw InvalidObjectClassHandle(
          L"The supplied ObjectClassHandle is not known in this federation execution.");
    }
    auto const decodedName = umbra::detail::wideFromUtf8(*encodedName);
    if (!decodedName) {
      throw RTIinternalError(
          L"The embedded federation stored an object class name that is not valid UTF-8 text.");
    }
    objectClassName = *decodedName;
  }
  appendSuccessfulServiceReportToFileIfSelected(
      L"GetObjectClassName",
      umbra::detail::MomServiceType::support_services,
      {{umbra::detail::MomArgumentType::object_class_handle,
        L"Object class handle",
        umbra::detail::formatMomObjectClassHandle(objectClass)}},
      {umbra::detail::MomArgumentType::string,
       L"Object class name",
       umbra::detail::formatMomString(objectClassName)});
  return objectClassName;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Object Class Name", exception);
    appendFailedServiceReportToFileIfSelected(
        L"GetObjectClassName",
        umbra::detail::MomServiceType::support_services,
        {{umbra::detail::MomArgumentType::object_class_handle,
          L"Object class handle",
          umbra::detail::formatMomObjectClassHandle(objectClass)}},
        describeAmbassadorException(exception));
    throw;
  }
}

}  // namespace rti1516_2025::umbra_binding_detail
