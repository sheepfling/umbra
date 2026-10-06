#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded Query Attribute Ownership reports 2025 federate and unowned attributes",
    "[integration][development-profile][ownership-management]"
    "[query-attribute-ownership]"
    "[rti.service.query-attribute-ownership]"
    "[federate.callback.inform-attribute-ownership]"
    "[federate.callback.attribute-is-not-owned]") {
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
  AttributeHandleSet noAttributes;

  REQUIRE_THROWS_AS(
      unjoined->queryAttributeOwnership(invalidObjectInstance, noAttributes),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->queryAttributeOwnership(invalidObjectInstance, noAttributes),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      owner->joinFederationExecution(L"query-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(
      requester->joinFederationExecution(L"query-requester", L"subscriber", federationName));

  auto const child = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliableBaseA = owner->getAttributeHandle(child, L"ReliableBaseA");
  auto const reliableChild = owner->getAttributeHandle(child, L"ReliableChild");
  auto const unownedChild = owner->getAttributeHandle(child, L"UnownedChild");
  REQUIRE(child.isValid());
  REQUIRE(reliableBaseA.isValid());
  REQUIRE(reliableChild.isValid());
  REQUIRE(unownedChild.isValid());

  AttributeHandleSet const ownedAttributes{reliableBaseA, reliableChild};
  AttributeHandleSet const requesterSubscriptions{
      reliableBaseA,
      reliableChild,
      unownedChild,
  };
  AttributeHandleSet const queriedAttributes{reliableBaseA, unownedChild};
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(child, requesterSubscriptions));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, ownedAttributes));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE(requesterReports.objectDiscoveryReports.empty());
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1);
  REQUIRE(requester->getKnownObjectClassHandle(objectInstance) == child);

  AttributeHandle invalidAttribute;
  AttributeHandleSet const invalidAttributes{invalidAttribute};
  REQUIRE_THROWS_AS(
      requester->queryAttributeOwnership(invalidObjectInstance, queriedAttributes),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      requester->queryAttributeOwnership(objectInstance, invalidAttributes),
      rti1516_2025::AttributeNotDefined);

  // One successful query groups attributes by the standard ownership result:
  // a concrete joined federate is identified through Inform Attribute
  // Ownership, while an available attribute reaches Attribute Is Not Owned.
  REQUIRE_NOTHROW(requester->queryAttributeOwnership(objectInstance, queriedAttributes));
  REQUIRE(requesterReports.attributeOwnershipReports.empty());
  REQUIRE(ownerReports.attributeOwnershipReports.empty());
  REQUIRE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipReports.size() == 2);

  auto const federateReport = std::find_if(
      requesterReports.attributeOwnershipReports.begin(),
      requesterReports.attributeOwnershipReports.end(),
      [](ReportingFederateAmbassador::AttributeOwnershipReport const& report) {
        return report.kind == ReportingFederateAmbassador::AttributeOwnershipReport::Kind::federate;
      });
  auto const unownedReport = std::find_if(
      requesterReports.attributeOwnershipReports.begin(),
      requesterReports.attributeOwnershipReports.end(),
      [](ReportingFederateAmbassador::AttributeOwnershipReport const& report) {
        return report.kind == ReportingFederateAmbassador::AttributeOwnershipReport::Kind::unowned;
      });
  REQUIRE(federateReport != requesterReports.attributeOwnershipReports.end());
  REQUIRE(unownedReport != requesterReports.attributeOwnershipReports.end());
  REQUIRE(federateReport->objectInstance == objectInstance);
  REQUIRE(federateReport->attributes == AttributeHandleSet{reliableBaseA});
  REQUIRE(federateReport->owner == owner->getFederateHandle(L"query-owner"));
  REQUIRE(unownedReport->objectInstance == objectInstance);
  REQUIRE(unownedReport->attributes == AttributeHandleSet{unownedChild});
  REQUIRE(std::none_of(
      requesterReports.attributeOwnershipReports.begin(),
      requesterReports.attributeOwnershipReports.end(),
      [](ReportingFederateAmbassador::AttributeOwnershipReport const& report) {
        return report.kind == ReportingFederateAmbassador::AttributeOwnershipReport::Kind::rti;
      }));

  // Remove Object Instance nullifies reports that were queued by an earlier
  // query. The queued removal still reaches the requester, but no stale
  // ownership result may enter its FederateAmbassador afterward.
  VariableLengthData deletionTag;
  REQUIRE_NOTHROW(requester->queryAttributeOwnership(objectInstance, queriedAttributes));
  REQUIRE_NOTHROW(owner->deleteObjectInstance(objectInstance, deletionTag));
  REQUIRE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipReports.size() == 2);
  REQUIRE(requesterReports.objectRemovalReports.size() == 1);
  REQUIRE(requesterReports.objectRemovalReports.front().objectInstance == objectInstance);

  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}
} // namespace
