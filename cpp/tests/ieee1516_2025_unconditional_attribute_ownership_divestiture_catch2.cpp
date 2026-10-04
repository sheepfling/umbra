#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded Unconditional Attribute Ownership Divestiture offers eligible 2025 federates",
    "[integration][development-profile][ownership-management]"
    "[rti.service.unconditional-attribute-ownership-divestiture]"
    "[rti.service.attribute-ownership-acquisition]"
    "[rti.service.attribute-ownership-acquisition-if-available]"
    "[federate.callback.request-attribute-ownership-assumption]"
    "[federate.callback.attribute-ownership-acquisition-notification]"
    "[ownership-assumption-research]") {
  TestFederateAmbassador unjoinedFederate;
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador regularRequesterReports;
  ReportingFederateAmbassador ifAvailableRequesterReports;
  ReportingFederateAmbassador invitedCandidateReports;
  ReportingFederateAmbassador staleCandidateReports;
  ReportingFederateAmbassador unpublishedCandidateReports;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto regularRequester = makeRti();
  auto ifAvailableRequester = makeRti();
  auto invitedCandidate = makeRti();
  auto staleCandidate = makeRti();
  auto unpublishedCandidate = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const regularAcquisitionTagBytes[] = {0xA2, 0x07, 0x20};
  unsigned char const pendingIfAvailableTagBytes[] = {0xB2, 0x07, 0x20};
  unsigned char const assumptionTagBytes[] = {0xD2, 0x07, 0x20};
  unsigned char const ifAvailableAcquisitionTagBytes[] = {0xC2, 0x07, 0x20};
  VariableLengthData const regularAcquisitionTag(
      regularAcquisitionTagBytes,
      sizeof(regularAcquisitionTagBytes));
  VariableLengthData const pendingIfAvailableTag(
      pendingIfAvailableTagBytes,
      sizeof(pendingIfAvailableTagBytes));
  VariableLengthData const assumptionTag(assumptionTagBytes, sizeof(assumptionTagBytes));
  VariableLengthData const ifAvailableAcquisitionTag(
      ifAvailableAcquisitionTagBytes,
      sizeof(ifAvailableAcquisitionTagBytes));
  ObjectInstanceHandle invalidObjectInstance;
  AttributeHandle invalidAttribute;
  AttributeHandleSet noAttributes;

  REQUIRE_THROWS_AS(
      unjoined->unconditionalAttributeOwnershipDivestiture(
          invalidObjectInstance,
          noAttributes,
          assumptionTag),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->unconditionalAttributeOwnershipDivestiture(
          invalidObjectInstance,
          noAttributes,
          assumptionTag),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regularRequester->connect(regularRequesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(ifAvailableRequester->connect(ifAvailableRequesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(invitedCandidate->connect(invitedCandidateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(staleCandidate->connect(staleCandidateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(unpublishedCandidate->connect(unpublishedCandidateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      owner->joinFederationExecution(L"unconditional-divestiture-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(regularRequester->joinFederationExecution(
      L"unconditional-divestiture-regular-requester",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(ifAvailableRequester->joinFederationExecution(
      L"unconditional-divestiture-if-available-requester",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(invitedCandidate->joinFederationExecution(
      L"unconditional-divestiture-invited-candidate",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(staleCandidate->joinFederationExecution(
      L"unconditional-divestiture-stale-candidate",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(unpublishedCandidate->joinFederationExecution(
      L"unconditional-divestiture-unpublished-candidate",
      L"publisher",
      federationName));

  auto const child = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliableBaseA = owner->getAttributeHandle(child, L"ReliableBaseA");
  auto const reliableBaseB = owner->getAttributeHandle(child, L"ReliableBaseB");
  auto const reliableChild = owner->getAttributeHandle(child, L"ReliableChild");
  auto const unownedChild = owner->getAttributeHandle(child, L"UnownedChild");
  REQUIRE(child.isValid());
  REQUIRE(reliableBaseA.isValid());
  REQUIRE(reliableBaseB.isValid());
  REQUIRE(reliableChild.isValid());
  REQUIRE(unownedChild.isValid());
  AttributeHandleSet const regularAttributes{reliableBaseA};
  AttributeHandleSet const ifAvailableAttributes{reliableBaseB};
  AttributeHandleSet const candidateAttributes{reliableChild, unownedChild};
  AttributeHandleSet const ownedAttributes{
      reliableBaseA,
      reliableBaseB,
      reliableChild,
      unownedChild,
  };

  // Discovery gives every eventual candidate a known class. Only the two
  // selected candidates publish the offered attributes; a known but
  // unpublished federate must not receive the §7.4 callback.
  REQUIRE_NOTHROW(regularRequester->subscribeObjectClassAttributes(child, regularAttributes));
  REQUIRE_NOTHROW(
      ifAvailableRequester->subscribeObjectClassAttributes(child, ifAvailableAttributes));
  REQUIRE_NOTHROW(invitedCandidate->subscribeObjectClassAttributes(child, candidateAttributes));
  REQUIRE_NOTHROW(staleCandidate->subscribeObjectClassAttributes(child, candidateAttributes));
  REQUIRE_NOTHROW(
      unpublishedCandidate->subscribeObjectClassAttributes(child, candidateAttributes));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, ownedAttributes));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(regularRequester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(ifAvailableRequester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(invitedCandidate->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(staleCandidate->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(unpublishedCandidate->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(regularRequesterReports.objectDiscoveryReports.size() == 1);
  REQUIRE(ifAvailableRequesterReports.objectDiscoveryReports.size() == 1);
  REQUIRE(invitedCandidateReports.objectDiscoveryReports.size() == 1);
  REQUIRE(staleCandidateReports.objectDiscoveryReports.size() == 1);
  REQUIRE(unpublishedCandidateReports.objectDiscoveryReports.size() == 1);
  REQUIRE_NOTHROW(regularRequester->publishObjectClassAttributes(child, regularAttributes));
  REQUIRE_NOTHROW(
      ifAvailableRequester->publishObjectClassAttributes(child, ifAvailableAttributes));
  REQUIRE_NOTHROW(invitedCandidate->publishObjectClassAttributes(child, candidateAttributes));
  REQUIRE_NOTHROW(staleCandidate->publishObjectClassAttributes(child, candidateAttributes));

  REQUIRE_THROWS_AS(
      owner->unconditionalAttributeOwnershipDivestiture(
          invalidObjectInstance,
          ownedAttributes,
          assumptionTag),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      owner->unconditionalAttributeOwnershipDivestiture(
          objectInstance,
          AttributeHandleSet{invalidAttribute},
          assumptionTag),
      rti1516_2025::AttributeNotDefined);
  REQUIRE_THROWS_AS(
      owner->unconditionalAttributeOwnershipDivestiture(
          objectInstance,
          AttributeHandleSet{reliableBaseA, invalidAttribute},
          assumptionTag),
      rti1516_2025::AttributeNotDefined);
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_THROWS_AS(
      regularRequester->unconditionalAttributeOwnershipDivestiture(
          objectInstance,
          regularAttributes,
          assumptionTag),
      rti1516_2025::AttributeNotOwned);

  // A regular acquirer is not offered the same attribute. Its existing
  // Acquisition Pending request instead becomes regular notification work
  // when unconditional divestiture makes the attribute unowned.
  REQUIRE_NOTHROW(regularRequester->attributeOwnershipAcquisition(
      objectInstance,
      regularAttributes,
      regularAcquisitionTag));
  REQUIRE_NOTHROW(ifAvailableRequester->attributeOwnershipAcquisitionIfAvailable(
      objectInstance,
      ifAvailableAttributes,
      pendingIfAvailableTag));
  REQUIRE_NOTHROW(owner->unconditionalAttributeOwnershipDivestiture(
      objectInstance,
      ownedAttributes,
      assumptionTag));
  REQUIRE_FALSE(owner->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_FALSE(owner->isAttributeOwnedByFederate(objectInstance, reliableBaseB));
  REQUIRE_FALSE(owner->isAttributeOwnedByFederate(objectInstance, reliableChild));
  REQUIRE_FALSE(owner->isAttributeOwnedByFederate(objectInstance, unownedChild));
  REQUIRE_FALSE(regularRequester->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_FALSE(ifAvailableRequester->isAttributeOwnedByFederate(objectInstance, reliableBaseB));
  REQUIRE_FALSE(invitedCandidate->isAttributeOwnedByFederate(objectInstance, reliableChild));

  // The old owner-side release work is stale.  Draining that internal work
  // must not make the divesting federate observe any callback, and a
  // candidate that stops publishing before its queued callback also receives
  // no stale offer.
  REQUIRE_NOTHROW(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.callbackOrder.empty());
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.empty());
  REQUIRE(ownerReports.attributeOwnershipAssumptionReports.empty());
  REQUIRE_NOTHROW(staleCandidate->unpublishObjectClassAttributes(child, candidateAttributes));
  REQUIRE_FALSE(staleCandidate->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(staleCandidateReports.attributeOwnershipAssumptionReports.empty());

  // A stale callback is terminal for that queued work, but not for the
  // continuing unowned search. Re-publishing the candidate must make it
  // eligible for exactly one fresh assumption offer.
  REQUIRE_NOTHROW(staleCandidate->publishObjectClassAttributes(child, candidateAttributes));
  REQUIRE_FALSE(staleCandidate->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(staleCandidateReports.attributeOwnershipAssumptionReports.size() == 1);
  auto const& reofferedAssumption =
      staleCandidateReports.attributeOwnershipAssumptionReports.front();
  REQUIRE(reofferedAssumption.objectInstance == objectInstance);
  REQUIRE(reofferedAssumption.attributes == candidateAttributes);
  REQUIRE(variableLengthDataBytes(reofferedAssumption.userSuppliedTag) ==
          std::vector<unsigned char>(
              assumptionTagBytes,
              assumptionTagBytes + sizeof(assumptionTagBytes)));

  REQUIRE_FALSE(unpublishedCandidate->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(unpublishedCandidateReports.attributeOwnershipAssumptionReports.empty());

  // A pre-existing If Available request keeps its own terminal callback and
  // is not duplicated as an assumption offer. It establishes ownership only
  // when that original callback begins after the unconditional transition.
  REQUIRE_FALSE(ifAvailableRequester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ifAvailableRequesterReports.attributeOwnershipAssumptionReports.empty());
  REQUIRE(ifAvailableRequesterReports.attributeOwnershipAcquisitionReports.size() == 1);
  auto const& ifAvailableNotification =
      ifAvailableRequesterReports.attributeOwnershipAcquisitionReports.front();
  REQUIRE(ifAvailableNotification.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification);
  REQUIRE(ifAvailableNotification.objectInstance == objectInstance);
  REQUIRE(ifAvailableNotification.attributes == ifAvailableAttributes);
  REQUIRE(variableLengthDataBytes(ifAvailableNotification.userSuppliedTag) ==
          std::vector<unsigned char>(
              pendingIfAvailableTagBytes,
              pendingIfAvailableTagBytes + sizeof(pendingIfAvailableTagBytes)));
  REQUIRE(ifAvailableRequester->isAttributeOwnedByFederate(objectInstance, reliableBaseB));
  REQUIRE_NOTHROW(
      ifAvailableRequester->unpublishObjectClassAttributes(child, ifAvailableAttributes));

  REQUIRE_FALSE(regularRequester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(regularRequesterReports.attributeOwnershipAcquisitionReports.size() == 1);
  auto const& regularNotification =
      regularRequesterReports.attributeOwnershipAcquisitionReports.front();
  REQUIRE(regularNotification.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification);
  REQUIRE(regularNotification.objectInstance == objectInstance);
  REQUIRE(regularNotification.attributes == regularAttributes);
  REQUIRE(variableLengthDataBytes(regularNotification.userSuppliedTag) ==
          std::vector<unsigned char>(
              regularAcquisitionTagBytes,
              regularAcquisitionTagBytes + sizeof(regularAcquisitionTagBytes)));
  REQUIRE(regularRequester->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_NOTHROW(
      regularRequester->unpublishObjectClassAttributes(child, regularAttributes));

  // The still-eligible, non-pending candidate receives a single grouped offer
  // with the unconditional-divestiture tag. The offer changes no ownership;
  // only its later acquisition request can do so.
  REQUIRE_FALSE(invitedCandidate->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(invitedCandidateReports.attributeOwnershipAssumptionReports.size() == 1);
  auto const& assumption = invitedCandidateReports.attributeOwnershipAssumptionReports.front();
  REQUIRE(assumption.objectInstance == objectInstance);
  REQUIRE(assumption.attributes == candidateAttributes);
  REQUIRE(variableLengthDataBytes(assumption.userSuppliedTag) ==
          std::vector<unsigned char>(
              assumptionTagBytes,
              assumptionTagBytes + sizeof(assumptionTagBytes)));
  REQUIRE_FALSE(invitedCandidate->isAttributeOwnedByFederate(objectInstance, reliableChild));
  REQUIRE_FALSE(invitedCandidate->isAttributeOwnedByFederate(objectInstance, unownedChild));

  REQUIRE_NOTHROW(invitedCandidate->attributeOwnershipAcquisitionIfAvailable(
      objectInstance,
      candidateAttributes,
      ifAvailableAcquisitionTag));
  REQUIRE_FALSE(invitedCandidate->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(invitedCandidateReports.attributeOwnershipAcquisitionReports.size() == 1);
  auto const& candidateNotification =
      invitedCandidateReports.attributeOwnershipAcquisitionReports.front();
  REQUIRE(candidateNotification.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification);
  REQUIRE(candidateNotification.objectInstance == objectInstance);
  REQUIRE(candidateNotification.attributes == candidateAttributes);
  REQUIRE(variableLengthDataBytes(candidateNotification.userSuppliedTag) ==
          std::vector<unsigned char>(
              ifAvailableAcquisitionTagBytes,
              ifAvailableAcquisitionTagBytes + sizeof(ifAvailableAcquisitionTagBytes)));
  REQUIRE(invitedCandidate->isAttributeOwnedByFederate(objectInstance, reliableChild));
  REQUIRE(invitedCandidate->isAttributeOwnedByFederate(objectInstance, unownedChild));

  REQUIRE_NOTHROW(owner->unpublishObjectClassAttributes(child, ownedAttributes));
  REQUIRE_NOTHROW(invitedCandidate->unpublishObjectClassAttributes(child, candidateAttributes));
  REQUIRE_NOTHROW(staleCandidate->unpublishObjectClassAttributes(child, candidateAttributes));
  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(regularRequester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(ifAvailableRequester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(invitedCandidate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(staleCandidate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(unpublishedCandidate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(regularRequester->disconnect());
  REQUIRE_NOTHROW(ifAvailableRequester->disconnect());
  REQUIRE_NOTHROW(invitedCandidate->disconnect());
  REQUIRE_NOTHROW(staleCandidate->disconnect());
  REQUIRE_NOTHROW(unpublishedCandidate->disconnect());
}
} // namespace
