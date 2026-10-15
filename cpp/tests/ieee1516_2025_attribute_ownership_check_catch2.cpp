#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded Is Attribute Owned By Federate reads 2025 ownership state",
    "[integration][development-profile][ownership-management]"
    "[rti.service.is-attribute-owned-by-federate]") {
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
  ObjectInstanceHandle invalidObjectInstance;
  AttributeHandle invalidAttribute;

  REQUIRE_THROWS_AS(
      unjoined->isAttributeOwnedByFederate(invalidObjectInstance, invalidAttribute),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->isAttributeOwnedByFederate(invalidObjectInstance, invalidAttribute),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      owner->joinFederationExecution(L"ownership-check-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(
      requester->joinFederationExecution(L"ownership-check-requester", L"subscriber", federationName));

  auto const child = owner->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const reliableBaseA = owner->getAttributeHandle(
      child, fixture_hla::fixture::reliable_base_a);
  auto const unownedChild = owner->getAttributeHandle(
      child, fixture_hla::fixture::unowned_child);
  REQUIRE(child.isValid());
  REQUIRE(reliableBaseA.isValid());
  REQUIRE(unownedChild.isValid());

  AttributeHandleSet const ownedAttributes{reliableBaseA};
  AttributeHandleSet const requesterSubscriptions{reliableBaseA, unownedChild};
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(child, requesterSubscriptions));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, ownedAttributes));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1);
  REQUIRE(requester->getKnownObjectClassHandle(objectInstance) == child);

  // The same shared 2025 ownership snapshot returns true only to its current
  // joined owner. A known remote owner and a defined-but-unowned attribute are
  // both false for the requester without triggering a callback.
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_FALSE(owner->isAttributeOwnedByFederate(objectInstance, unownedChild));
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(objectInstance, unownedChild));

  REQUIRE_THROWS_AS(
      requester->isAttributeOwnedByFederate(invalidObjectInstance, reliableBaseA),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      requester->isAttributeOwnedByFederate(objectInstance, invalidAttribute),
      rti1516_2025::AttributeNotDefined);

  // Once receive-order removal starts, the instance is no longer queryable
  // through this read-only ownership service before the removal callback runs.
  VariableLengthData deletionTag;
  REQUIRE_NOTHROW(owner->deleteObjectInstance(objectInstance, deletionTag));
  REQUIRE_THROWS_AS(
      requester->isAttributeOwnedByFederate(objectInstance, reliableBaseA),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectRemovalReports.size() == 1);

  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}

TEST_CASE(
    "Embedded attributes added by a join-time FOM merge remain unowned until notification",
    "[integration][development-profile][federation-management][ownership-management]"
    "[fom-module-management][rti.service.join-federation-execution]"
    "[rti.service.is-attribute-owned-by-federate]"
    "[rti.service.attribute-ownership-acquisition-if-available]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.attribute-ownership-acquisition-notification]"
    "[ownership-after-fom-merge]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador joiningReports;
  auto owner = makeRti();
  auto joining = makeRti();
  auto const federationName = nextFederationName();
  auto const sourceDirectory = std::filesystem::path(UMBRA_SOURCE_DIRECTORY);
  auto const initialModule =
      (sourceDirectory / "cpp" / "tests" / "data" / "attribute-update-passel-fom.xml")
          .wstring();
  auto const extensionModule =
      (sourceDirectory / "cpp" / "tests" / "data" /
       "ownership-post-join-fom-extension.xml")
          .wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(joining->connect(joiningReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName, initialModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"merge-owner", L"publisher", federationName));

  auto const objectClass = owner->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const reliableBaseA = owner->getAttributeHandle(
      objectClass, fixture_hla::fixture::reliable_base_a);
  AttributeHandleSet const initiallyPublished{reliableBaseA};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(objectClass, initiallyPublished));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(objectClass));
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, reliableBaseA));

  std::vector<std::wstring> const additionalModules{extensionModule};
  REQUIRE_NOTHROW(joining->joinFederationExecution(
      L"merge-joiner", L"subscriber", federationName, additionalModules));
  auto const joiningClass = joining->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const joiningBaseAttribute = joining->getAttributeHandle(
      joiningClass, fixture_hla::fixture::reliable_base_a);
  auto const addedAttribute = joining->getAttributeHandle(
      joiningClass,
      fixture_hla::fixture::added_after_registration);
  AttributeHandleSet const subscribedAttributes{joiningBaseAttribute, addedAttribute};
  REQUIRE_NOTHROW(joining->subscribeObjectClassAttributes(joiningClass, subscribedAttributes));
  while (joining->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  REQUIRE(joiningReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(joiningReports.objectDiscoveryReports.front().objectInstance == objectInstance);
  REQUIRE(joiningReports.objectDiscoveryReports.front().objectClass == joiningClass);

  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_FALSE(owner->isAttributeOwnedByFederate(objectInstance, addedAttribute));
  REQUIRE_FALSE(joining->isAttributeOwnedByFederate(objectInstance, joiningBaseAttribute));
  REQUIRE_FALSE(joining->isAttributeOwnedByFederate(objectInstance, addedAttribute));

  AttributeHandleSet const addedAttributeSet{addedAttribute};
  REQUIRE_NOTHROW(joining->publishObjectClassAttributes(joiningClass, addedAttributeSet));
  REQUIRE_FALSE(owner->isAttributeOwnedByFederate(objectInstance, addedAttribute));
  REQUIRE_FALSE(joining->isAttributeOwnedByFederate(objectInstance, addedAttribute));

  unsigned char const acquisitionTagBytes[] = {0xA9, 0x25};
  VariableLengthData const acquisitionTag(acquisitionTagBytes, sizeof(acquisitionTagBytes));
  REQUIRE_NOTHROW(joining->attributeOwnershipAcquisitionIfAvailable(
      objectInstance,
      addedAttributeSet,
      acquisitionTag));
  REQUIRE_FALSE(joining->isAttributeOwnedByFederate(objectInstance, addedAttribute));
  while (joining->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  REQUIRE(joiningReports.attributeOwnershipAcquisitionReports.size() == 1U);
  auto const& notification = joiningReports.attributeOwnershipAcquisitionReports.front();
  REQUIRE(notification.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification);
  REQUIRE(notification.objectInstance == objectInstance);
  REQUIRE(notification.attributes == addedAttributeSet);
  REQUIRE(joining->isAttributeOwnedByFederate(objectInstance, addedAttribute));
  REQUIRE_FALSE(owner->isAttributeOwnedByFederate(objectInstance, addedAttribute));

  REQUIRE_NOTHROW(joining->resignFederationExecution(
      rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES));
  REQUIRE_NOTHROW(owner->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(joining->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(joining->disconnect());
}
} // namespace
