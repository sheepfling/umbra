#include "internal/runtime/umbra_rti_ambassador.hpp"

#include "internal/federation/process_federation_client.hpp"
#include "internal/handles/interaction_class_handle.hpp"
#include "internal/handles/region_handle.hpp"
#include "internal/observability/mom_service_report_encoding.hpp"
#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/ambassador_string_helpers.hpp"
#include "internal/runtime/umbra_rti_ambassador_service_failure_translation.hpp"

#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <utility>

namespace rti1516_2025::umbra_binding_detail {

using namespace service_failure_translation;

void UmbraRtiAmbassador::subscribeInteractionClassWithRegions(
    InteractionClassHandle const& interactionClass,
    RegionHandleSet const& regions,
    bool active) {
  auto instrumentationScope = beginRtiCall("subscribeInteractionClassWithRegions");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const interactionClassValue = interactionClassHandleValue(interactionClass);
    if (!interactionClassValue) {
      throw InteractionClassNotDefined(
          L"Subscribe Interaction Class With Regions requires a defined InteractionClassHandle.");
    }
    std::set<std::uint64_t> regionValues;
    for (RegionHandle const& region : regions) {
      auto const value = regionHandleValue(region);
      if (!value) {
        throw InvalidRegion(
            L"Subscribe Interaction Class With Regions requires valid RegionHandle values.");
      }
      regionValues.insert(*value);
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
            L"Subscribe Interaction Class With Regions requires membership in a federation execution.");
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
      processClient->subscribeInteractionClassWithRegions(
          std::move(federationName),
          federateId,
          *interactionClassValue,
          std::move(regionValues),
          active);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif
  std::vector<umbra::detail::DeclarationAdvisory> declarationAdvisories;
  {
  std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
  requireConnectedForFederationManagement(lifecycle_);
  requireFederationServiceOperationAvailable(L"Subscribe Interaction Class With Regions");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Subscribe Interaction Class With Regions requires membership in a federation execution.");
  }

  auto& registry = embeddedFederationRegistry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  auto const interactionClassValue = interactionClassHandleValue(interactionClass);
  if (!interactionClassValue) {
    throw InteractionClassNotDefined(
        L"Subscribe Interaction Class With Regions requires a defined InteractionClassHandle.");
  }
  std::set<std::uint64_t> regionValues;
  for (RegionHandle const& region : regions) {
    auto const value = regionHandleValue(region);
    if (!value) {
      throw InvalidRegion(
          L"Subscribe Interaction Class With Regions requires valid RegionHandle values.");
    }
    regionValues.insert(*value);
  }

  auto const result = registry.setInteractionClassRegionalSubscription(
      *joinedFederationName_,
      *joinedFederateId_,
      *interactionClassValue,
      regionValues,
      active);
  if (result != umbra::detail::RegionalInteractionClassDeclarationStatus::applied) {
    throwRegionalInteractionClassDeclarationFailure(result);
  }
  declarationAdvisories = registry.planDeclarationAdvisories(*joinedFederationName_);
  }
  // §9.10 is a successful-void DDM service.  Emit after releasing the native
  // transaction locks because HLA_IMMEDIATE may re-enter the observer.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"SubscribeInteractionClassWithRegions",
      umbra::detail::MomServiceType::data_distribution_management,
      {{umbra::detail::MomArgumentType::interaction_class_handle,
        L"Interaction class designator",
        umbra::detail::formatMomInteractionClassHandle(interactionClass)},
       {umbra::detail::MomArgumentType::region_handle_set,
        L"Set of region designators",
        umbra::detail::formatMomRegionHandleSet(regions)},
       {umbra::detail::MomArgumentType::boolean,
        L"Optional passive subscription indicator",
        umbra::detail::formatMomBoolean(!active)}},
      true);
  queueAmbassadorDeclarationAdvisories(std::move(declarationAdvisories));
  } catch (Exception const& exception) {
    emitExceptionReport(L"Subscribe Interaction Class With Regions", exception);
    appendFailedServiceReportToFileIfSelected(
        L"SubscribeInteractionClassWithRegions",
        umbra::detail::MomServiceType::data_distribution_management,
        {{umbra::detail::MomArgumentType::interaction_class_handle,
          L"Interaction class designator",
          umbra::detail::formatMomInteractionClassHandle(interactionClass)},
         {umbra::detail::MomArgumentType::region_handle_set,
          L"Set of region designators",
          umbra::detail::formatMomRegionHandleSet(regions)},
         {umbra::detail::MomArgumentType::boolean,
          L"Optional passive subscription indicator",
          umbra::detail::formatMomBoolean(!active)}},
        describeAmbassadorException(exception));
    throw;
  }
}

void UmbraRtiAmbassador::unsubscribeInteractionClassWithRegions(
    InteractionClassHandle const& interactionClass,
    RegionHandleSet const& regions) {
  auto instrumentationScope = beginRtiCall("unsubscribeInteractionClassWithRegions");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const interactionClassValue = interactionClassHandleValue(interactionClass);
    if (!interactionClassValue) {
      throw InteractionClassNotDefined(
          L"Unsubscribe Interaction Class With Regions requires a defined InteractionClassHandle.");
    }
    std::set<std::uint64_t> regionValues;
    for (RegionHandle const& region : regions) {
      auto const value = regionHandleValue(region);
      if (!value) {
        throw InvalidRegion(
            L"Unsubscribe Interaction Class With Regions requires valid RegionHandle values.");
      }
      regionValues.insert(*value);
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
            L"Unsubscribe Interaction Class With Regions requires membership in a federation execution.");
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
      processClient->unsubscribeInteractionClassWithRegions(
          std::move(federationName),
          federateId,
          *interactionClassValue,
          std::move(regionValues));
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif
  std::vector<umbra::detail::DeclarationAdvisory> declarationAdvisories;
  {
  std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
  requireConnectedForFederationManagement(lifecycle_);
  requireFederationServiceOperationAvailable(L"Unsubscribe Interaction Class With Regions");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Unsubscribe Interaction Class With Regions requires membership in a federation execution.");
  }

  auto& registry = embeddedFederationRegistry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  auto const interactionClassValue = interactionClassHandleValue(interactionClass);
  if (!interactionClassValue) {
    throw InteractionClassNotDefined(
        L"Unsubscribe Interaction Class With Regions requires a defined InteractionClassHandle.");
  }
  std::set<std::uint64_t> regionValues;
  for (RegionHandle const& region : regions) {
    auto const value = regionHandleValue(region);
    if (!value) {
      throw InvalidRegion(
          L"Unsubscribe Interaction Class With Regions requires valid RegionHandle values.");
    }
    regionValues.insert(*value);
  }

  auto const result = registry.removeInteractionClassRegionalSubscription(
      *joinedFederationName_,
      *joinedFederateId_,
      *interactionClassValue,
      regionValues);
  if (result != umbra::detail::RegionalInteractionClassDeclarationStatus::applied) {
    throwRegionalInteractionClassDeclarationFailure(result);
  }
  declarationAdvisories = registry.planDeclarationAdvisories(*joinedFederationName_);
  }
  // §9.11 is the paired successful-void DDM removal.  Keep public interaction
  // delivery outside the native transaction locks.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"UnsubscribeInteractionClassWithRegions",
      umbra::detail::MomServiceType::data_distribution_management,
      {{umbra::detail::MomArgumentType::interaction_class_handle,
        L"Interaction class designator",
        umbra::detail::formatMomInteractionClassHandle(interactionClass)},
       {umbra::detail::MomArgumentType::region_handle_set,
        L"Set of region designators",
        umbra::detail::formatMomRegionHandleSet(regions)}},
      true);
  queueAmbassadorDeclarationAdvisories(std::move(declarationAdvisories));
  } catch (Exception const& exception) {
    emitExceptionReport(L"Unsubscribe Interaction Class With Regions", exception);
    appendFailedServiceReportToFileIfSelected(
        L"UnsubscribeInteractionClassWithRegions",
        umbra::detail::MomServiceType::data_distribution_management,
        {{umbra::detail::MomArgumentType::interaction_class_handle,
          L"Interaction class designator",
          umbra::detail::formatMomInteractionClassHandle(interactionClass)},
         {umbra::detail::MomArgumentType::region_handle_set,
          L"Set of region designators",
          umbra::detail::formatMomRegionHandleSet(regions)}},
        describeAmbassadorException(exception));
    throw;
  }
}

}  // namespace rti1516_2025::umbra_binding_detail
