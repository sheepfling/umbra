#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded final-federate resignation applies directive two regardless of the supplied action",
    "[integration][development-profile][federation-management][object-management]"
    "[resign-action-final-federate]"
    "[rti.service.resign-federation-execution][federate.callback.object-instance-name-reservation-succeeded]") {
  ReportingFederateAmbassador ownerReports;
  auto owner = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"last-federate", L"publisher", federationName));

  auto const server = owner->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = owner->getAttributeHandle(server, fixture_hla::fixture::efficiency);
  AttributeHandleSet const efficiencyOnly{efficiency};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, efficiencyOnly));

  ObjectInstanceHandle originalObject;
  REQUIRE_NOTHROW(originalObject = owner->registerObjectInstance(server));
  auto const reusableObjectName = owner->getObjectInstanceName(originalObject);

  // 4.12.4 forces the delete-object pass for the last joined federate even
  // when the caller supplies NO_ACTION.  The deleted name must be reusable
  // after a new member joins the still-existing federation execution.
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"rejoined-federate", L"publisher", federationName));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(owner->reserveObjectInstanceName(reusableObjectName));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.objectInstanceNameReservationSucceededReports.size() == 1);
  REQUIRE(
      ownerReports.objectInstanceNameReservationSucceededReports.front().objectInstanceName ==
      reusableObjectName);

  ObjectInstanceHandle replacementObject;
  REQUIRE_NOTHROW(
      replacementObject = owner->registerObjectInstance(server, reusableObjectName));
  REQUIRE(replacementObject.isValid());
  REQUIRE(replacementObject != originalObject);

  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
}
} // namespace
