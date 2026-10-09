#include "internal/runtime/umbra_rti_ambassador.hpp"

#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/ambassador_string_helpers.hpp"
#include "internal/runtime/umbra_rti_ambassador_service_failure_translation.hpp"

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "internal/federation/federation_management_coordinator.hpp"
#include "internal/handles/attribute_handle.hpp"
#include "internal/handles/object_class_handle.hpp"
#include "internal/runtime/utf8_string.hpp"
#endif

#include <mutex>
#include <utility>
#include <vector>

namespace rti1516_2025::umbra_binding_detail {
void UmbraRtiAmbassador::subscribeObjectClassAttributes(
    ObjectClassHandle const& objectClass,
    AttributeHandleSet const& attributes,
    bool active,
    std::wstring const& updateRateDesignator) {
  auto instrumentationScope = beginRtiCall("subscribeObjectClassAttributes");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const objectClassValueResult = objectClassHandleValue(objectClass);
    if (!objectClassValueResult) {
      throw ObjectClassNotDefined(
          L"Subscribe Object Class Attributes requires a defined ObjectClassHandle.");
    }
    auto const attributeValues = ambassadorAttributeHandleValues(attributes);
    if (!attributeValues) {
      throw AttributeNotDefined(
          L"Subscribe Object Class Attributes requires defined AttributeHandle values.");
    }
    auto const encodedUpdateRateDesignator =
        umbra::detail::utf8FromWide(updateRateDesignator);
    if (!encodedUpdateRateDesignator) {
      throw InvalidUpdateRateDesignator(
          L"Subscribe Object Class Attributes received an update-rate designator that is not valid UTF-8 text.");
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
            L"Subscribe Object Class Attributes requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    try {
      processClient->subscribeObjectClassAttributes(
          std::move(federationName),
          federateId,
          *objectClassValueResult,
          std::vector<std::uint64_t>{
              attributeValues->begin(), attributeValues->end()},
          active,
          *encodedUpdateRateDesignator);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif
  std::wstring federationName;
  std::vector<umbra::detail::DeclarationAdvisory> declarationAdvisories;
  std::vector<umbra::detail::ObjectInstanceDiscoveryRecipient> discoveries;
  std::vector<umbra::detail::ObjectInstanceDiscoveryRecipient> momDiscoveries;
  std::vector<umbra::detail::ObjectInstanceScopeChangeRecipient> changes;
  std::vector<umbra::detail::AttributeRelevanceAdvisoryRecipient>
      attributeRelevanceAdvisories;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Subscribe Object Class Attributes");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Subscribe Object Class Attributes requires membership in a federation execution.");
    }

    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const objectClassHandle = objectClassHandleValue(objectClass);
    if (!objectClassHandle) {
      throw ObjectClassNotDefined(
          L"Subscribe Object Class Attributes requires a defined ObjectClassHandle.");
    }
    auto const attributeHandles = ambassadorAttributeHandleValues(attributes);
    if (!attributeHandles) {
      throw AttributeNotDefined(
          L"Subscribe Object Class Attributes requires defined AttributeHandle values.");
    }
    auto const encodedUpdateRateDesignator =
        umbra::detail::utf8FromWide(updateRateDesignator);
    if (!encodedUpdateRateDesignator) {
      throw InvalidUpdateRateDesignator(
          L"Subscribe Object Class Attributes received an update-rate designator that is not valid UTF-8 text.");
    }
    auto result = registry.setObjectClassAttributeSubscriptionWithScopeChanges(
        *joinedFederationName_,
        *joinedFederateId_,
        *objectClassHandle,
        *attributeHandles,
        active,
        *encodedUpdateRateDesignator);
    if (result.status != umbra::detail::ObjectClassAttributeDeclarationStatus::applied) {
      throwObjectClassAttributeDeclarationFailureForFederationManagement(result.status);
    }

    federationName = *joinedFederationName_;
    changes = std::move(result.recipients);
    attributeRelevanceAdvisories = std::move(result.attributeRelevanceAdvisories);
    declarationAdvisories = registry.planDeclarationAdvisories(federationName);
    discoveries = registry.planObjectInstanceDiscoveriesForFederate(
        federationName,
        *joinedFederateId_);
    momDiscoveries = registry.planJoinedFederateMomObjectDiscoveriesForFederate(
        federationName,
        *joinedFederateId_);
  }
  // The §5.8 subscription has succeeded before this point. The public C++
  // `active` argument has the inverse meaning of the service narrative's
  // Optional passive subscription indicator. An empty update-rate designator
  // selects the default rate and therefore occupies its required Table 5
  // argument position as Null rather than as an empty String. Emit the
  // successful record outside native locks, before queued callbacks.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"SubscribeObjectClassAttributes",
      umbra::detail::MomServiceType::declaration_management,
      {{umbra::detail::MomArgumentType::object_class_handle,
        L"Object class designator",
        umbra::detail::formatMomObjectClassHandle(objectClass)},
       {umbra::detail::MomArgumentType::attribute_handle_set,
        L"Set of attribute designators",
        umbra::detail::formatMomAttributeHandleSet(attributes)},
       {umbra::detail::MomArgumentType::boolean,
        L"Optional passive subscription indicator",
        umbra::detail::formatMomBoolean(!active)},
       {updateRateDesignator.empty() ? umbra::detail::MomArgumentType::null_value
                                     : umbra::detail::MomArgumentType::string,
        L"Optional update rate designator",
        updateRateDesignator.empty() ? umbra::detail::formatMomNull()
                                     : umbra::detail::formatMomString(updateRateDesignator)}},
      true);
  queueAmbassadorDeclarationAdvisories(std::move(declarationAdvisories));
  queueAmbassadorObjectInstanceScopeChanges(std::move(changes), federationName);
  queueAmbassadorAttributeRelevanceAdvisories(std::move(attributeRelevanceAdvisories), federationName);
  queueAmbassadorObjectInstanceDiscoveries(std::move(discoveries), federationName);
  queueAmbassadorObjectInstanceDiscoveries(std::move(momDiscoveries), federationName);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Subscribe Object Class Attributes", exception);
    appendFailedServiceReportToFileIfSelected(
        L"SubscribeObjectClassAttributes",
        umbra::detail::MomServiceType::declaration_management,
        {{umbra::detail::MomArgumentType::object_class_handle,
          L"Object class designator",
          umbra::detail::formatMomObjectClassHandle(objectClass)},
         {umbra::detail::MomArgumentType::attribute_handle_set,
          L"Set of attribute designators",
          umbra::detail::formatMomAttributeHandleSet(attributes)},
         {umbra::detail::MomArgumentType::boolean,
          L"Optional passive subscription indicator",
          umbra::detail::formatMomBoolean(!active)},
         {updateRateDesignator.empty() ? umbra::detail::MomArgumentType::null_value
                                       : umbra::detail::MomArgumentType::string,
          L"Optional update rate designator",
          updateRateDesignator.empty() ? umbra::detail::formatMomNull()
                                       : umbra::detail::formatMomString(updateRateDesignator)}},
        describeAmbassadorException(exception),
        true);
    throw;
  }
}

void UmbraRtiAmbassador::unsubscribeObjectClass(
    ObjectClassHandle const& objectClass) {
  auto instrumentationScope = beginRtiCall("unsubscribeObjectClass");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const objectClassValueResult = objectClassHandleValue(objectClass);
    if (!objectClassValueResult) {
      throw ObjectClassNotDefined(
          L"Unsubscribe Object Class requires a defined ObjectClassHandle.");
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
            L"Unsubscribe Object Class requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    try {
      processClient->unsubscribeObjectClassAttributes(
          std::move(federationName),
          federateId,
          *objectClassValueResult,
          {});
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif
  std::wstring federationName;
  std::vector<umbra::detail::DeclarationAdvisory> declarationAdvisories;
  std::vector<umbra::detail::ObjectInstanceScopeChangeRecipient> changes;
  std::vector<umbra::detail::AttributeRelevanceAdvisoryRecipient>
      attributeRelevanceAdvisories;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Unsubscribe Object Class");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Unsubscribe Object Class requires membership in a federation execution.");
    }

    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const objectClassHandle = objectClassHandleValue(objectClass);
    if (!objectClassHandle) {
      throw ObjectClassNotDefined(
          L"Unsubscribe Object Class requires a defined ObjectClassHandle.");
    }
    auto const declaration = registry.objectClassAttributeDeclarationFor(
        *joinedFederationName_,
        *joinedFederateId_,
        *objectClassHandle);
    if (!declaration) {
      throw ObjectClassNotDefined(
          L"The supplied ObjectClassHandle is not defined in this federation execution.");
    }

    AttributeHandleSet ordinaryAttributes;
    for (auto const& [attributeHandle, active] : declaration->subscribedAttributes) {
      static_cast<void>(active);
      ordinaryAttributes.insert(makeAttributeHandle(attributeHandle));
    }
    auto const attributeHandles = ambassadorAttributeHandleValues(ordinaryAttributes);
    if (!attributeHandles) {
      throw AttributeNotDefined(
          L"Unsubscribe Object Class could not decode its subscribed attributes.");
    }
    auto result = registry.setObjectClassAttributeSubscriptionWithScopeChanges(
        *joinedFederationName_,
        *joinedFederateId_,
        *objectClassHandle,
        *attributeHandles,
        std::nullopt,
        std::string{});
    if (result.status != umbra::detail::ObjectClassAttributeDeclarationStatus::applied) {
      throwObjectClassAttributeDeclarationFailureForFederationManagement(result.status);
    }
    federationName = *joinedFederationName_;
    changes = std::move(result.recipients);
    attributeRelevanceAdvisories = std::move(result.attributeRelevanceAdvisories);
    declarationAdvisories = registry.planDeclarationAdvisories(federationName);
  }
  // The C++ whole-class overload represents §5.9 without its optional set of
  // attribute designators. Table 5 keeps that supplied-argument slot, and
  // §11.5.1 requires an unused optional argument to be reported as Null.
  // Emit outside native locks before separately queued callbacks.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"UnsubscribeObjectClassAttributes",
      umbra::detail::MomServiceType::declaration_management,
      {{umbra::detail::MomArgumentType::object_class_handle,
        L"Object class designator",
        umbra::detail::formatMomObjectClassHandle(objectClass)},
       {umbra::detail::MomArgumentType::null_value,
        L"Optional set of attribute designators",
        umbra::detail::formatMomNull()}},
      true);
  queueAmbassadorDeclarationAdvisories(std::move(declarationAdvisories));
  queueAmbassadorObjectInstanceScopeChanges(std::move(changes), federationName);
  queueAmbassadorAttributeRelevanceAdvisories(std::move(attributeRelevanceAdvisories), federationName);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Unsubscribe Object Class", exception);
    appendFailedServiceReportToFileIfSelected(
        L"UnsubscribeObjectClassAttributes",
        umbra::detail::MomServiceType::declaration_management,
        {{umbra::detail::MomArgumentType::object_class_handle,
          L"Object class designator",
          umbra::detail::formatMomObjectClassHandle(objectClass)},
         {umbra::detail::MomArgumentType::null_value,
          L"Optional set of attribute designators",
          umbra::detail::formatMomNull()}},
        describeAmbassadorException(exception),
        true);
    throw;
  }
}

void UmbraRtiAmbassador::unsubscribeObjectClassAttributes(
    ObjectClassHandle const& objectClass,
    AttributeHandleSet const& attributes) {
  auto instrumentationScope = beginRtiCall("unsubscribeObjectClassAttributes");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const objectClassValueResult = objectClassHandleValue(objectClass);
    if (!objectClassValueResult) {
      throw ObjectClassNotDefined(
          L"Unsubscribe Object Class Attributes requires a defined ObjectClassHandle.");
    }
    auto const attributeValues = ambassadorAttributeHandleValues(attributes);
    if (!attributeValues) {
      throw AttributeNotDefined(
          L"Unsubscribe Object Class Attributes requires defined AttributeHandle values.");
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
            L"Unsubscribe Object Class Attributes requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    try {
      processClient->unsubscribeObjectClassAttributes(
          std::move(federationName),
          federateId,
          *objectClassValueResult,
          std::vector<std::uint64_t>{
              attributeValues->begin(), attributeValues->end()});
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif
  std::wstring federationName;
  std::vector<umbra::detail::DeclarationAdvisory> declarationAdvisories;
  std::vector<umbra::detail::ObjectInstanceScopeChangeRecipient> changes;
  std::vector<umbra::detail::AttributeRelevanceAdvisoryRecipient>
      attributeRelevanceAdvisories;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Unsubscribe Object Class Attributes");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Unsubscribe Object Class Attributes requires membership in a federation execution.");
    }

    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const objectClassHandle = objectClassHandleValue(objectClass);
    if (!objectClassHandle) {
      throw ObjectClassNotDefined(
          L"Unsubscribe Object Class Attributes requires a defined ObjectClassHandle.");
    }
    auto const attributeHandles = ambassadorAttributeHandleValues(attributes);
    if (!attributeHandles) {
      throw AttributeNotDefined(
          L"Unsubscribe Object Class Attributes requires defined AttributeHandle values.");
    }
    auto result = registry.setObjectClassAttributeSubscriptionWithScopeChanges(
        *joinedFederationName_,
        *joinedFederateId_,
        *objectClassHandle,
        *attributeHandles,
        std::nullopt,
        std::string{});
    if (result.status != umbra::detail::ObjectClassAttributeDeclarationStatus::applied) {
      throwObjectClassAttributeDeclarationFailureForFederationManagement(result.status);
    }
    federationName = *joinedFederationName_;
    changes = std::move(result.recipients);
    attributeRelevanceAdvisories = std::move(result.attributeRelevanceAdvisories);
    declarationAdvisories = registry.planDeclarationAdvisories(federationName);
  }
  // The accepted §5.9 subset transition is the service-report boundary.
  // Unlike the whole-class overload, this entry point supplies the optional
  // set of attribute designators, including a supplied-empty set. Emit the
  // successful record outside native locks before queued callbacks.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"UnsubscribeObjectClassAttributes",
      umbra::detail::MomServiceType::declaration_management,
      {{umbra::detail::MomArgumentType::object_class_handle,
        L"Object class designator",
        umbra::detail::formatMomObjectClassHandle(objectClass)},
       {umbra::detail::MomArgumentType::attribute_handle_set,
        L"Optional set of attribute designators",
        umbra::detail::formatMomAttributeHandleSet(attributes)}},
      true);
  queueAmbassadorDeclarationAdvisories(std::move(declarationAdvisories));
  queueAmbassadorObjectInstanceScopeChanges(std::move(changes), federationName);
  queueAmbassadorAttributeRelevanceAdvisories(std::move(attributeRelevanceAdvisories), federationName);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Unsubscribe Object Class Attributes", exception);
    appendFailedServiceReportToFileIfSelected(
        L"UnsubscribeObjectClassAttributes",
        umbra::detail::MomServiceType::declaration_management,
        {{umbra::detail::MomArgumentType::object_class_handle,
          L"Object class designator",
          umbra::detail::formatMomObjectClassHandle(objectClass)},
         {umbra::detail::MomArgumentType::attribute_handle_set,
          L"Optional set of attribute designators",
          umbra::detail::formatMomAttributeHandleSet(attributes)}},
        describeAmbassadorException(exception),
        true);
    throw;
  }
}


void UmbraRtiAmbassador::subscribeObjectClassAttributesWithRegions(
    ObjectClassHandle const& objectClass,
    AttributeHandleSetRegionHandleSetPairVector const& attributesAndRegions,
    bool active,
    std::wstring const& updateRateDesignator) {
  auto instrumentationScope = beginRtiCall("subscribeObjectClassAttributesWithRegions");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const objectClassValueResult = objectClassHandleValue(objectClass);
    if (!objectClassValueResult) {
      throw ObjectClassNotDefined(
          L"Subscribe Object Class Attributes With Regions requires a defined ObjectClassHandle.");
    }
    auto pairValues = ambassadorAttributeRegionPairValues(attributesAndRegions);
    if (pairValues.invalidAttributeHandle) {
      throw AttributeNotDefined(
          L"Subscribe Object Class Attributes With Regions requires defined AttributeHandle values.");
    }
    if (pairValues.invalidRegionHandle) {
      throw InvalidRegion(
          L"Subscribe Object Class Attributes With Regions requires defined RegionHandle values.");
    }
    auto const encodedUpdateRateDesignator =
        umbra::detail::utf8FromWide(updateRateDesignator);
    if (!encodedUpdateRateDesignator) {
      throw InvalidUpdateRateDesignator(
          L"Subscribe Object Class Attributes With Regions received an update-rate designator that is not valid UTF-8 text.");
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
            L"Subscribe Object Class Attributes With Regions requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    try {
      processClient->subscribeObjectClassAttributesWithRegions(
          std::move(federationName),
          federateId,
          *objectClassValueResult,
          std::move(pairValues.values),
          active,
          *encodedUpdateRateDesignator);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif
  std::wstring federationName;
  std::vector<umbra::detail::DeclarationAdvisory> declarationAdvisories;
  std::vector<umbra::detail::ObjectInstanceDiscoveryRecipient> discoveries;
  std::vector<umbra::detail::ObjectInstanceDiscoveryRecipient> momDiscoveries;
  std::vector<umbra::detail::ObjectInstanceScopeChangeRecipient> changes;
  std::vector<umbra::detail::AttributeRelevanceAdvisoryRecipient>
      attributeRelevanceAdvisories;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Subscribe Object Class Attributes With Regions");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Subscribe Object Class Attributes With Regions requires membership in a federation execution.");
    }
    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const objectClassHandle = objectClassHandleValue(objectClass);
    if (!objectClassHandle) {
      throw ObjectClassNotDefined(
          L"Subscribe Object Class Attributes With Regions requires a defined ObjectClassHandle.");
    }
    auto pairValues = ambassadorAttributeRegionPairValues(attributesAndRegions);
    if (pairValues.invalidAttributeHandle) {
      throw AttributeNotDefined(
          L"Subscribe Object Class Attributes With Regions requires defined AttributeHandle values.");
    }
    if (pairValues.invalidRegionHandle) {
      throw InvalidRegion(
          L"Subscribe Object Class Attributes With Regions requires defined RegionHandle values.");
    }
    auto const encodedUpdateRateDesignator =
        umbra::detail::utf8FromWide(updateRateDesignator);
    if (!encodedUpdateRateDesignator) {
      throw InvalidUpdateRateDesignator(
          L"Subscribe Object Class Attributes With Regions received an update-rate designator that is not valid UTF-8 text.");
    }
    auto result = registry.setObjectClassAttributeRegionalSubscriptionWithScopeChanges(
        *joinedFederationName_,
        *joinedFederateId_,
        *objectClassHandle,
        pairValues.values,
        active,
        *encodedUpdateRateDesignator);
    if (result.status !=
        umbra::detail::RegionalObjectClassAttributeDeclarationStatus::applied) {
      service_failure_translation::throwRegionalObjectClassAttributeDeclarationFailure(result.status);
    }
    federationName = *joinedFederationName_;
    changes = std::move(result.recipients);
    attributeRelevanceAdvisories = std::move(result.attributeRelevanceAdvisories);
    declarationAdvisories = registry.planDeclarationAdvisories(federationName);
    discoveries = registry.planObjectInstanceDiscoveriesForFederate(
        federationName,
        *joinedFederateId_);
    // Regional declarations are evaluated against the RTI-owned
    // HLAfederate point as well as ordinary object instances.  The registry
    // planner performs the callback-time recheck, so this reservation is
    // safe for both evoked and immediate callback models.
    momDiscoveries = registry.planJoinedFederateMomObjectDiscoveriesForFederate(
        federationName,
        *joinedFederateId_);
  }
  // §9.8 is a successful-void DDM service.  Emit after the registry
  // transaction has released its locks, because HLA_IMMEDIATE observers may
  // re-enter this ambassador from the callback route.  The C++ `active`
  // argument is the inverse of the standard's optional passive-subscription
  // indicator; an empty update-rate designator occupies the Table 5 position
  // as Null.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"SubscribeObjectClassAttributesWithRegions",
      umbra::detail::MomServiceType::data_distribution_management,
      {{umbra::detail::MomArgumentType::object_class_handle,
        L"Object class designator",
        umbra::detail::formatMomObjectClassHandle(objectClass)},
       {umbra::detail::MomArgumentType::attribute_set_region_set_pair_list,
        L"Collection of attribute designator set and region designator set pairs",
        umbra::detail::formatMomAttributeSetRegionSetPairList(attributesAndRegions)},
       {umbra::detail::MomArgumentType::boolean,
        L"Optional passive subscription indicator",
        umbra::detail::formatMomBoolean(!active)},
       {updateRateDesignator.empty() ? umbra::detail::MomArgumentType::null_value
                                     : umbra::detail::MomArgumentType::string,
        L"Optional update rate designator",
        updateRateDesignator.empty() ? umbra::detail::formatMomNull()
                                     : umbra::detail::formatMomString(updateRateDesignator)}},
      true);
  queueAmbassadorObjectInstanceScopeChanges(std::move(changes), federationName);
  queueAmbassadorAttributeRelevanceAdvisories(std::move(attributeRelevanceAdvisories), federationName);
  queueAmbassadorDeclarationAdvisories(std::move(declarationAdvisories));
  queueAmbassadorObjectInstanceDiscoveries(std::move(discoveries), federationName);
  queueAmbassadorObjectInstanceDiscoveries(std::move(momDiscoveries), federationName);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Subscribe Object Class Attributes With Regions", exception);
    appendFailedServiceReportToFileIfSelected(
        L"SubscribeObjectClassAttributesWithRegions",
        umbra::detail::MomServiceType::data_distribution_management,
        {{umbra::detail::MomArgumentType::object_class_handle,
          L"Object class designator",
          umbra::detail::formatMomObjectClassHandle(objectClass)},
         {umbra::detail::MomArgumentType::attribute_set_region_set_pair_list,
          L"Collection of attribute designator set and region designator set pairs",
          umbra::detail::formatMomAttributeSetRegionSetPairList(attributesAndRegions)},
         {umbra::detail::MomArgumentType::boolean,
          L"Optional passive subscription indicator",
          umbra::detail::formatMomBoolean(!active)},
         {updateRateDesignator.empty() ? umbra::detail::MomArgumentType::null_value
                                       : umbra::detail::MomArgumentType::string,
          L"Optional update rate designator",
          updateRateDesignator.empty() ? umbra::detail::formatMomNull()
                                       : umbra::detail::formatMomString(updateRateDesignator)}},
        describeAmbassadorException(exception));
    throw;
  }
}

void UmbraRtiAmbassador::unsubscribeObjectClassAttributesWithRegions(
    ObjectClassHandle const& objectClass,
    AttributeHandleSetRegionHandleSetPairVector const& attributesAndRegions) {
  auto instrumentationScope = beginRtiCall("unsubscribeObjectClassAttributesWithRegions");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const objectClassValueResult = objectClassHandleValue(objectClass);
    if (!objectClassValueResult) {
      throw ObjectClassNotDefined(
          L"Unsubscribe Object Class Attributes With Regions requires a defined ObjectClassHandle.");
    }
    auto pairValues = ambassadorAttributeRegionPairValues(attributesAndRegions);
    if (pairValues.invalidAttributeHandle) {
      throw AttributeNotDefined(
          L"Unsubscribe Object Class Attributes With Regions requires defined AttributeHandle values.");
    }
    if (pairValues.invalidRegionHandle) {
      throw InvalidRegion(
          L"Unsubscribe Object Class Attributes With Regions requires defined RegionHandle values.");
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
            L"Unsubscribe Object Class Attributes With Regions requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    try {
      processClient->unsubscribeObjectClassAttributesWithRegions(
          std::move(federationName),
          federateId,
          *objectClassValueResult,
          std::move(pairValues.values));
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif
  std::wstring federationName;
  std::vector<umbra::detail::DeclarationAdvisory> declarationAdvisories;
  std::vector<umbra::detail::ObjectInstanceScopeChangeRecipient> changes;
  std::vector<umbra::detail::AttributeRelevanceAdvisoryRecipient>
      attributeRelevanceAdvisories;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Unsubscribe Object Class Attributes With Regions");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Unsubscribe Object Class Attributes With Regions requires membership in a federation execution.");
    }
    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const objectClassHandle = objectClassHandleValue(objectClass);
    if (!objectClassHandle) {
      throw ObjectClassNotDefined(
          L"Unsubscribe Object Class Attributes With Regions requires a defined ObjectClassHandle.");
    }
    auto pairValues = ambassadorAttributeRegionPairValues(attributesAndRegions);
    if (pairValues.invalidAttributeHandle) {
      throw AttributeNotDefined(
          L"Unsubscribe Object Class Attributes With Regions requires defined AttributeHandle values.");
    }
    if (pairValues.invalidRegionHandle) {
      throw InvalidRegion(
          L"Unsubscribe Object Class Attributes With Regions requires defined RegionHandle values.");
    }
    auto result = registry.removeObjectClassAttributeRegionalSubscriptionWithScopeChanges(
        *joinedFederationName_,
        *joinedFederateId_,
        *objectClassHandle,
        pairValues.values);
    if (result.status !=
        umbra::detail::RegionalObjectClassAttributeDeclarationStatus::applied) {
      service_failure_translation::throwRegionalObjectClassAttributeDeclarationFailure(result.status);
    }
    federationName = *joinedFederationName_;
    changes = std::move(result.recipients);
    attributeRelevanceAdvisories = std::move(result.attributeRelevanceAdvisories);
    declarationAdvisories = registry.planDeclarationAdvisories(federationName);
  }
  // §9.9 has the object-class and AttributeRegionAssociationList supplied
  // shape and a successful-void return.  Keep the public MOM route outside
  // the native transaction locks; accepted empty pairs remain real calls.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"UnsubscribeObjectClassAttributesWithRegions",
      umbra::detail::MomServiceType::data_distribution_management,
      {{umbra::detail::MomArgumentType::object_class_handle,
        L"Object class designator",
        umbra::detail::formatMomObjectClassHandle(objectClass)},
       {umbra::detail::MomArgumentType::attribute_set_region_set_pair_list,
        L"Collection of attribute designator set and region designator set pairs",
        umbra::detail::formatMomAttributeSetRegionSetPairList(attributesAndRegions)}},
      true);
  queueAmbassadorObjectInstanceScopeChanges(std::move(changes), federationName);
  queueAmbassadorAttributeRelevanceAdvisories(std::move(attributeRelevanceAdvisories), federationName);
  queueAmbassadorDeclarationAdvisories(std::move(declarationAdvisories));
  } catch (Exception const& exception) {
    emitExceptionReport(L"Unsubscribe Object Class Attributes With Regions", exception);
    appendFailedServiceReportToFileIfSelected(
        L"UnsubscribeObjectClassAttributesWithRegions",
        umbra::detail::MomServiceType::data_distribution_management,
        {{umbra::detail::MomArgumentType::object_class_handle,
          L"Object class designator",
          umbra::detail::formatMomObjectClassHandle(objectClass)},
         {umbra::detail::MomArgumentType::attribute_set_region_set_pair_list,
          L"Collection of attribute designator set and region designator set pairs",
          umbra::detail::formatMomAttributeSetRegionSetPairList(attributesAndRegions)}},
        describeAmbassadorException(exception));
    throw;
  }
}

}  // namespace rti1516_2025::umbra_binding_detail
