#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded non-final resignation disconnects without disrupting the remaining joined federate",
    "[integration][development-profile][federation-management]"
    "[federate-lifetime-disconnect][ordinary-resign-disconnect]"
    "[rti.service.resign-federation-execution][rti.service.disconnect]") {
  ReportingFederateAmbassador departingReports;
  ReportingFederateAmbassador continuingReports;
  auto departing = makeRti();
  auto continuing = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(departing->connect(departingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(continuing->connect(continuingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(departing->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(departing->joinFederationExecution(
      L"departing-federate", L"publisher", federationName));
  REQUIRE_NOTHROW(continuing->joinFederationExecution(
      L"continuing-federate", L"publisher", federationName));

  auto const objectClass =
      continuing->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const attribute =
      continuing->getAttributeHandle(objectClass, fixture_hla::fixture::efficiency);
  REQUIRE(objectClass.isValid());
  REQUIRE(attribute.isValid());

  REQUIRE_NOTHROW(departing->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(departing->disconnect());

  AttributeHandleSet const attributes{attribute};
  REQUIRE_NOTHROW(
      continuing->publishObjectClassAttributes(objectClass, attributes));
  ObjectInstanceHandle object;
  REQUIRE_NOTHROW(object = continuing->registerObjectInstance(objectClass));
  REQUIRE(object.isValid());

  REQUIRE_NOTHROW(continuing->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(continuing->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(continuing->disconnect());
}
} // namespace
