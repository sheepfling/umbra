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

constexpr char kReportMomExceptionInteractionClassName[] =
    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportMOMexception";
constexpr char kMomExceptionReportParameterErrorParameterName[] =
    "HLAparameterError";
constexpr std::uint64_t kMomMomExceptionReportEndpointRegionHandle =
    std::numeric_limits<std::uint64_t>::max() - 16U;

}  // namespace

MomExceptionReportPlan EmbeddedFederationRegistry::planMomExceptionReport(
    std::wstring const& federationName,
    std::uint64_t reportedFederateId) const {
  auto instrumentationScope = beginInstrumentation("planMomExceptionReport");
  std::scoped_lock lock(mutex_);
  MomExceptionReportPlan result;
  result.reportedFederateId = reportedFederateId;
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status = MomExceptionReportStatus::federation_does_not_exist;
    return result;
  }
  if (!federation->second.members.contains(reportedFederateId)) {
    result.status = MomExceptionReportStatus::reported_federate_not_member;
    return result;
  }
  if (!federation->second.definition.catalog ||
      !federation->second.interactionClassHandles ||
      !federation->second.parameterHandles ||
      !federation->second.dimensionHandles) {
    result.status = MomExceptionReportStatus::inconsistent_catalog;
    return result;
  }

  auto const reportClassHandle = federation->second.interactionClassHandles->handleFor(
      kReportMomExceptionInteractionClassName);
  auto const federateDimensionHandle = federation->second.dimensionHandles->handleFor(
      kHlaFederateDimensionName);
  auto const federateParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportMomExceptionInteractionClassName,
      kFederateLostFederateParameterName);
  auto const serviceParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportMomExceptionInteractionClassName,
      kExceptionReportServiceParameterName);
  auto const exceptionParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportMomExceptionInteractionClassName,
      kExceptionReportExceptionParameterName);
  auto const parameterErrorParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportMomExceptionInteractionClassName,
      kMomExceptionReportParameterErrorParameterName);
  if (!reportClassHandle || !federateDimensionHandle ||
      !federateParameterHandle || !serviceParameterHandle || !exceptionParameterHandle ||
      !parameterErrorParameterHandle) {
    result.status = MomExceptionReportStatus::inconsistent_catalog;
    return result;
  }

  auto const normalizedFederate = normalizedHandleValue(
      federation->second.normalizationSeed,
      reportedFederateId,
      kFederateNormalizationKind);
  result.routing.interactionClassHandle = *reportClassHandle;
  result.routing.federateParameterHandle = *federateParameterHandle;
  result.routing.serviceParameterHandle = *serviceParameterHandle;
  result.routing.exceptionParameterHandle = *exceptionParameterHandle;
  result.routing.parameterErrorParameterHandle = *parameterErrorParameterHandle;
  result.routing.endpointRegionHandle = kMomMomExceptionReportEndpointRegionHandle;
  result.routing.endpointRegion = {
      {*federateDimensionHandle},
      {{*federateDimensionHandle, {normalizedFederate, normalizedFederate + 1U}}},
      true,
  };
  std::vector<std::uint64_t> const sentParameterHandles{
      result.routing.federateParameterHandle,
      result.routing.serviceParameterHandle,
      result.routing.exceptionParameterHandle,
      result.routing.parameterErrorParameterHandle,
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
EmbeddedFederationRegistry::momExceptionReportRecipientFor(
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
      kReportMomExceptionInteractionClassName);
  auto const federateDimensionHandle = federation->second.dimensionHandles->handleFor(
      kHlaFederateDimensionName);
  auto const federateParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportMomExceptionInteractionClassName,
      kFederateLostFederateParameterName);
  auto const serviceParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportMomExceptionInteractionClassName,
      kExceptionReportServiceParameterName);
  auto const exceptionParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportMomExceptionInteractionClassName,
      kExceptionReportExceptionParameterName);
  auto const parameterErrorParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportMomExceptionInteractionClassName,
      kMomExceptionReportParameterErrorParameterName);
  if (!reportClassHandle || !federateDimensionHandle ||
      !federateParameterHandle || !serviceParameterHandle || !exceptionParameterHandle ||
      !parameterErrorParameterHandle) {
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
      {kMomMomExceptionReportEndpointRegionHandle, endpointRegion},
  };
  std::set<std::uint64_t> const endpointRegionHandles{
      kMomMomExceptionReportEndpointRegionHandle,
  };
  return candidateReceiveOrderInteractionRecipient(
      federation->second,
      InteractionProducer::rti(),
      receivingFederateId,
      *reportClassHandle,
      {*federateParameterHandle,
       *serviceParameterHandle,
       *exceptionParameterHandle,
       *parameterErrorParameterHandle},
      &endpointRegionHandles,
      &endpointOverride);
}


}  // namespace umbra::detail
