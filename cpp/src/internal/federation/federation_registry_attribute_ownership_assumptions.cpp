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
UnconditionalAttributeOwnershipDivestiturePlan
EmbeddedFederationRegistry::planUnconditionalAttributeOwnershipDivestiture(
    std::wstring const& federationName,
    std::uint64_t divestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> const& attributeHandles,
    std::vector<unsigned char> userSuppliedTag) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {UnconditionalAttributeOwnershipDivestitureStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(divestingFederateId)) {
    return {UnconditionalAttributeOwnershipDivestitureStatus::divesting_federate_not_member};
  }
  // IEEE 1516.1-2025 11.4.2(k) prohibits ownership transfer of predefined
  // RTI-owned MOM attributes.
  if (federation->second.rtiOwnedJoinedFederateMomObjects.contains(objectInstanceHandle)) {
    return {UnconditionalAttributeOwnershipDivestitureStatus::attribute_owned_by_rti};
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() || instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(divestingFederateId)) {
    return {UnconditionalAttributeOwnershipDivestitureStatus::object_instance_not_known};
  }
  if (!federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    return {UnconditionalAttributeOwnershipDivestitureStatus::inconsistent_catalog};
  }

  auto const divestingKnownClass = instance->second.knownObjectClassHandlesByFederate.find(
      divestingFederateId);
  auto const divestingKnownClassName = federation->second.objectClassHandles->nameFor(
      divestingKnownClass->second);
  if (!divestingKnownClassName ||
      federation->second.definition.catalog->objectClass(*divestingKnownClassName) == nullptr) {
    return {UnconditionalAttributeOwnershipDivestitureStatus::inconsistent_catalog};
  }
  for (std::uint64_t const attributeHandle : attributeHandles) {
    if (!federation->second.attributeHandles->nameFor(
            federation->second.definition.catalog.get(),
            *divestingKnownClassName,
            attributeHandle)) {
      return {UnconditionalAttributeOwnershipDivestitureStatus::attribute_not_defined};
    }
    auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
    if (owner == instance->second.attributeOwnersByHandle.end() ||
        owner->second != divestingFederateId) {
      return {UnconditionalAttributeOwnershipDivestitureStatus::attribute_not_owned};
    }
  }

  // An empty supplied set has no ownership, pending-acquisition, or callback
  // effect after the common connection/member/known-instance validation.
  if (attributeHandles.empty()) {
    return {};
  }

  // Unconditional divestiture supersedes any negotiated offer or prior
  // assumption interval for the same attribute. Drop those reservations before
  // constructing the new candidate set so an old evoked callback cannot leak
  // the previous tag into the new unowned search.
  for (std::uint64_t const attributeHandle : attributeHandles) {
    clearOwnershipAssumptionSearch(instance->second, attributeHandle);
  }

  auto recipientHasPendingAcquisition = [&instance](
                                          std::uint64_t receivingFederateId,
                                          std::uint64_t attributeHandle) {
    for (auto const& [requestId, pending] :
         instance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests) {
      static_cast<void>(requestId);
      if (pending.requestingFederateId == receivingFederateId &&
          pending.desiredAttributeHandles.contains(attributeHandle)) {
        return true;
      }
    }
    for (auto const& [requestId, pending] :
         instance->second.pendingAttributeOwnershipAcquisitionRequests) {
      static_cast<void>(requestId);
      if (pending.requestingFederateId == receivingFederateId &&
          pending.desiredAttributeHandles.contains(attributeHandle)) {
        return true;
      }
    }
    for (auto const& [cancellationId, cancellation] :
         instance->second.pendingAttributeOwnershipAcquisitionCancellations) {
      static_cast<void>(cancellationId);
      if (cancellation.requestingFederateId == receivingFederateId &&
          cancellation.attributeHandles.contains(attributeHandle)) {
        // Keep a cancellation confirmation terminal: an old offer must not
        // race ahead of the corresponding callback boundary.
        return true;
      }
    }
    return false;
  };

  // Complete every potentially allocating candidate lookup before the
  // unconditional state transition. A recipient needs a known class, its
  // corresponding attribute published there, and no still-pending acquisition
  // state for that attribute. Existing regular/If Available requesters are
  // resolved through their own standard callback paths below rather than being
  // offered a duplicate Request Attribute Ownership Assumption callback.
  std::map<std::uint64_t, std::set<std::uint64_t>> offeredAttributesByFederate;
  for (auto const& [receivingFederateId, membership] : federation->second.members) {
    static_cast<void>(membership);
    if (receivingFederateId == divestingFederateId) {
      continue;
    }
    auto const receivingKnownClass = instance->second.knownObjectClassHandlesByFederate.find(
        receivingFederateId);
    if (receivingKnownClass == instance->second.knownObjectClassHandlesByFederate.end()) {
      continue;
    }
    auto const receivingKnownClassName = federation->second.objectClassHandles->nameFor(
        receivingKnownClass->second);
    if (!receivingKnownClassName ||
        federation->second.definition.catalog->objectClass(*receivingKnownClassName) == nullptr) {
      return {UnconditionalAttributeOwnershipDivestitureStatus::inconsistent_catalog};
    }
    auto const publishedAttributes = publishedObjectClassAttributes(
        federation->second,
        receivingFederateId,
        receivingKnownClass->second);
    if (!publishedAttributes) {
      return {UnconditionalAttributeOwnershipDivestitureStatus::inconsistent_catalog};
    }
    for (std::uint64_t const attributeHandle : attributeHandles) {
      if (!federation->second.attributeHandles->nameFor(
              federation->second.definition.catalog.get(),
              *receivingKnownClassName,
              attributeHandle) ||
          !publishedAttributes->contains(attributeHandle) ||
          recipientHasPendingAcquisition(receivingFederateId, attributeHandle)) {
        continue;
      }
      offeredAttributesByFederate[receivingFederateId].insert(attributeHandle);
    }
  }

  UnconditionalAttributeOwnershipDivestiturePlan result;
  result.assumptionRecipients.reserve(offeredAttributesByFederate.size());
  for (auto& [receivingFederateId, offeredAttributes] : offeredAttributesByFederate) {
    auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
        receivingFederateId);
    if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
        !callbackRoute->second) {
      return {UnconditionalAttributeOwnershipDivestitureStatus::inconsistent_catalog};
    }
    result.assumptionRecipients.push_back({
        receivingFederateId,
        objectInstanceHandle,
        std::move(offeredAttributes),
        callbackRoute->second,
        userSuppliedTag,
    });
  }

  // Retain the recipients already offered so later discovery/publication
  // events can continue the search without duplicating this callback.
  for (auto const& recipient : result.assumptionRecipients) {
    for (std::uint64_t const attributeHandle : recipient.attributeHandles) {
      instance->second.ownershipAssumptionRecipientsByAttribute[attributeHandle].insert(
          recipient.receivingFederateId);
    }
    instance->second.pendingAttributeOwnershipAssumptionCallbacks.push_back({
        recipient.receivingFederateId,
        recipient.attributeHandles,
        userSuppliedTag,
    });
  }
  for (std::uint64_t const attributeHandle : attributeHandles) {
    // Keep an empty search record when no current recipient was eligible;
    // later discovery or publication events must still be able to continue
    // the required assumption search.
    instance->second.ownershipAssumptionRecipientsByAttribute.try_emplace(attributeHandle);
    instance->second.ownershipAssumptionUserSuppliedTagsByAttribute.insert_or_assign(
        attributeHandle,
        userSuppliedTag);
  }

  // IEEE 1516.1-2025 §7.2 makes the supplied attributes unowned immediately;
  // no accepting federate is required for this transition. Existing regular
  // acquisition work is then replanned from this new unowned state, while an
  // existing If Available reservation retains the callback it already owns.
  for (std::uint64_t const attributeHandle : attributeHandles) {
    instance->second.attributeOwnersByHandle.erase(attributeHandle);
    clearUpdateRegionAssociation(instance->second, attributeHandle);
    instance->second.attributeTransportationTypes.erase(attributeHandle);
    instance->second.attributeOrderTypes.erase(attributeHandle);
    for (auto pending = instance->second.pendingAttributeTransportationTypeChanges.begin();
         pending != instance->second.pendingAttributeTransportationTypeChanges.end();) {
      pending->second.attributeHandles.erase(attributeHandle);
      if (pending->second.attributeHandles.empty()) {
        pending = instance->second.pendingAttributeTransportationTypeChanges.erase(pending);
      } else {
        ++pending;
      }
    }
    // A successful unconditional divestiture ends any earlier negotiated
    // waiting state for the same attribute. Its queued confirmation callback
    // will recheck this missing state and become harmless.
    instance->second.pendingNegotiatedAttributeOwnershipDivestitures.erase(attributeHandle);
  }
  refreshRegionUsage(federation->second);
  result.acquisitionWorkItems = planPendingAttributeOwnershipAcquisitionWork(
      federation->second,
      instance->second);
  return result;
}

std::optional<AttributeOwnershipAssumptionDelivery>
EmbeddedFederationRegistry::attributeOwnershipAssumptionDeliveryFor(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> const& scheduledAttributeHandles) {
  if (scheduledAttributeHandles.empty()) {
    return std::nullopt;
  }

  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      !federation->second.members.contains(receivingFederateId)) {
    return std::nullopt;
  }
  auto const instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() || instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(receivingFederateId) ||
      !federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    return std::nullopt;
  }

  // Consume the exact queued callback identity before evaluating current
  // eligibility. This is the one-shot begin boundary: a stale callback is
  // still consumed, while a callback that has already entered user code is
  // absent from the durable pending ledger and will not be replayed.
  auto pendingCallback = std::find_if(
      instance->second.pendingAttributeOwnershipAssumptionCallbacks.begin(),
      instance->second.pendingAttributeOwnershipAssumptionCallbacks.end(),
      [&](PendingAttributeOwnershipAssumptionCallback const& callback) {
        return callback.receivingFederateId == receivingFederateId &&
            callback.attributeHandles == scheduledAttributeHandles;
      });
  if (pendingCallback !=
      instance->second.pendingAttributeOwnershipAssumptionCallbacks.end()) {
    instance->second.pendingAttributeOwnershipAssumptionCallbacks.erase(pendingCallback);
  }

  auto const receivingKnownClass = instance->second.knownObjectClassHandlesByFederate.find(
      receivingFederateId);
  auto const receivingKnownClassName = federation->second.objectClassHandles->nameFor(
      receivingKnownClass->second);
  if (!receivingKnownClassName ||
      federation->second.definition.catalog->objectClass(*receivingKnownClassName) == nullptr) {
    return std::nullopt;
  }
  auto const publishedAttributes = publishedObjectClassAttributes(
      federation->second,
      receivingFederateId,
      receivingKnownClass->second);

  auto releaseStaleCandidateReservations = [&] {
    // A candidate can become ineligible after the assumption work item is
    // queued (for example, by unpublishing before HLA_EVOKED dispatch).  The
    // callback is then suppressed, but that terminal outcome must not make a
    // later publication permanently invisible to the continuing search.
    // Keep the empty per-attribute search record: it still represents the
    // unowned disposition and lets a later discovery/publication re-plan it.
    for (std::uint64_t const attributeHandle : scheduledAttributeHandles) {
      auto search = instance->second.ownershipAssumptionRecipientsByAttribute.find(
          attributeHandle);
      if (search != instance->second.ownershipAssumptionRecipientsByAttribute.end()) {
        search->second.erase(receivingFederateId);
      }
    }
  };

  if (!publishedAttributes) {
    releaseStaleCandidateReservations();
    return std::nullopt;
  }

  AttributeOwnershipAssumptionDelivery delivery;
  delivery.objectInstanceHandle = objectInstanceHandle;
  for (std::uint64_t const attributeHandle : scheduledAttributeHandles) {
    auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
    auto const negotiated =
        instance->second.pendingNegotiatedAttributeOwnershipDivestitures.find(attributeHandle);
    bool const negotiatedOffer =
        owner != instance->second.attributeOwnersByHandle.end() &&
        negotiated != instance->second.pendingNegotiatedAttributeOwnershipDivestitures.end() &&
        negotiated->second.divestingFederateId == owner->second &&
        owner->second != receivingFederateId;
    if (!federation->second.attributeHandles->nameFor(
            federation->second.definition.catalog.get(),
            *receivingKnownClassName,
            attributeHandle) ||
        !publishedAttributes->contains(attributeHandle) ||
        (owner != instance->second.attributeOwnersByHandle.end() && !negotiatedOffer)) {
      auto search = instance->second.ownershipAssumptionRecipientsByAttribute.find(
          attributeHandle);
      if (search != instance->second.ownershipAssumptionRecipientsByAttribute.end()) {
        search->second.erase(receivingFederateId);
      }
      continue;
    }

    bool hasPendingAcquisition = false;
    for (auto const& [requestId, pending] :
         instance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests) {
      static_cast<void>(requestId);
      if (pending.requestingFederateId == receivingFederateId &&
          pending.desiredAttributeHandles.contains(attributeHandle)) {
        hasPendingAcquisition = true;
        break;
      }
    }
    if (!hasPendingAcquisition) {
      for (auto const& [requestId, pending] :
           instance->second.pendingAttributeOwnershipAcquisitionRequests) {
        static_cast<void>(requestId);
        if (pending.requestingFederateId == receivingFederateId &&
            pending.desiredAttributeHandles.contains(attributeHandle)) {
          hasPendingAcquisition = true;
          break;
        }
      }
    }
    if (!hasPendingAcquisition) {
      for (auto const& [cancellationId, cancellation] :
           instance->second.pendingAttributeOwnershipAcquisitionCancellations) {
        static_cast<void>(cancellationId);
        if (cancellation.requestingFederateId == receivingFederateId &&
            cancellation.attributeHandles.contains(attributeHandle)) {
          hasPendingAcquisition = true;
          break;
        }
      }
    }
    if (hasPendingAcquisition) {
      // The queued assumption is terminal for this attribute once the
      // candidate starts any standard acquisition path. Do not leave the
      // reservation behind when that acquisition later cancels or resolves;
      // the continuing unowned search must be able to reconsider it.
      auto search = instance->second.ownershipAssumptionRecipientsByAttribute.find(
          attributeHandle);
      if (search != instance->second.ownershipAssumptionRecipientsByAttribute.end()) {
        search->second.erase(receivingFederateId);
      }
    } else {
      delivery.attributeHandles.insert(attributeHandle);
    }
  }

  if (delivery.attributeHandles.empty()) {
    return std::nullopt;
  }
  return delivery;
}

std::vector<AttributeOwnershipAssumptionRecipient>
EmbeddedFederationRegistry::planAttributeOwnershipAssumptionsForFederateLocked(
    Federation& federation,
    std::uint64_t receivingFederateId,
    std::optional<std::uint64_t> objectInstanceFilter,
    std::set<std::uint64_t> const* attributeFilter) {
  if (!federation.members.contains(receivingFederateId) ||
      !federation.definition.catalog ||
      !federation.objectClassHandles ||
      !federation.attributeHandles) {
    return {};
  }

  auto const callbackRoute = federation.interactionCallbackRoutes.find(
      receivingFederateId);
  if (callbackRoute == federation.interactionCallbackRoutes.end() ||
      !callbackRoute->second) {
    return {};
  }

  auto recipientHasPendingAcquisition = [](
                                           Federation::ObjectInstance const& instance,
                                           std::uint64_t federateId,
                                           std::uint64_t attributeHandle) {
    for (auto const& [requestId, pending] :
         instance.pendingAttributeOwnershipAcquisitionIfAvailableRequests) {
      static_cast<void>(requestId);
      if (pending.requestingFederateId == federateId &&
          pending.desiredAttributeHandles.contains(attributeHandle)) {
        return true;
      }
    }
    for (auto const& [requestId, pending] :
         instance.pendingAttributeOwnershipAcquisitionRequests) {
      static_cast<void>(requestId);
      if (pending.requestingFederateId == federateId &&
          pending.desiredAttributeHandles.contains(attributeHandle)) {
        return true;
      }
    }
    for (auto const& [cancellationId, cancellation] :
         instance.pendingAttributeOwnershipAcquisitionCancellations) {
      static_cast<void>(cancellationId);
      if (cancellation.requestingFederateId == federateId &&
          cancellation.attributeHandles.contains(attributeHandle)) {
        return true;
      }
    }
    return false;
  };

  std::map<std::uint64_t,
           std::map<std::vector<unsigned char>, std::set<std::uint64_t>>>
      offeredAttributesByObjectAndTag;
  for (auto& [objectInstanceHandle, instance] : federation.objectInstances) {
    if (objectInstanceFilter && objectInstanceHandle != *objectInstanceFilter) {
      continue;
    }
    if (instance.deleteAccepted) {
      continue;
    }
    auto const knownClass = instance.knownObjectClassHandlesByFederate.find(
        receivingFederateId);
    if (knownClass == instance.knownObjectClassHandlesByFederate.end()) {
      continue;
    }
    auto const knownClassName = federation.objectClassHandles->nameFor(
        knownClass->second);
    if (!knownClassName ||
        federation.definition.catalog->objectClass(*knownClassName) == nullptr) {
      continue;
    }
    auto const publishedAttributes = publishedObjectClassAttributes(
        federation,
        receivingFederateId,
        knownClass->second);
    if (!publishedAttributes) {
      continue;
    }

    for (auto search = instance.ownershipAssumptionRecipientsByAttribute.begin();
         search != instance.ownershipAssumptionRecipientsByAttribute.end();) {
      std::uint64_t const attributeHandle = search->first;
      if (attributeFilter && !attributeFilter->contains(attributeHandle)) {
        ++search;
        continue;
      }
      auto const owner = instance.attributeOwnersByHandle.find(attributeHandle);
      if (owner != instance.attributeOwnersByHandle.end()) {
        auto const negotiated =
            instance.pendingNegotiatedAttributeOwnershipDivestitures.find(attributeHandle);
        bool const negotiatedOffer =
            negotiated != instance.pendingNegotiatedAttributeOwnershipDivestitures.end() &&
            negotiated->second.divestingFederateId == owner->second;
        // A negotiated offer is the only ownership-assumption path that may
        // retain an owner while the callback search is active. Never offer it
        // back to the divesting owner itself; that owner is already the
        // authoritative source of the pending transfer.
        if (!negotiatedOffer || owner->second == receivingFederateId) {
          if (!negotiatedOffer) {
            instance.ownershipAssumptionUserSuppliedTagsByAttribute.erase(attributeHandle);
            search = instance.ownershipAssumptionRecipientsByAttribute.erase(search);
          } else {
            ++search;
          }
          continue;
        }
      }
      if (search->second.contains(receivingFederateId) ||
          !federation.attributeHandles->nameFor(
              federation.definition.catalog.get(),
              *knownClassName,
              attributeHandle) ||
          !publishedAttributes->contains(attributeHandle) ||
          recipientHasPendingAcquisition(instance, receivingFederateId, attributeHandle)) {
        ++search;
        continue;
      }

      // Reserve the tuple before returning it.  The callback-entry recheck
      // still suppresses a now-stale offer, but a repeated declaration event
      // cannot enqueue another copy while this one is outstanding.
      search->second.insert(receivingFederateId);
      std::vector<unsigned char> userSuppliedTag;
      auto const tag = instance.ownershipAssumptionUserSuppliedTagsByAttribute.find(
          attributeHandle);
      if (tag != instance.ownershipAssumptionUserSuppliedTagsByAttribute.end()) {
        userSuppliedTag = tag->second;
      }
      offeredAttributesByObjectAndTag[objectInstanceHandle][std::move(userSuppliedTag)].insert(
          attributeHandle);
      ++search;
    }
  }

  std::vector<AttributeOwnershipAssumptionRecipient> result;
  for (auto& [objectInstanceHandle, attributesByTag] : offeredAttributesByObjectAndTag) {
    result.reserve(result.size() + attributesByTag.size());
    for (auto& [userSuppliedTag, attributeHandles] : attributesByTag) {
      result.push_back({
          receivingFederateId,
          objectInstanceHandle,
          std::move(attributeHandles),
          callbackRoute->second,
          std::move(userSuppliedTag),
      });
      auto const object = federation.objectInstances.find(objectInstanceHandle);
      if (object != federation.objectInstances.end()) {
        auto const& queued = result.back();
        object->second.pendingAttributeOwnershipAssumptionCallbacks.push_back({
            queued.receivingFederateId,
            queued.attributeHandles,
            queued.userSuppliedTag,
        });
      }
    }
  }
  return result;
}

std::vector<AttributeOwnershipAssumptionRecipient>
EmbeddedFederationRegistry::planAttributeOwnershipAssumptionsForFederate(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::optional<std::uint64_t> objectInstanceFilter,
    std::set<std::uint64_t> const* attributeFilter) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {};
  }
  return planAttributeOwnershipAssumptionsForFederateLocked(
      federation->second,
      receivingFederateId,
      objectInstanceFilter,
      attributeFilter);
}


}  // namespace umbra::detail
