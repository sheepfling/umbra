#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

TEST_CASE(
    "Immediate callbacks apply the bounded automatic NoAction forced-resign policy synchronously",
    "[integration][development-profile][federation-management][transport]"
    "[ownership-management][connection-lost-automatic-no-action-immediate]"
    "[standalone][2025][callback-model][immediate]"
    "[connection-lost-no-action-policy-immediate]"
    "[rti.service.connection-lost]"
    "[rti.service.get-automatic-resign-directive]"
    "[rti.service.set-automatic-resign-directive]"
    "[federate.callback.connection-lost]"
    "[federate.callback.request-attribute-ownership-assumption]") {
  FederationEventFederateAmbassador lostReports;
  ReportingFederateAmbassador survivingReports;
  auto lost = makeRti();
  auto surviving = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(lost->connect(lostReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(
      surviving->connect(survivingReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(lost->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle lostFederate;
  REQUIRE_NOTHROW(lostFederate = lost->joinFederationExecution(
      L"automatic-no-action-immediate-lost",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(surviving->joinFederationExecution(
      L"automatic-no-action-immediate-survivor",
      L"subscriber",
      federationName));

  auto const server =
      lost->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency =
      lost->getAttributeHandle(server, fixture_hla::fixture::efficiency);
  auto const privilegeToDelete = lost->getAttributeHandle(
      server,
      standard_hla::mom::privilege_to_delete_object);
  AttributeHandleSet const efficiencyOnly{efficiency};
  AttributeHandleSet const expectedAssumption{efficiency, privilegeToDelete};
  REQUIRE(server.isValid());
  REQUIRE(efficiency.isValid());
  REQUIRE(privilegeToDelete.isValid());
  REQUIRE_NOTHROW(lost->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(
      surviving->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(
      surviving->publishObjectClassAttributes(server, efficiencyOnly));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = lost->registerObjectInstance(server));
  auto const objectInstanceName = lost->getObjectInstanceName(objectInstance);
  REQUIRE(survivingReports.objectDiscoveryReports.size() == 1);
  REQUIRE(survivingReports.objectDiscoveryReports.front().objectInstance ==
          objectInstance);

  REQUIRE_NOTHROW(lost->setAutomaticResignDirective(rti1516_2025::NO_ACTION));
  REQUIRE(lost->getAutomaticResignDirective() == rti1516_2025::NO_ACTION);

  std::wstring const faultDescription =
      L"automatic NoAction immediate transport fault";
  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      faultDescription));

  REQUIRE(lostReports.faultDescriptions ==
          std::vector<std::wstring>{faultDescription});
  REQUIRE(survivingReports.attributeOwnershipAssumptionReports.size() == 1);
  auto const& assumption =
      survivingReports.attributeOwnershipAssumptionReports.front();
  REQUIRE(assumption.objectInstance == objectInstance);
  REQUIRE(assumption.attributes == expectedAssumption);
  REQUIRE(survivingReports.objectRemovalReports.empty());
  REQUIRE(surviving->getObjectInstanceHandle(objectInstanceName) ==
          objectInstance);
  REQUIRE_FALSE(
      surviving->isAttributeOwnedByFederate(objectInstance, efficiency));
  REQUIRE_FALSE(surviving->isAttributeOwnedByFederate(
      objectInstance,
      privilegeToDelete));
  REQUIRE(surviving->getFederateName(lostFederate) ==
          L"automatic-no-action-immediate-lost");

  REQUIRE_NOTHROW(surviving->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(surviving->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(lost->connect(lostReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(surviving->disconnect());
}

} // namespace
