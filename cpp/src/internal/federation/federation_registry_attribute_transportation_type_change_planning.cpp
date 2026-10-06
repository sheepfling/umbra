#include "internal/federation/federation_registry.hpp"
#include "internal/federation/federation_registry_value_helpers.hpp"

#include <set>
#include <string>
#include <utility>

namespace umbra::detail {

AttributeTransportationTypeChangePlan
EmbeddedFederationRegistry::planAttributeTransportationTypeChange(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> const& attributeHandles,
    std::string transportationName) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {AttributeTransportationTypeChangeStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(requestingFederateId)) {
    return {AttributeTransportationTypeChangeStatus::requesting_federate_not_member};
  }
  if (!isSupportedTransportationName(
          federation->second.definition.catalog.get(),
          transportationName)) {
    return {AttributeTransportationTypeChangeStatus::invalid_transportation_type};
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() ||
      instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(requestingFederateId)) {
    return {AttributeTransportationTypeChangeStatus::object_instance_not_known};
  }
  auto const knownClass = instance->second.knownObjectClassHandlesByFederate.at(
      requestingFederateId);
  if (!validObjectClassAttributes(
          federation->second,
          knownClass,
          attributeHandles)) {
    return {AttributeTransportationTypeChangeStatus::attribute_not_defined};
  }
  for (std::uint64_t const attributeHandle : attributeHandles) {
    auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
    if (owner == instance->second.attributeOwnersByHandle.end() ||
        owner->second != requestingFederateId) {
      return {AttributeTransportationTypeChangeStatus::attribute_not_owned};
    }
    for (auto const& [requestId, pending] :
         instance->second.pendingAttributeTransportationTypeChanges) {
      static_cast<void>(requestId);
      if (pending.attributeHandles.contains(attributeHandle)) {
        return {AttributeTransportationTypeChangeStatus::attribute_already_being_changed};
      }
    }
  }
  auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
      requestingFederateId);
  if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
      !callbackRoute->second) {
    return {AttributeTransportationTypeChangeStatus::callback_route_missing};
  }
  if (attributeHandles.empty()) {
    return {
        AttributeTransportationTypeChangeStatus::applied,
        0,
        objectInstanceHandle,
        {},
        std::move(transportationName),
        callbackRoute->second,
    };
  }

  auto const requestId = federation->second.nextAttributeTransportationTypeChangeRequestId++;
  instance->second.pendingAttributeTransportationTypeChanges.emplace(
      requestId,
      Federation::ObjectInstance::PendingAttributeTransportationTypeChange{
          requestingFederateId,
          attributeHandles,
          transportationName,
      });
  return {
      AttributeTransportationTypeChangeStatus::applied,
      requestId,
      objectInstanceHandle,
      attributeHandles,
      std::move(transportationName),
      callbackRoute->second,
  };
}

}  // namespace umbra::detail
