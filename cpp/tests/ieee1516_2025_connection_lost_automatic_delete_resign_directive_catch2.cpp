#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

TEST_CASE(
    "Embedded transport loss applies the configured automatic delete resign directive",
    "[integration][development-profile][federation-management][transport]"
    "[object-management][rti.service.connection-lost]"
    "[rti.service.get-automatic-resign-directive]"
    "[rti.service.set-automatic-resign-directive]"
    "[federate.callback.connection-lost][federate.callback.remove-object-instance]") {
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
  FederateHandle lostFederate;
  REQUIRE_NOTHROW(lostFederate = lost->joinFederationExecution(
      L"automatic-delete-lost",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(surviving->joinFederationExecution(
      L"automatic-delete-survivor",
      L"subscriber",
      federationName));

  auto const server = lost->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = lost->getAttributeHandle(server, fixture_hla::fixture::efficiency);
  AttributeHandleSet const efficiencyOnly{efficiency};
  REQUIRE_NOTHROW(lost->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(surviving->subscribeObjectClassAttributes(server, efficiencyOnly));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = lost->registerObjectInstance(server));
  auto const objectInstanceName = lost->getObjectInstanceName(objectInstance);
  REQUIRE_FALSE(surviving->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(survivingReports.objectDiscoveryReports.size() == 1);
  REQUIRE(survivingReports.objectRemovalReports.empty());

  // Do not rely on the current FDD default: verify that the per-federate
  // directive selected through the official support service controls the
  // forced-resignation disposition.
  REQUIRE_NOTHROW(lost->setAutomaticResignDirective(rti1516_2025::DELETE_OBJECTS));
  REQUIRE(lost->getAutomaticResignDirective() == rti1516_2025::DELETE_OBJECTS);

  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      L"automatic delete transport fault"));
  REQUIRE(lostReports.faultDescriptions.empty());
  REQUIRE(survivingReports.objectRemovalReports.empty());

  REQUIRE_FALSE(lost->evokeCallback(0.0));
  REQUIRE(lostReports.faultDescriptions ==
          std::vector<std::wstring>{L"automatic delete transport fault"});
  REQUIRE(lostReports.resignationDescriptions.empty());
  REQUIRE_FALSE(surviving->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(survivingReports.objectRemovalReports.size() == 1);
  auto const& removal = survivingReports.objectRemovalReports.front();
  REQUIRE(removal.objectInstance == objectInstance);
  REQUIRE(removal.producingFederate == lostFederate);
  // Connection Lost performs a resignation on behalf of the lost federate.
  // The returned identity remains a valid designator even though the member
  // report below no longer lists it as joined.
  REQUIRE(surviving->getFederateName(lostFederate) == L"automatic-delete-lost");
  REQUIRE_THROWS_AS(
      surviving->getObjectInstanceHandle(objectInstanceName),
      rti1516_2025::ObjectInstanceNotKnown);

  REQUIRE_NOTHROW(surviving->listFederationExecutionMembers(federationName));
  REQUIRE_FALSE(surviving->evokeCallback(0.0));
  REQUIRE(survivingReports.federationExecutionMemberReports.size() == 1);
  REQUIRE(survivingReports.federationExecutionMemberReports.front().members.size() == 1);
  REQUIRE(
      survivingReports.federationExecutionMemberReports.front().members.front().federateName ==
      L"automatic-delete-survivor");

  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(surviving->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(surviving->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(surviving->disconnect());
}

} // namespace
