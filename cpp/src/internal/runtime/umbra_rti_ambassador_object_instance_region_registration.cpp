#include "internal/runtime/umbra_rti_ambassador.hpp"

#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/ambassador_string_helpers.hpp"

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "internal/federation/federation_management_coordinator.hpp"
#include "internal/handles/object_class_handle.hpp"
#include "internal/handles/object_instance_handle.hpp"
#endif

#include <mutex>
#include <utility>
#include <vector>

namespace rti1516_2025::umbra_binding_detail {
ObjectInstanceHandle UmbraRtiAmbassador::registerObjectInstanceWithRegions(
    ObjectClassHandle const& objectClass,
    AttributeHandleSetRegionHandleSetPairVector const& attributesAndRegions) {
  auto instrumentationScope = beginRtiCall("registerObjectInstanceWithRegions");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const objectClassValueResult = objectClassHandleValue(objectClass);
    if (!objectClassValueResult) {
      throw ObjectClassNotDefined(
          L"Register Object Instance With Regions requires a defined ObjectClassHandle.");
    }
    auto pairValues = ambassadorAttributeRegionPairValues(attributesAndRegions);
    if (pairValues.invalidAttributeHandle) {
      throw AttributeNotDefined(
          L"Register Object Instance With Regions requires defined AttributeHandle values.");
    }
    if (pairValues.invalidRegionHandle) {
      throw InvalidRegion(
          L"Register Object Instance With Regions requires defined RegionHandle values.");
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
            L"Register Object Instance With Regions requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    umbra::detail::ProcessFederationRegisterObjectInstanceResult registration;
    try {
      registration = processClient->registerObjectInstanceWithRegions(
          std::move(federationName),
          federateId,
          *objectClassValueResult,
          std::move(pairValues.values));
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    if (registration.status !=
        umbra::detail::ObjectInstanceRegistrationStatus::applied) {
      throwObjectInstanceRegistrationFailureForFederationManagement(registration.status);
    }
    if (registration.objectInstanceHandle == 0U ||
        registration.objectInstanceName.empty()) {
      throw RTIinternalError(
          L"The private process endpoint returned an invalid regional object-instance registration.");
    }
    return makeObjectInstanceHandle(registration.objectInstanceHandle);
  }
#endif
  std::wstring federationName;
  std::vector<umbra::detail::ObjectInstanceDiscoveryRecipient> discoveries;
  std::uint64_t objectInstanceHandle = 0;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Register Object Instance With Regions");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Register Object Instance With Regions requires membership in a federation execution.");
    }
    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const objectClassHandle = objectClassHandleValue(objectClass);
    if (!objectClassHandle) {
      throw ObjectClassNotDefined(
          L"Register Object Instance With Regions requires a defined ObjectClassHandle.");
    }
    auto pairValues = ambassadorAttributeRegionPairValues(attributesAndRegions);
    if (pairValues.invalidAttributeHandle) {
      throw AttributeNotDefined(
          L"Register Object Instance With Regions requires defined AttributeHandle values.");
    }
    if (pairValues.invalidRegionHandle) {
      throw InvalidRegion(
          L"Register Object Instance With Regions requires defined RegionHandle values.");
    }
    auto const registration = registry.registerObjectInstance(
        *joinedFederationName_,
        *joinedFederateId_,
        *objectClassHandle,
        &pairValues.values);
    if (registration.status != umbra::detail::ObjectInstanceRegistrationStatus::applied) {
      throwObjectInstanceRegistrationFailureForFederationManagement(registration.status);
    }
    federationName = *joinedFederationName_;
    objectInstanceHandle = registration.objectInstanceHandle;
    discoveries = registry.planObjectInstanceDiscoveriesForInstance(
        federationName,
        objectInstanceHandle);
  }
  auto const result = makeObjectInstanceHandle(objectInstanceHandle);
  appendSuccessfulServiceReportToFileIfSelected(
      L"RegisterObjectInstanceWithRegions",
      umbra::detail::MomServiceType::object_management,
      {{umbra::detail::MomArgumentType::object_class_handle,
        L"Object class designator",
        umbra::detail::formatMomObjectClassHandle(objectClass)},
       {umbra::detail::MomArgumentType::attribute_set_region_set_pair_list,
        L"Collection of attribute designator set and region designator set pairs",
        umbra::detail::formatMomAttributeSetRegionSetPairList(attributesAndRegions)}},
      {umbra::detail::MomArgumentType::object_instance_handle,
       L"Object instance designator",
       umbra::detail::formatMomObjectInstanceHandle(result)},
      true);
  queueAmbassadorObjectInstanceDiscoveries(std::move(discoveries), federationName);
  return result;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Register Object Instance With Regions", exception);
    appendFailedServiceReportToFileIfSelected(
        L"RegisterObjectInstanceWithRegions",
        umbra::detail::MomServiceType::object_management,
        {{umbra::detail::MomArgumentType::object_class_handle,
          L"Object class designator",
          umbra::detail::formatMomObjectClassHandle(objectClass)},
         {umbra::detail::MomArgumentType::attribute_set_region_set_pair_list,
          L"Collection of attribute designator set and region designator set pairs",
          umbra::detail::formatMomAttributeSetRegionSetPairList(attributesAndRegions)}},
        describeAmbassadorException(exception),
        true);
    throw;
  }
}

ObjectInstanceHandle UmbraRtiAmbassador::registerObjectInstanceWithRegions(
    ObjectClassHandle const& objectClass,
    AttributeHandleSetRegionHandleSetPairVector const& attributesAndRegions,
    std::wstring const& objectInstanceName) {
  auto instrumentationScope = beginRtiCall("registerObjectInstanceWithRegions");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const objectClassValueResult = objectClassHandleValue(objectClass);
    if (!objectClassValueResult) {
      throw ObjectClassNotDefined(
          L"Register Object Instance With Regions requires a defined ObjectClassHandle.");
    }
    auto pairValues = ambassadorAttributeRegionPairValues(attributesAndRegions);
    if (pairValues.invalidAttributeHandle) {
      throw AttributeNotDefined(
          L"Register Object Instance With Regions requires defined AttributeHandle values.");
    }
    if (pairValues.invalidRegionHandle) {
      throw InvalidRegion(
          L"Register Object Instance With Regions requires defined RegionHandle values.");
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
            L"Register Object Instance With Regions requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    umbra::detail::ProcessFederationRegisterObjectInstanceResult registration;
    try {
      registration = processClient->registerObjectInstanceWithRegions(
          std::move(federationName),
          federateId,
          *objectClassValueResult,
          std::move(pairValues.values),
          objectInstanceName);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    if (registration.status !=
        umbra::detail::ObjectInstanceRegistrationStatus::applied) {
      throwObjectInstanceRegistrationFailureForFederationManagement(registration.status);
    }
    if (registration.objectInstanceHandle == 0U ||
        registration.objectInstanceName.empty()) {
      throw RTIinternalError(
          L"The private process endpoint returned an invalid regional object-instance registration.");
    }
    return makeObjectInstanceHandle(registration.objectInstanceHandle);
  }
#endif
  std::wstring federationName;
  std::vector<umbra::detail::ObjectInstanceDiscoveryRecipient> discoveries;
  std::uint64_t objectInstanceHandle = 0;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Register Object Instance With Regions");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Register Object Instance With Regions requires membership in a federation execution.");
    }
    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const objectClassHandle = objectClassHandleValue(objectClass);
    if (!objectClassHandle) {
      throw ObjectClassNotDefined(
          L"Register Object Instance With Regions requires a defined ObjectClassHandle.");
    }
    auto pairValues = ambassadorAttributeRegionPairValues(attributesAndRegions);
    if (pairValues.invalidAttributeHandle) {
      throw AttributeNotDefined(
          L"Register Object Instance With Regions requires defined AttributeHandle values.");
    }
    if (pairValues.invalidRegionHandle) {
      throw InvalidRegion(
          L"Register Object Instance With Regions requires defined RegionHandle values.");
    }
    auto const registration = registry.registerObjectInstance(
        *joinedFederationName_,
        *joinedFederateId_,
        *objectClassHandle,
        &pairValues.values,
        &objectInstanceName);
    if (registration.status != umbra::detail::ObjectInstanceRegistrationStatus::applied) {
      throwObjectInstanceRegistrationFailureForFederationManagement(registration.status);
    }
    federationName = *joinedFederationName_;
    objectInstanceHandle = registration.objectInstanceHandle;
    discoveries = registry.planObjectInstanceDiscoveriesForInstance(
        federationName,
        objectInstanceHandle);
  }
  auto const result = makeObjectInstanceHandle(objectInstanceHandle);
  appendSuccessfulServiceReportToFileIfSelected(
      L"RegisterObjectInstanceWithRegions",
      umbra::detail::MomServiceType::object_management,
      {{umbra::detail::MomArgumentType::object_class_handle,
        L"Object class designator",
        umbra::detail::formatMomObjectClassHandle(objectClass)},
       {umbra::detail::MomArgumentType::attribute_set_region_set_pair_list,
        L"Collection of attribute designator set and region designator set pairs",
        umbra::detail::formatMomAttributeSetRegionSetPairList(attributesAndRegions)},
       {umbra::detail::MomArgumentType::string,
        L"Object instance name",
        umbra::detail::formatMomString(objectInstanceName)}},
      {umbra::detail::MomArgumentType::object_instance_handle,
       L"Object instance designator",
       umbra::detail::formatMomObjectInstanceHandle(result)},
      true);
  queueAmbassadorObjectInstanceDiscoveries(std::move(discoveries), federationName);
  return result;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Register Object Instance With Regions", exception);
    appendFailedServiceReportToFileIfSelected(
        L"RegisterObjectInstanceWithRegions",
        umbra::detail::MomServiceType::object_management,
        {{umbra::detail::MomArgumentType::object_class_handle,
          L"Object class designator",
          umbra::detail::formatMomObjectClassHandle(objectClass)},
         {umbra::detail::MomArgumentType::attribute_set_region_set_pair_list,
          L"Collection of attribute designator set and region designator set pairs",
          umbra::detail::formatMomAttributeSetRegionSetPairList(attributesAndRegions)},
         {umbra::detail::MomArgumentType::string,
          L"Object instance name",
          umbra::detail::formatMomString(objectInstanceName)}},
        describeAmbassadorException(exception),
        true);
    throw;
  }
}


ObjectInstanceHandle UmbraRtiAmbassador::registerObjectInstance(
    ObjectClassHandle const& objectClass) {
  auto instrumentationScope = beginRtiCall("registerObjectInstance");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const objectClassValueResult = objectClassHandleValue(objectClass);
    if (!objectClassValueResult) {
      throw ObjectClassNotDefined(
          L"Register Object Instance requires a defined ObjectClassHandle.");
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
            L"Register Object Instance requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    umbra::detail::ProcessFederationRegisterObjectInstanceResult registration;
    try {
      registration = processClient->registerObjectInstance(
          std::move(federationName), federateId, *objectClassValueResult);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    if (registration.status !=
        umbra::detail::ObjectInstanceRegistrationStatus::applied) {
      throwObjectInstanceRegistrationFailureForFederationManagement(registration.status);
    }
    if (registration.objectInstanceHandle == 0U ||
        registration.objectInstanceName.empty()) {
      throw RTIinternalError(
          L"The private process endpoint returned an invalid object-instance registration.");
    }
    return makeObjectInstanceHandle(registration.objectInstanceHandle);
  }
#endif
  std::wstring federationName;
  std::vector<umbra::detail::ObjectInstanceDiscoveryRecipient> discoveries;
  std::uint64_t objectInstanceHandle = 0;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Register Object Instance");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Register Object Instance requires membership in a federation execution.");
    }

    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const objectClassHandle = objectClassHandleValue(objectClass);
    if (!objectClassHandle) {
      throw ObjectClassNotDefined(
          L"Register Object Instance requires a defined ObjectClassHandle.");
    }

    auto const registration = registry.registerObjectInstance(
        *joinedFederationName_,
        *joinedFederateId_,
        *objectClassHandle);
    if (registration.status != umbra::detail::ObjectInstanceRegistrationStatus::applied) {
      throwObjectInstanceRegistrationFailureForFederationManagement(registration.status);
    }

    federationName = *joinedFederationName_;
    objectInstanceHandle = registration.objectInstanceHandle;
    discoveries = registry.planObjectInstanceDiscoveriesForInstance(
        federationName,
        objectInstanceHandle);
  }
  auto const result = makeObjectInstanceHandle(objectInstanceHandle);
  appendSuccessfulServiceReportToFileIfSelected(
      L"RegisterObjectInstance",
      umbra::detail::MomServiceType::object_management,
      {{umbra::detail::MomArgumentType::object_class_handle,
        L"Object class designator",
        umbra::detail::formatMomObjectClassHandle(objectClass)}},
      {umbra::detail::MomArgumentType::object_instance_handle,
       L"Object instance designator",
       umbra::detail::formatMomObjectInstanceHandle(result)},
      true);
  queueAmbassadorObjectInstanceDiscoveries(std::move(discoveries), federationName);
  return result;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Register Object Instance", exception);
    appendFailedServiceReportToFileIfSelected(
        L"RegisterObjectInstance",
        umbra::detail::MomServiceType::object_management,
        {{umbra::detail::MomArgumentType::object_class_handle,
          L"Object class designator",
          umbra::detail::formatMomObjectClassHandle(objectClass)}},
        describeAmbassadorException(exception),
        true);
    throw;
  }
}

ObjectInstanceHandle UmbraRtiAmbassador::registerObjectInstance(
    ObjectClassHandle const& objectClass,
    std::wstring const& objectInstanceName) {
  auto instrumentationScope = beginRtiCall("registerObjectInstance");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const objectClassValueResult = objectClassHandleValue(objectClass);
    if (!objectClassValueResult) {
      throw ObjectClassNotDefined(
          L"Register Object Instance requires a defined ObjectClassHandle.");
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
            L"Register Object Instance requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    umbra::detail::ProcessFederationRegisterObjectInstanceResult registration;
    try {
      registration = processClient->registerObjectInstance(
          std::move(federationName),
          federateId,
          *objectClassValueResult,
          objectInstanceName);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    if (registration.status !=
        umbra::detail::ObjectInstanceRegistrationStatus::applied) {
      throwObjectInstanceRegistrationFailureForFederationManagement(registration.status);
    }
    if (registration.objectInstanceHandle == 0U ||
        registration.objectInstanceName.empty()) {
      throw RTIinternalError(
          L"The private process endpoint returned an invalid object-instance registration.");
    }
    return makeObjectInstanceHandle(registration.objectInstanceHandle);
  }
#endif
  std::wstring federationName;
  std::vector<umbra::detail::ObjectInstanceDiscoveryRecipient> discoveries;
  std::uint64_t objectInstanceHandle = 0;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Register Object Instance");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Register Object Instance requires membership in a federation execution.");
    }

    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const objectClassHandle = objectClassHandleValue(objectClass);
    if (!objectClassHandle) {
      throw ObjectClassNotDefined(
          L"Register Object Instance requires a defined ObjectClassHandle.");
    }

    auto const registration = registry.registerObjectInstance(
        *joinedFederationName_,
        *joinedFederateId_,
        *objectClassHandle,
        nullptr,
        &objectInstanceName);
    if (registration.status != umbra::detail::ObjectInstanceRegistrationStatus::applied) {
      throwObjectInstanceRegistrationFailureForFederationManagement(registration.status);
    }

    federationName = *joinedFederationName_;
    objectInstanceHandle = registration.objectInstanceHandle;
    discoveries = registry.planObjectInstanceDiscoveriesForInstance(
        federationName,
        objectInstanceHandle);
  }
  auto const result = makeObjectInstanceHandle(objectInstanceHandle);
  appendSuccessfulServiceReportToFileIfSelected(
      L"RegisterObjectInstance",
      umbra::detail::MomServiceType::object_management,
      {{umbra::detail::MomArgumentType::object_class_handle,
        L"Object class designator",
        umbra::detail::formatMomObjectClassHandle(objectClass)},
       {umbra::detail::MomArgumentType::string,
        L"Object instance name",
        umbra::detail::formatMomString(objectInstanceName)}},
      {umbra::detail::MomArgumentType::object_instance_handle,
       L"Object instance designator",
       umbra::detail::formatMomObjectInstanceHandle(result)},
      true);
  queueAmbassadorObjectInstanceDiscoveries(std::move(discoveries), federationName);
  return result;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Register Object Instance", exception);
    appendFailedServiceReportToFileIfSelected(
        L"RegisterObjectInstance",
        umbra::detail::MomServiceType::object_management,
        {{umbra::detail::MomArgumentType::object_class_handle,
          L"Object class designator",
          umbra::detail::formatMomObjectClassHandle(objectClass)},
         {umbra::detail::MomArgumentType::string,
          L"Object instance name",
          umbra::detail::formatMomString(objectInstanceName)}},
        describeAmbassadorException(exception),
        true);
    throw;
  }
}

}  // namespace rti1516_2025::umbra_binding_detail
