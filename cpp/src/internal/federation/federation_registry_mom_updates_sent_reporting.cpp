#include "internal/federation/federation_registry.hpp"
#include "internal/federation/federation_registry_constants.hpp"
#include "internal/federation/federation_registry_value_helpers.hpp"

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

constexpr char kReportUpdatesSentInteractionClassName[] =
    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportUpdatesSent";

constexpr std::uint64_t kMomUpdatesSentEndpointRegionHandle =
    std::numeric_limits<std::uint64_t>::max() - 6U;

}  // namespace

MomUpdatesSentReportPlan
EmbeddedFederationRegistry::planMomUpdatesSentReport(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t reportedFederateId) const {
  auto instrumentationScope = beginInstrumentation("planMomUpdatesSentReport");
  std::scoped_lock lock(mutex_);
  MomUpdatesSentReportPlan result;
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
  auto const reportedMember = federation->second.members.find(reportedFederateId);
  if (reportedMember == federation->second.members.end()) {
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
      kReportUpdatesSentInteractionClassName);
  auto const federateDimensionHandle = federation->second.dimensionHandles->handleFor(
      kHlaFederateDimensionName);
  auto const transportationParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportUpdatesSentInteractionClassName,
      "HLAtransportation");
  auto const updateCountsParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportUpdatesSentInteractionClassName,
      "HLAupdateCounts");
  if (!reportClassHandle || !federateDimensionHandle ||
      !transportationParameterHandle || !updateCountsParameterHandle) {
    result.status = MomObjectInstanceCountsReportStatus::inconsistent_catalog;
    return result;
  }

  // HLAreportUpdatesSent requires one response for each supported
  // transportation type, including an empty HLAupdateCounts NULL response.
  result.objectClassCountsByTransportation.try_emplace("HLAreliable");
  result.objectClassCountsByTransportation.try_emplace("HLAbestEffort");
  for (auto const& [objectClassHandle, byTransportation] :
       reportedMember->second.successfulUpdateCountsByClassAndTransportation) {
    if (objectClassHandle == 0U) {
      continue;
    }
    for (auto const& [transportationName, count] : byTransportation) {
      if (count == 0U || !isSupportedTransportationName(
                              federation->second.definition.catalog.get(),
                              transportationName)) {
        continue;
      }
      result.objectClassCountsByTransportation[transportationName][objectClassHandle] =
          count;
    }
  }

  auto const normalizedFederate = normalizedHandleValue(
      federation->second.normalizationSeed,
      reportedFederateId,
      kFederateNormalizationKind);
  result.routing.interactionClassHandle = *reportClassHandle;
  result.routing.transportationParameterHandle = *transportationParameterHandle;
  result.routing.updateCountsParameterHandle = *updateCountsParameterHandle;
  result.routing.endpointRegionHandle = kMomUpdatesSentEndpointRegionHandle;
  result.routing.endpointRegion = {
      {*federateDimensionHandle},
      {{*federateDimensionHandle, {normalizedFederate, normalizedFederate + 1U}}},
      true,
  };
  std::vector<std::uint64_t> const sentParameterHandles{
      *transportationParameterHandle,
      *updateCountsParameterHandle,
  };
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
EmbeddedFederationRegistry::momUpdatesSentReportRecipientFor(
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
      kReportUpdatesSentInteractionClassName);
  auto const federateDimensionHandle = federation->second.dimensionHandles->handleFor(
      kHlaFederateDimensionName);
  auto const transportationParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportUpdatesSentInteractionClassName,
      "HLAtransportation");
  auto const updateCountsParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportUpdatesSentInteractionClassName,
      "HLAupdateCounts");
  if (!reportClassHandle || !federateDimensionHandle ||
      !transportationParameterHandle || !updateCountsParameterHandle) {
    return std::nullopt;
  }
  auto const normalizedFederate = normalizedHandleValue(
      federation->second.normalizationSeed,
      reportedFederateId,
      kFederateNormalizationKind);
  RegionSpecificationSnapshot const endpointRegion{
      {*federateDimensionHandle},
      {{*federateDimensionHandle, {normalizedFederate, normalizedFederate + 1U}}},
      true,
  };
  std::map<std::uint64_t, RegionSpecificationSnapshot> const endpointOverride{
      {kMomUpdatesSentEndpointRegionHandle, endpointRegion},
  };
  std::set<std::uint64_t> const endpointRegionHandles{
      kMomUpdatesSentEndpointRegionHandle,
  };
  return candidateReceiveOrderInteractionRecipient(
      federation->second,
      InteractionProducer::rti(),
      receivingFederateId,
      *reportClassHandle,
      {*transportationParameterHandle, *updateCountsParameterHandle},
      &endpointRegionHandles,
      &endpointOverride);
}


}  // namespace umbra::detail
