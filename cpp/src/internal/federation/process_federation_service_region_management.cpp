#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_federation_service_helpers.hpp"
#include "internal/handles/dimension_handle.hpp"
#include "internal/handles/region_handle.hpp"
#include "internal/observability/mom_service_report_encoding.hpp"

#include <set>
#include <utility>

namespace umbra::detail {

TransportServiceMessage ProcessFederationService::handleCreateRegion(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const createRequest =
      decodeProcessFederationCreateRegionRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != createRequest.federationName ||
        state->second.federateId != createRequest.federateId ||
        !registry_.memberById(
            createRequest.federationName, createRequest.federateId)) {
      return rejected(request);
    }
  }
  std::set<std::uint64_t> dimensions(
      createRequest.dimensionHandles.begin(), createRequest.dimensionHandles.end());
  auto const result = registry_.createRegion(
      createRequest.federationName, createRequest.federateId, dimensions);
  if (result.status == RegionServiceStatus::applied) {
    rti1516_2025::DimensionHandleSet reportDimensions;
    for (auto const handle : dimensions) {
      reportDimensions.insert(
          rti1516_2025::umbra_binding_detail::makeDimensionHandle(handle));
    }
    if (!appendSelectedServiceReportRecord(
            session,
            createRequest.federationName,
            createRequest.federateId,
            static_cast<std::uint16_t>(MomServiceType::data_distribution_management),
            [reportDimensions = std::move(reportDimensions),
             regionHandle = result.regionHandle](std::uint32_t serialNumber) {
              return formatMomSuccessfulServiceReportRecord(
                  serialNumber,
                  L"CreateRegion",
                  {{MomArgumentType::dimension_handle_set,
                    L"Set of dimension designators",
                    formatMomDimensionHandleSet(reportDimensions)}},
                  {MomArgumentType::region_handle,
                   L"Region designator",
                   formatMomRegionHandle(
                       rti1516_2025::umbra_binding_detail::makeRegionHandle(
                           regionHandle))});
            })) {
      return internalError(request);
    }
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationCreateRegionResult(
          ProcessFederationCreateRegionResult{result.status, result.regionHandle}));
}

TransportServiceMessage ProcessFederationService::handleCommitRegionModifications(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const commitRequest =
      decodeProcessFederationRegionSetRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != commitRequest.federationName ||
        state->second.federateId != commitRequest.federateId ||
        !registry_.memberById(
            commitRequest.federationName, commitRequest.federateId)) {
      return rejected(request);
    }
  }
  std::set<std::uint64_t> regions(
      commitRequest.regionHandles.begin(), commitRequest.regionHandles.end());
  auto plan = registry_.commitRegionModificationsWithScopeChanges(
      commitRequest.federationName, commitRequest.federateId, regions);
  if (plan.status == RegionServiceStatus::applied &&
      !enqueueObjectInstanceDiscoveries(
          commitRequest.federationName, std::move(plan.discoveries))) {
    return internalError(request);
  }
  if (plan.status == RegionServiceStatus::applied &&
      !enqueueObjectInstanceScopeChanges(
          commitRequest.federationName, std::move(plan.recipients))) {
    return internalError(request);
  }
  if (plan.status == RegionServiceStatus::applied &&
      !enqueueAttributeRelevanceAdvisories(
          commitRequest.federationName,
          std::move(plan.attributeRelevanceAdvisories))) {
    return internalError(request);
  }
  if (plan.status == RegionServiceStatus::applied) {
    rti1516_2025::RegionHandleSet reportRegions;
    for (auto const handle : regions) {
      reportRegions.insert(
          rti1516_2025::umbra_binding_detail::makeRegionHandle(handle));
    }
    if (!appendSelectedServiceReportRecord(
            session,
            commitRequest.federationName,
            commitRequest.federateId,
            static_cast<std::uint16_t>(MomServiceType::data_distribution_management),
            [reportRegions = std::move(reportRegions)](
                std::uint32_t serialNumber) {
              return formatMomSuccessfulVoidServiceReportRecord(
                  serialNumber,
                  L"CommitRegionModifications",
                  {{MomArgumentType::region_handle_set,
                    L"Set of region designators",
                    formatMomRegionHandleSet(reportRegions)}});
            })) {
      return internalError(request);
    }
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationRegionStatusResult(
          ProcessFederationRegionStatusResult{plan.status}));
}

TransportServiceMessage ProcessFederationService::handleDeleteRegion(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const regionRequest = decodeProcessFederationRegionRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != regionRequest.federationName ||
        state->second.federateId != regionRequest.federateId ||
        !registry_.memberById(
            regionRequest.federationName, regionRequest.federateId)) {
      return rejected(request);
    }
  }
  auto const status = registry_.deleteRegion(
      regionRequest.federationName,
      regionRequest.federateId,
      regionRequest.regionHandle);
  if (status == RegionServiceStatus::applied &&
      !appendSelectedServiceReportRecord(
          session,
          regionRequest.federationName,
          regionRequest.federateId,
          static_cast<std::uint16_t>(MomServiceType::data_distribution_management),
          [regionHandle = regionRequest.regionHandle](std::uint32_t serialNumber) {
            return formatMomSuccessfulVoidServiceReportRecord(
                serialNumber,
                L"DeleteRegion",
                {{MomArgumentType::region_handle,
                  L"Region designator",
                  formatMomRegionHandle(
                      rti1516_2025::umbra_binding_detail::makeRegionHandle(
                          regionHandle))}});
          })) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationRegionStatusResult(
          ProcessFederationRegionStatusResult{status}));
}

TransportServiceMessage ProcessFederationService::handleGetDimensionHandleSet(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const regionRequest = decodeProcessFederationRegionRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != regionRequest.federationName ||
        state->second.federateId != regionRequest.federateId ||
        !registry_.memberById(
            regionRequest.federationName, regionRequest.federateId)) {
      return rejected(request);
    }
  }
  auto const result = registry_.dimensionHandleSetForRegion(
      regionRequest.federationName,
      regionRequest.federateId,
      regionRequest.regionHandle);
  if (result.status == RegionServiceStatus::applied) {
    rti1516_2025::DimensionHandleSet reportHandles;
    for (auto const handle : result.dimensionHandles) {
      reportHandles.insert(
          rti1516_2025::umbra_binding_detail::makeDimensionHandle(handle));
    }
    if (!appendSelectedServiceReportRecord(
            session,
            regionRequest.federationName,
            regionRequest.federateId,
            static_cast<std::uint16_t>(MomServiceType::support_services),
            [regionHandle = regionRequest.regionHandle,
             reportHandles = std::move(reportHandles)](
                std::uint32_t serialNumber) {
              return formatMomSuccessfulServiceReportRecord(
                  serialNumber,
                  L"GetDimensionHandleSet",
                  {{MomArgumentType::region_handle,
                    L"Region handle",
                    formatMomRegionHandle(
                        rti1516_2025::umbra_binding_detail::makeRegionHandle(
                            regionHandle))}},
                  {MomArgumentType::dimension_handle_set,
                   L"A set of dimensions",
                   formatMomDimensionHandleSet(reportHandles)});
            })) {
      return internalError(request);
    }
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationDimensionSetResult(
          ProcessFederationDimensionSetResult{
              result.status,
              std::vector<std::uint64_t>(
                  result.dimensionHandles.begin(), result.dimensionHandles.end())}));
}

TransportServiceMessage ProcessFederationService::handleGetRangeBounds(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const boundsRequest =
      decodeProcessFederationGetRangeBoundsRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != boundsRequest.federationName ||
        state->second.federateId != boundsRequest.federateId ||
        !registry_.memberById(
            boundsRequest.federationName, boundsRequest.federateId)) {
      return rejected(request);
    }
  }
  auto const result = registry_.rangeBoundsForRegion(
      boundsRequest.federationName,
      boundsRequest.federateId,
      boundsRequest.regionHandle,
      boundsRequest.dimensionHandle);
  if (result.status == RegionServiceStatus::applied &&
      !appendSelectedServiceReportRecord(
          session,
          boundsRequest.federationName,
          boundsRequest.federateId,
          static_cast<std::uint16_t>(MomServiceType::data_distribution_management),
          [regionHandle = boundsRequest.regionHandle,
           dimensionHandle = boundsRequest.dimensionHandle,
           range = result.range](std::uint32_t serialNumber) {
            return formatMomSuccessfulServiceReportRecord(
                serialNumber,
                L"GetRangeBounds",
                {{MomArgumentType::region_handle,
                  L"Region handle",
                  formatMomRegionHandle(
                      rti1516_2025::umbra_binding_detail::makeRegionHandle(
                          regionHandle))},
                 {MomArgumentType::dimension_handle,
                  L"Dimension handle",
                  formatMomDimensionHandle(
                      rti1516_2025::umbra_binding_detail::makeDimensionHandle(
                          dimensionHandle))}},
                {MomArgumentType::range_bounds,
                 L"Range bounds",
                 formatMomRangeBounds(range.lowerBound, range.upperBound)});
          })) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationRangeBoundsResult(
          ProcessFederationRangeBoundsResult{
              result.status, result.range.lowerBound, result.range.upperBound}));
}

TransportServiceMessage ProcessFederationService::handleSetRangeBounds(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const boundsRequest =
      decodeProcessFederationSetRangeBoundsRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != boundsRequest.federationName ||
        state->second.federateId != boundsRequest.federateId ||
        !registry_.memberById(
            boundsRequest.federationName, boundsRequest.federateId)) {
      return rejected(request);
    }
  }
  auto const status = registry_.setRangeBounds(
      boundsRequest.federationName,
      boundsRequest.federateId,
      boundsRequest.regionHandle,
      boundsRequest.dimensionHandle,
      RegionRangeBounds{boundsRequest.lowerBound, boundsRequest.upperBound});
  if (status == RegionServiceStatus::applied &&
      !appendSelectedServiceReportRecord(
          session,
          boundsRequest.federationName,
          boundsRequest.federateId,
          static_cast<std::uint16_t>(MomServiceType::data_distribution_management),
          [regionHandle = boundsRequest.regionHandle,
           dimensionHandle = boundsRequest.dimensionHandle,
           lowerBound = boundsRequest.lowerBound,
           upperBound = boundsRequest.upperBound](
              std::uint32_t serialNumber) {
            return formatMomSuccessfulVoidServiceReportRecord(
                serialNumber,
                L"SetRangeBounds",
                {{MomArgumentType::region_handle,
                  L"Region handle",
                  formatMomRegionHandle(
                      rti1516_2025::umbra_binding_detail::makeRegionHandle(
                          regionHandle))},
                 {MomArgumentType::dimension_handle,
                  L"Dimension handle",
                  formatMomDimensionHandle(
                      rti1516_2025::umbra_binding_detail::makeDimensionHandle(
                          dimensionHandle))},
                 {MomArgumentType::number,
                  L"Range lower bound",
                  formatMomNumber(std::to_wstring(lowerBound))},
                 {MomArgumentType::number,
                  L"Range upper bound",
                  formatMomNumber(std::to_wstring(upperBound))}});
          })) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationRegionStatusResult(
          ProcessFederationRegionStatusResult{status}));
}

}  // namespace umbra::detail
