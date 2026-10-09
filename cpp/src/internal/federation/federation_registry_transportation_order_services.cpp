#include "internal/federation/federation_registry.hpp"
#include "internal/federation/federation_registry_value_helpers.hpp"

#include <set>
#include <string>
#include <utility>

namespace umbra::detail {

AttributeOrderTypeChangeStatus EmbeddedFederationRegistry::changeAttributeOrderType(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> const& attributeHandles,
    rti1516_2025::OrderType orderType) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return AttributeOrderTypeChangeStatus::federation_does_not_exist;
  }
  if (!federation->second.members.contains(requestingFederateId)) {
    return AttributeOrderTypeChangeStatus::requesting_federate_not_member;
  }
  if (orderType != rti1516_2025::RECEIVE && orderType != rti1516_2025::TIMESTAMP) {
    return AttributeOrderTypeChangeStatus::invalid_order_type;
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() ||
      instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(requestingFederateId)) {
    return AttributeOrderTypeChangeStatus::object_instance_not_known;
  }
  auto const knownClass = instance->second.knownObjectClassHandlesByFederate.at(
      requestingFederateId);
  if (!validObjectClassAttributes(federation->second, knownClass, attributeHandles)) {
    return AttributeOrderTypeChangeStatus::attribute_not_defined;
  }
  for (std::uint64_t const attributeHandle : attributeHandles) {
    auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
    if (owner == instance->second.attributeOwnersByHandle.end() ||
        owner->second != requestingFederateId) {
      return AttributeOrderTypeChangeStatus::attribute_not_owned;
    }
  }
  for (std::uint64_t const attributeHandle : attributeHandles) {
    instance->second.attributeOrderTypes.insert_or_assign(attributeHandle, orderType);
  }
  return AttributeOrderTypeChangeStatus::applied;
}

std::optional<AttributeTransportationTypeChangeDelivery>
EmbeddedFederationRegistry::beginAttributeTransportationTypeChange(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t requestId) {
  if (requestId == 0) {
    return std::nullopt;
  }
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }
  for (auto& [objectInstanceHandle, instance] : federation->second.objectInstances) {
    auto pending = instance.pendingAttributeTransportationTypeChanges.find(requestId);
    if (pending == instance.pendingAttributeTransportationTypeChanges.end()) {
      continue;
    }
    auto pendingState = std::move(pending->second);
    instance.pendingAttributeTransportationTypeChanges.erase(pending);
    if (pendingState.requestingFederateId != requestingFederateId ||
        instance.deleteAccepted ||
        !federation->second.members.contains(requestingFederateId) ||
        !instance.knownObjectClassHandlesByFederate.contains(requestingFederateId)) {
      return std::nullopt;
    }
    for (std::uint64_t const attributeHandle : pendingState.attributeHandles) {
      auto const owner = instance.attributeOwnersByHandle.find(attributeHandle);
      if (owner == instance.attributeOwnersByHandle.end() ||
          owner->second != requestingFederateId) {
        return std::nullopt;
      }
    }
    for (std::uint64_t const attributeHandle : pendingState.attributeHandles) {
      instance.attributeTransportationTypes.insert_or_assign(
          attributeHandle,
          pendingState.transportationName);
    }
    return AttributeTransportationTypeChangeDelivery{
        objectInstanceHandle,
        std::move(pendingState.attributeHandles),
        std::move(pendingState.transportationName),
    };
  }
  return std::nullopt;
}

void EmbeddedFederationRegistry::cancelAttributeTransportationTypeChange(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t requestId) {
  if (requestId == 0) {
    return;
  }
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return;
  }
  for (auto& [objectInstanceHandle, instance] : federation->second.objectInstances) {
    static_cast<void>(objectInstanceHandle);
    auto pending = instance.pendingAttributeTransportationTypeChanges.find(requestId);
    if (pending != instance.pendingAttributeTransportationTypeChanges.end() &&
        pending->second.requestingFederateId == requestingFederateId) {
      instance.pendingAttributeTransportationTypeChanges.erase(pending);
      return;
    }
  }
}

AttributeTransportationTypeDefaultStatus
EmbeddedFederationRegistry::changeDefaultAttributeTransportationType(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectClassHandle,
    std::set<std::uint64_t> const& attributeHandles,
    std::string transportationName) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return AttributeTransportationTypeDefaultStatus::federation_does_not_exist;
  }
  if (!federation->second.members.contains(requestingFederateId)) {
    return AttributeTransportationTypeDefaultStatus::requesting_federate_not_member;
  }
  if (!isSupportedTransportationName(
          federation->second.definition.catalog.get(),
          transportationName)) {
    return AttributeTransportationTypeDefaultStatus::invalid_transportation_type;
  }
  if (!validObjectClassAttributes(
          federation->second,
          objectClassHandle,
          attributeHandles)) {
    return validObjectClass(federation->second, objectClassHandle)
        ? AttributeTransportationTypeDefaultStatus::attribute_not_defined
        : AttributeTransportationTypeDefaultStatus::object_class_not_defined;
  }
  auto& declarations = federation->second.objectClassAttributeDeclarations[requestingFederateId];
  auto& perClass = declarations.byObjectClass[objectClassHandle];
  for (std::uint64_t const attributeHandle : attributeHandles) {
    perClass.defaultTransportationTypes.insert_or_assign(
        attributeHandle,
        transportationName);
  }
  return AttributeTransportationTypeDefaultStatus::applied;
}

AttributeOrderTypeDefaultStatus EmbeddedFederationRegistry::changeDefaultAttributeOrderType(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectClassHandle,
    std::set<std::uint64_t> const& attributeHandles,
    rti1516_2025::OrderType orderType) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return AttributeOrderTypeDefaultStatus::federation_does_not_exist;
  }
  if (!federation->second.members.contains(requestingFederateId)) {
    return AttributeOrderTypeDefaultStatus::requesting_federate_not_member;
  }
  if (orderType != rti1516_2025::RECEIVE && orderType != rti1516_2025::TIMESTAMP) {
    return AttributeOrderTypeDefaultStatus::invalid_order_type;
  }
  if (!validObjectClass(federation->second, objectClassHandle)) {
    return AttributeOrderTypeDefaultStatus::object_class_not_defined;
  }
  if (!validObjectClassAttributes(
          federation->second,
          objectClassHandle,
          attributeHandles)) {
    return AttributeOrderTypeDefaultStatus::attribute_not_defined;
  }
  auto& declarations = federation->second.objectClassAttributeDeclarations[requestingFederateId];
  auto& perClass = declarations.byObjectClass[objectClassHandle];
  for (std::uint64_t const attributeHandle : attributeHandles) {
    perClass.defaultOrderTypes.insert_or_assign(attributeHandle, orderType);
  }
  return AttributeOrderTypeDefaultStatus::applied;
}

AttributeTransportationTypeQueryPlan
EmbeddedFederationRegistry::planAttributeTransportationTypeQuery(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t attributeHandle) const {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {AttributeTransportationTypeQueryStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(requestingFederateId)) {
    return {AttributeTransportationTypeQueryStatus::requesting_federate_not_member};
  }

  // RTI-owned MOM objects share the public object-instance handle namespace
  // with federate-created instances, but their lifetime/visibility ledger is
  // intentionally separate.  Query Attribute Transportation Type is still a
  // normal object-management service for a discovered MOM object: resolve
  // the immutable MIM declaration rather than looking for a synthetic
  // ObjectInstance record that does not exist.
  auto const momObject = federation->second.rtiOwnedJoinedFederateMomObjects.find(
      objectInstanceHandle);
  if (momObject != federation->second.rtiOwnedJoinedFederateMomObjects.end()) {
    if (!momObject->second.knownFederateIds.contains(requestingFederateId)) {
      return {AttributeTransportationTypeQueryStatus::object_instance_not_known};
    }
    if (!momObject->second.effectiveAttributeHandles.contains(attributeHandle) ||
        !validObjectClassAttributes(
            federation->second,
            momObject->second.objectClassHandle,
            std::set<std::uint64_t>{attributeHandle})) {
      return {AttributeTransportationTypeQueryStatus::attribute_not_defined};
    }
    auto const transportationName = attributeTransportationName(
        federation->second,
        momObject->second.objectClassHandle,
        attributeHandle);
    if (!transportationName) {
      return {AttributeTransportationTypeQueryStatus::inconsistent_catalog};
    }
    auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
        requestingFederateId);
    if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
        !callbackRoute->second) {
      return {AttributeTransportationTypeQueryStatus::callback_route_missing};
    }
    return {
        AttributeTransportationTypeQueryStatus::applied,
        objectInstanceHandle,
        attributeHandle,
        *transportationName,
        callbackRoute->second,
    };
  }

  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() ||
      instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(requestingFederateId)) {
    return {AttributeTransportationTypeQueryStatus::object_instance_not_known};
  }
  auto const knownClass = instance->second.knownObjectClassHandlesByFederate.at(
      requestingFederateId);
  if (!validObjectClassAttributes(
          federation->second,
          knownClass,
          std::set<std::uint64_t>{attributeHandle})) {
    return {AttributeTransportationTypeQueryStatus::attribute_not_defined};
  }
  auto const transportationName = effectiveAttributeTransportationName(
      federation->second,
      instance->second,
      attributeHandle);
  if (!transportationName) {
    return {AttributeTransportationTypeQueryStatus::inconsistent_catalog};
  }
  auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
      requestingFederateId);
  if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
      !callbackRoute->second) {
    return {AttributeTransportationTypeQueryStatus::callback_route_missing};
  }
  return {
      AttributeTransportationTypeQueryStatus::applied,
      objectInstanceHandle,
      attributeHandle,
      *transportationName,
      callbackRoute->second,
  };
}

std::optional<AttributeTransportationTypeQueryPlan>
EmbeddedFederationRegistry::attributeTransportationTypeQueryFor(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t attributeHandle) const {
  auto plan = planAttributeTransportationTypeQuery(
      federationName,
      requestingFederateId,
      objectInstanceHandle,
      attributeHandle);
  if (plan.status != AttributeTransportationTypeQueryStatus::applied) {
    return std::nullopt;
  }
  return plan;
}

InteractionTransportationTypeChangePlan
EmbeddedFederationRegistry::planInteractionTransportationTypeChange(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t interactionClassHandle,
    std::string transportationName) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {InteractionTransportationTypeChangeStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(requestingFederateId)) {
    return {InteractionTransportationTypeChangeStatus::requesting_federate_not_member};
  }
  if (!validInteractionClass(federation->second, interactionClassHandle)) {
    return {InteractionTransportationTypeChangeStatus::interaction_class_not_defined};
  }
  if (!isSupportedTransportationName(
      federation->second.definition.catalog.get(),
      transportationName)) {
    return {InteractionTransportationTypeChangeStatus::invalid_transportation_type};
  }
  auto declarations = federation->second.interactionDeclarations.find(requestingFederateId);
  bool published =
      declarations != federation->second.interactionDeclarations.end() &&
      declarations->second.publishedInteractionClasses.contains(interactionClassHandle);
  if (!published && declarations != federation->second.interactionDeclarations.end()) {
    // Directed interactions are published on an object-class/interaction-class
    // pair.  They still use the interaction-class transportation control
    // service, so an active directed publication is a valid publication
    // boundary for this request as well.
    for (auto const& [objectClassHandle, directedClasses] :
         declarations->second.publishedObjectClassDirectedInteractions) {
      static_cast<void>(objectClassHandle);
      if (directedClasses.contains(interactionClassHandle)) {
        published = true;
        break;
      }
    }
  }
  if (!published) {
    return {InteractionTransportationTypeChangeStatus::interaction_class_not_published};
  }
  if (declarations->second.pendingInteractionTransportationTypeChanges.contains(
          interactionClassHandle)) {
    return {InteractionTransportationTypeChangeStatus::interaction_class_already_being_changed};
  }
  auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
      requestingFederateId);
  if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
      !callbackRoute->second) {
    return {InteractionTransportationTypeChangeStatus::callback_route_missing};
  }
  declarations->second.pendingInteractionTransportationTypeChanges.insert_or_assign(
      interactionClassHandle,
      std::move(transportationName));
  return {
      InteractionTransportationTypeChangeStatus::applied,
      interactionClassHandle,
      declarations->second.pendingInteractionTransportationTypeChanges.at(
          interactionClassHandle),
      callbackRoute->second,
  };
}

InteractionOrderTypeChangeStatus EmbeddedFederationRegistry::changeInteractionOrderType(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t interactionClassHandle,
    rti1516_2025::OrderType orderType) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return InteractionOrderTypeChangeStatus::federation_does_not_exist;
  }
  if (!federation->second.members.contains(requestingFederateId)) {
    return InteractionOrderTypeChangeStatus::requesting_federate_not_member;
  }
  if (orderType != rti1516_2025::RECEIVE && orderType != rti1516_2025::TIMESTAMP) {
    return InteractionOrderTypeChangeStatus::invalid_order_type;
  }
  if (!validInteractionClass(federation->second, interactionClassHandle)) {
    return InteractionOrderTypeChangeStatus::interaction_class_not_defined;
  }
  auto declarations = federation->second.interactionDeclarations.find(requestingFederateId);
  bool published =
      declarations != federation->second.interactionDeclarations.end() &&
      declarations->second.publishedInteractionClasses.contains(interactionClassHandle);
  // Directed interactions are published through an object-class declaration,
  // but they remain interaction classes for the order-control service. Treat
  // any active directed publication as the corresponding interaction-class
  // publication boundary.
  if (!published && declarations != federation->second.interactionDeclarations.end()) {
    for (auto const& [objectClassHandle, directedClasses] :
         declarations->second.publishedObjectClassDirectedInteractions) {
      static_cast<void>(objectClassHandle);
      if (directedClasses.contains(interactionClassHandle)) {
        published = true;
        break;
      }
    }
  }
  if (!published) {
    return InteractionOrderTypeChangeStatus::interaction_class_not_published;
  }
  declarations->second.interactionOrderTypes.insert_or_assign(interactionClassHandle, orderType);
  return InteractionOrderTypeChangeStatus::applied;
}

std::optional<std::string>
EmbeddedFederationRegistry::beginInteractionTransportationTypeChange(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t interactionClassHandle) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }
  auto declarations = federation->second.interactionDeclarations.find(requestingFederateId);
  if (declarations == federation->second.interactionDeclarations.end()) {
    return std::nullopt;
  }
  auto pending = declarations->second.pendingInteractionTransportationTypeChanges.find(
      interactionClassHandle);
  if (pending == declarations->second.pendingInteractionTransportationTypeChanges.end()) {
    return std::nullopt;
  }
  auto transportationName = std::move(pending->second);
  declarations->second.pendingInteractionTransportationTypeChanges.erase(pending);
  bool published = declarations->second.publishedInteractionClasses.contains(
      interactionClassHandle);
  if (!published) {
    for (auto const& [objectClassHandle, directedClasses] :
         declarations->second.publishedObjectClassDirectedInteractions) {
      static_cast<void>(objectClassHandle);
      if (directedClasses.contains(interactionClassHandle)) {
        published = true;
        break;
      }
    }
  }
  if (!federation->second.members.contains(requestingFederateId) || !published) {
    return std::nullopt;
  }
  declarations->second.interactionTransportationTypes.insert_or_assign(
      interactionClassHandle,
      transportationName);
  return transportationName;
}

void EmbeddedFederationRegistry::cancelInteractionTransportationTypeChange(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t interactionClassHandle) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return;
  }
  auto declarations = federation->second.interactionDeclarations.find(requestingFederateId);
  if (declarations != federation->second.interactionDeclarations.end()) {
    declarations->second.pendingInteractionTransportationTypeChanges.erase(
        interactionClassHandle);
  }
}

InteractionTransportationTypeQueryPlan
EmbeddedFederationRegistry::planInteractionTransportationTypeQuery(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t queriedFederateId,
    std::uint64_t interactionClassHandle) const {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {InteractionTransportationTypeQueryStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(requestingFederateId)) {
    return {InteractionTransportationTypeQueryStatus::requesting_federate_not_member};
  }
  if (!validInteractionClass(federation->second, interactionClassHandle) ||
      !federation->second.definition.catalog ||
      !federation->second.interactionClassHandles) {
    return {InteractionTransportationTypeQueryStatus::interaction_class_not_defined};
  }
  auto const className = federation->second.interactionClassHandles->nameFor(
      interactionClassHandle);
  auto const* interactionClass = className
      ? federation->second.definition.catalog->interactionClass(*className)
      : nullptr;
  if (interactionClass == nullptr || interactionClass->transportation.empty()) {
    return {InteractionTransportationTypeQueryStatus::inconsistent_catalog};
  }
  std::string transportationName = interactionClass->transportation;
  auto declarations = federation->second.interactionDeclarations.find(queriedFederateId);
  bool published =
      declarations != federation->second.interactionDeclarations.end() &&
      declarations->second.publishedInteractionClasses.contains(interactionClassHandle);
  if (!published && declarations != federation->second.interactionDeclarations.end()) {
    for (auto const& [objectClassHandle, directedClasses] :
         declarations->second.publishedObjectClassDirectedInteractions) {
      static_cast<void>(objectClassHandle);
      if (directedClasses.contains(interactionClassHandle)) {
        published = true;
        break;
      }
    }
  }
  if (published) {
    auto const overrideType = declarations->second.interactionTransportationTypes.find(
        interactionClassHandle);
    if (overrideType != declarations->second.interactionTransportationTypes.end()) {
      transportationName = overrideType->second;
    }
  }
  auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
      requestingFederateId);
  if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
      !callbackRoute->second) {
    return {InteractionTransportationTypeQueryStatus::callback_route_missing};
  }
  return {
      InteractionTransportationTypeQueryStatus::applied,
      queriedFederateId,
      interactionClassHandle,
      std::move(transportationName),
      callbackRoute->second,
  };
}

std::optional<InteractionTransportationTypeQueryPlan>
EmbeddedFederationRegistry::interactionTransportationTypeQueryFor(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t queriedFederateId,
    std::uint64_t interactionClassHandle) const {
  auto plan = planInteractionTransportationTypeQuery(
      federationName,
      requestingFederateId,
      queriedFederateId,
      interactionClassHandle);
  if (plan.status != InteractionTransportationTypeQueryStatus::applied) {
    return std::nullopt;
  }
  return plan;
}


}  // namespace umbra::detail
