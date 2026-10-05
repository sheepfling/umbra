#include "internal/federation/federation_registry.hpp"
#include "internal/federation/federation_registry_constants.hpp"
#include "internal/fom/fom_catalog.hpp"
#include "internal/time/federation_time_grant_policy.hpp"

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

constexpr char kFederateLostFederateNameParameterName[] = "HLAfederateName";
constexpr char kFederateLostTimestampParameterName[] = "HLAtimeStamp";
constexpr char kFederateLostFaultDescriptionParameterName[] = "HLAfaultDescription";
constexpr std::uint64_t kMomFederateLostEndpointRegionHandle =
    std::numeric_limits<std::uint64_t>::max() - 1U;

}  // namespace

std::optional<FederateLostReportRouting>
EmbeddedFederationRegistry::federateLostReportRoutingFor(
    Federation const& federation,
    std::uint64_t reportedFederateId) {
  if (reportedFederateId == 0U || !federation.definition.catalog ||
      !federation.interactionClassHandles || !federation.parameterHandles ||
      !federation.dimensionHandles) {
    return std::nullopt;
  }

  auto const reportClassHandle = federation.interactionClassHandles->handleFor(
      kReportFederateLostInteractionClassName);
  auto const federateDimensionHandle = federation.dimensionHandles->handleFor(
      kHlaFederateDimensionName);
  auto const federateParameterHandle = federation.parameterHandles->handleFor(
      federation.definition.catalog.get(),
      kReportFederateLostInteractionClassName,
      kFederateLostFederateParameterName);
  auto const federateNameParameterHandle = federation.parameterHandles->handleFor(
      federation.definition.catalog.get(),
      kReportFederateLostInteractionClassName,
      kFederateLostFederateNameParameterName);
  auto const timestampParameterHandle = federation.parameterHandles->handleFor(
      federation.definition.catalog.get(),
      kReportFederateLostInteractionClassName,
      kFederateLostTimestampParameterName);
  auto const faultDescriptionParameterHandle = federation.parameterHandles->handleFor(
      federation.definition.catalog.get(),
      kReportFederateLostInteractionClassName,
      kFederateLostFaultDescriptionParameterName);
  if (!reportClassHandle || !federateDimensionHandle || !federateParameterHandle ||
      !federateNameParameterHandle || !timestampParameterHandle ||
      !faultDescriptionParameterHandle) {
    return std::nullopt;
  }

  auto const normalizedFederate = normalizedHandleValue(
      federation.normalizationSeed,
      reportedFederateId,
      kFederateNormalizationKind);
  FederateLostReportRouting result;
  result.interactionClassHandle = *reportClassHandle;
  result.federateParameterHandle = *federateParameterHandle;
  result.federateNameParameterHandle = *federateNameParameterHandle;
  result.timestampParameterHandle = *timestampParameterHandle;
  result.faultDescriptionParameterHandle = *faultDescriptionParameterHandle;
  result.endpointRegionHandle = kMomFederateLostEndpointRegionHandle;
  result.endpointRegion = {
      {*federateDimensionHandle},
      {{*federateDimensionHandle, {normalizedFederate, normalizedFederate + 1U}}},
      true,
  };
  return result;
}

FederateLostReportPlan EmbeddedFederationRegistry::planFederateLostReport(
    std::wstring const& federationName,
    std::uint64_t reportedFederateId) const {
  auto instrumentationScope = beginInstrumentation("planFederateLostReport");
  std::scoped_lock lock(mutex_);
  FederateLostReportPlan result;
  result.reportedFederateId = reportedFederateId;
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status = FederateLostReportStatus::federation_does_not_exist;
    return result;
  }
  auto const reportedMember = federation->second.members.find(reportedFederateId);
  if (reportedMember == federation->second.members.end()) {
    result.status = FederateLostReportStatus::reported_federate_not_member;
    return result;
  }
  auto routing = federateLostReportRoutingFor(federation->second, reportedFederateId);
  if (!routing) {
    result.status = FederateLostReportStatus::inconsistent_catalog;
    return result;
  }
  auto const timeState = federation->second.timeCoordinator.timeStateFor(reportedFederateId);
  if (!timeState) {
    result.status = FederateLostReportStatus::inconsistent_time_state;
    return result;
  }
  auto const time = timeState->snapshot();
  if (!time.active || !time.currentTime) {
    result.status = FederateLostReportStatus::inconsistent_time_state;
    return result;
  }

  result.reportedFederateName = reportedMember->second.name;
  result.reportedFederateWasTimeRegulating = time.timeRegulating;
  result.lastKnownTime = time.currentTime;
  result.routing = std::move(*routing);
  std::vector<std::uint64_t> const sentParameterHandles{
      result.routing.federateParameterHandle,
      result.routing.federateNameParameterHandle,
      result.routing.timestampParameterHandle,
      result.routing.faultDescriptionParameterHandle,
  };
  std::map<std::uint64_t, RegionSpecificationSnapshot> const endpointOverride{
      {result.routing.endpointRegionHandle, result.routing.endpointRegion},
  };
  std::set<std::uint64_t> const endpointRegionHandles{
      result.routing.endpointRegionHandle,
  };
  for (auto const& [federateId, membership] : federation->second.members) {
    static_cast<void>(membership);
    // The source names joined federates that remain in the federation. The
    // transport-fault target is still a member while this plan is captured,
    // so it must be excluded explicitly before its resignation occurs.
    if (federateId == reportedFederateId) {
      continue;
    }
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
EmbeddedFederationRegistry::federateLostReportRecipientFor(
    std::wstring const& federationName,
    std::uint64_t reportedFederateId,
    std::uint64_t receivingFederateId) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() || receivingFederateId == reportedFederateId ||
      !federation->second.members.contains(receivingFederateId) ||
      !federation->second.federateNamesById.contains(reportedFederateId)) {
    return std::nullopt;
  }
  auto routing = federateLostReportRoutingFor(federation->second, reportedFederateId);
  if (!routing) {
    return std::nullopt;
  }
  std::vector<std::uint64_t> const sentParameterHandles{
      routing->federateParameterHandle,
      routing->federateNameParameterHandle,
      routing->timestampParameterHandle,
      routing->faultDescriptionParameterHandle,
  };
  std::map<std::uint64_t, RegionSpecificationSnapshot> const endpointOverride{
      {routing->endpointRegionHandle, routing->endpointRegion},
  };
  std::set<std::uint64_t> const endpointRegionHandles{
      routing->endpointRegionHandle,
  };
  return candidateReceiveOrderInteractionRecipient(
      federation->second,
      InteractionProducer::rti(),
      receivingFederateId,
      routing->interactionClassHandle,
      sentParameterHandles,
      &endpointRegionHandles,
      &endpointOverride);
}


}  // namespace umbra::detail
