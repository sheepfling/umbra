#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

TEST_CASE(
    "Immediate callbacks apply the final-federate forced directive-two rule synchronously",
    "[integration][development-profile][federation-management][transport]"
    "[object-management][connection-lost-final-federate-immediate]"
    "[standalone][2025][callback-model][immediate]"
    "[rti.service.connection-lost]"
    "[rti.service.get-automatic-resign-directive]"
    "[rti.service.set-automatic-resign-directive]"
    "[rti.service.reserve-object-instance-name]"
    "[federate.callback.connection-lost]"
    "[federate.callback.object-instance-name-reservation-succeeded]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(rti->connect(reports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(rti->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"automatic-final-immediate-lost",
      L"publisher",
      federationName));

  auto const server = rti->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = rti->getAttributeHandle(server, fixture_hla::fixture::efficiency);
  AttributeHandleSet const efficiencyOnly{efficiency};
  REQUIRE_NOTHROW(rti->publishObjectClassAttributes(server, efficiencyOnly));

  ObjectInstanceHandle originalObject;
  REQUIRE_NOTHROW(originalObject = rti->registerObjectInstance(server));
  auto const reusableObjectName = rti->getObjectInstanceName(originalObject);

  // The final-member rule is directive two even when the configured automatic
  // directive is NO_ACTION.  HLA_IMMEDIATE must deliver Connection Lost before
  // failEmbeddedTransportConnectionForTesting returns.
  REQUIRE_NOTHROW(rti->setAutomaticResignDirective(rti1516_2025::NO_ACTION));
  REQUIRE(rti->getAutomaticResignDirective() == rti1516_2025::NO_ACTION);
  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *rti,
      L"final-federate immediate transport fault"));
  REQUIRE(reports.faultDescriptions ==
          std::vector<std::wstring>{L"final-federate immediate transport fault"});
  REQUIRE(reports.resignationDescriptions.empty());
  REQUIRE_THROWS_AS(rti->disconnect(), rti1516_2025::NotConnected);

  // Rejoining begins a new lifetime.  The old object name must be reservable,
  // proving that final-member cleanup used directive two synchronously.
  REQUIRE_NOTHROW(rti->connect(reports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"automatic-final-immediate-rejoined",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(rti->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(rti->reserveObjectInstanceName(reusableObjectName));
  REQUIRE(reports.objectInstanceNameReservationSucceededReports.size() == 1);
  REQUIRE(
      reports.objectInstanceNameReservationSucceededReports.front().objectInstanceName ==
      reusableObjectName);

  ObjectInstanceHandle replacementObject;
  REQUIRE_NOTHROW(
      replacementObject = rti->registerObjectInstance(server, reusableObjectName));
  REQUIRE(replacementObject.isValid());
  REQUIRE(replacementObject != originalObject);

  REQUIRE_NOTHROW(rti->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

} // namespace
