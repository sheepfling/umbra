#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded Confirm Divestiture rejects an RTI-owned joined-federate MOM attribute",
    "[integration][development-profile][federation-management][mom]"
    "[ownership-management][rti-owned-mom-confirm-divestiture]"
    "[rti.service.confirm-divestiture][rti.service.query-attribute-ownership]"
    "[rti.service.is-attribute-owned-by-federate]"
    "[federate.callback.attribute-is-owned-by-rti][2025]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-nrg-disabled-fom.xml")
          .wstring();

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"rti-owned-confirm-subject", L"observer", federationName));

  auto const momClass = rti->getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const federateNameAttribute = rti->getAttributeHandle(momClass, L"HLAfederateName");
  auto const federateHandleAttribute = rti->getAttributeHandle(momClass, L"HLAfederateHandle");
  AttributeHandleSet const queriedAttributes{
      federateNameAttribute,
      federateHandleAttribute,
  };
  REQUIRE(momClass.isValid());
  REQUIRE(federateNameAttribute.isValid());
  REQUIRE(federateHandleAttribute.isValid());
  REQUIRE_NOTHROW(rti->subscribeObjectClassAttributes(momClass, queriedAttributes));
  while (rti->evokeCallback(0.0)) {
  }

  auto const discovered = std::find_if(
      reports.objectDiscoveryReports.begin(),
      reports.objectDiscoveryReports.end(),
      [&](ReportingFederateAmbassador::ObjectDiscoveryReport const& report) {
        return report.objectClass == momClass;
      });
  REQUIRE(discovered != reports.objectDiscoveryReports.end());
  auto const momObject = discovered->objectInstance;
  REQUIRE_FALSE(rti->isAttributeOwnedByFederate(momObject, federateNameAttribute));

  REQUIRE_THROWS_AS(
      rti->confirmDivestiture(
          momObject,
          AttributeHandleSet{federateNameAttribute},
          VariableLengthData{}),
      rti1516_2025::RTIinternalError);
  REQUIRE_FALSE(rti->isAttributeOwnedByFederate(momObject, federateNameAttribute));

  REQUIRE_NOTHROW(rti->queryAttributeOwnership(momObject, queriedAttributes));
  REQUIRE(reports.attributeOwnershipReports.empty());
  REQUIRE_FALSE(rti->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(reports.attributeOwnershipReports.size() == 1U);
  auto const& ownership = reports.attributeOwnershipReports.front();
  REQUIRE(ownership.kind == ReportingFederateAmbassador::AttributeOwnershipReport::Kind::rti);
  REQUIRE(ownership.objectInstance == momObject);
  REQUIRE(ownership.attributes == queriedAttributes);
  REQUIRE_FALSE(ownership.owner.isValid());

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}
} // namespace
