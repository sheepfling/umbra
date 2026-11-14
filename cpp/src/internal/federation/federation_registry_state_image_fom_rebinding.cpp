#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_service_protocol.hpp"

#include "internal/fom/fom_catalog.hpp"

#include <map>
#include <utility>

namespace umbra::detail {
namespace {

using HandleMap = std::map<std::uint64_t, std::uint64_t>;

struct FomHandleMaps final {
  HandleMap objectClasses;
  HandleMap attributes;
  HandleMap interactionClasses;
  HandleMap parameters;
  HandleMap dimensions;
};

bool addMapping(HandleMap& mappings, std::uint64_t saved, std::uint64_t live) {
  return saved != 0U && live != 0U && mappings.emplace(saved, live).second;
}

bool translateHandle(std::uint64_t& handle, HandleMap const& mappings) {
  if (handle == 0U) {
    return true;
  }
  auto const found = mappings.find(handle);
  if (found == mappings.end()) {
    return false;
  }
  handle = found->second;
  return true;
}

bool translateHandles(
    std::vector<std::uint64_t>& handles,
    HandleMap const& mappings) {
  for (auto& handle : handles) {
    if (!translateHandle(handle, mappings)) {
      return false;
    }
  }
  return true;
}

bool translateRegionSnapshot(
    FederationStateImageInteractionRegionSnapshot& snapshot,
    HandleMap const& dimensions) {
  if (!translateHandles(snapshot.dimensionHandles, dimensions)) {
    return false;
  }
  for (auto& range : snapshot.committedRangeBounds) {
    if (!translateHandle(range.dimensionHandle, dimensions)) {
      return false;
    }
  }
  return true;
}

bool translateObject(
    FederationStateImageObject& object,
    FomHandleMaps const& mappings) {
  if (!translateHandle(object.registeredObjectClassHandle, mappings.objectClasses)) {
    return false;
  }
  for (auto& attribute : object.attributes) {
    if (!translateHandle(attribute.handle, mappings.attributes)) {
      return false;
    }
  }
  for (auto& value : object.attributeValues) {
    if (!translateHandle(value.attributeHandle, mappings.attributes)) {
      return false;
    }
  }
  for (auto& request : object.pendingAttributeValueUpdateRequests) {
    if (!translateHandles(request.requestedAttributeHandles, mappings.attributes)) {
      return false;
    }
  }
  for (auto& request : object.pendingAttributeValueUpdateClassRequests) {
    if (!translateHandle(request.requestedObjectClassHandle, mappings.objectClasses) ||
        !translateHandles(request.requestedAttributeHandles, mappings.attributes)) {
      return false;
    }
  }
  for (auto& request : object.pendingAttributeValueUpdateRegionalRequests) {
    if (!translateHandle(request.requestedObjectClassHandle, mappings.objectClasses) ||
        !translateHandles(request.requestedAttributeHandles, mappings.attributes)) {
      return false;
    }
    for (auto& [attributeHandle, regionHandles] : request.requestRegionsByAttribute) {
      static_cast<void>(regionHandles);
      if (!translateHandle(attributeHandle, mappings.attributes)) {
        return false;
      }
    }
  }
  for (auto& request : object.pendingAttributeOwnershipAcquisitionIfAvailableRequests) {
    if (!translateHandles(request.desiredAttributeHandles, mappings.attributes)) {
      return false;
    }
  }
  for (auto& request : object.pendingAttributeOwnershipAcquisitionRequests) {
    if (!translateHandles(request.desiredAttributeHandles, mappings.attributes) ||
        !translateHandles(request.notificationQueuedAttributeHandles, mappings.attributes) ||
        !translateHandles(request.unavailableQueuedAttributeHandles, mappings.attributes)) {
      return false;
    }
    for (auto& [owner, handles] : request.releaseCallbacksQueuedByOwningFederate) {
      static_cast<void>(owner);
      if (!translateHandles(handles, mappings.attributes)) {
        return false;
      }
    }
  }
  for (auto& cancellation : object.pendingAttributeOwnershipAcquisitionCancellations) {
    if (!translateHandles(cancellation.attributeHandles, mappings.attributes)) {
      return false;
    }
  }
  for (auto& notification : object.pendingAttributeOwnershipDivestitureIfWantedNotifications) {
    if (!translateHandles(notification.attributeHandles, mappings.attributes)) {
      return false;
    }
  }
  for (auto& notification : object.pendingConfirmDivestitureNotifications) {
    if (!translateHandles(notification.attributeHandles, mappings.attributes)) {
      return false;
    }
  }
  for (auto& request : object.pendingAttributeTransportationTypeChanges) {
    if (!translateHandles(request.attributeHandles, mappings.attributes)) {
      return false;
    }
  }
  for (auto& divestiture : object.pendingNegotiatedAttributeOwnershipDivestitures) {
    if (!translateHandle(divestiture.attributeHandle, mappings.attributes)) {
      return false;
    }
  }
  for (auto& recipients : object.ownershipAssumptionRecipientsByAttribute) {
    if (!translateHandle(recipients.attributeHandle, mappings.attributes)) {
      return false;
    }
  }
  for (auto& tag : object.ownershipAssumptionUserSuppliedTagsByAttribute) {
    if (!translateHandle(tag.attributeHandle, mappings.attributes)) {
      return false;
    }
  }
  for (auto& knownClass : object.knownObjectClassHandlesByFederate) {
    if (!translateHandle(knownClass.objectClassHandle, mappings.objectClasses)) {
      return false;
    }
  }
  return true;
}

}  // namespace

bool EmbeddedFederationRegistry::rebindFomHandlesInStateImage(
    FederationStateImage& image,
    Federation const& liveFederation) {
  if (!image.fomHandlesPresent || !liveFederation.definition.catalog ||
      !liveFederation.objectClassHandles || !liveFederation.attributeHandles ||
      !liveFederation.interactionClassHandles || !liveFederation.parameterHandles ||
      !liveFederation.dimensionHandles) {
    return false;
  }

  FomHandleMaps mappings;
  auto const* catalog = liveFederation.definition.catalog.get();
  for (auto const& identity : image.fomHandles) {
    std::optional<std::uint64_t> liveHandle;
    HandleMap* destination = nullptr;
    switch (identity.kind) {
      case FederationStateImageFomHandleKind::object_class:
        destination = &mappings.objectClasses;
        liveHandle = liveFederation.objectClassHandles->handleFor(identity.name);
        break;
      case FederationStateImageFomHandleKind::attribute:
        destination = &mappings.attributes;
        liveHandle = liveFederation.attributeHandles->handleFor(
            catalog, identity.containingName, identity.name);
        break;
      case FederationStateImageFomHandleKind::interaction_class:
        destination = &mappings.interactionClasses;
        liveHandle = liveFederation.interactionClassHandles->handleFor(identity.name);
        break;
      case FederationStateImageFomHandleKind::parameter:
        destination = &mappings.parameters;
        liveHandle = liveFederation.parameterHandles->handleFor(
            catalog, identity.containingName, identity.name);
        break;
      case FederationStateImageFomHandleKind::dimension:
        destination = &mappings.dimensions;
        liveHandle = liveFederation.dimensionHandles->handleFor(identity.name);
        break;
    }
    if (destination == nullptr || !liveHandle ||
        !addMapping(*destination, identity.handle, *liveHandle)) {
      return false;
    }
  }

  for (auto& member : image.members) {
    for (auto& count : member.successfulUpdateCountsByClassAndTransportation) {
      if (!translateHandle(count.objectClassHandle, mappings.objectClasses)) {
        return false;
      }
    }
    for (auto& objectClass : member.successfullyUpdatedObjectInstanceClassHandles) {
      if (!translateHandle(objectClass.objectClassHandle, mappings.objectClasses)) {
        return false;
      }
    }
    for (auto& count : member.successfulReflectionCountsByClassAndTransportation) {
      if (!translateHandle(count.objectClassHandle, mappings.objectClasses)) {
        return false;
      }
    }
    for (auto& objectClass : member.successfullyReflectedObjectInstanceClassHandles) {
      if (!translateHandle(objectClass.objectClassHandle, mappings.objectClasses)) {
        return false;
      }
    }
    for (auto& count : member.successfulInteractionCountsByClassAndTransportation) {
      if (!translateHandle(count.interactionClassHandle, mappings.interactionClasses)) {
        return false;
      }
    }
    for (auto& count : member.successfulDirectedInteractionCountsByClassAndTransportation) {
      if (!translateHandle(count.interactionClassHandle, mappings.interactionClasses)) {
        return false;
      }
    }
    for (auto& count : member.successfulInteractionReceiptCountsByClassAndTransportation) {
      if (!translateHandle(count.interactionClassHandle, mappings.interactionClasses)) {
        return false;
      }
    }
    for (auto& count : member.successfulDirectedInteractionReceiptCountsByClassAndTransportation) {
      if (!translateHandle(count.interactionClassHandle, mappings.interactionClasses)) {
        return false;
      }
    }
  }

  for (auto& object : image.objects) {
    if (!translateObject(object, mappings)) {
      return false;
    }
  }
  for (auto& association : image.deferredUpdateRegionAssociations) {
    if (!translateHandle(association.attributeHandle, mappings.attributes)) {
      return false;
    }
  }
  for (auto& query : image.pendingAttributeOwnershipQueries) {
    if (!translateHandles(query.requestedAttributeHandles, mappings.attributes)) {
      return false;
    }
  }
  for (auto& assumption : image.pendingAttributeOwnershipAssumptions) {
    if (!translateHandles(assumption.attributeHandles, mappings.attributes)) {
      return false;
    }
  }

  for (auto& declarations : image.interactionDeclarations) {
    if (!translateHandles(declarations.publishedInteractionClasses, mappings.interactionClasses)) {
      return false;
    }
    for (auto& subscription : declarations.subscribedInteractionClasses) {
      if (!translateHandle(subscription.interactionClassHandle, mappings.interactionClasses)) {
        return false;
      }
    }
    for (auto& subscription : declarations.regionalSubscribedInteractionClasses) {
      if (!translateHandle(subscription.interactionClassHandle, mappings.interactionClasses)) {
        return false;
      }
    }
    for (auto& directed : declarations.publishedObjectClassDirectedInteractions) {
      if (!translateHandle(directed.objectClassHandle, mappings.objectClasses) ||
          !translateHandle(directed.interactionClassHandle, mappings.interactionClasses)) {
        return false;
      }
    }
    for (auto& directed : declarations.subscribedObjectClassDirectedInteractions) {
      if (!translateHandle(directed.objectClassHandle, mappings.objectClasses) ||
          !translateHandle(directed.interactionClassHandle, mappings.interactionClasses)) {
        return false;
      }
    }
    for (auto& value : declarations.interactionTransportationTypes) {
      if (!translateHandle(value.interactionClassHandle, mappings.interactionClasses)) {
        return false;
      }
    }
    for (auto& value : declarations.interactionOrderTypes) {
      if (!translateHandle(value.interactionClassHandle, mappings.interactionClasses)) {
        return false;
      }
    }
    for (auto& value : declarations.pendingInteractionTransportationTypeChanges) {
      if (!translateHandle(value.interactionClassHandle, mappings.interactionClasses)) {
        return false;
      }
    }
  }

  auto const translateInteractionParameters = [&](auto& parameters) {
    for (auto& parameter : parameters) {
      if (!translateHandle(parameter.parameterHandle, mappings.parameters)) {
        return false;
      }
    }
    return true;
  };
  auto const translateRegionSnapshots = [&](auto& snapshots) {
    for (auto& snapshot : snapshots) {
      if (!translateRegionSnapshot(snapshot, mappings.dimensions)) {
        return false;
      }
    }
    return true;
  };
  for (auto& message : image.tsoInteractionMessages) {
    if (!translateHandle(message.sentInteractionClassHandle, mappings.interactionClasses) ||
        !translateHandles(message.sentParameterHandles, mappings.parameters) ||
        !translateInteractionParameters(message.parameters) ||
        !translateRegionSnapshots(message.sentRegionSnapshots)) {
      return false;
    }
  }
  for (auto& message : image.tsoAttributeUpdateMessages) {
    for (auto& attribute : message.attributes) {
      if (!translateHandle(attribute.attributeHandle, mappings.attributes)) {
        return false;
      }
    }
    for (auto& recipient : message.passelsByRecipient) {
      for (auto& passel : recipient.passels) {
        if (!translateHandles(passel.sentAttributeHandles, mappings.attributes) ||
            !translateRegionSnapshots(passel.sentRegionSnapshots)) {
          return false;
        }
      }
    }
    if (!translateRegionSnapshots(message.sentRegionSnapshots)) {
      return false;
    }
  }
  for (auto& message : image.tsoObjectDeletionMessages) {
    if (!message.reconstitution) {
      continue;
    }
    if (!translateObject(message.reconstitution->object, mappings)) {
      return false;
    }
    for (auto& [federateId, objectClassHandle] :
         message.reconstitution->knownObjectClassHandlesByFederate) {
      static_cast<void>(federateId);
      if (!translateHandle(objectClassHandle, mappings.objectClasses)) {
        return false;
      }
    }
  }
  for (auto& message : image.tsoDirectedInteractionMessages) {
    if (!translateHandle(message.sentInteractionClassHandle, mappings.interactionClasses) ||
        !translateHandles(message.sentParameterHandles, mappings.parameters) ||
        !translateInteractionParameters(message.parameters)) {
      return false;
    }
    for (auto& recipient : message.recipients) {
      if (!translateHandle(
              recipient.receivedInteractionClassHandle,
              mappings.interactionClasses) ||
          !translateHandles(recipient.receivedParameterHandles, mappings.parameters)) {
        return false;
      }
    }
  }
  for (auto& interaction : image.queuedReceiveOrderInteractions) {
    if (!translateHandle(interaction.interactionClassHandle, mappings.interactionClasses) ||
        !translateHandles(interaction.parameterHandles, mappings.parameters)) {
      return false;
    }
    std::vector<std::uint8_t> payloadBytes;
    payloadBytes.reserve(interaction.payload.size());
    for (unsigned char const byte : interaction.payload) {
      payloadBytes.push_back(static_cast<std::uint8_t>(byte));
    }
    auto envelope = decodeProcessFederationInteractionEnvelope(payloadBytes);
    if (!envelope) {
      // Older private callers used the queue payload as an opaque tag and
      // carried no parameter handles. Preserve that legacy case; a parameter
      // value envelope must be decoded so its handle keys can follow the same
      // FOM rebinding as the event's parameter-handle list.
      if (!interaction.parameterHandles.empty()) {
        return false;
      }
      continue;
    }
    for (auto& [parameterHandle, value] : envelope->parameterValues) {
      static_cast<void>(value);
      if (!translateHandle(parameterHandle, mappings.parameters)) {
        return false;
      }
    }
    auto reboundPayload = encodeProcessFederationInteractionEnvelope(*envelope);
    interaction.payload.assign(reboundPayload.begin(), reboundPayload.end());
  }
  for (auto& update : image.queuedReceiveOrderAttributeUpdates) {
    for (auto& value : update.values) {
      if (!translateHandle(value.attributeHandle, mappings.attributes)) {
        return false;
      }
    }
  }

  for (auto& region : image.regions) {
    if (!translateHandles(region.dimensionHandles, mappings.dimensions)) {
      return false;
    }
    for (auto& range : region.pendingRangeBounds) {
      if (!translateHandle(range.dimensionHandle, mappings.dimensions)) {
        return false;
      }
    }
    for (auto& range : region.committedRangeBounds) {
      if (!translateHandle(range.dimensionHandle, mappings.dimensions)) {
        return false;
      }
    }
  }
  for (auto& declarations : image.objectClassAttributeDeclarations) {
    for (auto& objectClass : declarations.classes) {
      if (!translateHandle(objectClass.objectClassHandle, mappings.objectClasses) ||
          !translateHandles(
              objectClass.explicitlyPublishedAttributeHandles, mappings.attributes)) {
        return false;
      }
      for (auto& value : objectClass.subscribedAttributes) {
        if (!translateHandle(value.attributeHandle, mappings.attributes)) {
          return false;
        }
      }
      for (auto& value : objectClass.subscribedUpdateRateDesignators) {
        if (!translateHandle(value.attributeHandle, mappings.attributes)) {
          return false;
        }
      }
      for (auto& value : objectClass.regionalSubscribedAttributes) {
        if (!translateHandle(value.attributeHandle, mappings.attributes)) {
          return false;
        }
      }
      for (auto& value : objectClass.regionalSubscribedUpdateRateDesignators) {
        if (!translateHandle(value.attributeHandle, mappings.attributes)) {
          return false;
        }
      }
      for (auto& value : objectClass.defaultTransportationTypes) {
        if (!translateHandle(value.attributeHandle, mappings.attributes)) {
          return false;
        }
      }
      for (auto& value : objectClass.defaultOrderTypes) {
        if (!translateHandle(value.attributeHandle, mappings.attributes)) {
          return false;
        }
      }
    }
  }
  return true;
}

}  // namespace umbra::detail
