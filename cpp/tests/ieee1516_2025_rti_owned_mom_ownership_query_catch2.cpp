#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded RTI-owned MOM attributes participate in ownership queries",
    "[integration][development-profile][federation-management][mom]"
    "[ownership-management][rti-owned-state][query-attribute-ownership]"
    "[rti-owned-mom-ownership-query]"
    "[rti.service.query-attribute-ownership]"
    "[rti.service.is-attribute-owned-by-federate]"
    "[federate.callback.attribute-is-owned-by-rti]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-nrg-disabled-fom.xml")
          .wstring();
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"rti-owned-query-subject", L"observer", federationName));

  auto const momClass = rti->getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const federateNameAttribute = rti->getAttributeHandle(momClass, L"HLAfederateName");
  auto const federateHandleAttribute = rti->getAttributeHandle(momClass, L"HLAfederateHandle");
  AttributeHandleSet const queriedAttributes{
      federateNameAttribute,
      federateHandleAttribute,
  };
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

  // The read-only boolean query must report false for an RTI-owned MOM
  // attribute; the invoking joined federate never becomes its owner.
  REQUIRE_FALSE(rti->isAttributeOwnedByFederate(momObject, federateNameAttribute));

  // Query Attribute Ownership has a distinct callback for the RTI owner.  It
  // must not be confused with either a federate owner or an unowned attribute.
  REQUIRE_NOTHROW(rti->queryAttributeOwnership(momObject, queriedAttributes));
  REQUIRE(reports.attributeOwnershipReports.empty());
  REQUIRE_FALSE(rti->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(reports.attributeOwnershipReports.size() == 1U);
  auto const& report = reports.attributeOwnershipReports.front();
  REQUIRE(report.kind == ReportingFederateAmbassador::AttributeOwnershipReport::Kind::rti);
  REQUIRE(report.objectInstance == momObject);
  REQUIRE(report.attributes == queriedAttributes);
  REQUIRE_FALSE(report.owner.isValid());

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}
} // namespace
