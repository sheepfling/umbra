#include "internal/federation/federation_registry.hpp"
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
AttributeOwnershipQueryPlan EmbeddedFederationRegistry::planAttributeOwnershipQuery(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> const& requestedAttributeHandles) {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {AttributeOwnershipQueryStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(requestingFederateId)) {
    return {AttributeOwnershipQueryStatus::requesting_federate_not_member};
  }
  auto persistRecipient =
      [&federation](AttributeOwnershipQueryRecipient recipient)
      -> std::optional<AttributeOwnershipQueryRecipient> {
    auto& execution = federation->second;
    if (execution.nextAttributeOwnershipQueryRequestId == 0U ||
        execution.nextAttributeOwnershipQueryRequestId ==
            std::numeric_limits<std::uint64_t>::max()) {
      return std::nullopt;
    }
    auto const requestId = execution.nextAttributeOwnershipQueryRequestId;
    Federation::PendingAttributeOwnershipQuery pending;
    pending.requestingFederateId = recipient.receivingFederateId;
    pending.objectInstanceHandle = recipient.objectInstanceHandle;
    pending.reportKind = recipient.reportKind;
    pending.owningFederateId = recipient.owningFederateId;
    pending.requestedAttributeHandles = recipient.attributeHandles;
    auto const [position, inserted] =
        execution.pendingAttributeOwnershipQueries.emplace(
            requestId,
            std::move(pending));
    static_cast<void>(position);
    if (!inserted) {
      return std::nullopt;
    }
    ++execution.nextAttributeOwnershipQueryRequestId;
    recipient.requestId = requestId;
    return recipient;
  };
  // RTI-owned joined-federate MOM instances are not federate-created object
  // instances. Their ownership answer is nevertheless a normal standard
  // Query Attribute Ownership result, delivered to the requesting federate
  // through its callback route.
  auto const momObject = federation->second.rtiOwnedJoinedFederateMomObjects.find(
      objectInstanceHandle);
  if (momObject != federation->second.rtiOwnedJoinedFederateMomObjects.end()) {
    if (!momObject->second.knownFederateIds.contains(requestingFederateId)) {
      return {AttributeOwnershipQueryStatus::object_instance_not_known};
    }
    if (!federation->second.definition.catalog ||
        !federation->second.objectClassHandles ||
        !federation->second.attributeHandles) {
      return {AttributeOwnershipQueryStatus::inconsistent_catalog};
    }
    auto const objectClassName = federation->second.objectClassHandles->nameFor(
        momObject->second.objectClassHandle);
    if (!objectClassName ||
        federation->second.definition.catalog->objectClass(*objectClassName) == nullptr) {
      return {AttributeOwnershipQueryStatus::inconsistent_catalog};
    }
    for (std::uint64_t const attributeHandle : requestedAttributeHandles) {
      if (!momObject->second.effectiveAttributeHandles.contains(attributeHandle) ||
          !federation->second.attributeHandles->nameFor(
              federation->second.definition.catalog.get(),
              *objectClassName,
              attributeHandle)) {
        return {AttributeOwnershipQueryStatus::attribute_not_defined};
      }
    }
    AttributeOwnershipQueryPlan result;
    auto recipient = candidateAttributeOwnershipQueryRecipient(
        federation->second,
        requestingFederateId,
        objectInstanceHandle,
        AttributeOwnershipQueryReportKind::rti,
        0,
        requestedAttributeHandles);
    if (recipient) {
      auto persisted = persistRecipient(std::move(*recipient));
      if (!persisted) {
        result.status = AttributeOwnershipQueryStatus::inconsistent_catalog;
        return result;
      }
      result.recipients.push_back(std::move(*persisted));
    }
    return result;
  }
  auto const instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() || instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(requestingFederateId)) {
    return {AttributeOwnershipQueryStatus::object_instance_not_known};
  }
  if (!federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    return {AttributeOwnershipQueryStatus::inconsistent_catalog};
  }

  auto const knownClass = instance->second.knownObjectClassHandlesByFederate.find(
      requestingFederateId);
  auto const knownClassName = federation->second.objectClassHandles->nameFor(knownClass->second);
  if (!knownClassName ||
      federation->second.definition.catalog->objectClass(*knownClassName) == nullptr) {
    return {AttributeOwnershipQueryStatus::inconsistent_catalog};
  }
  for (std::uint64_t const attributeHandle : requestedAttributeHandles) {
    if (!federation->second.attributeHandles->nameFor(
            federation->second.definition.catalog.get(),
            *knownClassName,
            attributeHandle)) {
      return {AttributeOwnershipQueryStatus::attribute_not_defined};
    }
  }

  std::map<std::uint64_t, std::set<std::uint64_t>> attributesByOwner;
  std::set<std::uint64_t> unownedAttributes;
  for (std::uint64_t const attributeHandle : requestedAttributeHandles) {
    auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
    if (owner == instance->second.attributeOwnersByHandle.end()) {
      unownedAttributes.insert(attributeHandle);
      continue;
    }
    // The present runtime has no standard ownership-resignation lifecycle
    // slice. A retained owner ID without a joined federate cannot be reported
    // as either a federate or an unowned attribute without falsifying 2025
    // ownership semantics, so make that incomplete state explicit.
    if (!federation->second.members.contains(owner->second)) {
      return {AttributeOwnershipQueryStatus::inconsistent_catalog};
    }
    attributesByOwner[owner->second].insert(attributeHandle);
  }

  AttributeOwnershipQueryPlan result;
  result.recipients.reserve(attributesByOwner.size() + (unownedAttributes.empty() ? 0U : 1U));
  for (auto const& [owningFederateId, ownedAttributeHandles] : attributesByOwner) {
    auto recipient = candidateAttributeOwnershipQueryRecipient(
        federation->second,
        requestingFederateId,
        objectInstanceHandle,
        AttributeOwnershipQueryReportKind::federate,
        owningFederateId,
        ownedAttributeHandles);
    if (recipient) {
      auto persisted = persistRecipient(std::move(*recipient));
      if (!persisted) {
        result.status = AttributeOwnershipQueryStatus::inconsistent_catalog;
        return result;
      }
      result.recipients.push_back(std::move(*persisted));
    }
  }
  if (!unownedAttributes.empty()) {
    auto recipient = candidateAttributeOwnershipQueryRecipient(
        federation->second,
        requestingFederateId,
        objectInstanceHandle,
        AttributeOwnershipQueryReportKind::unowned,
        0,
        unownedAttributes);
    if (recipient) {
      auto persisted = persistRecipient(std::move(*recipient));
      if (!persisted) {
        result.status = AttributeOwnershipQueryStatus::inconsistent_catalog;
        return result;
      }
      result.recipients.push_back(std::move(*persisted));
    }
  }
  return result;
}

std::optional<AttributeOwnershipQueryRecipient>
EmbeddedFederationRegistry::attributeOwnershipQueryRecipientFor(
    std::wstring const& federationName,
    std::uint64_t requestId,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    AttributeOwnershipQueryReportKind reportKind,
    std::uint64_t owningFederateId,
    std::set<std::uint64_t> const& requestedAttributeHandles) {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }
  auto pending = federation->second.pendingAttributeOwnershipQueries.find(requestId);
  if (pending == federation->second.pendingAttributeOwnershipQueries.end()) {
    return std::nullopt;
  }
  auto const pendingRequest = pending->second;
  federation->second.pendingAttributeOwnershipQueries.erase(pending);
  if (pendingRequest.requestingFederateId != requestingFederateId ||
      pendingRequest.objectInstanceHandle != objectInstanceHandle ||
      pendingRequest.reportKind != reportKind ||
      pendingRequest.owningFederateId != owningFederateId ||
      pendingRequest.requestedAttributeHandles != requestedAttributeHandles) {
    return std::nullopt;
  }
  auto result = candidateAttributeOwnershipQueryRecipient(
      federation->second,
      pendingRequest.requestingFederateId,
      pendingRequest.objectInstanceHandle,
      pendingRequest.reportKind,
      pendingRequest.owningFederateId,
      pendingRequest.requestedAttributeHandles);
  if (result) {
    result->requestId = requestId;
  }
  return result;
}

AttributeOwnershipCheckResult EmbeddedFederationRegistry::attributeOwnedByFederate(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t attributeHandle) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {AttributeOwnershipCheckStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(requestingFederateId)) {
    return {AttributeOwnershipCheckStatus::requesting_federate_not_member};
  }
  // A discovered RTI-owned MOM object is known to the requesting federate
  // through the separate MOM ledger. Its attributes are valid object-class
  // attributes but none are owned by the invoking federate, so answer the
  // standard boolean query directly instead of reporting ObjectInstanceNotKnown.
  auto const momObject = federation->second.rtiOwnedJoinedFederateMomObjects.find(
      objectInstanceHandle);
  if (momObject != federation->second.rtiOwnedJoinedFederateMomObjects.end()) {
    if (!momObject->second.knownFederateIds.contains(requestingFederateId)) {
      return {AttributeOwnershipCheckStatus::object_instance_not_known};
    }
    if (!federation->second.definition.catalog ||
        !federation->second.objectClassHandles ||
        !federation->second.attributeHandles) {
      return {AttributeOwnershipCheckStatus::inconsistent_catalog};
    }
    auto const objectClassName = federation->second.objectClassHandles->nameFor(
        momObject->second.objectClassHandle);
    if (!objectClassName ||
        federation->second.definition.catalog->objectClass(*objectClassName) == nullptr) {
      return {AttributeOwnershipCheckStatus::inconsistent_catalog};
    }
    if (!momObject->second.effectiveAttributeHandles.contains(attributeHandle) ||
        !federation->second.attributeHandles->nameFor(
            federation->second.definition.catalog.get(),
            *objectClassName,
            attributeHandle)) {
      return {AttributeOwnershipCheckStatus::attribute_not_defined};
    }
    return {AttributeOwnershipCheckStatus::applied, false};
  }
  auto const instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() || instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(requestingFederateId)) {
    return {AttributeOwnershipCheckStatus::object_instance_not_known};
  }
  if (!federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    return {AttributeOwnershipCheckStatus::inconsistent_catalog};
  }

  auto const knownClass = instance->second.knownObjectClassHandlesByFederate.find(
      requestingFederateId);
  auto const knownClassName = federation->second.objectClassHandles->nameFor(knownClass->second);
  if (!knownClassName ||
      federation->second.definition.catalog->objectClass(*knownClassName) == nullptr) {
    return {AttributeOwnershipCheckStatus::inconsistent_catalog};
  }
  if (!federation->second.attributeHandles->nameFor(
          federation->second.definition.catalog.get(),
          *knownClassName,
          attributeHandle)) {
    return {AttributeOwnershipCheckStatus::attribute_not_defined};
  }

  auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
  return {
      AttributeOwnershipCheckStatus::applied,
      owner != instance->second.attributeOwnersByHandle.end() &&
          owner->second == requestingFederateId,
  };
}

AttributeOwnershipAcquisitionIfAvailablePlan
EmbeddedFederationRegistry::planAttributeOwnershipAcquisitionIfAvailable(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> const& desiredAttributeHandles,
    std::vector<unsigned char> userSuppliedTag) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {AttributeOwnershipAcquisitionIfAvailableStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(requestingFederateId)) {
    return {AttributeOwnershipAcquisitionIfAvailableStatus::requesting_federate_not_member};
  }
  // IEEE 1516.1-2025 11.4.2(k) prohibits ownership transfer of predefined
  // RTI-owned MOM attributes. Keep the MOM ledger separate from application
  // objectInstances so this boundary cannot be misreported as unknown.
  if (federation->second.rtiOwnedJoinedFederateMomObjects.contains(objectInstanceHandle)) {
    return {AttributeOwnershipAcquisitionIfAvailableStatus::attribute_owned_by_rti};
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() || instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(requestingFederateId)) {
    return {AttributeOwnershipAcquisitionIfAvailableStatus::object_instance_not_known};
  }
  if (!federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    return {AttributeOwnershipAcquisitionIfAvailableStatus::inconsistent_catalog};
  }

  auto const knownClass = instance->second.knownObjectClassHandlesByFederate.find(
      requestingFederateId);
  auto const knownClassName = federation->second.objectClassHandles->nameFor(knownClass->second);
  if (!knownClassName ||
      federation->second.definition.catalog->objectClass(*knownClassName) == nullptr) {
    return {AttributeOwnershipAcquisitionIfAvailableStatus::inconsistent_catalog};
  }
  for (std::uint64_t const attributeHandle : desiredAttributeHandles) {
    if (!federation->second.attributeHandles->nameFor(
            federation->second.definition.catalog.get(),
            *knownClassName,
            attributeHandle)) {
      return {AttributeOwnershipAcquisitionIfAvailableStatus::attribute_not_defined};
    }
  }

  // An empty request changes neither ownership nor the private ownership
  // state chart. Its object/federate preconditions above still apply.
  if (desiredAttributeHandles.empty()) {
    return {};
  }

  auto const publishedAttributes = publishedObjectClassAttributes(
      federation->second,
      requestingFederateId,
      knownClass->second);
  if (!publishedAttributes) {
    return {AttributeOwnershipAcquisitionIfAvailableStatus::inconsistent_catalog};
  }
  if (publishedAttributes->empty()) {
    return {AttributeOwnershipAcquisitionIfAvailableStatus::object_class_not_published};
  }
  for (std::uint64_t const attributeHandle : desiredAttributeHandles) {
    if (!publishedAttributes->contains(attributeHandle)) {
      return {AttributeOwnershipAcquisitionIfAvailableStatus::attribute_not_published};
    }
  }

  for (std::uint64_t const attributeHandle : desiredAttributeHandles) {
    auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
    if (owner == instance->second.attributeOwnersByHandle.end()) {
      continue;
    }
    if (owner->second == requestingFederateId) {
      return {AttributeOwnershipAcquisitionIfAvailableStatus::federate_owns_attributes};
    }
    if (!federation->second.members.contains(owner->second)) {
      // A completed resignation must not leave a stale owner record. Treat a
      // surviving record as an internal catalog inconsistency rather than
      // manufacturing an acquisition outcome from an invalid state.
      return {AttributeOwnershipAcquisitionIfAvailableStatus::inconsistent_catalog};
    }
  }

  // IEEE 1516.1-2025 7.1.2.2 prohibits entering Willing to Acquire after
  // this federate has already entered regular Acquisition Pending for the
  // same attribute.  The official C++ binding exposes that condition as
  // AttributeAlreadyBeingAcquired.
  for (auto const& [requestId, pending] :
       instance->second.pendingAttributeOwnershipAcquisitionRequests) {
    static_cast<void>(requestId);
    if (pending.requestingFederateId != requestingFederateId) {
      continue;
    }
    for (std::uint64_t const attributeHandle : desiredAttributeHandles) {
      if (pending.desiredAttributeHandles.contains(attributeHandle)) {
        return {AttributeOwnershipAcquisitionIfAvailableStatus::attribute_already_being_acquired};
      }
    }
  }
  for (auto const& [cancellationId, cancellation] :
       instance->second.pendingAttributeOwnershipAcquisitionCancellations) {
    static_cast<void>(cancellationId);
    if (cancellation.requestingFederateId != requestingFederateId) {
      continue;
    }
    for (std::uint64_t const attributeHandle : desiredAttributeHandles) {
      if (cancellation.attributeHandles.contains(attributeHandle)) {
        return {AttributeOwnershipAcquisitionIfAvailableStatus::attribute_already_being_acquired};
      }
    }
  }

  std::set<std::uint64_t> newlyRequestedAttributeHandles = desiredAttributeHandles;
  for (auto const& [requestId, pending] :
       instance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests) {
    static_cast<void>(requestId);
    if (pending.requestingFederateId == requestingFederateId) {
      for (std::uint64_t const attributeHandle : pending.desiredAttributeHandles) {
        newlyRequestedAttributeHandles.erase(attributeHandle);
      }
    }
  }

  // IEEE 1516.1-2025 7.9 leaves an attribute unchanged when this federate
  // invokes If Available while it is already Willing to Acquire.  It is not
  // the AttributeAlreadyBeingAcquired exception, which applies to a pending
  // regular Attribute Ownership Acquisition request.  Do not manufacture a
  // second reservation or terminal callback for the existing WTA attribute.
  if (newlyRequestedAttributeHandles.empty()) {
    return {};
  }

  if (federation->second.nextAttributeOwnershipAcquisitionIfAvailableRequestId == 0 ||
      federation->second.nextAttributeOwnershipAcquisitionIfAvailableRequestId ==
          std::numeric_limits<std::uint64_t>::max() ||
      federation->second.nextAttributeOwnershipAcquisitionRequestSequence == 0 ||
      federation->second.nextAttributeOwnershipAcquisitionRequestSequence ==
          std::numeric_limits<std::uint64_t>::max()) {
    return {AttributeOwnershipAcquisitionIfAvailableStatus::inconsistent_catalog};
  }
  auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
      requestingFederateId);
  if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
      !callbackRoute->second) {
    return {AttributeOwnershipAcquisitionIfAvailableStatus::inconsistent_catalog};
  }

  std::uint64_t const requestId =
      federation->second.nextAttributeOwnershipAcquisitionIfAvailableRequestId;
  std::uint64_t const requestSequence =
      federation->second.nextAttributeOwnershipAcquisitionRequestSequence;
  auto const [pending, inserted] =
      instance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests.try_emplace(
          requestId,
          Federation::ObjectInstance::PendingAttributeOwnershipAcquisitionIfAvailable{
              requestingFederateId,
              requestSequence,
              std::move(newlyRequestedAttributeHandles),
              std::move(userSuppliedTag),
          });
  static_cast<void>(pending);
  if (!inserted) {
    return {AttributeOwnershipAcquisitionIfAvailableStatus::inconsistent_catalog};
  }
  ++federation->second.nextAttributeOwnershipAcquisitionIfAvailableRequestId;
  ++federation->second.nextAttributeOwnershipAcquisitionRequestSequence;
  return {
      AttributeOwnershipAcquisitionIfAvailableStatus::applied,
      requestId,
      callbackRoute->second,
  };
}

std::optional<AttributeOwnershipAcquisitionIfAvailableDelivery>
EmbeddedFederationRegistry::beginAttributeOwnershipAcquisitionIfAvailable(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t requestId) {
  if (requestId == 0) {
    return std::nullopt;
  }

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end()) {
    return std::nullopt;
  }
  auto pending = instance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests.find(
      requestId);
  if (pending == instance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests.end()) {
    return std::nullopt;
  }

  auto clearPending = [&] {
    instance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests.erase(pending);
  };
  if (pending->second.requestingFederateId != requestingFederateId ||
      instance->second.deleteAccepted ||
      !federation->second.members.contains(requestingFederateId) ||
      !instance->second.knownObjectClassHandlesByFederate.contains(requestingFederateId) ||
      !federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    clearPending();
    return std::nullopt;
  }

  auto const knownClass = instance->second.knownObjectClassHandlesByFederate.find(
      requestingFederateId);
  auto const knownClassName = federation->second.objectClassHandles->nameFor(knownClass->second);
  if (!knownClassName ||
      federation->second.definition.catalog->objectClass(*knownClassName) == nullptr) {
    clearPending();
    return std::nullopt;
  }
  auto const publishedAttributes = publishedObjectClassAttributes(
      federation->second,
      requestingFederateId,
      knownClass->second);
  if (!publishedAttributes) {
    clearPending();
    return std::nullopt;
  }

  // An If Available callback is normally the one-shot boundary that resolves
  // its willing-to-acquire reservation.  When a negotiated divestiture has
  // selected this reservation as its candidate, however, that ordinary
  // callback must not consume the request (or report unavailable) before the
  // owner receives and confirms Request Divestiture Confirmation.  The
  // negotiated confirmation owns the transfer boundary; leave this durable
  // request in place until Confirm Divestiture removes it.
  for (auto const& [attributeHandle, divestiture] :
       instance->second.pendingNegotiatedAttributeOwnershipDivestitures) {
    if (!divestiture.acquiringFederateIsIfAvailable ||
        divestiture.acquiringFederateId != requestingFederateId ||
        divestiture.acquisitionRequestId != requestId ||
        (!divestiture.confirmationQueued && !divestiture.confirmationDelivered) ||
        !pending->second.desiredAttributeHandles.contains(attributeHandle)) {
      continue;
    }
    return std::nullopt;
  }

  AttributeOwnershipAcquisitionIfAvailableDelivery delivery;
  delivery.objectInstanceHandle = objectInstanceHandle;
  auto revisedAttributeOwners = instance->second.attributeOwnersByHandle;
  for (std::uint64_t const attributeHandle : pending->second.desiredAttributeHandles) {
    if (!federation->second.attributeHandles->nameFor(
            federation->second.definition.catalog.get(),
            *knownClassName,
            attributeHandle) ||
        !publishedAttributes->contains(attributeHandle)) {
      // A federate must keep publishing until its terminal callback. If it
      // does not, do not invoke a callback whose standard preconditions are
      // no longer true.
      clearPending();
      return std::nullopt;
    }

    auto const owner = revisedAttributeOwners.find(attributeHandle);
    if (owner == revisedAttributeOwners.end()) {
      delivery.securedAttributeHandles.insert(attributeHandle);
      revisedAttributeOwners.emplace(attributeHandle, requestingFederateId);
      continue;
    }
    if (owner->second == requestingFederateId) {
      // This bounded runtime has no other transfer operation that can reach
      // this branch. If a future ownership transition did so first, the
      // matching terminal notification remains the truthful response.
      delivery.securedAttributeHandles.insert(attributeHandle);
      continue;
    }
    if (!federation->second.members.contains(owner->second)) {
      clearPending();
      return std::nullopt;
    }
    delivery.unavailableAttributeHandles.insert(attributeHandle);
  }

  if (delivery.securedAttributeHandles.empty() && delivery.unavailableAttributeHandles.empty()) {
    clearPending();
    return std::nullopt;
  }

  // Ownership becomes visible immediately before its matching standard
  // callback, never at request acceptance. This preserves the pending
  // Willing to Acquire state for an evoked callback model.
  instance->second.attributeOwnersByHandle.swap(revisedAttributeOwners);
  for (std::uint64_t const attributeHandle : delivery.securedAttributeHandles) {
    clearOwnershipAssumptionSearch(instance->second, attributeHandle);
    // The previous owner's non-default update-region association is not
    // inherited by the acquirer. A region association that the acquirer
    // established while it was not owner is promoted at this ownership
    // boundary instead.
    clearUpdateRegionAssociation(instance->second, attributeHandle);
    promoteDeferredUpdateRegionAssociation(
        instance->second,
        requestingFederateId,
        attributeHandle);
    auto const transportationName = attributeDefaultTransportationName(
        federation->second,
        requestingFederateId,
        knownClass->second,
        attributeHandle);
    if (transportationName) {
      instance->second.attributeTransportationTypes.insert_or_assign(
        attributeHandle,
        *transportationName);
    }
    auto const orderType = attributeDefaultOrderType(
        federation->second,
        requestingFederateId,
        knownClass->second,
        attributeHandle);
    if (orderType) {
      instance->second.attributeOrderTypes.insert_or_assign(attributeHandle, *orderType);
    } else {
      instance->second.attributeOrderTypes.erase(attributeHandle);
    }
  }
  refreshRegionUsage(federation->second);
  clearPending();
  return delivery;
}

void EmbeddedFederationRegistry::cancelAttributeOwnershipAcquisitionIfAvailable(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t requestId) {
  if (requestId == 0) {
    return;
  }

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return;
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end()) {
    return;
  }
  auto pending = instance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests.find(
      requestId);
  if (pending != instance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests.end() &&
      pending->second.requestingFederateId == requestingFederateId) {
    instance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests.erase(pending);
  }
}

}  // namespace umbra::detail
