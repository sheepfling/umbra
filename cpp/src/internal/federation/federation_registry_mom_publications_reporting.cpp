#include "internal/federation/federation_registry.hpp"
#include "internal/federation/federation_registry_constants.hpp"

#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <utility>
#include <vector>

namespace umbra::detail {

namespace {

constexpr char kReportObjectClassPublicationInteractionClassName[] =
    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectClassPublication";

constexpr char kReportInteractionPublicationInteractionClassName[] =
    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportInteractionPublication";

constexpr char kReportDirectedInteractionPublicationInteractionClassName[] =
    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportDirectedInteractionPublication";

constexpr std::uint64_t kMomPublicationsEndpointRegionHandle =
    std::numeric_limits<std::uint64_t>::max() - 13U;

}  // namespace

MomPublicationsReportPlan EmbeddedFederationRegistry::planMomPublicationsReport(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId) const {
  auto instrumentationScope = beginInstrumentation("planMomPublicationsReport");
  std::scoped_lock lock(mutex_);
  MomPublicationsReportPlan result;
  result.reportedFederateId = requestingFederateId;
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status = MomPublicationsReportStatus::federation_does_not_exist;
    return result;
  }
  if (!federation->second.members.contains(requestingFederateId)) {
    result.status = MomPublicationsReportStatus::requesting_federate_not_member;
    return result;
  }
  if (!federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles ||
      !federation->second.interactionClassHandles ||
      !federation->second.parameterHandles ||
      !federation->second.dimensionHandles) {
    result.status = MomPublicationsReportStatus::inconsistent_catalog;
    return result;
  }

  auto const objectReportClassHandle = federation->second.interactionClassHandles->handleFor(
      kReportObjectClassPublicationInteractionClassName);
  auto const interactionReportClassHandle = federation->second.interactionClassHandles->handleFor(
      kReportInteractionPublicationInteractionClassName);
  auto const directedReportClassHandle = federation->second.interactionClassHandles->handleFor(
      kReportDirectedInteractionPublicationInteractionClassName);
  auto const federateDimensionHandle = federation->second.dimensionHandles->handleFor(
      kHlaFederateDimensionName);
  auto const objectReportNumberOfClasses = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportObjectClassPublicationInteractionClassName,
      kReportNumberOfClassesParameterName);
  auto const objectReportObjectClass = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportObjectClassPublicationInteractionClassName,
      kReportObjectClassParameterName);
  auto const objectReportAttributeList = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportObjectClassPublicationInteractionClassName,
      kReportAttributeListParameterName);
  auto const interactionReportClassList = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportInteractionPublicationInteractionClassName,
      kReportInteractionClassListParameterName);
  auto const directedReportNumberOfClasses = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportDirectedInteractionPublicationInteractionClassName,
      kReportNumberOfClassesParameterName);
  auto const directedReportObjectClass = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportDirectedInteractionPublicationInteractionClassName,
      kReportObjectClassParameterName);
  auto const directedReportClassList = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportDirectedInteractionPublicationInteractionClassName,
      kReportInteractionClassListParameterName);
  if (!objectReportClassHandle || !interactionReportClassHandle ||
      !directedReportClassHandle || !federateDimensionHandle ||
      !objectReportNumberOfClasses || !objectReportObjectClass ||
      !objectReportAttributeList || !interactionReportClassList ||
      !directedReportNumberOfClasses || !directedReportObjectClass ||
      !directedReportClassList) {
    result.status = MomPublicationsReportStatus::inconsistent_catalog;
    return result;
  }

  auto const objectDeclarations = federation->second.objectClassAttributeDeclarations.find(
      requestingFederateId);
  if (objectDeclarations != federation->second.objectClassAttributeDeclarations.end()) {
    for (auto const& [objectClassHandle, declaration] : objectDeclarations->second.byObjectClass) {
      static_cast<void>(declaration);
      auto const attributes = publishedObjectClassAttributes(
          federation->second, requestingFederateId, objectClassHandle);
      if (!attributes) {
        result.status = MomPublicationsReportStatus::inconsistent_catalog;
        return result;
      }
      if (!attributes->empty()) {
        result.objectClassAttributesByClass.emplace(objectClassHandle, *attributes);
      }
    }
  }

  auto const interactionDeclarations = federation->second.interactionDeclarations.find(
      requestingFederateId);
  if (interactionDeclarations != federation->second.interactionDeclarations.end()) {
    result.interactionClassHandles =
        interactionDeclarations->second.publishedInteractionClasses;
    result.directedInteractionClassesByObjectClass =
        interactionDeclarations->second.publishedObjectClassDirectedInteractions;
  }

  auto const normalizedFederate = normalizedHandleValue(
      federation->second.normalizationSeed,
      requestingFederateId,
      kFederateNormalizationKind);
  RegionSpecificationSnapshot const endpointRegion{
      {*federateDimensionHandle},
      {{*federateDimensionHandle, {normalizedFederate, normalizedFederate + 1U}}},
      true,
  };
  result.objectClassRouting = {
      *objectReportClassHandle,
      *objectReportNumberOfClasses,
      *objectReportObjectClass,
      *objectReportAttributeList,
      kMomPublicationsEndpointRegionHandle,
      endpointRegion,
  };
  result.interactionRouting = {
      *interactionReportClassHandle,
      *interactionReportClassList,
      kMomPublicationsEndpointRegionHandle,
      endpointRegion,
  };
  result.directedRouting = {
      *directedReportClassHandle,
      *directedReportNumberOfClasses,
      *directedReportObjectClass,
      *directedReportClassList,
      kMomPublicationsEndpointRegionHandle,
      endpointRegion,
  };
  std::map<std::uint64_t, RegionSpecificationSnapshot> const endpointOverride{
      {kMomPublicationsEndpointRegionHandle, endpointRegion},
  };
  std::set<std::uint64_t> const endpointRegionHandles{
      kMomPublicationsEndpointRegionHandle,
  };
  auto const objectReportSentParameters = std::vector<std::uint64_t>{
      *objectReportNumberOfClasses,
      *objectReportObjectClass,
      *objectReportAttributeList,
  };
  auto const interactionReportSentParameters = std::vector<std::uint64_t>{
      *interactionReportClassList,
  };
  auto const directedReportSentParameters = std::vector<std::uint64_t>{
      *directedReportNumberOfClasses,
      *directedReportObjectClass,
      *directedReportClassList,
  };
  for (auto const& [federateId, membership] : federation->second.members) {
    static_cast<void>(membership);
    auto objectRecipient = candidateReceiveOrderInteractionRecipient(
        federation->second,
        InteractionProducer::rti(),
        federateId,
        *objectReportClassHandle,
        objectReportSentParameters,
        &endpointRegionHandles,
        &endpointOverride);
    if (objectRecipient) {
      result.objectClassRecipients.push_back(std::move(*objectRecipient));
    }
    auto interactionRecipient = candidateReceiveOrderInteractionRecipient(
        federation->second,
        InteractionProducer::rti(),
        federateId,
        *interactionReportClassHandle,
        interactionReportSentParameters,
        &endpointRegionHandles,
        &endpointOverride);
    if (interactionRecipient) {
      result.interactionRecipients.push_back(std::move(*interactionRecipient));
    }
    auto directedRecipient = candidateReceiveOrderInteractionRecipient(
        federation->second,
        InteractionProducer::rti(),
        federateId,
        *directedReportClassHandle,
        directedReportSentParameters,
        &endpointRegionHandles,
        &endpointOverride);
    if (directedRecipient) {
      result.directedRecipients.push_back(std::move(*directedRecipient));
    }
  }
  return result;
}

std::optional<ReceiveOrderInteractionRecipient>
EmbeddedFederationRegistry::momObjectClassPublicationReportRecipientFor(
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
      kReportObjectClassPublicationInteractionClassName);
  auto const federateDimensionHandle = federation->second.dimensionHandles->handleFor(
      kHlaFederateDimensionName);
  auto const numberOfClassesParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportObjectClassPublicationInteractionClassName,
      kReportNumberOfClassesParameterName);
  auto const objectClassParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportObjectClassPublicationInteractionClassName,
      kReportObjectClassParameterName);
  auto const attributeListParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportObjectClassPublicationInteractionClassName,
      kReportAttributeListParameterName);
  if (!reportClassHandle || !federateDimensionHandle ||
      !numberOfClassesParameterHandle || !objectClassParameterHandle ||
      !attributeListParameterHandle) {
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
      {kMomPublicationsEndpointRegionHandle, endpointRegion},
  };
  std::set<std::uint64_t> const endpointRegionHandles{
      kMomPublicationsEndpointRegionHandle,
  };
  return candidateReceiveOrderInteractionRecipient(
      federation->second,
      InteractionProducer::rti(),
      receivingFederateId,
      *reportClassHandle,
      {*numberOfClassesParameterHandle,
       *objectClassParameterHandle,
       *attributeListParameterHandle},
      &endpointRegionHandles,
      &endpointOverride);
}

std::optional<ReceiveOrderInteractionRecipient>
EmbeddedFederationRegistry::momInteractionPublicationReportRecipientFor(
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
      kReportInteractionPublicationInteractionClassName);
  auto const federateDimensionHandle = federation->second.dimensionHandles->handleFor(
      kHlaFederateDimensionName);
  auto const interactionClassListParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportInteractionPublicationInteractionClassName,
      kReportInteractionClassListParameterName);
  if (!reportClassHandle || !federateDimensionHandle ||
      !interactionClassListParameterHandle) {
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
      {kMomPublicationsEndpointRegionHandle, endpointRegion},
  };
  std::set<std::uint64_t> const endpointRegionHandles{
      kMomPublicationsEndpointRegionHandle,
  };
  return candidateReceiveOrderInteractionRecipient(
      federation->second,
      InteractionProducer::rti(),
      receivingFederateId,
      *reportClassHandle,
      {*interactionClassListParameterHandle},
      &endpointRegionHandles,
      &endpointOverride);
}

std::optional<ReceiveOrderInteractionRecipient>
EmbeddedFederationRegistry::momDirectedInteractionPublicationReportRecipientFor(
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
      kReportDirectedInteractionPublicationInteractionClassName);
  auto const federateDimensionHandle = federation->second.dimensionHandles->handleFor(
      kHlaFederateDimensionName);
  auto const numberOfClassesParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportDirectedInteractionPublicationInteractionClassName,
      kReportNumberOfClassesParameterName);
  auto const objectClassParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportDirectedInteractionPublicationInteractionClassName,
      kReportObjectClassParameterName);
  auto const interactionClassListParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportDirectedInteractionPublicationInteractionClassName,
      kReportInteractionClassListParameterName);
  if (!reportClassHandle || !federateDimensionHandle ||
      !numberOfClassesParameterHandle || !objectClassParameterHandle ||
      !interactionClassListParameterHandle) {
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
      {kMomPublicationsEndpointRegionHandle, endpointRegion},
  };
  std::set<std::uint64_t> const endpointRegionHandles{
      kMomPublicationsEndpointRegionHandle,
  };
  return candidateReceiveOrderInteractionRecipient(
      federation->second,
      InteractionProducer::rti(),
      receivingFederateId,
      *reportClassHandle,
      {*numberOfClassesParameterHandle,
       *objectClassParameterHandle,
       *interactionClassListParameterHandle},
      &endpointRegionHandles,
      &endpointOverride);
}

}  // namespace umbra::detail
