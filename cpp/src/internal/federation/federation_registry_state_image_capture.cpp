#include "internal/federation/federation_registry.hpp"
#include "internal/federation/federation_state_image.hpp"

#include <RTI/encoding/BasicDataElements.h>

#include <algorithm>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace umbra::detail {

namespace {

template <typename LogicalValue>
std::optional<std::string> encodedLogicalValue(
    std::shared_ptr<LogicalValue const> const& value) {
  if (!value) {
    return std::nullopt;
  }
  auto const encoded = value->encode();
  auto const* bytes = static_cast<unsigned char const*>(encoded.data());
  return std::string{
      reinterpret_cast<char const*>(bytes),
      encoded.size()};
}

std::string bytesFromVariableLengthData(
    rti1516_2025::VariableLengthData const& value) {
  if (value.size() == 0U) {
    return {};
  }
  auto const* bytes = static_cast<unsigned char const*>(value.data());
  if (bytes == nullptr) {
    throw std::runtime_error(
        "A non-empty VariableLengthData value has no data in the state image.");
  }
  return std::string{
      reinterpret_cast<char const*>(bytes),
      value.size()};
}

} // namespace

FederationStateImage EmbeddedFederationRegistry::stateImageFor(
    Federation const& federation,
    std::wstring const& federationName) {
  FederationStateImage image;
  image.federationName = federationName;
  image.logicalTimeImplementationName =
      federation.definition.logicalTimeImplementationName;
  image.normalizationSeed = federation.normalizationSeed;
  image.federationSwitches =
      (federation.autoProvideSwitch ? 1U : 0U) |
      (federation.advisoriesUseKnownClassSwitch ? 1U << 1U : 0U) |
      (federation.nonRegulatedGrantSwitch ? 1U << 2U : 0U) |
      (federation.delaySubscriptionEvaluationSwitch ? 1U << 3U : 0U) |
      (federation.allowRelaxedDDMSwitch ? 1U << 4U : 0U);
  image.lastSaveName = federation.lastSaveName;
  image.lastSaveTimeEncoding = encodedLogicalValue(federation.lastSaveTime);
  image.nextSaveName = federation.nextSaveName;
  image.nextSaveTimeEncoding = encodedLogicalValue(federation.nextSaveTime);
  image.saveHistoryPresent = true;

  image.interactionDeclarationCount = federation.interactionDeclarations.size();
  image.synchronizationPointCount = federation.synchronizationPoints.size();
  image.objectClassDeclarationCount = federation.objectClassAttributeDeclarations.size();
  image.regionCount = federation.regions.size();
  image.objectInstanceCount = federation.objectInstances.size();
  image.tsoInteractionMessageCount = federation.tsoInteractionMessages.size();
  image.tsoAttributeUpdateMessageCount = federation.tsoAttributeUpdateMessages.size();
  image.tsoObjectDeletionMessageCount = federation.tsoObjectDeletionMessages.size();
  image.tsoDirectedInteractionMessageCount = federation.tsoDirectedInteractionMessages.size();

  image.nextRegionHandle = federation.nextRegionHandle;
  image.nextSubscriptionGeneration = federation.nextSubscriptionGeneration;
  image.nextObjectInstanceHandle = federation.nextObjectInstanceHandle;
  image.nextAttributeOwnershipAcquisitionIfAvailableRequestId =
      federation.nextAttributeOwnershipAcquisitionIfAvailableRequestId;
  image.nextAttributeOwnershipAcquisitionRequestId =
      federation.nextAttributeOwnershipAcquisitionRequestId;
  image.nextAttributeOwnershipAcquisitionRequestSequence =
      federation.nextAttributeOwnershipAcquisitionRequestSequence;
  image.nextAttributeOwnershipAcquisitionCancellationId =
      federation.nextAttributeOwnershipAcquisitionCancellationId;
  image.nextAttributeOwnershipDivestitureIfWantedNotificationId =
      federation.nextAttributeOwnershipDivestitureIfWantedNotificationId;
  image.nextConfirmDivestitureNotificationId =
      federation.nextConfirmDivestitureNotificationId;
  image.nextAttributeTransportationTypeChangeRequestId =
      federation.nextAttributeTransportationTypeChangeRequestId;
  image.nextAttributeValueUpdateRequestId =
      federation.nextAttributeValueUpdateRequestId;
  image.nextAttributeOwnershipQueryRequestId =
      federation.nextAttributeOwnershipQueryRequestId;
  image.nextTimeAdvanceGrantDispatchIdentity =
      federation.nextTimeAdvanceGrantDispatchIdentity;

  image.members.reserve(federation.members.size());
  for (auto const& [memberId, member] : federation.members) {
    FederationStateImageMember savedMember;
    savedMember.id = memberId;
    savedMember.name = member.name;
    savedMember.type = member.type;
    savedMember.switches =
        (member.objectClassRelevanceAdvisorySwitch ? 1U : 0U) |
        (member.attributeScopeAdvisorySwitch ? 1U << 1U : 0U) |
        (member.attributeRelevanceAdvisorySwitch ? 1U << 2U : 0U) |
        (member.interactionRelevanceAdvisorySwitch ? 1U << 3U : 0U) |
        (member.conveyRegionDesignatorSetsSwitch ? 1U << 4U : 0U) |
        (member.serviceReportingSwitch ? 1U << 5U : 0U) |
        (member.exceptionReportingSwitch ? 1U << 6U : 0U) |
        (member.sendServiceReportsToFileSwitch ? 1U << 7U : 0U);
    savedMember.automaticResignAction =
        static_cast<std::uint32_t>(member.automaticResignAction);
    savedMember.momReportPeriodSeconds = member.momReportPeriodSeconds;
    savedMember.nextMomServiceReportSerialNumber =
        member.nextMomServiceReportSerialNumber;
    savedMember.successfulUpdateAttributeValuesCount =
        member.successfulUpdateAttributeValuesCount;
    savedMember.successfulUpdateCountsByClassAndTransportation.reserve(
        [&member] {
          std::size_t count = 0U;
          for (auto const& [objectClassHandle, transportationCounts] :
               member.successfulUpdateCountsByClassAndTransportation) {
            static_cast<void>(objectClassHandle);
            count += transportationCounts.size();
          }
          return count;
        }());
    for (auto const& [objectClassHandle, transportationCounts] :
         member.successfulUpdateCountsByClassAndTransportation) {
      for (auto const& [transportationName, count] : transportationCounts) {
        savedMember.successfulUpdateCountsByClassAndTransportation.push_back({
            objectClassHandle,
            transportationName,
            count,
        });
      }
    }
    savedMember.successfullyUpdatedObjectInstanceHandles.assign(
        member.successfullyUpdatedObjectInstanceHandles.begin(),
        member.successfullyUpdatedObjectInstanceHandles.end());
    savedMember.successfullyUpdatedObjectInstanceClassHandles.reserve(
        member.successfullyUpdatedObjectInstanceClassHandles.size());
    for (auto const& [objectInstanceHandle, objectClassHandle] :
         member.successfullyUpdatedObjectInstanceClassHandles) {
      savedMember.successfullyUpdatedObjectInstanceClassHandles.push_back({
          objectInstanceHandle,
          objectClassHandle,
      });
    }
    savedMember.successfulReflectionsReceivedCount =
        member.successfulReflectionsReceivedCount;
    savedMember.reflectionTelemetryPresent = true;
    savedMember.successfulObjectInstanceRegistrationsCount =
        member.successfulObjectInstanceRegistrationsCount;
    savedMember.successfulObjectInstanceDeletionsCount =
        member.successfulObjectInstanceDeletionsCount;
    savedMember.successfulObjectInstanceRemovalsCount =
        member.successfulObjectInstanceRemovalsCount;
    savedMember.successfulObjectInstanceDiscoveriesCount =
        member.successfulObjectInstanceDiscoveriesCount;
    savedMember.objectLifecycleTelemetryPresent = true;
    savedMember.successfulReflectionCountsByClassAndTransportation.reserve(
        [&member] {
          std::size_t count = 0U;
          for (auto const& [objectClassHandle, transportationCounts] :
               member.successfulReflectionCountsByClassAndTransportation) {
            static_cast<void>(objectClassHandle);
            count += transportationCounts.size();
          }
          return count;
        }());
    for (auto const& [objectClassHandle, transportationCounts] :
         member.successfulReflectionCountsByClassAndTransportation) {
      for (auto const& [transportationName, count] : transportationCounts) {
        savedMember.successfulReflectionCountsByClassAndTransportation.push_back({
            objectClassHandle,
            transportationName,
            count,
        });
      }
    }
    savedMember.successfullyReflectedObjectInstanceHandles.assign(
        member.successfullyReflectedObjectInstanceHandles.begin(),
        member.successfullyReflectedObjectInstanceHandles.end());
    savedMember.successfullyReflectedObjectInstanceClassHandles.reserve(
        member.successfullyReflectedObjectInstanceClassHandles.size());
    for (auto const& [objectInstanceHandle, objectClassHandle] :
         member.successfullyReflectedObjectInstanceClassHandles) {
      savedMember.successfullyReflectedObjectInstanceClassHandles.push_back({
          objectInstanceHandle,
          objectClassHandle,
      });
    }
    savedMember.reflectionProjectionTelemetryPresent = true;
    savedMember.successfulInteractionsSentCount =
        member.successfulInteractionsSentCount;
    savedMember.successfulDirectedInteractionsSentCount =
        member.successfulDirectedInteractionsSentCount;
    savedMember.successfulInteractionCountsByClassAndTransportation.reserve(
        [&member] {
          std::size_t count = 0U;
          for (auto const& [interactionClassHandle, transportationCounts] :
               member.successfulInteractionCountsByClassAndTransportation) {
            static_cast<void>(interactionClassHandle);
            count += transportationCounts.size();
          }
          return count;
        }());
    for (auto const& [interactionClassHandle, transportationCounts] :
         member.successfulInteractionCountsByClassAndTransportation) {
      for (auto const& [transportationName, count] : transportationCounts) {
        savedMember.successfulInteractionCountsByClassAndTransportation.push_back({
            interactionClassHandle,
            transportationName,
            count,
        });
      }
    }
    savedMember.successfulDirectedInteractionCountsByClassAndTransportation.reserve(
        [&member] {
          std::size_t count = 0U;
          for (auto const& [interactionClassHandle, transportationCounts] :
               member.successfulDirectedInteractionCountsByClassAndTransportation) {
            static_cast<void>(interactionClassHandle);
            count += transportationCounts.size();
          }
          return count;
        }());
    for (auto const& [interactionClassHandle, transportationCounts] :
         member.successfulDirectedInteractionCountsByClassAndTransportation) {
      for (auto const& [transportationName, count] : transportationCounts) {
        savedMember.successfulDirectedInteractionCountsByClassAndTransportation.push_back({
            interactionClassHandle,
            transportationName,
            count,
        });
      }
    }
    savedMember.interactionSendTelemetryPresent = true;
    savedMember.successfulInteractionsReceivedCount =
        member.successfulInteractionsReceivedCount;
    savedMember.successfulDirectedInteractionsReceivedCount =
        member.successfulDirectedInteractionsReceivedCount;
    savedMember.successfulInteractionReceiptCountsByClassAndTransportation.reserve(
        [&member] {
          std::size_t count = 0U;
          for (auto const& [interactionClassHandle, transportationCounts] :
               member.successfulInteractionReceiptCountsByClassAndTransportation) {
            static_cast<void>(interactionClassHandle);
            count += transportationCounts.size();
          }
          return count;
        }());
    for (auto const& [interactionClassHandle, transportationCounts] :
         member.successfulInteractionReceiptCountsByClassAndTransportation) {
      for (auto const& [transportationName, count] : transportationCounts) {
        savedMember.successfulInteractionReceiptCountsByClassAndTransportation.push_back({
            interactionClassHandle,
            transportationName,
            count,
        });
      }
    }
    savedMember.successfulDirectedInteractionReceiptCountsByClassAndTransportation.reserve(
        [&member] {
          std::size_t count = 0U;
          for (auto const& [interactionClassHandle, transportationCounts] :
               member.successfulDirectedInteractionReceiptCountsByClassAndTransportation) {
            static_cast<void>(interactionClassHandle);
            count += transportationCounts.size();
          }
          return count;
        }());
    for (auto const& [interactionClassHandle, transportationCounts] :
         member.successfulDirectedInteractionReceiptCountsByClassAndTransportation) {
      for (auto const& [transportationName, count] : transportationCounts) {
        savedMember.successfulDirectedInteractionReceiptCountsByClassAndTransportation.push_back({
            interactionClassHandle,
            transportationName,
            count,
        });
      }
    }
    savedMember.interactionReceiptTelemetryPresent = true;
    image.members.push_back(std::move(savedMember));
  }

  image.federateNamesById.reserve(federation.federateNamesById.size());
  for (auto const& [federateId, federateName] : federation.federateNamesById) {
    image.federateNamesById.emplace_back(federateId, federateName);
  }
  image.reservedObjectInstanceNames.reserve(
      federation.reservedObjectInstanceNamesByFederate.size());
  for (auto const& [objectInstanceName, federateId] :
       federation.reservedObjectInstanceNamesByFederate) {
    image.reservedObjectInstanceNames.push_back({
        federateId,
        objectInstanceName,
    });
  }
  image.reservedObjectInstanceNamesPresent = true;

  image.regions.reserve(federation.regions.size());
  for (auto const& [regionHandle, region] : federation.regions) {
    FederationStateImageRegion savedRegion;
    savedRegion.handle = regionHandle;
    savedRegion.ownerFederateId = region.ownerFederateId;
    savedRegion.dimensionHandles.assign(
        region.dimensionHandles.begin(),
        region.dimensionHandles.end());
    savedRegion.pendingRangeBounds.reserve(region.pendingRangeBounds.size());
    for (auto const& [dimensionHandle, bounds] : region.pendingRangeBounds) {
      savedRegion.pendingRangeBounds.push_back({
          dimensionHandle,
          static_cast<std::uint64_t>(bounds.lowerBound),
          static_cast<std::uint64_t>(bounds.upperBound),
      });
    }
    savedRegion.committedRangeBounds.reserve(region.committedRangeBounds.size());
    for (auto const& [dimensionHandle, bounds] : region.committedRangeBounds) {
      savedRegion.committedRangeBounds.push_back({
          dimensionHandle,
          static_cast<std::uint64_t>(bounds.lowerBound),
          static_cast<std::uint64_t>(bounds.upperBound),
      });
    }
    savedRegion.specificationCommitted = region.specificationCommitted;
    savedRegion.inUse = region.inUse;
    image.regions.push_back(std::move(savedRegion));
  }
  image.regionsPresent = true;

  image.synchronizationPoints.reserve(federation.synchronizationPoints.size());
  for (auto const& [label, point] : federation.synchronizationPoints) {
    FederationStateImageSynchronizationPoint savedPoint;
    savedPoint.label = label;
    savedPoint.userSuppliedTag = std::string(
        point.userSuppliedTag.begin(),
        point.userSuppliedTag.end());
    savedPoint.synchronizationSet.assign(
        point.synchronizationSet.begin(),
        point.synchronizationSet.end());
    savedPoint.announcedFederates.assign(
        point.announcedFederates.begin(),
        point.announcedFederates.end());
    savedPoint.lateJoinExpansionAllowed = point.lateJoinExpansionAllowed;
    savedPoint.achievedFederates.reserve(point.achievedFederates.size());
    for (auto const& [federateId, succeeded] : point.achievedFederates) {
      savedPoint.achievedFederates.emplace_back(federateId, succeeded);
    }
    image.synchronizationPoints.push_back(std::move(savedPoint));
  }
  image.synchronizationPointsPresent = true;

  image.objectClassAttributeDeclarations.reserve(
      federation.objectClassAttributeDeclarations.size());
  for (auto const& [federateId, declarations] :
       federation.objectClassAttributeDeclarations) {
    FederationStateImageObjectClassAttributeDeclarations savedDeclarations;
    savedDeclarations.federateId = federateId;
    savedDeclarations.subscriptionGeneration = declarations.subscriptionGeneration;
    savedDeclarations.classes.reserve(declarations.byObjectClass.size());
    for (auto const& [objectClassHandle, perClass] : declarations.byObjectClass) {
      FederationStateImageObjectClassAttributeClass savedClass;
      savedClass.objectClassHandle = objectClassHandle;
      savedClass.privilegeToDeleteExplicitlyUnpublished =
          perClass.privilegeToDeleteExplicitlyUnpublished;
      savedClass.explicitlyPublishedAttributeHandles.assign(
          perClass.explicitlyPublishedAttributes.begin(),
          perClass.explicitlyPublishedAttributes.end());
      savedClass.subscribedAttributes.reserve(perClass.subscribedAttributes.size());
      for (auto const& [attributeHandle, active] : perClass.subscribedAttributes) {
        savedClass.subscribedAttributes.push_back({attributeHandle, active});
      }
      savedClass.subscribedUpdateRateDesignators.reserve(
          perClass.subscribedUpdateRateDesignators.size());
      for (auto const& [attributeHandle, designator] :
           perClass.subscribedUpdateRateDesignators) {
        savedClass.subscribedUpdateRateDesignators.push_back({
            attributeHandle,
            designator,
        });
      }
      for (auto const& [attributeHandle, regions] :
           perClass.regionalSubscribedAttributes) {
        for (auto const& [regionHandle, active] : regions) {
          savedClass.regionalSubscribedAttributes.push_back({
              attributeHandle,
              regionHandle,
              active,
          });
        }
      }
      for (auto const& [attributeHandle, regions] :
           perClass.regionalSubscribedUpdateRateDesignators) {
        for (auto const& [regionHandle, designator] : regions) {
          savedClass.regionalSubscribedUpdateRateDesignators.push_back({
              attributeHandle,
              regionHandle,
              designator,
          });
        }
      }
      for (auto const& [attributeHandle, transportation] :
           perClass.defaultTransportationTypes) {
        savedClass.defaultTransportationTypes.push_back({
            attributeHandle,
            transportation,
        });
      }
      for (auto const& [attributeHandle, order] : perClass.defaultOrderTypes) {
        savedClass.defaultOrderTypes.push_back({
            attributeHandle,
            static_cast<std::uint32_t>(order),
        });
      }
      savedDeclarations.classes.push_back(std::move(savedClass));
    }
    image.objectClassAttributeDeclarations.push_back(std::move(savedDeclarations));
  }
  image.objectClassAttributeDeclarationsPresent = true;

  image.interactionDeclarations.reserve(federation.interactionDeclarations.size());
  for (auto const& [federateId, declarations] : federation.interactionDeclarations) {
    FederationStateImageInteractionDeclaration savedDeclarations;
    savedDeclarations.federateId = federateId;
    savedDeclarations.publishedInteractionClasses.assign(
        declarations.publishedInteractionClasses.begin(),
        declarations.publishedInteractionClasses.end());
    savedDeclarations.subscribedInteractionClasses.reserve(
        declarations.subscribedInteractionClasses.size());
    for (auto const& [interactionClassHandle, active] :
         declarations.subscribedInteractionClasses) {
      savedDeclarations.subscribedInteractionClasses.push_back({
          interactionClassHandle,
          active,
      });
    }
    for (auto const& [interactionClassHandle, regions] :
         declarations.regionalSubscribedInteractionClasses) {
      for (auto const& [regionHandle, active] : regions) {
        savedDeclarations.regionalSubscribedInteractionClasses.push_back({
            interactionClassHandle,
            regionHandle,
            active,
        });
      }
    }
    for (auto const& [objectClassHandle, interactionClassHandles] :
         declarations.publishedObjectClassDirectedInteractions) {
      for (auto const interactionClassHandle : interactionClassHandles) {
        savedDeclarations.publishedObjectClassDirectedInteractions.push_back({
            objectClassHandle,
            interactionClassHandle,
        });
      }
    }
    for (auto const& [objectClassHandle, interactionClassHandles] :
         declarations.subscribedObjectClassDirectedInteractions) {
      for (auto const& [interactionClassHandle, active] : interactionClassHandles) {
        savedDeclarations.subscribedObjectClassDirectedInteractions.push_back({
            objectClassHandle,
            interactionClassHandle,
            active,
        });
      }
    }
    for (auto const& [interactionClassHandle, transportation] :
         declarations.interactionTransportationTypes) {
      savedDeclarations.interactionTransportationTypes.push_back({
          interactionClassHandle,
          transportation,
      });
    }
    for (auto const& [interactionClassHandle, order] : declarations.interactionOrderTypes) {
      savedDeclarations.interactionOrderTypes.push_back({
          interactionClassHandle,
          static_cast<std::uint32_t>(order),
      });
    }
    for (auto const& [interactionClassHandle, transportation] :
         declarations.pendingInteractionTransportationTypeChanges) {
      savedDeclarations.pendingInteractionTransportationTypeChanges.push_back({
          interactionClassHandle,
          transportation,
      });
    }
    image.interactionDeclarations.push_back(std::move(savedDeclarations));
  }

  image.tsoInteractionMessages.reserve(federation.tsoInteractionMessages.size());
  for (auto const& [messageId, message] : federation.tsoInteractionMessages) {
    FederationStateImageTsoInteractionMessage savedMessage;
    savedMessage.messageId = messageId;
    savedMessage.producingFederateId = message.producingFederateId;
    savedMessage.sentInteractionClassHandle = message.sentInteractionClassHandle;
    savedMessage.sentParameterHandles = message.sentParameterHandles;
    savedMessage.parameters.reserve(message.parameters.size());
    for (auto const& [parameterHandle, value] : message.parameters) {
      savedMessage.parameters.push_back({
          parameterHandle,
          bytesFromVariableLengthData(value),
      });
    }
    savedMessage.userSuppliedTag = bytesFromVariableLengthData(message.userSuppliedTag);
    savedMessage.transportationName = message.transportationName;
    savedMessage.sentRegionHandles.assign(
        message.sentRegionHandles.begin(),
        message.sentRegionHandles.end());
    savedMessage.sentRegionSnapshots.reserve(message.sentRegionSnapshots.size());
    for (auto const& [regionHandle, snapshot] : message.sentRegionSnapshots) {
      FederationStateImageInteractionRegionSnapshot savedSnapshot;
      savedSnapshot.regionHandle = regionHandle;
      savedSnapshot.specificationCommitted = snapshot.specificationCommitted;
      savedSnapshot.dimensionHandles.assign(
          snapshot.dimensionHandles.begin(),
          snapshot.dimensionHandles.end());
      savedSnapshot.committedRangeBounds.reserve(snapshot.committedRangeBounds.size());
      for (auto const& [dimensionHandle, bounds] : snapshot.committedRangeBounds) {
        savedSnapshot.committedRangeBounds.push_back({
            dimensionHandle,
            static_cast<std::uint64_t>(bounds.lowerBound),
            static_cast<std::uint64_t>(bounds.upperBound),
        });
      }
      savedMessage.sentRegionSnapshots.push_back(std::move(savedSnapshot));
    }
    savedMessage.defaultRegionUsed = message.defaultRegionUsed;
    savedMessage.timestampEncoding = encodedLogicalValue(message.timestamp);
    savedMessage.sentOrderType = static_cast<std::uint32_t>(message.sentOrderType);
    savedMessage.receivedOrderType = static_cast<std::uint32_t>(message.receivedOrderType);
    image.tsoInteractionMessages.push_back(std::move(savedMessage));
  }

  image.tsoAttributeUpdateMessages.reserve(
      federation.tsoAttributeUpdateMessages.size());
  for (auto const& [messageId, message] : federation.tsoAttributeUpdateMessages) {
    FederationStateImageTsoAttributeUpdateMessage savedMessage;
    savedMessage.messageId = messageId;
    savedMessage.producingFederateId = message.producingFederateId;
    savedMessage.objectInstanceHandle = message.objectInstanceHandle;
    savedMessage.attributes.reserve(message.attributes.size());
    for (auto const& [attributeHandle, value] : message.attributes) {
      savedMessage.attributes.push_back({
          attributeHandle,
          bytesFromVariableLengthData(value),
      });
    }
    savedMessage.userSuppliedTag = bytesFromVariableLengthData(message.userSuppliedTag);
    savedMessage.passelsByRecipient.reserve(message.passelsByRecipient.size());
    for (auto const& [recipientFederateId, passels] : message.passelsByRecipient) {
      FederationStateImageTsoAttributeUpdateRecipient savedRecipient;
      savedRecipient.receivingFederateId = recipientFederateId;
      savedRecipient.passels.reserve(passels.size());
      for (auto const& passel : passels) {
        FederationStateImageTsoAttributeUpdatePassel savedPassel;
        savedPassel.transportationName = passel.transportationName;
        savedPassel.sentAttributeHandles.assign(
            passel.sentAttributeHandles.begin(),
            passel.sentAttributeHandles.end());
        savedPassel.sentRegionHandles.assign(
            passel.sentRegionHandles.begin(),
            passel.sentRegionHandles.end());
        savedPassel.sentRegionSnapshots.reserve(passel.sentRegionSnapshots.size());
        for (auto const& [regionHandle, snapshot] : passel.sentRegionSnapshots) {
          FederationStateImageInteractionRegionSnapshot savedSnapshot;
          savedSnapshot.regionHandle = regionHandle;
          savedSnapshot.specificationCommitted = snapshot.specificationCommitted;
          savedSnapshot.dimensionHandles.assign(
              snapshot.dimensionHandles.begin(),
              snapshot.dimensionHandles.end());
          savedSnapshot.committedRangeBounds.reserve(snapshot.committedRangeBounds.size());
          for (auto const& [dimensionHandle, bounds] : snapshot.committedRangeBounds) {
            savedSnapshot.committedRangeBounds.push_back({
                dimensionHandle,
                static_cast<std::uint64_t>(bounds.lowerBound),
                static_cast<std::uint64_t>(bounds.upperBound),
            });
          }
          savedPassel.sentRegionSnapshots.push_back(std::move(savedSnapshot));
        }
        savedPassel.defaultRegionUsed = passel.defaultRegionUsed;
        savedPassel.preferredOrderType =
            static_cast<std::uint32_t>(passel.preferredOrderType);
        savedRecipient.passels.push_back(std::move(savedPassel));
      }
      savedMessage.passelsByRecipient.push_back(std::move(savedRecipient));
    }
    savedMessage.sentRegionSnapshots.reserve(message.sentRegionSnapshots.size());
    for (auto const& [regionHandle, snapshot] : message.sentRegionSnapshots) {
      FederationStateImageInteractionRegionSnapshot savedSnapshot;
      savedSnapshot.regionHandle = regionHandle;
      savedSnapshot.specificationCommitted = snapshot.specificationCommitted;
      savedSnapshot.dimensionHandles.assign(
          snapshot.dimensionHandles.begin(),
          snapshot.dimensionHandles.end());
      savedSnapshot.committedRangeBounds.reserve(snapshot.committedRangeBounds.size());
      for (auto const& [dimensionHandle, bounds] : snapshot.committedRangeBounds) {
        savedSnapshot.committedRangeBounds.push_back({
            dimensionHandle,
            static_cast<std::uint64_t>(bounds.lowerBound),
            static_cast<std::uint64_t>(bounds.upperBound),
        });
      }
      savedMessage.sentRegionSnapshots.push_back(std::move(savedSnapshot));
    }
    savedMessage.timestampEncoding = encodedLogicalValue(message.timestamp);
    image.tsoAttributeUpdateMessages.push_back(std::move(savedMessage));
  }

  image.tsoObjectDeletionMessages.reserve(
      federation.tsoObjectDeletionMessages.size());
  for (auto const& [messageId, message] : federation.tsoObjectDeletionMessages) {
    FederationStateImageTsoObjectDeletionMessage savedMessage;
    savedMessage.messageId = messageId;
    savedMessage.producingFederateId = message.producingFederateId;
    savedMessage.objectInstanceHandle = message.objectInstanceHandle;
    savedMessage.userSuppliedTag = bytesFromVariableLengthData(message.userSuppliedTag);
    savedMessage.sentOrderType = static_cast<std::uint32_t>(message.sentOrderType);
    savedMessage.recipients.reserve(message.recipients.size());
    for (auto const& recipient : message.recipients) {
      savedMessage.recipients.push_back({
          recipient.receivingFederateId,
          recipient.objectInstanceHandle,
      });
    }
    savedMessage.timestampEncoding = encodedLogicalValue(message.timestamp);
    auto const reconstitution = federation.tsoObjectDeletionReconstitutionRecords.find(
        messageId);
    if (reconstitution == federation.tsoObjectDeletionReconstitutionRecords.end()) {
      throw std::logic_error(
          "A timestamped object deletion has no invocation-time reconstitution record.");
    }
    auto const& invocationObject = reconstitution->second.objectInstanceAtInvocation;
    FederationStateImageTsoObjectDeletionReconstitution savedReconstitution;
    savedReconstitution.object.handle = invocationObject.handle;
    savedReconstitution.object.name = invocationObject.name;
    savedReconstitution.object.registeredObjectClassHandle =
        invocationObject.registeredObjectClassHandle;
    savedReconstitution.object.producingFederateId = invocationObject.producingFederateId;
    savedReconstitution.object.deleteAccepted = invocationObject.deleteAccepted;
    savedReconstitution.object.pendingOperationCount =
        invocationObject.pendingAttributeOwnershipAcquisitionIfAvailableRequests.size() +
        invocationObject.pendingAttributeOwnershipAcquisitionRequests.size() +
        invocationObject.pendingAttributeOwnershipAcquisitionCancellations.size() +
        invocationObject.pendingAttributeOwnershipDivestitureIfWantedNotifications.size() +
        invocationObject.pendingNegotiatedAttributeOwnershipDivestitures.size() +
        invocationObject.pendingConfirmDivestitureNotifications.size() +
        invocationObject.pendingAttributeTransportationTypeChanges.size() +
        invocationObject.pendingDiscoveryFederates.size() +
        invocationObject.pendingRemovalFederates.size() +
        invocationObject.connectionLossAutomaticRemovalFederates.size() +
        invocationObject.deferredConnectionLossTsoRemovalFederates.size() +
        invocationObject.pendingTimestampedRemovalFederates.size() +
        invocationObject.ownershipAssumptionRecipientsByAttribute.size() +
        invocationObject.ownershipAssumptionUserSuppliedTagsByAttribute.size() +
        (invocationObject.pendingTimestampedDeletionMessageId.has_value() ? 1U : 0U);
    std::set<std::uint64_t> invocationAttributeHandles;
    for (auto const& [attributeHandle, owner] : invocationObject.attributeOwnersByHandle) {
      static_cast<void>(owner);
      invocationAttributeHandles.insert(attributeHandle);
    }
    for (auto const& [attributeHandle, transportation] :
         invocationObject.attributeTransportationTypes) {
      static_cast<void>(transportation);
      invocationAttributeHandles.insert(attributeHandle);
    }
    for (auto const& [attributeHandle, order] : invocationObject.attributeOrderTypes) {
      static_cast<void>(order);
      invocationAttributeHandles.insert(attributeHandle);
    }
    for (auto const& [attributeHandle, regions] : invocationObject.updateRegionsByAttribute) {
      static_cast<void>(regions);
      invocationAttributeHandles.insert(attributeHandle);
    }
    for (auto const& [attributeHandle, value] : invocationObject.attributeValues) {
      static_cast<void>(value);
      invocationAttributeHandles.insert(attributeHandle);
    }
    savedReconstitution.object.attributes.reserve(invocationAttributeHandles.size());
    for (auto const attributeHandle : invocationAttributeHandles) {
      FederationStateImageObjectAttribute savedAttribute;
      savedAttribute.handle = attributeHandle;
      auto const owner = invocationObject.attributeOwnersByHandle.find(attributeHandle);
      if (owner != invocationObject.attributeOwnersByHandle.end()) {
        savedAttribute.ownerFederateId = owner->second;
      }
      auto const transportation = invocationObject.attributeTransportationTypes.find(
          attributeHandle);
      if (transportation != invocationObject.attributeTransportationTypes.end()) {
        savedAttribute.transportationName = transportation->second;
      }
      auto const order = invocationObject.attributeOrderTypes.find(attributeHandle);
      if (order != invocationObject.attributeOrderTypes.end()) {
        savedAttribute.orderType = static_cast<std::uint32_t>(order->second);
      }
      auto const regions = invocationObject.updateRegionsByAttribute.find(attributeHandle);
      if (regions != invocationObject.updateRegionsByAttribute.end()) {
        savedAttribute.updateRegionHandles.assign(
            regions->second.begin(), regions->second.end());
      }
      savedReconstitution.object.attributes.push_back(std::move(savedAttribute));
    }
    savedReconstitution.object.attributeValues.reserve(
        invocationObject.attributeValues.size());
    for (auto const& [attributeHandle, value] : invocationObject.attributeValues) {
      savedReconstitution.object.attributeValues.push_back({
          attributeHandle,
          bytesFromVariableLengthData(value),
      });
    }
    savedReconstitution.object.attributeValuesPresent = true;
    savedReconstitution.knownObjectClassHandlesByFederate.reserve(
        invocationObject.knownObjectClassHandlesByFederate.size());
    for (auto const& [knownFederateId, knownObjectClassHandle] :
         invocationObject.knownObjectClassHandlesByFederate) {
      savedReconstitution.knownObjectClassHandlesByFederate.emplace_back(
          knownFederateId,
          knownObjectClassHandle);
    }
    savedMessage.reconstitution = std::move(savedReconstitution);
    image.tsoObjectDeletionMessages.push_back(std::move(savedMessage));
  }

  image.tsoRequestRetractionRecords.reserve(
      federation.tsoRequestRetractionRecords.size());
  for (auto const& [messageId, record] : federation.tsoRequestRetractionRecords) {
    FederationStateImageTsoRequestRetractionRecord savedRecord;
    savedRecord.messageId = messageId;
    savedRecord.producingFederateId = record.producingFederateId;
    savedRecord.timestampEncoding = encodedLogicalValue(record.timestamp);
    savedRecord.retractionApplied = record.retractionApplied;
    savedRecord.terminal = record.terminal;
    savedRecord.producerResigned = record.producerResigned;
    savedRecord.deliveryRequiredAfterConnectionLoss =
        record.deliveryRequiredAfterConnectionLoss;
    savedRecord.recipientStates.reserve(record.recipientStates.size());
    for (auto const& [receivingFederateId, state] : record.recipientStates) {
      savedRecord.recipientStates.push_back({
          receivingFederateId,
          static_cast<std::uint32_t>(state),
      });
    }
    image.tsoRequestRetractionRecords.push_back(std::move(savedRecord));
  }

  image.tsoDirectedInteractionMessages.reserve(
      federation.tsoDirectedInteractionMessages.size());
  for (auto const& [messageId, message] : federation.tsoDirectedInteractionMessages) {
    FederationStateImageTsoDirectedInteractionMessage savedMessage;
    savedMessage.messageId = messageId;
    savedMessage.producingFederateId = message.producingFederateId;
    savedMessage.objectInstanceHandle = message.objectInstanceHandle;
    savedMessage.sentInteractionClassHandle = message.sentInteractionClassHandle;
    savedMessage.sentParameterHandles = message.sentParameterHandles;
    savedMessage.parameters.reserve(message.parameters.size());
    for (auto const& [parameterHandle, value] : message.parameters) {
      savedMessage.parameters.push_back({
          parameterHandle,
          bytesFromVariableLengthData(value),
      });
    }
    savedMessage.userSuppliedTag = bytesFromVariableLengthData(message.userSuppliedTag);
    savedMessage.transportationName = message.transportationName;
    savedMessage.recipients.reserve(message.recipients.size());
    for (auto const& recipient : message.recipients) {
      FederationStateImageTsoDirectedInteractionRecipient savedRecipient;
      savedRecipient.receivingFederateId = recipient.receivingFederateId;
      savedRecipient.objectInstanceHandle = recipient.objectInstanceHandle;
      savedRecipient.receivedInteractionClassHandle =
          recipient.receivedInteractionClassHandle;
      savedRecipient.receivedParameterHandles.assign(
          recipient.receivedParameterHandles.begin(),
          recipient.receivedParameterHandles.end());
      savedMessage.recipients.push_back(std::move(savedRecipient));
    }
    savedMessage.timestampEncoding = encodedLogicalValue(message.timestamp);
    savedMessage.sentOrderType = static_cast<std::uint32_t>(message.sentOrderType);
    savedMessage.receivedOrderType = static_cast<std::uint32_t>(message.receivedOrderType);
    image.tsoDirectedInteractionMessages.push_back(std::move(savedMessage));
  }

  auto const timeStates = federation.timeCoordinator.snapshot();
  image.timeStates.reserve(timeStates.size());
  image.tsoQueueEntries.reserve(
      federation.tsoInteractionMessages.size() +
      federation.tsoAttributeUpdateMessages.size() +
      federation.tsoObjectDeletionMessages.size() +
      federation.tsoDirectedInteractionMessages.size());
  auto appendQueueEntries = [
      &image](std::vector<TsoQueuedMessage> const& entries, std::uint32_t phase) {
    for (auto const& entry : entries) {
      image.tsoQueueEntries.push_back({
          entry.messageId,
          entry.recipientFederateId,
          entry.sequence,
          phase,
          encodedLogicalValue(entry.timestamp),
      });
    }
  };
  for (auto const& registered : timeStates) {
    auto const& state = registered.time;
    FederationStateImageTimeState savedTime;
    savedTime.federateId = registered.federateId;
    savedTime.implementationName = state.implementationName;
    savedTime.flags =
        (state.active ? 1U : 0U) |
        (state.timeRegulating ? 1U << 1U : 0U) |
        (state.timeConstrained ? 1U << 2U : 0U) |
        (state.asynchronousDeliveryEnabled ? 1U << 3U : 0U) |
        (state.timeAdvancePending ? 1U << 4U : 0U) |
        (state.timeRegulationPending ? 1U << 5U : 0U) |
        (state.timeConstrainedPending ? 1U << 6U : 0U) |
        (state.minimumTimestampIsExclusive ? 1U << 7U : 0U);
    savedTime.pendingGeneration = state.pendingTimeAdvanceGeneration;
    savedTime.advanceMode = static_cast<std::uint32_t>(state.advanceMode);
    savedTime.currentTimeEncoding = encodedLogicalValue(state.currentTime);
    savedTime.optimisticTimeEncoding = encodedLogicalValue(state.optimisticTime);
    savedTime.requestedTimeEncoding = encodedLogicalValue(state.requestedTime);
    savedTime.advanceRequestTimeEncoding = encodedLogicalValue(state.advanceRequestTime);
    savedTime.lookaheadEncoding = encodedLogicalValue(state.lookahead);
    savedTime.requestedLookaheadEncoding = encodedLogicalValue(state.requestedLookahead);
    auto const tso = federation.timeCoordinator.tsoSnapshotFor(registered.federateId);
    savedTime.queuedTsoCount = tso.queued.size();
    savedTime.inTransitTsoCount = tso.inTransit.size();
    savedTime.deliveredTsoCount = tso.deliveredSinceLastAdvance.size();
    savedTime.pendingTimeRegulationGeneration =
        state.pendingTimeRegulationGeneration;
    savedTime.pendingTimeConstrainedGeneration =
        state.pendingTimeConstrainedGeneration;
    savedTime.nextGeneration = state.nextGeneration;
    savedTime.pendingModifiedLookaheadEncoding =
        encodedLogicalValue(state.pendingModifiedLookahead);
    savedTime.applicationRequestLedgerPresent = true;
    image.timeStates.push_back(std::move(savedTime));
  }

  // Every joined federate is a valid TSO recipient, including members that
  // have not enabled a time-management role and therefore have no
  // FederationStateImageTimeState record. Snapshot queue phases by member so
  // save images retain the recipient identity for all accepted fanout.
  for (auto const& [federateId, member] : federation.members) {
    static_cast<void>(member);
    auto const tso = federation.timeCoordinator.tsoSnapshotFor(federateId);
    appendQueueEntries(tso.queued, 0U);
    appendQueueEntries(tso.inTransit, 1U);
    appendQueueEntries(tso.deliveredSinceLastAdvance, 2U);
  }

  image.objects.reserve(federation.objectInstances.size());
  for (auto const& [objectHandle, object] : federation.objectInstances) {
    FederationStateImageObject savedObject;
    savedObject.handle = objectHandle;
    savedObject.name = object.name;
    savedObject.registeredObjectClassHandle = object.registeredObjectClassHandle;
    savedObject.producingFederateId = object.producingFederateId;
    savedObject.deleteAccepted = object.deleteAccepted;
    savedObject.pendingOperationCount =
        object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.size() +
        object.pendingAttributeOwnershipAcquisitionRequests.size() +
        object.pendingAttributeOwnershipAcquisitionCancellations.size() +
        object.pendingAttributeOwnershipDivestitureIfWantedNotifications.size() +
        object.pendingNegotiatedAttributeOwnershipDivestitures.size() +
        object.pendingConfirmDivestitureNotifications.size() +
        object.pendingAttributeTransportationTypeChanges.size() +
        object.pendingAttributeValueUpdateRequests.size() +
        object.pendingAttributeValueUpdateClassRequests.size() +
        object.pendingAttributeValueUpdateRegionalRequests.size() +
        std::count_if(
            federation.pendingAttributeOwnershipQueries.begin(),
            federation.pendingAttributeOwnershipQueries.end(),
            [&objectHandle](auto const& entry) {
              return entry.second.objectInstanceHandle == objectHandle;
            }) +
        object.pendingDiscoveryFederates.size() +
        object.pendingRemovalFederates.size() +
        object.connectionLossAutomaticRemovalFederates.size() +
        object.deferredConnectionLossTsoRemovalFederates.size() +
        object.pendingTimestampedRemovalFederates.size() +
        object.ownershipAssumptionRecipientsByAttribute.size() +
        object.ownershipAssumptionUserSuppliedTagsByAttribute.size() +
        object.pendingAttributeOwnershipAssumptionCallbacks.size() +
        (object.pendingTimestampedDeletionMessageId.has_value() ? 1U : 0U);

    std::set<std::uint64_t> attributeHandles;
    for (auto const& [attributeHandle, owner] : object.attributeOwnersByHandle) {
      static_cast<void>(owner);
      attributeHandles.insert(attributeHandle);
    }
    for (auto const& [attributeHandle, transportation] : object.attributeTransportationTypes) {
      static_cast<void>(transportation);
      attributeHandles.insert(attributeHandle);
    }
    for (auto const& [attributeHandle, order] : object.attributeOrderTypes) {
      static_cast<void>(order);
      attributeHandles.insert(attributeHandle);
    }
    for (auto const& [attributeHandle, regions] : object.updateRegionsByAttribute) {
      static_cast<void>(regions);
      attributeHandles.insert(attributeHandle);
    }
    for (auto const& [attributeHandle, value] : object.attributeValues) {
      static_cast<void>(value);
      attributeHandles.insert(attributeHandle);
    }
    for (auto const& [attributeHandle, recipients] :
         object.ownershipAssumptionRecipientsByAttribute) {
      static_cast<void>(recipients);
      attributeHandles.insert(attributeHandle);
    }
    for (auto const& [attributeHandle, tag] :
         object.ownershipAssumptionUserSuppliedTagsByAttribute) {
      static_cast<void>(tag);
      attributeHandles.insert(attributeHandle);
    }
    savedObject.attributes.reserve(attributeHandles.size());
    for (auto const attributeHandle : attributeHandles) {
      FederationStateImageObjectAttribute savedAttribute;
      savedAttribute.handle = attributeHandle;
      auto const owner = object.attributeOwnersByHandle.find(attributeHandle);
      if (owner != object.attributeOwnersByHandle.end()) {
        savedAttribute.ownerFederateId = owner->second;
      }
      auto const transportation = object.attributeTransportationTypes.find(attributeHandle);
      if (transportation != object.attributeTransportationTypes.end()) {
        savedAttribute.transportationName = transportation->second;
      }
      auto const order = object.attributeOrderTypes.find(attributeHandle);
      if (order != object.attributeOrderTypes.end()) {
        savedAttribute.orderType = static_cast<std::uint32_t>(order->second);
      }
      auto const regions = object.updateRegionsByAttribute.find(attributeHandle);
      if (regions != object.updateRegionsByAttribute.end()) {
        savedAttribute.updateRegionHandles.assign(
            regions->second.begin(), regions->second.end());
      }
      savedObject.attributes.push_back(std::move(savedAttribute));
    }
    savedObject.attributeValues.reserve(object.attributeValues.size());
    for (auto const& [attributeHandle, value] : object.attributeValues) {
      savedObject.attributeValues.push_back({
          attributeHandle,
          bytesFromVariableLengthData(value),
      });
    }
    savedObject.attributeValuesPresent = true;
    savedObject.pendingAttributeValueUpdateRequestsPresent = true;
    savedObject.pendingAttributeValueUpdateRequests.reserve(
        object.pendingAttributeValueUpdateRequests.size());
    for (auto const& [requestId, request] :
         object.pendingAttributeValueUpdateRequests) {
      savedObject.pendingAttributeValueUpdateRequests.push_back({
          requestId,
          request.requestingFederateId,
          request.providingFederateId,
          std::vector<std::uint64_t>(
              request.requestedAttributeHandles.begin(),
              request.requestedAttributeHandles.end()),
          std::string(request.userSuppliedTag.begin(),
                      request.userSuppliedTag.end()),
      });
    }
    savedObject.pendingAttributeValueUpdateClassRequestsPresent = true;
    savedObject.pendingAttributeValueUpdateClassRequests.reserve(
        object.pendingAttributeValueUpdateClassRequests.size());
    for (auto const& [requestId, request] :
         object.pendingAttributeValueUpdateClassRequests) {
      savedObject.pendingAttributeValueUpdateClassRequests.push_back({
          requestId,
          request.requestingFederateId,
          request.providingFederateId,
          request.requestedObjectClassHandle,
          std::vector<std::uint64_t>(
              request.requestedAttributeHandles.begin(),
              request.requestedAttributeHandles.end()),
          std::string(request.userSuppliedTag.begin(),
                      request.userSuppliedTag.end()),
      });
    }
    savedObject.pendingAttributeValueUpdateRegionalRequestsPresent = true;
    savedObject.pendingAttributeValueUpdateRegionalRequests.reserve(
        object.pendingAttributeValueUpdateRegionalRequests.size());
    for (auto const& [requestId, request] :
         object.pendingAttributeValueUpdateRegionalRequests) {
      FederationStateImagePendingAttributeValueUpdateRegional savedRequest;
      savedRequest.requestId = requestId;
      savedRequest.requestingFederateId = request.requestingFederateId;
      savedRequest.providingFederateId = request.providingFederateId;
      savedRequest.requestedObjectClassHandle = request.requestedObjectClassHandle;
      savedRequest.requestedAttributeHandles.assign(
          request.requestedAttributeHandles.begin(),
          request.requestedAttributeHandles.end());
      for (auto const& [attributeHandle, regionHandles] :
           request.requestRegionsByAttribute) {
        savedRequest.requestRegionsByAttribute.push_back({
            attributeHandle,
            std::vector<std::uint64_t>(
                regionHandles.begin(), regionHandles.end()),
        });
      }
      savedRequest.userSuppliedTag.assign(
          request.userSuppliedTag.begin(), request.userSuppliedTag.end());
      savedObject.pendingAttributeValueUpdateRegionalRequests.push_back(
          std::move(savedRequest));
    }
    savedObject.pendingAttributeOwnershipAcquisitionIfAvailableRequests.reserve(
        object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.size());
    for (auto const& [requestId, request] :
         object.pendingAttributeOwnershipAcquisitionIfAvailableRequests) {
      savedObject.pendingAttributeOwnershipAcquisitionIfAvailableRequests.push_back({
          requestId,
          request.requestingFederateId,
          request.requestSequence,
          std::vector<std::uint64_t>(
              request.desiredAttributeHandles.begin(),
              request.desiredAttributeHandles.end()),
          std::string(request.userSuppliedTag.begin(), request.userSuppliedTag.end()),
      });
    }
    savedObject.pendingAttributeOwnershipAcquisitionRequests.reserve(
        object.pendingAttributeOwnershipAcquisitionRequests.size());
    for (auto const& [requestId, request] :
         object.pendingAttributeOwnershipAcquisitionRequests) {
      FederationStateImagePendingAttributeOwnershipAcquisition savedRequest;
      savedRequest.requestId = requestId;
      savedRequest.requestingFederateId = request.requestingFederateId;
      savedRequest.requestSequence = request.requestSequence;
      savedRequest.desiredAttributeHandles.assign(
          request.desiredAttributeHandles.begin(),
          request.desiredAttributeHandles.end());
      savedRequest.notificationQueuedAttributeHandles.assign(
          request.notificationQueuedAttributeHandles.begin(),
          request.notificationQueuedAttributeHandles.end());
      savedRequest.unavailableQueuedAttributeHandles.assign(
          request.unavailableQueuedAttributeHandles.begin(),
          request.unavailableQueuedAttributeHandles.end());
      savedRequest.releaseCallbacksQueuedByOwningFederate.reserve(
          request.releaseCallbacksQueuedByOwningFederate.size());
      for (auto const& [ownerFederateId, attributes] :
           request.releaseCallbacksQueuedByOwningFederate) {
        savedRequest.releaseCallbacksQueuedByOwningFederate.emplace_back(
            ownerFederateId,
            std::vector<std::uint64_t>(attributes.begin(), attributes.end()));
      }
      savedRequest.userSuppliedTag =
          std::string(request.userSuppliedTag.begin(), request.userSuppliedTag.end());
      savedObject.pendingAttributeOwnershipAcquisitionRequests.push_back(
          std::move(savedRequest));
    }
    savedObject.pendingAttributeOwnershipAcquisitionCancellations.reserve(
        object.pendingAttributeOwnershipAcquisitionCancellations.size());
    for (auto const& [cancellationId, cancellation] :
         object.pendingAttributeOwnershipAcquisitionCancellations) {
      savedObject.pendingAttributeOwnershipAcquisitionCancellations.push_back({
          cancellationId,
          cancellation.requestingFederateId,
          std::vector<std::uint64_t>(
              cancellation.attributeHandles.begin(),
              cancellation.attributeHandles.end()),
      });
    }
    savedObject.pendingAttributeOwnershipDivestitureIfWantedNotifications.reserve(
        object.pendingAttributeOwnershipDivestitureIfWantedNotifications.size());
    for (auto const& [notificationId, notification] :
         object.pendingAttributeOwnershipDivestitureIfWantedNotifications) {
      savedObject.pendingAttributeOwnershipDivestitureIfWantedNotifications.push_back({
          notificationId,
          notification.receivingFederateId,
          std::vector<std::uint64_t>(
              notification.attributeHandles.begin(),
              notification.attributeHandles.end()),
          std::string(
              notification.userSuppliedTag.begin(),
              notification.userSuppliedTag.end()),
      });
    }
    savedObject.pendingConfirmDivestitureNotifications.reserve(
        object.pendingConfirmDivestitureNotifications.size());
    for (auto const& [notificationId, notification] :
         object.pendingConfirmDivestitureNotifications) {
      savedObject.pendingConfirmDivestitureNotifications.push_back({
          notificationId,
          notification.receivingFederateId,
          std::vector<std::uint64_t>(
              notification.attributeHandles.begin(),
              notification.attributeHandles.end()),
          std::string(
              notification.userSuppliedTag.begin(),
              notification.userSuppliedTag.end()),
      });
    }
    savedObject.pendingAttributeTransportationTypeChanges.reserve(
        object.pendingAttributeTransportationTypeChanges.size());
    for (auto const& [requestId, request] :
         object.pendingAttributeTransportationTypeChanges) {
      savedObject.pendingAttributeTransportationTypeChanges.push_back({
          requestId,
          request.requestingFederateId,
          std::vector<std::uint64_t>(
              request.attributeHandles.begin(),
              request.attributeHandles.end()),
          request.transportationName,
      });
    }
    savedObject.pendingNegotiatedAttributeOwnershipDivestitures.reserve(
        object.pendingNegotiatedAttributeOwnershipDivestitures.size());
    for (auto const& [attributeHandle, divestiture] :
         object.pendingNegotiatedAttributeOwnershipDivestitures) {
      savedObject.pendingNegotiatedAttributeOwnershipDivestitures.push_back({
          attributeHandle,
          divestiture.divestingFederateId,
          divestiture.acquiringFederateId,
          divestiture.acquisitionRequestId,
          divestiture.acquiringFederateIsIfAvailable,
          divestiture.confirmationQueued,
          divestiture.confirmationDelivered,
          std::string(divestiture.userSuppliedTag.begin(),
                      divestiture.userSuppliedTag.end()),
      });
    }
    savedObject.ownershipAssumptionRecipientsByAttribute.reserve(
        object.ownershipAssumptionRecipientsByAttribute.size());
    for (auto const& [attributeHandle, recipients] :
         object.ownershipAssumptionRecipientsByAttribute) {
      savedObject.ownershipAssumptionRecipientsByAttribute.push_back({
          attributeHandle,
          std::vector<std::uint64_t>(recipients.begin(), recipients.end()),
      });
    }
    savedObject.ownershipAssumptionUserSuppliedTagsByAttribute.reserve(
        object.ownershipAssumptionUserSuppliedTagsByAttribute.size());
    for (auto const& [attributeHandle, tag] :
         object.ownershipAssumptionUserSuppliedTagsByAttribute) {
      savedObject.ownershipAssumptionUserSuppliedTagsByAttribute.push_back({
          attributeHandle,
          std::string(tag.begin(), tag.end()),
      });
    }
    savedObject.knownObjectClassHandlesByFederate.reserve(
        object.knownObjectClassHandlesByFederate.size());
    for (auto const& [federateId, objectClassHandle] :
         object.knownObjectClassHandlesByFederate) {
      savedObject.knownObjectClassHandlesByFederate.push_back({
          federateId,
          objectClassHandle,
      });
    }
    savedObject.pendingDiscoveryFederateIds.assign(
        object.pendingDiscoveryFederates.begin(),
        object.pendingDiscoveryFederates.end());
    savedObject.pendingRemovalFederateIds.assign(
        object.pendingRemovalFederates.begin(),
        object.pendingRemovalFederates.end());
    savedObject.connectionLossAutomaticRemovalFederateIds.assign(
        object.connectionLossAutomaticRemovalFederates.begin(),
        object.connectionLossAutomaticRemovalFederates.end());
    savedObject.deferredConnectionLossTsoRemovalFederateIds.assign(
        object.deferredConnectionLossTsoRemovalFederates.begin(),
        object.deferredConnectionLossTsoRemovalFederates.end());
    savedObject.pendingTimestampedRemovalFederateIds.assign(
        object.pendingTimestampedRemovalFederates.begin(),
        object.pendingTimestampedRemovalFederates.end());
    savedObject.pendingTimestampedDeletionMessageId =
        object.pendingTimestampedDeletionMessageId;
    image.objects.push_back(std::move(savedObject));
  }
  image.deferredUpdateRegionAssociationsPresent = true;
  for (auto const& [objectInstanceHandle, object] : federation.objectInstances) {
    for (auto const& [federateId, associationsByAttribute] :
         object.deferredUpdateRegionsByFederate) {
      for (auto const& [attributeHandle, regionHandles] : associationsByAttribute) {
        if (regionHandles.empty()) {
          continue;
        }
        image.deferredUpdateRegionAssociations.push_back({
            objectInstanceHandle,
            federateId,
            attributeHandle,
            std::vector<std::uint64_t>(regionHandles.begin(), regionHandles.end()),
        });
      }
    }
  }
  image.pendingAttributeOwnershipAssumptionsPresent = true;
  for (auto const& [objectInstanceHandle, object] : federation.objectInstances) {
    for (auto const& callback : object.pendingAttributeOwnershipAssumptionCallbacks) {
      image.pendingAttributeOwnershipAssumptions.push_back({
          objectInstanceHandle,
          callback.receivingFederateId,
          std::vector<std::uint64_t>(
              callback.attributeHandles.begin(),
              callback.attributeHandles.end()),
          std::string(callback.userSuppliedTag.begin(), callback.userSuppliedTag.end()),
      });
    }
  }
  image.pendingAttributeOwnershipQueriesPresent = true;
  image.pendingAttributeOwnershipQueries.reserve(
      federation.pendingAttributeOwnershipQueries.size());
  for (auto const& [requestId, request] :
       federation.pendingAttributeOwnershipQueries) {
    image.pendingAttributeOwnershipQueries.push_back({
        requestId,
        request.requestingFederateId,
        request.objectInstanceHandle,
        static_cast<std::uint32_t>(request.reportKind),
        request.owningFederateId,
        std::vector<std::uint64_t>(
            request.requestedAttributeHandles.begin(),
            request.requestedAttributeHandles.end()),
    });
  }
  return image;
}

} // namespace umbra::detail
