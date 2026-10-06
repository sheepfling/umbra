#include "internal/federation/federation_registry.hpp"
#include "internal/federation/federation_registry_constants.hpp"
#include "internal/federation/federation_registry_value_helpers.hpp"

#include <cstdint>
#include <limits>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <utility>
#include <vector>

namespace umbra::detail {

namespace {

constexpr char kReportReflectionsReceivedInteractionClassName[] =
    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportReflectionsReceived";
constexpr char kReportReflectionCountsParameterName[] = "HLAreflectCounts";
constexpr std::uint64_t kMomReflectionsReceivedEndpointRegionHandle =
    std::numeric_limits<std::uint64_t>::max() - 9U;

}  // namespace

MomReflectionsReceivedReportPlan
EmbeddedFederationRegistry::planMomReflectionsReceivedReport(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t reportedFederateId) const {
  auto instrumentationScope = beginInstrumentation(
      "planMomReflectionsReceivedReport");
  std::scoped_lock lock(mutex_);
  MomReflectionsReceivedReportPlan result;
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
      kReportReflectionsReceivedInteractionClassName);
  auto const federateDimensionHandle = federation->second.dimensionHandles->handleFor(
      kHlaFederateDimensionName);
  auto const transportationParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportReflectionsReceivedInteractionClassName,
      "HLAtransportation");
  auto const reflectionCountsParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportReflectionsReceivedInteractionClassName,
      kReportReflectionCountsParameterName);
  if (!reportClassHandle || !federateDimensionHandle ||
      !transportationParameterHandle || !reflectionCountsParameterHandle) {
    result.status = MomObjectInstanceCountsReportStatus::inconsistent_catalog;
    return result;
  }

  // HLAreportReflectionsReceived requires one response for each standard
  // transportation type, including an empty HLAobjectClassBasedCounts NULL
  // response when that bucket has no accepted reflection callbacks.
  result.objectClassCountsByTransportation.try_emplace("HLAreliable");
  result.objectClassCountsByTransportation.try_emplace("HLAbestEffort");
  for (auto const& [objectClassHandle, byTransportation] :
       reportedMember->second.successfulReflectionCountsByClassAndTransportation) {
    if (objectClassHandle == 0U) {
      continue;
    }
    for (auto const& [transportationName, count] : byTransportation) {
      if (count == 0U || !isSupportedTransportationName(
                              federation->second.definition.catalog.get(),
                              transportationName)) {
        continue;
      }
      result.objectClassCountsByTransportation[transportationName][
          objectClassHandle] = count;
    }
  }

  auto const normalizedFederate = normalizedHandleValue(
      federation->second.normalizationSeed,
      reportedFederateId,
      kFederateNormalizationKind);
  result.routing.interactionClassHandle = *reportClassHandle;
  result.routing.transportationParameterHandle = *transportationParameterHandle;
  result.routing.reflectionCountsParameterHandle = *reflectionCountsParameterHandle;
  result.routing.endpointRegionHandle = kMomReflectionsReceivedEndpointRegionHandle;
  result.routing.endpointRegion = {
      {*federateDimensionHandle},
      {{*federateDimensionHandle, {normalizedFederate, normalizedFederate + 1U}}},
      true,
  };
  std::vector<std::uint64_t> const sentParameterHandles{
      *transportationParameterHandle,
      *reflectionCountsParameterHandle,
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
EmbeddedFederationRegistry::momReflectionsReceivedReportRecipientFor(
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
      kReportReflectionsReceivedInteractionClassName);
  auto const federateDimensionHandle = federation->second.dimensionHandles->handleFor(
      kHlaFederateDimensionName);
  auto const transportationParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportReflectionsReceivedInteractionClassName,
      "HLAtransportation");
  auto const reflectionCountsParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportReflectionsReceivedInteractionClassName,
      kReportReflectionCountsParameterName);
  if (!reportClassHandle || !federateDimensionHandle ||
      !transportationParameterHandle || !reflectionCountsParameterHandle) {
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
      {kMomReflectionsReceivedEndpointRegionHandle, endpointRegion},
  };
  std::set<std::uint64_t> const endpointRegionHandles{
      kMomReflectionsReceivedEndpointRegionHandle,
  };
  return candidateReceiveOrderInteractionRecipient(
      federation->second,
      InteractionProducer::rti(),
      receivingFederateId,
      *reportClassHandle,
      {*transportationParameterHandle, *reflectionCountsParameterHandle},
      &endpointRegionHandles,
      &endpointOverride);
}

}  // namespace umbra::detail
