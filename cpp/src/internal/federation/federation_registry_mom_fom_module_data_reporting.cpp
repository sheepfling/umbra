#include "internal/federation/federation_registry.hpp"
#include "internal/federation/federation_registry_constants.hpp"
#include "internal/fom/fom_catalog.hpp"

#include <RTI/encoding/BasicDataElements.h>

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

constexpr char kReportFomModuleDataInteractionClassName[] =
    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportFOMmoduleData";
constexpr std::uint64_t kMomFomModuleDataEndpointRegionHandle =
    std::numeric_limits<std::uint64_t>::max() - 15U;

}  // namespace


MomFomModuleDataReportPlan EmbeddedFederationRegistry::planMomFomModuleDataReport(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t reportedFederateId,
    std::uint32_t moduleIndex) const {
  auto instrumentationScope = beginInstrumentation("planMomFomModuleDataReport");
  std::scoped_lock lock(mutex_);
  MomFomModuleDataReportPlan result;
  result.reportedFederateId = reportedFederateId;
  result.moduleIndex = moduleIndex;
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status = MomFomModuleDataReportStatus::federation_does_not_exist;
    return result;
  }
  if (!federation->second.members.contains(requestingFederateId)) {
    result.status = MomFomModuleDataReportStatus::requesting_federate_not_member;
    return result;
  }
  if (!federation->second.members.contains(reportedFederateId)) {
    result.status = MomFomModuleDataReportStatus::reported_federate_not_member;
    return result;
  }
  if (moduleIndex > static_cast<std::uint32_t>(
          std::numeric_limits<rti1516_2025::Integer32>::max())) {
    result.status = MomFomModuleDataReportStatus::invalid_module_index;
    return result;
  }
  if (!federation->second.definition.catalog ||
      !federation->second.interactionClassHandles ||
      !federation->second.parameterHandles ||
      !federation->second.dimensionHandles) {
    result.status = MomFomModuleDataReportStatus::inconsistent_catalog;
    return result;
  }

  auto const reportClassHandle = federation->second.interactionClassHandles->handleFor(
      kReportFomModuleDataInteractionClassName);
  auto const federateDimensionHandle = federation->second.dimensionHandles->handleFor(
      kHlaFederateDimensionName);
  auto const moduleIndicatorParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportFomModuleDataInteractionClassName,
      "HLAFOMmoduleIndicator");
  auto const moduleDataParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportFomModuleDataInteractionClassName,
      "HLAFOMmoduleData");
  if (!reportClassHandle || !federateDimensionHandle ||
      !moduleIndicatorParameterHandle || !moduleDataParameterHandle) {
    result.status = MomFomModuleDataReportStatus::inconsistent_catalog;
    return result;
  }

  JoinedFederateMomObjectSnapshot const* snapshot = nullptr;
  for (auto const& [objectHandle, object] :
       federation->second.rtiOwnedJoinedFederateMomObjects) {
    static_cast<void>(objectHandle);
    if (!object.federationExecutionObject &&
        object.joinedFederateId == reportedFederateId) {
      snapshot = &object;
      break;
    }
  }
  if (snapshot == nullptr || moduleIndex >= snapshot->fomModuleContents.size()) {
    result.status = MomFomModuleDataReportStatus::invalid_module_index;
    return result;
  }
  result.moduleData = snapshot->fomModuleContents[moduleIndex];

  auto const normalizedFederate = normalizedHandleValue(
      federation->second.normalizationSeed,
      reportedFederateId,
      kFederateNormalizationKind);
  result.routing.interactionClassHandle = *reportClassHandle;
  result.routing.moduleIndicatorParameterHandle = *moduleIndicatorParameterHandle;
  result.routing.moduleDataParameterHandle = *moduleDataParameterHandle;
  result.routing.endpointRegionHandle = kMomFomModuleDataEndpointRegionHandle;
  result.routing.endpointRegion = {
      {*federateDimensionHandle},
      {{*federateDimensionHandle, {normalizedFederate, normalizedFederate + 1U}}},
      true,
  };
  std::vector<std::uint64_t> const sentParameterHandles{
      *moduleIndicatorParameterHandle,
      *moduleDataParameterHandle,
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
EmbeddedFederationRegistry::momFomModuleDataReportRecipientFor(
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
      kReportFomModuleDataInteractionClassName);
  auto const federateDimensionHandle = federation->second.dimensionHandles->handleFor(
      kHlaFederateDimensionName);
  auto const moduleIndicatorParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportFomModuleDataInteractionClassName,
      "HLAFOMmoduleIndicator");
  auto const moduleDataParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportFomModuleDataInteractionClassName,
      "HLAFOMmoduleData");
  if (!reportClassHandle || !federateDimensionHandle ||
      !moduleIndicatorParameterHandle || !moduleDataParameterHandle) {
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
      {kMomFomModuleDataEndpointRegionHandle, endpointRegion},
  };
  std::set<std::uint64_t> const endpointRegionHandles{
      kMomFomModuleDataEndpointRegionHandle,
  };
  return candidateReceiveOrderInteractionRecipient(
      federation->second,
      InteractionProducer::rti(),
      receivingFederateId,
      *reportClassHandle,
      {*moduleIndicatorParameterHandle, *moduleDataParameterHandle},
      &endpointRegionHandles,
      &endpointOverride);
}

}  // namespace umbra::detail
