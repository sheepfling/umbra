#include "internal/runtime/umbra_rti_ambassador.hpp"

#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/ambassador_string_helpers.hpp"

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "internal/federation/federation_management_coordinator.hpp"
#include "internal/handles/object_instance_handle.hpp"
#endif

#include <mutex>
#include <utility>
#include <vector>

namespace rti1516_2025::umbra_binding_detail {
void UmbraRtiAmbassador::associateRegionsForUpdates(
    ObjectInstanceHandle const& objectInstance,
    AttributeHandleSetRegionHandleSetPairVector const& attributesAndRegions) {
  auto instrumentationScope = beginRtiCall("associateRegionsForUpdates");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const objectInstanceValueResult = objectInstanceHandleValue(objectInstance);
    if (!objectInstanceValueResult) {
      throw ObjectInstanceNotKnown(
          L"Associate Regions For Updates requires a known ObjectInstanceHandle.");
    }
    auto pairValues = ambassadorAttributeRegionPairValues(attributesAndRegions);
    if (pairValues.invalidAttributeHandle) {
      throw AttributeNotDefined(
          L"Associate Regions For Updates requires defined AttributeHandle values.");
    }
    if (pairValues.invalidRegionHandle) {
      throw InvalidRegion(
          L"Associate Regions For Updates requires defined RegionHandle values.");
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
            L"Associate Regions For Updates requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    umbra::detail::ProcessFederationObjectInstanceRegionAssociationResult result;
    try {
      result = processClient->associateRegionsForUpdates(
          std::move(federationName),
          federateId,
          *objectInstanceValueResult,
          std::move(pairValues.values));
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    if (result.status !=
        umbra::detail::ObjectInstanceRegionAssociationStatus::applied) {
      throwObjectInstanceRegionAssociationFailureForFederationManagement(result.status);
    }
    return;
  }
#endif
  std::wstring federationName;
  std::vector<umbra::detail::ObjectInstanceDiscoveryRecipient> discoveries;
  std::vector<umbra::detail::ObjectInstanceScopeChangeRecipient> changes;
  std::vector<umbra::detail::AttributeRelevanceAdvisoryRecipient>
      attributeRelevanceAdvisories;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Associate Regions For Updates");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Associate Regions For Updates requires membership in a federation execution.");
    }
    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const objectInstanceHandle = objectInstanceHandleValue(objectInstance);
    if (!objectInstanceHandle) {
      throw ObjectInstanceNotKnown(
          L"Associate Regions For Updates requires a known ObjectInstanceHandle.");
    }
    auto pairValues = ambassadorAttributeRegionPairValues(attributesAndRegions);
    if (pairValues.invalidAttributeHandle) {
      throw AttributeNotDefined(
          L"Associate Regions For Updates requires defined AttributeHandle values.");
    }
    if (pairValues.invalidRegionHandle) {
      throw InvalidRegion(
          L"Associate Regions For Updates requires defined RegionHandle values.");
    }
    auto result = registry.associateRegionsForUpdatesWithScopeChanges(
        *joinedFederationName_,
        *joinedFederateId_,
        *objectInstanceHandle,
        pairValues.values);
    if (result.status != umbra::detail::ObjectInstanceRegionAssociationStatus::applied) {
      throwObjectInstanceRegionAssociationFailureForFederationManagement(result.status);
    }
    federationName = *joinedFederationName_;
    discoveries = std::move(result.discoveries);
    changes = std::move(result.recipients);
    attributeRelevanceAdvisories = std::move(result.attributeRelevanceAdvisories);
  }
  // §9.6 returns None and supplies an ObjectInstanceHandle plus the
  // AttributeSetRegionSetPairList type-4 collection. Emit the accepted
  // transition after releasing native locks and before callbacks are queued.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"AssociateRegionsForUpdates",
      umbra::detail::MomServiceType::data_distribution_management,
      {{umbra::detail::MomArgumentType::object_instance_handle,
        L"Object instance designator",
        umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
       {umbra::detail::MomArgumentType::attribute_set_region_set_pair_list,
        L"Collection of attribute designator set and region designator set pairs",
        umbra::detail::formatMomAttributeSetRegionSetPairList(attributesAndRegions)}},
      true);
  queueAmbassadorObjectInstanceDiscoveries(std::move(discoveries), federationName);
  queueAmbassadorObjectInstanceScopeChanges(std::move(changes), federationName);
  queueAmbassadorAttributeRelevanceAdvisories(
      std::move(attributeRelevanceAdvisories), federationName);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Associate Regions For Updates", exception);
    appendFailedServiceReportToFileIfSelected(
        L"AssociateRegionsForUpdates",
        umbra::detail::MomServiceType::data_distribution_management,
        {{umbra::detail::MomArgumentType::object_instance_handle,
          L"Object instance designator",
          umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
         {umbra::detail::MomArgumentType::attribute_set_region_set_pair_list,
          L"Collection of attribute designator set and region designator set pairs",
          umbra::detail::formatMomAttributeSetRegionSetPairList(attributesAndRegions)}},
        describeAmbassadorException(exception));
    throw;
  }
}

void UmbraRtiAmbassador::unassociateRegionsForUpdates(
    ObjectInstanceHandle const& objectInstance,
    AttributeHandleSetRegionHandleSetPairVector const& attributesAndRegions) {
  auto instrumentationScope = beginRtiCall("unassociateRegionsForUpdates");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const objectInstanceValueResult = objectInstanceHandleValue(objectInstance);
    if (!objectInstanceValueResult) {
      throw ObjectInstanceNotKnown(
          L"Unassociate Regions For Updates requires a known ObjectInstanceHandle.");
    }
    auto pairValues = ambassadorAttributeRegionPairValues(attributesAndRegions);
    if (pairValues.invalidAttributeHandle) {
      throw AttributeNotDefined(
          L"Unassociate Regions For Updates requires defined AttributeHandle values.");
    }
    if (pairValues.invalidRegionHandle) {
      throw InvalidRegion(
          L"Unassociate Regions For Updates requires defined RegionHandle values.");
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
            L"Unassociate Regions For Updates requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    umbra::detail::ProcessFederationObjectInstanceRegionAssociationResult result;
    try {
      result = processClient->unassociateRegionsForUpdates(
          std::move(federationName),
          federateId,
          *objectInstanceValueResult,
          std::move(pairValues.values));
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    if (result.status !=
        umbra::detail::ObjectInstanceRegionAssociationStatus::applied) {
      throwObjectInstanceRegionAssociationFailureForFederationManagement(result.status);
    }
    return;
  }
#endif
  std::wstring federationName;
  std::vector<umbra::detail::ObjectInstanceDiscoveryRecipient> discoveries;
  std::vector<umbra::detail::ObjectInstanceScopeChangeRecipient> changes;
  std::vector<umbra::detail::AttributeRelevanceAdvisoryRecipient>
      attributeRelevanceAdvisories;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Unassociate Regions For Updates");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Unassociate Regions For Updates requires membership in a federation execution.");
    }
    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const objectInstanceHandle = objectInstanceHandleValue(objectInstance);
    if (!objectInstanceHandle) {
      throw ObjectInstanceNotKnown(
          L"Unassociate Regions For Updates requires a known ObjectInstanceHandle.");
    }
    auto pairValues = ambassadorAttributeRegionPairValues(attributesAndRegions);
    if (pairValues.invalidAttributeHandle) {
      throw AttributeNotDefined(
          L"Unassociate Regions For Updates requires defined AttributeHandle values.");
    }
    if (pairValues.invalidRegionHandle) {
      throw InvalidRegion(
          L"Unassociate Regions For Updates requires defined RegionHandle values.");
    }
    auto result = registry.unassociateRegionsForUpdatesWithScopeChanges(
        *joinedFederationName_,
        *joinedFederateId_,
        *objectInstanceHandle,
        pairValues.values);
    if (result.status != umbra::detail::ObjectInstanceRegionAssociationStatus::applied) {
      throwObjectInstanceRegionAssociationFailureForFederationManagement(result.status);
    }
    federationName = *joinedFederationName_;
    discoveries = std::move(result.discoveries);
    changes = std::move(result.recipients);
    attributeRelevanceAdvisories = std::move(result.attributeRelevanceAdvisories);
  }
  // §9.7 has the same supplied shape and successful-void return. Keep the
  // interaction route outside native locks; validation failures stop above.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"UnassociateRegionsForUpdates",
      umbra::detail::MomServiceType::data_distribution_management,
      {{umbra::detail::MomArgumentType::object_instance_handle,
        L"Object instance designator",
        umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
       {umbra::detail::MomArgumentType::attribute_set_region_set_pair_list,
        L"Collection of attribute designator set and region designator set pairs",
        umbra::detail::formatMomAttributeSetRegionSetPairList(attributesAndRegions)}},
      true);
  queueAmbassadorObjectInstanceDiscoveries(std::move(discoveries), federationName);
  queueAmbassadorObjectInstanceScopeChanges(std::move(changes), federationName);
  queueAmbassadorAttributeRelevanceAdvisories(
      std::move(attributeRelevanceAdvisories), federationName);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Unassociate Regions For Updates", exception);
    appendFailedServiceReportToFileIfSelected(
        L"UnassociateRegionsForUpdates",
        umbra::detail::MomServiceType::data_distribution_management,
        {{umbra::detail::MomArgumentType::object_instance_handle,
          L"Object instance designator",
          umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
         {umbra::detail::MomArgumentType::attribute_set_region_set_pair_list,
          L"Collection of attribute designator set and region designator set pairs",
          umbra::detail::formatMomAttributeSetRegionSetPairList(attributesAndRegions)}},
        describeAmbassadorException(exception));
    throw;
  }
}

}  // namespace rti1516_2025::umbra_binding_detail
