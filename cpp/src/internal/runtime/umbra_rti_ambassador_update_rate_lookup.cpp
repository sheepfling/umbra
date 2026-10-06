#include "internal/runtime/umbra_rti_ambassador.hpp"

#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/ambassador_string_helpers.hpp"

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "internal/federation/federation_management_coordinator.hpp"
#include "internal/handles/attribute_handle.hpp"
#include "internal/handles/object_instance_handle.hpp"
#include "internal/runtime/utf8_string.hpp"
#endif

#include <mutex>
#include <utility>

namespace rti1516_2025::umbra_binding_detail {

double UmbraRtiAmbassador::getUpdateRateValue(
    std::wstring const& updateRateDesignator) {
  auto instrumentationScope = beginRtiCall("getUpdateRateValue");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const encodedDesignator =
        umbra::detail::utf8FromWide(updateRateDesignator);
    if (!encodedDesignator) {
      throw InvalidUpdateRateDesignator(
          L"The supplied update-rate designator is not valid UTF-8 text.");
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
            L"Get Update Rate Value requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    umbra::detail::ProcessFederationUpdateRateValueResult lookup;
    try {
      lookup = processClient->getUpdateRateValue(
          std::move(federationName), federateId, *encodedDesignator);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    switch (lookup.status) {
      case umbra::detail::ProcessFederationUpdateRateValueStatus::applied:
        return lookup.value;
      case umbra::detail::ProcessFederationUpdateRateValueStatus::
          federation_does_not_exist:
      case umbra::detail::ProcessFederationUpdateRateValueStatus::
          federate_not_member:
        throw FederateNotExecutionMember(
            L"The process federation no longer records this RTI ambassador as a member.");
      case umbra::detail::ProcessFederationUpdateRateValueStatus::
          invalid_update_rate_designator:
        throw InvalidUpdateRateDesignator(
            L"The supplied update-rate designator is not defined by the current FDD.");
      case umbra::detail::ProcessFederationUpdateRateValueStatus::
          object_instance_not_known:
      case umbra::detail::ProcessFederationUpdateRateValueStatus::
          attribute_not_defined:
      case umbra::detail::ProcessFederationUpdateRateValueStatus::
          inconsistent_catalog:
        throw RTIinternalError(
            L"The process federation could not resolve the current FDD update-rate table.");
    }
    throw RTIinternalError(
        L"The process federation returned an unknown update-rate query outcome.");
  }
#endif
  double result = 0.0;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Get Update Rate Value requires membership in a federation execution.");
    }

    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }

    auto const encodedDesignator = umbra::detail::utf8FromWide(updateRateDesignator);
    if (!encodedDesignator) {
      throw InvalidUpdateRateDesignator(
          L"The supplied update-rate designator is not valid UTF-8 text.");
    }
    auto const lookup = registry.updateRateValueForDesignator(
        *joinedFederationName_,
        *joinedFederateId_,
        *encodedDesignator);
    switch (lookup.status) {
      case umbra::detail::UpdateRateValueStatus::applied:
        result = lookup.value;
        break;
      case umbra::detail::UpdateRateValueStatus::federation_does_not_exist:
      case umbra::detail::UpdateRateValueStatus::federate_not_member:
        throw FederateNotExecutionMember(
            L"The embedded federation no longer records this RTI ambassador as a member.");
      case umbra::detail::UpdateRateValueStatus::invalid_update_rate_designator:
        throw InvalidUpdateRateDesignator(
            L"The supplied update-rate designator is not defined by the current FDD.");
      case umbra::detail::UpdateRateValueStatus::object_instance_not_known:
      case umbra::detail::UpdateRateValueStatus::attribute_not_defined:
        throw RTIinternalError(
            L"The embedded federation returned an invalid update-rate query outcome.");
      case umbra::detail::UpdateRateValueStatus::inconsistent_catalog:
        throw RTIinternalError(
            L"The embedded federation could not resolve the current FDD update-rate table.");
    }
  }
  appendSuccessfulServiceReportToFileIfSelected(
      L"GetUpdateRateValue",
      umbra::detail::MomServiceType::support_services,
      {{umbra::detail::MomArgumentType::string,
        L"Update rate name",
        umbra::detail::formatMomString(updateRateDesignator)}},
      {umbra::detail::MomArgumentType::number,
       L"Maximum update rate value",
       umbra::detail::formatMomNumber(std::to_wstring(result))});
  return result;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Update Rate Value", exception);
    appendFailedServiceReportToFileIfSelected(
        L"GetUpdateRateValue",
        umbra::detail::MomServiceType::support_services,
        {{umbra::detail::MomArgumentType::string,
          L"Update rate name",
          umbra::detail::formatMomString(updateRateDesignator)}},
        describeAmbassadorException(exception));
    throw;
  }
}

double UmbraRtiAmbassador::getUpdateRateValueForAttribute(
    ObjectInstanceHandle const& objectInstance,
    AttributeHandle const& attribute) {
  auto instrumentationScope = beginRtiCall("getUpdateRateValueForAttribute");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const objectInstanceValue = objectInstanceHandleValue(objectInstance);
    if (!objectInstanceValue) {
      throw ObjectInstanceNotKnown(
          L"Get Update Rate Value For Attribute requires a known ObjectInstanceHandle.");
    }
    auto const attributeValue = attributeHandleValue(attribute);
    if (!attributeValue) {
      throw AttributeNotDefined(
          L"Get Update Rate Value For Attribute requires a defined AttributeHandle.");
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
            L"Get Update Rate Value For Attribute requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    umbra::detail::ProcessFederationUpdateRateValueResult lookup;
    try {
      lookup = processClient->getUpdateRateValueForAttribute(
          std::move(federationName),
          federateId,
          *objectInstanceValue,
          *attributeValue);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    switch (lookup.status) {
      case umbra::detail::ProcessFederationUpdateRateValueStatus::applied:
        return lookup.value;
      case umbra::detail::ProcessFederationUpdateRateValueStatus::
          federation_does_not_exist:
      case umbra::detail::ProcessFederationUpdateRateValueStatus::
          federate_not_member:
        throw FederateNotExecutionMember(
            L"The process federation no longer records this RTI ambassador as a member.");
      case umbra::detail::ProcessFederationUpdateRateValueStatus::
          object_instance_not_known:
        throw ObjectInstanceNotKnown(
            L"The supplied ObjectInstanceHandle is not known to this federate.");
      case umbra::detail::ProcessFederationUpdateRateValueStatus::
          attribute_not_defined:
        throw AttributeNotDefined(
            L"The supplied AttributeHandle is not defined for this known object instance.");
      case umbra::detail::ProcessFederationUpdateRateValueStatus::
          invalid_update_rate_designator:
      case umbra::detail::ProcessFederationUpdateRateValueStatus::
          inconsistent_catalog:
        throw RTIinternalError(
            L"The process federation could not resolve the current FDD update-rate table.");
    }
    throw RTIinternalError(
        L"The process federation returned an unknown update-rate query outcome.");
  }
#endif
  double result = 0.0;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Get Update Rate Value For Attribute requires membership in a federation execution.");
    }

    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }

    auto const objectInstanceValue = objectInstanceHandleValue(objectInstance);
    if (!objectInstanceValue) {
      throw ObjectInstanceNotKnown(
          L"Get Update Rate Value For Attribute requires a known ObjectInstanceHandle.");
    }
    auto const attributeValue = attributeHandleValue(attribute);
    if (!attributeValue) {
      throw AttributeNotDefined(
          L"Get Update Rate Value For Attribute requires a defined AttributeHandle.");
    }

    auto const lookup = registry.updateRateValueForAttribute(
        *joinedFederationName_,
        *joinedFederateId_,
        *objectInstanceValue,
        *attributeValue);
    switch (lookup.status) {
      case umbra::detail::UpdateRateValueStatus::applied:
        result = lookup.value;
        break;
      case umbra::detail::UpdateRateValueStatus::federation_does_not_exist:
      case umbra::detail::UpdateRateValueStatus::federate_not_member:
        throw FederateNotExecutionMember(
            L"The embedded federation no longer records this RTI ambassador as a member.");
      case umbra::detail::UpdateRateValueStatus::object_instance_not_known:
        throw ObjectInstanceNotKnown(
            L"The supplied ObjectInstanceHandle is not known to this federate.");
      case umbra::detail::UpdateRateValueStatus::attribute_not_defined:
        throw AttributeNotDefined(
            L"The supplied AttributeHandle is not defined for this known object instance.");
      case umbra::detail::UpdateRateValueStatus::invalid_update_rate_designator:
        throw RTIinternalError(
            L"The embedded federation returned an invalid update-rate designator outcome.");
      case umbra::detail::UpdateRateValueStatus::inconsistent_catalog:
        throw RTIinternalError(
            L"The embedded federation could not resolve the current FDD update-rate table.");
    }
  }
  appendSuccessfulServiceReportToFileIfSelected(
      L"GetUpdateRateValueForAttribute",
      umbra::detail::MomServiceType::support_services,
      {{umbra::detail::MomArgumentType::object_instance_handle,
        L"Object instance handle",
        umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
       {umbra::detail::MomArgumentType::attribute_handle,
        L"Attribute handle",
        umbra::detail::formatMomAttributeHandle(attribute)}},
      {umbra::detail::MomArgumentType::number,
       L"Maximum update rate value",
       umbra::detail::formatMomNumber(std::to_wstring(result))});
  return result;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Update Rate Value For Attribute", exception);
    appendFailedServiceReportToFileIfSelected(
        L"GetUpdateRateValueForAttribute",
        umbra::detail::MomServiceType::support_services,
        {{umbra::detail::MomArgumentType::object_instance_handle,
          L"Object instance handle",
          umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
         {umbra::detail::MomArgumentType::attribute_handle,
          L"Attribute handle",
          umbra::detail::formatMomAttributeHandle(attribute)}},
        describeAmbassadorException(exception));
    throw;
  }
}

}  // namespace rti1516_2025::umbra_binding_detail
