#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_federation_service_helpers.hpp"

#include <iterator>
#include <optional>
#include <set>
#include <utility>

namespace umbra::detail {

TransportServiceMessage ProcessFederationService::handleInteractionClassRegionalSubscription(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const subscriptionRequest =
      decodeProcessFederationInteractionClassRegionalSubscriptionRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != subscriptionRequest.federationName ||
        state->second.federateId != subscriptionRequest.federateId ||
        !registry_.memberById(
            subscriptionRequest.federationName,
            subscriptionRequest.federateId)) {
      return rejected(request);
    }
  }

  auto const result =
      request.operation ==
              TransportServiceOperation::subscribe_interaction_class_with_regions
          ? registry_.setInteractionClassRegionalSubscription(
                subscriptionRequest.federationName,
                subscriptionRequest.federateId,
                subscriptionRequest.interactionClassHandle,
                subscriptionRequest.regionHandles,
                subscriptionRequest.active)
          : request.operation ==
                    TransportServiceOperation::unsubscribe_interaction_class_with_regions
                ? registry_.removeInteractionClassRegionalSubscription(
                      subscriptionRequest.federationName,
                      subscriptionRequest.federateId,
                      subscriptionRequest.interactionClassHandle,
                      subscriptionRequest.regionHandles)
                : RegionalInteractionClassDeclarationStatus::inconsistent_catalog;
  if (result != RegionalInteractionClassDeclarationStatus::applied) {
    return rejected(request);
  }
  return responseFor(request, TransportServiceStatus::ok);
}

TransportServiceMessage
ProcessFederationService::handleObjectClassDirectedInteractionDeclaration(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const declarationRequest =
      decodeProcessFederationObjectClassDirectedInteractionDeclarationRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != declarationRequest.federationName ||
        state->second.federateId != declarationRequest.federateId ||
        !registry_.memberById(
            declarationRequest.federationName, declarationRequest.federateId)) {
      return rejected(request);
    }
  }

  std::optional<std::set<std::uint64_t>> interactionClassHandles;
  if (declarationRequest.interactionClassHandles) {
    interactionClassHandles.emplace(
        declarationRequest.interactionClassHandles->begin(),
        declarationRequest.interactionClassHandles->end());
  }

  DirectedInteractionDeclarationStatus result =
      DirectedInteractionDeclarationStatus::inconsistent_catalog;
  switch (request.operation) {
    case TransportServiceOperation::publish_object_class_directed_interactions:
      result = registry_.publishObjectClassDirectedInteractions(
          declarationRequest.federationName,
          declarationRequest.federateId,
          declarationRequest.objectClassHandle,
          interactionClassHandles.value_or(std::set<std::uint64_t>{}));
      break;
    case TransportServiceOperation::unpublish_object_class_directed_interactions:
      result = registry_.unpublishObjectClassDirectedInteractions(
          declarationRequest.federationName,
          declarationRequest.federateId,
          declarationRequest.objectClassHandle,
          interactionClassHandles);
      break;
    case TransportServiceOperation::subscribe_object_class_directed_interactions:
      result = registry_.subscribeObjectClassDirectedInteractions(
          declarationRequest.federationName,
          declarationRequest.federateId,
          declarationRequest.objectClassHandle,
          interactionClassHandles.value_or(std::set<std::uint64_t>{}),
          declarationRequest.universally);
      break;
    case TransportServiceOperation::unsubscribe_object_class_directed_interactions:
      result = registry_.unsubscribeObjectClassDirectedInteractions(
          declarationRequest.federationName,
          declarationRequest.federateId,
          declarationRequest.objectClassHandle,
          interactionClassHandles);
      break;
    default:
      return invalid(request);
  }
  if (result != DirectedInteractionDeclarationStatus::applied) {
    return rejected(request);
  }
  return responseFor(request, TransportServiceStatus::ok);
}

TransportServiceMessage
ProcessFederationService::handleObjectClassAttributeDeclaration(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const declarationRequest =
      decodeProcessFederationObjectClassAttributeDeclarationRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != declarationRequest.federationName ||
        state->second.federateId != declarationRequest.federateId ||
        !registry_.memberById(
            declarationRequest.federationName, declarationRequest.federateId)) {
      return rejected(request);
    }
  }

  std::set<std::uint64_t> attributeHandles(
      declarationRequest.attributeHandles.begin(),
      declarationRequest.attributeHandles.end());
  bool const publishing =
      request.operation == TransportServiceOperation::publish_object_class_attributes;
  if (!publishing &&
      request.operation == TransportServiceOperation::unpublish_object_class &&
      attributeHandles.empty()) {
    // The official whole-class overload is represented by an empty private
    // attribute vector. Resolve the exact current publication set at the
    // process-owned registry so implicit HLAprivilegeToDeleteObject state is
    // removed together with explicit publications.
    auto const published = registry_.publishedObjectClassAttributeHandles(
        declarationRequest.federationName,
        declarationRequest.federateId,
        declarationRequest.objectClassHandle);
    if (!published) {
      return rejected(request);
    }
    attributeHandles = *published;
  }
  auto const result = registry_.setObjectClassAttributePublication(
      declarationRequest.federationName,
      declarationRequest.federateId,
      declarationRequest.objectClassHandle,
      attributeHandles,
      publishing);
  if (result != ObjectClassAttributeDeclarationStatus::applied) {
    return rejected(request);
  }
  return responseFor(request, TransportServiceStatus::ok);
}

TransportServiceMessage
ProcessFederationService::handleObjectClassAttributeSubscription(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const subscriptionRequest =
      decodeProcessFederationObjectClassAttributeSubscriptionRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != subscriptionRequest.federationName ||
        state->second.federateId != subscriptionRequest.federateId ||
        !registry_.memberById(
            subscriptionRequest.federationName,
            subscriptionRequest.federateId)) {
      return rejected(request);
    }
  }

  std::set<std::uint64_t> attributeHandles(
      subscriptionRequest.attributeHandles.begin(),
      subscriptionRequest.attributeHandles.end());
  if (request.operation == TransportServiceOperation::unsubscribe_object_class_attributes &&
      attributeHandles.empty()) {
    auto const declaration = registry_.objectClassAttributeDeclarationFor(
        subscriptionRequest.federationName,
        subscriptionRequest.federateId,
        subscriptionRequest.objectClassHandle);
    if (!declaration) {
      return rejected(request);
    }
    for (auto const& [attributeHandle, active] : declaration->subscribedAttributes) {
      static_cast<void>(active);
      attributeHandles.insert(attributeHandle);
    }
  }

  auto const result = registry_.setObjectClassAttributeSubscriptionWithScopeChanges(
      subscriptionRequest.federationName,
      subscriptionRequest.federateId,
      subscriptionRequest.objectClassHandle,
      attributeHandles,
      request.operation == TransportServiceOperation::subscribe_object_class_attributes
          ? std::optional<bool>{subscriptionRequest.active}
          : std::nullopt,
      subscriptionRequest.updateRateDesignator);
  if (result.status != ObjectClassAttributeDeclarationStatus::applied) {
    return rejected(request);
  }
  if (request.operation ==
      TransportServiceOperation::subscribe_object_class_attributes) {
    // A subscription can make already-registered instances newly visible.
    // Keep that discovery projection on the same process event boundary as
    // registration rather than requiring a fixture to mutate the registry.
    // RTI-owned MOM objects use a separate registry ledger because they are
    // not federate-created instances, but the public subscription still has
    // to expose both projections through the same official discovery route.
    auto discoveries = registry_.planObjectInstanceDiscoveriesForFederate(
        subscriptionRequest.federationName,
        subscriptionRequest.federateId);
    auto momDiscoveries = registry_.planJoinedFederateMomObjectDiscoveriesForFederate(
        subscriptionRequest.federationName,
        subscriptionRequest.federateId);
    discoveries.insert(
        discoveries.end(),
        std::make_move_iterator(momDiscoveries.begin()),
        std::make_move_iterator(momDiscoveries.end()));
    if (!enqueueObjectInstanceDiscoveries(
            subscriptionRequest.federationName,
            std::move(discoveries))) {
      return internalError(request);
    }
  }
  if (!enqueueObjectInstanceScopeChanges(
          subscriptionRequest.federationName, std::move(result.recipients)) ||
      !enqueueAttributeRelevanceAdvisories(
          subscriptionRequest.federationName,
          std::move(result.attributeRelevanceAdvisories))) {
    return internalError(request);
  }
  return responseFor(request, TransportServiceStatus::ok);
}

TransportServiceMessage
ProcessFederationService::handleObjectClassAttributeRegionalSubscription(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const subscriptionRequest =
      decodeProcessFederationObjectClassAttributeRegionalSubscriptionRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != subscriptionRequest.federationName ||
        state->second.federateId != subscriptionRequest.federateId ||
        !registry_.memberById(
            subscriptionRequest.federationName,
            subscriptionRequest.federateId)) {
      return rejected(request);
    }
  }

  RegionalObjectClassAttributeSubscriptionScopePlan result;
  if (request.operation ==
      TransportServiceOperation::subscribe_object_class_attributes_with_regions) {
    result = registry_.setObjectClassAttributeRegionalSubscriptionWithScopeChanges(
        subscriptionRequest.federationName,
        subscriptionRequest.federateId,
        subscriptionRequest.objectClassHandle,
        subscriptionRequest.attributesAndRegions,
        subscriptionRequest.active,
        subscriptionRequest.updateRateDesignator);
  } else if (request.operation ==
             TransportServiceOperation::unsubscribe_object_class_attributes_with_regions) {
    result = registry_.removeObjectClassAttributeRegionalSubscriptionWithScopeChanges(
        subscriptionRequest.federationName,
        subscriptionRequest.federateId,
        subscriptionRequest.objectClassHandle,
        subscriptionRequest.attributesAndRegions);
  } else {
    return invalid(request);
  }
  if (result.status !=
      RegionalObjectClassAttributeDeclarationStatus::applied) {
    return rejected(request);
  }

  if (request.operation ==
      TransportServiceOperation::subscribe_object_class_attributes_with_regions) {
    // A regional subscription can make already-registered instances newly
    // visible. Discovery remains ordered before any scope transition. Include
    // RTI-owned MOM objects here as well; their immutable HLAfederate point is
    // evaluated by the registry's MOM discovery planner.
    auto discoveries = registry_.planObjectInstanceDiscoveriesForFederate(
        subscriptionRequest.federationName,
        subscriptionRequest.federateId);
    auto momDiscoveries = registry_.planJoinedFederateMomObjectDiscoveriesForFederate(
        subscriptionRequest.federationName,
        subscriptionRequest.federateId);
    discoveries.insert(
        discoveries.end(),
        std::make_move_iterator(momDiscoveries.begin()),
        std::make_move_iterator(momDiscoveries.end()));
    if (!enqueueObjectInstanceDiscoveries(
            subscriptionRequest.federationName,
            std::move(discoveries))) {
      return internalError(request);
    }
  }
  if (!enqueueObjectInstanceScopeChanges(
          subscriptionRequest.federationName, std::move(result.recipients))) {
    return internalError(request);
  }
  if (!enqueueAttributeRelevanceAdvisories(
          subscriptionRequest.federationName,
          std::move(result.attributeRelevanceAdvisories))) {
    return internalError(request);
  }
  return responseFor(request, TransportServiceStatus::ok);
}

}  // namespace umbra::detail
