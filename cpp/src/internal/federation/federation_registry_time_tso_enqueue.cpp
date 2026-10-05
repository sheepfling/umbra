#include "internal/federation/federation_registry.hpp"
#include "internal/federation/federation_state_image.hpp"

#include "internal/fom/fom_catalog.hpp"
#include "internal/fom/fdd_document.hpp"
#include "internal/fom/hla_names.hpp"
#include "internal/time/federation_time_bounds.hpp"
#include "internal/time/federation_time_grant_policy.hpp"
#include "internal/handles/handle_variable_array_encoding.hpp"
#include "internal/runtime/utf8_string.hpp"

#include <RTI/encoding/BasicDataElements.h>
#include <RTI/time/HLAlogicalTimeFactoryFactory.h>

#include <algorithm>
#include <atomic>
#include <iterator>
#include <limits>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <tuple>
#include <utility>

namespace umbra::detail {

FederationTimeGrantDispatchResult EmbeddedFederationRegistry::requestTimeAdvanceGrant(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t generation) {
  auto instrumentationScope = beginInstrumentation("requestTimeAdvanceGrant");
  if (federateId == 0 || generation == 0) {
    return {FederationTimeGrantStatus::inconsistent_temporal_state, {}};
  }

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {FederationTimeGrantStatus::federation_does_not_exist, {}};
  }
  if (!federation->second.members.contains(federateId)) {
    return {FederationTimeGrantStatus::federate_not_member, {}};
  }
  if (federation->second.pendingTimeAdvanceGrants.contains(federateId)) {
    return {FederationTimeGrantStatus::stale_generation, {}};
  }

  auto const factory =
      federation->second.timeAdvanceGrantDispatchFactories.find(federateId);
  if (factory == federation->second.timeAdvanceGrantDispatchFactories.end() ||
      !factory->second ||
      federation->second.nextTimeAdvanceGrantDispatchIdentity == 0 ||
      federation->second.nextTimeAdvanceGrantDispatchIdentity ==
          std::numeric_limits<std::uint64_t>::max()) {
    return {FederationTimeGrantStatus::inconsistent_temporal_state, {}};
  }

  auto const dispatchIdentity = federation->second.nextTimeAdvanceGrantDispatchIdentity++;
  FederationTimeGrantDispatch dispatch;
  try {
    dispatch = factory->second(federateId, generation, dispatchIdentity);
  } catch (...) {
    return {FederationTimeGrantStatus::inconsistent_temporal_state, {}};
  }
  if (!dispatch) {
    return {FederationTimeGrantStatus::inconsistent_temporal_state, {}};
  }

  auto [pending, inserted] = federation->second.pendingTimeAdvanceGrants.emplace(
      federateId,
      Federation::PendingTimeAdvanceGrant{
          generation,
          dispatchIdentity,
          std::move(dispatch),
          false,
      });
  static_cast<void>(pending);
  if (!inserted) {
    return {FederationTimeGrantStatus::stale_generation, {}};
  }

  return {FederationTimeGrantStatus::applied, scheduleEligibleTimeAdvanceGrants(federation->second)};
}

FederationTimeGrantDispatchResult EmbeddedFederationRegistry::requestTimeAdvanceGrant(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t generation,
    FederationTimeGrantDispatch dispatch) {
  auto instrumentationScope = beginInstrumentation("requestTimeAdvanceGrant");
  if (federateId == 0 || generation == 0 || !dispatch) {
    return {FederationTimeGrantStatus::inconsistent_temporal_state, {}};
  }

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {FederationTimeGrantStatus::federation_does_not_exist, {}};
  }
  if (!federation->second.members.contains(federateId)) {
    return {FederationTimeGrantStatus::federate_not_member, {}};
  }
  if (federation->second.pendingTimeAdvanceGrants.contains(federateId)) {
    return {FederationTimeGrantStatus::stale_generation, {}};
  }

  auto [pending, inserted] = federation->second.pendingTimeAdvanceGrants.emplace(
      federateId,
      Federation::PendingTimeAdvanceGrant{generation, 0, std::move(dispatch), false});
  static_cast<void>(pending);
  if (!inserted) {
    return {FederationTimeGrantStatus::stale_generation, {}};
  }

  return {FederationTimeGrantStatus::applied, scheduleEligibleTimeAdvanceGrants(federation->second)};
}

FederationTimeGrantDispatchResult EmbeddedFederationRegistry::reevaluateTimeAdvanceGrants(
    std::wstring const& federationName) {
  auto instrumentationScope = beginInstrumentation("reevaluateTimeAdvanceGrants");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {FederationTimeGrantStatus::federation_does_not_exist, {}};
  }
  return {FederationTimeGrantStatus::applied, scheduleEligibleTimeAdvanceGrants(federation->second)};
}

FederationTimeGrantStatus EmbeddedFederationRegistry::beginTimeAdvanceGrant(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t generation,
    std::uint64_t dispatchIdentity) {
  auto instrumentationScope = beginInstrumentation("beginTimeAdvanceGrant");
  if (federateId == 0 || generation == 0) {
    return FederationTimeGrantStatus::inconsistent_temporal_state;
  }

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return FederationTimeGrantStatus::federation_does_not_exist;
  }
  if (!federation->second.members.contains(federateId)) {
    return FederationTimeGrantStatus::federate_not_member;
  }
  auto pending = federation->second.pendingTimeAdvanceGrants.find(federateId);
  if (pending == federation->second.pendingTimeAdvanceGrants.end()) {
    return FederationTimeGrantStatus::time_advance_not_pending;
  }
  if (pending->second.generation != generation ||
      pending->second.dispatchIdentity != dispatchIdentity) {
    return FederationTimeGrantStatus::stale_generation;
  }
  if (!pending->second.dispatchQueued) {
    return FederationTimeGrantStatus::grant_not_ready;
  }

  auto snapshot = makeTimeSnapshot(federation->second);
  if (!snapshot) {
    pending->second.dispatchQueued = false;
    return FederationTimeGrantStatus::inconsistent_temporal_state;
  }
  auto const requester = std::find_if(
      snapshot->federates.begin(),
      snapshot->federates.end(),
      [federateId](FederationTimeFederateSnapshot const& candidate) {
        return candidate.membership.id == federateId;
      });
  if (requester == snapshot->federates.end()) {
    pending->second.dispatchQueued = false;
    return FederationTimeGrantStatus::federate_not_member;
  }

  FederationTimeBounds const bounds = FederationTimeBoundsCalculator{}.calculate(*snapshot, federateId);
  FederationTimeAdvanceGrantDecision const decision =
      FederationTimeAdvanceGrantPolicy{}.decide(requester->time, bounds);
  bool const grantEligible = decision.mayGrant() || connectionLossTsoDeliveryMayGrant(
      federation->second,
      federateId,
      requester->time,
      bounds);
  if (!grantEligible) {
    pending->second.dispatchQueued = false;
    switch (decision.status) {
      case FederationTimeAdvanceGrantStatus::wait_for_galt:
      case FederationTimeAdvanceGrantStatus::wait_for_non_regulated_grant:
        return FederationTimeGrantStatus::grant_not_ready;
      case FederationTimeAdvanceGrantStatus::no_time_advance_pending:
        return FederationTimeGrantStatus::time_advance_not_pending;
      case FederationTimeAdvanceGrantStatus::inconsistent_temporal_state:
        return FederationTimeGrantStatus::inconsistent_temporal_state;
      case FederationTimeAdvanceGrantStatus::grant:
        break;
    }
    return FederationTimeGrantStatus::inconsistent_temporal_state;
  }

  federation->second.pendingTimeAdvanceGrants.erase(pending);
  static_cast<void>(federation->second.timeCoordinator.clearDeliveredTsoMessages(federateId));
  return FederationTimeGrantStatus::applied;
}

FederationTsoMessageIdResult EmbeddedFederationRegistry::allocateTsoMessageId(
    std::wstring const& federationName) {
  auto instrumentationScope = beginInstrumentation("allocateTsoMessageId");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {FederationTsoRegistryStatus::federation_does_not_exist, 0};
  }
  auto const messageId = federation->second.timeCoordinator.allocateTsoMessageId();
  if (messageId == 0) {
    return {FederationTsoRegistryStatus::invalid_request, 0};
  }
  return {FederationTsoRegistryStatus::applied, messageId};
}

FederationTsoEnqueueResult EmbeddedFederationRegistry::enqueueTsoMessage(
    std::wstring const& federationName,
    std::uint64_t messageId,
    std::uint64_t recipientFederateId,
    std::shared_ptr<rti1516_2025::LogicalTime const> timestamp) {
  auto instrumentationScope = beginInstrumentation("enqueueTsoMessage");
  if (messageId == 0 || recipientFederateId == 0 || !timestamp) {
    return {FederationTsoRegistryStatus::invalid_request,
            TsoMessageQueueStatus::invalid_message_id};
  }
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {FederationTsoRegistryStatus::federation_does_not_exist,
            TsoMessageQueueStatus::invalid_message_id};
  }
  if (!federation->second.members.contains(recipientFederateId)) {
    return {FederationTsoRegistryStatus::federate_not_member,
            TsoMessageQueueStatus::invalid_recipient};
  }
  auto const result = federation->second.timeCoordinator.enqueueTsoMessage(
      messageId,
      recipientFederateId,
      std::move(timestamp));
  return {FederationTsoRegistryStatus::applied, result.status};
}

std::optional<std::shared_ptr<rti1516_2025::LogicalTime const>>
EmbeddedFederationRegistry::earliestTsoTimestampFor(
    std::wstring const& federationName,
    std::uint64_t recipientFederateId) const {
  if (recipientFederateId == 0) {
    return std::nullopt;
  }
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      !federation->second.members.contains(recipientFederateId)) {
    return std::nullopt;
  }
  return federation->second.timeCoordinator.earliestTsoTimestampFor(recipientFederateId);
}

FederationTsoInteractionEnqueueResult EmbeddedFederationRegistry::enqueueTsoInteraction(
    std::wstring const& federationName,
    TsoInteractionMessage message,
    std::vector<std::uint64_t> const& queuedRecipientFederateIds,
    std::vector<std::uint64_t> const& allTimestampedRecipientFederateIds) {
  auto instrumentationScope = beginInstrumentation("enqueueTsoInteraction");
  if (message.producingFederateId == 0 || message.sentInteractionClassHandle == 0 ||
      !message.timestamp || message.timestamp->isInitial() || message.timestamp->isFinal()) {
    return {FederationTsoRegistryStatus::invalid_request,
            TsoMessageQueueStatus::invalid_message_id,
            0,
            0};
  }

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {FederationTsoRegistryStatus::federation_does_not_exist,
            TsoMessageQueueStatus::invalid_message_id,
            0,
            0};
  }
  if (!federation->second.members.contains(message.producingFederateId)) {
    return {FederationTsoRegistryStatus::federate_not_member,
            TsoMessageQueueStatus::invalid_recipient,
            0,
            0};
  }
  if (message.timestamp->implementationName() !=
      federation->second.definition.logicalTimeImplementationName) {
    return {FederationTsoRegistryStatus::invalid_request,
            TsoMessageQueueStatus::logical_time_implementation_mismatch,
            0,
            0};
  }

  std::set<std::uint64_t> allRecipients;
  for (std::uint64_t const recipientFederateId : allTimestampedRecipientFederateIds) {
    if (recipientFederateId == 0 ||
        !federation->second.members.contains(recipientFederateId) ||
        !allRecipients.insert(recipientFederateId).second) {
      return {FederationTsoRegistryStatus::invalid_request,
              TsoMessageQueueStatus::invalid_recipient,
              0,
              0};
    }
  }
  std::set<std::uint64_t> queuedRecipients;
  for (std::uint64_t const recipientFederateId : queuedRecipientFederateIds) {
    if (!allRecipients.contains(recipientFederateId) ||
        !queuedRecipients.insert(recipientFederateId).second) {
      return {FederationTsoRegistryStatus::invalid_request,
              TsoMessageQueueStatus::invalid_recipient,
              0,
              0};
    }
  }

  // A producing federate may resign before this queued interaction reaches a
  // constrained recipient. Its public RegionHandles are then released from
  // the live federation map, so retain the committed invocation-time specs
  // alongside the opaque handles for callback-boundary overlap evaluation.
  // The public timestamped regional service supplies the snapshots captured
  // by its accepted planning pass. Do not re-read mutable live regions here:
  // doing so would let a region mutation between planning and queue admission
  // give immediate and TSO recipients different DDM realizations for one
  // service invocation.
  if (message.sentRegionHandles.empty()) {
    message.sentRegionSnapshots.clear();
  } else {
    if (message.sentRegionSnapshots.size() != message.sentRegionHandles.size()) {
      return {FederationTsoRegistryStatus::invalid_request,
              TsoMessageQueueStatus::invalid_recipient,
              0,
              0};
    }
    for (std::uint64_t const regionHandle : message.sentRegionHandles) {
      auto const snapshot = message.sentRegionSnapshots.find(regionHandle);
      if (snapshot == message.sentRegionSnapshots.end() ||
          !snapshot->second.specificationCommitted ||
          snapshot->second.committedRangeBounds.size() !=
              snapshot->second.dimensionHandles.size()) {
        return {FederationTsoRegistryStatus::invalid_request,
                TsoMessageQueueStatus::invalid_recipient,
                0,
                0};
      }
    }
  }

  auto const messageId = federation->second.timeCoordinator.allocateTsoMessageId();
  if (messageId == 0) {
    return {FederationTsoRegistryStatus::invalid_request,
            TsoMessageQueueStatus::invalid_message_id,
            0,
            0};
  }
  message.messageId = messageId;
  auto const [messagePosition, inserted] =
      federation->second.tsoInteractionMessages.emplace(messageId, std::move(message));
  if (!inserted) {
    return {FederationTsoRegistryStatus::invalid_request,
            TsoMessageQueueStatus::invalid_message_id,
            0,
            0};
  }

  Federation::TsoRequestRetractionRecord retractionRecord;
  retractionRecord.producingFederateId = messagePosition->second.producingFederateId;
  retractionRecord.timestamp = messagePosition->second.timestamp;
  for (std::uint64_t const recipientFederateId : allRecipients) {
    retractionRecord.recipientStates.emplace(
        recipientFederateId,
        Federation::TsoRecipientDeliveryState::pending);
  }
  auto const [recordPosition, recordInserted] =
      federation->second.tsoRequestRetractionRecords.emplace(
          messageId, std::move(retractionRecord));
  static_cast<void>(recordPosition);
  if (!recordInserted) {
    federation->second.tsoInteractionMessages.erase(messagePosition);
    return {FederationTsoRegistryStatus::invalid_request,
            TsoMessageQueueStatus::invalid_message_id,
            0,
            0};
  }

  std::size_t enqueuedCount = 0;
  TsoMessageQueueStatus queueStatus = TsoMessageQueueStatus::applied;
  for (std::uint64_t const recipientFederateId : queuedRecipients) {
    auto const result = federation->second.timeCoordinator.enqueueTsoMessage(
        messageId,
        recipientFederateId,
        messagePosition->second.timestamp);
    if (result.status != TsoMessageQueueStatus::applied) {
      queueStatus = result.status;
      break;
    }
    ++enqueuedCount;
  }
  if (queueStatus != TsoMessageQueueStatus::applied) {
    static_cast<void>(federation->second.timeCoordinator.retractTsoMessage(messageId));
    federation->second.tsoInteractionMessages.erase(messageId);
    federation->second.tsoRequestRetractionRecords.erase(messageId);
    return {FederationTsoRegistryStatus::invalid_request,
            queueStatus,
            0,
            enqueuedCount};
  }

  // A timestamped Send Interaction returns its designator even when no
  // recipient is eligible. The retraction ledger then suffices for a legal
  // Retract or eventual MessageCanNoLongerBeRetracted classification; no
  // typed interaction payload need be retained in that no-fanout case.
  reclaimTsoMessagePayload(federation->second, messageId);

  return {FederationTsoRegistryStatus::applied,
          TsoMessageQueueStatus::applied,
          messageId,
          enqueuedCount};
}

FederationTsoAttributeUpdateEnqueueResult
EmbeddedFederationRegistry::enqueueTsoAttributeUpdate(
    std::wstring const& federationName,
    TsoAttributeUpdateMessage message,
    std::vector<std::uint64_t> const& queuedRecipientFederateIds,
    std::vector<std::uint64_t> const& allTimestampedRecipientFederateIds) {
  auto instrumentationScope = beginInstrumentation("enqueueTsoAttributeUpdate");
  if (message.producingFederateId == 0 ||
      message.objectInstanceHandle == 0 ||
      message.attributes.empty() ||
      !message.timestamp || message.timestamp->isInitial() || message.timestamp->isFinal()) {
    return {FederationTsoRegistryStatus::invalid_request,
            TsoMessageQueueStatus::invalid_message_id,
            0,
            0};
  }

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {FederationTsoRegistryStatus::federation_does_not_exist,
            TsoMessageQueueStatus::invalid_message_id,
            0,
            0};
  }
  if (!federation->second.members.contains(message.producingFederateId)) {
    return {FederationTsoRegistryStatus::federate_not_member,
            TsoMessageQueueStatus::invalid_recipient,
            0,
            0};
  }
  if (message.timestamp->implementationName() !=
      federation->second.definition.logicalTimeImplementationName) {
    return {FederationTsoRegistryStatus::invalid_request,
            TsoMessageQueueStatus::logical_time_implementation_mismatch,
            0,
            0};
  }

  std::set<std::uint64_t> allRecipients;
  for (std::uint64_t const recipientFederateId : allTimestampedRecipientFederateIds) {
    if (recipientFederateId == 0 ||
        !federation->second.members.contains(recipientFederateId) ||
        !message.passelsByRecipient.contains(recipientFederateId) ||
        !allRecipients.insert(recipientFederateId).second) {
      return {FederationTsoRegistryStatus::invalid_request,
              TsoMessageQueueStatus::invalid_recipient,
              0,
              0};
    }
  }
  std::set<std::uint64_t> queuedRecipients;
  for (std::uint64_t const recipientFederateId : queuedRecipientFederateIds) {
    if (!allRecipients.contains(recipientFederateId) ||
        !queuedRecipients.insert(recipientFederateId).second) {
      return {FederationTsoRegistryStatus::invalid_request,
              TsoMessageQueueStatus::invalid_recipient,
              0,
              0};
    }
  }

  // A producing federate may resign before a queued regional reflection
  // reaches a constrained recipient. Preserve every explicit source-region
  // specification at invocation time; resignation removes both the public
  // RegionHandle and the object's live update-region association, neither of
  // which may erase an already accepted TSO passel's DDM scope. The public
  // timestamped service has already captured these specifications while it
  // planned the invocation. Re-reading the live region map here would allow a
  // source-range mutation between planning and queue admission to give
  // immediate and TSO recipients different DDM realizations for one call.
  std::set<std::uint64_t> explicitRegionHandles;
  for (auto const& [recipientFederateId, passels] : message.passelsByRecipient) {
    static_cast<void>(recipientFederateId);
    for (auto const& passel : passels) {
      explicitRegionHandles.insert(
          passel.sentRegionHandles.begin(),
          passel.sentRegionHandles.end());
    }
  }
  if (explicitRegionHandles.empty()) {
    message.sentRegionSnapshots.clear();
  } else {
    if (message.sentRegionSnapshots.size() != explicitRegionHandles.size()) {
      return {FederationTsoRegistryStatus::invalid_request,
              TsoMessageQueueStatus::invalid_recipient,
              0,
              0};
    }
    for (std::uint64_t const regionHandle : explicitRegionHandles) {
      auto const snapshot = message.sentRegionSnapshots.find(regionHandle);
      if (snapshot == message.sentRegionSnapshots.end() ||
          !snapshot->second.specificationCommitted ||
          snapshot->second.committedRangeBounds.size() !=
              snapshot->second.dimensionHandles.size()) {
        return {FederationTsoRegistryStatus::invalid_request,
                TsoMessageQueueStatus::invalid_recipient,
                0,
                0};
      }
    }
  }

  auto const messageId = federation->second.timeCoordinator.allocateTsoMessageId();
  if (messageId == 0) {
    return {FederationTsoRegistryStatus::invalid_request,
            TsoMessageQueueStatus::invalid_message_id,
            0,
            0};
  }
  message.messageId = messageId;
  auto const [messagePosition, inserted] =
      federation->second.tsoAttributeUpdateMessages.emplace(messageId, std::move(message));
  if (!inserted) {
    return {FederationTsoRegistryStatus::invalid_request,
            TsoMessageQueueStatus::invalid_message_id,
            0,
            0};
  }

  Federation::TsoRequestRetractionRecord retractionRecord;
  retractionRecord.producingFederateId = messagePosition->second.producingFederateId;
  retractionRecord.timestamp = messagePosition->second.timestamp;
  for (std::uint64_t const recipientFederateId : allRecipients) {
    retractionRecord.recipientStates.emplace(
        recipientFederateId,
        Federation::TsoRecipientDeliveryState::pending);
  }
  auto const [recordPosition, recordInserted] =
      federation->second.tsoRequestRetractionRecords.emplace(
          messageId, std::move(retractionRecord));
  static_cast<void>(recordPosition);
  if (!recordInserted) {
    federation->second.tsoAttributeUpdateMessages.erase(messagePosition);
    return {FederationTsoRegistryStatus::invalid_request,
            TsoMessageQueueStatus::invalid_message_id,
            0,
            0};
  }

  std::size_t enqueuedCount = 0;
  TsoMessageQueueStatus queueStatus = TsoMessageQueueStatus::applied;
  for (std::uint64_t const recipientFederateId : queuedRecipients) {
    auto const result = federation->second.timeCoordinator.enqueueTsoMessage(
        messageId,
        recipientFederateId,
        messagePosition->second.timestamp);
    if (result.status != TsoMessageQueueStatus::applied) {
      queueStatus = result.status;
      break;
    }
    ++enqueuedCount;
  }
  if (queueStatus != TsoMessageQueueStatus::applied) {
    static_cast<void>(federation->second.timeCoordinator.retractTsoMessage(messageId));
    federation->second.tsoAttributeUpdateMessages.erase(messageId);
    federation->second.tsoRequestRetractionRecords.erase(messageId);
    return {FederationTsoRegistryStatus::invalid_request,
            queueStatus,
            0,
            enqueuedCount};
  }

  // A timestamped Update Attribute Values invocation returns its designator
  // even when no recipient is eligible. The retraction ledger then suffices
  // for a legal Retract or eventual MessageCanNoLongerBeRetracted
  // classification; no typed attribute payload need be retained in that
  // no-fanout case.
  reclaimTsoMessagePayload(federation->second, messageId);

  return {FederationTsoRegistryStatus::applied,
          TsoMessageQueueStatus::applied,
          messageId,
          enqueuedCount};
}

ObjectInstanceDeletionPlan EmbeddedFederationRegistry::planTsoObjectInstanceDeletion(
    std::wstring const& federationName,
    std::uint64_t producingFederateId,
    std::uint64_t objectInstanceHandle) const {
  auto instrumentationScope = beginInstrumentation("planTsoObjectInstanceDeletion");
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {ObjectInstanceDeletionStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(producingFederateId)) {
    return {ObjectInstanceDeletionStatus::federate_not_member};
  }
  auto const instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() ||
      instance->second.deleteAccepted ||
      instance->second.pendingTimestampedDeletionMessageId.has_value() ||
      !instance->second.knownObjectClassHandlesByFederate.contains(producingFederateId)) {
    return {ObjectInstanceDeletionStatus::object_instance_not_known};
  }
  if (!federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    return {ObjectInstanceDeletionStatus::inconsistent_catalog};
  }

  auto const objectClassName = federation->second.objectClassHandles->nameFor(
      instance->second.registeredObjectClassHandle);
  if (!objectClassName) {
    return {ObjectInstanceDeletionStatus::inconsistent_catalog};
  }
  auto const privilegeToDelete = federation->second.attributeHandles->handleFor(
      federation->second.definition.catalog.get(),
      *objectClassName,
      "HLAprivilegeToDeleteObject");
  if (!privilegeToDelete) {
    return {ObjectInstanceDeletionStatus::inconsistent_catalog};
  }
  auto const privilegeOwner = instance->second.attributeOwnersByHandle.find(*privilegeToDelete);
  if (privilegeOwner == instance->second.attributeOwnersByHandle.end() ||
      privilegeOwner->second != producingFederateId) {
    return {ObjectInstanceDeletionStatus::delete_privilege_not_held};
  }

  auto const preferredOrderType = effectiveAttributeOrderType(
      federation->second, instance->second, *privilegeToDelete);
  if (!preferredOrderType) {
    return {ObjectInstanceDeletionStatus::inconsistent_catalog};
  }

  ObjectInstanceDeletionPlan result;
  result.preferredOrderType = *preferredOrderType;
  result.recipients.reserve(instance->second.knownObjectClassHandlesByFederate.size());
  for (auto const& [receivingFederateId, knownObjectClassHandle] :
       instance->second.knownObjectClassHandlesByFederate) {
    static_cast<void>(knownObjectClassHandle);
    if (receivingFederateId == producingFederateId ||
        !federation->second.members.contains(receivingFederateId)) {
      continue;
    }
    auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
        receivingFederateId);
    if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
        !callbackRoute->second) {
      continue;
    }
    result.recipients.push_back({
        receivingFederateId,
        instance->second.handle,
        callbackRoute->second,
    });
  }
  return result;
}

FederationTsoObjectDeletionEnqueueResult
EmbeddedFederationRegistry::enqueueTsoObjectDeletion(
    std::wstring const& federationName,
    std::uint64_t producingFederateId,
    std::uint64_t objectInstanceHandle,
    TsoObjectDeletionMessage message,
    std::vector<std::uint64_t> const& recipientFederateIds) {
  auto instrumentationScope = beginInstrumentation("enqueueTsoObjectDeletion");
  FederationTsoObjectDeletionEnqueueResult invalid;
  invalid.status = FederationTsoRegistryStatus::invalid_request;
  invalid.queueStatus = TsoMessageQueueStatus::invalid_message_id;
  invalid.deletionStatus = ObjectInstanceDeletionStatus::inconsistent_catalog;
  if (producingFederateId == 0 || objectInstanceHandle == 0 || !message.timestamp) {
    return invalid;
  }

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    invalid.status = FederationTsoRegistryStatus::federation_does_not_exist;
    invalid.deletionStatus = ObjectInstanceDeletionStatus::federation_does_not_exist;
    return invalid;
  }
  if (!federation->second.members.contains(producingFederateId)) {
    invalid.status = FederationTsoRegistryStatus::federate_not_member;
    invalid.deletionStatus = ObjectInstanceDeletionStatus::federate_not_member;
    return invalid;
  }

  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() ||
      instance->second.deleteAccepted ||
      instance->second.pendingTimestampedDeletionMessageId.has_value() ||
      !instance->second.knownObjectClassHandlesByFederate.contains(producingFederateId)) {
    invalid.deletionStatus = ObjectInstanceDeletionStatus::object_instance_not_known;
    return invalid;
  }
  if (!federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    invalid.deletionStatus = ObjectInstanceDeletionStatus::inconsistent_catalog;
    return invalid;
  }

  auto const objectClassName = federation->second.objectClassHandles->nameFor(
      instance->second.registeredObjectClassHandle);
  if (!objectClassName) {
    invalid.deletionStatus = ObjectInstanceDeletionStatus::inconsistent_catalog;
    return invalid;
  }
  auto const privilegeToDelete = federation->second.attributeHandles->handleFor(
      federation->second.definition.catalog.get(),
      *objectClassName,
      "HLAprivilegeToDeleteObject");
  if (!privilegeToDelete) {
    invalid.deletionStatus = ObjectInstanceDeletionStatus::inconsistent_catalog;
    return invalid;
  }
  auto const privilegeOwner = instance->second.attributeOwnersByHandle.find(*privilegeToDelete);
  if (privilegeOwner == instance->second.attributeOwnersByHandle.end() ||
      privilegeOwner->second != producingFederateId) {
    invalid.deletionStatus = ObjectInstanceDeletionStatus::delete_privilege_not_held;
    return invalid;
  }

  std::vector<TsoObjectDeletionRecipient> recipients;
  recipients.reserve(instance->second.knownObjectClassHandlesByFederate.size());
  for (auto const& [receivingFederateId, knownObjectClassHandle] :
       instance->second.knownObjectClassHandlesByFederate) {
    static_cast<void>(knownObjectClassHandle);
    if (receivingFederateId == producingFederateId ||
        !federation->second.members.contains(receivingFederateId)) {
      continue;
    }
    auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
        receivingFederateId);
    if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
        !callbackRoute->second) {
      continue;
    }
    auto const reportRoute = federation->second.serviceReportRoutes.find(
        receivingFederateId);
    recipients.push_back({
        receivingFederateId,
        objectInstanceHandle,
        callbackRoute->second,
        reportRoute == federation->second.serviceReportRoutes.end()
            ? FederateServiceReportRoute{}
            : reportRoute->second,
    });
  }

  std::set<std::uint64_t> recipientSet;
  for (std::uint64_t const recipientFederateId : recipientFederateIds) {
    if (!recipientSet.insert(recipientFederateId).second ||
        !federation->second.members.contains(recipientFederateId) ||
        recipientFederateId == producingFederateId ||
        std::none_of(
            recipients.begin(),
            recipients.end(),
            [recipientFederateId](TsoObjectDeletionRecipient const& recipient) {
              return recipient.receivingFederateId == recipientFederateId;
            })) {
      invalid.status = FederationTsoRegistryStatus::invalid_request;
      invalid.queueStatus = TsoMessageQueueStatus::invalid_recipient;
      invalid.deletionStatus = ObjectInstanceDeletionStatus::inconsistent_catalog;
      return invalid;
    }
  }

  auto const messageId = federation->second.timeCoordinator.allocateTsoMessageId();
  if (messageId == 0) {
    invalid.deletionStatus = ObjectInstanceDeletionStatus::inconsistent_catalog;
    return invalid;
  }
  message.messageId = messageId;
  message.producingFederateId = producingFederateId;
  message.objectInstanceHandle = objectInstanceHandle;
  message.recipients = recipients;
  auto const [messagePosition, inserted] =
      federation->second.tsoObjectDeletionMessages.emplace(messageId, std::move(message));
  if (!inserted) {
    invalid.deletionStatus = ObjectInstanceDeletionStatus::inconsistent_catalog;
    return invalid;
  }

  // Preserve the complete invocation-time object state before the first
  // timestamped removal commits deleteAccepted.  IEEE 1516.1-2025 requires a
  // legal later Retract to reconstitute this instance and reassume its
  // attributes' original ownership; callback payload alone cannot do that.
  auto const [reconstitutionPosition, reconstitutionInserted] =
      federation->second.tsoObjectDeletionReconstitutionRecords.emplace(
          messageId,
          Federation::TsoObjectDeletionReconstitutionRecord{instance->second});
  static_cast<void>(reconstitutionPosition);
  if (!reconstitutionInserted) {
    federation->second.tsoObjectDeletionMessages.erase(messagePosition);
    invalid.deletionStatus = ObjectInstanceDeletionStatus::inconsistent_catalog;
    return invalid;
  }

  Federation::TsoRequestRetractionRecord retractionRecord;
  retractionRecord.producingFederateId = producingFederateId;
  retractionRecord.timestamp = messagePosition->second.timestamp;
  for (auto const& recipient : recipients) {
    retractionRecord.recipientStates.emplace(
        recipient.receivingFederateId,
        Federation::TsoRecipientDeliveryState::pending);
  }
  auto const [retractionPosition, retractionInserted] =
      federation->second.tsoRequestRetractionRecords.emplace(
          messageId,
          std::move(retractionRecord));
  static_cast<void>(retractionPosition);
  if (!retractionInserted) {
    federation->second.tsoObjectDeletionReconstitutionRecords.erase(messageId);
    federation->second.tsoObjectDeletionMessages.erase(messagePosition);
    invalid.deletionStatus = ObjectInstanceDeletionStatus::inconsistent_catalog;
    return invalid;
  }

  instance->second.pendingTimestampedDeletionMessageId = messageId;
  instance->second.pendingTimestampedRemovalFederates.clear();
  for (auto const& recipient : recipients) {
    instance->second.pendingTimestampedRemovalFederates.insert(
        recipient.receivingFederateId);
  }

  std::size_t enqueuedCount = 0;
  TsoMessageQueueStatus queueStatus = TsoMessageQueueStatus::applied;
  for (std::uint64_t const recipientFederateId : recipientFederateIds) {
    auto const result = federation->second.timeCoordinator.enqueueTsoMessage(
        messageId,
        recipientFederateId,
        messagePosition->second.timestamp);
    if (result.status != TsoMessageQueueStatus::applied) {
      queueStatus = result.status;
      break;
    }
    ++enqueuedCount;
  }
  if (queueStatus != TsoMessageQueueStatus::applied) {
    static_cast<void>(federation->second.timeCoordinator.retractTsoMessage(messageId));
    federation->second.tsoObjectDeletionMessages.erase(messageId);
    federation->second.tsoObjectDeletionReconstitutionRecords.erase(messageId);
    federation->second.tsoRequestRetractionRecords.erase(messageId);
    instance->second.pendingTimestampedDeletionMessageId.reset();
    instance->second.pendingTimestampedRemovalFederates.clear();
    return {FederationTsoRegistryStatus::invalid_request,
            queueStatus,
            0,
            enqueuedCount,
            std::move(recipients),
            ObjectInstanceDeletionStatus::inconsistent_catalog};
  }

  auto const deletingMember = federation->second.members.find(producingFederateId);
  if (deletingMember != federation->second.members.end() &&
      deletingMember->second.successfulObjectInstanceDeletionsCount !=
          std::numeric_limits<std::uint64_t>::max()) {
    ++deletingMember->second.successfulObjectInstanceDeletionsCount;
  }

  // No other federate knows this object.  The deletion is accepted without a
  // callback, while a queued or immediate recipient keeps the object alive
  // until its corresponding Remove Object Instance boundary.  Keep the
  // timestamped-deletion marker after acceptance in either case: its message
  // retraction designator remains valid until the producer reaches the
  // standard's time bound, and retaining the object/name prevents a later
  // reconstitution from colliding with a newly registered instance.
  if (recipients.empty()) {
    instance->second.deleteAccepted = true;
    instance->second.knownObjectClassHandlesByFederate.erase(producingFederateId);
    instance->second.pendingTimestampedRemovalFederates.clear();
  }

  return {FederationTsoRegistryStatus::applied,
          TsoMessageQueueStatus::applied,
          messageId,
          enqueuedCount,
          std::move(recipients),
          ObjectInstanceDeletionStatus::applied};
}

FederationTsoDirectedInteractionEnqueueResult
EmbeddedFederationRegistry::enqueueTsoDirectedInteraction(
    std::wstring const& federationName,
    TsoDirectedInteractionMessage message,
    std::vector<std::uint64_t> const& queuedRecipientFederateIds) {
  auto instrumentationScope = beginInstrumentation("enqueueTsoDirectedInteraction");
  if (message.producingFederateId == 0 ||
      message.objectInstanceHandle == 0 ||
      message.sentInteractionClassHandle == 0 ||
      message.sentParameterHandles.size() != message.parameters.size() ||
      message.recipients.empty() || !message.timestamp ||
      message.timestamp->isInitial() || message.timestamp->isFinal()) {
    return {FederationTsoRegistryStatus::invalid_request,
            TsoMessageQueueStatus::invalid_message_id,
            0,
            0};
  }

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {FederationTsoRegistryStatus::federation_does_not_exist,
            TsoMessageQueueStatus::invalid_message_id,
            0,
            0};
  }
  if (!federation->second.members.contains(message.producingFederateId)) {
    return {FederationTsoRegistryStatus::federate_not_member,
            TsoMessageQueueStatus::invalid_recipient,
            0,
            0};
  }
  if (message.timestamp->implementationName() !=
      federation->second.definition.logicalTimeImplementationName) {
    return {FederationTsoRegistryStatus::invalid_request,
            TsoMessageQueueStatus::logical_time_implementation_mismatch,
            0,
            0};
  }

  std::set<std::uint64_t> knownRecipients;
  for (auto const& recipient : message.recipients) {
    if (recipient.receivingFederateId == 0 ||
        recipient.receivingFederateId == message.producingFederateId ||
        !recipient.callbackRoute ||
        !federation->second.members.contains(recipient.receivingFederateId) ||
        !knownRecipients.insert(recipient.receivingFederateId).second) {
      return {FederationTsoRegistryStatus::invalid_request,
              TsoMessageQueueStatus::invalid_recipient,
              0,
              0};
    }
  }
  std::set<std::uint64_t> queuedRecipients;
  for (auto const recipientFederateId : queuedRecipientFederateIds) {
    if (!queuedRecipients.insert(recipientFederateId).second ||
        !knownRecipients.contains(recipientFederateId)) {
      return {FederationTsoRegistryStatus::invalid_request,
              TsoMessageQueueStatus::invalid_recipient,
              0,
              0};
    }
  }

  auto const messageId = federation->second.timeCoordinator.allocateTsoMessageId();
  if (messageId == 0) {
    return {FederationTsoRegistryStatus::invalid_request,
            TsoMessageQueueStatus::invalid_message_id,
            0,
            0};
  }
  message.messageId = messageId;
  auto const [messagePosition, inserted] =
      federation->second.tsoDirectedInteractionMessages.emplace(
          messageId, std::move(message));
  if (!inserted) {
    return {FederationTsoRegistryStatus::invalid_request,
            TsoMessageQueueStatus::invalid_message_id,
            0,
            0};
  }

  Federation::TsoRequestRetractionRecord retractionRecord;
  retractionRecord.producingFederateId = messagePosition->second.producingFederateId;
  retractionRecord.timestamp = messagePosition->second.timestamp;
  for (auto const recipientFederateId : knownRecipients) {
    retractionRecord.recipientStates.emplace(
        recipientFederateId,
        Federation::TsoRecipientDeliveryState::pending);
  }
  auto const [recordPosition, recordInserted] =
      federation->second.tsoRequestRetractionRecords.emplace(
          messageId, std::move(retractionRecord));
  static_cast<void>(recordPosition);
  if (!recordInserted) {
    federation->second.tsoDirectedInteractionMessages.erase(messagePosition);
    return {FederationTsoRegistryStatus::invalid_request,
            TsoMessageQueueStatus::invalid_message_id,
            0,
            0};
  }

  std::size_t enqueuedCount = 0;
  TsoMessageQueueStatus queueStatus = TsoMessageQueueStatus::applied;
  for (auto const recipientFederateId : queuedRecipients) {
    auto const result = federation->second.timeCoordinator.enqueueTsoMessage(
        messageId,
        recipientFederateId,
        messagePosition->second.timestamp);
    if (result.status != TsoMessageQueueStatus::applied) {
      queueStatus = result.status;
      break;
    }
    ++enqueuedCount;
  }
  if (queueStatus != TsoMessageQueueStatus::applied) {
    static_cast<void>(federation->second.timeCoordinator.retractTsoMessage(messageId));
    federation->second.tsoDirectedInteractionMessages.erase(messageId);
    federation->second.tsoRequestRetractionRecords.erase(messageId);
    return {FederationTsoRegistryStatus::invalid_request,
            queueStatus,
            0,
            enqueuedCount};
  }

  return {FederationTsoRegistryStatus::applied,
          TsoMessageQueueStatus::applied,
          messageId,
          enqueuedCount};
}


}  // namespace umbra::detail
