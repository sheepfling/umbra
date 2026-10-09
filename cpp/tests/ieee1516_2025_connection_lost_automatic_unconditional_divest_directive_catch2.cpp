#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

TEST_CASE(
    "Embedded transport loss applies the configured automatic unconditional-divest directive",
    "[integration][development-profile][federation-management][transport]"
    "[ownership-management][rti.service.connection-lost]"
    "[rti.service.get-automatic-resign-directive]"
    "[rti.service.set-automatic-resign-directive]"
    "[federate.callback.connection-lost]"
    "[federate.callback.request-attribute-ownership-assumption]") {
  FederationEventFederateAmbassador lostReports;
  ReportingFederateAmbassador survivingReports;
  auto lost = makeRti();
  auto surviving = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(surviving->connect(survivingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(lost->joinFederationExecution(
      L"automatic-divest-lost",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(surviving->joinFederationExecution(
      L"automatic-divest-survivor",
      L"subscriber",
      federationName));

  auto const server = lost->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = lost->getAttributeHandle(server, fixture_hla::fixture::efficiency);
  auto const privilegeToDelete = lost->getAttributeHandle(
      server,
      standard_hla::mom::privilege_to_delete_object);
  AttributeHandleSet const efficiencyOnly{efficiency};
  AttributeHandleSet const expectedAssumption{efficiency, privilegeToDelete};
  REQUIRE_NOTHROW(lost->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(surviving->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(surviving->publishObjectClassAttributes(server, efficiencyOnly));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = lost->registerObjectInstance(server));
  auto const objectInstanceName = lost->getObjectInstanceName(objectInstance);
  REQUIRE_FALSE(surviving->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(survivingReports.objectDiscoveryReports.size() == 1);

  // IEEE 1516.1-2025 §4.1.5 requires loss cleanup to use the member's
  // Automatic Resign Directive. Unlike the delete branch above, directive 1
  // retains the object while unconditionally divesting its owned attributes.
  REQUIRE_NOTHROW(lost->setAutomaticResignDirective(
      rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES));
  REQUIRE(
      lost->getAutomaticResignDirective() ==
      rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES);

  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      L"automatic unconditional-divest transport fault"));
  REQUIRE(lostReports.faultDescriptions.empty());
  REQUIRE(survivingReports.attributeOwnershipAssumptionReports.empty());
  REQUIRE(survivingReports.objectRemovalReports.empty());

  REQUIRE_FALSE(lost->evokeCallback(0.0));
  REQUIRE(lostReports.faultDescriptions == std::vector<std::wstring>{
      L"automatic unconditional-divest transport fault"});
  REQUIRE_FALSE(surviving->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(survivingReports.attributeOwnershipAssumptionReports.size() == 1);
  auto const& assumption = survivingReports.attributeOwnershipAssumptionReports.front();
  REQUIRE(assumption.objectInstance == objectInstance);
  REQUIRE(assumption.attributes == expectedAssumption);
  REQUIRE(survivingReports.objectRemovalReports.empty());
  REQUIRE(surviving->getObjectInstanceHandle(objectInstanceName) == objectInstance);

  REQUIRE_NOTHROW(surviving->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(surviving->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(surviving->disconnect());
}

} // namespace
