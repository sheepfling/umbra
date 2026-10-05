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

constexpr char kReportObjectInstanceInformationInteractionClassName[] =
    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectInstanceInformation";
constexpr std::uint64_t kMomObjectInstanceInformationEndpointRegionHandle =
    std::numeric_limits<std::uint64_t>::max() - 12U;

}  // namespace


MomObjectInstanceInformationReportPlan
EmbeddedFederationRegistry::planMomObjectInstanceInformationReport(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle) const {
  auto instrumentationScope = beginInstrumentation(
      "planMomObjectInstanceInformationReport");
  std::scoped_lock lock(mutex_);
  MomObjectInstanceInformationReportPlan result;
  result.reportedFederateId = requestingFederateId;
  result.objectInstanceHandle = objectInstanceHandle;
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status =
        MomObjectInstanceInformationReportStatus::federation_does_not_exist;
    return result;
  }
  if (!federation->second.members.contains(requestingFederateId)) {
    result.status =
        MomObjectInstanceInformationReportStatus::requesting_federate_not_member;
    return result;
  }
  if (!federation->second.definition.catalog ||
      !federation->second.interactionClassHandles ||
      !federation->second.parameterHandles ||
      !federation->second.dimensionHandles) {
    result.status =
        MomObjectInstanceInformationReportStatus::inconsistent_catalog;
    return result;
  }

  auto const reportClassHandle = federation->second.interactionClassHandles->handleFor(
      kReportObjectInstanceInformationInteractionClassName);
  auto const federateDimensionHandle = federation->second.dimensionHandles->handleFor(
      kHlaFederateDimensionName);
  auto const objectInstanceParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportObjectInstanceInformationInteractionClassName,
      "HLAobjectInstance");
  auto const ownedInstanceAttributeListParameterHandle =
      federation->second.parameterHandles->handleFor(
          federation->second.definition.catalog.get(),
          kReportObjectInstanceInformationInteractionClassName,
          "HLAownedInstanceAttributeList");
  auto const registeredClassParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportObjectInstanceInformationInteractionClassName,
      "HLAregisteredClass");
  auto const knownClassParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportObjectInstanceInformationInteractionClassName,
      "HLAknownClass");
  if (!reportClassHandle || !federateDimensionHandle ||
      !objectInstanceParameterHandle ||
      !ownedInstanceAttributeListParameterHandle ||
      !registeredClassParameterHandle || !knownClassParameterHandle) {
    result.status =
        MomObjectInstanceInformationReportStatus::inconsistent_catalog;
    return result;
  }

  auto const instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance != federation->second.objectInstances.end() &&
      !instance->second.deleteAccepted) {
    auto const knownClass = instance->second.knownObjectClassHandlesByFederate.find(
        requestingFederateId);
    if (knownClass != instance->second.knownObjectClassHandlesByFederate.end()) {
      result.objectKnown = true;
      result.registeredObjectClassHandle = instance->second.registeredObjectClassHandle;
      result.knownObjectClassHandle = knownClass->second;
      for (auto const& [attributeHandle, owner] : instance->second.attributeOwnersByHandle) {
        if (owner == requestingFederateId) {
          result.ownedAttributeHandles.insert(attributeHandle);
        }
      }
    }
  }

  auto const normalizedFederate = normalizedHandleValue(
      federation->second.normalizationSeed,
      requestingFederateId,
      kFederateNormalizationKind);
  result.routing.interactionClassHandle = *reportClassHandle;
  result.routing.objectInstanceParameterHandle = *objectInstanceParameterHandle;
  result.routing.ownedInstanceAttributeListParameterHandle =
      *ownedInstanceAttributeListParameterHandle;
  result.routing.registeredClassParameterHandle = *registeredClassParameterHandle;
  result.routing.knownClassParameterHandle = *knownClassParameterHandle;
  result.routing.endpointRegionHandle =
      kMomObjectInstanceInformationEndpointRegionHandle;
  result.routing.endpointRegion = {
      {*federateDimensionHandle},
      {{*federateDimensionHandle, {normalizedFederate, normalizedFederate + 1U}}},
      true,
  };
  std::vector<std::uint64_t> const sentParameterHandles{
      *objectInstanceParameterHandle,
      *ownedInstanceAttributeListParameterHandle,
      *registeredClassParameterHandle,
      *knownClassParameterHandle,
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
EmbeddedFederationRegistry::momObjectInstanceInformationReportRecipientFor(
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
      kReportObjectInstanceInformationInteractionClassName);
  auto const federateDimensionHandle = federation->second.dimensionHandles->handleFor(
      kHlaFederateDimensionName);
  auto const objectInstanceParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportObjectInstanceInformationInteractionClassName,
      "HLAobjectInstance");
  auto const ownedInstanceAttributeListParameterHandle =
      federation->second.parameterHandles->handleFor(
          federation->second.definition.catalog.get(),
          kReportObjectInstanceInformationInteractionClassName,
          "HLAownedInstanceAttributeList");
  auto const registeredClassParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportObjectInstanceInformationInteractionClassName,
      "HLAregisteredClass");
  auto const knownClassParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportObjectInstanceInformationInteractionClassName,
      "HLAknownClass");
  if (!reportClassHandle || !federateDimensionHandle ||
      !objectInstanceParameterHandle ||
      !ownedInstanceAttributeListParameterHandle ||
      !registeredClassParameterHandle || !knownClassParameterHandle) {
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
      {kMomObjectInstanceInformationEndpointRegionHandle, endpointRegion},
  };
  std::set<std::uint64_t> const endpointRegionHandles{
      kMomObjectInstanceInformationEndpointRegionHandle,
  };
  return candidateReceiveOrderInteractionRecipient(
      federation->second,
      InteractionProducer::rti(),
      receivingFederateId,
      *reportClassHandle,
      {*objectInstanceParameterHandle,
       *ownedInstanceAttributeListParameterHandle,
       *registeredClassParameterHandle,
       *knownClassParameterHandle},
      &endpointRegionHandles,
      &endpointOverride);
}

}  // namespace umbra::detail
