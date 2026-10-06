#include "internal/federation/federation_registry.hpp"
#include "internal/federation/federation_registry_constants.hpp"
#include "internal/federation/federation_registry_value_helpers.hpp"

#include <algorithm>
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

constexpr char kReportObjectClassSubscriptionInteractionClassName[] =
    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectClassSubscription";

constexpr char kReportInteractionSubscriptionInteractionClassName[] =
    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportInteractionSubscription";

constexpr char kReportDirectedInteractionSubscriptionInteractionClassName[] =
    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportDirectedInteractionSubscription";

constexpr char kReportActiveParameterName[] = "HLAactive";

constexpr char kReportMaxUpdateRateParameterName[] = "HLAmaxUpdateRate";

constexpr char kReportUniversalParameterName[] = "HLAuniversal";

constexpr std::uint64_t kMomSubscriptionsEndpointRegionHandle =
    std::numeric_limits<std::uint64_t>::max() - 14U;

}  // namespace

MomSubscriptionsReportPlan EmbeddedFederationRegistry::planMomSubscriptionsReport(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId) const {
  auto instrumentationScope = beginInstrumentation("planMomSubscriptionsReport");
  std::scoped_lock lock(mutex_);
  MomSubscriptionsReportPlan result;
  result.reportedFederateId = requestingFederateId;
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status = MomSubscriptionsReportStatus::federation_does_not_exist;
    return result;
  }
  if (!federation->second.members.contains(requestingFederateId)) {
    result.status = MomSubscriptionsReportStatus::requesting_federate_not_member;
    return result;
  }
  if (!federation->second.definition.catalog ||
      !federation->second.interactionClassHandles ||
      !federation->second.parameterHandles ||
      !federation->second.dimensionHandles) {
    result.status = MomSubscriptionsReportStatus::inconsistent_catalog;
    return result;
  }

  auto const catalog = federation->second.definition.catalog.get();
  auto const objectReportClassHandle = federation->second.interactionClassHandles->handleFor(
      kReportObjectClassSubscriptionInteractionClassName);
  auto const interactionReportClassHandle = federation->second.interactionClassHandles->handleFor(
      kReportInteractionSubscriptionInteractionClassName);
  auto const directedReportClassHandle = federation->second.interactionClassHandles->handleFor(
      kReportDirectedInteractionSubscriptionInteractionClassName);
  auto const federateDimensionHandle = federation->second.dimensionHandles->handleFor(
      kHlaFederateDimensionName);
  auto const objectReportNumberOfClasses = federation->second.parameterHandles->handleFor(
      catalog, kReportObjectClassSubscriptionInteractionClassName,
      kReportNumberOfClassesParameterName);
  auto const objectReportObjectClass = federation->second.parameterHandles->handleFor(
      catalog, kReportObjectClassSubscriptionInteractionClassName,
      kReportObjectClassParameterName);
  auto const objectReportActive = federation->second.parameterHandles->handleFor(
      catalog, kReportObjectClassSubscriptionInteractionClassName,
      kReportActiveParameterName);
  auto const objectReportMaxUpdateRate = federation->second.parameterHandles->handleFor(
      catalog, kReportObjectClassSubscriptionInteractionClassName,
      kReportMaxUpdateRateParameterName);
  auto const objectReportAttributeList = federation->second.parameterHandles->handleFor(
      catalog, kReportObjectClassSubscriptionInteractionClassName,
      kReportAttributeListParameterName);
  auto const interactionReportClassList = federation->second.parameterHandles->handleFor(
      catalog, kReportInteractionSubscriptionInteractionClassName,
      kReportInteractionClassListParameterName);
  auto const directedReportNumberOfClasses = federation->second.parameterHandles->handleFor(
      catalog, kReportDirectedInteractionSubscriptionInteractionClassName,
      kReportNumberOfClassesParameterName);
  auto const directedReportObjectClass = federation->second.parameterHandles->handleFor(
      catalog, kReportDirectedInteractionSubscriptionInteractionClassName,
      kReportObjectClassParameterName);
  auto const directedReportUniversal = federation->second.parameterHandles->handleFor(
      catalog, kReportDirectedInteractionSubscriptionInteractionClassName,
      kReportUniversalParameterName);
  auto const directedReportClassList = federation->second.parameterHandles->handleFor(
      catalog, kReportDirectedInteractionSubscriptionInteractionClassName,
      kReportInteractionClassListParameterName);
  if (!objectReportClassHandle || !interactionReportClassHandle ||
      !directedReportClassHandle || !federateDimensionHandle ||
      !objectReportNumberOfClasses || !objectReportObjectClass ||
      !objectReportActive || !objectReportMaxUpdateRate ||
      !objectReportAttributeList || !interactionReportClassList ||
      !directedReportNumberOfClasses || !directedReportObjectClass ||
      !directedReportClassList) {
    result.status = MomSubscriptionsReportStatus::inconsistent_catalog;
    return result;
  }

  auto const updateSnapshot = [&](std::uint64_t objectClassHandle,
                                  bool active,
                                  std::uint64_t attributeHandle,
                                  std::string const& suppliedRate) {
    auto& snapshot = result.objectClassSubscriptions[objectClassHandle][active];
    snapshot.objectClassHandle = objectClassHandle;
    snapshot.active = active;
    snapshot.attributeHandles.insert(attributeHandle);
    auto const normalizedRate = suppliedRate.empty()
        ? std::string{"HLAdefault"}
        : suppliedRate;
    auto const rateValue = updateRateValueForNormalizedDesignator(
        *catalog, normalizedRate);
    if (!rateValue) {
      return false;
    }
    if (snapshot.maxUpdateRate.empty()) {
      snapshot.maxUpdateRate = normalizedRate;
      return true;
    }
    auto const currentValue = updateRateValueForNormalizedDesignator(
        *catalog, snapshot.maxUpdateRate);
    if (!currentValue) {
      return false;
    }
    if (*rateValue > *currentValue ||
        (*rateValue == *currentValue && normalizedRate < snapshot.maxUpdateRate)) {
      snapshot.maxUpdateRate = normalizedRate;
    }
    return true;
  };

  auto const objectDeclarations = federation->second.objectClassAttributeDeclarations.find(
      requestingFederateId);
  if (objectDeclarations != federation->second.objectClassAttributeDeclarations.end()) {
    for (auto const& [objectClassHandle, declaration] : objectDeclarations->second.byObjectClass) {
      for (auto const& [attributeHandle, active] : declaration.subscribedAttributes) {
        auto const rate = declaration.subscribedUpdateRateDesignators.find(attributeHandle);
        if (!updateSnapshot(
                objectClassHandle,
                active,
                attributeHandle,
                rate == declaration.subscribedUpdateRateDesignators.end()
                    ? std::string{}
                    : rate->second)) {
          result.status = MomSubscriptionsReportStatus::inconsistent_catalog;
          return result;
        }
      }
      for (auto const& [attributeHandle, regions] : declaration.regionalSubscribedAttributes) {
        auto const rates = declaration.regionalSubscribedUpdateRateDesignators.find(attributeHandle);
        for (auto const& [regionHandle, active] : regions) {
          auto const rate = rates == declaration.regionalSubscribedUpdateRateDesignators.end()
              ? std::map<std::uint64_t, std::string>::const_iterator{}
              : rates->second.find(regionHandle);
          if (!updateSnapshot(
                  objectClassHandle,
                  active,
                  attributeHandle,
                  rates == declaration.regionalSubscribedUpdateRateDesignators.end() ||
                          rate == rates->second.end()
                      ? std::string{}
                      : rate->second)) {
            result.status = MomSubscriptionsReportStatus::inconsistent_catalog;
            return result;
          }
        }
      }
    }
  }

  auto const interactionDeclarations = federation->second.interactionDeclarations.find(
      requestingFederateId);
  if (interactionDeclarations != federation->second.interactionDeclarations.end()) {
    for (auto const& [interactionClassHandle, active] :
         interactionDeclarations->second.subscribedInteractionClasses) {
      result.interactionSubscriptionActives[interactionClassHandle].insert(active);
    }
    for (auto const& [interactionClassHandle, regions] :
         interactionDeclarations->second.regionalSubscribedInteractionClasses) {
      for (auto const& [regionHandle, active] : regions) {
        static_cast<void>(regionHandle);
        result.interactionSubscriptionActives[interactionClassHandle].insert(active);
      }
    }
    for (auto const& [objectClassHandle, interactions] :
         interactionDeclarations->second.subscribedObjectClassDirectedInteractions) {
      for (auto const& [interactionClassHandle, universal] : interactions) {
        result.directedInteractionClassesByObjectClassAndUniversal[objectClassHandle][universal]
            .insert(interactionClassHandle);
      }
    }
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
      *objectReportActive,
      *objectReportMaxUpdateRate,
      *objectReportAttributeList,
      kMomSubscriptionsEndpointRegionHandle,
      endpointRegion,
  };
  result.interactionRouting = {
      *interactionReportClassHandle,
      *interactionReportClassList,
      kMomSubscriptionsEndpointRegionHandle,
      endpointRegion,
  };
  result.directedRouting = {
      *directedReportClassHandle,
      *directedReportNumberOfClasses,
      *directedReportObjectClass,
      directedReportUniversal.value_or(0U),
      *directedReportClassList,
      kMomSubscriptionsEndpointRegionHandle,
      endpointRegion,
  };
  std::map<std::uint64_t, RegionSpecificationSnapshot> const endpointOverride{
      {kMomSubscriptionsEndpointRegionHandle, endpointRegion},
  };
  std::set<std::uint64_t> const endpointRegionHandles{
      kMomSubscriptionsEndpointRegionHandle,
  };
  auto const objectReportSentParameters = std::vector<std::uint64_t>{
      *objectReportNumberOfClasses,
      *objectReportObjectClass,
      *objectReportActive,
      *objectReportMaxUpdateRate,
      *objectReportAttributeList,
  };
  auto const interactionReportSentParameters = std::vector<std::uint64_t>{
      *interactionReportClassList,
  };
  auto directedReportSentParameters = std::vector<std::uint64_t>{
      *directedReportNumberOfClasses,
      *directedReportObjectClass,
      *directedReportClassList,
  };
  if (directedReportUniversal) {
    directedReportSentParameters.insert(
        directedReportSentParameters.begin() + 2,
        *directedReportUniversal);
  }
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
EmbeddedFederationRegistry::momObjectClassSubscriptionReportRecipientFor(
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
      kReportObjectClassSubscriptionInteractionClassName);
  auto const federateDimensionHandle = federation->second.dimensionHandles->handleFor(
      kHlaFederateDimensionName);
  auto const numberOfClassesParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportObjectClassSubscriptionInteractionClassName,
      kReportNumberOfClassesParameterName);
  auto const objectClassParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportObjectClassSubscriptionInteractionClassName,
      kReportObjectClassParameterName);
  auto const activeParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportObjectClassSubscriptionInteractionClassName,
      kReportActiveParameterName);
  auto const maxUpdateRateParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportObjectClassSubscriptionInteractionClassName,
      kReportMaxUpdateRateParameterName);
  auto const attributeListParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportObjectClassSubscriptionInteractionClassName,
      kReportAttributeListParameterName);
  if (!reportClassHandle || !federateDimensionHandle ||
      !numberOfClassesParameterHandle || !objectClassParameterHandle ||
      !activeParameterHandle || !maxUpdateRateParameterHandle ||
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
      {kMomSubscriptionsEndpointRegionHandle, endpointRegion},
  };
  std::set<std::uint64_t> const endpointRegionHandles{
      kMomSubscriptionsEndpointRegionHandle,
  };
  return candidateReceiveOrderInteractionRecipient(
      federation->second,
      InteractionProducer::rti(),
      receivingFederateId,
      *reportClassHandle,
      {*numberOfClassesParameterHandle,
       *objectClassParameterHandle,
       *activeParameterHandle,
       *maxUpdateRateParameterHandle,
       *attributeListParameterHandle},
      &endpointRegionHandles,
      &endpointOverride);
}

std::optional<ReceiveOrderInteractionRecipient>
EmbeddedFederationRegistry::momInteractionSubscriptionReportRecipientFor(
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
      kReportInteractionSubscriptionInteractionClassName);
  auto const federateDimensionHandle = federation->second.dimensionHandles->handleFor(
      kHlaFederateDimensionName);
  auto const interactionClassListParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportInteractionSubscriptionInteractionClassName,
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
      {kMomSubscriptionsEndpointRegionHandle, endpointRegion},
  };
  std::set<std::uint64_t> const endpointRegionHandles{
      kMomSubscriptionsEndpointRegionHandle,
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
EmbeddedFederationRegistry::momDirectedInteractionSubscriptionReportRecipientFor(
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
      kReportDirectedInteractionSubscriptionInteractionClassName);
  auto const federateDimensionHandle = federation->second.dimensionHandles->handleFor(
      kHlaFederateDimensionName);
  auto const numberOfClassesParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportDirectedInteractionSubscriptionInteractionClassName,
      kReportNumberOfClassesParameterName);
  auto const objectClassParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportDirectedInteractionSubscriptionInteractionClassName,
      kReportObjectClassParameterName);
  auto const universalParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportDirectedInteractionSubscriptionInteractionClassName,
      kReportUniversalParameterName);
  auto const interactionClassListParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kReportDirectedInteractionSubscriptionInteractionClassName,
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
      {kMomSubscriptionsEndpointRegionHandle, endpointRegion},
  };
  std::set<std::uint64_t> const endpointRegionHandles{
      kMomSubscriptionsEndpointRegionHandle,
  };
  std::vector<std::uint64_t> sentParameters{
      *numberOfClassesParameterHandle,
      *objectClassParameterHandle,
      *interactionClassListParameterHandle,
  };
  if (universalParameterHandle) {
    sentParameters.insert(sentParameters.begin() + 2, *universalParameterHandle);
  }
  return candidateReceiveOrderInteractionRecipient(
      federation->second,
      InteractionProducer::rti(),
      receivingFederateId,
      *reportClassHandle,
      sentParameters,
      &endpointRegionHandles,
      &endpointOverride);
}

}  // namespace umbra::detail
