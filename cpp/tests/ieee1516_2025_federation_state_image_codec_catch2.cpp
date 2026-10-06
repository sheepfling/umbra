#include <catch2/catch_test_macros.hpp>

#include "internal/federation/federation_registry.hpp"
#include "internal/federation/federation_state_image.hpp"
#include "internal/fom/libxml2_fom_composer.hpp"
#include "internal/fom/libxml2_fom_validator.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include <RTI/time/HLAinteger64Time.h>
#include <RTI/time/HLAinteger64Interval.h>

namespace {

using umbra::detail::EmbeddedFederationRegistry;
using umbra::detail::FederationDefinition;
using umbra::detail::FederationRegistryStatus;
using umbra::detail::FederationTimeGrantStatus;
using umbra::detail::FomModuleKind;
using umbra::detail::PrevalidatedFomModule;
using umbra::detail::InstrumentationLayer;
using umbra::detail::ObjectInstanceNameReservationStatus;
using umbra::detail::FomCompositionStatus;
using umbra::detail::FomValidationStatus;
using umbra::detail::LibXml2FomModuleComposer;
using umbra::detail::LibXml2FomValidator;

TEST_CASE(
    "Federation state images round-trip canonical control, temporal, object, and interaction declaration state",
    "[unit][kernel][federation-registry][save-restore][durable-save][state-image]"
    "[tso-retraction-ledger-state][ownership-ledger-state][object-visibility-state]"
    "[object-lifecycle-state][membership-update-state][membership-reflection-state]"
    "[membership-lifecycle-state][membership-interaction-send-state]"
    "[membership-interaction-receive-state][object-class-declaration-state]"
    "[synchronization-state][region-state][application-value-state]"
    "[pending-application-request-state]") {
  umbra::detail::FederationStateImage image;
  image.federationName = L"exercise / Δ";
  image.logicalTimeImplementationName = L"HLAinteger64Time";
  image.normalizationSeed = 0x123456789abcdef0ULL;
  image.federationSwitches = 0x1fU;
  image.interactionDeclarationCount = 1U;
  image.objectClassDeclarationCount = 1U;
  image.regionCount = 1U;
  image.objectInstanceCount = 3U;
  image.nextObjectInstanceHandle = 19U;
  image.members.push_back({
      7U,
      L"alice / Δ",
      L"trainer",
      0xa5U,
      4U,
      -3,
      12U,
  });
  image.members.front().successfulUpdateAttributeValuesCount = 3U;
  image.members.front().successfulUpdateCountsByClassAndTransportation = {
      {4U, "HLAreliable", 3U},
  };
  image.members.front().successfullyUpdatedObjectInstanceHandles = {19U, 20U};
  image.members.front().successfullyUpdatedObjectInstanceClassHandles = {
      {19U, 4U},
      {20U, 4U},
  };
  image.members.front().successfulReflectionsReceivedCount = 9U;
  image.members.front().successfulObjectInstanceRegistrationsCount = 10U;
  image.members.front().successfulObjectInstanceDeletionsCount = 11U;
  image.members.front().successfulObjectInstanceRemovalsCount = 12U;
  image.members.front().successfulObjectInstanceDiscoveriesCount = 13U;
  image.members.front().successfulReflectionCountsByClassAndTransportation = {
      {4U, "HLAreliable", 2U},
  };
  image.members.front().successfullyReflectedObjectInstanceHandles = {19U, 20U};
  image.members.front().successfullyReflectedObjectInstanceClassHandles = {
      {19U, 4U},
      {20U, 4U},
  };
  image.members.front().successfulInteractionsSentCount = 6U;
  image.members.front().successfulDirectedInteractionsSentCount = 2U;
  image.members.front().successfulInteractionCountsByClassAndTransportation = {
      {22U, "HLAreliable", 6U},
      {23U, "HLAbestEffort", 1U},
  };
  image.members.front().successfulDirectedInteractionCountsByClassAndTransportation = {
      {22U, "HLAreliable", 2U},
  };
  image.members.front().successfulInteractionsReceivedCount = 8U;
  image.members.front().successfulDirectedInteractionsReceivedCount = 3U;
  image.members.front().successfulInteractionReceiptCountsByClassAndTransportation = {
      {22U, "HLAreliable", 8U},
      {23U, "HLAbestEffort", 1U},
  };
  image.members.front().successfulDirectedInteractionReceiptCountsByClassAndTransportation = {
      {22U, "HLAreliable", 3U},
  };
  image.federateNamesById.emplace_back(7U, L"alice / Δ");
  image.reservedObjectInstanceNames = {
      {7U, L"reserved-table"},
  };
  image.synchronizationPointCount = 1U;
  image.synchronizationPoints = {
      {L"restore-sync", std::string{"sync\0", 5U}, {7U}, {7U}, {{7U, false}}},
  };
  image.regions = {
      {8U, 7U, {8U}, {{8U, 1U, 20U}}, {{8U, 2U, 20U}}, true, true},
  };
  umbra::detail::FederationStateImageObjectClassAttributeDeclarations declarations;
  declarations.federateId = 7U;
  declarations.subscriptionGeneration = 17U;
  umbra::detail::FederationStateImageObjectClassAttributeClass objectClass;
  objectClass.objectClassHandle = 4U;
  objectClass.privilegeToDeleteExplicitlyUnpublished = true;
  objectClass.explicitlyPublishedAttributeHandles = {3U};
  objectClass.subscribedAttributes = {{3U, true}};
  // An empty designator is the normative request for the FOM default rate;
  // the state image must preserve that distinction from a missing record.
  objectClass.subscribedUpdateRateDesignators = {{3U, ""}};
  objectClass.regionalSubscribedAttributes = {{3U, 8U, true}};
  objectClass.regionalSubscribedUpdateRateDesignators = {{3U, 8U, "HLAreliable"}};
  objectClass.defaultTransportationTypes = {{3U, "HLAreliable"}};
  objectClass.defaultOrderTypes = {{3U, 1U}};
  declarations.classes.push_back(std::move(objectClass));
  image.objectClassAttributeDeclarations.push_back(std::move(declarations));

  umbra::detail::FederationStateImageTimeState time;
  time.federateId = 7U;
  time.implementationName = L"HLAinteger64Time";
  time.flags = 0x7fU;
  time.pendingGeneration = 11U;
  time.advanceMode = 2U;
  time.pendingTimeRegulationGeneration = 12U;
  time.pendingTimeConstrainedGeneration = 13U;
  time.nextGeneration = 14U;
  time.pendingModifiedLookaheadEncoding = std::string{"\x10\x11", 2U};
  time.applicationRequestLedgerPresent = true;
  time.currentTimeEncoding = std::string{"\0\x01\x02", 3U};
  time.lookaheadEncoding = std::string{"\x03\x04", 2U};
  time.queuedTsoCount = 2U;
  time.inTransitTsoCount = 1U;
  image.timeStates.push_back(std::move(time));
  image.objects.push_back({
      19U,
      L"table-19",
      4U,
      7U,
      false,
      16U,
      {{3U, 7U, "HLAreliable", 1U, {8U, 9U}}},
  });
  image.objects.front().attributeValues = {
      {3U, std::string{"\0\xfe", 2U}},
  };
  image.objects.front().attributeValuesPresent = true;
  image.objects.front().pendingAttributeOwnershipAcquisitionIfAvailableRequests.push_back({
      41U,
      7U,
      12U,
      {3U},
      std::string{"if-available\0", 13U},
  });
  image.objects.front().pendingAttributeOwnershipAcquisitionRequests.push_back({
      42U,
      7U,
      13U,
      {3U},
      {3U},
      {},
      {{8U, {3U}}},
      std::string{"regular\0", 8U},
  });
  image.objects.front().pendingAttributeOwnershipAcquisitionCancellations.push_back({
      43U,
      7U,
      {3U},
  });
  image.objects.front().pendingAttributeOwnershipDivestitureIfWantedNotifications.push_back({
      44U,
      8U,
      {3U},
      std::string{"divestiture-if-wanted\0", 22U},
  });
  image.objects.front().pendingConfirmDivestitureNotifications.push_back({
      45U,
      8U,
      {3U},
      std::string{"confirm-divestiture\0", 20U},
  });
  image.objects.front().pendingAttributeTransportationTypeChanges.push_back({
      46U,
      7U,
      {3U},
      "HLAbestEffort",
  });
  image.objects.front().pendingNegotiatedAttributeOwnershipDivestitures.push_back({
      3U,
      7U,
      8U,
      42U,
      false,
      true,
      false,
      std::string{"negotiated\0", 11U},
  });
  image.objects.front().ownershipAssumptionRecipientsByAttribute.push_back({
      3U,
      {8U, 9U},
  });
  image.objects.front().ownershipAssumptionUserSuppliedTagsByAttribute.push_back({
      3U,
      std::string{"assume\0", 7U},
  });
  image.objects.front().knownObjectClassHandlesByFederate.push_back({
      7U,
      4U,
  });
  image.objects.front().pendingDiscoveryFederateIds = {8U};
  image.objects.front().pendingRemovalFederateIds = {9U};
  image.objects.front().connectionLossAutomaticRemovalFederateIds = {9U};
  image.objects.front().deferredConnectionLossTsoRemovalFederateIds = {9U};
  image.objects.front().pendingTimestampedRemovalFederateIds = {8U};
  image.objects.front().pendingTimestampedDeletionMessageId = 88U;
  umbra::detail::FederationStateImageInteractionDeclaration interactions;
  interactions.federateId = 7U;
  interactions.publishedInteractionClasses = {21U};
  interactions.subscribedInteractionClasses = {{22U, true}};
  interactions.regionalSubscribedInteractionClasses = {{22U, 8U, true}};
  interactions.publishedObjectClassDirectedInteractions = {{4U, 23U}};
  interactions.subscribedObjectClassDirectedInteractions = {{4U, 24U, false}};
  interactions.interactionTransportationTypes = {{22U, "HLAreliable"}};
  interactions.interactionOrderTypes = {{22U, 1U}};
  interactions.pendingInteractionTransportationTypeChanges = {{22U, "HLAbestEffort"}};
  image.interactionDeclarations.push_back(std::move(interactions));

  umbra::detail::FederationStateImageTsoInteractionMessage tsoInteraction;
  tsoInteraction.messageId = 55U;
  tsoInteraction.producingFederateId = 7U;
  tsoInteraction.sentInteractionClassHandle = 22U;
  tsoInteraction.sentParameterHandles = {31U};
  tsoInteraction.parameters = {{31U, std::string{"\0\x01", 2U}}};
  tsoInteraction.userSuppliedTag = std::string{"tag\0", 4U};
  tsoInteraction.transportationName = "HLAreliable";
  tsoInteraction.sentRegionHandles = {8U};
  tsoInteraction.sentRegionSnapshots = {{
      8U,
      true,
      {8U},
      {{8U, 10U, 20U}},
  }};
  tsoInteraction.defaultRegionUsed = false;
  tsoInteraction.timestampEncoding = std::string{"\x05\x06", 2U};
  tsoInteraction.sentOrderType = 1U;
  tsoInteraction.receivedOrderType = 1U;
  image.tsoInteractionMessageCount = 1U;
  image.tsoInteractionMessages.push_back(std::move(tsoInteraction));

  umbra::detail::FederationStateImageTsoAttributeUpdateMessage attributeUpdate;
  attributeUpdate.messageId = 66U;
  attributeUpdate.producingFederateId = 7U;
  attributeUpdate.objectInstanceHandle = 19U;
  attributeUpdate.attributes = {{7U, std::string{"\x0a\x0b", 2U}}};
  attributeUpdate.userSuppliedTag = std::string{"attribute", 9U};
  attributeUpdate.passelsByRecipient = {{
      8U,
      {{
          "HLAreliable",
          {7U},
          {},
          {},
          false,
          1U,
      }},
  }};
  attributeUpdate.timestampEncoding = std::string{"\x0c\x0d", 2U};
  image.tsoAttributeUpdateMessageCount = 1U;
  image.tsoAttributeUpdateMessages.push_back(std::move(attributeUpdate));

  umbra::detail::FederationStateImageTsoDirectedInteractionMessage directedInteraction;
  directedInteraction.messageId = 77U;
  directedInteraction.producingFederateId = 7U;
  directedInteraction.objectInstanceHandle = 19U;
  directedInteraction.sentInteractionClassHandle = 24U;
  directedInteraction.sentParameterHandles = {32U};
  directedInteraction.parameters = {{32U, std::string{"\x07", 1U}}};
  directedInteraction.userSuppliedTag = std::string{"directed", 8U};
  directedInteraction.transportationName = "HLAreliable";
  directedInteraction.recipients = {{8U, 19U, 24U, {32U}}};
  directedInteraction.timestampEncoding = std::string{"\x08\x09", 2U};
  directedInteraction.sentOrderType = 1U;
  directedInteraction.receivedOrderType = 1U;
  image.tsoDirectedInteractionMessageCount = 1U;
  image.tsoDirectedInteractionMessages.push_back(std::move(directedInteraction));
  umbra::detail::FederationStateImageTsoObjectDeletionMessage objectDeletion;
  objectDeletion.messageId = 88U;
  objectDeletion.producingFederateId = 7U;
  objectDeletion.objectInstanceHandle = 19U;
  objectDeletion.userSuppliedTag = std::string{"delete\0", 7U};
  objectDeletion.recipients = {{8U, 19U}};
  objectDeletion.timestampEncoding = std::string{"\x0e\x0f", 2U};
  objectDeletion.sentOrderType = 2U;
  umbra::detail::FederationStateImageTsoObjectDeletionReconstitution reconstitution;
  reconstitution.object = {
      19U,
      L"table-19",
      4U,
      7U,
      false,
      0U,
      {{3U, 7U, "HLAreliable", 1U, {8U, 9U}}},
  };
  reconstitution.knownObjectClassHandlesByFederate = {{7U, 4U}, {8U, 4U}};
  objectDeletion.reconstitution = std::move(reconstitution);
  image.tsoObjectDeletionMessageCount = 1U;
  image.tsoObjectDeletionMessages.push_back(std::move(objectDeletion));
  umbra::detail::FederationStateImageTsoRequestRetractionRecord retraction;
  retraction.messageId = 55U;
  retraction.producingFederateId = 7U;
  retraction.timestampEncoding = std::string{"\x05\x06", 2U};
  retraction.recipientStates = {{8U, 1U}, {9U, 2U}};
  image.tsoRequestRetractionRecords.push_back(std::move(retraction));
  umbra::detail::FederationStateImageTsoRequestRetractionRecord terminalRetraction;
  terminalRetraction.messageId = 99U;
  terminalRetraction.producingFederateId = 7U;
  terminalRetraction.retractionApplied = true;
  terminalRetraction.terminal = true;
  terminalRetraction.producerResigned = true;
  terminalRetraction.recipientStates = {{8U, 3U}};
  image.tsoRequestRetractionRecords.push_back(std::move(terminalRetraction));
  image.tsoQueueEntries = {
      {55U, 8U, 3U, 0U, std::string{"\x05\x06", 2U}},
      {88U, 8U, 5U, 0U, std::string{"\x0e\x0f", 2U}},
      {77U, 8U, 4U, 1U, std::string{"\x08\x09", 2U}},
  };

  auto const encoded = umbra::detail::FederationStateImageCodec::encode(image);
  REQUIRE(encoded.starts_with("umbra-federation-state/v1\n"));
  auto const decoded = umbra::detail::FederationStateImageCodec::decode(encoded);
  REQUIRE(decoded.federationName == image.federationName);
  REQUIRE(decoded.logicalTimeImplementationName == image.logicalTimeImplementationName);
  REQUIRE(decoded.normalizationSeed == image.normalizationSeed);
  REQUIRE(decoded.members.size() == 1U);
  REQUIRE(decoded.members.front().name == L"alice / Δ");
  REQUIRE(decoded.members.front().momReportPeriodSeconds == -3);
  REQUIRE(decoded.members.front().successfulUpdateAttributeValuesCount == 3U);
  REQUIRE(decoded.members.front().successfulUpdateCountsByClassAndTransportation.size() == 1U);
  REQUIRE(decoded.members.front().successfulUpdateCountsByClassAndTransportation.front()
              .objectClassHandle == 4U);
  REQUIRE(decoded.members.front().successfulUpdateCountsByClassAndTransportation.front()
              .transportationName == "HLAreliable");
  REQUIRE(decoded.members.front().successfullyUpdatedObjectInstanceHandles ==
      std::vector<std::uint64_t>{19U, 20U});
  REQUIRE(decoded.members.front().successfullyUpdatedObjectInstanceClassHandles.size() == 2U);
  REQUIRE(decoded.members.front().successfullyUpdatedObjectInstanceClassHandles.back()
              .objectClassHandle == 4U);
  REQUIRE(decoded.members.front().successfulReflectionsReceivedCount == 9U);
  REQUIRE(decoded.members.front().successfulObjectInstanceRegistrationsCount == 10U);
  REQUIRE(decoded.members.front().successfulObjectInstanceDeletionsCount == 11U);
  REQUIRE(decoded.members.front().successfulObjectInstanceRemovalsCount == 12U);
  REQUIRE(decoded.members.front().successfulObjectInstanceDiscoveriesCount == 13U);
  REQUIRE(decoded.members.front().successfulReflectionCountsByClassAndTransportation.size() ==
      1U);
  REQUIRE(decoded.members.front().successfulReflectionCountsByClassAndTransportation.front()
              .objectClassHandle == 4U);
  REQUIRE(decoded.members.front().successfulReflectionCountsByClassAndTransportation.front()
              .transportationName == "HLAreliable");
  REQUIRE(decoded.members.front().successfullyReflectedObjectInstanceHandles ==
      std::vector<std::uint64_t>{19U, 20U});
  REQUIRE(decoded.members.front().successfullyReflectedObjectInstanceClassHandles.size() == 2U);
  REQUIRE(decoded.members.front().successfullyReflectedObjectInstanceClassHandles.back()
              .objectClassHandle == 4U);
  REQUIRE(decoded.members.front().successfulInteractionsSentCount == 6U);
  REQUIRE(decoded.members.front().successfulDirectedInteractionsSentCount == 2U);
  REQUIRE(decoded.members.front().successfulInteractionCountsByClassAndTransportation.size() ==
      2U);
  REQUIRE(decoded.members.front().successfulInteractionCountsByClassAndTransportation.front()
              .interactionClassHandle == 22U);
  REQUIRE(decoded.members.front().successfulDirectedInteractionCountsByClassAndTransportation
              .size() == 1U);
  REQUIRE(decoded.members.front()
              .successfulDirectedInteractionCountsByClassAndTransportation.front()
              .count == 2U);
  REQUIRE(decoded.members.front().successfulInteractionsReceivedCount == 8U);
  REQUIRE(decoded.members.front().successfulDirectedInteractionsReceivedCount == 3U);
  REQUIRE(decoded.members.front().successfulInteractionReceiptCountsByClassAndTransportation
              .size() == 2U);
  REQUIRE(decoded.members.front().successfulInteractionReceiptCountsByClassAndTransportation
              .front().interactionClassHandle == 22U);
  REQUIRE(decoded.members.front().successfulDirectedInteractionReceiptCountsByClassAndTransportation
              .size() == 1U);
  REQUIRE(decoded.members.front().successfulDirectedInteractionReceiptCountsByClassAndTransportation
              .front().count == 3U);
  REQUIRE(decoded.reservedObjectInstanceNames.size() == 1U);
  REQUIRE(decoded.reservedObjectInstanceNames.front().federateId == 7U);
  REQUIRE(decoded.reservedObjectInstanceNames.front().objectInstanceName ==
      L"reserved-table");
  REQUIRE(decoded.synchronizationPoints.size() == 1U);
  REQUIRE(decoded.synchronizationPoints.front().label == L"restore-sync");
  REQUIRE(decoded.synchronizationPoints.front().userSuppliedTag ==
      std::string{"sync\0", 5U});
  REQUIRE(decoded.synchronizationPoints.front().synchronizationSet ==
      std::vector<std::uint64_t>{7U});
  REQUIRE(decoded.synchronizationPoints.front().announcedFederates ==
      std::vector<std::uint64_t>{7U});
  REQUIRE(decoded.synchronizationPoints.front().achievedFederates ==
      std::vector<std::pair<std::uint64_t, bool>>{{7U, false}});
  REQUIRE(decoded.regions.size() == 1U);
  REQUIRE(decoded.regions.front().handle == 8U);
  REQUIRE(decoded.regions.front().ownerFederateId == 7U);
  REQUIRE(decoded.regions.front().dimensionHandles ==
      std::vector<std::uint64_t>{8U});
  REQUIRE(decoded.regions.front().pendingRangeBounds.front().lowerBound == 1U);
  REQUIRE(decoded.regions.front().committedRangeBounds.front().upperBound == 20U);
  REQUIRE(decoded.regions.front().specificationCommitted);
  REQUIRE(decoded.regions.front().inUse);
  REQUIRE(decoded.objectClassAttributeDeclarations.size() == 1U);
  REQUIRE(decoded.objectClassAttributeDeclarations.front().federateId == 7U);
  REQUIRE(decoded.objectClassAttributeDeclarations.front().subscriptionGeneration == 17U);
  REQUIRE(decoded.objectClassAttributeDeclarations.front().classes.size() == 1U);
  auto const& decodedObjectClass =
      decoded.objectClassAttributeDeclarations.front().classes.front();
  REQUIRE(decodedObjectClass.objectClassHandle == 4U);
  REQUIRE(decodedObjectClass.privilegeToDeleteExplicitlyUnpublished);
  REQUIRE(decodedObjectClass.explicitlyPublishedAttributeHandles ==
      std::vector<std::uint64_t>{3U});
  REQUIRE(decodedObjectClass.subscribedAttributes.size() == 1U);
  REQUIRE(decodedObjectClass.subscribedAttributes.front().active);
  REQUIRE(decodedObjectClass.subscribedUpdateRateDesignators.size() == 1U);
  REQUIRE(decodedObjectClass.subscribedUpdateRateDesignators.front().value.empty());
  REQUIRE(decodedObjectClass.regionalSubscribedAttributes.size() == 1U);
  REQUIRE(decodedObjectClass.regionalSubscribedAttributes.front().regionHandle == 8U);
  REQUIRE(decodedObjectClass.regionalSubscribedUpdateRateDesignators.size() == 1U);
  REQUIRE(decodedObjectClass.regionalSubscribedUpdateRateDesignators.front().value ==
      "HLAreliable");
  REQUIRE(decodedObjectClass.defaultTransportationTypes.size() == 1U);
  REQUIRE(decodedObjectClass.defaultTransportationTypes.front().value == "HLAreliable");
  REQUIRE(decodedObjectClass.defaultOrderTypes.size() == 1U);
  REQUIRE(decodedObjectClass.defaultOrderTypes.front().orderType == 1U);
  REQUIRE(decoded.timeStates.size() == 1U);
  REQUIRE(decoded.timeStates.front().currentTimeEncoding ==
      std::optional<std::string>{std::string{"\0\x01\x02", 3U}});
  REQUIRE(decoded.timeStates.front().pendingTimeRegulationGeneration == 12U);
  REQUIRE(decoded.timeStates.front().pendingTimeConstrainedGeneration == 13U);
  REQUIRE(decoded.timeStates.front().nextGeneration == 14U);
  REQUIRE(decoded.timeStates.front().pendingModifiedLookaheadEncoding ==
      std::optional<std::string>{std::string{"\x10\x11", 2U}});
  REQUIRE(decoded.timeStates.front().applicationRequestLedgerPresent);
  REQUIRE(decoded.timeStates.front().queuedTsoCount == 2U);
  REQUIRE(decoded.objects.size() == 1U);
  REQUIRE(decoded.objects.front().name == L"table-19");
  REQUIRE(decoded.objects.front().attributes.size() == 1U);
  REQUIRE(decoded.objects.front().attributes.front().ownerFederateId == 7U);
  REQUIRE(decoded.objects.front().attributes.front().updateRegionHandles ==
      std::vector<std::uint64_t>{8U, 9U});
  REQUIRE(decoded.objects.front().attributeValuesPresent);
  REQUIRE(decoded.objects.front().attributeValues.size() == 1U);
  REQUIRE(decoded.objects.front().attributeValues.front().attributeHandle == 3U);
  REQUIRE(decoded.objects.front().attributeValues.front().value ==
      std::string{"\0\xfe", 2U});
  REQUIRE(decoded.objects.front()
              .pendingAttributeOwnershipAcquisitionIfAvailableRequests.size() == 1U);
  REQUIRE(decoded.objects.front()
              .pendingAttributeOwnershipAcquisitionIfAvailableRequests.front()
              .requestId == 41U);
  REQUIRE(decoded.objects.front()
              .pendingAttributeOwnershipAcquisitionIfAvailableRequests.front()
              .requestSequence == 12U);
  REQUIRE(decoded.objects.front()
              .pendingAttributeOwnershipAcquisitionIfAvailableRequests.front()
              .desiredAttributeHandles == std::vector<std::uint64_t>{3U});
  REQUIRE(decoded.objects.front()
              .pendingAttributeOwnershipAcquisitionIfAvailableRequests.front()
              .userSuppliedTag == std::string{"if-available\0", 13U});
  REQUIRE(decoded.objects.front().pendingAttributeOwnershipAcquisitionRequests.size() == 1U);
  REQUIRE(decoded.objects.front().pendingAttributeOwnershipAcquisitionRequests.front()
              .requestId == 42U);
  REQUIRE(decoded.objects.front().pendingAttributeOwnershipAcquisitionRequests.front()
              .notificationQueuedAttributeHandles == std::vector<std::uint64_t>{3U});
  REQUIRE(decoded.objects.front().pendingAttributeOwnershipAcquisitionRequests.front()
              .releaseCallbacksQueuedByOwningFederate ==
          std::vector<std::pair<std::uint64_t, std::vector<std::uint64_t>>>{{8U, {3U}}});
  REQUIRE(decoded.objects.front().pendingAttributeOwnershipAcquisitionRequests.front()
              .userSuppliedTag == std::string{"regular\0", 8U});
  REQUIRE(decoded.objects.front().pendingAttributeOwnershipAcquisitionCancellations.size() == 1U);
  REQUIRE(decoded.objects.front().pendingAttributeOwnershipAcquisitionCancellations.front()
              .cancellationId == 43U);
  REQUIRE(decoded.objects.front().pendingAttributeOwnershipAcquisitionCancellations.front()
              .requestingFederateId == 7U);
  REQUIRE(decoded.objects.front().pendingAttributeOwnershipAcquisitionCancellations.front()
              .attributeHandles == std::vector<std::uint64_t>{3U});
  REQUIRE(decoded.objects.front().pendingAttributeOwnershipDivestitureIfWantedNotifications.size() == 1U);
  REQUIRE(decoded.objects.front().pendingAttributeOwnershipDivestitureIfWantedNotifications.front()
              .notificationId == 44U);
  REQUIRE(decoded.objects.front().pendingAttributeOwnershipDivestitureIfWantedNotifications.front()
              .receivingFederateId == 8U);
  REQUIRE(decoded.objects.front().pendingAttributeOwnershipDivestitureIfWantedNotifications.front()
              .attributeHandles == std::vector<std::uint64_t>{3U});
  REQUIRE(decoded.objects.front().pendingAttributeOwnershipDivestitureIfWantedNotifications.front()
              .userSuppliedTag == std::string{"divestiture-if-wanted\0", 22U});
  REQUIRE(decoded.objects.front().pendingConfirmDivestitureNotifications.size() == 1U);
  REQUIRE(decoded.objects.front().pendingConfirmDivestitureNotifications.front()
              .notificationId == 45U);
  REQUIRE(decoded.objects.front().pendingConfirmDivestitureNotifications.front()
              .receivingFederateId == 8U);
  REQUIRE(decoded.objects.front().pendingConfirmDivestitureNotifications.front()
              .attributeHandles == std::vector<std::uint64_t>{3U});
  REQUIRE(decoded.objects.front().pendingConfirmDivestitureNotifications.front()
              .userSuppliedTag == std::string{"confirm-divestiture\0", 20U});
  REQUIRE(decoded.objects.front().pendingAttributeTransportationTypeChanges.size() == 1U);
  REQUIRE(decoded.objects.front().pendingAttributeTransportationTypeChanges.front()
              .requestId == 46U);
  REQUIRE(decoded.objects.front().pendingAttributeTransportationTypeChanges.front()
              .requestingFederateId == 7U);
  REQUIRE(decoded.objects.front().pendingAttributeTransportationTypeChanges.front()
              .attributeHandles == std::vector<std::uint64_t>{3U});
  REQUIRE(decoded.objects.front().pendingAttributeTransportationTypeChanges.front()
              .transportationName == "HLAbestEffort");
  REQUIRE(decoded.objects.front().pendingNegotiatedAttributeOwnershipDivestitures.size() == 1U);
  REQUIRE(decoded.objects.front().pendingNegotiatedAttributeOwnershipDivestitures.front()
              .attributeHandle == 3U);
  REQUIRE(decoded.objects.front().pendingNegotiatedAttributeOwnershipDivestitures.front()
              .divestingFederateId == 7U);
  REQUIRE(decoded.objects.front().pendingNegotiatedAttributeOwnershipDivestitures.front()
              .acquiringFederateId == 8U);
  REQUIRE(decoded.objects.front().pendingNegotiatedAttributeOwnershipDivestitures.front()
              .acquisitionRequestId == 42U);
  REQUIRE(decoded.objects.front().pendingNegotiatedAttributeOwnershipDivestitures.front()
              .confirmationQueued);
  REQUIRE_FALSE(decoded.objects.front().pendingNegotiatedAttributeOwnershipDivestitures.front()
                    .confirmationDelivered);
  REQUIRE(decoded.objects.front().pendingNegotiatedAttributeOwnershipDivestitures.front()
              .userSuppliedTag == std::string{"negotiated\0", 11U});
  REQUIRE(decoded.objects.front().ownershipAssumptionRecipientsByAttribute.size() == 1U);
  REQUIRE(decoded.objects.front().ownershipAssumptionRecipientsByAttribute.front()
              .attributeHandle == 3U);
  REQUIRE(decoded.objects.front().ownershipAssumptionRecipientsByAttribute.front()
              .recipientFederateIds == std::vector<std::uint64_t>{8U, 9U});
  REQUIRE(decoded.objects.front().ownershipAssumptionUserSuppliedTagsByAttribute.size() == 1U);
  REQUIRE(decoded.objects.front().ownershipAssumptionUserSuppliedTagsByAttribute.front()
              .attributeHandle == 3U);
  REQUIRE(decoded.objects.front().ownershipAssumptionUserSuppliedTagsByAttribute.front()
              .userSuppliedTag == std::string{"assume\0", 7U});
  REQUIRE(decoded.objects.front().knownObjectClassHandlesByFederate.size() == 1U);
  REQUIRE(decoded.objects.front().knownObjectClassHandlesByFederate.front().federateId == 7U);
  REQUIRE(decoded.objects.front().knownObjectClassHandlesByFederate.front().objectClassHandle == 4U);
  REQUIRE(decoded.objects.front().pendingDiscoveryFederateIds ==
      std::vector<std::uint64_t>{8U});
  REQUIRE(decoded.objects.front().pendingRemovalFederateIds ==
      std::vector<std::uint64_t>{9U});
  REQUIRE(decoded.objects.front().connectionLossAutomaticRemovalFederateIds ==
      std::vector<std::uint64_t>{9U});
  REQUIRE(decoded.objects.front().deferredConnectionLossTsoRemovalFederateIds ==
      std::vector<std::uint64_t>{9U});
  REQUIRE(decoded.objects.front().pendingTimestampedRemovalFederateIds ==
      std::vector<std::uint64_t>{8U});
  REQUIRE(decoded.objects.front().pendingTimestampedDeletionMessageId ==
      std::optional<std::uint64_t>{88U});
  REQUIRE(decoded.interactionDeclarations.size() == 1U);
  REQUIRE(decoded.interactionDeclarations.front().federateId == 7U);
  REQUIRE(decoded.interactionDeclarations.front().publishedInteractionClasses ==
      std::vector<std::uint64_t>{21U});
  REQUIRE(decoded.interactionDeclarations.front().subscribedInteractionClasses.front().active);
  REQUIRE(decoded.interactionDeclarations.front()
              .regionalSubscribedInteractionClasses.front()
              .regionHandle == 8U);
  REQUIRE(decoded.interactionDeclarations.front()
              .publishedObjectClassDirectedInteractions.front()
              .interactionClassHandle == 23U);
  REQUIRE_FALSE(decoded.interactionDeclarations.front()
                    .subscribedObjectClassDirectedInteractions.front()
                    .active);
  REQUIRE(decoded.interactionDeclarations.front()
              .interactionTransportationTypes.front()
              .value == "HLAreliable");
  REQUIRE(decoded.interactionDeclarations.front().interactionOrderTypes.front().orderType == 1U);
  REQUIRE(decoded.interactionDeclarations.front()
              .pendingInteractionTransportationTypeChanges.front()
              .value == "HLAbestEffort");
  REQUIRE(decoded.tsoInteractionMessages.size() == 1U);
  REQUIRE(decoded.tsoInteractionMessages.front().messageId == 55U);
  REQUIRE(decoded.tsoInteractionMessages.front().parameters.front().value ==
      std::string{"\0\x01", 2U});
  REQUIRE(decoded.tsoInteractionMessages.front().userSuppliedTag ==
      std::string{"tag\0", 4U});
  REQUIRE(decoded.tsoInteractionMessages.front().sentRegionSnapshots.front()
              .committedRangeBounds.front()
              .upperBound == 20U);
  REQUIRE(decoded.tsoInteractionMessages.front().timestampEncoding ==
      std::optional<std::string>{std::string{"\x05\x06", 2U}});
  REQUIRE(decoded.tsoInteractionMessages.front().sentParameterHandles ==
      std::vector<std::uint64_t>{31U});
  REQUIRE(decoded.tsoInteractionMessages.front().sentRegionHandles ==
      std::vector<std::uint64_t>{8U});
  REQUIRE(decoded.tsoAttributeUpdateMessages.size() == 1U);
  REQUIRE(decoded.tsoAttributeUpdateMessages.front().messageId == 66U);
  REQUIRE(decoded.tsoAttributeUpdateMessages.front().attributes.front().attributeHandle == 7U);
  REQUIRE(decoded.tsoAttributeUpdateMessages.front().attributes.front().value ==
      std::string{"\x0a\x0b", 2U});
  REQUIRE(decoded.tsoAttributeUpdateMessages.front().passelsByRecipient.front()
              .passels.front()
              .sentAttributeHandles == std::vector<std::uint64_t>{7U});
  REQUIRE(decoded.tsoDirectedInteractionMessages.size() == 1U);
  REQUIRE(decoded.tsoDirectedInteractionMessages.front().messageId == 77U);
  REQUIRE(decoded.tsoDirectedInteractionMessages.front().objectInstanceHandle == 19U);
  REQUIRE(decoded.tsoDirectedInteractionMessages.front().recipients.front()
              .receivingFederateId == 8U);
  REQUIRE(decoded.tsoDirectedInteractionMessages.front().recipients.front()
              .receivedParameterHandles == std::vector<std::uint64_t>{32U});
  REQUIRE(decoded.tsoObjectDeletionMessages.size() == 1U);
  REQUIRE((decoded.tsoObjectDeletionMessages.front().messageId == 88U &&
      decoded.tsoObjectDeletionMessages.front().sentOrderType == 2U));
  REQUIRE(decoded.tsoObjectDeletionMessages.front().userSuppliedTag ==
      std::string{"delete\0", 7U});
  REQUIRE(decoded.tsoObjectDeletionMessages.front().recipients.front()
              .objectInstanceHandle == 19U);
  REQUIRE(decoded.tsoObjectDeletionMessages.front().reconstitution.has_value());
  REQUIRE(decoded.tsoObjectDeletionMessages.front().reconstitution->object.name ==
      L"table-19");
  REQUIRE(decoded.tsoObjectDeletionMessages.front().reconstitution
              ->knownObjectClassHandlesByFederate.size() == 2U);
  REQUIRE(decoded.tsoRequestRetractionRecords.size() == 2U);
  REQUIRE(decoded.tsoRequestRetractionRecords.front().messageId == 55U);
  REQUIRE(decoded.tsoRequestRetractionRecords.front().recipientStates.size() == 2U);
  REQUIRE(decoded.tsoRequestRetractionRecords.front().recipientStates.front().state == 1U);
  REQUIRE(decoded.tsoRequestRetractionRecords.front().recipientStates.back().state == 2U);
  REQUIRE(decoded.tsoRequestRetractionRecords.back().messageId == 99U);
  REQUIRE(decoded.tsoRequestRetractionRecords.back().timestampEncoding == std::nullopt);
  REQUIRE(decoded.tsoRequestRetractionRecords.back().retractionApplied);
  REQUIRE(decoded.tsoRequestRetractionRecords.back().recipientStates.front().state == 3U);
  REQUIRE(decoded.tsoQueueEntries.size() == 3U);
  REQUIRE(decoded.tsoQueueEntries.front().phase == 0U);
  REQUIRE(decoded.tsoQueueEntries.back().phase == 1U);
  REQUIRE(decoded.tsoQueueEntries.back().sequence == 4U);
  REQUIRE(umbra::detail::FederationStateImageCodec::encode(decoded) == encoded);

  REQUIRE_THROWS(umbra::detail::FederationStateImageCodec::decode(encoded + "junk"));
  auto malformed = encoded;
  malformed.replace(0U, std::string_view{"umbra-federation-state/v1"}.size(),
      "umbra-federation-state/v0");
  REQUIRE_THROWS(umbra::detail::FederationStateImageCodec::decode(malformed));
  auto malformedLifecycle = image;
  malformedLifecycle.objects.front().pendingTimestampedDeletionMessageId.reset();
  REQUIRE_THROWS(umbra::detail::FederationStateImageCodec::encode(malformedLifecycle));
  malformedLifecycle = image;
  malformedLifecycle.objects.front().deferredConnectionLossTsoRemovalFederateIds = {7U};
  REQUIRE_THROWS(umbra::detail::FederationStateImageCodec::encode(malformedLifecycle));
  auto malformedReservation = image;
  malformedReservation.reservedObjectInstanceNames.push_back({7U, L"reserved-table"});
  REQUIRE_THROWS(umbra::detail::FederationStateImageCodec::encode(malformedReservation));
  auto malformedSynchronization = image;
  malformedSynchronization.synchronizationPoints.front().announcedFederates = {8U};
  REQUIRE_THROWS(
      umbra::detail::FederationStateImageCodec::encode(malformedSynchronization));
  auto malformedRegion = image;
  malformedRegion.regions.front().pendingRangeBounds.front().dimensionHandle = 9U;
  REQUIRE_THROWS(umbra::detail::FederationStateImageCodec::encode(malformedRegion));
  malformedRegion = image;
  malformedRegion.regions.front().committedRangeBounds.front().lowerBound = 20U;
  REQUIRE_THROWS(umbra::detail::FederationStateImageCodec::encode(malformedRegion));
  auto malformedDeclaration = image;
  malformedDeclaration.objectClassAttributeDeclarations.front().classes.front()
      .subscribedAttributes.push_back({3U, true});
  REQUIRE_THROWS(umbra::detail::FederationStateImageCodec::encode(malformedDeclaration));
  auto malformedDeferredLookahead = image;
  malformedDeferredLookahead.timeStates.front().flags &= ~(1U << 1U);
  REQUIRE_THROWS(
      umbra::detail::FederationStateImageCodec::encode(malformedDeferredLookahead));
}


}  // namespace
