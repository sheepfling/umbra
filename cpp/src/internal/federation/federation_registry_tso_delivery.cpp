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

FederationTsoRetractionResult EmbeddedFederationRegistry::retractTsoMessage(
    std::wstring const& federationName,
    std::uint64_t messageId) {
  auto instrumentationScope = beginInstrumentation("retractTsoMessage");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {FederationTsoRegistryStatus::federation_does_not_exist,
            {TsoMessageQueueStatus::message_not_found, 0}};
  }
  if (messageId == 0) {
    return {FederationTsoRegistryStatus::invalid_request,
            {TsoMessageQueueStatus::invalid_message_id, 0}};
  }
  return {FederationTsoRegistryStatus::applied,
          federation->second.timeCoordinator.retractTsoMessage(messageId)};
}

FederationTsoRetractionResult EmbeddedFederationRegistry::retractTsoInteraction(
    std::wstring const& federationName,
    std::uint64_t producingFederateId,
    std::uint64_t messageId) {
  auto instrumentationScope = beginInstrumentation("retractTsoInteraction");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {FederationTsoRegistryStatus::federation_does_not_exist,
            {TsoMessageQueueStatus::message_not_found, 0}};
  }
  if (!federation->second.members.contains(producingFederateId)) {
    return {FederationTsoRegistryStatus::federate_not_member,
            {TsoMessageQueueStatus::invalid_recipient, 0}};
  }
  auto const message = federation->second.tsoInteractionMessages.find(messageId);
  if (message == federation->second.tsoInteractionMessages.end() ||
      message->second.producingFederateId != producingFederateId) {
    return {FederationTsoRegistryStatus::invalid_request,
            {TsoMessageQueueStatus::message_not_found, 0}};
  }
  return {FederationTsoRegistryStatus::applied,
          federation->second.timeCoordinator.retractTsoMessage(messageId)};
}

std::size_t EmbeddedFederationRegistry::retireTsoMessagePayloadsAtOrBefore(
    std::wstring const& federationName,
    std::uint64_t producingFederateId,
    rti1516_2025::LogicalTime const& retractionLowerBound) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      !federation->second.members.contains(producingFederateId)) {
    return 0;
  }

  std::size_t retiredCount = 0;
  for (auto& [messageId, record] : federation->second.tsoRequestRetractionRecords) {
    static_cast<void>(messageId);
    if (record.producingFederateId != producingFederateId || record.terminal ||
        !record.timestamp) {
      continue;
    }

    bool stillRetractable = false;
    try {
      // Clause 8.22.3 permits a Retract only for a timestamp strictly later
      // than the producer's current/requested time plus actual lookahead.
      stillRetractable = retractionLowerBound < *record.timestamp;
    } catch (...) {
      // A mismatched/corrupt private time value must retain its payload rather
      // than turning a potentially valid public designator into a tombstone.
      continue;
    }
    if (stillRetractable) {
      continue;
    }

    record.terminal = true;
    record.timestamp.reset();
    ++retiredCount;
  }
  reclaimTsoMessagePayloads(federation->second);
  return retiredCount;
}

FederationTsoRetractionResult EmbeddedFederationRegistry::retractTsoMessageForProducer(
    std::wstring const& federationName,
    std::uint64_t producingFederateId,
    std::uint64_t messageId,
    std::shared_ptr<rti1516_2025::LogicalTime const> const& retractionLowerBound,
    bool enforceTimestampEligibility) {
  auto instrumentationScope = beginInstrumentation("retractTsoMessageForProducer");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {FederationTsoRegistryStatus::federation_does_not_exist,
            {TsoMessageQueueStatus::message_not_found, 0}};
  }
  if (!federation->second.members.contains(producingFederateId)) {
    return {FederationTsoRegistryStatus::federate_not_member,
            {TsoMessageQueueStatus::invalid_recipient, 0}};
  }

  auto timestampIsRetractable = [&retractionLowerBound, enforceTimestampEligibility](
                                    std::shared_ptr<rti1516_2025::LogicalTime const> const& timestamp) {
    if (!enforceTimestampEligibility) {
      return true;
    }
    if (!retractionLowerBound || !timestamp) {
      return false;
    }
    try {
      // 8.22.3 requires a strict inequality: equal to current/requested time
      // plus actual lookahead is not sufficient for a legal Retract.
      return *retractionLowerBound < *timestamp;
    } catch (...) {
      return false;
    }
  };

  // Supported timestamped message families have a federation-owned recipient
  // ledger, so a legal retraction can both suppress still-pending fanout and
  // request retraction from recipients whose original callback has entered
  // user code.
  if (auto const retractionRecord =
          federation->second.tsoRequestRetractionRecords.find(messageId);
      retractionRecord != federation->second.tsoRequestRetractionRecords.end()) {
    if (retractionRecord->second.producingFederateId != producingFederateId) {
      return {FederationTsoRegistryStatus::invalid_request,
              {TsoMessageQueueStatus::message_not_found, 0}};
    }
    if (retractionRecord->second.terminal) {
      return {FederationTsoRegistryStatus::applied,
              {retractionRecord->second.retractionApplied
                   ? TsoMessageQueueStatus::message_already_retracted
                   : TsoMessageQueueStatus::message_not_found,
               0},
              false};
    }
    if (!timestampIsRetractable(retractionRecord->second.timestamp)) {
      return {FederationTsoRegistryStatus::applied,
              {TsoMessageQueueStatus::message_not_found, 0},
              false};
    }

    if (retractionRecord->second.retractionApplied) {
      return {FederationTsoRegistryStatus::applied,
              {TsoMessageQueueStatus::message_already_retracted, 0}};
    }

    auto const objectDeletion = federation->second.tsoObjectDeletionMessages.find(messageId);
    bool const retractsObjectDeletion =
        objectDeletion != federation->second.tsoObjectDeletionMessages.end();
    auto reconstitution = federation->second.tsoObjectDeletionReconstitutionRecords.end();
    if (retractsObjectDeletion) {
      reconstitution = federation->second.tsoObjectDeletionReconstitutionRecords.find(messageId);
      if (reconstitution == federation->second.tsoObjectDeletionReconstitutionRecords.end()) {
        // A deletion callback may not be retracted without the exact
        // invocation-time object state required by 8.22.  Do not downgrade
        // this to a callback-only notification.
        return {FederationTsoRegistryStatus::invalid_request,
                {TsoMessageQueueStatus::message_not_found, 0}};
      }
    }

    auto queueResult = federation->second.timeCoordinator.retractPendingTsoMessage(messageId);
    if (queueResult.status == TsoMessageQueueStatus::message_not_found) {
      // A supported timestamped message sent only to nonconstrained recipients
      // never enters the temporal queue. A resigned pending recipient may
      // likewise have had its entry removed by lifecycle cleanup. The ledger
      // remains authoritative for the legal service state in either case.
      queueResult = {TsoMessageQueueStatus::applied, 0};
    }
    if (queueResult.status != TsoMessageQueueStatus::applied) {
      return {FederationTsoRegistryStatus::applied, queueResult};
    }

    FederationTsoRetractionResult result{
        FederationTsoRegistryStatus::applied,
        queueResult};
    for (auto& [receivingFederateId, recipientState] :
         retractionRecord->second.recipientStates) {
      if (recipientState == Federation::TsoRecipientDeliveryState::delivered) {
        recipientState = Federation::TsoRecipientDeliveryState::retracted;
        auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
            receivingFederateId);
        if (federation->second.members.contains(receivingFederateId) &&
            callbackRoute != federation->second.interactionCallbackRoutes.end() &&
            callbackRoute->second) {
          result.requestRetractionNotifications.push_back({
              receivingFederateId,
              messageId,
              callbackRoute->second,
          });
        }
      } else if (recipientState == Federation::TsoRecipientDeliveryState::pending) {
        recipientState = Federation::TsoRecipientDeliveryState::retracted;
      }
    }

    if (retractsObjectDeletion) {
      auto const objectInstanceHandle = objectDeletion->second.objectInstanceHandle;
      auto instance = federation->second.objectInstances.find(objectInstanceHandle);
      bool const deletionCommitted =
          instance == federation->second.objectInstances.end() || instance->second.deleteAccepted;

      if (deletionCommitted) {
        // Restore the exact invocation-time state first.  Request Retraction
        // is queued only after this registry transition, so an immediate
        // recipient can observe a reconstituted known instance while handling
        // its callback.  Preserve only currently joined holders: the standard
        // reassumes attributes to federates that are still joined, while a
        // departed handle cannot regain ownership in this bounded profile.
        auto restored = reconstitution->second.objectInstanceAtInvocation;
        for (auto known = restored.knownObjectClassHandlesByFederate.begin();
             known != restored.knownObjectClassHandlesByFederate.end();) {
          if (!federation->second.members.contains(known->first)) {
            known = restored.knownObjectClassHandlesByFederate.erase(known);
          } else {
            ++known;
          }
        }
        for (auto owner = restored.attributeOwnersByHandle.begin();
             owner != restored.attributeOwnersByHandle.end();) {
          if (!federation->second.members.contains(owner->second)) {
            owner = restored.attributeOwnersByHandle.erase(owner);
          } else {
            ++owner;
          }
        }
        for (auto association = restored.updateRegionsByAttribute.begin();
             association != restored.updateRegionsByAttribute.end();) {
          for (auto region = association->second.begin(); region != association->second.end();) {
            if (!federation->second.regions.contains(*region)) {
              region = association->second.erase(region);
            } else {
              ++region;
            }
          }
          if (association->second.empty()) {
            association = restored.updateRegionsByAttribute.erase(association);
          } else {
            ++association;
          }
        }
        restored.deleteAccepted = false;
        restored.pendingTimestampedDeletionMessageId.reset();
        restored.pendingTimestampedRemovalFederates.clear();
        federation->second.objectInstanceHandlesByName.insert_or_assign(
            restored.name,
            restored.handle);
        federation->second.objectInstances.insert_or_assign(
            objectInstanceHandle,
            std::move(restored));
        refreshRegionUsage(federation->second);
      } else if (instance != federation->second.objectInstances.end() &&
                 instance->second.pendingTimestampedDeletionMessageId == messageId) {
        // Before any Remove Object Instance callback starts, the object has
        // not been deleted.  Cancelling the pending deletion must not roll
        // back unrelated state changes that occurred after the invocation.
        instance->second.pendingTimestampedDeletionMessageId.reset();
        instance->second.pendingTimestampedRemovalFederates.clear();
      }

      federation->second.tsoObjectDeletionReconstitutionRecords.erase(messageId);
      federation->second.tsoObjectDeletionMessages.erase(messageId);
    }
    retractionRecord->second.retractionApplied = true;
    retractionRecord->second.terminal = true;
    retractionRecord->second.timestamp.reset();
    reclaimTsoMessagePayload(federation->second, messageId);
    return result;
  }

  // Every timestamped deletion must carry the execution-owned recipient
  // ledger and reconstitution snapshot installed by enqueueTsoObjectDeletion.
  // Do not retain the former snapshot-less fallback here: it could only
  // suppress a pending callback and would make a delivered deletion look
  // retractable without the object state required by 8.22.3.
  if (federation->second.tsoObjectDeletionMessages.contains(messageId)) {
    return {FederationTsoRegistryStatus::invalid_request,
            {TsoMessageQueueStatus::message_not_found, 0}};
  }

  bool ownsMessage = false;
  std::shared_ptr<rti1516_2025::LogicalTime const> messageTimestamp;
  if (auto const interaction = federation->second.tsoInteractionMessages.find(messageId);
      interaction != federation->second.tsoInteractionMessages.end()) {
    ownsMessage = interaction->second.producingFederateId == producingFederateId;
    messageTimestamp = interaction->second.timestamp;
  }
  if (auto const attributeUpdate =
          federation->second.tsoAttributeUpdateMessages.find(messageId);
      attributeUpdate != federation->second.tsoAttributeUpdateMessages.end()) {
    ownsMessage = attributeUpdate->second.producingFederateId == producingFederateId;
    messageTimestamp = attributeUpdate->second.timestamp;
  }
  if (auto const directedInteraction =
          federation->second.tsoDirectedInteractionMessages.find(messageId);
      directedInteraction != federation->second.tsoDirectedInteractionMessages.end()) {
    ownsMessage = directedInteraction->second.producingFederateId == producingFederateId;
    messageTimestamp = directedInteraction->second.timestamp;
  }
  if (!ownsMessage) {
    return {FederationTsoRegistryStatus::invalid_request,
              {TsoMessageQueueStatus::message_not_found, 0}};
  }
  if (!timestampIsRetractable(messageTimestamp)) {
    return {FederationTsoRegistryStatus::applied,
            {TsoMessageQueueStatus::message_not_found, 0},
            false};
  }
  auto const queueResult = federation->second.timeCoordinator.retractTsoMessage(messageId);
  if (queueResult.status == TsoMessageQueueStatus::applied) {
    federation->second.tsoDirectedInteractionMessages.erase(messageId);
  }
  return {FederationTsoRegistryStatus::applied, queueResult};
}

bool EmbeddedFederationRegistry::beginTsoInteractionCallback(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t messageId) {
  auto instrumentationScope = beginInstrumentation("beginTsoInteractionCallback");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end() || receivingFederateId == 0 ||
      !federation->second.members.contains(receivingFederateId)) {
    return false;
  }
  auto record = federation->second.tsoRequestRetractionRecords.find(messageId);
  if (record == federation->second.tsoRequestRetractionRecords.end()) {
    return false;
  }
  auto recipient = record->second.recipientStates.find(receivingFederateId);
  if (recipient == record->second.recipientStates.end() ||
      recipient->second != Federation::TsoRecipientDeliveryState::pending) {
    return false;
  }
  recipient->second = Federation::TsoRecipientDeliveryState::delivered;
  reclaimTsoMessagePayload(federation->second, messageId);
  return true;
}

bool EmbeddedFederationRegistry::beginTsoAttributeUpdateCallback(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t messageId) {
  auto instrumentationScope = beginInstrumentation("beginTsoAttributeUpdateCallback");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end() || receivingFederateId == 0 ||
      !federation->second.members.contains(receivingFederateId)) {
    return false;
  }
  auto record = federation->second.tsoRequestRetractionRecords.find(messageId);
  if (record == federation->second.tsoRequestRetractionRecords.end()) {
    return false;
  }
  auto recipient = record->second.recipientStates.find(receivingFederateId);
  if (recipient == record->second.recipientStates.end() ||
      (recipient->second != Federation::TsoRecipientDeliveryState::pending &&
       recipient->second != Federation::TsoRecipientDeliveryState::delivered)) {
    return false;
  }
  recipient->second = Federation::TsoRecipientDeliveryState::delivered;
  applyTsoAttributeUpdateValues(federation->second, messageId);
  reclaimTsoMessagePayload(federation->second, messageId);
  return true;
}

bool EmbeddedFederationRegistry::finishTsoRecipientCallbackSuppressed(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t messageId) {
  auto instrumentationScope = beginInstrumentation("finishTsoRecipientCallbackSuppressed");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end() || receivingFederateId == 0 ||
      !federation->second.members.contains(receivingFederateId)) {
    return false;
  }
  auto record = federation->second.tsoRequestRetractionRecords.find(messageId);
  if (record == federation->second.tsoRequestRetractionRecords.end()) {
    return false;
  }
  auto recipient = record->second.recipientStates.find(receivingFederateId);
  if (recipient == record->second.recipientStates.end() ||
      recipient->second != Federation::TsoRecipientDeliveryState::pending) {
    return false;
  }

  // The temporal boundary was consumed, but the current declaration did not
  // qualify for a user callback. Preserve this as its own terminal state so a
  // later Retract cannot issue Request Retraction for a callback that never
  // began, while releasing any lifecycle guard that only protects pending
  // object-dependent delivery.
  recipient->second = Federation::TsoRecipientDeliveryState::suppressed;
  applyTsoAttributeUpdateValues(federation->second, messageId);
  reclaimTsoMessagePayload(federation->second, messageId);
  return true;
}

bool EmbeddedFederationRegistry::canDeliverTsoRequestRetraction(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t messageId) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() || receivingFederateId == 0 ||
      !federation->second.members.contains(receivingFederateId) ||
      !federation->second.interactionCallbackRoutes.contains(receivingFederateId)) {
    return false;
  }
  auto const record = federation->second.tsoRequestRetractionRecords.find(messageId);
  if (record == federation->second.tsoRequestRetractionRecords.end()) {
    return false;
  }
  auto const recipient = record->second.recipientStates.find(receivingFederateId);
  return recipient != record->second.recipientStates.end() &&
      recipient->second == Federation::TsoRecipientDeliveryState::retracted;
}

FederationTsoDeliveryRegistryResult EmbeddedFederationRegistry::beginTsoDelivery(
    std::wstring const& federationName,
    std::uint64_t recipientFederateId,
    rti1516_2025::LogicalTime const& boundary,
    bool inclusive) {
  auto instrumentationScope = beginInstrumentation("beginTsoDelivery");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {FederationTsoRegistryStatus::federation_does_not_exist,
            {FederationTsoDeliveryStatus::no_messages, {}}};
  }
  if (!federation->second.members.contains(recipientFederateId)) {
    return {FederationTsoRegistryStatus::federate_not_member,
            {FederationTsoDeliveryStatus::invalid_recipient, {}}};
  }
  return {FederationTsoRegistryStatus::applied,
          federation->second.timeCoordinator.beginTsoDelivery(
              recipientFederateId,
              boundary,
              inclusive)};
}

FederationTsoInteractionDeliveryRegistryResult
EmbeddedFederationRegistry::beginTsoInteractionDelivery(
    std::wstring const& federationName,
    std::uint64_t recipientFederateId,
    rti1516_2025::LogicalTime const& boundary,
    bool inclusive) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {FederationTsoRegistryStatus::federation_does_not_exist,
            FederationTsoDeliveryStatus::no_messages,
            {}};
  }
  if (!federation->second.members.contains(recipientFederateId)) {
    return {FederationTsoRegistryStatus::federate_not_member,
            FederationTsoDeliveryStatus::invalid_recipient,
            {}};
  }

  auto const delivery = federation->second.timeCoordinator.beginTsoDelivery(
      recipientFederateId,
      boundary,
      inclusive);
  if (delivery.status != FederationTsoDeliveryStatus::applied) {
    return {FederationTsoRegistryStatus::applied, delivery.status, {}};
  }

  std::vector<TsoInteractionDelivery> result;
  result.reserve(delivery.messages.size());
  for (auto const& queuedMessage : delivery.messages) {
    auto const message = federation->second.tsoInteractionMessages.find(
        queuedMessage.messageId);
    if (message == federation->second.tsoInteractionMessages.end()) {
      // A missing payload means the private temporal state is inconsistent;
      // leave the queue entry in transit so the caller cannot accidentally
      // report a standards callback with invented data.
      return {FederationTsoRegistryStatus::invalid_request,
              FederationTsoDeliveryStatus::message_not_in_transit,
              {}};
    }
    result.push_back({queuedMessage, message->second});
  }
  return {FederationTsoRegistryStatus::applied,
          FederationTsoDeliveryStatus::applied,
          std::move(result)};
}

FederationTsoPayloadDeliveryRegistryResult
EmbeddedFederationRegistry::beginTsoPayloadDelivery(
    std::wstring const& federationName,
    std::uint64_t recipientFederateId,
    rti1516_2025::LogicalTime const& boundary,
    bool inclusive) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {FederationTsoRegistryStatus::federation_does_not_exist,
            FederationTsoDeliveryStatus::no_messages,
            {}};
  }
  if (!federation->second.members.contains(recipientFederateId)) {
    return {FederationTsoRegistryStatus::federate_not_member,
            FederationTsoDeliveryStatus::invalid_recipient,
            {}};
  }

  auto const delivery = federation->second.timeCoordinator.beginTsoDelivery(
      recipientFederateId,
      boundary,
      inclusive);
  if (delivery.status != FederationTsoDeliveryStatus::applied) {
    return {FederationTsoRegistryStatus::applied, delivery.status, {}};
  }

  std::vector<TsoPayloadDelivery> result;
  result.reserve(delivery.messages.size());
  for (auto const& queuedMessage : delivery.messages) {
    if (auto const interaction = federation->second.tsoInteractionMessages.find(
            queuedMessage.messageId);
        interaction != federation->second.tsoInteractionMessages.end()) {
      result.emplace_back(TsoInteractionDelivery{queuedMessage, interaction->second});
      continue;
    }
    if (auto const attributeUpdate =
            federation->second.tsoAttributeUpdateMessages.find(queuedMessage.messageId);
        attributeUpdate != federation->second.tsoAttributeUpdateMessages.end()) {
      result.emplace_back(
          TsoAttributeUpdateDelivery{queuedMessage, attributeUpdate->second});
      continue;
    }
    if (auto const objectDeletion =
            federation->second.tsoObjectDeletionMessages.find(queuedMessage.messageId);
        objectDeletion != federation->second.tsoObjectDeletionMessages.end()) {
      result.emplace_back(
          TsoObjectDeletionDelivery{queuedMessage, objectDeletion->second});
      continue;
    }
    if (auto const directedInteraction =
            federation->second.tsoDirectedInteractionMessages.find(
                queuedMessage.messageId);
        directedInteraction != federation->second.tsoDirectedInteractionMessages.end()) {
      result.emplace_back(TsoDirectedInteractionDelivery{
          queuedMessage, directedInteraction->second});
      continue;
    }

    // A missing payload means the private temporal state is inconsistent;
    // leave every queue entry in transit so the caller cannot invent a
    // standards callback or silently lose a retraction boundary.
    return {FederationTsoRegistryStatus::invalid_request,
            FederationTsoDeliveryStatus::message_not_in_transit,
            {}};
  }
  return {FederationTsoRegistryStatus::applied,
          FederationTsoDeliveryStatus::applied,
          std::move(result)};
}

std::optional<RemovedObjectInstanceSnapshot>
EmbeddedFederationRegistry::beginTsoObjectInstanceRemoval(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t messageId) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() ||
      instance->second.pendingTimestampedDeletionMessageId != messageId ||
      !instance->second.pendingTimestampedRemovalFederates.contains(receivingFederateId)) {
    return std::nullopt;
  }

  auto retractionRecord = federation->second.tsoRequestRetractionRecords.find(messageId);
  if (retractionRecord == federation->second.tsoRequestRetractionRecords.end()) {
    return std::nullopt;
  }
  auto recipientState = retractionRecord->second.recipientStates.find(receivingFederateId);
  if (recipientState == retractionRecord->second.recipientStates.end() ||
      recipientState->second != Federation::TsoRecipientDeliveryState::pending) {
    // A concurrent legal Retract won the callback boundary.  The queue may
    // already have yielded this typed payload, but it must not commit a stale
    // Remove Object Instance after its recipient state was retracted.
    return std::nullopt;
  }

  instance->second.pendingTimestampedRemovalFederates.erase(receivingFederateId);
  auto const known = instance->second.knownObjectClassHandlesByFederate.find(
      receivingFederateId);
  if (!federation->second.members.contains(receivingFederateId) ||
      known == instance->second.knownObjectClassHandlesByFederate.end()) {
    return std::nullopt;
  }

  auto const receivingMember = federation->second.members.find(receivingFederateId);
  if (receivingMember != federation->second.members.end() &&
      receivingMember->second.successfulObjectInstanceRemovalsCount !=
          std::numeric_limits<std::uint64_t>::max()) {
    ++receivingMember->second.successfulObjectInstanceRemovalsCount;
  }

  // Commit the recipient state before changing the execution-wide object
  // state.  A later Retract observes this as a delivered recipient, restores
  // the invocation snapshot, and queues Request Retraction after releasing
  // the registry lock.
  recipientState->second = Federation::TsoRecipientDeliveryState::delivered;

  // The first accepted removal commits the federation-wide deletion and
  // removes the producing federate's local knowledge without inducing a
  // callback on that federate.  Other known recipients transition one at a
  // time as their timestamped callbacks begin.
  if (!instance->second.deleteAccepted) {
    instance->second.deleteAccepted = true;
    instance->second.pendingDiscoveryFederates.clear();
    instance->second.pendingAttributeValueUpdateRequests.clear();
    instance->second.pendingAttributeValueUpdateClassRequests.clear();
    instance->second.pendingAttributeValueUpdateRegionalRequests.clear();
    clearPendingAttributeOwnershipQueries(
        federation->second,
        instance->second.handle);
    instance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests.clear();
    instance->second.pendingAttributeOwnershipAcquisitionRequests.clear();
    instance->second.pendingAttributeOwnershipAcquisitionCancellations.clear();
    instance->second.pendingAttributeOwnershipDivestitureIfWantedNotifications.clear();
    instance->second.pendingNegotiatedAttributeOwnershipDivestitures.clear();
    instance->second.pendingConfirmDivestitureNotifications.clear();
    instance->second.knownObjectClassHandlesByFederate.erase(
        instance->second.producingFederateId);
  }

  RemovedObjectInstanceSnapshot const result{
      instance->second.handle,
      instance->second.producingFederateId,
  };
  instance->second.knownObjectClassHandlesByFederate.erase(known);
  // Retain the deletion marker and backing object while the returned
  // retraction designator remains execution-owned.  This makes a legal
  // post-delivery reconstitution atomic and prevents a name/handle collision
  // with a subsequently registered object.
  reclaimTsoMessagePayload(federation->second, messageId);
  return result;
}

FederationTsoDeliveryRegistryResult EmbeddedFederationRegistry::completeTsoDelivery(
    std::wstring const& federationName,
    TsoQueuedMessage const& message) {
  auto instrumentationScope = beginInstrumentation("completeTsoDelivery");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {FederationTsoRegistryStatus::federation_does_not_exist,
            {FederationTsoDeliveryStatus::message_not_in_transit, {}}};
  }
  if (!federation->second.members.contains(message.recipientFederateId)) {
    return {FederationTsoRegistryStatus::federate_not_member,
            {FederationTsoDeliveryStatus::invalid_recipient, {}}};
  }
  return {FederationTsoRegistryStatus::applied,
          {federation->second.timeCoordinator.completeTsoDelivery(message), {}}};
}

FederationTsoDeliveryRegistryResult
EmbeddedFederationRegistry::completeTsoDeliveryFor(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t messageId) {
  auto instrumentationScope = beginInstrumentation("completeTsoDeliveryFor");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {FederationTsoRegistryStatus::federation_does_not_exist,
            {FederationTsoDeliveryStatus::message_not_in_transit, {}}};
  }
  if (receivingFederateId == 0 || messageId == 0) {
    return {FederationTsoRegistryStatus::invalid_request,
            {FederationTsoDeliveryStatus::invalid_recipient, {}}};
  }
  if (!federation->second.members.contains(receivingFederateId)) {
    return {FederationTsoRegistryStatus::federate_not_member,
            {FederationTsoDeliveryStatus::invalid_recipient, {}}};
  }
  return {FederationTsoRegistryStatus::applied,
          {federation->second.timeCoordinator.completeTsoDeliveryFor(
               receivingFederateId, messageId), {}}};
}

std::vector<ObjectInstanceRemovalRecipient>
EmbeddedFederationRegistry::releaseConnectionLossDeferredObjectInstanceRemovals(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId) {
  auto instrumentationScope = beginInstrumentation(
      "releaseConnectionLossDeferredObjectInstanceRemovals");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end() || receivingFederateId == 0 ||
      !federation->second.members.contains(receivingFederateId)) {
    return {};
  }

  std::vector<ObjectInstanceRemovalRecipient> result;
  for (auto& [objectInstanceHandle, instance] : federation->second.objectInstances) {
    if (!instance.pendingRemovalFederates.contains(receivingFederateId) ||
        !instance.deferredConnectionLossTsoRemovalFederates.contains(
            receivingFederateId) ||
        hasPendingConnectionLossTsoObjectDelivery(
            federation->second,
            objectInstanceHandle,
            receivingFederateId)) {
      continue;
    }
    auto const route = federation->second.interactionCallbackRoutes.find(receivingFederateId);
    if (route == federation->second.interactionCallbackRoutes.end() || !route->second) {
      // The original forced-resign validation established a route. Retain the
      // reservation if a later private lifecycle transition has made that
      // route unavailable; recipient resignation clears it explicitly.
      continue;
    }
    auto const reportRoute = federation->second.serviceReportRoutes.find(receivingFederateId);

    // Claim the release exactly once. The original callback returned without
    // delivery at the protected boundary; this fresh route is submitted only
    // after the grant callback has completed.
    instance.deferredConnectionLossTsoRemovalFederates.erase(receivingFederateId);
    result.push_back({
        receivingFederateId,
        objectInstanceHandle,
        route->second,
        reportRoute == federation->second.serviceReportRoutes.end()
            ? FederateServiceReportRoute{}
            : reportRoute->second,
    });
  }
  return result;
}

std::optional<FederationTimeExecutionSnapshot> EmbeddedFederationRegistry::makeTimeSnapshot(
    Federation const& federation) {
  FederationTimeExecutionSnapshot result{federation.definition, {}};
  result.nonRegulatedGrant = federation.nonRegulatedGrantSwitch;
  auto const timeSnapshots = federation.timeCoordinator.snapshot();
  result.federates.reserve(timeSnapshots.size());
  for (auto const& timeSnapshot : timeSnapshots) {
    auto const member = federation.members.find(timeSnapshot.federateId);
    if (member == federation.members.end()) {
      // Registration and membership are committed together. A mismatch is a
      // private corruption signal, so do not expose a partial input set to a
      // GALT calculation or grant scheduler.
      return std::nullopt;
    }
    auto const tso = federation.timeCoordinator.tsoSnapshotFor(timeSnapshot.federateId);
    result.federates.push_back({
        member->second,
        timeSnapshot.time,
        std::move(tso.queued),
        std::move(tso.inTransit),
        std::move(tso.deliveredSinceLastAdvance),
    });
  }
  return result;
}

std::vector<FederationTimeGrantDispatch> EmbeddedFederationRegistry::scheduleEligibleTimeAdvanceGrants(
    Federation& federation) {
  auto snapshot = makeTimeSnapshot(federation);
  if (!snapshot) {
    return {};
  }

  FederationTimeBoundsCalculator boundsCalculator;
  FederationTimeAdvanceGrantPolicy grantPolicy;
  std::vector<FederationTimeGrantDispatch> result;
  for (auto& [federateId, pending] : federation.pendingTimeAdvanceGrants) {
    if (pending.dispatchQueued) {
      continue;
    }
    auto const requester = std::find_if(
        snapshot->federates.begin(),
        snapshot->federates.end(),
        [federateId](FederationTimeFederateSnapshot const& candidate) {
          return candidate.membership.id == federateId;
        });
    if (requester == snapshot->federates.end()) {
      continue;
    }
    FederationTimeBounds const bounds = boundsCalculator.calculate(*snapshot, federateId);
    bool const grantEligible = grantPolicy.decide(requester->time, bounds).mayGrant() ||
        connectionLossTsoDeliveryMayGrant(
            federation,
            federateId,
            requester->time,
            bounds);
    if (!grantEligible) {
      continue;
    }

    // Copy before marking the record queued. If copying the opaque dispatch
    // throws, the request remains eligible for a later re-evaluation.
    result.push_back(pending.dispatch);
    pending.dispatchQueued = true;
  }
  return result;
}

std::vector<FederationTimeGrantDispatch>
EmbeddedFederationRegistry::scheduleRestoredTimeRoleEnableDispatches(
    Federation& federation) {
  struct PendingRoleEnable final {
    std::uint64_t federateId = 0;
    std::uint64_t generation = 0;
    FederationTimeRoleEnableKind kind = FederationTimeRoleEnableKind::regulation;
  };

  std::vector<PendingRoleEnable> pending;
  for (auto const& savedTimeState : federation.timeCoordinator.snapshot()) {
    auto const& time = savedTimeState.time;
    if (time.timeRegulationPending !=
            (time.pendingTimeRegulationGeneration != 0U) ||
        time.timeConstrainedPending !=
            (time.pendingTimeConstrainedGeneration != 0U)) {
      throw std::logic_error(
          "A restored time state has inconsistent pending role-enable state.");
    }
    if (time.timeRegulationPending) {
      pending.push_back({
          savedTimeState.federateId,
          time.pendingTimeRegulationGeneration,
          FederationTimeRoleEnableKind::regulation,
      });
    }
    if (time.timeConstrainedPending) {
      pending.push_back({
          savedTimeState.federateId,
          time.pendingTimeConstrainedGeneration,
          FederationTimeRoleEnableKind::constrained,
      });
    }
  }

  // The generation allocator is shared by both role requests. Sorting keeps
  // the callback order that the requesting federate observed before save,
  // even when regulation and constrained enables were accepted together.
  std::sort(
      pending.begin(),
      pending.end(),
      [](PendingRoleEnable const& left, PendingRoleEnable const& right) {
        if (left.federateId != right.federateId) {
          return left.federateId < right.federateId;
        }
        return left.generation < right.generation;
      });

  std::vector<FederationTimeGrantDispatch> result;
  result.reserve(pending.size());
  for (auto const& role : pending) {
    auto const factory = federation.timeRoleEnableDispatchFactories.find(role.federateId);
    if (factory == federation.timeRoleEnableDispatchFactories.end() ||
        !factory->second) {
      throw std::logic_error(
          "A restored pending role-enable request has no live callback dispatcher.");
    }
    auto dispatch = factory->second(role.federateId, role.generation, role.kind);
    if (!dispatch) {
      throw std::logic_error(
          "The embedded federation could not recreate a saved role-enable callback.");
    }
    result.push_back(std::move(dispatch));
  }
  return result;
}


}  // namespace umbra::detail
