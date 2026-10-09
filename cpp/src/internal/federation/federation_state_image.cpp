#include "internal/federation/federation_state_image_codec_support.hpp"

namespace umbra::detail {
using namespace federation_state_image_codec_support;

std::string FederationStateImageCodec::encode(FederationStateImage const& image) {
  auto members = image.members;
  std::sort(
      members.begin(),
      members.end(),
      [](auto const& first, auto const& second) { return first.id < second.id; });
  validateMemberVector(members);

  auto names = image.federateNamesById;
  std::sort(
      names.begin(),
      names.end(),
      [](auto const& first, auto const& second) { return first.first < second.first; });
  validateNameVector(names);

  auto reservedObjectInstanceNames = image.reservedObjectInstanceNames;
  std::sort(
      reservedObjectInstanceNames.begin(),
      reservedObjectInstanceNames.end(),
      [](auto const& first, auto const& second) {
        return std::pair{first.objectInstanceName, first.federateId} <
            std::pair{second.objectInstanceName, second.federateId};
      });
  validateObjectInstanceNameReservationVector(reservedObjectInstanceNames);

  auto synchronizationPoints = image.synchronizationPoints;
  std::sort(
      synchronizationPoints.begin(),
      synchronizationPoints.end(),
      [](auto const& first, auto const& second) {
        return first.label < second.label;
      });
  for (auto& point : synchronizationPoints) {
    std::sort(point.synchronizationSet.begin(), point.synchronizationSet.end());
    std::sort(point.announcedFederates.begin(), point.announcedFederates.end());
    std::sort(
        point.achievedFederates.begin(),
        point.achievedFederates.end(),
        [](auto const& first, auto const& second) {
          return first.first < second.first;
        });
  }
  validateSynchronizationPointVector(synchronizationPoints);
  if (image.synchronizationPointCount != synchronizationPoints.size()) {
    throw std::logic_error(
        "Synchronization-point count does not match the typed state-image section.");
  }

  auto regions = image.regions;
  std::sort(
      regions.begin(),
      regions.end(),
      [](auto const& first, auto const& second) {
        return first.handle < second.handle;
      });
  for (auto& region : regions) {
    std::sort(region.dimensionHandles.begin(), region.dimensionHandles.end());
    std::sort(
        region.pendingRangeBounds.begin(),
        region.pendingRangeBounds.end(),
        [](auto const& first, auto const& second) {
          return first.dimensionHandle < second.dimensionHandle;
        });
    std::sort(
        region.committedRangeBounds.begin(),
        region.committedRangeBounds.end(),
        [](auto const& first, auto const& second) {
          return first.dimensionHandle < second.dimensionHandle;
        });
  }
  validateRegionVector(regions);
  if (image.regionCount != regions.size()) {
    throw std::logic_error(
        "Region count does not match the typed state-image section.");
  }

  auto objectClassAttributeDeclarations = image.objectClassAttributeDeclarations;
  std::sort(
      objectClassAttributeDeclarations.begin(),
      objectClassAttributeDeclarations.end(),
      [](auto const& first, auto const& second) {
        return first.federateId < second.federateId;
      });
  for (auto& declaration : objectClassAttributeDeclarations) {
    std::sort(
        declaration.classes.begin(),
        declaration.classes.end(),
        [](auto const& first, auto const& second) {
          return first.objectClassHandle < second.objectClassHandle;
        });
    for (auto& objectClass : declaration.classes) {
      std::sort(
          objectClass.explicitlyPublishedAttributeHandles.begin(),
          objectClass.explicitlyPublishedAttributeHandles.end());
      std::sort(
          objectClass.subscribedAttributes.begin(),
          objectClass.subscribedAttributes.end(),
          [](auto const& first, auto const& second) {
            return first.attributeHandle < second.attributeHandle;
          });
      std::sort(
          objectClass.subscribedUpdateRateDesignators.begin(),
          objectClass.subscribedUpdateRateDesignators.end(),
          [](auto const& first, auto const& second) {
            return first.attributeHandle < second.attributeHandle;
          });
      std::sort(
          objectClass.regionalSubscribedAttributes.begin(),
          objectClass.regionalSubscribedAttributes.end(),
          [](auto const& first, auto const& second) {
            return std::pair{first.attributeHandle, first.regionHandle} <
                std::pair{second.attributeHandle, second.regionHandle};
          });
      std::sort(
          objectClass.regionalSubscribedUpdateRateDesignators.begin(),
          objectClass.regionalSubscribedUpdateRateDesignators.end(),
          [](auto const& first, auto const& second) {
            return std::pair{first.attributeHandle, first.regionHandle} <
                std::pair{second.attributeHandle, second.regionHandle};
          });
      std::sort(
          objectClass.defaultTransportationTypes.begin(),
          objectClass.defaultTransportationTypes.end(),
          [](auto const& first, auto const& second) {
            return first.attributeHandle < second.attributeHandle;
          });
      std::sort(
          objectClass.defaultOrderTypes.begin(),
          objectClass.defaultOrderTypes.end(),
          [](auto const& first, auto const& second) {
            return first.attributeHandle < second.attributeHandle;
          });
    }
  }
  validateObjectClassAttributeDeclarationVector(objectClassAttributeDeclarations);
  if (image.objectClassDeclarationCount != objectClassAttributeDeclarations.size()) {
    throw std::logic_error(
        "Object-class declaration count does not match the typed state-image section.");
  }

  auto timeStates = image.timeStates;
  std::sort(
      timeStates.begin(),
      timeStates.end(),
      [](auto const& first, auto const& second) {
        return first.federateId < second.federateId;
      });
  validateTimeVector(timeStates);

  auto objects = image.objects;
  std::sort(
      objects.begin(),
      objects.end(),
      [](auto const& first, auto const& second) {
        return first.handle < second.handle;
      });
  for (auto& object : objects) {
    std::sort(
        object.attributes.begin(),
        object.attributes.end(),
        [](auto const& first, auto const& second) {
          return first.handle < second.handle;
        });
    std::sort(
        object.pendingAttributeValueUpdateRequests.begin(),
        object.pendingAttributeValueUpdateRequests.end(),
        [](auto const& first, auto const& second) {
          return first.requestId < second.requestId;
        });
    for (auto& request : object.pendingAttributeValueUpdateRequests) {
      std::sort(
          request.requestedAttributeHandles.begin(),
          request.requestedAttributeHandles.end());
    }
    std::sort(
        object.pendingAttributeValueUpdateClassRequests.begin(),
        object.pendingAttributeValueUpdateClassRequests.end(),
        [](auto const& first, auto const& second) {
          return first.requestId < second.requestId;
        });
    for (auto& request : object.pendingAttributeValueUpdateClassRequests) {
      std::sort(
          request.requestedAttributeHandles.begin(),
          request.requestedAttributeHandles.end());
    }
    std::sort(
        object.pendingAttributeValueUpdateRegionalRequests.begin(),
        object.pendingAttributeValueUpdateRegionalRequests.end(),
        [](auto const& first, auto const& second) {
          return first.requestId < second.requestId;
        });
    for (auto& request : object.pendingAttributeValueUpdateRegionalRequests) {
      std::sort(
          request.requestedAttributeHandles.begin(),
          request.requestedAttributeHandles.end());
      std::sort(
          request.requestRegionsByAttribute.begin(),
          request.requestRegionsByAttribute.end(),
          [](auto const& first, auto const& second) {
            return first.first < second.first;
          });
      for (auto& [attributeHandle, regionHandles] :
           request.requestRegionsByAttribute) {
        static_cast<void>(attributeHandle);
        std::sort(regionHandles.begin(), regionHandles.end());
      }
    }
    std::sort(
        object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.begin(),
        object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.end(),
        [](auto const& first, auto const& second) {
          return first.requestId < second.requestId;
        });
    for (auto& request :
         object.pendingAttributeOwnershipAcquisitionIfAvailableRequests) {
      std::sort(
          request.desiredAttributeHandles.begin(),
          request.desiredAttributeHandles.end());
    }
    std::sort(
        object.pendingAttributeOwnershipAcquisitionRequests.begin(),
        object.pendingAttributeOwnershipAcquisitionRequests.end(),
        [](auto const& first, auto const& second) {
          return first.requestId < second.requestId;
        });
    for (auto& request : object.pendingAttributeOwnershipAcquisitionRequests) {
      std::sort(
          request.desiredAttributeHandles.begin(),
          request.desiredAttributeHandles.end());
      std::sort(
          request.notificationQueuedAttributeHandles.begin(),
          request.notificationQueuedAttributeHandles.end());
      std::sort(
          request.unavailableQueuedAttributeHandles.begin(),
          request.unavailableQueuedAttributeHandles.end());
      std::sort(
          request.releaseCallbacksQueuedByOwningFederate.begin(),
          request.releaseCallbacksQueuedByOwningFederate.end(),
          [](auto const& first, auto const& second) {
            return first.first < second.first;
          });
      for (auto& [ownerFederateId, attributes] :
           request.releaseCallbacksQueuedByOwningFederate) {
        static_cast<void>(ownerFederateId);
        std::sort(attributes.begin(), attributes.end());
      }
    }
    std::sort(
        object.pendingAttributeOwnershipAcquisitionCancellations.begin(),
        object.pendingAttributeOwnershipAcquisitionCancellations.end(),
        [](auto const& first, auto const& second) {
          return first.cancellationId < second.cancellationId;
        });
    for (auto& cancellation :
         object.pendingAttributeOwnershipAcquisitionCancellations) {
      std::sort(
          cancellation.attributeHandles.begin(),
          cancellation.attributeHandles.end());
    }
    std::sort(
        object.pendingAttributeOwnershipDivestitureIfWantedNotifications.begin(),
        object.pendingAttributeOwnershipDivestitureIfWantedNotifications.end(),
        [](auto const& first, auto const& second) {
          return first.notificationId < second.notificationId;
        });
    for (auto& notification :
         object.pendingAttributeOwnershipDivestitureIfWantedNotifications) {
      std::sort(
          notification.attributeHandles.begin(),
          notification.attributeHandles.end());
    }
    std::sort(
        object.pendingConfirmDivestitureNotifications.begin(),
        object.pendingConfirmDivestitureNotifications.end(),
        [](auto const& first, auto const& second) {
          return first.notificationId < second.notificationId;
        });
    for (auto& notification : object.pendingConfirmDivestitureNotifications) {
      std::sort(
          notification.attributeHandles.begin(),
          notification.attributeHandles.end());
    }
    std::sort(
        object.pendingAttributeTransportationTypeChanges.begin(),
        object.pendingAttributeTransportationTypeChanges.end(),
        [](auto const& first, auto const& second) {
          return first.requestId < second.requestId;
        });
    for (auto& request : object.pendingAttributeTransportationTypeChanges) {
      std::sort(request.attributeHandles.begin(), request.attributeHandles.end());
    }
    std::sort(
        object.pendingNegotiatedAttributeOwnershipDivestitures.begin(),
        object.pendingNegotiatedAttributeOwnershipDivestitures.end(),
        [](auto const& first, auto const& second) {
          return first.attributeHandle < second.attributeHandle;
        });
    std::sort(
        object.ownershipAssumptionRecipientsByAttribute.begin(),
        object.ownershipAssumptionRecipientsByAttribute.end(),
        [](auto const& first, auto const& second) {
          return first.attributeHandle < second.attributeHandle;
        });
    for (auto& assumption : object.ownershipAssumptionRecipientsByAttribute) {
      std::sort(
          assumption.recipientFederateIds.begin(),
          assumption.recipientFederateIds.end());
    }
    std::sort(
        object.ownershipAssumptionUserSuppliedTagsByAttribute.begin(),
        object.ownershipAssumptionUserSuppliedTagsByAttribute.end(),
        [](auto const& first, auto const& second) {
          return first.attributeHandle < second.attributeHandle;
        });
    std::sort(
        object.knownObjectClassHandlesByFederate.begin(),
        object.knownObjectClassHandlesByFederate.end(),
        [](auto const& first, auto const& second) {
          return first.federateId < second.federateId;
        });
    std::sort(
        object.pendingDiscoveryFederateIds.begin(),
        object.pendingDiscoveryFederateIds.end());
    std::sort(
        object.pendingRemovalFederateIds.begin(),
        object.pendingRemovalFederateIds.end());
  }
  validateObjectVector(objects);

  auto deferredUpdateRegionAssociations = image.deferredUpdateRegionAssociations;
  std::sort(
      deferredUpdateRegionAssociations.begin(),
      deferredUpdateRegionAssociations.end(),
      [](auto const& first, auto const& second) {
        return std::tuple{
                   first.objectInstanceHandle,
                   first.federateId,
                   first.attributeHandle} <
               std::tuple{
                   second.objectInstanceHandle,
                   second.federateId,
                   second.attributeHandle};
      });
  for (auto& association : deferredUpdateRegionAssociations) {
    std::sort(association.regionHandles.begin(), association.regionHandles.end());
  }
  validateDeferredUpdateRegionAssociationVector(
      deferredUpdateRegionAssociations,
      objects);

  auto pendingAttributeOwnershipQueries =
      image.pendingAttributeOwnershipQueries;
  std::sort(
      pendingAttributeOwnershipQueries.begin(),
      pendingAttributeOwnershipQueries.end(),
      [](auto const& first, auto const& second) {
        return first.requestId < second.requestId;
      });
  for (auto& query : pendingAttributeOwnershipQueries) {
    std::sort(
        query.requestedAttributeHandles.begin(),
        query.requestedAttributeHandles.end());
  }
  validatePendingAttributeOwnershipQueryVector(
      pendingAttributeOwnershipQueries);

  auto pendingAttributeOwnershipAssumptions =
      image.pendingAttributeOwnershipAssumptions;
  std::sort(
      pendingAttributeOwnershipAssumptions.begin(),
      pendingAttributeOwnershipAssumptions.end(),
      [](auto const& first, auto const& second) {
        return std::tuple{
                   first.objectInstanceHandle,
                   first.receivingFederateId,
                   first.attributeHandles,
                   first.userSuppliedTag} <
            std::tuple{
                second.objectInstanceHandle,
                second.receivingFederateId,
                second.attributeHandles,
                second.userSuppliedTag};
      });
  for (auto& callback : pendingAttributeOwnershipAssumptions) {
    std::sort(callback.attributeHandles.begin(), callback.attributeHandles.end());
  }
  validatePendingAttributeOwnershipAssumptionVector(
      pendingAttributeOwnershipAssumptions);

  auto interactionDeclarations = image.interactionDeclarations;
  std::sort(
      interactionDeclarations.begin(),
      interactionDeclarations.end(),
      [](auto const& first, auto const& second) {
        return first.federateId < second.federateId;
      });
  for (auto& declaration : interactionDeclarations) {
    std::sort(
        declaration.publishedInteractionClasses.begin(),
        declaration.publishedInteractionClasses.end());
    std::sort(
        declaration.subscribedInteractionClasses.begin(),
        declaration.subscribedInteractionClasses.end(),
        [](auto const& first, auto const& second) {
          return first.interactionClassHandle < second.interactionClassHandle;
        });
    std::sort(
        declaration.regionalSubscribedInteractionClasses.begin(),
        declaration.regionalSubscribedInteractionClasses.end(),
        [](auto const& first, auto const& second) {
          return std::pair{first.interactionClassHandle, first.regionHandle} <
              std::pair{second.interactionClassHandle, second.regionHandle};
        });
    std::sort(
        declaration.publishedObjectClassDirectedInteractions.begin(),
        declaration.publishedObjectClassDirectedInteractions.end(),
        [](auto const& first, auto const& second) {
          return std::pair{first.objectClassHandle, first.interactionClassHandle} <
              std::pair{second.objectClassHandle, second.interactionClassHandle};
        });
    std::sort(
        declaration.subscribedObjectClassDirectedInteractions.begin(),
        declaration.subscribedObjectClassDirectedInteractions.end(),
        [](auto const& first, auto const& second) {
          return std::pair{first.objectClassHandle, first.interactionClassHandle} <
              std::pair{second.objectClassHandle, second.interactionClassHandle};
        });
    std::sort(
        declaration.interactionTransportationTypes.begin(),
        declaration.interactionTransportationTypes.end(),
        [](auto const& first, auto const& second) {
          return first.interactionClassHandle < second.interactionClassHandle;
        });
    std::sort(
        declaration.interactionOrderTypes.begin(),
        declaration.interactionOrderTypes.end(),
        [](auto const& first, auto const& second) {
          return first.interactionClassHandle < second.interactionClassHandle;
        });
    std::sort(
        declaration.pendingInteractionTransportationTypeChanges.begin(),
        declaration.pendingInteractionTransportationTypeChanges.end(),
        [](auto const& first, auto const& second) {
          return first.interactionClassHandle < second.interactionClassHandle;
        });
  }
  validateInteractionDeclarationVector(interactionDeclarations);

  auto tsoInteractionMessages = image.tsoInteractionMessages;
  std::sort(
      tsoInteractionMessages.begin(),
      tsoInteractionMessages.end(),
      [](auto const& first, auto const& second) {
        return first.messageId < second.messageId;
      });
  for (auto& message : tsoInteractionMessages) {
    std::sort(
        message.sentParameterHandles.begin(),
        message.sentParameterHandles.end());
    std::sort(
        message.parameters.begin(),
        message.parameters.end(),
        [](auto const& first, auto const& second) {
          return first.parameterHandle < second.parameterHandle;
        });
    std::sort(message.sentRegionHandles.begin(), message.sentRegionHandles.end());
    std::sort(
        message.sentRegionSnapshots.begin(),
        message.sentRegionSnapshots.end(),
        [](auto const& first, auto const& second) {
          return first.regionHandle < second.regionHandle;
        });
    for (auto& snapshot : message.sentRegionSnapshots) {
      std::sort(snapshot.dimensionHandles.begin(), snapshot.dimensionHandles.end());
      std::sort(
          snapshot.committedRangeBounds.begin(),
          snapshot.committedRangeBounds.end(),
          [](auto const& first, auto const& second) {
            return first.dimensionHandle < second.dimensionHandle;
          });
    }
  }
  validateTsoInteractionMessageVector(tsoInteractionMessages);

  auto tsoAttributeUpdateMessages = image.tsoAttributeUpdateMessages;
  std::sort(
      tsoAttributeUpdateMessages.begin(),
      tsoAttributeUpdateMessages.end(),
      [](auto const& first, auto const& second) {
        return first.messageId < second.messageId;
      });
  for (auto& message : tsoAttributeUpdateMessages) {
    std::sort(
        message.attributes.begin(),
        message.attributes.end(),
        [](auto const& first, auto const& second) {
          return first.attributeHandle < second.attributeHandle;
        });
    std::sort(
        message.passelsByRecipient.begin(),
        message.passelsByRecipient.end(),
        [](auto const& first, auto const& second) {
          return first.receivingFederateId < second.receivingFederateId;
        });
    for (auto& recipient : message.passelsByRecipient) {
      for (auto& passel : recipient.passels) {
        std::sort(
            passel.sentAttributeHandles.begin(),
            passel.sentAttributeHandles.end());
        std::sort(
            passel.sentRegionHandles.begin(),
            passel.sentRegionHandles.end());
        std::sort(
            passel.sentRegionSnapshots.begin(),
            passel.sentRegionSnapshots.end(),
            [](auto const& first, auto const& second) {
              return first.regionHandle < second.regionHandle;
            });
        for (auto& snapshot : passel.sentRegionSnapshots) {
          std::sort(snapshot.dimensionHandles.begin(), snapshot.dimensionHandles.end());
          std::sort(
              snapshot.committedRangeBounds.begin(),
              snapshot.committedRangeBounds.end(),
              [](auto const& first, auto const& second) {
                return first.dimensionHandle < second.dimensionHandle;
              });
        }
      }
    }
    std::sort(
        message.sentRegionSnapshots.begin(),
        message.sentRegionSnapshots.end(),
        [](auto const& first, auto const& second) {
          return first.regionHandle < second.regionHandle;
        });
    for (auto& snapshot : message.sentRegionSnapshots) {
      std::sort(snapshot.dimensionHandles.begin(), snapshot.dimensionHandles.end());
      std::sort(
          snapshot.committedRangeBounds.begin(),
          snapshot.committedRangeBounds.end(),
          [](auto const& first, auto const& second) {
            return first.dimensionHandle < second.dimensionHandle;
          });
    }
  }
  validateTsoAttributeUpdateMessageVector(tsoAttributeUpdateMessages);

  auto tsoObjectDeletionMessages = image.tsoObjectDeletionMessages;
  std::sort(
      tsoObjectDeletionMessages.begin(),
      tsoObjectDeletionMessages.end(),
      [](auto const& first, auto const& second) {
        return first.messageId < second.messageId;
      });
  for (auto& message : tsoObjectDeletionMessages) {
    std::sort(
        message.recipients.begin(),
        message.recipients.end(),
        [](auto const& first, auto const& second) {
          return first.receivingFederateId < second.receivingFederateId;
        });
  }
  validateTsoObjectDeletionMessageVector(tsoObjectDeletionMessages);

  auto tsoRequestRetractionRecords = image.tsoRequestRetractionRecords;
  std::sort(
      tsoRequestRetractionRecords.begin(),
      tsoRequestRetractionRecords.end(),
      [](auto const& first, auto const& second) {
        return first.messageId < second.messageId;
      });
  for (auto& record : tsoRequestRetractionRecords) {
    std::sort(
        record.recipientStates.begin(),
        record.recipientStates.end(),
        [](auto const& first, auto const& second) {
          return first.receivingFederateId < second.receivingFederateId;
        });
  }
  validateTsoRequestRetractionRecordVector(tsoRequestRetractionRecords);

  auto tsoDirectedInteractionMessages = image.tsoDirectedInteractionMessages;
  std::sort(
      tsoDirectedInteractionMessages.begin(),
      tsoDirectedInteractionMessages.end(),
      [](auto const& first, auto const& second) {
        return first.messageId < second.messageId;
      });
  for (auto& message : tsoDirectedInteractionMessages) {
    std::sort(
        message.sentParameterHandles.begin(),
        message.sentParameterHandles.end());
    std::sort(
        message.parameters.begin(),
        message.parameters.end(),
        [](auto const& first, auto const& second) {
          return first.parameterHandle < second.parameterHandle;
        });
    std::sort(
        message.recipients.begin(),
        message.recipients.end(),
        [](auto const& first, auto const& second) {
          return first.receivingFederateId < second.receivingFederateId;
        });
    for (auto& recipient : message.recipients) {
      std::sort(
          recipient.receivedParameterHandles.begin(),
          recipient.receivedParameterHandles.end());
    }
  }
  validateTsoDirectedInteractionMessageVector(tsoDirectedInteractionMessages);

  auto tsoQueueEntries = image.tsoQueueEntries;
  std::sort(
      tsoQueueEntries.begin(),
      tsoQueueEntries.end(),
      [](auto const& first, auto const& second) {
        if (first.recipientFederateId != second.recipientFederateId) {
          return first.recipientFederateId < second.recipientFederateId;
        }
        if (first.phase != second.phase) {
          return first.phase < second.phase;
        }
        if (first.sequence != second.sequence) {
          return first.sequence < second.sequence;
        }
        return first.messageId < second.messageId;
      });
  validateTsoQueueEntryVector(tsoQueueEntries);

  std::string result;
  result.reserve(
      512U + members.size() * 80U + names.size() * 40U +
      timeStates.size() * 220U + objects.size() * 160U +
      interactionDeclarations.size() * 180U + tsoInteractionMessages.size() * 260U +
      tsoAttributeUpdateMessages.size() * 300U +
      tsoObjectDeletionMessages.size() * 180U +
      tsoRequestRetractionRecords.size() * 140U +
      tsoDirectedInteractionMessages.size() * 260U + tsoQueueEntries.size() * 80U +
      pendingAttributeOwnershipQueries.size() * 100U +
      pendingAttributeOwnershipAssumptions.size() * 120U +
      deferredUpdateRegionAssociations.size() * 96U +
      reservedObjectInstanceNames.size() * 80U +
      synchronizationPoints.size() * 180U +
      regions.size() * 180U +
      objectClassAttributeDeclarations.size() * 180U +
      (image.saveHistoryPresent ? 120U : 0U));
  result += FederationStateImage::format;
  result += '\n';
  result += "federationName=";
  result += encodeWide(image.federationName);
  result += '\n';
  result += "logicalTimeImplementation=";
  result += encodeWide(image.logicalTimeImplementationName);
  result += '\n';
  appendScalar(result, "normalizationSeed", image.normalizationSeed);
  appendScalar(result, "federationSwitches", image.federationSwitches);
  result += "sectionCounts=";
  result += std::to_string(image.interactionDeclarationCount);
  result += ',';
  result += std::to_string(image.synchronizationPointCount);
  result += ',';
  result += std::to_string(image.objectClassDeclarationCount);
  result += ',';
  result += std::to_string(image.regionCount);
  result += ',';
  result += std::to_string(image.objectInstanceCount);
  result += ',';
  result += std::to_string(image.tsoInteractionMessageCount);
  result += ',';
  result += std::to_string(image.tsoAttributeUpdateMessageCount);
  result += ',';
  result += std::to_string(image.tsoObjectDeletionMessageCount);
  result += ',';
  result += std::to_string(image.tsoDirectedInteractionMessageCount);
  result += '\n';
  result += "allocators=";
  result += std::to_string(image.nextRegionHandle);
  result += ',';
  result += std::to_string(image.nextSubscriptionGeneration);
  result += ',';
  result += std::to_string(image.nextObjectInstanceHandle);
  result += ',';
  result += std::to_string(image.nextAttributeOwnershipAcquisitionIfAvailableRequestId);
  result += ',';
  result += std::to_string(image.nextAttributeOwnershipAcquisitionRequestId);
  result += ',';
  result += std::to_string(image.nextAttributeOwnershipAcquisitionRequestSequence);
  result += ',';
  result += std::to_string(image.nextAttributeOwnershipAcquisitionCancellationId);
  result += ',';
  result += std::to_string(image.nextAttributeOwnershipDivestitureIfWantedNotificationId);
  result += ',';
  result += std::to_string(image.nextConfirmDivestitureNotificationId);
  result += ',';
  result += std::to_string(image.nextAttributeTransportationTypeChangeRequestId);
  result += ',';
  result += std::to_string(image.nextAttributeValueUpdateRequestId);
  result += ',';
  result += std::to_string(image.nextAttributeOwnershipQueryRequestId);
  result += ',';
  result += std::to_string(image.nextTimeAdvanceGrantDispatchIdentity);
  result += '\n';

  appendScalar(result, "members", members.size());
  for (auto const& member : members) {
    result += "member=";
    result += std::to_string(member.id);
    result += '|';
    result += encodeWide(member.name);
    result += '|';
    result += encodeWide(member.type);
    result += '|';
    result += std::to_string(member.switches);
    result += '|';
    result += std::to_string(member.automaticResignAction);
    result += '|';
    result += std::to_string(member.momReportPeriodSeconds);
    result += '|';
    result += std::to_string(member.nextMomServiceReportSerialNumber);
    result += '|';
    result += std::to_string(member.successfulUpdateAttributeValuesCount);
    result += '|';
    result += std::to_string(
        member.successfulUpdateCountsByClassAndTransportation.size());
    result += '|';
    result += std::to_string(member.successfullyUpdatedObjectInstanceHandles.size());
    result += '|';
    result += std::to_string(
        member.successfullyUpdatedObjectInstanceClassHandles.size());
    result += '|';
    result += std::to_string(member.successfulReflectionsReceivedCount);
    result += '|';
    result += std::to_string(member.successfulObjectInstanceRegistrationsCount);
    result += '|';
    result += std::to_string(member.successfulObjectInstanceDeletionsCount);
    result += '|';
    result += std::to_string(member.successfulObjectInstanceRemovalsCount);
    result += '|';
    result += std::to_string(member.successfulObjectInstanceDiscoveriesCount);
    result += '|';
    result += std::to_string(
        member.successfulReflectionCountsByClassAndTransportation.size());
    result += '|';
    result += std::to_string(member.successfullyReflectedObjectInstanceHandles.size());
    result += '|';
    result += std::to_string(
        member.successfullyReflectedObjectInstanceClassHandles.size());
    result += '|';
    result += std::to_string(member.successfulInteractionsSentCount);
    result += '|';
    result += std::to_string(member.successfulDirectedInteractionsSentCount);
    result += '|';
    result += std::to_string(
        member.successfulInteractionCountsByClassAndTransportation.size());
    result += '|';
    result += std::to_string(
        member.successfulDirectedInteractionCountsByClassAndTransportation.size());
    result += '|';
    result += std::to_string(member.successfulInteractionsReceivedCount);
    result += '|';
    result += std::to_string(member.successfulDirectedInteractionsReceivedCount);
    result += '|';
    result += std::to_string(
        member.successfulInteractionReceiptCountsByClassAndTransportation.size());
    result += '|';
    result += std::to_string(
        member.successfulDirectedInteractionReceiptCountsByClassAndTransportation.size());
    result += '\n';
    for (auto const& update : member.successfulUpdateCountsByClassAndTransportation) {
      result += "memberUpdateCount=";
      result += std::to_string(member.id);
      result += '|';
      result += std::to_string(update.objectClassHandle);
      result += '|';
      result += hexEncode(update.transportationName);
      result += '|';
      result += std::to_string(update.count);
      result += '\n';
    }
    for (auto const objectInstanceHandle :
         member.successfullyUpdatedObjectInstanceHandles) {
      result += "memberUpdatedObject=";
      result += std::to_string(member.id);
      result += '|';
      result += std::to_string(objectInstanceHandle);
      result += '\n';
    }
    for (auto const& updatedObject :
         member.successfullyUpdatedObjectInstanceClassHandles) {
      result += "memberUpdatedObjectClass=";
      result += std::to_string(member.id);
      result += '|';
      result += std::to_string(updatedObject.objectInstanceHandle);
      result += '|';
      result += std::to_string(updatedObject.objectClassHandle);
      result += '\n';
    }
    for (auto const& reflection :
         member.successfulReflectionCountsByClassAndTransportation) {
      result += "memberReflectionCount=";
      result += std::to_string(member.id);
      result += '|';
      result += std::to_string(reflection.objectClassHandle);
      result += '|';
      result += hexEncode(reflection.transportationName);
      result += '|';
      result += std::to_string(reflection.count);
      result += '\n';
    }
    for (auto const objectInstanceHandle :
         member.successfullyReflectedObjectInstanceHandles) {
      result += "memberReflectedObject=";
      result += std::to_string(member.id);
      result += '|';
      result += std::to_string(objectInstanceHandle);
      result += '\n';
    }
    for (auto const& reflectedObject :
         member.successfullyReflectedObjectInstanceClassHandles) {
      result += "memberReflectedObjectClass=";
      result += std::to_string(member.id);
      result += '|';
      result += std::to_string(reflectedObject.objectInstanceHandle);
      result += '|';
      result += std::to_string(reflectedObject.objectClassHandle);
      result += '\n';
    }
    for (auto const& interaction :
         member.successfulInteractionCountsByClassAndTransportation) {
      result += "memberInteractionSendCount=";
      result += std::to_string(member.id);
      result += '|';
      result += std::to_string(interaction.interactionClassHandle);
      result += '|';
      result += hexEncode(interaction.transportationName);
      result += '|';
      result += std::to_string(interaction.count);
      result += '\n';
    }
    for (auto const& interaction :
         member.successfulDirectedInteractionCountsByClassAndTransportation) {
      result += "memberDirectedInteractionSendCount=";
      result += std::to_string(member.id);
      result += '|';
      result += std::to_string(interaction.interactionClassHandle);
      result += '|';
      result += hexEncode(interaction.transportationName);
      result += '|';
      result += std::to_string(interaction.count);
      result += '\n';
    }
    for (auto const& interaction :
         member.successfulInteractionReceiptCountsByClassAndTransportation) {
      result += "memberInteractionReceiptCount=";
      result += std::to_string(member.id);
      result += '|';
      result += std::to_string(interaction.interactionClassHandle);
      result += '|';
      result += hexEncode(interaction.transportationName);
      result += '|';
      result += std::to_string(interaction.count);
      result += '\n';
    }
    for (auto const& interaction :
         member.successfulDirectedInteractionReceiptCountsByClassAndTransportation) {
      result += "memberDirectedInteractionReceiptCount=";
      result += std::to_string(member.id);
      result += '|';
      result += std::to_string(interaction.interactionClassHandle);
      result += '|';
      result += hexEncode(interaction.transportationName);
      result += '|';
      result += std::to_string(interaction.count);
      result += '\n';
    }
  }

  appendScalar(result, "federateNames", names.size());
  for (auto const& [id, name] : names) {
    result += "name=";
    result += std::to_string(id);
    result += '|';
    result += encodeWide(name);
    result += '\n';
  }

  appendScalar(result, "timeStates", timeStates.size());
  for (auto const& state : timeStates) {
    result += "time=";
    result += std::to_string(state.federateId);
    result += '|';
    result += encodeWide(state.implementationName);
    result += '|';
    result += std::to_string(state.flags);
    result += '|';
    result += std::to_string(state.pendingGeneration);
    result += '|';
    result += std::to_string(state.advanceMode);
    result += '|';
    result += encodeOptional(state.currentTimeEncoding);
    result += '|';
    result += encodeOptional(state.optimisticTimeEncoding);
    result += '|';
    result += encodeOptional(state.requestedTimeEncoding);
    result += '|';
    result += encodeOptional(state.advanceRequestTimeEncoding);
    result += '|';
    result += encodeOptional(state.lookaheadEncoding);
    result += '|';
    result += encodeOptional(state.requestedLookaheadEncoding);
    result += '|';
    result += std::to_string(state.queuedTsoCount);
    result += '|';
    result += std::to_string(state.inTransitTsoCount);
    result += '|';
    result += std::to_string(state.deliveredTsoCount);
    result += '|';
    result += std::to_string(state.pendingTimeRegulationGeneration);
    result += '|';
    result += std::to_string(state.pendingTimeConstrainedGeneration);
    result += '|';
    result += std::to_string(state.nextGeneration);
    result += '|';
    result += encodeOptional(state.pendingModifiedLookaheadEncoding);
    result += '\n';
  }
  appendScalar(result, "objects", objects.size());
  for (auto const& object : objects) {
    result += "object=";
    result += std::to_string(object.handle);
    result += '|';
    result += encodeWide(object.name);
    result += '|';
    result += std::to_string(object.registeredObjectClassHandle);
    result += '|';
    result += std::to_string(object.producingFederateId);
    result += '|';
    result += (object.deleteAccepted ? "1" : "0");
    result += '|';
    result += std::to_string(object.pendingOperationCount);
    result += '|';
    result += std::to_string(object.attributes.size());
    result += '|';
    result += std::to_string(
        object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.size());
    result += '|';
    result += std::to_string(object.pendingAttributeOwnershipAcquisitionRequests.size());
    result += '|';
    result += std::to_string(
        object.pendingAttributeOwnershipAcquisitionCancellations.size());
    result += '|';
    result += std::to_string(
        object.pendingAttributeOwnershipDivestitureIfWantedNotifications.size());
    result += '|';
    result += std::to_string(object.pendingConfirmDivestitureNotifications.size());
    result += '|';
    result += std::to_string(object.pendingAttributeTransportationTypeChanges.size());
    result += '|';
    result += std::to_string(object.pendingNegotiatedAttributeOwnershipDivestitures.size());
    result += '|';
    result += std::to_string(object.ownershipAssumptionRecipientsByAttribute.size());
    result += '|';
    result += std::to_string(object.ownershipAssumptionUserSuppliedTagsByAttribute.size());
    result += '|';
    result += std::to_string(object.knownObjectClassHandlesByFederate.size());
    result += '|';
    result += std::to_string(object.pendingDiscoveryFederateIds.size());
    result += '|';
    result += std::to_string(object.pendingRemovalFederateIds.size());
    result += '|';
    result += std::to_string(object.connectionLossAutomaticRemovalFederateIds.size());
    result += '|';
    result += std::to_string(object.deferredConnectionLossTsoRemovalFederateIds.size());
    result += '|';
    result += std::to_string(object.pendingTimestampedRemovalFederateIds.size());
    result += '|';
    if (object.pendingTimestampedDeletionMessageId.has_value()) {
      result += std::to_string(*object.pendingTimestampedDeletionMessageId);
    } else {
      result += '-';
    }
    if (object.attributeValuesPresent || !object.attributeValues.empty() ||
        object.pendingAttributeValueUpdateRequestsPresent ||
        !object.pendingAttributeValueUpdateRequests.empty() ||
        object.pendingAttributeValueUpdateClassRequestsPresent ||
        !object.pendingAttributeValueUpdateClassRequests.empty() ||
        object.pendingAttributeValueUpdateRegionalRequestsPresent ||
        !object.pendingAttributeValueUpdateRegionalRequests.empty()) {
      result += '|';
      result += std::to_string(object.attributeValues.size());
    }
    if (object.pendingAttributeValueUpdateRequestsPresent ||
        !object.pendingAttributeValueUpdateRequests.empty() ||
        object.pendingAttributeValueUpdateClassRequestsPresent ||
        !object.pendingAttributeValueUpdateClassRequests.empty() ||
        object.pendingAttributeValueUpdateRegionalRequestsPresent ||
        !object.pendingAttributeValueUpdateRegionalRequests.empty()) {
      result += '|';
      result += std::to_string(object.pendingAttributeValueUpdateRequests.size());
    }
    if (object.pendingAttributeValueUpdateClassRequestsPresent ||
        !object.pendingAttributeValueUpdateClassRequests.empty() ||
        object.pendingAttributeValueUpdateRegionalRequestsPresent ||
        !object.pendingAttributeValueUpdateRegionalRequests.empty()) {
      result += '|';
      result += std::to_string(object.pendingAttributeValueUpdateClassRequests.size());
    }
    if (object.pendingAttributeValueUpdateRegionalRequestsPresent ||
        !object.pendingAttributeValueUpdateRegionalRequests.empty()) {
      result += '|';
      result += std::to_string(object.pendingAttributeValueUpdateRegionalRequests.size());
    }
    result += '\n';
    for (auto const& attribute : object.attributes) {
      result += "attribute=";
      result += std::to_string(attribute.handle);
      result += '|';
      result += std::to_string(attribute.ownerFederateId);
      result += '|';
      result += hexEncode(attribute.transportationName);
      result += '|';
      result += std::to_string(attribute.orderType);
      result += '|';
      for (std::size_t index = 0U; index < attribute.updateRegionHandles.size(); ++index) {
        if (index != 0U) {
          result += ',';
        }
        result += std::to_string(attribute.updateRegionHandles[index]);
      }
      result += '\n';
    }
    for (auto const& value : object.attributeValues) {
      result += "objectAttributeValue=";
      result += std::to_string(value.attributeHandle);
      result += '|';
      result += hexEncode(value.value);
      result += '\n';
    }
    for (auto const& request : object.pendingAttributeValueUpdateRequests) {
      result += "pendingAttributeValueUpdate=";
      result += std::to_string(request.requestId);
      result += '|';
      result += std::to_string(request.requestingFederateId);
      result += '|';
      result += std::to_string(request.providingFederateId);
      result += '|';
      result += hexEncode(request.userSuppliedTag);
      result += '|';
      for (std::size_t index = 0U;
           index < request.requestedAttributeHandles.size();
           ++index) {
        if (index != 0U) {
          result += ',';
        }
        result += std::to_string(request.requestedAttributeHandles[index]);
      }
      result += '\n';
    }
    for (auto const& request : object.pendingAttributeValueUpdateClassRequests) {
      result += "pendingAttributeValueUpdateClass=";
      result += std::to_string(request.requestId);
      result += '|';
      result += std::to_string(request.requestingFederateId);
      result += '|';
      result += std::to_string(request.providingFederateId);
      result += '|';
      result += std::to_string(request.requestedObjectClassHandle);
      result += '|';
      result += hexEncode(request.userSuppliedTag);
      result += '|';
      for (std::size_t index = 0U;
           index < request.requestedAttributeHandles.size();
           ++index) {
        if (index != 0U) {
          result += ',';
        }
        result += std::to_string(request.requestedAttributeHandles[index]);
      }
      result += '\n';
    }
    for (auto const& request : object.pendingAttributeValueUpdateRegionalRequests) {
      result += "pendingAttributeValueUpdateRegional=";
      result += std::to_string(request.requestId);
      result += '|';
      result += std::to_string(request.requestingFederateId);
      result += '|';
      result += std::to_string(request.providingFederateId);
      result += '|';
      result += std::to_string(request.requestedObjectClassHandle);
      result += '|';
      result += hexEncode(request.userSuppliedTag);
      result += '|';
      for (std::size_t index = 0U;
           index < request.requestedAttributeHandles.size();
           ++index) {
        if (index != 0U) {
          result += ',';
        }
        result += std::to_string(request.requestedAttributeHandles[index]);
      }
      result += '|';
      for (std::size_t index = 0U;
           index < request.requestRegionsByAttribute.size();
           ++index) {
        if (index != 0U) {
          result += ';';
        }
        auto const& [attributeHandle, regionHandles] =
            request.requestRegionsByAttribute[index];
        result += std::to_string(attributeHandle);
        result += ':';
        for (std::size_t regionIndex = 0U;
             regionIndex < regionHandles.size();
             ++regionIndex) {
          if (regionIndex != 0U) {
            result += ',';
          }
          result += std::to_string(regionHandles[regionIndex]);
        }
      }
      result += '\n';
    }
    for (auto const& request :
         object.pendingAttributeOwnershipAcquisitionIfAvailableRequests) {
      result += "pendingOwnershipIfAvailable=";
      result += std::to_string(request.requestId);
      result += '|';
      result += std::to_string(request.requestingFederateId);
      result += '|';
      result += std::to_string(request.requestSequence);
      result += '|';
      result += hexEncode(request.userSuppliedTag);
      result += '|';
      for (std::size_t index = 0U;
           index < request.desiredAttributeHandles.size();
           ++index) {
        if (index != 0U) {
          result += ',';
        }
        result += std::to_string(request.desiredAttributeHandles[index]);
      }
      result += '\n';
    }
    for (auto const& request : object.pendingAttributeOwnershipAcquisitionRequests) {
      result += "pendingOwnership=";
      result += std::to_string(request.requestId);
      result += '|';
      result += std::to_string(request.requestingFederateId);
      result += '|';
      result += std::to_string(request.requestSequence);
      result += '|';
      result += hexEncode(request.userSuppliedTag);
      auto appendAttributeList = [&result](std::vector<std::uint64_t> const& handles) {
        result += '|';
        for (std::size_t index = 0U; index < handles.size(); ++index) {
          if (index != 0U) {
            result += ',';
          }
          result += std::to_string(handles[index]);
        }
      };
      appendAttributeList(request.desiredAttributeHandles);
      appendAttributeList(request.notificationQueuedAttributeHandles);
      appendAttributeList(request.unavailableQueuedAttributeHandles);
      result += '|';
      result += std::to_string(
          request.releaseCallbacksQueuedByOwningFederate.size());
      result += '\n';
      for (auto const& [ownerFederateId, attributes] :
           request.releaseCallbacksQueuedByOwningFederate) {
        result += "pendingOwnershipRelease=";
        result += std::to_string(ownerFederateId);
        result += '|';
        for (std::size_t index = 0U; index < attributes.size(); ++index) {
          if (index != 0U) {
            result += ',';
          }
          result += std::to_string(attributes[index]);
        }
        result += '\n';
      }
    }
    for (auto const& cancellation :
         object.pendingAttributeOwnershipAcquisitionCancellations) {
      result += "pendingOwnershipCancellation=";
      result += std::to_string(cancellation.cancellationId);
      result += '|';
      result += std::to_string(cancellation.requestingFederateId);
      result += '|';
      for (std::size_t index = 0U;
           index < cancellation.attributeHandles.size();
           ++index) {
        if (index != 0U) {
          result += ',';
        }
        result += std::to_string(cancellation.attributeHandles[index]);
      }
      result += '\n';
    }
    for (auto const& notification :
         object.pendingAttributeOwnershipDivestitureIfWantedNotifications) {
      result += "pendingOwnershipDivestitureIfWanted=";
      result += std::to_string(notification.notificationId);
      result += '|';
      result += std::to_string(notification.receivingFederateId);
      result += '|';
      result += hexEncode(notification.userSuppliedTag);
      result += '|';
      for (std::size_t index = 0U;
           index < notification.attributeHandles.size();
           ++index) {
        if (index != 0U) {
          result += ',';
        }
        result += std::to_string(notification.attributeHandles[index]);
      }
      result += '\n';
    }
    for (auto const& notification : object.pendingConfirmDivestitureNotifications) {
      result += "pendingOwnershipConfirmDivestiture=";
      result += std::to_string(notification.notificationId);
      result += '|';
      result += std::to_string(notification.receivingFederateId);
      result += '|';
      result += hexEncode(notification.userSuppliedTag);
      result += '|';
      for (std::size_t index = 0U;
           index < notification.attributeHandles.size();
           ++index) {
        if (index != 0U) {
          result += ',';
        }
        result += std::to_string(notification.attributeHandles[index]);
      }
      result += '\n';
    }
    for (auto const& request : object.pendingAttributeTransportationTypeChanges) {
      result += "pendingAttributeTransportationTypeChange=";
      result += std::to_string(request.requestId);
      result += '|';
      result += std::to_string(request.requestingFederateId);
      result += '|';
      for (std::size_t index = 0U;
           index < request.attributeHandles.size();
           ++index) {
        if (index != 0U) {
          result += ',';
        }
        result += std::to_string(request.attributeHandles[index]);
      }
      result += '|';
      result += hexEncode(request.transportationName);
      result += '\n';
    }
    for (auto const& divestiture :
         object.pendingNegotiatedAttributeOwnershipDivestitures) {
      result += "pendingOwnershipNegotiatedDivestiture=";
      result += std::to_string(divestiture.attributeHandle);
      result += '|';
      result += std::to_string(divestiture.divestingFederateId);
      result += '|';
      result += std::to_string(divestiture.acquiringFederateId);
      result += '|';
      result += std::to_string(divestiture.acquisitionRequestId);
      result += '|';
      result += divestiture.acquiringFederateIsIfAvailable ? '1' : '0';
      result += '|';
      result += divestiture.confirmationQueued ? '1' : '0';
      result += '|';
      result += divestiture.confirmationDelivered ? '1' : '0';
      result += '|';
      result += hexEncode(divestiture.userSuppliedTag);
      result += '\n';
    }
    for (auto const& assumption : object.ownershipAssumptionRecipientsByAttribute) {
      result += "pendingOwnershipAssumptionRecipients=";
      result += std::to_string(assumption.attributeHandle);
      result += '|';
      for (std::size_t index = 0U;
           index < assumption.recipientFederateIds.size();
           ++index) {
        if (index != 0U) {
          result += ',';
        }
        result += std::to_string(assumption.recipientFederateIds[index]);
      }
      result += '\n';
    }
    for (auto const& assumption : object.ownershipAssumptionUserSuppliedTagsByAttribute) {
      result += "pendingOwnershipAssumptionTag=";
      result += std::to_string(assumption.attributeHandle);
      result += '|';
      result += hexEncode(assumption.userSuppliedTag);
      result += '\n';
    }
    for (auto const& known : object.knownObjectClassHandlesByFederate) {
      result += "objectKnownClass=";
      result += std::to_string(known.federateId);
      result += '|';
      result += std::to_string(known.objectClassHandle);
      result += '\n';
    }
    for (auto const federateId : object.pendingDiscoveryFederateIds) {
      result += "objectPendingDiscovery=";
      result += std::to_string(federateId);
      result += '\n';
    }
    for (auto const federateId : object.pendingRemovalFederateIds) {
      result += "objectPendingRemoval=";
      result += std::to_string(federateId);
      result += '\n';
    }
    for (auto const federateId : object.connectionLossAutomaticRemovalFederateIds) {
      result += "objectConnectionLossAutomaticRemoval=";
      result += std::to_string(federateId);
      result += '\n';
    }
    for (auto const federateId : object.deferredConnectionLossTsoRemovalFederateIds) {
      result += "objectDeferredConnectionLossTsoRemoval=";
      result += std::to_string(federateId);
      result += '\n';
    }
    for (auto const federateId : object.pendingTimestampedRemovalFederateIds) {
      result += "objectPendingTimestampedRemoval=";
      result += std::to_string(federateId);
      result += '\n';
    }
  }
  if (image.deferredUpdateRegionAssociationsPresent ||
      !deferredUpdateRegionAssociations.empty()) {
    appendScalar(
        result,
        "deferredUpdateRegionAssociations",
        deferredUpdateRegionAssociations.size());
    for (auto const& association : deferredUpdateRegionAssociations) {
      result += "deferredUpdateRegionAssociation=";
      result += std::to_string(association.objectInstanceHandle);
      result += '|';
      result += std::to_string(association.federateId);
      result += '|';
      result += std::to_string(association.attributeHandle);
      result += '|';
      for (std::size_t index = 0U; index < association.regionHandles.size(); ++index) {
        if (index != 0U) {
          result += ',';
        }
        result += std::to_string(association.regionHandles[index]);
      }
      result += '\n';
    }
  }
  appendScalar(
      result,
      "pendingAttributeOwnershipQueries",
      pendingAttributeOwnershipQueries.size());
  for (auto const& query : pendingAttributeOwnershipQueries) {
    result += "pendingAttributeOwnershipQuery=";
    result += std::to_string(query.requestId);
    result += '|';
    result += std::to_string(query.requestingFederateId);
    result += '|';
    result += std::to_string(query.objectInstanceHandle);
    result += '|';
    result += std::to_string(query.reportKind);
    result += '|';
    result += std::to_string(query.owningFederateId);
    result += '|';
    for (std::size_t index = 0U;
         index < query.requestedAttributeHandles.size();
         ++index) {
      if (index != 0U) {
        result += ',';
      }
      result += std::to_string(query.requestedAttributeHandles[index]);
    }
    result += '\n';
  }
  appendScalar(
      result,
      "pendingAttributeOwnershipAssumptionCallbacks",
      pendingAttributeOwnershipAssumptions.size());
  for (auto const& callback : pendingAttributeOwnershipAssumptions) {
    result += "pendingAttributeOwnershipAssumptionCallback=";
    result += std::to_string(callback.objectInstanceHandle);
    result += '|';
    result += std::to_string(callback.receivingFederateId);
    result += '|';
    for (std::size_t index = 0U; index < callback.attributeHandles.size(); ++index) {
      if (index != 0U) {
        result += ',';
      }
      result += std::to_string(callback.attributeHandles[index]);
    }
    result += '|';
    result += hexEncode(callback.userSuppliedTag);
    result += '\n';
  }
  appendScalar(result, "interactionDeclarations", interactionDeclarations.size());
  for (auto const& declaration : interactionDeclarations) {
    result += "interactionDeclaration=";
    result += std::to_string(declaration.federateId);
    result += '|';
    result += std::to_string(declaration.publishedInteractionClasses.size());
    result += '|';
    result += std::to_string(declaration.subscribedInteractionClasses.size());
    result += '|';
    result += std::to_string(declaration.regionalSubscribedInteractionClasses.size());
    result += '|';
    result += std::to_string(declaration.publishedObjectClassDirectedInteractions.size());
    result += '|';
    result += std::to_string(declaration.subscribedObjectClassDirectedInteractions.size());
    result += '|';
    result += std::to_string(declaration.interactionTransportationTypes.size());
    result += '|';
    result += std::to_string(declaration.interactionOrderTypes.size());
    result += '|';
    result += std::to_string(
        declaration.pendingInteractionTransportationTypeChanges.size());
    result += '\n';
    for (auto const handle : declaration.publishedInteractionClasses) {
      result += "publishedInteraction=";
      result += std::to_string(declaration.federateId);
      result += '|';
      result += std::to_string(handle);
      result += '\n';
    }
    for (auto const& subscription : declaration.subscribedInteractionClasses) {
      result += "subscribedInteraction=";
      result += std::to_string(declaration.federateId);
      result += '|';
      result += std::to_string(subscription.interactionClassHandle);
      result += '|';
      result += (subscription.active ? "1" : "0");
      result += '\n';
    }
    for (auto const& subscription : declaration.regionalSubscribedInteractionClasses) {
      result += "regionalSubscribedInteraction=";
      result += std::to_string(declaration.federateId);
      result += '|';
      result += std::to_string(subscription.interactionClassHandle);
      result += '|';
      result += std::to_string(subscription.regionHandle);
      result += '|';
      result += (subscription.active ? "1" : "0");
      result += '\n';
    }
    for (auto const& directed : declaration.publishedObjectClassDirectedInteractions) {
      result += "publishedDirectedInteraction=";
      result += std::to_string(declaration.federateId);
      result += '|';
      result += std::to_string(directed.objectClassHandle);
      result += '|';
      result += std::to_string(directed.interactionClassHandle);
      result += '\n';
    }
    for (auto const& directed : declaration.subscribedObjectClassDirectedInteractions) {
      result += "subscribedDirectedInteraction=";
      result += std::to_string(declaration.federateId);
      result += '|';
      result += std::to_string(directed.objectClassHandle);
      result += '|';
      result += std::to_string(directed.interactionClassHandle);
      result += '|';
      result += (directed.active ? "1" : "0");
      result += '\n';
    }
    for (auto const& value : declaration.interactionTransportationTypes) {
      result += "interactionTransport=";
      result += std::to_string(declaration.federateId);
      result += '|';
      result += std::to_string(value.interactionClassHandle);
      result += '|';
      result += hexEncode(value.value);
      result += '\n';
    }
    for (auto const& value : declaration.interactionOrderTypes) {
      result += "interactionOrder=";
      result += std::to_string(declaration.federateId);
      result += '|';
      result += std::to_string(value.interactionClassHandle);
      result += '|';
      result += std::to_string(value.orderType);
      result += '\n';
    }
    for (auto const& value : declaration.pendingInteractionTransportationTypeChanges) {
      result += "interactionPendingTransport=";
      result += std::to_string(declaration.federateId);
      result += '|';
      result += std::to_string(value.interactionClassHandle);
      result += '|';
      result += hexEncode(value.value);
      result += '\n';
    }
  }
  appendScalar(result, "tsoInteractionMessages", tsoInteractionMessages.size());
  for (auto const& message : tsoInteractionMessages) {
    result += "tsoInteractionMessage=";
    result += std::to_string(message.messageId);
    result += '|';
    result += std::to_string(message.producingFederateId);
    result += '|';
    result += std::to_string(message.sentInteractionClassHandle);
    result += '|';
    result += (message.defaultRegionUsed ? "1" : "0");
    result += '|';
    result += std::to_string(message.sentOrderType);
    result += '|';
    result += std::to_string(message.receivedOrderType);
    result += '|';
    result += encodeOptional(message.timestampEncoding);
    result += '|';
    result += hexEncode(message.userSuppliedTag);
    result += '|';
    result += hexEncode(message.transportationName);
    result += '|';
    result += std::to_string(message.sentParameterHandles.size());
    result += '|';
    result += std::to_string(message.parameters.size());
    result += '|';
    result += std::to_string(message.sentRegionHandles.size());
    result += '|';
    result += std::to_string(message.sentRegionSnapshots.size());
    result += '\n';
    for (auto const handle : message.sentParameterHandles) {
      result += "tsoInteractionParameterHandle=";
      result += std::to_string(handle);
      result += '\n';
    }
    for (auto const& parameter : message.parameters) {
      result += "tsoInteractionParameter=";
      result += std::to_string(parameter.parameterHandle);
      result += '|';
      result += hexEncode(parameter.value);
      result += '\n';
    }
    for (auto const handle : message.sentRegionHandles) {
      result += "tsoInteractionRegionHandle=";
      result += std::to_string(handle);
      result += '\n';
    }
    for (auto const& snapshot : message.sentRegionSnapshots) {
      result += "tsoInteractionRegionSnapshot=";
      result += std::to_string(snapshot.regionHandle);
      result += '|';
      result += (snapshot.specificationCommitted ? "1" : "0");
      result += '|';
      result += std::to_string(snapshot.dimensionHandles.size());
      result += '|';
      result += std::to_string(snapshot.committedRangeBounds.size());
      result += '\n';
      for (auto const dimension : snapshot.dimensionHandles) {
        result += "tsoInteractionRegionDimension=";
        result += std::to_string(dimension);
        result += '\n';
      }
      for (auto const& range : snapshot.committedRangeBounds) {
        result += "tsoInteractionRegionRange=";
        result += std::to_string(range.dimensionHandle);
        result += '|';
        result += std::to_string(range.lowerBound);
        result += '|';
        result += std::to_string(range.upperBound);
        result += '\n';
      }
    }
  }
  appendScalar(
      result,
      "tsoDirectedInteractionMessages",
      tsoDirectedInteractionMessages.size());
  for (auto const& message : tsoDirectedInteractionMessages) {
    result += "tsoDirectedInteractionMessage=";
    result += std::to_string(message.messageId);
    result += '|';
    result += std::to_string(message.producingFederateId);
    result += '|';
    result += std::to_string(message.objectInstanceHandle);
    result += '|';
    result += std::to_string(message.sentInteractionClassHandle);
    result += '|';
    result += std::to_string(message.sentOrderType);
    result += '|';
    result += std::to_string(message.receivedOrderType);
    result += '|';
    result += encodeOptional(message.timestampEncoding);
    result += '|';
    result += hexEncode(message.userSuppliedTag);
    result += '|';
    result += hexEncode(message.transportationName);
    result += '|';
    result += std::to_string(message.sentParameterHandles.size());
    result += '|';
    result += std::to_string(message.parameters.size());
    result += '|';
    result += std::to_string(message.recipients.size());
    result += '\n';
    for (auto const handle : message.sentParameterHandles) {
      result += "tsoDirectedInteractionParameterHandle=";
      result += std::to_string(handle);
      result += '\n';
    }
    for (auto const& parameter : message.parameters) {
      result += "tsoDirectedInteractionParameter=";
      result += std::to_string(parameter.parameterHandle);
      result += '|';
      result += hexEncode(parameter.value);
      result += '\n';
    }
    for (auto const& recipient : message.recipients) {
      result += "tsoDirectedInteractionRecipient=";
      result += std::to_string(recipient.receivingFederateId);
      result += '|';
      result += std::to_string(recipient.objectInstanceHandle);
      result += '|';
      result += std::to_string(recipient.receivedInteractionClassHandle);
      result += '|';
      result += std::to_string(recipient.receivedParameterHandles.size());
      result += '\n';
      for (auto const handle : recipient.receivedParameterHandles) {
        result += "tsoDirectedInteractionRecipientParameter=";
        result += std::to_string(handle);
        result += '\n';
      }
    }
  }
  auto appendRegionSnapshot = [&result](
                                  std::string_view marker,
                                  std::string_view dimensionMarker,
                                  std::string_view rangeMarker,
                                  FederationStateImageInteractionRegionSnapshot const& snapshot) {
    result += marker;
    result += '=';
    result += std::to_string(snapshot.regionHandle);
    result += '|';
    result += (snapshot.specificationCommitted ? "1" : "0");
    result += '|';
    result += std::to_string(snapshot.dimensionHandles.size());
    result += '|';
    result += std::to_string(snapshot.committedRangeBounds.size());
    result += '\n';
    for (auto const dimension : snapshot.dimensionHandles) {
      result += dimensionMarker;
      result += '=';
      result += std::to_string(dimension);
      result += '\n';
    }
    for (auto const& range : snapshot.committedRangeBounds) {
      result += rangeMarker;
      result += '=';
      result += std::to_string(range.dimensionHandle);
      result += '|';
      result += std::to_string(range.lowerBound);
      result += '|';
      result += std::to_string(range.upperBound);
      result += '\n';
    }
  };
  appendScalar(
      result,
      "tsoAttributeUpdateMessages",
      tsoAttributeUpdateMessages.size());
  for (auto const& message : tsoAttributeUpdateMessages) {
    result += "tsoAttributeUpdateMessage=";
    result += std::to_string(message.messageId);
    result += '|';
    result += std::to_string(message.producingFederateId);
    result += '|';
    result += std::to_string(message.objectInstanceHandle);
    result += '|';
    result += encodeOptional(message.timestampEncoding);
    result += '|';
    result += hexEncode(message.userSuppliedTag);
    result += '|';
    result += std::to_string(message.attributes.size());
    result += '|';
    result += std::to_string(message.passelsByRecipient.size());
    result += '|';
    result += std::to_string(message.sentRegionSnapshots.size());
    result += '\n';
    for (auto const& attribute : message.attributes) {
      result += "tsoAttributeUpdateAttribute=";
      result += std::to_string(attribute.attributeHandle);
      result += '|';
      result += hexEncode(attribute.value);
      result += '\n';
    }
    for (auto const& recipient : message.passelsByRecipient) {
      result += "tsoAttributeUpdateRecipient=";
      result += std::to_string(recipient.receivingFederateId);
      result += '|';
      result += std::to_string(recipient.passels.size());
      result += '\n';
      for (auto const& passel : recipient.passels) {
        result += "tsoAttributeUpdatePassel=";
        result += hexEncode(passel.transportationName);
        result += '|';
        result += (passel.defaultRegionUsed ? "1" : "0");
        result += '|';
        result += std::to_string(passel.preferredOrderType);
        result += '|';
        result += std::to_string(passel.sentAttributeHandles.size());
        result += '|';
        result += std::to_string(passel.sentRegionHandles.size());
        result += '|';
        result += std::to_string(passel.sentRegionSnapshots.size());
        result += '\n';
        for (auto const handle : passel.sentAttributeHandles) {
          result += "tsoAttributeUpdatePasselAttributeHandle=";
          result += std::to_string(handle);
          result += '\n';
        }
        for (auto const handle : passel.sentRegionHandles) {
          result += "tsoAttributeUpdatePasselRegionHandle=";
          result += std::to_string(handle);
          result += '\n';
        }
        for (auto const& snapshot : passel.sentRegionSnapshots) {
          appendRegionSnapshot(
              "tsoAttributeUpdatePasselRegionSnapshot",
              "tsoAttributeUpdatePasselRegionDimension",
              "tsoAttributeUpdatePasselRegionRange",
              snapshot);
        }
      }
    }
    for (auto const& snapshot : message.sentRegionSnapshots) {
      appendRegionSnapshot(
          "tsoAttributeUpdateRegionSnapshot",
          "tsoAttributeUpdateRegionDimension",
          "tsoAttributeUpdateRegionRange",
          snapshot);
    }
  }
  appendScalar(
      result,
      "tsoObjectDeletionMessages",
      tsoObjectDeletionMessages.size());
  for (auto const& message : tsoObjectDeletionMessages) {
    result += "tsoObjectDeletionMessage=";
    result += std::to_string(message.messageId);
    result += '|';
    result += std::to_string(message.producingFederateId);
    result += '|';
    result += std::to_string(message.objectInstanceHandle);
    result += '|';
    result += encodeOptional(message.timestampEncoding);
    result += '|';
    result += hexEncode(message.userSuppliedTag);
    result += '|';
    result += std::to_string(message.recipients.size());
    result += '|';
    result += message.reconstitution.has_value() ? "1" : "0";
    result += '|';
    result += message.reconstitution
        ? std::to_string(message.reconstitution->knownObjectClassHandlesByFederate.size())
        : "0";
    result += '|';
    result += message.reconstitution
        ? std::to_string(message.reconstitution->object.attributes.size())
        : "0";
    result += '|';
    result += message.reconstitution
        ? std::to_string(message.reconstitution->object.attributeValues.size())
        : "0";
    result += '|';
    result += std::to_string(message.sentOrderType);
    result += '\n';
    for (auto const& recipient : message.recipients) {
      result += "tsoObjectDeletionRecipient=";
      result += std::to_string(recipient.receivingFederateId);
      result += '|';
      result += std::to_string(recipient.objectInstanceHandle);
      result += '\n';
    }
    if (message.reconstitution) {
      auto const& object = message.reconstitution->object;
      result += "tsoObjectDeletionReconstitution=";
      result += std::to_string(object.handle);
      result += '|';
      result += encodeWide(object.name);
      result += '|';
      result += std::to_string(object.registeredObjectClassHandle);
      result += '|';
      result += std::to_string(object.producingFederateId);
      result += '|';
      result += object.deleteAccepted ? "1" : "0";
      result += '|';
      result += std::to_string(object.pendingOperationCount);
      result += '\n';
      for (auto const& attribute : object.attributes) {
        result += "tsoObjectDeletionReconstitutionAttribute=";
        result += std::to_string(attribute.handle);
        result += '|';
        result += std::to_string(attribute.ownerFederateId);
        result += '|';
        result += hexEncode(attribute.transportationName);
        result += '|';
        result += std::to_string(attribute.orderType);
        result += '|';
        for (std::size_t index = 0U;
             index < attribute.updateRegionHandles.size(); ++index) {
          if (index != 0U) {
            result += ',';
          }
          result += std::to_string(attribute.updateRegionHandles[index]);
        }
        result += '\n';
      }
      for (auto const& value : object.attributeValues) {
        result += "tsoObjectDeletionReconstitutionValue=";
        result += std::to_string(value.attributeHandle);
        result += '|';
        result += hexEncode(value.value);
        result += '\n';
      }
      for (auto const& [federateId, objectClassHandle] :
           message.reconstitution->knownObjectClassHandlesByFederate) {
        result += "tsoObjectDeletionReconstitutionKnown=";
        result += std::to_string(federateId);
        result += '|';
        result += std::to_string(objectClassHandle);
        result += '\n';
      }
    }
  }
  appendScalar(
      result,
      "tsoRequestRetractionRecords",
      tsoRequestRetractionRecords.size());
  for (auto const& record : tsoRequestRetractionRecords) {
    result += "tsoRequestRetractionRecord=";
    result += std::to_string(record.messageId);
    result += '|';
    result += std::to_string(record.producingFederateId);
    result += '|';
    result += encodeOptional(record.timestampEncoding);
    result += '|';
    result += record.retractionApplied ? "1" : "0";
    result += '|';
    result += record.terminal ? "1" : "0";
    result += '|';
    result += record.producerResigned ? "1" : "0";
    result += '|';
    result += record.deliveryRequiredAfterConnectionLoss ? "1" : "0";
    result += '|';
    result += std::to_string(record.recipientStates.size());
    result += '\n';
    for (auto const& recipient : record.recipientStates) {
      result += "tsoRequestRetractionRecipient=";
      result += std::to_string(recipient.receivingFederateId);
      result += '|';
      result += std::to_string(recipient.state);
      result += '\n';
    }
  }
  appendScalar(result, "tsoQueueEntries", tsoQueueEntries.size());
  for (auto const& entry : tsoQueueEntries) {
    result += "tsoQueue=";
    result += std::to_string(entry.recipientFederateId);
    result += '|';
    result += std::to_string(entry.messageId);
    result += '|';
    result += std::to_string(entry.sequence);
    result += '|';
    result += std::to_string(entry.phase);
    result += '|';
    result += encodeOptional(entry.timestampEncoding);
    result += '\n';
  }

  appendScalar(
      result,
      "reservedObjectInstanceNames",
      reservedObjectInstanceNames.size());
  for (auto const& reservation : reservedObjectInstanceNames) {
    result += "reservedObjectInstanceName=";
    result += std::to_string(reservation.federateId);
    result += '|';
    result += encodeWide(reservation.objectInstanceName);
    result += '\n';
  }
  appendScalar(result, "synchronizationPoints", synchronizationPoints.size());
  for (auto const& point : synchronizationPoints) {
    result += "synchronizationPoint=";
    result += encodeWide(point.label);
    result += '|';
    result += hexEncode(point.userSuppliedTag);
    result += '|';
    result += std::to_string(point.synchronizationSet.size());
    result += '|';
    result += std::to_string(point.announcedFederates.size());
    result += '|';
    result += std::to_string(point.achievedFederates.size());
    result += '|';
    result += point.lateJoinExpansionAllowed ? "1" : "0";
    result += '\n';
    for (auto const federateId : point.synchronizationSet) {
      result += "synchronizationPointMember=";
      result += std::to_string(federateId);
      result += '\n';
    }
    for (auto const federateId : point.announcedFederates) {
      result += "synchronizationPointAnnounced=";
      result += std::to_string(federateId);
      result += '\n';
    }
    for (auto const& [federateId, succeeded] : point.achievedFederates) {
      result += "synchronizationPointAchieved=";
      result += std::to_string(federateId);
      result += '|';
      result += succeeded ? "1" : "0";
      result += '\n';
    }
  }
  appendScalar(result, "regions", regions.size());
  for (auto const& region : regions) {
    result += "region=";
    result += std::to_string(region.handle);
    result += '|';
    result += std::to_string(region.ownerFederateId);
    result += '|';
    result += region.specificationCommitted ? "1" : "0";
    result += '|';
    result += region.inUse ? "1" : "0";
    result += '|';
    result += std::to_string(region.dimensionHandles.size());
    result += '|';
    result += std::to_string(region.pendingRangeBounds.size());
    result += '|';
    result += std::to_string(region.committedRangeBounds.size());
    result += '\n';
    for (auto const dimensionHandle : region.dimensionHandles) {
      result += "regionDimension=";
      result += std::to_string(dimensionHandle);
      result += '\n';
    }
    for (auto const& range : region.pendingRangeBounds) {
      result += "regionPendingRange=";
      result += std::to_string(range.dimensionHandle);
      result += '|';
      result += std::to_string(range.lowerBound);
      result += '|';
      result += std::to_string(range.upperBound);
      result += '\n';
    }
    for (auto const& range : region.committedRangeBounds) {
      result += "regionCommittedRange=";
      result += std::to_string(range.dimensionHandle);
      result += '|';
      result += std::to_string(range.lowerBound);
      result += '|';
      result += std::to_string(range.upperBound);
      result += '\n';
    }
  }
  appendScalar(
      result,
      "objectClassAttributeDeclarations",
      objectClassAttributeDeclarations.size());
  for (auto const& declaration : objectClassAttributeDeclarations) {
    result += "objectClassAttributeDeclaration=";
    result += std::to_string(declaration.federateId);
    result += '|';
    result += std::to_string(declaration.subscriptionGeneration);
    result += '|';
    result += std::to_string(declaration.classes.size());
    result += '\n';
    for (auto const& objectClass : declaration.classes) {
      result += "objectClassAttributeDeclarationClass=";
      result += std::to_string(declaration.federateId);
      result += '|';
      result += std::to_string(objectClass.objectClassHandle);
      result += '|';
      result += objectClass.privilegeToDeleteExplicitlyUnpublished ? "1" : "0";
      result += '|';
      result += std::to_string(objectClass.explicitlyPublishedAttributeHandles.size());
      result += '|';
      result += std::to_string(objectClass.subscribedAttributes.size());
      result += '|';
      result += std::to_string(objectClass.subscribedUpdateRateDesignators.size());
      result += '|';
      result += std::to_string(objectClass.regionalSubscribedAttributes.size());
      result += '|';
      result += std::to_string(
          objectClass.regionalSubscribedUpdateRateDesignators.size());
      result += '|';
      result += std::to_string(objectClass.defaultTransportationTypes.size());
      result += '|';
      result += std::to_string(objectClass.defaultOrderTypes.size());
      result += '\n';
      for (auto const attributeHandle : objectClass.explicitlyPublishedAttributeHandles) {
        result += "objectClassAttributePublished=";
        result += std::to_string(declaration.federateId);
        result += '|';
        result += std::to_string(objectClass.objectClassHandle);
        result += '|';
        result += std::to_string(attributeHandle);
        result += '\n';
      }
      for (auto const& subscription : objectClass.subscribedAttributes) {
        result += "objectClassAttributeSubscribed=";
        result += std::to_string(declaration.federateId);
        result += '|';
        result += std::to_string(objectClass.objectClassHandle);
        result += '|';
        result += std::to_string(subscription.attributeHandle);
        result += '|';
        result += subscription.active ? "1" : "0";
        result += '\n';
      }
      for (auto const& value : objectClass.subscribedUpdateRateDesignators) {
        result += "objectClassAttributeSubscribedRate=";
        result += std::to_string(declaration.federateId);
        result += '|';
        result += std::to_string(objectClass.objectClassHandle);
        result += '|';
        result += std::to_string(value.attributeHandle);
        result += '|';
        result += hexEncode(value.value);
        result += '\n';
      }
      for (auto const& subscription : objectClass.regionalSubscribedAttributes) {
        result += "objectClassAttributeRegionalSubscribed=";
        result += std::to_string(declaration.federateId);
        result += '|';
        result += std::to_string(objectClass.objectClassHandle);
        result += '|';
        result += std::to_string(subscription.attributeHandle);
        result += '|';
        result += std::to_string(subscription.regionHandle);
        result += '|';
        result += subscription.active ? "1" : "0";
        result += '\n';
      }
      for (auto const& value : objectClass.regionalSubscribedUpdateRateDesignators) {
        result += "objectClassAttributeRegionalSubscribedRate=";
        result += std::to_string(declaration.federateId);
        result += '|';
        result += std::to_string(objectClass.objectClassHandle);
        result += '|';
        result += std::to_string(value.attributeHandle);
        result += '|';
        result += std::to_string(value.regionHandle);
        result += '|';
        result += hexEncode(value.value);
        result += '\n';
      }
      for (auto const& value : objectClass.defaultTransportationTypes) {
        result += "objectClassAttributeDefaultTransport=";
        result += std::to_string(declaration.federateId);
        result += '|';
        result += std::to_string(objectClass.objectClassHandle);
        result += '|';
        result += std::to_string(value.attributeHandle);
        result += '|';
        result += hexEncode(value.value);
        result += '\n';
      }
      for (auto const& value : objectClass.defaultOrderTypes) {
        result += "objectClassAttributeDefaultOrder=";
        result += std::to_string(declaration.federateId);
        result += '|';
        result += std::to_string(objectClass.objectClassHandle);
        result += '|';
        result += std::to_string(value.attributeHandle);
        result += '|';
        result += std::to_string(value.orderType);
        result += '\n';
      }
    }
  }
  if (image.saveHistoryPresent) {
    if (image.lastSaveName.empty() && image.lastSaveTimeEncoding.has_value()) {
      throw std::logic_error(
          "Last-save time cannot be present without a last-save name.");
    }
    if (image.nextSaveName.empty() && image.nextSaveTimeEncoding.has_value()) {
      throw std::logic_error(
          "Next-save time cannot be present without a next-save name.");
    }
    result += "saveHistory=";
    result += encodeWide(image.lastSaveName);
    result += '|';
    result += encodeOptional(image.lastSaveTimeEncoding);
    result += '|';
    result += encodeWide(image.nextSaveName);
    result += '|';
    result += encodeOptional(image.nextSaveTimeEncoding);
    result += '\n';
  }
  result += "end\n";
  return result;
}

}  // namespace umbra::detail
