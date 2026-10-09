#include "internal/runtime/umbra_rti_ambassador.hpp"

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_client.hpp"
#include "internal/handles/dimension_handle.hpp"
#include "internal/handles/interaction_class_handle.hpp"
#include "internal/handles/object_class_handle.hpp"
#include "internal/handles/region_handle.hpp"
#include "internal/observability/mom_service_report_encoding.hpp"
#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/ambassador_string_helpers.hpp"
#include "internal/runtime/umbra_rti_ambassador_service_failure_translation.hpp"
#include "internal/runtime/utf8_string.hpp"

#include <cstdint>
#include <mutex>
#include <optional>
#include <set>
#include <utility>
#include <vector>

namespace rti1516_2025::umbra_binding_detail {
using namespace service_failure_translation;

namespace {

DimensionHandleSet makeDimensionHandleSet(std::set<std::uint64_t> const &handles) {
  DimensionHandleSet result;
  for (std::uint64_t const handle : handles) {
    result.insert(makeDimensionHandle(handle));
  }
  return result;
}

}  // namespace

DimensionHandleSet UmbraRtiAmbassador::getAvailableDimensionsForObjectClass(
    ObjectClassHandle const &objectClass) {
  auto instrumentationScope = beginRtiCall("getAvailableDimensionsForObjectClass");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const objectClassValue = objectClassHandleValue(objectClass);
    if (!objectClassValue) {
      throw InvalidObjectClassHandle(
          L"Get Available Dimensions for Object Class requires a valid ObjectClassHandle.");
    }
    std::wstring federationName;
    std::uint64_t federateId = 0U;
    umbra::detail::ProcessFederationClient *processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Get Available Dimensions for Object Class requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    umbra::detail::ProcessFederationAvailableDimensionsResult result;
    try {
      result = processClient->availableDimensionsForObjectClass(
          std::move(federationName), federateId, *objectClassValue);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const &error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const &error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    if (!result.found) {
      throw InvalidObjectClassHandle(
          L"The supplied ObjectClassHandle is not known in this federation execution.");
    }
    return makeDimensionHandleSet(
        std::set<std::uint64_t>(
            result.dimensionHandles.begin(), result.dimensionHandles.end()));
  }
#endif
  DimensionHandleSet availableDimensions;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Get Available Dimensions for Object Class requires membership in a federation execution.");
    }

    auto &registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const objectClassHandle = objectClassHandleValue(objectClass);
    if (!objectClassHandle) {
      throw InvalidObjectClassHandle(
          L"Get Available Dimensions for Object Class requires a valid ObjectClassHandle.");
    }
    auto const handles = registry.availableDimensionsForObjectClass(
        *joinedFederationName_, *objectClassHandle);
    if (!handles) {
      throw InvalidObjectClassHandle(
          L"The supplied ObjectClassHandle is not known in this federation execution.");
    }
    availableDimensions = makeDimensionHandleSet(*handles);
  }

  appendSuccessfulServiceReportToFileIfSelected(
      L"GetAvailableDimensionsForObjectClass",
      umbra::detail::MomServiceType::support_services,
      {{umbra::detail::MomArgumentType::object_class_handle,
        L"Object class handle",
        umbra::detail::formatMomObjectClassHandle(objectClass)}},
      {umbra::detail::MomArgumentType::dimension_handle_set,
       L"A set of dimension handles",
       umbra::detail::formatMomDimensionHandleSet(availableDimensions)});
  return availableDimensions;
  } catch (Exception const &exception) {
    emitExceptionReport(L"Get Available Dimensions For Object Class", exception);
    appendFailedServiceReportToFileIfSelected(
        L"GetAvailableDimensionsForObjectClass",
        umbra::detail::MomServiceType::support_services,
        {{umbra::detail::MomArgumentType::object_class_handle,
          L"Object class handle",
          umbra::detail::formatMomObjectClassHandle(objectClass)}},
        describeAmbassadorException(exception));
    throw;
  }
}

DimensionHandleSet UmbraRtiAmbassador::getAvailableDimensionsForInteractionClass(
    InteractionClassHandle const &interactionClass) {
  auto instrumentationScope = beginRtiCall("getAvailableDimensionsForInteractionClass");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const interactionClassValue = interactionClassHandleValue(interactionClass);
    if (!interactionClassValue) {
      throw InvalidInteractionClassHandle(
          L"Get Available Dimensions for Interaction Class requires a valid InteractionClassHandle.");
    }
    std::wstring federationName;
    std::uint64_t federateId = 0U;
    umbra::detail::ProcessFederationClient *processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Get Available Dimensions for Interaction Class requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    umbra::detail::ProcessFederationAvailableDimensionsResult result;
    try {
      result = processClient->availableDimensionsForInteractionClass(
          std::move(federationName), federateId, *interactionClassValue);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const &error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const &error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    if (!result.found) {
      throw InvalidInteractionClassHandle(
          L"The supplied InteractionClassHandle is not known in this federation execution.");
    }
    return makeDimensionHandleSet(
        std::set<std::uint64_t>(
            result.dimensionHandles.begin(), result.dimensionHandles.end()));
  }
#endif
  DimensionHandleSet availableDimensions;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Get Available Dimensions for Interaction Class requires membership in a federation execution.");
    }

    auto &registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const interactionClassHandle = interactionClassHandleValue(interactionClass);
    if (!interactionClassHandle) {
      throw InvalidInteractionClassHandle(
          L"Get Available Dimensions for Interaction Class requires a valid InteractionClassHandle.");
    }
    auto const handles = registry.availableDimensionsForInteractionClass(
        *joinedFederationName_, *interactionClassHandle);
    if (!handles) {
      throw InvalidInteractionClassHandle(
          L"The supplied InteractionClassHandle is not known in this federation execution.");
    }
    availableDimensions = makeDimensionHandleSet(*handles);
  }

  appendSuccessfulServiceReportToFileIfSelected(
      L"GetAvailableDimensionsForInteractionClass",
      umbra::detail::MomServiceType::support_services,
      {{umbra::detail::MomArgumentType::interaction_class_handle,
        L"Interaction class handle",
        umbra::detail::formatMomInteractionClassHandle(interactionClass)}},
      {umbra::detail::MomArgumentType::dimension_handle_set,
       L"A set of dimension handles",
       umbra::detail::formatMomDimensionHandleSet(availableDimensions)});
  return availableDimensions;
  } catch (Exception const &exception) {
    emitExceptionReport(L"Get Available Dimensions For Interaction Class", exception);
    appendFailedServiceReportToFileIfSelected(
        L"GetAvailableDimensionsForInteractionClass",
        umbra::detail::MomServiceType::support_services,
        {{umbra::detail::MomArgumentType::interaction_class_handle,
          L"Interaction class handle",
          umbra::detail::formatMomInteractionClassHandle(interactionClass)}},
        describeAmbassadorException(exception));
    throw;
  }
}

DimensionHandle UmbraRtiAmbassador::getDimensionHandle(std::wstring const &dimensionName) {
  auto instrumentationScope = beginRtiCall("getDimensionHandle");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    std::wstring federationName;
    std::uint64_t federateId = 0U;
    umbra::detail::ProcessFederationClient *processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Get Dimension Handle requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    auto const encodedName = umbra::detail::utf8FromWide(dimensionName);
    if (!encodedName) {
      throw NameNotFound(L"The supplied dimension name is not valid UTF-8 text.");
    }
    std::optional<std::uint64_t> handle;
    try {
      handle = processClient->lookupDimensionHandle(
          std::move(federationName), federateId, *encodedName);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const &error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const &error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    if (!handle) {
      throw NameNotFound(
          L"The supplied dimension name is not defined in this federation execution.");
    }
    return makeDimensionHandle(*handle);
  }
#endif
  DimensionHandle dimensionHandle;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Get Dimension Handle requires membership in a federation execution.");
    }

    auto &registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const encodedName = umbra::detail::utf8FromWide(dimensionName);
    if (!encodedName) {
      throw NameNotFound(L"The supplied dimension name is not valid UTF-8 text.");
    }
    auto const handle = registry.dimensionHandleFor(*joinedFederationName_, *encodedName);
    if (!handle) {
      throw NameNotFound(
          L"The supplied dimension name is not defined in this federation execution.");
    }
    dimensionHandle = makeDimensionHandle(*handle);
  }

  appendSuccessfulServiceReportToFileIfSelected(
      L"GetDimensionHandle",
      umbra::detail::MomServiceType::support_services,
      {{umbra::detail::MomArgumentType::string,
        L"Dimension name",
        umbra::detail::formatMomString(dimensionName)}},
      {umbra::detail::MomArgumentType::dimension_handle,
       L"Dimension handle",
       umbra::detail::formatMomDimensionHandle(dimensionHandle)});
  return dimensionHandle;
  } catch (Exception const &exception) {
    emitExceptionReport(L"Get Dimension Handle", exception);
    appendFailedServiceReportToFileIfSelected(
        L"GetDimensionHandle",
        umbra::detail::MomServiceType::support_services,
        {{umbra::detail::MomArgumentType::string,
          L"Dimension name",
          umbra::detail::formatMomString(dimensionName)}},
        describeAmbassadorException(exception));
    throw;
  }
}

std::wstring UmbraRtiAmbassador::getDimensionName(DimensionHandle const &dimension) {
  auto instrumentationScope = beginRtiCall("getDimensionName");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const dimensionValue = dimensionHandleValue(dimension);
    if (!dimensionValue) {
      throw InvalidDimensionHandle(
          L"Get Dimension Name requires a valid DimensionHandle.");
    }
    std::wstring federationName;
    std::uint64_t federateId = 0U;
    umbra::detail::ProcessFederationClient *processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Get Dimension Name requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    std::optional<std::wstring> dimensionName;
    try {
      dimensionName = processClient->lookupDimensionName(
          federationName, federateId, *dimensionValue);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const &error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const &error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    if (!dimensionName) {
      throw InvalidDimensionHandle(
          L"The supplied DimensionHandle is not known in this federation execution.");
    }
    return *dimensionName;
  }
#endif
  std::wstring dimensionName;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Get Dimension Name requires membership in a federation execution.");
    }

    auto &registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const handle = dimensionHandleValue(dimension);
    if (!handle) {
      throw InvalidDimensionHandle(L"Get Dimension Name requires a valid DimensionHandle.");
    }
    auto const encodedName = registry.dimensionNameFor(*joinedFederationName_, *handle);
    if (!encodedName) {
      throw InvalidDimensionHandle(
          L"The supplied DimensionHandle is not known in this federation execution.");
    }
    auto const decodedName = umbra::detail::wideFromUtf8(*encodedName);
    if (!decodedName) {
      throw RTIinternalError(
          L"The embedded federation stored a dimension name that is not valid UTF-8 text.");
    }
    dimensionName = *decodedName;
  }

  appendSuccessfulServiceReportToFileIfSelected(
      L"GetDimensionName",
      umbra::detail::MomServiceType::support_services,
      {{umbra::detail::MomArgumentType::dimension_handle,
        L"Dimension handle",
        umbra::detail::formatMomDimensionHandle(dimension)}},
      {umbra::detail::MomArgumentType::string,
       L"Dimension name",
       umbra::detail::formatMomString(dimensionName)});
  return dimensionName;
  } catch (Exception const &exception) {
    emitExceptionReport(L"Get Dimension Name", exception);
    appendFailedServiceReportToFileIfSelected(
        L"GetDimensionName",
        umbra::detail::MomServiceType::support_services,
        {{umbra::detail::MomArgumentType::dimension_handle,
          L"Dimension handle",
          umbra::detail::formatMomDimensionHandle(dimension)}},
        describeAmbassadorException(exception));
    throw;
  }
}

unsigned long UmbraRtiAmbassador::getDimensionUpperBound(DimensionHandle const &dimension) {
  auto instrumentationScope = beginRtiCall("getDimensionUpperBound");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const dimensionValue = dimensionHandleValue(dimension);
    if (!dimensionValue) {
      throw InvalidDimensionHandle(
          L"Get Dimension Upper Bound requires a valid DimensionHandle.");
    }
    std::wstring federationName;
    std::uint64_t federateId = 0U;
    umbra::detail::ProcessFederationClient *processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Get Dimension Upper Bound requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    umbra::detail::ProcessFederationDimensionUpperBoundResult result;
    try {
      result = processClient->lookupDimensionUpperBound(
          std::move(federationName), federateId, *dimensionValue);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const &error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const &error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    if (!result.found) {
      throw InvalidDimensionHandle(
          L"The supplied DimensionHandle is not known in this federation execution.");
    }
    return result.upperBound;
  }
#endif
  unsigned long upperBound = 0UL;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Get Dimension Upper Bound requires membership in a federation execution.");
    }

    auto &registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const handle = dimensionHandleValue(dimension);
    if (!handle) {
      throw InvalidDimensionHandle(
          L"Get Dimension Upper Bound requires a valid DimensionHandle.");
    }
    auto const result = registry.dimensionUpperBoundFor(*joinedFederationName_, *handle);
    if (!result) {
      throw InvalidDimensionHandle(
          L"The supplied DimensionHandle is not known in this federation execution.");
    }
    upperBound = *result;
  }

  appendSuccessfulServiceReportToFileIfSelected(
      L"GetDimensionUpperBound",
      umbra::detail::MomServiceType::support_services,
      {{umbra::detail::MomArgumentType::dimension_handle,
        L"Dimension handle",
        umbra::detail::formatMomDimensionHandle(dimension)}},
      {umbra::detail::MomArgumentType::number,
       L"Dimension upper bound",
       umbra::detail::formatMomNumber(std::to_wstring(upperBound))});
  return upperBound;
  } catch (Exception const &exception) {
    emitExceptionReport(L"Get Dimension Upper Bound", exception);
    appendFailedServiceReportToFileIfSelected(
        L"GetDimensionUpperBound",
        umbra::detail::MomServiceType::support_services,
        {{umbra::detail::MomArgumentType::dimension_handle,
          L"Dimension handle",
          umbra::detail::formatMomDimensionHandle(dimension)}},
        describeAmbassadorException(exception));
    throw;
  }
}

RegionHandle UmbraRtiAmbassador::createRegion(DimensionHandleSet const &dimensions) {
  auto instrumentationScope = beginRtiCall("createRegion");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    std::vector<std::uint64_t> dimensionValues;
    dimensionValues.reserve(dimensions.size());
    for (DimensionHandle const &dimension : dimensions) {
      auto const value = dimensionHandleValue(dimension);
      if (!value) {
        throw InvalidDimensionHandle(
            L"Create Region requires valid DimensionHandle values.");
      }
      dimensionValues.push_back(*value);
    }
    std::wstring federationName;
    std::uint64_t federateId = 0U;
    umbra::detail::ProcessFederationClient *processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Create Region requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    umbra::detail::ProcessFederationCreateRegionResult result;
    try {
      result = processClient->createRegion(
          std::move(federationName), federateId, std::move(dimensionValues));
    } catch (umbra::detail::ProcessFederationServiceProtocolError const &error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const &error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    if (result.status != umbra::detail::RegionServiceStatus::applied) {
      throwRegionServiceFailure(result.status, L"Create Region");
    }
    return makeRegionHandle(result.regionHandle);
  }
#endif
  RegionHandle createdRegion;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Create Region");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Create Region requires membership in a federation execution.");
    }

    std::set<std::uint64_t> dimensionValues;
    for (DimensionHandle const &dimension : dimensions) {
      auto const value = dimensionHandleValue(dimension);
      if (!value) {
        throw InvalidDimensionHandle(
            L"Create Region requires valid DimensionHandle values.");
      }
      dimensionValues.insert(*value);
    }

    auto &registry = embeddedFederationRegistry();
    auto const result = registry.createRegion(
        *joinedFederationName_, *joinedFederateId_, dimensionValues);
    if (result.status != umbra::detail::RegionServiceStatus::applied) {
      throwRegionServiceFailure(result.status, L"Create Region");
    }
    createdRegion = makeRegionHandle(result.regionHandle);
  }

  appendSuccessfulServiceReportToFileIfSelected(
      L"CreateRegion",
      umbra::detail::MomServiceType::data_distribution_management,
      {{umbra::detail::MomArgumentType::dimension_handle_set,
        L"Set of dimension designators",
        umbra::detail::formatMomDimensionHandleSet(dimensions)}},
      {umbra::detail::MomArgumentType::region_handle,
       L"Region designator",
       umbra::detail::formatMomRegionHandle(createdRegion)});
  return createdRegion;
  } catch (Exception const &exception) {
    emitExceptionReport(L"Create Region", exception);
    appendFailedServiceReportToFileIfSelected(
        L"CreateRegion",
        umbra::detail::MomServiceType::data_distribution_management,
        {{umbra::detail::MomArgumentType::dimension_handle_set,
          L"Set of dimension designators",
          umbra::detail::formatMomDimensionHandleSet(dimensions)}},
        describeAmbassadorException(exception));
    throw;
  }
}

void UmbraRtiAmbassador::commitRegionModifications(RegionHandleSet const &regions) {
  auto instrumentationScope = beginRtiCall("commitRegionModifications");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    std::vector<std::uint64_t> regionValues;
    regionValues.reserve(regions.size());
    for (RegionHandle const &region : regions) {
      auto const value = regionHandleValue(region);
      if (!value) {
        throw InvalidRegion(
            L"Commit Region Modifications requires valid RegionHandle values.");
      }
      regionValues.push_back(*value);
    }
    std::wstring federationName;
    std::uint64_t federateId = 0U;
    umbra::detail::ProcessFederationClient *processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Commit Region Modifications requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    umbra::detail::ProcessFederationRegionStatusResult result;
    try {
      result = processClient->commitRegionModifications(
          std::move(federationName), federateId, std::move(regionValues));
    } catch (umbra::detail::ProcessFederationServiceProtocolError const &error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const &error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    if (result.status != umbra::detail::RegionServiceStatus::applied) {
      throwRegionServiceFailure(result.status, L"Commit Region Modifications");
    }
    return;
  }
#endif
  std::wstring federationName;
  std::vector<umbra::detail::ObjectInstanceDiscoveryRecipient> discoveries;
  std::vector<umbra::detail::ObjectInstanceScopeChangeRecipient> changes;
  std::vector<umbra::detail::AttributeRelevanceAdvisoryRecipient>
      attributeRelevanceAdvisories;
  std::vector<umbra::detail::ObjectInstanceDiscoveryRecipient> momDiscoveries;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Commit Region Modifications");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Commit Region Modifications requires membership in a federation execution.");
    }

    std::set<std::uint64_t> regionValues;
    for (RegionHandle const &region : regions) {
      auto const value = regionHandleValue(region);
      if (!value) {
        throw InvalidRegion(L"Commit Region Modifications requires valid RegionHandle values.");
      }
      regionValues.insert(*value);
    }

    auto &registry = embeddedFederationRegistry();
    auto scopePlan = registry.commitRegionModificationsWithScopeChanges(
        *joinedFederationName_, *joinedFederateId_, regionValues);
    if (scopePlan.status != umbra::detail::RegionServiceStatus::applied) {
      throwRegionServiceFailure(scopePlan.status, L"Commit Region Modifications");
    }
    federationName = *joinedFederationName_;
    discoveries = std::move(scopePlan.discoveries);
    changes = std::move(scopePlan.recipients);
    attributeRelevanceAdvisories = std::move(scopePlan.attributeRelevanceAdvisories);
    // A committed region mutation can make an existing RTI-owned MOM point
    // newly eligible without changing the subscription declaration. Reuse
    // the normal MOM planner so discovery is reserved exactly once and is
    // rechecked again at callback entry.
    momDiscoveries = registry.planJoinedFederateMomObjectDiscoveriesForFederate(
        federationName,
        *joinedFederateId_);
  }
  // The public MOM interaction may synchronously enter an observer callback.
  // Emit only after the service transaction has released both native locks.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"CommitRegionModifications",
      umbra::detail::MomServiceType::data_distribution_management,
      {{umbra::detail::MomArgumentType::region_handle_set,
        L"Set of region designators",
        umbra::detail::formatMomRegionHandleSet(regions)}},
      true);
  // A region mutation can make an already-registered object discoverable.
  // Commit the discovery callback before any scope-transition callbacks so the
  // recipient's known-instance state is established at the public boundary.
  queueAmbassadorObjectInstanceDiscoveries(std::move(discoveries), federationName);
  queueAmbassadorObjectInstanceScopeChanges(std::move(changes), federationName);
  queueAmbassadorAttributeRelevanceAdvisories(std::move(attributeRelevanceAdvisories), federationName);
  queueAmbassadorObjectInstanceDiscoveries(std::move(momDiscoveries), federationName);
  } catch (Exception const &exception) {
    emitExceptionReport(L"Commit Region Modifications", exception);
    appendFailedServiceReportToFileIfSelected(
        L"CommitRegionModifications",
        umbra::detail::MomServiceType::data_distribution_management,
        {{umbra::detail::MomArgumentType::region_handle_set,
          L"Set of region designators",
          umbra::detail::formatMomRegionHandleSet(regions)}},
        describeAmbassadorException(exception));
    throw;
  }
}

void UmbraRtiAmbassador::deleteRegion(RegionHandle const &region) {
  auto instrumentationScope = beginRtiCall("deleteRegion");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const regionValue = regionHandleValue(region);
    if (!regionValue) {
      throw InvalidRegion(L"Delete Region requires a valid RegionHandle.");
    }
    std::wstring federationName;
    std::uint64_t federateId = 0U;
    umbra::detail::ProcessFederationClient *processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Delete Region requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    umbra::detail::ProcessFederationRegionStatusResult result;
    try {
      result = processClient->deleteRegion(
          std::move(federationName), federateId, *regionValue);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const &error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const &error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    if (result.status != umbra::detail::RegionServiceStatus::applied) {
      throwRegionServiceFailure(result.status, L"Delete Region");
    }
    return;
  }
#endif
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Delete Region");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Delete Region requires membership in a federation execution.");
    }
    auto const value = regionHandleValue(region);
    if (!value) {
      throw InvalidRegion(L"Delete Region requires a valid RegionHandle.");
    }

    auto &registry = embeddedFederationRegistry();
    auto const status = registry.deleteRegion(
        *joinedFederationName_, *joinedFederateId_, *value);
    if (status != umbra::detail::RegionServiceStatus::applied) {
      throwRegionServiceFailure(status, L"Delete Region");
    }
  }
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"DeleteRegion",
      umbra::detail::MomServiceType::data_distribution_management,
      {{umbra::detail::MomArgumentType::region_handle,
        L"Region designator",
        umbra::detail::formatMomRegionHandle(region)}},
      true);
  } catch (Exception const &exception) {
    emitExceptionReport(L"Delete Region", exception);
    appendFailedServiceReportToFileIfSelected(
        L"DeleteRegion",
        umbra::detail::MomServiceType::data_distribution_management,
        {{umbra::detail::MomArgumentType::region_handle,
          L"Region designator",
          umbra::detail::formatMomRegionHandle(region)}},
        describeAmbassadorException(exception));
    throw;
  }
}

DimensionHandleSet UmbraRtiAmbassador::getDimensionHandleSet(RegionHandle const &region) {
  auto instrumentationScope = beginRtiCall("getDimensionHandleSet");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const regionValue = regionHandleValue(region);
    if (!regionValue) {
      throw InvalidRegion(L"Get Dimension Handle Set requires a valid RegionHandle.");
    }
    std::wstring federationName;
    std::uint64_t federateId = 0U;
    umbra::detail::ProcessFederationClient *processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Get Dimension Handle Set requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    umbra::detail::ProcessFederationDimensionSetResult result;
    try {
      result = processClient->dimensionHandleSetForRegion(
          std::move(federationName), federateId, *regionValue);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const &error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const &error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    if (result.status != umbra::detail::RegionServiceStatus::applied) {
      throwRegionServiceFailure(result.status, L"Get Dimension Handle Set");
    }
    return makeDimensionHandleSet(
        std::set<std::uint64_t>(
            result.dimensionHandles.begin(), result.dimensionHandles.end()));
  }
#endif
  DimensionHandleSet dimensionHandles;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Get Dimension Handle Set");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Get Dimension Handle Set requires membership in a federation execution.");
    }
    auto const value = regionHandleValue(region);
    if (!value) {
      throw InvalidRegion(L"Get Dimension Handle Set requires a valid RegionHandle.");
    }

    auto &registry = embeddedFederationRegistry();
    auto const result = registry.dimensionHandleSetForRegion(
        *joinedFederationName_, *joinedFederateId_, *value);
    if (result.status != umbra::detail::RegionServiceStatus::applied) {
      throwRegionServiceFailure(result.status, L"Get Dimension Handle Set");
    }
    dimensionHandles = makeDimensionHandleSet(result.dimensionHandles);
  }

  appendSuccessfulServiceReportToFileIfSelected(
      L"GetDimensionHandleSet",
      umbra::detail::MomServiceType::support_services,
      {{umbra::detail::MomArgumentType::region_handle,
        L"Region handle",
        umbra::detail::formatMomRegionHandle(region)}},
      {umbra::detail::MomArgumentType::dimension_handle_set,
       L"A set of dimensions",
       umbra::detail::formatMomDimensionHandleSet(dimensionHandles)});
  return dimensionHandles;
  } catch (Exception const &exception) {
    emitExceptionReport(L"Get Dimension Handle Set", exception);
    appendFailedServiceReportToFileIfSelected(
        L"GetDimensionHandleSet",
        umbra::detail::MomServiceType::support_services,
        {{umbra::detail::MomArgumentType::region_handle,
          L"Region handle",
          umbra::detail::formatMomRegionHandle(region)}},
        describeAmbassadorException(exception));
    throw;
  }
}

RangeBounds UmbraRtiAmbassador::getRangeBounds(
    RegionHandle const &region,
    DimensionHandle const &dimension) {
  auto instrumentationScope = beginRtiCall("getRangeBounds");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const regionValue = regionHandleValue(region);
    if (!regionValue) {
      throw InvalidRegion(L"Get Range Bounds requires a valid RegionHandle.");
    }
    auto const dimensionValue = dimensionHandleValue(dimension);
    if (!dimensionValue) {
      throw RegionDoesNotContainSpecifiedDimension(
          L"Get Range Bounds requires a dimension contained by the region.");
    }
    std::wstring federationName;
    std::uint64_t federateId = 0U;
    umbra::detail::ProcessFederationClient *processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Get Range Bounds requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    umbra::detail::ProcessFederationRangeBoundsResult result;
    try {
      result = processClient->rangeBoundsForRegion(
          std::move(federationName), federateId, *regionValue, *dimensionValue);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const &error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const &error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    if (result.status != umbra::detail::RegionServiceStatus::applied) {
      throwRegionServiceFailure(result.status, L"Get Range Bounds");
    }
    return RangeBounds(result.lowerBound, result.upperBound);
  }
#endif
  RangeBounds rangeBounds;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Get Range Bounds");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Get Range Bounds requires membership in a federation execution.");
    }
    auto const regionValue = regionHandleValue(region);
    if (!regionValue) {
      throw InvalidRegion(L"Get Range Bounds requires a valid RegionHandle.");
    }
    auto const dimensionValue = dimensionHandleValue(dimension);
    if (!dimensionValue) {
      throw RegionDoesNotContainSpecifiedDimension(
          L"Get Range Bounds requires a dimension contained by the region.");
    }

    auto &registry = embeddedFederationRegistry();
    auto const result = registry.rangeBoundsForRegion(
        *joinedFederationName_, *joinedFederateId_, *regionValue, *dimensionValue);
    if (result.status != umbra::detail::RegionServiceStatus::applied) {
      throwRegionServiceFailure(result.status, L"Get Range Bounds");
    }
    rangeBounds = RangeBounds(result.range.lowerBound, result.range.upperBound);
  }

  appendSuccessfulServiceReportToFileIfSelected(
      L"GetRangeBounds",
      umbra::detail::MomServiceType::data_distribution_management,
      {{umbra::detail::MomArgumentType::region_handle,
        L"Region handle",
        umbra::detail::formatMomRegionHandle(region)},
       {umbra::detail::MomArgumentType::dimension_handle,
        L"Dimension handle",
        umbra::detail::formatMomDimensionHandle(dimension)}},
      {umbra::detail::MomArgumentType::range_bounds,
       L"Range bounds",
       umbra::detail::formatMomRangeBounds(
           rangeBounds.getLowerBound(), rangeBounds.getUpperBound())});
  return rangeBounds;
  } catch (Exception const &exception) {
    emitExceptionReport(L"Get Range Bounds", exception);
    appendFailedServiceReportToFileIfSelected(
        L"GetRangeBounds",
        umbra::detail::MomServiceType::data_distribution_management,
        {{umbra::detail::MomArgumentType::region_handle,
          L"Region handle",
          umbra::detail::formatMomRegionHandle(region)},
         {umbra::detail::MomArgumentType::dimension_handle,
          L"Dimension handle",
          umbra::detail::formatMomDimensionHandle(dimension)}},
        describeAmbassadorException(exception));
    throw;
  }
}

void UmbraRtiAmbassador::setRangeBounds(
    RegionHandle const &region,
    DimensionHandle const &dimension,
    RangeBounds const &rangeBounds) {
  auto instrumentationScope = beginRtiCall("setRangeBounds");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const regionValue = regionHandleValue(region);
    if (!regionValue) {
      throw InvalidRegion(L"Set Range Bounds requires a valid RegionHandle.");
    }
    auto const dimensionValue = dimensionHandleValue(dimension);
    if (!dimensionValue) {
      throw RegionDoesNotContainSpecifiedDimension(
          L"Set Range Bounds requires a dimension contained by the region.");
    }
    std::wstring federationName;
    std::uint64_t federateId = 0U;
    umbra::detail::ProcessFederationClient *processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Set Range Bounds requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    umbra::detail::ProcessFederationRegionStatusResult result;
    try {
      result = processClient->setRangeBounds(
          std::move(federationName),
          federateId,
          *regionValue,
          *dimensionValue,
          rangeBounds.getLowerBound(),
          rangeBounds.getUpperBound());
    } catch (umbra::detail::ProcessFederationServiceProtocolError const &error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const &error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    if (result.status != umbra::detail::RegionServiceStatus::applied) {
      throwRegionServiceFailure(result.status, L"Set Range Bounds");
    }
    return;
  }
#endif
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Set Range Bounds");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Set Range Bounds requires membership in a federation execution.");
    }
    auto const regionValue = regionHandleValue(region);
    if (!regionValue) {
      throw InvalidRegion(L"Set Range Bounds requires a valid RegionHandle.");
    }
    auto const dimensionValue = dimensionHandleValue(dimension);
    if (!dimensionValue) {
      throw RegionDoesNotContainSpecifiedDimension(
          L"Set Range Bounds requires a dimension contained by the region.");
    }

    auto &registry = embeddedFederationRegistry();
    auto const status = registry.setRangeBounds(
        *joinedFederationName_,
        *joinedFederateId_,
        *regionValue,
        *dimensionValue,
        umbra::detail::RegionRangeBounds{
            rangeBounds.getLowerBound(),
            rangeBounds.getUpperBound(),
        });
    if (status != umbra::detail::RegionServiceStatus::applied) {
      throwRegionServiceFailure(status, L"Set Range Bounds");
    }
  }
  // Keep HLA_IMMEDIATE re-entry outside the DDM transaction locks.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"SetRangeBounds",
      umbra::detail::MomServiceType::data_distribution_management,
      {{umbra::detail::MomArgumentType::region_handle,
        L"Region handle",
        umbra::detail::formatMomRegionHandle(region)},
       {umbra::detail::MomArgumentType::dimension_handle,
        L"Dimension handle",
        umbra::detail::formatMomDimensionHandle(dimension)},
       {umbra::detail::MomArgumentType::number,
        L"Range lower bound",
        umbra::detail::formatMomNumber(std::to_wstring(rangeBounds.getLowerBound()))},
       {umbra::detail::MomArgumentType::number,
        L"Range upper bound",
        umbra::detail::formatMomNumber(std::to_wstring(rangeBounds.getUpperBound()))}},
      true);
  } catch (Exception const &exception) {
    emitExceptionReport(L"Set Range Bounds", exception);
    appendFailedServiceReportToFileIfSelected(
        L"SetRangeBounds",
        umbra::detail::MomServiceType::data_distribution_management,
        {{umbra::detail::MomArgumentType::region_handle,
          L"Region handle",
          umbra::detail::formatMomRegionHandle(region)},
         {umbra::detail::MomArgumentType::dimension_handle,
          L"Dimension handle",
          umbra::detail::formatMomDimensionHandle(dimension)},
         {umbra::detail::MomArgumentType::number,
          L"Range lower bound",
          umbra::detail::formatMomNumber(std::to_wstring(rangeBounds.getLowerBound()))},
         {umbra::detail::MomArgumentType::number,
          L"Range upper bound",
          umbra::detail::formatMomNumber(std::to_wstring(rangeBounds.getUpperBound()))}},
        describeAmbassadorException(exception));
    throw;
  }
}

}  // namespace rti1516_2025::umbra_binding_detail
#endif
