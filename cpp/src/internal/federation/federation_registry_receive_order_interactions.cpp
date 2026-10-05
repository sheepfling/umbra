#include "internal/federation/federation_registry.hpp"
#include "internal/fom/fom_catalog.hpp"

#include <algorithm>
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace umbra::detail {

ReceiveOrderInteractionPlan EmbeddedFederationRegistry::planReceiveOrderInteraction(
    std::wstring const& federationName,
    std::uint64_t producingFederateId,
    std::uint64_t sentInteractionClassHandle,
    std::vector<std::uint64_t> const& sentParameterHandles,
    std::set<std::uint64_t> const* sentRegionHandles) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {ReceiveOrderInteractionStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(producingFederateId)) {
    return {ReceiveOrderInteractionStatus::producing_federate_not_member};
  }
  if (!validInteractionClass(federation->second, sentInteractionClassHandle)) {
    return {ReceiveOrderInteractionStatus::interaction_class_not_defined};
  }
  if (!federation->second.definition.catalog ||
      !federation->second.interactionClassHandles ||
      !federation->second.parameterHandles) {
    return {ReceiveOrderInteractionStatus::inconsistent_catalog};
  }

  auto const sentClassName = federation->second.interactionClassHandles->nameFor(
      sentInteractionClassHandle);
  auto const* sentClass = sentClassName
      ? federation->second.definition.catalog->interactionClass(*sentClassName)
      : nullptr;
  if (sentClass == nullptr || sentClass->transportation.empty()) {
    return {ReceiveOrderInteractionStatus::inconsistent_catalog};
  }

  auto const declarations = federation->second.interactionDeclarations.find(producingFederateId);
  if (declarations == federation->second.interactionDeclarations.end() ||
      !declarations->second.publishedInteractionClasses.contains(sentInteractionClassHandle)) {
    return {ReceiveOrderInteractionStatus::interaction_class_not_published};
  }

  std::set<std::uint64_t> uniqueParameterHandles;
  for (std::uint64_t const parameterHandle : sentParameterHandles) {
    if (!uniqueParameterHandles.insert(parameterHandle).second ||
        !federation->second.parameterHandles->nameFor(
            federation->second.definition.catalog.get(),
            *sentClassName,
            parameterHandle)) {
      return {ReceiveOrderInteractionStatus::interaction_parameter_not_defined};
    }
  }

  ReceiveOrderInteractionPlan result;
  auto const effectiveTransportation = effectiveInteractionTransportationName(
      federation->second,
      producingFederateId,
      sentInteractionClassHandle);
  if (!effectiveTransportation) {
    return {ReceiveOrderInteractionStatus::inconsistent_catalog};
  }
  result.transportationName = *effectiveTransportation;
  auto const preferredOrderType = interactionOrderType(
      federation->second,
      producingFederateId,
      sentInteractionClassHandle);
  if (!preferredOrderType) {
    return {ReceiveOrderInteractionStatus::inconsistent_catalog};
  }
  result.preferredOrderType = *preferredOrderType;
  auto const availableDimensions = availableInteractionDimensions(
      federation->second,
      sentInteractionClassHandle);
  if (!availableDimensions) {
    return {ReceiveOrderInteractionStatus::inconsistent_catalog};
  }
  result.defaultRegionUsed = sentRegionHandles == nullptr && !availableDimensions->empty();
  if (sentRegionHandles != nullptr) {
    if (sentRegionHandles->empty()) {
      // §9.12 explicitly accepts an empty set as a no-send operation, not as
      // an invocation of the ordinary Send Interaction service.
      return result;
    }
    for (std::uint64_t const regionHandle : *sentRegionHandles) {
      auto const region = federation->second.regions.find(regionHandle);
      if (region == federation->second.regions.end()) {
        return {ReceiveOrderInteractionStatus::invalid_region};
      }
      if (region->second.ownerFederateId != producingFederateId) {
        return {ReceiveOrderInteractionStatus::region_not_created_by_this_federate};
      }
      if (!region->second.specificationCommitted ||
          region->second.committedRangeBounds.size() != region->second.dimensionHandles.size()) {
        return {ReceiveOrderInteractionStatus::invalid_region};
      }
      if (!std::includes(
              availableDimensions->begin(),
              availableDimensions->end(),
              region->second.dimensionHandles.begin(),
              region->second.dimensionHandles.end())) {
        return {ReceiveOrderInteractionStatus::invalid_region_context};
      }
      result.sentRegionSnapshots.emplace(
          regionHandle,
          RegionSpecificationSnapshot{
              region->second.dimensionHandles,
              region->second.committedRangeBounds,
              region->second.specificationCommitted});
    }
  }
  // 8.1.8 makes subscription evaluation a federation-wide, creation-time
  // policy. Retain every joined non-source recipient with a live callback
  // route when delayed evaluation is enabled, including explicit regional
  // sends. The route re-evaluates the full subscription and region-overlap
  // projection at its actual receive-order or TSO delivery boundary, so a
  // later declaration can make the message deliverable.
  bool const delaySubscriptionEvaluation =
      federation->second.delaySubscriptionEvaluationSwitch;
  for (auto const& [federateId, membership] : federation->second.members) {
    static_cast<void>(membership);
    auto recipient = candidateReceiveOrderInteractionRecipient(
        federation->second,
        InteractionProducer::joinedFederate(producingFederateId),
        federateId,
        sentInteractionClassHandle,
        sentParameterHandles,
        sentRegionHandles);
    if (recipient) {
      result.recipients.push_back(std::move(*recipient));
      continue;
    }
    if (!delaySubscriptionEvaluation || federateId == producingFederateId) {
      continue;
    }
    auto const callbackRoute = federation->second.interactionCallbackRoutes.find(federateId);
    if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
        !callbackRoute->second) {
      continue;
    }

    // The received class and parameter projection intentionally remain unset:
    // they are determined against the recipient's current subscriptions when
    // the queued callback becomes eligible for delivery.
    ReceiveOrderInteractionRecipient deferredRecipient;
    deferredRecipient.federateId = federateId;
    deferredRecipient.callbackRoute = callbackRoute->second;
    result.recipients.push_back(std::move(deferredRecipient));
  }
  return result;
}

std::optional<ReceiveOrderInteractionRecipient>
EmbeddedFederationRegistry::receiveOrderInteractionRecipientFor(
    std::wstring const& federationName,
    std::uint64_t producingFederateId,
    std::uint64_t receivingFederateId,
    std::uint64_t sentInteractionClassHandle,
    std::vector<std::uint64_t> const& sentParameterHandles,
    std::set<std::uint64_t> const* sentRegionHandles,
    std::map<std::uint64_t, RegionSpecificationSnapshot> const* regionOverrides) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }
  return candidateReceiveOrderInteractionRecipient(
      federation->second,
      InteractionProducer::joinedFederate(producingFederateId),
      receivingFederateId,
      sentInteractionClassHandle,
      sentParameterHandles,
      sentRegionHandles,
      regionOverrides);
}


}  // namespace umbra::detail
