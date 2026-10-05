#include "internal/federation/federation_registry.hpp"
#include "internal/federation/federation_registry_constants.hpp"
#include "internal/fom/fom_catalog.hpp"

#include <cstdint>
#include <limits>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace umbra::detail {

namespace {

constexpr char kReportObjectInstancesReflectedInteractionClassName[] =
    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectInstancesReflected";
constexpr std::uint64_t kMomObjectInstancesReflectedEndpointRegionHandle =
    std::numeric_limits<std::uint64_t>::max() - 5U;

}  // namespace


MomObjectInstanceCountsReportPlan
EmbeddedFederationRegistry::planMomObjectInstancesReflectedReport(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t reportedFederateId) const {
  auto instrumentationScope = beginInstrumentation(
      "planMomObjectInstancesReflectedReport");
  std::scoped_lock lock(mutex_);
  MomObjectInstanceCountsReportPlan result;
  result.reportedFederateId = reportedFederateId;
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status = MomObjectInstanceCountsReportStatus::federation_does_not_exist;
    return result;
  }
  if (!federation->second.members.contains(requestingFederateId)) {
    result.status = MomObjectInstanceCountsReportStatus::requesting_federate_not_member;
    return result;
  }
  if (!federation->second.members.contains(reportedFederateId)) {
    result.status = MomObjectInstanceCountsReportStatus::reported_federate_not_member;
    return result;
  }
  if (!federation->second.definition.catalog ||
      !federation->second.interactionClassHandles ||
      !federation->second.parameterHandles ||
      !federation->second.dimensionHandles) {
    result.status = MomObjectInstanceCountsReportStatus::inconsistent_catalog;
    return result;
  }

  auto const reportClassHandle = federation->second.interactionClassHandles->handleFor(
      kReportObjectInstancesReflectedInteractionClassName);
  auto const federateDimensionHandle = federation->second.dimensionHandles->handleFor(
      kHlaFederateDimensionName);
  auto const countsParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportObjectInstancesReflectedInteractionClassName,
      kReportObjectInstanceCountsParameterName);
  if (!reportClassHandle || !federateDimensionHandle || !countsParameterHandle) {
    result.status = MomObjectInstanceCountsReportStatus::inconsistent_catalog;
    return result;
  }

  for (auto const& [objectInstanceHandle, objectClassHandle] :
       federation->second.members.at(reportedFederateId)
           .successfullyReflectedObjectInstanceClassHandles) {
    static_cast<void>(objectInstanceHandle);
    if (objectClassHandle == 0U) {
      continue;
    }
    auto& count = result.objectClassCounts[objectClassHandle];
    if (count != std::numeric_limits<std::uint64_t>::max()) {
      ++count;
    }
  }

  auto const normalizedFederate = normalizedHandleValue(
      federation->second.normalizationSeed,
      reportedFederateId,
      kFederateNormalizationKind);
  result.routing.interactionClassHandle = *reportClassHandle;
  result.routing.objectInstanceCountsParameterHandle = *countsParameterHandle;
  result.routing.endpointRegionHandle = kMomObjectInstancesReflectedEndpointRegionHandle;
  result.routing.endpointRegion = {
      {*federateDimensionHandle},
      {{*federateDimensionHandle, {normalizedFederate, normalizedFederate + 1U}}},
      true,
  };
  std::vector<std::uint64_t> const sentParameterHandles{*countsParameterHandle};
  std::map<std::uint64_t, RegionSpecificationSnapshot> const endpointOverride{
      {result.routing.endpointRegionHandle, result.routing.endpointRegion},
  };
  std::set<std::uint64_t> const endpointRegionHandles{
      result.routing.endpointRegionHandle,
  };
  for (auto const& [federateId, membership] : federation->second.members) {
    static_cast<void>(membership);
    auto recipient = candidateReceiveOrderInteractionRecipient(
        federation->second,
        InteractionProducer::rti(),
        federateId,
        result.routing.interactionClassHandle,
        sentParameterHandles,
        &endpointRegionHandles,
        &endpointOverride);
    if (recipient) {
      result.recipients.push_back(std::move(*recipient));
    }
  }
  return result;
}

std::optional<ReceiveOrderInteractionRecipient>
EmbeddedFederationRegistry::momObjectInstancesReflectedReportRecipientFor(
    std::wstring const& federationName,
    std::uint64_t reportedFederateId,
    std::uint64_t receivingFederateId) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      !federation->second.members.contains(reportedFederateId) ||
      !federation->second.members.contains(receivingFederateId) ||
      !federation->second.definition.catalog ||
      !federation->second.interactionClassHandles ||
      !federation->second.parameterHandles ||
      !federation->second.dimensionHandles) {
    return std::nullopt;
  }
  auto const reportClassHandle = federation->second.interactionClassHandles->handleFor(
      kReportObjectInstancesReflectedInteractionClassName);
  auto const federateDimensionHandle = federation->second.dimensionHandles->handleFor(
      kHlaFederateDimensionName);
  auto const countsParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportObjectInstancesReflectedInteractionClassName,
      kReportObjectInstanceCountsParameterName);
  if (!reportClassHandle || !federateDimensionHandle || !countsParameterHandle) {
    return std::nullopt;
  }
  auto const normalizedFederate = normalizedHandleValue(
      federation->second.normalizationSeed,
      reportedFederateId,
      kFederateNormalizationKind);
  auto const endpointRegionHandle = kMomObjectInstancesReflectedEndpointRegionHandle;
  RegionSpecificationSnapshot const endpointRegion{
      {*federateDimensionHandle},
      {{*federateDimensionHandle, {normalizedFederate, normalizedFederate + 1U}}},
      true,
  };
  std::map<std::uint64_t, RegionSpecificationSnapshot> const endpointOverride{
      {endpointRegionHandle, endpointRegion},
  };
  std::set<std::uint64_t> const endpointRegionHandles{endpointRegionHandle};
  return candidateReceiveOrderInteractionRecipient(
      federation->second,
      InteractionProducer::rti(),
      receivingFederateId,
      *reportClassHandle,
      {*countsParameterHandle},
      &endpointRegionHandles,
      &endpointOverride);
}

}  // namespace umbra::detail
