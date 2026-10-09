#include "internal/federation/federation_registry.hpp"
#include "internal/federation/federation_state_image.hpp"
#include "internal/federation/federation_registry_state_image_restore_helpers.hpp"

#include <RTI/encoding/BasicDataElements.h>

#include <algorithm>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace umbra::detail {

using federation_registry_state_image_restore_helpers::decodeLogicalTimeEncoding;
using federation_registry_state_image_restore_helpers::decodeSavedOrderType;
using federation_registry_state_image_restore_helpers::decodeSavedRegionSnapshot;
using federation_registry_state_image_restore_helpers::variableLengthDataFromBytes;

void EmbeddedFederationRegistry::restoreTsoPayloadsFromStateImage(
    Federation& federation,
    FederationStateImage const& image,
    Federation const& liveFederation) {
  if (image.logicalTimeImplementationName !=
      federation.definition.logicalTimeImplementationName) {
    throw std::logic_error(
        "The saved TSO payloads use a different logical-time implementation.");
  }
  if (image.tsoInteractionMessageCount != image.tsoInteractionMessages.size() ||
      image.tsoAttributeUpdateMessageCount != image.tsoAttributeUpdateMessages.size() ||
      image.tsoObjectDeletionMessageCount != image.tsoObjectDeletionMessages.size() ||
      image.tsoDirectedInteractionMessageCount !=
          image.tsoDirectedInteractionMessages.size()) {
    throw std::logic_error(
          "The saved TSO payload counts do not match their durable sections.");
  }

  // Decode into replacement maps first.  A malformed payload or unavailable
  // live route therefore cannot leave a partially rehydrated snapshot behind.
  std::map<std::uint64_t, Federation::TsoRequestRetractionRecord>
      retractionRecords;
  for (auto const& savedRecord : image.tsoRequestRetractionRecords) {
    if (savedRecord.messageId == 0U || savedRecord.producingFederateId == 0U ||
        (savedRecord.retractionApplied && !savedRecord.terminal) ||
        (!savedRecord.terminal && !savedRecord.timestampEncoding.has_value())) {
      throw std::logic_error(
          "The saved TSO Request Retraction record has incomplete identity or state.");
    }
    auto timestamp = decodeLogicalTimeEncoding(
        image.logicalTimeImplementationName,
        savedRecord.timestampEncoding);
    if (timestamp && (timestamp->isInitial() || timestamp->isFinal())) {
      throw std::logic_error(
          "The saved TSO Request Retraction record has an invalid timestamp.");
    }
    Federation::TsoRequestRetractionRecord record;
    record.producingFederateId = savedRecord.producingFederateId;
    record.timestamp = std::move(timestamp);
    record.retractionApplied = savedRecord.retractionApplied;
    record.terminal = savedRecord.terminal;
    record.producerResigned = savedRecord.producerResigned;
    record.deliveryRequiredAfterConnectionLoss =
        savedRecord.deliveryRequiredAfterConnectionLoss;
    for (auto const& savedRecipient : savedRecord.recipientStates) {
      if (savedRecipient.receivingFederateId == 0U ||
          savedRecipient.state > 3U) {
        throw std::logic_error(
            "The saved TSO Request Retraction record has an invalid recipient state.");
      }
      auto const [position, inserted] = record.recipientStates.emplace(
          savedRecipient.receivingFederateId,
          static_cast<Federation::TsoRecipientDeliveryState>(
              savedRecipient.state));
      static_cast<void>(position);
      if (!inserted) {
        throw std::logic_error(
            "The saved TSO Request Retraction record has duplicate recipients.");
      }
    }

    auto const [position, inserted] = retractionRecords.emplace(
        savedRecord.messageId,
        std::move(record));
    static_cast<void>(position);
    if (!inserted) {
      throw std::logic_error(
          "The saved TSO Request Retraction section contains duplicate message IDs.");
    }
  }

  std::map<std::uint64_t, TsoInteractionMessage> interactions;
  for (auto const& savedMessage : image.tsoInteractionMessages) {
    if (savedMessage.messageId == 0U || savedMessage.producingFederateId == 0U ||
        savedMessage.sentInteractionClassHandle == 0U ||
        !savedMessage.timestampEncoding.has_value() ||
        savedMessage.sentParameterHandles.size() != savedMessage.parameters.size()) {
      throw std::logic_error(
          "The saved TSO interaction payload has incomplete identity or parameters.");
    }
    auto const sentTimestamp = decodeLogicalTimeEncoding(
        image.logicalTimeImplementationName,
        savedMessage.timestampEncoding);
    if (!sentTimestamp || sentTimestamp->isInitial() || sentTimestamp->isFinal()) {
      throw std::logic_error(
          "The saved TSO interaction payload has an invalid timestamp.");
    }
    auto const sentOrder = decodeSavedOrderType(savedMessage.sentOrderType);
    auto const receivedOrder = decodeSavedOrderType(savedMessage.receivedOrderType);
    if (!sentOrder || !receivedOrder) {
      throw std::logic_error(
          "The saved TSO interaction payload has an invalid order type.");
    }

    std::set<std::uint64_t> sentParameterHandles;
    for (auto const parameterHandle : savedMessage.sentParameterHandles) {
      if (parameterHandle == 0U ||
          !sentParameterHandles.insert(parameterHandle).second) {
        throw std::logic_error(
            "The saved TSO interaction payload has duplicate parameter handles.");
      }
    }

    TsoInteractionMessage message;
    message.messageId = savedMessage.messageId;
    message.producingFederateId = savedMessage.producingFederateId;
    message.sentInteractionClassHandle = savedMessage.sentInteractionClassHandle;
    message.sentParameterHandles = savedMessage.sentParameterHandles;
    message.parameters.reserve(savedMessage.parameters.size());
    for (auto const& savedParameter : savedMessage.parameters) {
      if (savedParameter.parameterHandle == 0U ||
          !sentParameterHandles.contains(savedParameter.parameterHandle)) {
        throw std::logic_error(
            "The saved TSO interaction payload contains an unknown parameter.");
      }
      message.parameters.emplace_back(
          savedParameter.parameterHandle,
          variableLengthDataFromBytes(savedParameter.value));
    }
    message.userSuppliedTag =
        variableLengthDataFromBytes(savedMessage.userSuppliedTag);
    message.transportationName = savedMessage.transportationName;
    message.sentRegionHandles.insert(
        savedMessage.sentRegionHandles.begin(),
        savedMessage.sentRegionHandles.end());
    if (message.sentRegionHandles.size() != savedMessage.sentRegionHandles.size() ||
        (!message.sentRegionHandles.empty() &&
         savedMessage.sentRegionSnapshots.size() !=
             message.sentRegionHandles.size()) ||
        (message.sentRegionHandles.empty() &&
         !savedMessage.sentRegionSnapshots.empty())) {
      throw std::logic_error(
          "The saved TSO interaction payload has inconsistent region snapshots.");
    }
    for (auto const& savedSnapshot : savedMessage.sentRegionSnapshots) {
      if (!message.sentRegionHandles.contains(savedSnapshot.regionHandle)) {
        throw std::logic_error(
            "The saved TSO interaction payload contains an unknown region snapshot.");
      }
      auto const [position, inserted] = message.sentRegionSnapshots.emplace(
          savedSnapshot.regionHandle,
          decodeSavedRegionSnapshot(savedSnapshot));
      static_cast<void>(position);
      if (!inserted) {
        throw std::logic_error(
            "The saved TSO interaction payload contains duplicate region snapshots.");
      }
    }
    message.defaultRegionUsed = savedMessage.defaultRegionUsed;
    message.timestamp = std::move(sentTimestamp);
    message.sentOrderType = *sentOrder;
    message.receivedOrderType = *receivedOrder;
    auto const [position, inserted] = interactions.emplace(
        message.messageId,
        std::move(message));
    static_cast<void>(position);
    if (!inserted) {
      throw std::logic_error(
          "The saved TSO interaction payload contains duplicate message IDs.");
    }
  }

  std::map<std::uint64_t, TsoObjectDeletionMessage> objectDeletions;
  std::map<std::uint64_t, Federation::TsoObjectDeletionReconstitutionRecord>
      objectDeletionReconstitutions;
  for (auto const& savedMessage : image.tsoObjectDeletionMessages) {
    if (savedMessage.messageId == 0U || savedMessage.producingFederateId == 0U ||
        savedMessage.objectInstanceHandle == 0U ||
        !savedMessage.timestampEncoding.has_value()) {
      throw std::logic_error(
          "The saved TSO object-deletion payload has incomplete identity.");
    }
    auto const sentTimestamp = decodeLogicalTimeEncoding(
        image.logicalTimeImplementationName,
        savedMessage.timestampEncoding);
    if (!sentTimestamp || sentTimestamp->isInitial() || sentTimestamp->isFinal()) {
      throw std::logic_error(
          "The saved TSO object-deletion payload has an invalid timestamp.");
    }
    auto const sentOrder = decodeSavedOrderType(savedMessage.sentOrderType);
    if (!sentOrder) {
      throw std::logic_error(
          "The saved TSO object-deletion payload has an invalid order type.");
    }

    TsoObjectDeletionMessage message;
    message.messageId = savedMessage.messageId;
    message.producingFederateId = savedMessage.producingFederateId;
    message.objectInstanceHandle = savedMessage.objectInstanceHandle;
    message.userSuppliedTag =
        variableLengthDataFromBytes(savedMessage.userSuppliedTag);
    message.timestamp = std::move(sentTimestamp);
    message.sentOrderType = *sentOrder;

    std::set<std::uint64_t> recipientIds;
    message.recipients.reserve(savedMessage.recipients.size());
    for (auto const& savedRecipient : savedMessage.recipients) {
      if (savedRecipient.receivingFederateId == 0U ||
          savedRecipient.receivingFederateId == message.producingFederateId ||
          savedRecipient.objectInstanceHandle != message.objectInstanceHandle ||
          !recipientIds.insert(savedRecipient.receivingFederateId).second ||
          !liveFederation.members.contains(savedRecipient.receivingFederateId)) {
        throw std::logic_error(
            "The saved TSO object-deletion payload has an invalid recipient.");
      }
      auto const callbackRoute = liveFederation.interactionCallbackRoutes.find(
          savedRecipient.receivingFederateId);
      if (callbackRoute == liveFederation.interactionCallbackRoutes.end() ||
          !callbackRoute->second) {
        throw std::logic_error(
            "The saved TSO object-deletion payload has no live callback route.");
      }
      auto const reportRoute = liveFederation.serviceReportRoutes.find(
          savedRecipient.receivingFederateId);
      message.recipients.push_back({
          savedRecipient.receivingFederateId,
          savedRecipient.objectInstanceHandle,
          callbackRoute->second,
          reportRoute == liveFederation.serviceReportRoutes.end()
              ? FederateServiceReportRoute{}
              : reportRoute->second,
      });
    }
    if (!savedMessage.reconstitution.has_value()) {
      throw std::logic_error(
          "The saved TSO object-deletion payload has no invocation snapshot.");
    }
    auto const& savedReconstitution = *savedMessage.reconstitution;
    auto const& savedObject = savedReconstitution.object;
    if (savedObject.handle != message.objectInstanceHandle ||
        savedObject.registeredObjectClassHandle == 0U ||
        savedObject.producingFederateId != message.producingFederateId ||
        savedObject.deleteAccepted || savedObject.pendingOperationCount != 0U) {
      throw std::logic_error(
          "The saved TSO object-deletion invocation snapshot is not rehydratable.");
    }
    if (!liveFederation.members.contains(savedObject.producingFederateId)) {
      throw std::logic_error(
          "The saved TSO object-deletion invocation producer is no longer joined.");
    }
    Federation::ObjectInstance restoredObject;
    restoredObject.handle = savedObject.handle;
    restoredObject.name = savedObject.name;
    restoredObject.registeredObjectClassHandle = savedObject.registeredObjectClassHandle;
    restoredObject.producingFederateId = savedObject.producingFederateId;
    std::set<std::uint64_t> knownAttributeHandles;
    for (auto const& savedAttribute : savedObject.attributes) {
      if (savedAttribute.handle == 0U) {
        throw std::logic_error(
            "The saved TSO object-deletion invocation has an invalid attribute.");
      }
      if (savedAttribute.ownerFederateId != 0U) {
        if (!liveFederation.members.contains(savedAttribute.ownerFederateId)) {
          throw std::logic_error(
              "The saved TSO object-deletion invocation has a departed attribute owner.");
        }
        restoredObject.attributeOwnersByHandle.emplace(
            savedAttribute.handle,
            savedAttribute.ownerFederateId);
      }
      if (!savedAttribute.transportationName.empty()) {
        restoredObject.attributeTransportationTypes.emplace(
            savedAttribute.handle,
            savedAttribute.transportationName);
      }
      if (savedAttribute.orderType != 0U) {
        auto const order = decodeSavedOrderType(savedAttribute.orderType);
        if (!order) {
          throw std::logic_error(
              "The saved TSO object-deletion invocation has an invalid attribute order.");
        }
        restoredObject.attributeOrderTypes.emplace(savedAttribute.handle, *order);
      }
      if (!savedAttribute.updateRegionHandles.empty()) {
        auto& regions = restoredObject.updateRegionsByAttribute[savedAttribute.handle];
        for (auto const regionHandle : savedAttribute.updateRegionHandles) {
          if (regionHandle == 0U || !federation.regions.contains(regionHandle) ||
              !regions.insert(regionHandle).second) {
            throw std::logic_error(
                "The saved TSO object-deletion invocation has an invalid update region.");
          }
        }
      }
      knownAttributeHandles.insert(savedAttribute.handle);
    }
    if (savedObject.attributeValuesPresent) {
      std::uint64_t previousAttributeHandle = 0U;
      for (auto const& savedValue : savedObject.attributeValues) {
        if (savedValue.attributeHandle == 0U ||
            savedValue.attributeHandle <= previousAttributeHandle ||
            !knownAttributeHandles.contains(savedValue.attributeHandle)) {
          throw std::logic_error(
              "The saved TSO object-deletion invocation has an invalid application value.");
        }
        previousAttributeHandle = savedValue.attributeHandle;
        restoredObject.attributeValues.emplace(
            savedValue.attributeHandle,
            variableLengthDataFromBytes(savedValue.value));
      }
    }
    std::uint64_t previousKnownFederate = 0U;
    for (auto const& [knownFederateId, knownObjectClassHandle] :
         savedReconstitution.knownObjectClassHandlesByFederate) {
      if (knownFederateId == 0U || knownFederateId <= previousKnownFederate ||
          knownObjectClassHandle == 0U ||
          !liveFederation.members.contains(knownFederateId)) {
        throw std::logic_error(
            "The saved TSO object-deletion invocation has invalid known-instance state.");
      }
      previousKnownFederate = knownFederateId;
      restoredObject.knownObjectClassHandlesByFederate.emplace(
          knownFederateId,
          knownObjectClassHandle);
    }
    auto const [reconstitutionPosition, reconstitutionInserted] =
        objectDeletionReconstitutions.emplace(
            message.messageId,
            Federation::TsoObjectDeletionReconstitutionRecord{
                std::move(restoredObject)});
    static_cast<void>(reconstitutionPosition);
    if (!reconstitutionInserted) {
      throw std::logic_error(
          "The saved TSO object-deletion payload has duplicate invocation snapshots.");
    }
    auto const [position, inserted] = objectDeletions.emplace(
        message.messageId,
        std::move(message));
    static_cast<void>(position);
    if (!inserted) {
      throw std::logic_error(
          "The saved TSO object-deletion payload contains duplicate message IDs.");
    }
  }

  std::map<std::uint64_t, TsoAttributeUpdateMessage> attributeUpdates;
  for (auto const& savedMessage : image.tsoAttributeUpdateMessages) {
    if (savedMessage.messageId == 0U || savedMessage.producingFederateId == 0U ||
        savedMessage.objectInstanceHandle == 0U ||
        savedMessage.attributes.empty() ||
        !savedMessage.timestampEncoding.has_value()) {
      throw std::logic_error(
          "The saved TSO attribute-update payload has incomplete identity or values.");
    }
    auto const sentTimestamp = decodeLogicalTimeEncoding(
        image.logicalTimeImplementationName,
        savedMessage.timestampEncoding);
    if (!sentTimestamp || sentTimestamp->isInitial() || sentTimestamp->isFinal()) {
      throw std::logic_error(
          "The saved TSO attribute-update payload has an invalid timestamp.");
    }

    TsoAttributeUpdateMessage message;
    message.messageId = savedMessage.messageId;
    message.producingFederateId = savedMessage.producingFederateId;
    message.objectInstanceHandle = savedMessage.objectInstanceHandle;
    message.attributes.reserve(savedMessage.attributes.size());
    std::set<std::uint64_t> attributeHandles;
    for (auto const& savedAttribute : savedMessage.attributes) {
      if (savedAttribute.attributeHandle == 0U ||
          !attributeHandles.insert(savedAttribute.attributeHandle).second) {
        throw std::logic_error(
            "The saved TSO attribute-update payload has duplicate attributes.");
      }
      message.attributes.emplace_back(
          savedAttribute.attributeHandle,
          variableLengthDataFromBytes(savedAttribute.value));
    }
    message.userSuppliedTag =
        variableLengthDataFromBytes(savedMessage.userSuppliedTag);

    std::set<std::uint64_t> explicitRegionHandles;
    for (auto const& savedRecipient : savedMessage.passelsByRecipient) {
      if (savedRecipient.receivingFederateId == 0U ||
          !liveFederation.members.contains(savedRecipient.receivingFederateId) ||
          message.passelsByRecipient.contains(savedRecipient.receivingFederateId)) {
        throw std::logic_error(
            "The saved TSO attribute-update payload has an invalid recipient.");
      }
      auto& passels = message.passelsByRecipient[savedRecipient.receivingFederateId];
      passels.reserve(savedRecipient.passels.size());
      for (auto const& savedPassel : savedRecipient.passels) {
        auto const preferredOrder = decodeSavedOrderType(
            savedPassel.preferredOrderType);
        if (!preferredOrder) {
          throw std::logic_error(
              "The saved TSO attribute-update passel has an invalid order type.");
        }
        TsoAttributeUpdatePassel passel;
        passel.transportationName = savedPassel.transportationName;
        passel.defaultRegionUsed = savedPassel.defaultRegionUsed;
        passel.preferredOrderType = *preferredOrder;
        passel.sentAttributeHandles.assign(
            savedPassel.sentAttributeHandles.begin(),
            savedPassel.sentAttributeHandles.end());
        passel.sentRegionHandles.insert(
            savedPassel.sentRegionHandles.begin(),
            savedPassel.sentRegionHandles.end());
        if (passel.sentAttributeHandles.size() !=
                savedPassel.sentAttributeHandles.size() ||
            passel.sentRegionHandles.size() !=
                savedPassel.sentRegionHandles.size()) {
          throw std::logic_error(
              "The saved TSO attribute-update passel has duplicate handles.");
        }
        if (!std::is_sorted(
                passel.sentAttributeHandles.begin(),
                passel.sentAttributeHandles.end()) ||
            std::adjacent_find(
                passel.sentAttributeHandles.begin(),
                passel.sentAttributeHandles.end()) !=
                passel.sentAttributeHandles.end()) {
          throw std::logic_error(
              "The saved TSO attribute-update passel has unordered attributes.");
        }
        for (auto const attributeHandle : passel.sentAttributeHandles) {
          if (attributeHandle == 0U || !attributeHandles.contains(attributeHandle)) {
            throw std::logic_error(
                "The saved TSO attribute-update passel has an unknown attribute.");
          }
        }
        for (auto const regionHandle : passel.sentRegionHandles) {
          if (regionHandle == 0U) {
            throw std::logic_error(
                "The saved TSO attribute-update passel has an invalid region.");
          }
          explicitRegionHandles.insert(regionHandle);
        }
        for (auto const& savedSnapshot : savedPassel.sentRegionSnapshots) {
          if (!passel.sentRegionHandles.contains(savedSnapshot.regionHandle)) {
            throw std::logic_error(
                "The saved TSO attribute-update passel has an unknown region snapshot.");
          }
          auto const [position, inserted] = passel.sentRegionSnapshots.emplace(
              savedSnapshot.regionHandle,
              decodeSavedRegionSnapshot(savedSnapshot));
          static_cast<void>(position);
          if (!inserted) {
            throw std::logic_error(
                "The saved TSO attribute-update passel has duplicate region snapshots.");
          }
        }
        if (passel.sentRegionSnapshots.size() != passel.sentRegionHandles.size()) {
          throw std::logic_error(
              "The saved TSO attribute-update passel has incomplete region snapshots.");
        }
        passels.push_back(std::move(passel));
      }
    }
    for (auto const& savedSnapshot : savedMessage.sentRegionSnapshots) {
      if (!explicitRegionHandles.contains(savedSnapshot.regionHandle)) {
        throw std::logic_error(
            "The saved TSO attribute-update message has an unknown region snapshot.");
      }
      auto const [position, inserted] = message.sentRegionSnapshots.emplace(
          savedSnapshot.regionHandle,
          decodeSavedRegionSnapshot(savedSnapshot));
      static_cast<void>(position);
      if (!inserted) {
        throw std::logic_error(
            "The saved TSO attribute-update message has duplicate region snapshots.");
      }
    }
    if (message.sentRegionSnapshots.size() != explicitRegionHandles.size()) {
      throw std::logic_error(
          "The saved TSO attribute-update message has incomplete region snapshots.");
    }
    message.timestamp = std::move(sentTimestamp);
    auto const [position, inserted] = attributeUpdates.emplace(
        savedMessage.messageId,
        std::move(message));
    static_cast<void>(position);
    if (!inserted) {
      throw std::logic_error(
          "The saved TSO attribute-update payload contains duplicate message IDs.");
    }
  }

  std::map<std::uint64_t, TsoDirectedInteractionMessage> directedInteractions;
  for (auto const& savedMessage : image.tsoDirectedInteractionMessages) {
    if (savedMessage.messageId == 0U || savedMessage.producingFederateId == 0U ||
        savedMessage.objectInstanceHandle == 0U ||
        savedMessage.sentInteractionClassHandle == 0U ||
        !savedMessage.timestampEncoding.has_value() ||
        savedMessage.sentParameterHandles.size() != savedMessage.parameters.size() ||
        savedMessage.recipients.empty()) {
      throw std::logic_error(
          "The saved directed TSO payload has incomplete identity or recipients.");
    }
    auto const sentTimestamp = decodeLogicalTimeEncoding(
        image.logicalTimeImplementationName,
        savedMessage.timestampEncoding);
    if (!sentTimestamp || sentTimestamp->isInitial() || sentTimestamp->isFinal()) {
      throw std::logic_error(
          "The saved directed TSO payload has an invalid timestamp.");
    }
    auto const sentOrder = decodeSavedOrderType(savedMessage.sentOrderType);
    auto const receivedOrder = decodeSavedOrderType(savedMessage.receivedOrderType);
    if (!sentOrder || !receivedOrder) {
      throw std::logic_error(
          "The saved directed TSO payload has an invalid order type.");
    }

    std::set<std::uint64_t> sentParameterHandles;
    for (auto const parameterHandle : savedMessage.sentParameterHandles) {
      if (parameterHandle == 0U ||
          !sentParameterHandles.insert(parameterHandle).second) {
        throw std::logic_error(
            "The saved directed TSO payload has duplicate parameter handles.");
      }
    }

    TsoDirectedInteractionMessage message;
    message.messageId = savedMessage.messageId;
    message.producingFederateId = savedMessage.producingFederateId;
    message.objectInstanceHandle = savedMessage.objectInstanceHandle;
    message.sentInteractionClassHandle = savedMessage.sentInteractionClassHandle;
    message.sentParameterHandles = savedMessage.sentParameterHandles;
    message.parameters.reserve(savedMessage.parameters.size());
    for (auto const& savedParameter : savedMessage.parameters) {
      if (savedParameter.parameterHandle == 0U ||
          !sentParameterHandles.contains(savedParameter.parameterHandle)) {
        throw std::logic_error(
            "The saved directed TSO payload contains an unknown parameter.");
      }
      message.parameters.emplace_back(
          savedParameter.parameterHandle,
          variableLengthDataFromBytes(savedParameter.value));
    }
    message.userSuppliedTag =
        variableLengthDataFromBytes(savedMessage.userSuppliedTag);
    message.transportationName = savedMessage.transportationName;
    message.timestamp = std::move(sentTimestamp);
    message.sentOrderType = *sentOrder;
    message.receivedOrderType = *receivedOrder;

    std::set<std::uint64_t> recipientIds;
    message.recipients.reserve(savedMessage.recipients.size());
    for (auto const& savedRecipient : savedMessage.recipients) {
      if (savedRecipient.receivingFederateId == 0U ||
          savedRecipient.receivingFederateId == message.producingFederateId ||
          savedRecipient.objectInstanceHandle == 0U ||
          savedRecipient.receivedInteractionClassHandle == 0U ||
          !recipientIds.insert(savedRecipient.receivingFederateId).second) {
        throw std::logic_error(
            "The saved directed TSO payload has an invalid recipient.");
      }

      // A recipient may have crossed its callback boundary and resigned
      // after the send but before the save.  Its immutable accepted-send
      // projection remains in the durable image so the retraction ledger can
      // preserve the delivered/retracted state, but there is no live route to
      // rebind after a fresh-registry restore.  Skip only that departed route;
      // still-joined recipients retain their exact projected callback data.
      if (!liveFederation.members.contains(savedRecipient.receivingFederateId)) {
        continue;
      }
      auto const liveRoute = liveFederation.interactionCallbackRoutes.find(
          savedRecipient.receivingFederateId);
      if (liveRoute == liveFederation.interactionCallbackRoutes.end() ||
          !liveRoute->second) {
        throw std::logic_error(
            "The saved directed TSO payload has no live callback route.");
      }
      TsoDirectedInteractionRecipient recipient;
      recipient.receivingFederateId = savedRecipient.receivingFederateId;
      recipient.objectInstanceHandle = savedRecipient.objectInstanceHandle;
      recipient.receivedInteractionClassHandle =
          savedRecipient.receivedInteractionClassHandle;
      recipient.receivedParameterHandles.insert(
          savedRecipient.receivedParameterHandles.begin(),
          savedRecipient.receivedParameterHandles.end());
      if (recipient.receivedParameterHandles.size() !=
          savedRecipient.receivedParameterHandles.size()) {
        throw std::logic_error(
            "The saved directed TSO payload has duplicate recipient parameters.");
      }
      recipient.callbackRoute = liveRoute->second;
      message.recipients.push_back(std::move(recipient));
    }
    auto const [position, inserted] = directedInteractions.emplace(
        message.messageId,
        std::move(message));
    static_cast<void>(position);
    if (!inserted) {
      throw std::logic_error(
          "The saved directed TSO payload contains duplicate message IDs.");
    }
  }

  // The primary object ledger carries the live timestamped-deletion marker;
  // the payload section carries the message and recipient retraction state.
  // Bind them before replacing the execution maps so a restore cannot leave a
  // pending removal pointing at an unrelated or already-reclaimed message.
  for (auto const& [objectInstanceHandle, object] : federation.objectInstances) {
    if (!object.pendingTimestampedDeletionMessageId.has_value()) {
      if (!object.pendingTimestampedRemovalFederates.empty()) {
        throw std::logic_error(
            "The saved object lifecycle ledger has timestamped recipients without a deletion message.");
      }
      continue;
    }
    auto const messageId = *object.pendingTimestampedDeletionMessageId;
    auto const deletion = objectDeletions.find(messageId);
    if (deletion == objectDeletions.end() ||
        deletion->second.objectInstanceHandle != objectInstanceHandle ||
        deletion->second.producingFederateId != object.producingFederateId) {
      throw std::logic_error(
          "The saved object lifecycle ledger is not bound to its TSO deletion payload.");
    }
    auto const retraction = retractionRecords.find(messageId);
    if (retraction == retractionRecords.end()) {
      throw std::logic_error(
          "The saved object lifecycle ledger has no TSO deletion retraction record.");
    }
    std::set<std::uint64_t> recipientIds;
    for (auto const& recipient : deletion->second.recipients) {
      if (!recipientIds.insert(recipient.receivingFederateId).second) {
        throw std::logic_error(
            "The saved TSO deletion payload has duplicate recipient identities.");
      }
    }
    for (auto const receivingFederateId : object.pendingTimestampedRemovalFederates) {
      if (!recipientIds.contains(receivingFederateId)) {
        throw std::logic_error(
            "The saved object lifecycle ledger has a timestamped recipient outside its payload.");
      }
      auto const recipientState = retraction->second.recipientStates.find(
          receivingFederateId);
      if (recipientState == retraction->second.recipientStates.end() ||
          recipientState->second != Federation::TsoRecipientDeliveryState::pending) {
        throw std::logic_error(
            "The saved object lifecycle ledger has a timestamped recipient that is not pending.");
      }
    }
  }
  for (auto const& [messageId, deletion] : objectDeletions) {
    auto const object = federation.objectInstances.find(
        deletion.objectInstanceHandle);
    if (object == federation.objectInstances.end() ||
        object->second.pendingTimestampedDeletionMessageId != messageId) {
      throw std::logic_error(
          "The saved TSO deletion payload has no matching object lifecycle marker.");
    }
  }

  federation.tsoInteractionMessages = std::move(interactions);
  federation.tsoRequestRetractionRecords = std::move(retractionRecords);
  federation.tsoAttributeUpdateMessages = std::move(attributeUpdates);
  federation.tsoObjectDeletionMessages = std::move(objectDeletions);
  federation.tsoObjectDeletionReconstitutionRecords =
      std::move(objectDeletionReconstitutions);
  federation.tsoDirectedInteractionMessages = std::move(directedInteractions);
}

} // namespace umbra::detail
