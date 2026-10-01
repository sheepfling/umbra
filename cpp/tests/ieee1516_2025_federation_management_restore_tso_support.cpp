#include "ieee1516_2025_federation_management_fixture_support.hpp"

void runTimedMultiRecipientRegionalResignationAfterRestore(
    rti1516_2025::ResignAction resignationAction,
    bool useIfAvailableAcquisition,
    bool useNegotiatedDivestiture,
    bool useTwoCandidateNegotiatedContinuation,
    bool cancelRetainedNegotiatedConfirmation,
    bool useMixedRegularIfAvailableCandidateContinuation,
    bool cancelRetainedNegotiatedConfirmationBeforeDelivery,
    bool useRegularRetainedCandidate) {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador firstReceiverReports;
  ReportingFederateAmbassador secondReceiverReports;
  ReportingFederateAmbassador clockReports;
  auto publisher = makeRti();
  auto firstReceiver = makeRti();
  auto secondReceiver = makeRti();
  auto clock = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const valueBytes[] = {0x44, 0x45, 0x4C, 0x2D, 0x4D};
  unsigned char const tagBytes[] = {0x44, 0x45, 0x4C, 0x2D, 0x54};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  std::wstring const saveLabel = L"timed-multi-recipient-regional-resignation";

  auto const drain = [](RTIambassador& ambassador) {
    while (ambassador.evokeCallback(0.0)) {
    }
  };

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(firstReceiver->connect(firstReceiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(secondReceiver->connect(secondReceiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(clock->connect(clockReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timed-multi-regional-resignation-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(firstReceiver->joinFederationExecution(
      L"timed-multi-regional-resignation-first",
      L"subscriber",
      federationName));
  REQUIRE_NOTHROW(secondReceiver->joinFederationExecution(
      L"timed-multi-regional-resignation-second",
      L"subscriber",
      federationName));
  REQUIRE_NOTHROW(clock->joinFederationExecution(
      L"timed-multi-regional-resignation-clock",
      L"publisher",
      federationName));
  suppressDeclarationRelevanceAdvisories(*publisher);

  auto const soda = publisher->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
  auto const flavor = publisher->getAttributeHandle(soda, fixture_hla::fixture::flavor);
  auto const privilegeToDelete = publisher->getAttributeHandle(
      soda,
      standard_hla::mom::privilege_to_delete_object);
  auto const sodaFlavor = publisher->getDimensionHandle(fixture_hla::fixture::soda_flavor);
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(privilegeToDelete.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(soda, flavorOnly));
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(soda, flavorOnly, TIMESTAMP));
  if (useTwoCandidateNegotiatedContinuation) {
    // The independent clock must know the object before it can be retained as
    // the second ownership candidate.  Its ordinary subscription is isolated
    // from the two constrained regional delivery routes below.
    REQUIRE_NOTHROW(clock->subscribeObjectClassAttributes(soda, flavorOnly));
  }

  auto const publisherRegion = publisher->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion, sodaFlavor, RangeBounds(0UL, 2UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{publisherRegion}));
  AttributeHandleSetRegionHandleSetPairVector const publisherPair{{
      flavorOnly,
      RegionHandleSet{publisherRegion},
  }};

  auto prepareReceiver = [&](RTIambassador& receiver) {
    auto const region = receiver.createRegion(DimensionHandleSet{sodaFlavor});
    REQUIRE_NOTHROW(receiver.setRangeBounds(
        region, sodaFlavor, RangeBounds(1UL, 3UL)));
    REQUIRE_NOTHROW(receiver.commitRegionModifications(RegionHandleSet{region}));
    AttributeHandleSetRegionHandleSetPairVector const pair{{
        flavorOnly,
        RegionHandleSet{region},
    }};
    REQUIRE_NOTHROW(receiver.subscribeObjectClassAttributesWithRegions(soda, pair));
    REQUIRE_NOTHROW(receiver.setConveyRegionDesignatorSetsSwitch(true));
    return std::pair{region, pair};
  };
  auto const [firstReceiverRegion, firstReceiverPair] = prepareReceiver(*firstReceiver);
  auto const [secondReceiverRegion, secondReceiverPair] = prepareReceiver(*secondReceiver);

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
      soda,
      publisherPair));
  drain(*firstReceiver);
  drain(*secondReceiver);
  REQUIRE(firstReceiverReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(secondReceiverReports.objectDiscoveryReports.size() == 1U);

  REQUIRE_NOTHROW(firstReceiver->enableTimeConstrained());
  drain(*firstReceiver);
  REQUIRE_NOTHROW(secondReceiver->enableTimeConstrained());
  drain(*secondReceiver);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drain(*publisher);
  REQUIRE_NOTHROW(clock->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drain(*clock);

  AttributeHandleValueMap attributeValues;
  attributeValues.emplace(
      flavor,
      VariableLengthData(valueBytes, sizeof(valueBytes)));
  auto const retraction = publisher->updateAttributeValues(
      objectInstance,
      attributeValues,
      tag,
      rti1516_2025::HLAinteger64Time(8));
  REQUIRE(retraction.isValid());
  REQUIRE(firstReceiverReports.attributeReflectionReports.empty());
  REQUIRE(secondReceiverReports.attributeReflectionReports.empty());

  REQUIRE_NOTHROW(publisher->requestFederationSave(
      saveLabel,
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(firstReceiver->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(secondReceiver->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(clock->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  for (int pass = 0; pass != 3; ++pass) {
    drain(*publisher);
    drain(*firstReceiver);
    drain(*secondReceiver);
    drain(*clock);
  }
  REQUIRE(firstReceiverReports.initiateFederateSaveReports ==
      std::vector<std::wstring>{saveLabel});
  REQUIRE(secondReceiverReports.initiateFederateSaveReports ==
      std::vector<std::wstring>{saveLabel});
  REQUIRE(publisherReports.initiateFederateSaveReports ==
      std::vector<std::wstring>{saveLabel});
  REQUIRE(clockReports.initiateFederateSaveReports ==
      std::vector<std::wstring>{saveLabel});

  REQUIRE_NOTHROW(publisher->federateSaveBegun());
  REQUIRE_NOTHROW(firstReceiver->federateSaveBegun());
  REQUIRE_NOTHROW(secondReceiver->federateSaveBegun());
  REQUIRE_NOTHROW(clock->federateSaveBegun());
  REQUIRE_NOTHROW(publisher->federateSaveComplete());
  REQUIRE_NOTHROW(firstReceiver->federateSaveComplete());
  REQUIRE_NOTHROW(secondReceiver->federateSaveComplete());
  REQUIRE_NOTHROW(clock->federateSaveComplete());
  drain(*publisher);
  drain(*firstReceiver);
  drain(*secondReceiver);
  drain(*clock);
  REQUIRE(publisherReports.federationSavedReportCount == 1U);
  REQUIRE(firstReceiverReports.federationSavedReportCount == 1U);
  REQUIRE(secondReceiverReports.federationSavedReportCount == 1U);
  REQUIRE(clockReports.federationSavedReportCount == 1U);

  // The source mutation is deliberately outside the saved image. Restore must
  // recover the original source-region realization before the resignation action.
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion, sodaFlavor, RangeBounds(3UL, 4UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{publisherRegion}));
  drain(*firstReceiver);
  drain(*secondReceiver);

  REQUIRE_NOTHROW(publisher->requestFederationRestore(saveLabel));
  drain(*publisher);
  drain(*firstReceiver);
  drain(*secondReceiver);
  drain(*clock);
  REQUIRE_NOTHROW(publisher->federateRestoreComplete());
  REQUIRE_NOTHROW(firstReceiver->federateRestoreComplete());
  REQUIRE_NOTHROW(secondReceiver->federateRestoreComplete());
  REQUIRE_NOTHROW(clock->federateRestoreComplete());
  drain(*publisher);
  drain(*firstReceiver);
  drain(*secondReceiver);
  drain(*clock);
  REQUIRE(publisherReports.federationRestoredReportCount == 1U);
  REQUIRE(firstReceiverReports.federationRestoredReportCount == 1U);
  REQUIRE(secondReceiverReports.federationRestoredReportCount == 1U);
  REQUIRE(clockReports.federationRestoredReportCount == 1U);

  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion, sodaFlavor, RangeBounds(3UL, 4UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{publisherRegion}));
  drain(*firstReceiver);
  drain(*secondReceiver);

  bool const pendingCancellation =
      resignationAction == rti1516_2025::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS;
  bool const preservesObject = resignationAction == rti1516_2025::NO_ACTION;
  unsigned char const secondWillingTagBytes[] = {0x53, 0x45, 0x43, 0x2D, 0x54};
  auto verifyRetainedRegularReleaseRequest = [&] {
    if (useRegularRetainedCandidate) {
      REQUIRE(publisherReports.attributeOwnershipReleaseRequestReports.size() == 1U);
      auto const& release = publisherReports.attributeOwnershipReleaseRequestReports.front();
      REQUIRE(release.objectInstance == objectInstance);
      REQUIRE(release.attributes == flavorOnly);
      REQUIRE(variableLengthDataBytes(release.userSuppliedTag) ==
              std::vector<unsigned char>(
                  secondWillingTagBytes,
                  secondWillingTagBytes + sizeof(secondWillingTagBytes)));
    } else {
      REQUIRE(publisherReports.attributeOwnershipReleaseRequestReports.empty());
    }
  };
  if (pendingCancellation) {
    // Publish after restore so the first constrained recipient can create a
    // real pending acquisition without changing the saved TSO invocation.
    REQUIRE_NOTHROW(firstReceiver->publishObjectClassAttributes(soda, flavorOnly));
    unsigned char const acquisitionTagBytes[] = {0x43, 0x41, 0x4E, 0x2D, 0x54};
    VariableLengthData const acquisitionTag(
        acquisitionTagBytes,
        sizeof(acquisitionTagBytes));
    VariableLengthData const secondWillingTag(
        secondWillingTagBytes,
        sizeof(secondWillingTagBytes));
    bool const firstCandidateUsesIfAvailable =
        !useMixedRegularIfAvailableCandidateContinuation &&
        (useIfAvailableAcquisition || useNegotiatedDivestiture);
    if (firstCandidateUsesIfAvailable) {
      REQUIRE_NOTHROW(firstReceiver->attributeOwnershipAcquisitionIfAvailable(
          objectInstance,
          flavorOnly,
          acquisitionTag));
    } else {
      REQUIRE_NOTHROW(firstReceiver->attributeOwnershipAcquisition(
          objectInstance,
          flavorOnly,
          acquisitionTag));
    }
    REQUIRE(firstReceiverReports.attributeOwnershipAcquisitionReports.empty());
    if (useTwoCandidateNegotiatedContinuation) {
      // Keep a second WTA request alive on the independent clock federate.
      // The first request is selected by the owner's negotiated search and
      // then removed by directive three when that requester leaves.  The
      // mixed-form companion deliberately makes the first request regular
      // and the retained clock request If Available, proving that request
      // ordering is independent of the acquisition form.  Using the clock as
      // the retained candidate keeps the two regional recipients in their
      // subscriber-only delivery role for the common TSO assertion.
      drain(*clock);
      REQUIRE(clockReports.objectDiscoveryReports.size() == 1U);
      REQUIRE_NOTHROW(clock->publishObjectClassAttributes(soda, flavorOnly));
      if (useRegularRetainedCandidate) {
        REQUIRE_NOTHROW(clock->attributeOwnershipAcquisition(
            objectInstance,
            flavorOnly,
            secondWillingTag));
      } else {
        REQUIRE_NOTHROW(clock->attributeOwnershipAcquisitionIfAvailable(
            objectInstance,
            flavorOnly,
            secondWillingTag));
      }
      REQUIRE(clockReports.attributeOwnershipAcquisitionReports.empty());
    }
    if (useNegotiatedDivestiture) {
      unsigned char const negotiatedTagBytes[] = {0x4E, 0x45, 0x47, 0x2D, 0x54};
      VariableLengthData const negotiatedTag(
          negotiatedTagBytes,
          sizeof(negotiatedTagBytes));
      // The current owner queues one negotiated confirmation for the WTA
      // candidate, but keeps ownership until that callback is confirmed.
      REQUIRE_NOTHROW(publisher->negotiatedAttributeOwnershipDivestiture(
          objectInstance,
          flavorOnly,
          negotiatedTag));
      REQUIRE(publisher->isAttributeOwnedByFederate(objectInstance, flavor));
      REQUIRE_FALSE(firstReceiver->isAttributeOwnedByFederate(objectInstance, flavor));
      REQUIRE(publisherReports.divestitureConfirmationReports.empty());
    } else if (useIfAvailableAcquisition) {
      // If Available records a private Willing-to-Acquire reservation.  It
      // must not ask the current owner to release anything.
      REQUIRE(publisherReports.attributeOwnershipReleaseRequestReports.empty());
      REQUIRE(publisher->isAttributeOwnedByFederate(objectInstance, flavor));
      REQUIRE_FALSE(firstReceiver->isAttributeOwnedByFederate(objectInstance, flavor));
    } else {
      REQUIRE(publisherReports.attributeOwnershipReleaseRequestReports.empty());
    }
    REQUIRE_NOTHROW(firstReceiver->resignFederationExecution(resignationAction));
    // Any requester-side terminal callback (and, for regular acquisition, the
    // owner-release callback) was queued before the requester resigned;
    // cancellation must consume it as stale work rather than deliver it.
    drain(*firstReceiver);
    drain(*publisher);
    verifyRetainedRegularReleaseRequest();
    REQUIRE(firstReceiverReports.attributeOwnershipAcquisitionReports.empty());
    if (useTwoCandidateNegotiatedContinuation) {
      // Replanning the owner's negotiated request after the selected WTA
      // candidate resigns must select the retained candidate, preserving its
      // acquisition tag.  Keep the confirmation callback pending until the
      // common Flush Queue boundary below so the original publisher-owned TSO
      // passel remains deliverable to every surviving regional recipient.
      REQUIRE(publisherReports.divestitureConfirmationReports.empty());
      unsigned char const negotiatedTagBytes[] = {0x4E, 0x45, 0x47, 0x2D, 0x54};
      VariableLengthData const negotiatedTag(
          negotiatedTagBytes,
          sizeof(negotiatedTagBytes));
      REQUIRE_NOTHROW(publisher->negotiatedAttributeOwnershipDivestiture(
          objectInstance,
          flavorOnly,
          negotiatedTag));
      REQUIRE(publisher->isAttributeOwnedByFederate(objectInstance, flavor));
      if (cancelRetainedNegotiatedConfirmationBeforeDelivery) {
        // Cancel before the queued Request Divestiture Confirmation enters
        // user code.  The owner keeps ownership, the stale confirmation is
        // consumed without a callback, and the private retained WTA remains
        // unowned until resignation cleanup below.
        REQUIRE_NOTHROW(publisher->cancelNegotiatedAttributeOwnershipDivestiture(
            objectInstance,
            flavorOnly));
        REQUIRE(publisher->isAttributeOwnedByFederate(objectInstance, flavor));
        REQUIRE_FALSE(clock->isAttributeOwnedByFederate(objectInstance, flavor));
        REQUIRE(clockReports.attributeOwnershipAcquisitionReports.empty());
        REQUIRE_THROWS_AS(
            publisher->confirmDivestiture(
                objectInstance,
                flavorOnly,
                VariableLengthData{}),
            rti1516_2025::AttributeDivestitureWasNotRequested);
        drain(*publisher);
        REQUIRE(publisherReports.divestitureConfirmationReports.empty());
        verifyRetainedRegularReleaseRequest();
      } else {
        drain(*publisher);
        REQUIRE(publisherReports.divestitureConfirmationReports.size() == 1U);
        auto const& confirmation = publisherReports.divestitureConfirmationReports.front();
        REQUIRE(confirmation.objectInstance == objectInstance);
        REQUIRE(confirmation.attributes == flavorOnly);
        REQUIRE(variableLengthDataBytes(confirmation.userSuppliedTag) ==
                std::vector<unsigned char>(
                    secondWillingTagBytes,
                    secondWillingTagBytes + sizeof(secondWillingTagBytes)));
        REQUIRE(publisher->isAttributeOwnedByFederate(objectInstance, flavor));
        REQUIRE_FALSE(clock->isAttributeOwnedByFederate(objectInstance, flavor));
        REQUIRE(clockReports.attributeOwnershipAcquisitionReports.empty());
      }
    } else if (useNegotiatedDivestiture) {
      REQUIRE(publisherReports.divestitureConfirmationReports.empty());
    }
    REQUIRE(firstReceiverReports.attributeReflectionReports.empty());
  } else {
    if (preservesObject) {
      // NO_ACTION may only leave a federate with no owned attributes.  Give
      // the accepted passel an unowned interval without creating a new
      // acquisition candidate; the saved producer/payload metadata remains
      // authoritative for the queued delivery.
      unsigned char const divestitureTagBytes[] = {0x4E, 0x4F, 0x2D, 0x41};
      VariableLengthData const divestitureTag(
          divestitureTagBytes,
          sizeof(divestitureTagBytes));
      REQUIRE_NOTHROW(publisher->unconditionalAttributeOwnershipDivestiture(
          objectInstance,
          AttributeHandleSet{flavor, privilegeToDelete},
          divestitureTag));
      REQUIRE_FALSE(publisher->isAttributeOwnedByFederate(objectInstance, flavor));
      REQUIRE_FALSE(publisher->isAttributeOwnedByFederate(objectInstance, privilegeToDelete));
    }
    REQUIRE_NOTHROW(publisher->resignFederationExecution(resignationAction));
    drain(*firstReceiver);
    drain(*secondReceiver);
    REQUIRE(firstReceiverReports.attributeReflectionReports.empty());
    REQUIRE(secondReceiverReports.attributeReflectionReports.empty());
  }

  firstReceiverReports.callbackOrder.clear();
  secondReceiverReports.callbackOrder.clear();
  if (!pendingCancellation) {
    REQUIRE_NOTHROW(firstReceiver->flushQueueRequest(
        rti1516_2025::HLAinteger64Time(10)));
  }
  REQUIRE_NOTHROW(secondReceiver->flushQueueRequest(
      rti1516_2025::HLAinteger64Time(10)));
  if (pendingCancellation) {
    auto const publisherGrantCountBefore = publisherReports.timeAdvanceGrantReports.size();
    REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(9)));
    while (publisherReports.timeAdvanceGrantReports.size() == publisherGrantCountBefore &&
           publisher->evokeCallback(0.0)) {
    }
  }
  REQUIRE_NOTHROW(clock->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(9)));
  while (clockReports.timeAdvanceGrantReports.empty() &&
         clock->evokeCallback(0.0)) {
  }
  while (firstReceiverReports.flushQueueGrantReports.empty() &&
         firstReceiver->evokeCallback(0.0)) {
  }
  while (secondReceiverReports.flushQueueGrantReports.empty() &&
         secondReceiver->evokeCallback(0.0)) {
  }
  REQUIRE(clockReports.timeAdvanceGrantReports.size() == 1U);
  if (!pendingCancellation) {
    REQUIRE(firstReceiverReports.flushQueueGrantReports.size() == 1U);
  }
  REQUIRE(secondReceiverReports.flushQueueGrantReports.size() == 1U);
  if (pendingCancellation) {
    REQUIRE(firstReceiverReports.objectRemovalReports.empty());
    REQUIRE(firstReceiverReports.attributeReflectionReports.empty());
    REQUIRE(secondReceiverReports.objectRemovalReports.empty());
    REQUIRE(secondReceiverReports.attributeReflectionReports.size() == 1U);
    REQUIRE(secondReceiverReports.attributeReflectionReports.front().objectInstance == objectInstance);
    REQUIRE(secondReceiverReports.callbackOrder ==
            std::vector<std::string>{"reflect", "flush-grant"});
  } else if (preservesObject) {
    REQUIRE(firstReceiverReports.objectRemovalReports.empty());
    REQUIRE(secondReceiverReports.objectRemovalReports.empty());
    REQUIRE_NOTHROW(firstReceiver->getObjectInstanceName(objectInstance));
    REQUIRE_NOTHROW(secondReceiver->getObjectInstanceName(objectInstance));
    auto const verifyPreservedRegionalReflection =
        [&](ReportingFederateAmbassador const& reports) {
          REQUIRE(reports.attributeReflectionReports.size() == 1U);
          REQUIRE(reports.attributeReflectionReports.front().objectInstance == objectInstance);
          REQUIRE(reports.attributeReflectionReports.front().attributeValues.size() == 1U);
          REQUIRE(reports.attributeReflectionReports.front().attributeValues.contains(flavor));
          REQUIRE(variableLengthDataBytes(
                      reports.attributeReflectionReports.front().attributeValues.at(flavor)) ==
                  std::vector<unsigned char>(valueBytes, valueBytes + sizeof(valueBytes)));
          REQUIRE(variableLengthDataBytes(reports.attributeReflectionReports.front().userSuppliedTag) ==
                  std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
          REQUIRE(reports.attributeReflectionReports.front().producingFederate == publisherHandle);
          REQUIRE(reports.attributeReflectionReports.front().timeImplementationName ==
                  standard_hla::mom::integer64_time);
          REQUIRE(reports.attributeReflectionReports.front().timeValue == L"8");
          REQUIRE(reports.attributeReflectionReports.front().sentOrderType == TIMESTAMP);
          REQUIRE(reports.attributeReflectionReports.front().receivedOrderType == TIMESTAMP);
          REQUIRE(reports.attributeReflectionReports.front().sentRegionsSupplied);
          REQUIRE(reports.attributeReflectionReports.front().sentRegions ==
                  RegionHandleSet{publisherRegion});
          REQUIRE(reports.attributeReflectionReports.front().retractionSupplied);
          REQUIRE(reports.attributeReflectionReports.front().retractionValid);
          REQUIRE(reports.callbackOrder ==
                  std::vector<std::string>{"reflect", "flush-grant"});
        };
    verifyPreservedRegionalReflection(firstReceiverReports);
    verifyPreservedRegionalReflection(secondReceiverReports);
  } else {
    REQUIRE(firstReceiverReports.objectRemovalReports.size() == 1U);
    REQUIRE(secondReceiverReports.objectRemovalReports.size() == 1U);
    REQUIRE(firstReceiverReports.objectRemovalReports.front().objectInstance == objectInstance);
    REQUIRE(secondReceiverReports.objectRemovalReports.front().objectInstance == objectInstance);
    REQUIRE_THROWS_AS(
        firstReceiver->getObjectInstanceName(objectInstance),
        rti1516_2025::ObjectInstanceNotKnown);
    REQUIRE_THROWS_AS(
        secondReceiver->getObjectInstanceName(objectInstance),
        rti1516_2025::ObjectInstanceNotKnown);
    REQUIRE(firstReceiverReports.attributeReflectionReports.empty());
    REQUIRE(secondReceiverReports.attributeReflectionReports.empty());
    REQUIRE(firstReceiverReports.callbackOrder == std::vector<std::string>{"flush-grant"});
    REQUIRE(secondReceiverReports.callbackOrder == std::vector<std::string>{"flush-grant"});
  }

  if (pendingCancellation && useTwoCandidateNegotiatedContinuation) {
    // The saved TSO passel has now reached both regional recipients.  Only
    // then either complete or cancel the retained-candidate handoff, proving
    // that the ownership decision does not alter the delivery snapshot.
    if (cancelRetainedNegotiatedConfirmation ||
        cancelRetainedNegotiatedConfirmationBeforeDelivery) {
      if (cancelRetainedNegotiatedConfirmationBeforeDelivery) {
        // The negotiated request was cancelled before its confirmation
        // callback was delivered above.  No second cancellation or callback
        // is possible at this boundary.
        REQUIRE(publisherReports.divestitureConfirmationReports.empty());
        REQUIRE(publisher->isAttributeOwnedByFederate(objectInstance, flavor));
        REQUIRE_FALSE(clock->isAttributeOwnedByFederate(objectInstance, flavor));
        REQUIRE(clockReports.attributeOwnershipAcquisitionReports.empty());
        verifyRetainedRegularReleaseRequest();
      } else {
      REQUIRE_NOTHROW(publisher->cancelNegotiatedAttributeOwnershipDivestiture(
          objectInstance,
          flavorOnly));
      REQUIRE(publisher->isAttributeOwnedByFederate(objectInstance, flavor));
      REQUIRE_FALSE(clock->isAttributeOwnedByFederate(objectInstance, flavor));
      REQUIRE(clockReports.attributeOwnershipAcquisitionReports.empty());
      REQUIRE_THROWS_AS(
          publisher->confirmDivestiture(
              objectInstance,
              flavorOnly,
              VariableLengthData{}),
          rti1516_2025::AttributeDivestitureWasNotRequested);
      drain(*publisher);
      REQUIRE(publisherReports.divestitureConfirmationReports.size() == 1U);
      // Cancelling the negotiated request removes only the owner's waiting
      // state.  A retained If Available reservation remains private to the
      // candidate, while a retained regular request keeps its already queued
      // owner-release callback.  The resignation cleanup below clears either
      // pending candidate without changing the saved delivery snapshot.
      verifyRetainedRegularReleaseRequest();
      }
    } else {
      // Complete the retained-candidate handoff only after the common Flush
      // Queue boundary so both regional recipients receive the publisher-owned
      // saved reflection before ownership changes.
      unsigned char const confirmationTagBytes[] = {0x43, 0x4F, 0x4E, 0x2D, 0x54};
      VariableLengthData const confirmationTag(
          confirmationTagBytes,
          sizeof(confirmationTagBytes));
      REQUIRE_NOTHROW(publisher->confirmDivestiture(
          objectInstance,
          flavorOnly,
          confirmationTag));
      REQUIRE_FALSE(publisher->isAttributeOwnedByFederate(objectInstance, flavor));
      REQUIRE(clock->isAttributeOwnedByFederate(objectInstance, flavor));
      REQUIRE(clockReports.attributeOwnershipAcquisitionReports.empty());
      drain(*clock);
      REQUIRE(clockReports.attributeOwnershipAcquisitionReports.size() == 1U);
      auto const& acquisitionNotification =
          clockReports.attributeOwnershipAcquisitionReports.front();
      REQUIRE(acquisitionNotification.kind ==
              ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification);
      REQUIRE(acquisitionNotification.objectInstance == objectInstance);
      REQUIRE(acquisitionNotification.attributes == flavorOnly);
      REQUIRE(variableLengthDataBytes(acquisitionNotification.userSuppliedTag) ==
              std::vector<unsigned char>(
                  confirmationTagBytes,
                  confirmationTagBytes + sizeof(confirmationTagBytes)));
    }
  }

  if (!pendingCancellation) {
    REQUIRE_NOTHROW(firstReceiver->unsubscribeObjectClassAttributesWithRegions(
        soda,
        firstReceiverPair));
  }
  REQUIRE_NOTHROW(secondReceiver->unsubscribeObjectClassAttributesWithRegions(
      soda,
      secondReceiverPair));
  if (!pendingCancellation) {
    REQUIRE_NOTHROW(firstReceiver->deleteRegion(firstReceiverRegion));
  }
  REQUIRE_NOTHROW(secondReceiver->deleteRegion(secondReceiverRegion));
  if (!pendingCancellation) {
    REQUIRE_NOTHROW(firstReceiver->resignFederationExecution(NO_ACTION));
  }
  REQUIRE_NOTHROW(secondReceiver->resignFederationExecution(NO_ACTION));
  if (pendingCancellation) {
    REQUIRE_NOTHROW(publisher->resignFederationExecution(
        rti1516_2025::DELETE_OBJECTS_THEN_DIVEST));
  }
  REQUIRE_NOTHROW(clock->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(clock->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(firstReceiver->disconnect());
  REQUIRE_NOTHROW(secondReceiver->disconnect());
  REQUIRE_NOTHROW(clock->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
