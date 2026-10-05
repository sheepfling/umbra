#include "internal/federation/federation_registry.hpp"
#include "internal/fom/fom_catalog.hpp"

#include <algorithm>
#include <cstdint>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace umbra::detail {
bool EmbeddedFederationRegistry::validDirectedInteractionForObjectClass(
    Federation const& federation,
    std::uint64_t objectClassHandle,
    std::uint64_t interactionClassHandle) {
  if (!validObjectClass(federation, objectClassHandle) ||
      !validInteractionClass(federation, interactionClassHandle) ||
      !federation.definition.catalog || !federation.objectClassHandles ||
      !federation.interactionClassHandles) {
    return false;
  }

  auto const objectClassName = federation.objectClassHandles->nameFor(objectClassHandle);
  auto const interactionClassName =
      federation.interactionClassHandles->nameFor(interactionClassHandle);
  if (!objectClassName || !interactionClassName) {
    return false;
  }

  std::set<std::string> visited;
  std::string currentClassName = *objectClassName;
  while (!currentClassName.empty() && visited.insert(currentClassName).second) {
    auto const* objectClass = federation.definition.catalog->objectClass(currentClassName);
    if (objectClass == nullptr) {
      return false;
    }
    if (objectClass->directedInteraction(*interactionClassName) != nullptr) {
      return true;
    }
    currentClassName = objectClass->parentName;
  }
  return false;
}

bool EmbeddedFederationRegistry::directedInteractionDeclarationApplies(
    Federation const& federation,
    std::uint64_t federateId,
    std::uint64_t registeredObjectClassHandle,
    std::uint64_t interactionClassHandle,
    bool publication) {
  if (!federation.objectClassHandles || !federation.definition.catalog) {
    return false;
  }
  auto const declarations = federation.interactionDeclarations.find(federateId);
  if (declarations == federation.interactionDeclarations.end()) {
    return false;
  }
  auto const registeredClassName =
      federation.objectClassHandles->nameFor(registeredObjectClassHandle);
  if (!registeredClassName) {
    return false;
  }
  std::set<std::string> visited;
  std::string currentClassName = *registeredClassName;
  while (!currentClassName.empty() && visited.insert(currentClassName).second) {
    auto const currentClassHandle = federation.objectClassHandles->handleFor(currentClassName);
    if (!currentClassHandle) {
      return false;
    }
    if (publication) {
      auto const declaration = declarations->second.publishedObjectClassDirectedInteractions.find(
          *currentClassHandle);
      if (declaration != declarations->second.publishedObjectClassDirectedInteractions.end() &&
          declaration->second.contains(interactionClassHandle)) {
        return true;
      }
    } else {
      auto const declaration = declarations->second.subscribedObjectClassDirectedInteractions.find(
          *currentClassHandle);
      if (declaration != declarations->second.subscribedObjectClassDirectedInteractions.end() &&
          declaration->second.contains(interactionClassHandle)) {
        return true;
      }
    }
    auto const* objectClass = federation.definition.catalog->objectClass(currentClassName);
    if (objectClass == nullptr) {
      return false;
    }
    currentClassName = objectClass->parentName;
  }
  return false;
}

std::optional<bool> EmbeddedFederationRegistry::directedInteractionSubscriptionIsUniversal(
    Federation const& federation,
    std::uint64_t federateId,
    std::uint64_t registeredObjectClassHandle,
    std::uint64_t interactionClassHandle) {
  if (!federation.objectClassHandles || !federation.definition.catalog) {
    return std::nullopt;
  }
  auto const declarations = federation.interactionDeclarations.find(federateId);
  if (declarations == federation.interactionDeclarations.end()) {
    return std::nullopt;
  }

  auto const registeredClassName =
      federation.objectClassHandles->nameFor(registeredObjectClassHandle);
  if (!registeredClassName) {
    return std::nullopt;
  }
  std::set<std::string> visited;
  std::string currentClassName = *registeredClassName;
  while (!currentClassName.empty() && visited.insert(currentClassName).second) {
    auto const currentClassHandle = federation.objectClassHandles->handleFor(currentClassName);
    if (!currentClassHandle) {
      return std::nullopt;
    }
    auto const perObjectClass =
        declarations->second.subscribedObjectClassDirectedInteractions.find(*currentClassHandle);
    if (perObjectClass != declarations->second.subscribedObjectClassDirectedInteractions.end()) {
      auto const subscription = perObjectClass->second.find(interactionClassHandle);
      if (subscription != perObjectClass->second.end()) {
        return subscription->second;
      }
    }
    auto const* objectClass = federation.definition.catalog->objectClass(currentClassName);
    if (objectClass == nullptr) {
      return std::nullopt;
    }
    currentClassName = objectClass->parentName;
  }
  return std::nullopt;
}

std::optional<ReceiveOrderDirectedInteractionRecipient>
EmbeddedFederationRegistry::candidateReceiveOrderDirectedInteractionRecipient(
    Federation const& federation,
    std::uint64_t producingFederateId,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t sentInteractionClassHandle,
    std::vector<std::uint64_t> const& sentParameterHandles,
    bool allowMissingProducingFederate) {
  bool const producingFederateIsJoined = federation.members.contains(producingFederateId);
  bool const retainDepartedProducerAcceptance =
      allowMissingProducingFederate && !producingFederateIsJoined;
  if (producingFederateId == receivingFederateId ||
      !federation.members.contains(receivingFederateId) ||
      (!producingFederateIsJoined && !retainDepartedProducerAcceptance) ||
      !federation.definition.catalog || !federation.objectClassHandles ||
      !federation.interactionClassHandles || !federation.parameterHandles) {
    return std::nullopt;
  }

  auto const instance = federation.objectInstances.find(objectInstanceHandle);
  // The normal receive-order predicate suppresses a directed interaction
  // whose target has been deleted. For an accepted timestamped payload whose
  // producer has departed, the target must remain valid through its recipient's
  // TSO boundary. The caller can set allowMissingProducingFederate only after
  // it has revalidated that exact message/recipient pair, so this does not
  // relax ordinary deletion or unaccepted source-membership semantics.
  if (instance == federation.objectInstances.end() ||
      (!retainDepartedProducerAcceptance && instance->second.deleteAccepted) ||
      !instance->second.knownObjectClassHandlesByFederate.contains(receivingFederateId) ||
      (!retainDepartedProducerAcceptance &&
       !instance->second.knownObjectClassHandlesByFederate.contains(producingFederateId))) {
    return std::nullopt;
  }
  if (!validDirectedInteractionForObjectClass(
          federation,
          instance->second.registeredObjectClassHandle,
          sentInteractionClassHandle) ||
      (!retainDepartedProducerAcceptance && !directedInteractionDeclarationApplies(
          federation,
          producingFederateId,
          instance->second.registeredObjectClassHandle,
          sentInteractionClassHandle,
          true))) {
    return std::nullopt;
  }
  auto const subscriptionIsUniversal = directedInteractionSubscriptionIsUniversal(
      federation,
      receivingFederateId,
      instance->second.registeredObjectClassHandle,
      sentInteractionClassHandle);
  // The optional carries declaration presence separately from selector kind:
  // engaged(false) is the valid by-ownership form, while disengaged means the
  // receiving federate has no applicable directed subscription.
  if (!subscriptionIsUniversal.has_value()) {
    return std::nullopt;
  }
  bool const ownsTargetAttribute = std::any_of(
      instance->second.attributeOwnersByHandle.begin(),
      instance->second.attributeOwnersByHandle.end(),
      [receivingFederateId](auto const& owner) {
        return owner.second == receivingFederateId;
      });
  if (!*subscriptionIsUniversal && !ownsTargetAttribute) {
    return std::nullopt;
  }

  auto const sentClassName =
      federation.interactionClassHandles->nameFor(sentInteractionClassHandle);
  if (!sentClassName ||
      federation.definition.catalog->interactionClass(*sentClassName) == nullptr) {
    return std::nullopt;
  }

  ReceiveOrderDirectedInteractionRecipient recipient;
  recipient.federateId = receivingFederateId;
  recipient.objectInstanceHandle = objectInstanceHandle;
  recipient.receivedInteractionClassHandle = sentInteractionClassHandle;
  for (std::uint64_t const parameterHandle : sentParameterHandles) {
    if (federation.parameterHandles->nameFor(
            federation.definition.catalog.get(),
            *sentClassName,
            parameterHandle)) {
      recipient.receivedParameterHandles.insert(parameterHandle);
    }
  }
  auto const callbackRoute = federation.interactionCallbackRoutes.find(receivingFederateId);
  if (callbackRoute == federation.interactionCallbackRoutes.end() || !callbackRoute->second) {
    return std::nullopt;
  }
  recipient.callbackRoute = callbackRoute->second;
  return recipient;
}

InteractionClassDeclarationStatus EmbeddedFederationRegistry::setInteractionClassPublication(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t interactionClassHandle,
    bool published) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return InteractionClassDeclarationStatus::federation_does_not_exist;
  }
  if (!federation->second.members.contains(federateId)) {
    return InteractionClassDeclarationStatus::federate_not_member;
  }
  if (!validInteractionClass(federation->second, interactionClassHandle)) {
    return InteractionClassDeclarationStatus::interaction_class_not_defined;
  }
  auto declarations = federation->second.interactionDeclarations.find(federateId);
  if (!published) {
    if (declarations == federation->second.interactionDeclarations.end()) {
      return InteractionClassDeclarationStatus::applied;
    }
    declarations->second.publishedInteractionClasses.erase(interactionClassHandle);
    if (declarations->second.publishedInteractionClasses.empty() &&
        declarations->second.subscribedInteractionClasses.empty() &&
        declarations->second.regionalSubscribedInteractionClasses.empty() &&
        declarations->second.publishedObjectClassDirectedInteractions.empty() &&
        declarations->second.subscribedObjectClassDirectedInteractions.empty()) {
      federation->second.interactionDeclarations.erase(declarations);
    }
    return InteractionClassDeclarationStatus::applied;
  }

  auto [state, insertedState] = federation->second.interactionDeclarations.try_emplace(federateId);
  try {
    static_cast<void>(state->second.publishedInteractionClasses.insert(interactionClassHandle));
  } catch (...) {
    if (insertedState && state->second.publishedInteractionClasses.empty() &&
        state->second.subscribedInteractionClasses.empty() &&
        state->second.regionalSubscribedInteractionClasses.empty() &&
        state->second.publishedObjectClassDirectedInteractions.empty() &&
        state->second.subscribedObjectClassDirectedInteractions.empty()) {
      federation->second.interactionDeclarations.erase(state);
    }
    throw;
  }
  return InteractionClassDeclarationStatus::applied;
}

DirectedInteractionDeclarationStatus
EmbeddedFederationRegistry::publishObjectClassDirectedInteractions(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::set<std::uint64_t> const& interactionClassHandles) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return DirectedInteractionDeclarationStatus::federation_does_not_exist;
  }
  if (!federation->second.members.contains(federateId)) {
    return DirectedInteractionDeclarationStatus::federate_not_member;
  }
  if (!validObjectClass(federation->second, objectClassHandle)) {
    return DirectedInteractionDeclarationStatus::object_class_not_defined;
  }
  for (std::uint64_t const interactionClassHandle : interactionClassHandles) {
    if (!validInteractionClass(federation->second, interactionClassHandle)) {
      return DirectedInteractionDeclarationStatus::interaction_class_not_defined;
    }
    if (!validDirectedInteractionForObjectClass(
            federation->second,
            objectClassHandle,
            interactionClassHandle)) {
      return DirectedInteractionDeclarationStatus::interaction_not_defined_for_object_class;
    }
  }
  if (interactionClassHandles.empty()) {
    return DirectedInteractionDeclarationStatus::applied;
  }

  auto [state, insertedState] = federation->second.interactionDeclarations.try_emplace(federateId);
  try {
    auto revised = state->second.publishedObjectClassDirectedInteractions;
    revised[objectClassHandle].insert(
        interactionClassHandles.begin(),
        interactionClassHandles.end());
    state->second.publishedObjectClassDirectedInteractions.swap(revised);
  } catch (...) {
    if (insertedState && state->second.publishedInteractionClasses.empty() &&
        state->second.subscribedInteractionClasses.empty() &&
        state->second.regionalSubscribedInteractionClasses.empty() &&
        state->second.publishedObjectClassDirectedInteractions.empty() &&
        state->second.subscribedObjectClassDirectedInteractions.empty()) {
      federation->second.interactionDeclarations.erase(state);
    }
    throw;
  }
  return DirectedInteractionDeclarationStatus::applied;
}

DirectedInteractionDeclarationStatus
EmbeddedFederationRegistry::unpublishObjectClassDirectedInteractions(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::optional<std::set<std::uint64_t>> const& interactionClassHandles) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return DirectedInteractionDeclarationStatus::federation_does_not_exist;
  }
  if (!federation->second.members.contains(federateId)) {
    return DirectedInteractionDeclarationStatus::federate_not_member;
  }
  if (!validObjectClass(federation->second, objectClassHandle)) {
    return DirectedInteractionDeclarationStatus::object_class_not_defined;
  }
  if (interactionClassHandles) {
    for (std::uint64_t const interactionClassHandle : *interactionClassHandles) {
      if (!validInteractionClass(federation->second, interactionClassHandle)) {
        return DirectedInteractionDeclarationStatus::interaction_class_not_defined;
      }
      if (!validDirectedInteractionForObjectClass(
              federation->second,
              objectClassHandle,
              interactionClassHandle)) {
        return DirectedInteractionDeclarationStatus::interaction_not_defined_for_object_class;
      }
    }
  }

  auto declarations = federation->second.interactionDeclarations.find(federateId);
  if (declarations != federation->second.interactionDeclarations.end()) {
    auto revised = declarations->second.publishedObjectClassDirectedInteractions;
    if (!interactionClassHandles) {
      revised.erase(objectClassHandle);
    } else {
      auto published = revised.find(objectClassHandle);
      if (published != revised.end()) {
        for (std::uint64_t const interactionClassHandle : *interactionClassHandles) {
          published->second.erase(interactionClassHandle);
        }
        if (published->second.empty()) {
          revised.erase(published);
        }
      }
    }
    declarations->second.publishedObjectClassDirectedInteractions.swap(revised);
    if (declarations->second.publishedInteractionClasses.empty() &&
        declarations->second.subscribedInteractionClasses.empty() &&
        declarations->second.regionalSubscribedInteractionClasses.empty() &&
        declarations->second.publishedObjectClassDirectedInteractions.empty() &&
        declarations->second.subscribedObjectClassDirectedInteractions.empty()) {
      federation->second.interactionDeclarations.erase(declarations);
    }
  }
  return DirectedInteractionDeclarationStatus::applied;
}

DirectedInteractionDeclarationStatus
EmbeddedFederationRegistry::subscribeObjectClassDirectedInteractions(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::set<std::uint64_t> const& interactionClassHandles,
    bool universally) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return DirectedInteractionDeclarationStatus::federation_does_not_exist;
  }
  if (!federation->second.members.contains(federateId)) {
    return DirectedInteractionDeclarationStatus::federate_not_member;
  }
  if (!validObjectClass(federation->second, objectClassHandle)) {
    return DirectedInteractionDeclarationStatus::object_class_not_defined;
  }
  for (std::uint64_t const interactionClassHandle : interactionClassHandles) {
    if (!validInteractionClass(federation->second, interactionClassHandle)) {
      return DirectedInteractionDeclarationStatus::interaction_class_not_defined;
    }
    if (!validDirectedInteractionForObjectClass(
            federation->second,
            objectClassHandle,
            interactionClassHandle)) {
      return DirectedInteractionDeclarationStatus::interaction_not_defined_for_object_class;
    }
  }
  if (interactionClassHandles.empty()) {
    return DirectedInteractionDeclarationStatus::applied;
  }

  auto [state, insertedState] = federation->second.interactionDeclarations.try_emplace(federateId);
  try {
    auto revised = state->second.subscribedObjectClassDirectedInteractions;
    auto& revisedClasses = revised[objectClassHandle];
    for (std::uint64_t const interactionClassHandle : interactionClassHandles) {
      revisedClasses.insert_or_assign(interactionClassHandle, universally);
    }
    state->second.subscribedObjectClassDirectedInteractions.swap(revised);
  } catch (...) {
    if (insertedState && state->second.publishedInteractionClasses.empty() &&
        state->second.subscribedInteractionClasses.empty() &&
        state->second.regionalSubscribedInteractionClasses.empty() &&
        state->second.publishedObjectClassDirectedInteractions.empty() &&
        state->second.subscribedObjectClassDirectedInteractions.empty()) {
      federation->second.interactionDeclarations.erase(state);
    }
    throw;
  }
  return DirectedInteractionDeclarationStatus::applied;
}

DirectedInteractionDeclarationStatus
EmbeddedFederationRegistry::unsubscribeObjectClassDirectedInteractions(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::optional<std::set<std::uint64_t>> const& interactionClassHandles) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return DirectedInteractionDeclarationStatus::federation_does_not_exist;
  }
  if (!federation->second.members.contains(federateId)) {
    return DirectedInteractionDeclarationStatus::federate_not_member;
  }
  if (!validObjectClass(federation->second, objectClassHandle)) {
    return DirectedInteractionDeclarationStatus::object_class_not_defined;
  }
  if (interactionClassHandles) {
    for (std::uint64_t const interactionClassHandle : *interactionClassHandles) {
      if (!validInteractionClass(federation->second, interactionClassHandle)) {
        return DirectedInteractionDeclarationStatus::interaction_class_not_defined;
      }
      if (!validDirectedInteractionForObjectClass(
              federation->second,
              objectClassHandle,
              interactionClassHandle)) {
        return DirectedInteractionDeclarationStatus::interaction_not_defined_for_object_class;
      }
    }
  }

  auto declarations = federation->second.interactionDeclarations.find(federateId);
  if (declarations != federation->second.interactionDeclarations.end()) {
    auto revised = declarations->second.subscribedObjectClassDirectedInteractions;
    if (!interactionClassHandles) {
      revised.erase(objectClassHandle);
    } else {
      auto subscribed = revised.find(objectClassHandle);
      if (subscribed != revised.end()) {
        for (std::uint64_t const interactionClassHandle : *interactionClassHandles) {
          subscribed->second.erase(interactionClassHandle);
        }
        if (subscribed->second.empty()) {
          revised.erase(subscribed);
        }
      }
    }
    declarations->second.subscribedObjectClassDirectedInteractions.swap(revised);
    if (declarations->second.publishedInteractionClasses.empty() &&
        declarations->second.subscribedInteractionClasses.empty() &&
        declarations->second.regionalSubscribedInteractionClasses.empty() &&
        declarations->second.publishedObjectClassDirectedInteractions.empty() &&
        declarations->second.subscribedObjectClassDirectedInteractions.empty()) {
      federation->second.interactionDeclarations.erase(declarations);
    }
  }
  return DirectedInteractionDeclarationStatus::applied;
}

InteractionClassDeclarationStatus EmbeddedFederationRegistry::setInteractionClassSubscription(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t interactionClassHandle,
    std::optional<bool> active) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return InteractionClassDeclarationStatus::federation_does_not_exist;
  }
  if (!federation->second.members.contains(federateId)) {
    return InteractionClassDeclarationStatus::federate_not_member;
  }
  if (!validInteractionClass(federation->second, interactionClassHandle)) {
    return InteractionClassDeclarationStatus::interaction_class_not_defined;
  }
  auto const member = federation->second.members.find(federateId);
  // The report-service interaction is excluded while Service Reporting is
  // enabled regardless of whether the caller requests an active or passive
  // subscription.  A passive declaration is still a subscription for the
  // §11.5 interlock and must not create a path around the MOM restriction.
  if (active.has_value() && member->second.serviceReportingSwitch &&
      isReportServiceInvocationInteractionClass(federation->second, interactionClassHandle)) {
    return InteractionClassDeclarationStatus::
        federate_service_invocations_are_being_reported_via_mom;
  }
  auto declarations = federation->second.interactionDeclarations.find(federateId);
  if (!active) {
    if (declarations == federation->second.interactionDeclarations.end()) {
      return InteractionClassDeclarationStatus::applied;
    }
    declarations->second.subscribedInteractionClasses.erase(interactionClassHandle);
    if (declarations->second.publishedInteractionClasses.empty() &&
        declarations->second.subscribedInteractionClasses.empty() &&
        declarations->second.regionalSubscribedInteractionClasses.empty() &&
        declarations->second.publishedObjectClassDirectedInteractions.empty() &&
        declarations->second.subscribedObjectClassDirectedInteractions.empty()) {
      federation->second.interactionDeclarations.erase(declarations);
    }
    return InteractionClassDeclarationStatus::applied;
  }

  auto [state, insertedState] = federation->second.interactionDeclarations.try_emplace(federateId);
  try {
    state->second.subscribedInteractionClasses.insert_or_assign(interactionClassHandle, *active);
  } catch (...) {
    if (insertedState && state->second.publishedInteractionClasses.empty() &&
        state->second.subscribedInteractionClasses.empty() &&
        state->second.regionalSubscribedInteractionClasses.empty() &&
        state->second.publishedObjectClassDirectedInteractions.empty() &&
        state->second.subscribedObjectClassDirectedInteractions.empty()) {
      federation->second.interactionDeclarations.erase(state);
    }
    throw;
  }
  return InteractionClassDeclarationStatus::applied;
}

RegionalInteractionClassDeclarationStatus
EmbeddedFederationRegistry::setInteractionClassRegionalSubscription(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t interactionClassHandle,
    std::set<std::uint64_t> const& regionHandles,
    bool active) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return RegionalInteractionClassDeclarationStatus::federation_does_not_exist;
  }
  if (!federation->second.members.contains(federateId)) {
    return RegionalInteractionClassDeclarationStatus::federate_not_member;
  }
  if (!validInteractionClass(federation->second, interactionClassHandle)) {
    return RegionalInteractionClassDeclarationStatus::interaction_class_not_defined;
  }
  auto const member = federation->second.members.find(federateId);
  if (member->second.serviceReportingSwitch &&
      isReportServiceInvocationInteractionClass(federation->second, interactionClassHandle)) {
    return RegionalInteractionClassDeclarationStatus::
        federate_service_invocations_are_being_reported_via_mom;
  }
  // The 2025 service explicitly gives an empty region set no effect.  It is
  // still a valid declaration invocation once the interaction class itself
  // has been resolved.
  if (regionHandles.empty()) {
    return RegionalInteractionClassDeclarationStatus::applied;
  }

  auto const availableDimensions = availableInteractionDimensions(
      federation->second,
      interactionClassHandle);
  if (!availableDimensions) {
    return RegionalInteractionClassDeclarationStatus::inconsistent_catalog;
  }
  for (std::uint64_t const regionHandle : regionHandles) {
    auto const region = federation->second.regions.find(regionHandle);
    if (region == federation->second.regions.end() ||
        !region->second.specificationCommitted ||
        region->second.committedRangeBounds.size() != region->second.dimensionHandles.size()) {
      return RegionalInteractionClassDeclarationStatus::invalid_region;
    }
    if (region->second.ownerFederateId != federateId) {
      return RegionalInteractionClassDeclarationStatus::region_not_created_by_this_federate;
    }
    if (!std::includes(
            availableDimensions->begin(),
            availableDimensions->end(),
            region->second.dimensionHandles.begin(),
            region->second.dimensionHandles.end())) {
      return RegionalInteractionClassDeclarationStatus::invalid_region_context;
    }
  }

  auto [state, insertedState] = federation->second.interactionDeclarations.try_emplace(federateId);
  try {
    auto& subscriptions = state->second.regionalSubscribedInteractionClasses[
        interactionClassHandle];
    for (std::uint64_t const regionHandle : regionHandles) {
      // Detailed 9.10 semantics permit an existing pair's active/passive
      // nature to change while keeping the pair in the subscription set.
      subscriptions.insert_or_assign(regionHandle, active);
    }
    refreshRegionUsage(federation->second);
  } catch (...) {
    if (insertedState && state->second.publishedInteractionClasses.empty() &&
        state->second.subscribedInteractionClasses.empty() &&
        state->second.regionalSubscribedInteractionClasses.empty() &&
        state->second.publishedObjectClassDirectedInteractions.empty() &&
        state->second.subscribedObjectClassDirectedInteractions.empty()) {
      federation->second.interactionDeclarations.erase(state);
    }
    throw;
  }
  return RegionalInteractionClassDeclarationStatus::applied;
}

RegionalInteractionClassDeclarationStatus
EmbeddedFederationRegistry::removeInteractionClassRegionalSubscription(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t interactionClassHandle,
    std::set<std::uint64_t> const& regionHandles) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return RegionalInteractionClassDeclarationStatus::federation_does_not_exist;
  }
  if (!federation->second.members.contains(federateId)) {
    return RegionalInteractionClassDeclarationStatus::federate_not_member;
  }
  if (!validInteractionClass(federation->second, interactionClassHandle)) {
    return RegionalInteractionClassDeclarationStatus::interaction_class_not_defined;
  }
  if (regionHandles.empty()) {
    return RegionalInteractionClassDeclarationStatus::applied;
  }

  for (std::uint64_t const regionHandle : regionHandles) {
    auto const region = federation->second.regions.find(regionHandle);
    if (region == federation->second.regions.end()) {
      return RegionalInteractionClassDeclarationStatus::invalid_region;
    }
    if (region->second.ownerFederateId != federateId) {
      return RegionalInteractionClassDeclarationStatus::region_not_created_by_this_federate;
    }
  }

  auto declarations = federation->second.interactionDeclarations.find(federateId);
  if (declarations != federation->second.interactionDeclarations.end()) {
    auto regional = declarations->second.regionalSubscribedInteractionClasses.find(
        interactionClassHandle);
    if (regional != declarations->second.regionalSubscribedInteractionClasses.end()) {
      for (std::uint64_t const regionHandle : regionHandles) {
        regional->second.erase(regionHandle);
      }
      if (regional->second.empty()) {
        declarations->second.regionalSubscribedInteractionClasses.erase(regional);
      }
    }
    if (declarations->second.publishedInteractionClasses.empty() &&
        declarations->second.subscribedInteractionClasses.empty() &&
        declarations->second.regionalSubscribedInteractionClasses.empty() &&
        declarations->second.publishedObjectClassDirectedInteractions.empty() &&
        declarations->second.subscribedObjectClassDirectedInteractions.empty()) {
      federation->second.interactionDeclarations.erase(declarations);
    }
  }
  refreshRegionUsage(federation->second);
  return RegionalInteractionClassDeclarationStatus::applied;
}

std::optional<InteractionClassDeclarationSnapshot>
EmbeddedFederationRegistry::interactionClassDeclarationFor(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t interactionClassHandle) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      !federation->second.members.contains(federateId) ||
      !validInteractionClass(federation->second, interactionClassHandle)) {
    return std::nullopt;
  }

  auto const declarations = federation->second.interactionDeclarations.find(federateId);
  if (declarations == federation->second.interactionDeclarations.end()) {
    return InteractionClassDeclarationSnapshot{};
  }
  auto const subscription = declarations->second.subscribedInteractionClasses.find(
      interactionClassHandle);
  return InteractionClassDeclarationSnapshot{
      declarations->second.publishedInteractionClasses.contains(interactionClassHandle),
      subscription == declarations->second.subscribedInteractionClasses.end()
          ? std::nullopt
          : std::optional<bool>{subscription->second},
  };
}

std::vector<DeclarationAdvisory> EmbeddedFederationRegistry::planDeclarationAdvisories(
    std::wstring const& federationName) {
  auto instrumentationScope = beginInstrumentation("planDeclarationAdvisories");
  std::scoped_lock lock(mutex_);
  return planDeclarationAdvisoriesLocked(federationName);
}

std::vector<DeclarationAdvisory>
EmbeddedFederationRegistry::planDeclarationAdvisoriesLocked(
    std::wstring const& federationName) {
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {};
  }

  auto& execution = federation->second;

  // A subscription at a class applies to instances/interactions declared at
  // that class and below.  Keep the hierarchy walk local to this planner so
  // it cannot accidentally consult a different federation's handle space.
  auto objectClassIsSameOrAncestor = [&](std::uint64_t ancestorHandle,
                                         std::uint64_t descendantHandle) {
    if (!execution.definition.catalog || !execution.objectClassHandles) {
      return false;
    }
    auto const ancestorName = execution.objectClassHandles->nameFor(ancestorHandle);
    auto const descendantName = execution.objectClassHandles->nameFor(descendantHandle);
    if (!ancestorName || !descendantName) {
      return false;
    }

    std::set<std::string> visited;
    std::string current = *descendantName;
    while (!current.empty() && visited.insert(current).second) {
      if (current == *ancestorName) {
        return true;
      }
      auto const* definition = execution.definition.catalog->objectClass(current);
      if (definition == nullptr) {
        return false;
      }
      current = definition->parentName;
    }
    return false;
  };

  auto interactionClassIsSameOrAncestor = [&](std::uint64_t ancestorHandle,
                                               std::uint64_t descendantHandle) {
    if (!execution.definition.catalog || !execution.interactionClassHandles) {
      return false;
    }
    auto const ancestorName =
        execution.interactionClassHandles->nameFor(ancestorHandle);
    auto const descendantName =
        execution.interactionClassHandles->nameFor(descendantHandle);
    if (!ancestorName || !descendantName) {
      return false;
    }

    std::set<std::string> visited;
    std::string current = *descendantName;
    while (!current.empty() && visited.insert(current).second) {
      if (current == *ancestorName) {
        return true;
      }
      auto const* definition = execution.definition.catalog->interactionClass(current);
      if (definition == nullptr) {
        return false;
      }
      current = definition->parentName;
    }
    return false;
  };

  std::set<std::pair<std::uint64_t, std::uint64_t>> currentObjectRelevance;
  for (auto const& [publishingFederateId, declarations] :
       execution.objectClassAttributeDeclarations) {
    if (!execution.members.contains(publishingFederateId)) {
      continue;
    }
    for (auto const& [publishedClassHandle, perClass] : declarations.byObjectClass) {
      static_cast<void>(perClass);
      auto const publishedAttributes = publishedObjectClassAttributes(
          execution,
          publishingFederateId,
          publishedClassHandle);
      if (!publishedAttributes || publishedAttributes->empty()) {
        continue;
      }

      bool relevant = false;
      for (auto const& [receivingFederateId, subscriptions] :
           execution.objectClassAttributeDeclarations) {
        if (receivingFederateId == publishingFederateId ||
            !execution.members.contains(receivingFederateId)) {
          continue;
        }
        for (auto const& [subscriptionClassHandle, subscribedClass] :
             subscriptions.byObjectClass) {
          if (!objectClassIsSameOrAncestor(
                  subscriptionClassHandle,
                  publishedClassHandle)) {
            continue;
          }
          for (auto const& [attributeHandle, active] :
               subscribedClass.subscribedAttributes) {
            if (active && publishedAttributes->contains(attributeHandle)) {
              relevant = true;
              break;
            }
          }
          if (!relevant) {
            // Regional attribute subscriptions are declaration state too. A
            // publisher cannot know the eventual object-region realization
            // when it registers an instance, so any active regional
            // subscription at a matching class makes the published class
            // relevant for Start/Stop Registration advisories. Passive
            // regional pairs remain retained without establishing relevance.
            for (auto const& [attributeHandle, regionStates] :
                 subscribedClass.regionalSubscribedAttributes) {
              if (!publishedAttributes->contains(attributeHandle)) {
                continue;
              }
              if (std::any_of(
                      regionStates.begin(),
                      regionStates.end(),
                      [](auto const& entry) { return entry.second; })) {
                relevant = true;
                break;
              }
            }
          }
          if (relevant) {
            break;
          }
        }
        if (relevant) {
          break;
        }
      }
      if (relevant) {
        currentObjectRelevance.emplace(publishingFederateId, publishedClassHandle);
      }
    }
  }

  std::set<std::pair<std::uint64_t, std::uint64_t>> currentInteractionRelevance;
  for (auto const& [publishingFederateId, declarations] :
       execution.interactionDeclarations) {
    if (!execution.members.contains(publishingFederateId)) {
      continue;
    }
    for (std::uint64_t const publishedClassHandle : declarations.publishedInteractionClasses) {
      bool relevant = false;
      for (auto const& [receivingFederateId, subscriptions] :
           execution.interactionDeclarations) {
        if (receivingFederateId == publishingFederateId ||
            !execution.members.contains(receivingFederateId)) {
          continue;
        }
        for (auto const& [subscriptionClassHandle, active] :
             subscriptions.subscribedInteractionClasses) {
          if (active && interactionClassIsSameOrAncestor(
                            subscriptionClassHandle,
                            publishedClassHandle)) {
            relevant = true;
            break;
          }
        }
        if (!relevant) {
          // Regional interaction subscriptions participate in the same
          // declaration-level relevance transition. Their committed region
          // overlap is a delivery concern; the declaration advisory only
          // tells the publisher that at least one active subscriber exists.
          for (auto const& [subscriptionClassHandle, regionStates] :
               subscriptions.regionalSubscribedInteractionClasses) {
            if (!interactionClassIsSameOrAncestor(
                    subscriptionClassHandle,
                    publishedClassHandle)) {
              continue;
            }
            if (std::any_of(
                    regionStates.begin(),
                    regionStates.end(),
                    [](auto const& entry) { return entry.second; })) {
              relevant = true;
              break;
            }
          }
        }
        if (relevant) {
          break;
        }
      }
      if (relevant) {
        currentInteractionRelevance.emplace(publishingFederateId, publishedClassHandle);
      }
    }
  }

  std::vector<DeclarationAdvisory> advisories;
  auto appendAdvisory = [&](DeclarationAdvisoryKind kind,
                            std::uint64_t receivingFederateId,
                            std::uint64_t classHandle,
                            bool enabled) {
    if (!enabled) {
      return;
    }
    auto const route = execution.interactionCallbackRoutes.find(receivingFederateId);
    if (route == execution.interactionCallbackRoutes.end() || !route->second) {
      return;
    }
    auto const reportRoute = execution.serviceReportRoutes.find(receivingFederateId);
    advisories.push_back({
        kind,
        receivingFederateId,
        classHandle,
        route->second,
        reportRoute == execution.serviceReportRoutes.end()
            ? FederateServiceReportRoute{}
            : reportRoute->second,
    });
  };

  for (auto const& relevance : currentObjectRelevance) {
    if (execution.objectClassRegistrationRelevance.contains(relevance)) {
      continue;
    }
    auto const member = execution.members.find(relevance.first);
    if (member == execution.members.end()) {
      continue;
    }
    appendAdvisory(
        DeclarationAdvisoryKind::start_registration_for_object_class,
        relevance.first,
        relevance.second,
        member->second.objectClassRelevanceAdvisorySwitch);
  }
  for (auto const& relevance : execution.objectClassRegistrationRelevance) {
    if (currentObjectRelevance.contains(relevance)) {
      continue;
    }
    auto const member = execution.members.find(relevance.first);
    auto const publishedAttributes = publishedObjectClassAttributes(
        execution,
        relevance.first,
        relevance.second);
    if (member == execution.members.end() ||
        !publishedAttributes || publishedAttributes->empty()) {
      continue;
    }
    appendAdvisory(
        DeclarationAdvisoryKind::stop_registration_for_object_class,
        relevance.first,
        relevance.second,
        member->second.objectClassRelevanceAdvisorySwitch);
  }

  for (auto const& relevance : currentInteractionRelevance) {
    if (execution.interactionRelevance.contains(relevance)) {
      continue;
    }
    auto const member = execution.members.find(relevance.first);
    if (member == execution.members.end()) {
      continue;
    }
    appendAdvisory(
        DeclarationAdvisoryKind::turn_interactions_on,
        relevance.first,
        relevance.second,
        member->second.interactionRelevanceAdvisorySwitch);
  }
  for (auto const& relevance : execution.interactionRelevance) {
    if (currentInteractionRelevance.contains(relevance)) {
      continue;
    }
    auto const member = execution.members.find(relevance.first);
    if (member == execution.members.end()) {
      continue;
    }
    auto const declarations = execution.interactionDeclarations.find(relevance.first);
    if (declarations == execution.interactionDeclarations.end() ||
        !declarations->second.publishedInteractionClasses.contains(relevance.second)) {
      continue;
    }
    appendAdvisory(
        DeclarationAdvisoryKind::turn_interactions_off,
        relevance.first,
        relevance.second,
        member->second.interactionRelevanceAdvisorySwitch);
  }

  execution.objectClassRegistrationRelevance = std::move(currentObjectRelevance);
  execution.interactionRelevance = std::move(currentInteractionRelevance);
  return advisories;
}

}  // namespace umbra::detail
