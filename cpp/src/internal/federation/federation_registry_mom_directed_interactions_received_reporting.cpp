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

constexpr char kReportDirectedInteractionsReceivedInteractionClassName[] =
    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportDirectedInteractionsReceived";

constexpr std::uint64_t kMomDirectedInteractionsReceivedEndpointRegionHandle =
    std::numeric_limits<std::uint64_t>::max() - 11U;

}  // namespace

MomDirectedInteractionsReceivedReportPlan
EmbeddedFederationRegistry::planMomDirectedInteractionsReceivedReport(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t reportedFederateId) const {
  auto instrumentationScope = beginInstrumentation(
      "planMomDirectedInteractionsReceivedReport");
  std::scoped_lock lock(mutex_);
  MomDirectedInteractionsReceivedReportPlan result;
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
      kReportDirectedInteractionsReceivedInteractionClassName);
  auto const federateDimensionHandle = federation->second.dimensionHandles->handleFor(
      kHlaFederateDimensionName);
  auto const transportationParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportDirectedInteractionsReceivedInteractionClassName,
      "HLAtransportation");
  auto const interactionCountsParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportDirectedInteractionsReceivedInteractionClassName,
      kReportInteractionCountsParameterName);
  if (!reportClassHandle || !federateDimensionHandle ||
      !transportationParameterHandle || !interactionCountsParameterHandle) {
    result.status = MomObjectInstanceCountsReportStatus::inconsistent_catalog;
    return result;
  }

  // HLAreportDirectedInteractionsReceived has the same two mandatory
  // transportation buckets and NULL response shape as its non-directed
  // counterpart, but reads only the directed receive ledger.
  result.interactionClassCountsByTransportation.try_emplace("HLAreliable");
  result.interactionClassCountsByTransportation.try_emplace("HLAbestEffort");
  for (auto const& [interactionClassHandle, byTransportation] :
       reportedMember->second.successfulDirectedInteractionReceiptCountsByClassAndTransportation) {
    if (interactionClassHandle == 0U) {
      continue;
    }
    for (auto const& [transportationName, count] : byTransportation) {
      if (count == 0U || !isSupportedTransportationName(
                              federation->second.definition.catalog.get(),
                              transportationName)) {
        continue;
      }
      result.interactionClassCountsByTransportation[transportationName][
          interactionClassHandle] = count;
    }
  }

  auto const normalizedFederate = normalizedHandleValue(
      federation->second.normalizationSeed,
      reportedFederateId,
      kFederateNormalizationKind);
  result.routing.interactionClassHandle = *reportClassHandle;
  result.routing.transportationParameterHandle = *transportationParameterHandle;
  result.routing.interactionCountsParameterHandle = *interactionCountsParameterHandle;
  result.routing.endpointRegionHandle = kMomDirectedInteractionsReceivedEndpointRegionHandle;
  result.routing.endpointRegion = {
      {*federateDimensionHandle},
      {{*federateDimensionHandle, {normalizedFederate, normalizedFederate + 1U}}},
      true,
  };
  std::vector<std::uint64_t> const sentParameterHandles{
      *transportationParameterHandle,
      *interactionCountsParameterHandle,
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
EmbeddedFederationRegistry::momDirectedInteractionsReceivedReportRecipientFor(
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
      kReportDirectedInteractionsReceivedInteractionClassName);
  auto const federateDimensionHandle = federation->second.dimensionHandles->handleFor(
      kHlaFederateDimensionName);
  auto const transportationParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportDirectedInteractionsReceivedInteractionClassName,
      "HLAtransportation");
  auto const interactionCountsParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportDirectedInteractionsReceivedInteractionClassName,
      kReportInteractionCountsParameterName);
  if (!reportClassHandle || !federateDimensionHandle ||
      !transportationParameterHandle || !interactionCountsParameterHandle) {
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
      {kMomDirectedInteractionsReceivedEndpointRegionHandle, endpointRegion},
  };
  std::set<std::uint64_t> const endpointRegionHandles{
      kMomDirectedInteractionsReceivedEndpointRegionHandle,
  };
  return candidateReceiveOrderInteractionRecipient(
      federation->second,
      InteractionProducer::rti(),
      receivingFederateId,
      *reportClassHandle,
      {*transportationParameterHandle, *interactionCountsParameterHandle},
      &endpointRegionHandles,
      &endpointOverride);
}

}  // namespace umbra::detail
