#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded RTI-forced resignation notifies the federate and permits Disconnect",
    "[integration][development-profile][federation-management]"
    "[federate-lifetime-disconnect][forced-resign-disconnect]"
    "[rti.service.disconnect][federate.callback.federate-resigned]") {
  std::wstring const reason = L"test RTI-initiated forced resignation";
  FederationEventFederateAmbassador departedReports;
  ReportingFederateAmbassador continuingReports;
  auto departed = makeRti();
  auto continuing = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(departed->connect(departedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(continuing->connect(continuingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(departed->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(departed->joinFederationExecution(
      L"rti-resigned-federate", L"publisher", federationName));
  REQUIRE_NOTHROW(continuing->joinFederationExecution(
      L"continuing-federate", L"subscriber", federationName));

  REQUIRE(umbra::detail::forceEmbeddedFederateResignationForTesting(
      *departed, reason));
  REQUIRE_FALSE(departed->evokeCallback(0.0));
  REQUIRE(departedReports.resignationDescriptions ==
          std::vector<std::wstring>{reason});

  REQUIRE_NOTHROW(continuing->listFederationExecutionMembers(federationName));
  REQUIRE_FALSE(continuing->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(continuingReports.federationExecutionMemberReports.size() == 1U);
  REQUIRE(continuingReports.federationExecutionMemberReports.front().members.size() == 1U);

  REQUIRE_NOTHROW(departed->disconnect());

  auto const objectClass =
      continuing->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const attribute =
      continuing->getAttributeHandle(objectClass, fixture_hla::fixture::efficiency);
  REQUIRE(objectClass.isValid());
  REQUIRE(attribute.isValid());
  REQUIRE_NOTHROW(
      continuing->publishObjectClassAttributes(objectClass, AttributeHandleSet{attribute}));
  ObjectInstanceHandle object;
  REQUIRE_NOTHROW(object = continuing->registerObjectInstance(objectClass));
  REQUIRE(object.isValid());

  REQUIRE_NOTHROW(continuing->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(continuing->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(continuing->disconnect());
}
} // namespace
