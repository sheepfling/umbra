#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded Attribute Ownership Acquisition honors 2025 release and denial callbacks",
    "[integration][development-profile][ownership-management]"
    "[rti.service.attribute-ownership-acquisition]"
    "[federate.callback.request-attribute-ownership-release]"
    "[rti.service.attribute-ownership-release-denied]"
    "[federate.callback.attribute-ownership-acquisition-notification]"
    "[federate.callback.attribute-ownership-unavailable]"
    "[attribute-ownership-acquisition-release-callback]") {
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
  unsigned char const willingToAcquireTagBytes[] = {0x19, 0x53};
  unsigned char const acquisitionTagBytes[] = {0xA5, 0x70, 0xE1};
  unsigned char const denialTagBytes[] = {0xD3, 0x1A, 0x1E, 0xD0};
  VariableLengthData const willingToAcquireTag(
      willingToAcquireTagBytes,
      sizeof(willingToAcquireTagBytes));
  VariableLengthData const acquisitionTag(acquisitionTagBytes, sizeof(acquisitionTagBytes));
  VariableLengthData const denialTag(denialTagBytes, sizeof(denialTagBytes));
  ObjectInstanceHandle invalidObjectInstance;
  AttributeHandle invalidAttribute;
  AttributeHandleSet noAttributes;

  REQUIRE_THROWS_AS(
      unjoined->attributeOwnershipAcquisition(
          invalidObjectInstance,
          noAttributes,
          acquisitionTag),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->attributeOwnershipAcquisition(
          invalidObjectInstance,
          noAttributes,
          acquisitionTag),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      owner->joinFederationExecution(L"regular-acquisition-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"regular-acquisition-requester",
      L"publisher",
      federationName));

  auto const child = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliableBaseA = owner->getAttributeHandle(child, L"ReliableBaseA");
  auto const unownedChild = owner->getAttributeHandle(child, L"UnownedChild");
  REQUIRE(child.isValid());
  REQUIRE(reliableBaseA.isValid());
  REQUIRE(unownedChild.isValid());

  AttributeHandleSet const requesterAttributes{reliableBaseA, unownedChild};
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(child, requesterAttributes));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, AttributeHandleSet{reliableBaseA}));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1);
  REQUIRE(requester->getKnownObjectClassHandle(objectInstance) == child);

  REQUIRE_THROWS_AS(
      requester->attributeOwnershipAcquisition(
          objectInstance,
          AttributeHandleSet{unownedChild},
          acquisitionTag),
      rti1516_2025::ObjectClassNotPublished);
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(child, requesterAttributes));
  REQUIRE_THROWS_AS(
      requester->attributeOwnershipAcquisition(
          invalidObjectInstance,
          AttributeHandleSet{unownedChild},
          acquisitionTag),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      requester->attributeOwnershipAcquisition(
          objectInstance,
          AttributeHandleSet{invalidAttribute},
          acquisitionTag),
      rti1516_2025::AttributeNotDefined);
  REQUIRE_THROWS_AS(
      owner->attributeOwnershipReleaseDenied(
          objectInstance,
          AttributeHandleSet{unownedChild},
          denialTag),
      rti1516_2025::AttributeNotOwned);

  // The regular service overrides this requester's still-pending WTA state.
  // The earlier queued If Available work therefore becomes a no-delivery
  // callback rather than reporting the remote-owned attribute unavailable.
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisitionIfAvailable(
      objectInstance,
      AttributeHandleSet{reliableBaseA},
      willingToAcquireTag));
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
      objectInstance,
      requesterAttributes,
      acquisitionTag));
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(objectInstance, unownedChild));
  REQUIRE_THROWS_AS(
      requester->attributeOwnershipAcquisitionIfAvailable(
          objectInstance,
          AttributeHandleSet{reliableBaseA},
          willingToAcquireTag),
      rti1516_2025::AttributeAlreadyBeingAcquired);
  // A repeated regular request preserves the Acquisition Pending state and
  // must not cause a duplicate Request Attribute Ownership Release callback.
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
      objectInstance,
      AttributeHandleSet{reliableBaseA},
      acquisitionTag));
  REQUIRE_THROWS_AS(
      requester->unpublishObjectClassAttributes(child, AttributeHandleSet{unownedChild}),
      rti1516_2025::OwnershipAcquisitionPending);
  REQUIRE_THROWS_AS(
      requester->unpublishObjectClassAttributes(child, AttributeHandleSet{reliableBaseA}),
      rti1516_2025::OwnershipAcquisitionPending);

  // The earlier If Available reservation leaves stale internal work in the
  // owner dispatcher after the regular request supersedes it.  Draining that
  // no-op must not be treated as a public callback; the following pass is the
  // observable empty-queue boundary before the single release request.
  REQUIRE_NOTHROW(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.size() == 1);
  auto const& releaseRequest = ownerReports.attributeOwnershipReleaseRequestReports.front();
  REQUIRE(releaseRequest.objectInstance == objectInstance);
  REQUIRE(releaseRequest.attributes == AttributeHandleSet{reliableBaseA});
  REQUIRE(variableLengthDataBytes(releaseRequest.userSuppliedTag) ==
          std::vector<unsigned char>(acquisitionTagBytes,
                                     acquisitionTagBytes + sizeof(acquisitionTagBytes)));

  // The WTA work was queued first and is now stale; the second callback is
  // the regular unowned-acquisition notification.
  REQUIRE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.empty());
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.size() == 1);
  auto const& notification = requesterReports.attributeOwnershipAcquisitionReports.front();
  REQUIRE(notification.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification);
  REQUIRE(notification.objectInstance == objectInstance);
  REQUIRE(notification.attributes == AttributeHandleSet{unownedChild});
  REQUIRE(variableLengthDataBytes(notification.userSuppliedTag) ==
          std::vector<unsigned char>(acquisitionTagBytes,
                                     acquisitionTagBytes + sizeof(acquisitionTagBytes)));
  REQUIRE(requester->isAttributeOwnedByFederate(objectInstance, unownedChild));
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_NOTHROW(requester->unpublishObjectClassAttributes(
      child,
      AttributeHandleSet{unownedChild}));
  REQUIRE_THROWS_AS(
      requester->unpublishObjectClassAttributes(child, AttributeHandleSet{reliableBaseA}),
      rti1516_2025::OwnershipAcquisitionPending);

  // Release Denied preserves the owner's attribute ownership, ends every
  // matching regular acquisition, and supplies its own tag to Unavailable.
  REQUIRE_NOTHROW(owner->attributeOwnershipReleaseDenied(
      objectInstance,
      AttributeHandleSet{reliableBaseA},
      denialTag));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.size() == 2);
  auto const& unavailable = requesterReports.attributeOwnershipAcquisitionReports.back();
  REQUIRE(unavailable.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::unavailable);
  REQUIRE(unavailable.objectInstance == objectInstance);
  REQUIRE(unavailable.attributes == AttributeHandleSet{reliableBaseA});
  REQUIRE(variableLengthDataBytes(unavailable.userSuppliedTag) ==
          std::vector<unsigned char>(denialTagBytes, denialTagBytes + sizeof(denialTagBytes)));
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_NOTHROW(requester->unpublishObjectClassAttributes(
      child,
      AttributeHandleSet{reliableBaseA}));
  REQUIRE_NOTHROW(owner->attributeOwnershipReleaseDenied(
      objectInstance,
      AttributeHandleSet{reliableBaseA},
      denialTag));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));

  // Receive-order removal invalidates a queued regular owner-release
  // callback before it can enter user code or create a terminal report.
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(
      child,
      AttributeHandleSet{reliableBaseA}));
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
      objectInstance,
      AttributeHandleSet{reliableBaseA},
      acquisitionTag));
  VariableLengthData deletionTag;
  REQUIRE_NOTHROW(owner->deleteObjectInstance(objectInstance, deletionTag));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.size() == 1);
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.size() == 2);
  REQUIRE(requesterReports.objectRemovalReports.size() == 1);

  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}
} // namespace
