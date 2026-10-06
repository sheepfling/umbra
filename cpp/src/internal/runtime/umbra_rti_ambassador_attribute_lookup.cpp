#include "internal/runtime/umbra_rti_ambassador.hpp"

#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/ambassador_string_helpers.hpp"

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "internal/federation/federation_management_coordinator.hpp"
#include "internal/handles/attribute_handle.hpp"
#include "internal/handles/object_class_handle.hpp"
#include "internal/runtime/utf8_string.hpp"
#endif

#include <mutex>
#include <optional>
#include <utility>

namespace rti1516_2025::umbra_binding_detail {

AttributeHandle UmbraRtiAmbassador::getAttributeHandle(
    ObjectClassHandle const& objectClass,
    std::wstring const& attributeName) {
  auto instrumentationScope = beginRtiCall("getAttributeHandle");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const objectClassHandle = objectClassHandleValue(objectClass);
    if (!objectClassHandle) {
      throw InvalidObjectClassHandle(
          L"Get Attribute Handle requires a valid ObjectClassHandle.");
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
            L"Get Attribute Handle requires membership in a federation execution.");
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
      handle = processClient->lookupAttributeHandle(
          std::move(federationName),
          federateId,
          *objectClassHandle,
          attributeName);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    if (!handle) {
      throw NameNotFound(
          L"The supplied attribute name is not defined for this object class.");
    }
    return makeAttributeHandle(*handle);
  }
#endif
  AttributeHandle result;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Get Attribute Handle requires membership in a federation execution.");
    }

    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }

    auto const objectClassHandleValueResult = objectClassHandleValue(objectClass);
    if (!objectClassHandleValueResult) {
      throw InvalidObjectClassHandle(
          L"Get Attribute Handle requires a valid ObjectClassHandle.");
    }
    auto const encodedObjectClassName = registry.objectClassNameFor(
        *joinedFederationName_,
        *objectClassHandleValueResult);
    if (!encodedObjectClassName) {
      throw InvalidObjectClassHandle(
          L"The supplied ObjectClassHandle is not known in this federation execution.");
    }

    auto const encodedAttributeName = umbra::detail::utf8FromWide(attributeName);
    if (!encodedAttributeName) {
      throw NameNotFound(L"The supplied attribute name is not valid UTF-8 text.");
    }
    auto const attributeHandle = registry.attributeHandleFor(
        *joinedFederationName_,
        *encodedObjectClassName,
        *encodedAttributeName);
    if (!attributeHandle) {
      throw NameNotFound(L"The supplied attribute name is not defined for this object class.");
    }
    result = makeAttributeHandle(*attributeHandle);
  }
  appendSuccessfulServiceReportToFileIfSelected(
      L"GetAttributeHandle",
      umbra::detail::MomServiceType::support_services,
      {{umbra::detail::MomArgumentType::object_class_handle,
        L"Object class handle",
        umbra::detail::formatMomObjectClassHandle(objectClass)},
       {umbra::detail::MomArgumentType::string,
        L"Class attribute name",
        umbra::detail::formatMomString(attributeName)}},
      {umbra::detail::MomArgumentType::attribute_handle,
       L"Class attribute handle",
       umbra::detail::formatMomAttributeHandle(result)});
  return result;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Attribute Handle", exception);
    appendFailedServiceReportToFileIfSelected(
        L"GetAttributeHandle",
        umbra::detail::MomServiceType::support_services,
        {{umbra::detail::MomArgumentType::object_class_handle,
          L"Object class handle",
          umbra::detail::formatMomObjectClassHandle(objectClass)},
         {umbra::detail::MomArgumentType::string,
          L"Class attribute name",
          umbra::detail::formatMomString(attributeName)}},
        describeAmbassadorException(exception));
    throw;
  }
}

std::wstring UmbraRtiAmbassador::getAttributeName(
    ObjectClassHandle const& objectClass,
    AttributeHandle const& attribute) {
  auto instrumentationScope = beginRtiCall("getAttributeName");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const objectClassHandle = objectClassHandleValue(objectClass);
    if (!objectClassHandle) {
      throw InvalidObjectClassHandle(
          L"Get Attribute Name requires a valid ObjectClassHandle.");
    }
    auto const attributeHandle = attributeHandleValue(attribute);
    if (!attributeHandle) {
      throw InvalidAttributeHandle(
          L"Get Attribute Name requires a valid AttributeHandle.");
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
            L"Get Attribute Name requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }

    std::optional<std::wstring> attributeName;
    try {
      attributeName = processClient->lookupAttributeName(
          federationName,
          federateId,
          *objectClassHandle,
          *attributeHandle);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    if (!attributeName) {
      throw AttributeNotDefined(
          L"The supplied AttributeHandle is not defined for this object class.");
    }
    return *attributeName;
  }
#endif
  std::wstring result;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Get Attribute Name requires membership in a federation execution.");
    }

    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }

    auto const objectClassHandleValueResult = objectClassHandleValue(objectClass);
    if (!objectClassHandleValueResult) {
      throw InvalidObjectClassHandle(
          L"Get Attribute Name requires a valid ObjectClassHandle.");
    }
    auto const encodedObjectClassName = registry.objectClassNameFor(
        *joinedFederationName_,
        *objectClassHandleValueResult);
    if (!encodedObjectClassName) {
      throw InvalidObjectClassHandle(
          L"The supplied ObjectClassHandle is not known in this federation execution.");
    }

    auto const attributeHandleValueResult = attributeHandleValue(attribute);
    if (!attributeHandleValueResult) {
      throw InvalidAttributeHandle(L"Get Attribute Name requires a valid AttributeHandle.");
    }
    auto const encodedAttributeName = registry.attributeNameFor(
        *joinedFederationName_,
        *encodedObjectClassName,
        *attributeHandleValueResult);
    if (!encodedAttributeName) {
      throw AttributeNotDefined(
          L"The supplied AttributeHandle is not defined for this object class.");
    }
    auto const attributeNameValue = umbra::detail::wideFromUtf8(*encodedAttributeName);
    if (!attributeNameValue) {
      throw RTIinternalError(
          L"The embedded federation stored an attribute name that is not valid UTF-8 text.");
    }
    result = *attributeNameValue;
  }
  appendSuccessfulServiceReportToFileIfSelected(
      L"GetAttributeName",
      umbra::detail::MomServiceType::support_services,
      {{umbra::detail::MomArgumentType::object_class_handle,
        L"Object class handle",
        umbra::detail::formatMomObjectClassHandle(objectClass)},
       {umbra::detail::MomArgumentType::attribute_handle,
        L"Class attribute handle",
        umbra::detail::formatMomAttributeHandle(attribute)}},
      {umbra::detail::MomArgumentType::string,
       L"Class attribute name",
       umbra::detail::formatMomString(result)});
  return result;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Attribute Name", exception);
    appendFailedServiceReportToFileIfSelected(
        L"GetAttributeName",
        umbra::detail::MomServiceType::support_services,
        {{umbra::detail::MomArgumentType::object_class_handle,
          L"Object class handle",
          umbra::detail::formatMomObjectClassHandle(objectClass)},
         {umbra::detail::MomArgumentType::attribute_handle,
          L"Class attribute handle",
          umbra::detail::formatMomAttributeHandle(attribute)}},
        describeAmbassadorException(exception));
    throw;
  }
}

}  // namespace rti1516_2025::umbra_binding_detail
