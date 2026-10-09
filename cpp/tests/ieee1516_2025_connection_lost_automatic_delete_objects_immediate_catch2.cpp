#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

TEST_CASE(
    "Immediate callbacks apply the configured automatic delete-objects directive synchronously",
    "[integration][development-profile][federation-management][transport]"
    "[ownership-management][object-management][connection-lost-automatic-delete-objects-immediate]"
    "[standalone][2025][callback-model][immediate]"
    "[rti.service.connection-lost]"
    "[rti.service.get-automatic-resign-directive]"
    "[rti.service.set-automatic-resign-directive]"
    "[federate.callback.connection-lost]"
    "[federate.callback.remove-object-instance]"
    "[multi-federate-callback-ordering]") {
  ReportingFederateAmbassador lostReports;
  ReportingFederateAmbassador survivingReports;
  auto lost = makeRti();
  auto surviving = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(lost->connect(lostReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(
      surviving->connect(survivingReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(lost->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(lost->joinFederationExecution(
      L"automatic-delete-objects-immediate-lost",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(surviving->joinFederationExecution(
      L"automatic-delete-objects-immediate-survivor",
      L"subscriber",
      federationName));

  auto const server = lost->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = lost->getAttributeHandle(server, fixture_hla::fixture::efficiency);
  AttributeHandleSet const efficiencyOnly{efficiency};
  REQUIRE_NOTHROW(lost->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(lost->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(surviving->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(surviving->subscribeObjectClassAttributes(server, efficiencyOnly));

  // The lost federate owns this object and therefore gives directive 2 one
  // delete-privileged object to remove at the transport-fault boundary.
  ObjectInstanceHandle deletedObject;
  REQUIRE_NOTHROW(deletedObject = lost->registerObjectInstance(server));
  auto const deletedObjectName = lost->getObjectInstanceName(deletedObject);
  REQUIRE(survivingReports.objectDiscoveryReports.size() == 1);
  REQUIRE(survivingReports.objectDiscoveryReports.front().objectInstance ==
          deletedObject);

  REQUIRE_NOTHROW(lost->setAutomaticResignDirective(rti1516_2025::DELETE_OBJECTS));
  REQUIRE(lost->getAutomaticResignDirective() == rti1516_2025::DELETE_OBJECTS);
  REQUIRE(survivingReports.objectRemovalReports.empty());

  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      L"automatic delete-objects immediate transport fault"));
  REQUIRE(lostReports.faultDescriptions == std::vector<std::wstring>{
      L"automatic delete-objects immediate transport fault"});
  REQUIRE(survivingReports.objectRemovalReports.size() == 1);
  REQUIRE(survivingReports.objectRemovalReports.front().objectInstance ==
          deletedObject);
  REQUIRE_THROWS_AS(
      surviving->getObjectInstanceHandle(deletedObjectName),
      rti1516_2025::ObjectInstanceNotKnown);

  REQUIRE_NOTHROW(surviving->resignFederationExecution(rti1516_2025::DELETE_OBJECTS));
  REQUIRE_NOTHROW(surviving->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(lost->connect(lostReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(surviving->disconnect());
}

} // namespace
