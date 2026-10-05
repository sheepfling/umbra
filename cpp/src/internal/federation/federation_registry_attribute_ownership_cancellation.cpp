#include "internal/federation/federation_registry.hpp"
#include "internal/fom/fom_catalog.hpp"

#include <cstdint>
#include <limits>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace umbra::detail {
AttributeOwnershipAcquisitionCancellationPlan
EmbeddedFederationRegistry::planAttributeOwnershipAcquisitionCancellation(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> const& attributeHandles) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {AttributeOwnershipAcquisitionCancellationStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(requestingFederateId)) {
    return {AttributeOwnershipAcquisitionCancellationStatus::requesting_federate_not_member};
  }
  // IEEE 1516.1-2025 11.4.2(k) prohibits ownership transfer of predefined
  // RTI-owned MOM attributes.
  if (federation->second.rtiOwnedJoinedFederateMomObjects.contains(objectInstanceHandle)) {
    return {AttributeOwnershipAcquisitionCancellationStatus::attribute_owned_by_rti};
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() || instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(requestingFederateId)) {
    return {AttributeOwnershipAcquisitionCancellationStatus::object_instance_not_known};
  }
  if (!federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    return {AttributeOwnershipAcquisitionCancellationStatus::inconsistent_catalog};
  }

  auto const knownClass = instance->second.knownObjectClassHandlesByFederate.find(
      requestingFederateId);
  auto const knownClassName = federation->second.objectClassHandles->nameFor(knownClass->second);
  if (!knownClassName ||
      federation->second.definition.catalog->objectClass(*knownClassName) == nullptr) {
    return {AttributeOwnershipAcquisitionCancellationStatus::inconsistent_catalog};
  }
  for (std::uint64_t const attributeHandle : attributeHandles) {
    if (!federation->second.attributeHandles->nameFor(
            federation->second.definition.catalog.get(),
            *knownClassName,
            attributeHandle)) {
      return {AttributeOwnershipAcquisitionCancellationStatus::attribute_not_defined};
    }
  }

  // Empty attribute sets have no cancellation transition or callback, while
  // still honoring the connection, federation, and known-instance checks.
  if (attributeHandles.empty()) {
    return {};
  }

  for (std::uint64_t const attributeHandle : attributeHandles) {
    auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
    if (owner != instance->second.attributeOwnersByHandle.end() &&
        owner->second == requestingFederateId) {
      return {AttributeOwnershipAcquisitionCancellationStatus::attribute_already_owned};
    }

    bool acquisitionWasRequested = false;
    for (auto const& [requestId, pending] :
         instance->second.pendingAttributeOwnershipAcquisitionRequests) {
      static_cast<void>(requestId);
      if (pending.requestingFederateId == requestingFederateId &&
          pending.desiredAttributeHandles.contains(attributeHandle)) {
        acquisitionWasRequested = true;
        break;
      }
    }
    if (!acquisitionWasRequested) {
      return {
          AttributeOwnershipAcquisitionCancellationStatus::attribute_acquisition_was_not_requested};
    }
  }

  if (federation->second.nextAttributeOwnershipAcquisitionCancellationId == 0 ||
      federation->second.nextAttributeOwnershipAcquisitionCancellationId ==
          std::numeric_limits<std::uint64_t>::max()) {
    return {AttributeOwnershipAcquisitionCancellationStatus::inconsistent_catalog};
  }
  auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
      requestingFederateId);
  if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
      !callbackRoute->second) {
    return {AttributeOwnershipAcquisitionCancellationStatus::inconsistent_catalog};
  }

  // Complete all potentially allocating copies before mutating the pending
  // acquisition state. A failed allocation must not leave a cancellation
  // reservation without the callback payload that makes it terminal.
  std::set<std::uint64_t> cancellationAttributes = attributeHandles;
  ObjectInstanceCallbackRoute cancellationCallbackRoute = callbackRoute->second;

  std::uint64_t const cancellationId =
      federation->second.nextAttributeOwnershipAcquisitionCancellationId;
  auto const [cancellation, inserted] =
      instance->second.pendingAttributeOwnershipAcquisitionCancellations.try_emplace(
          cancellationId,
          Federation::ObjectInstance::PendingAttributeOwnershipAcquisitionCancellation{
              requestingFederateId,
              cancellationAttributes,
          });
  static_cast<void>(cancellation);
  if (!inserted) {
    return {AttributeOwnershipAcquisitionCancellationStatus::inconsistent_catalog};
  }
  ++federation->second.nextAttributeOwnershipAcquisitionCancellationId;

  // The cancellation is accepted before its callback executes. New work is
  // suppressed by the cancellation reservation, while an owner-side release
  // callback that has already entered user code may still answer with the
  // required Attribute Ownership Unavailable race outcome.

  return {
      AttributeOwnershipAcquisitionCancellationStatus::applied,
      cancellationId,
      std::move(cancellationAttributes),
      std::move(cancellationCallbackRoute),
  };
}

std::optional<AttributeOwnershipAcquisitionCancellationDelivery>
EmbeddedFederationRegistry::beginAttributeOwnershipAcquisitionCancellation(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t cancellationId,
    std::set<std::uint64_t> const& scheduledAttributeHandles) {
  if (cancellationId == 0 || scheduledAttributeHandles.empty()) {
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
  auto cancellation = instance->second.pendingAttributeOwnershipAcquisitionCancellations.find(
      cancellationId);
  if (cancellation == instance->second.pendingAttributeOwnershipAcquisitionCancellations.end() ||
      cancellation->second.requestingFederateId != requestingFederateId ||
      cancellation->second.attributeHandles != scheduledAttributeHandles) {
    return std::nullopt;
  }

  auto consumeCancellationAndPlanFollowup = [&] {
    auto confirmedAttributeHandles = cancellation->second.attributeHandles;

    for (auto pending = instance->second.pendingAttributeOwnershipAcquisitionRequests.begin();
         pending != instance->second.pendingAttributeOwnershipAcquisitionRequests.end();) {
      for (std::uint64_t const attributeHandle : confirmedAttributeHandles) {
        pending->second.desiredAttributeHandles.erase(attributeHandle);
        pending->second.notificationQueuedAttributeHandles.erase(attributeHandle);
        pending->second.unavailableQueuedAttributeHandles.erase(attributeHandle);
        for (auto release = pending->second.releaseCallbacksQueuedByOwningFederate.begin();
             release != pending->second.releaseCallbacksQueuedByOwningFederate.end();) {
          release->second.erase(attributeHandle);
          if (release->second.empty()) {
            release = pending->second.releaseCallbacksQueuedByOwningFederate.erase(release);
          } else {
            ++release;
          }
        }
      }
      if (pending->second.desiredAttributeHandles.empty()) {
        pending = instance->second.pendingAttributeOwnershipAcquisitionRequests.erase(pending);
      } else {
        ++pending;
      }
    }
    instance->second.pendingAttributeOwnershipAcquisitionCancellations.erase(cancellation);
    return std::pair{
        std::move(confirmedAttributeHandles),
        planPendingAttributeOwnershipAcquisitionWork(federation->second, instance->second),
    };
  };
  if (instance->second.deleteAccepted ||
      !federation->second.members.contains(requestingFederateId) ||
      !instance->second.knownObjectClassHandlesByFederate.contains(requestingFederateId)) {
    auto [ignoredConfirmedAttributeHandles, followupWorkItems] =
        consumeCancellationAndPlanFollowup();
    static_cast<void>(ignoredConfirmedAttributeHandles);
    if (followupWorkItems.empty()) {
      return std::nullopt;
    }
    return AttributeOwnershipAcquisitionCancellationDelivery{
        objectInstanceHandle,
        {},
        std::move(followupWorkItems),
    };
  }

  auto [confirmedAttributeHandles, followupWorkItems] =
      consumeCancellationAndPlanFollowup();
  return AttributeOwnershipAcquisitionCancellationDelivery{
      objectInstanceHandle,
      std::move(confirmedAttributeHandles),
      std::move(followupWorkItems),
  };
}

std::optional<AttributeOwnershipUnavailableRecipient>
EmbeddedFederationRegistry::attributeOwnershipUnavailableRecipientFor(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> const& attributeHandles) {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      !federation->second.members.contains(receivingFederateId)) {
    return std::nullopt;
  }
  auto const instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() || instance->second.deleteAccepted) {
    return std::nullopt;
  }
  auto const knownClass = instance->second.knownObjectClassHandlesByFederate.find(
      receivingFederateId);
  if (knownClass == instance->second.knownObjectClassHandlesByFederate.end() ||
      !federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    return std::nullopt;
  }
  auto const knownClassName = federation->second.objectClassHandles->nameFor(knownClass->second);
  if (!knownClassName ||
      federation->second.definition.catalog->objectClass(*knownClassName) == nullptr) {
    return std::nullopt;
  }
  for (std::uint64_t const attributeHandle : attributeHandles) {
    if (!federation->second.attributeHandles->nameFor(
            federation->second.definition.catalog.get(),
            *knownClassName,
            attributeHandle)) {
      return std::nullopt;
    }
  }
  auto const callbackRoute = federation->second.interactionCallbackRoutes.find(receivingFederateId);
  if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
      !callbackRoute->second) {
    return std::nullopt;
  }

  std::set<std::uint64_t> deliverableAttributeHandles;
  for (auto const& [requestId, pending] :
       instance->second.pendingAttributeOwnershipAcquisitionRequests) {
    static_cast<void>(requestId);
    if (pending.requestingFederateId != receivingFederateId) {
      continue;
    }
    for (std::uint64_t const attributeHandle : attributeHandles) {
      if (pending.unavailableQueuedAttributeHandles.contains(attributeHandle)) {
        deliverableAttributeHandles.insert(attributeHandle);
      }
    }
  }
  if (deliverableAttributeHandles.empty()) {
    return std::nullopt;
  }

  // If cancellation was requested while the owner callback was in flight,
  // release denial is the winning terminal event. Consume the cancellation
  // reservation so its queued confirmation becomes harmless.
  for (auto cancellation =
           instance->second.pendingAttributeOwnershipAcquisitionCancellations.begin();
       cancellation !=
           instance->second.pendingAttributeOwnershipAcquisitionCancellations.end();) {
    if (cancellation->second.requestingFederateId == receivingFederateId) {
      for (std::uint64_t const attributeHandle : deliverableAttributeHandles) {
        cancellation->second.attributeHandles.erase(attributeHandle);
      }
    }
    if (cancellation->second.attributeHandles.empty()) {
      cancellation =
          instance->second.pendingAttributeOwnershipAcquisitionCancellations.erase(cancellation);
    } else {
      ++cancellation;
    }
  }

  for (auto pending = instance->second.pendingAttributeOwnershipAcquisitionRequests.begin();
       pending != instance->second.pendingAttributeOwnershipAcquisitionRequests.end();) {
    if (pending->second.requestingFederateId != receivingFederateId) {
      ++pending;
      continue;
    }
    for (std::uint64_t const attributeHandle : deliverableAttributeHandles) {
      pending->second.unavailableQueuedAttributeHandles.erase(attributeHandle);
      pending->second.desiredAttributeHandles.erase(attributeHandle);
      pending->second.notificationQueuedAttributeHandles.erase(attributeHandle);
      for (auto release = pending->second.releaseCallbacksQueuedByOwningFederate.begin();
           release != pending->second.releaseCallbacksQueuedByOwningFederate.end();) {
        release->second.erase(attributeHandle);
        if (release->second.empty()) {
          release = pending->second.releaseCallbacksQueuedByOwningFederate.erase(release);
        } else {
          ++release;
        }
      }
    }
    if (pending->second.desiredAttributeHandles.empty()) {
      pending = instance->second.pendingAttributeOwnershipAcquisitionRequests.erase(pending);
    } else {
      ++pending;
    }
  }
  return AttributeOwnershipUnavailableRecipient{
      receivingFederateId,
      objectInstanceHandle,
      std::move(deliverableAttributeHandles),
      callbackRoute->second,
  };
}


}  // namespace umbra::detail
