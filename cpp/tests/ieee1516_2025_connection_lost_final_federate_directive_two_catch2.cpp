#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

TEST_CASE(
    "Embedded final-federate transport loss forces directive two",
    "[integration][development-profile][federation-management][transport]"
    "[object-management][connection-lost-final-federate]"
    "[rti.service.connection-lost]"
    "[rti.service.get-automatic-resign-directive]"
    "[rti.service.set-automatic-resign-directive]"
    "[rti.service.reserve-object-instance-name]"
    "[connection-lost-final-federate-directive-two]"
    "[federate.callback.connection-lost]"
    "[federate.callback.object-instance-name-reservation-succeeded]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED));
  REQUIRE_NOTHROW(rti->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle lostFederate;
  REQUIRE_NOTHROW(lostFederate = rti->joinFederationExecution(
      L"automatic-final-lost",
      L"publisher",
      federationName));

  auto const server = rti->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = rti->getAttributeHandle(server, fixture_hla::fixture::efficiency);
  AttributeHandleSet const efficiencyOnly{efficiency};
  REQUIRE_NOTHROW(rti->publishObjectClassAttributes(server, efficiencyOnly));

  ObjectInstanceHandle originalObject;
  REQUIRE_NOTHROW(originalObject = rti->registerObjectInstance(server));
  auto const reusableObjectName = rti->getObjectInstanceName(originalObject);

  // 4.12.4 applies directive two when the final joined federate leaves.  A
  // transport loss must use that same final-member rule even when the
  // configured automatic directive is NO_ACTION.
  REQUIRE_NOTHROW(rti->setAutomaticResignDirective(NO_ACTION));
  REQUIRE(rti->getAutomaticResignDirective() == NO_ACTION);
  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *rti,
      L"final-federate transport fault"));
  REQUIRE(reports.faultDescriptions.empty());

  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.faultDescriptions ==
          std::vector<std::wstring>{L"final-federate transport fault"});
  REQUIRE(reports.resignationDescriptions.empty());
  REQUIRE_THROWS_AS(rti->disconnect(), rti1516_2025::NotConnected);

  // The federation execution remains available after its last member is
  // removed.  Rejoining with a fresh lifetime must be able to reserve the
  // deleted object's name, proving that the final-member cleanup used
  // directive two rather than the configured NO_ACTION policy.
  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"automatic-final-rejoined",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(rti->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(rti->reserveObjectInstanceName(reusableObjectName));
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.objectInstanceNameReservationSucceededReports.size() == 1);
  REQUIRE(
      reports.objectInstanceNameReservationSucceededReports.front().objectInstanceName ==
      reusableObjectName);

  ObjectInstanceHandle replacementObject;
  REQUIRE_NOTHROW(
      replacementObject = rti->registerObjectInstance(server, reusableObjectName));
  REQUIRE(replacementObject.isValid());
  REQUIRE(replacementObject != originalObject);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

} // namespace
