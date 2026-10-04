#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded Attribute Ownership Acquisition If Available resolves 2025 ownership callbacks",
    "[integration][development-profile][ownership-management]"
    "[rti.service.attribute-ownership-acquisition-if-available]"
    "[federate.callback.attribute-ownership-acquisition-notification]"
    "[federate.callback.attribute-ownership-unavailable]") {
  TestFederateAmbassador unjoinedFederate;
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x51, 0xA7, 0x0C};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  ObjectInstanceHandle invalidObjectInstance;
  AttributeHandle invalidAttribute;
  AttributeHandleSet noAttributes;

  REQUIRE_THROWS_AS(
      unjoined->attributeOwnershipAcquisitionIfAvailable(
          invalidObjectInstance,
          noAttributes,
          tag),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->attributeOwnershipAcquisitionIfAvailable(
          invalidObjectInstance,
          noAttributes,
          tag),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      owner->joinFederationExecution(L"acquisition-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(
      requester->joinFederationExecution(L"acquisition-requester", L"publisher", federationName));

  auto const child = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliableBaseA = owner->getAttributeHandle(child, L"ReliableBaseA");
  auto const reliableChild = owner->getAttributeHandle(child, L"ReliableChild");
  auto const unownedChild = owner->getAttributeHandle(child, L"UnownedChild");
  REQUIRE(child.isValid());
  REQUIRE(reliableBaseA.isValid());
  REQUIRE(reliableChild.isValid());
  REQUIRE(unownedChild.isValid());

  AttributeHandleSet const ownerPublishedAttributes{reliableBaseA};
  AttributeHandleSet const requesterSubscriptions{
      reliableBaseA,
      reliableChild,
      unownedChild,
  };
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(child, requesterSubscriptions));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, ownerPublishedAttributes));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1);
  REQUIRE(requester->getKnownObjectClassHandle(objectInstance) == child);

  // The requester must publish at its known class before it can enter the
  // 2025 Willing to Acquire state. Publishing only one requested attribute
  // distinguishes class publication from per-attribute publication.
  REQUIRE_THROWS_AS(
      requester->attributeOwnershipAcquisitionIfAvailable(
          objectInstance,
          AttributeHandleSet{unownedChild},
          tag),
      rti1516_2025::ObjectClassNotPublished);
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(
      child,
      AttributeHandleSet{unownedChild}));
  REQUIRE_THROWS_AS(
      requester->attributeOwnershipAcquisitionIfAvailable(
          objectInstance,
          AttributeHandleSet{reliableBaseA},
          tag),
      rti1516_2025::AttributeNotPublished);
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(
      child,
      AttributeHandleSet{reliableBaseA, reliableChild}));

  REQUIRE_THROWS_AS(
      requester->attributeOwnershipAcquisitionIfAvailable(
          invalidObjectInstance,
          AttributeHandleSet{unownedChild},
          tag),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      requester->attributeOwnershipAcquisitionIfAvailable(
          objectInstance,
          AttributeHandleSet{invalidAttribute},
          tag),
      rti1516_2025::AttributeNotDefined);

  AttributeHandleSet const mixedAvailabilityAttributes{unownedChild, reliableBaseA};
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisitionIfAvailable(
      objectInstance,
      mixedAvailabilityAttributes,
      tag));
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.empty());
  // Ownership transfers only when the matching callback begins. Until then,
  // 2025 requires a repeated If Available request for the same WTA attributes
  // to leave them unchanged, not fail or queue another terminal callback.
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(objectInstance, unownedChild));
  REQUIRE_NOTHROW(
      requester->attributeOwnershipAcquisitionIfAvailable(
          objectInstance,
          AttributeHandleSet{unownedChild},
          tag));
  // A mixed repeat preserves the already-pending attribute and independently
  // admits the new eligible attribute into Willing to Acquire.
  REQUIRE_NOTHROW(
      requester->attributeOwnershipAcquisitionIfAvailable(
          objectInstance,
          AttributeHandleSet{unownedChild, reliableChild},
          tag));
  // The coupled declaration-management precondition prevents the requester
  // from withdrawing a publication that the pending 7.9 request still needs.
  REQUIRE_THROWS_AS(
      requester->unpublishObjectClassAttributes(child, AttributeHandleSet{unownedChild}),
      rti1516_2025::OwnershipAcquisitionPending);

  REQUIRE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.size() == 3);
  auto const notification = std::find_if(
      requesterReports.attributeOwnershipAcquisitionReports.begin(),
      requesterReports.attributeOwnershipAcquisitionReports.end(),
      [](ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport const& report) {
        return report.kind ==
            ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification;
      });
  auto const unavailable = std::find_if(
      requesterReports.attributeOwnershipAcquisitionReports.begin(),
      requesterReports.attributeOwnershipAcquisitionReports.end(),
      [](ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport const& report) {
        return report.kind ==
            ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::unavailable;
      });
  REQUIRE(notification != requesterReports.attributeOwnershipAcquisitionReports.end());
  REQUIRE(unavailable != requesterReports.attributeOwnershipAcquisitionReports.end());
  REQUIRE(notification->objectInstance == objectInstance);
  REQUIRE(notification->attributes == AttributeHandleSet{unownedChild});
  REQUIRE(variableLengthDataBytes(notification->userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(unavailable->objectInstance == objectInstance);
  REQUIRE(unavailable->attributes == AttributeHandleSet{reliableBaseA});
  REQUIRE(variableLengthDataBytes(unavailable->userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  auto const additionalNotification = std::find_if(
      requesterReports.attributeOwnershipAcquisitionReports.begin(),
      requesterReports.attributeOwnershipAcquisitionReports.end(),
      [&reliableChild](
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport const& report) {
        return report.kind ==
                   ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification &&
               report.attributes == AttributeHandleSet{reliableChild};
      });
  REQUIRE(additionalNotification != requesterReports.attributeOwnershipAcquisitionReports.end());
  REQUIRE(additionalNotification->objectInstance == objectInstance);
  REQUIRE(requester->isAttributeOwnedByFederate(objectInstance, unownedChild));
  REQUIRE(requester->isAttributeOwnedByFederate(objectInstance, reliableChild));
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_THROWS_AS(
      requester->attributeOwnershipAcquisitionIfAvailable(
          objectInstance,
          AttributeHandleSet{unownedChild},
          tag),
      rti1516_2025::FederateOwnsAttributes);
  REQUIRE_NOTHROW(
      requester->unpublishObjectClassAttributes(child, AttributeHandleSet{unownedChild}));

  // Remove Object Instance nullifies a queued If Available callback before it
  // can establish ownership of a deleted instance. Its queued removal still
  // reaches the requester afterward.
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisitionIfAvailable(
      objectInstance,
      AttributeHandleSet{reliableBaseA},
      tag));
  VariableLengthData deletionTag;
  REQUIRE_NOTHROW(owner->deleteObjectInstance(objectInstance, deletionTag));
  REQUIRE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.size() == 3);
  REQUIRE(requesterReports.objectRemovalReports.size() == 1);
  REQUIRE(requesterReports.objectRemovalReports.front().objectInstance == objectInstance);
  REQUIRE(ownerReports.attributeOwnershipAcquisitionReports.empty());

  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}
} // namespace
