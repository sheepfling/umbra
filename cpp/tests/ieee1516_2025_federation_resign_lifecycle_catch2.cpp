#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded federation-management services commit only a prevalidated federation definition",
    "[integration][development-profile][federation-management][rti.service.create-federation-execution]"
    "[rti.service.destroy-federation-execution][rti.service.join-federation-execution]"
    "[rti.service.resign-federation-execution][m16.transition.federate-resign]") {
  TestFederateAmbassador creatorFederate;
  TestFederateAmbassador joiningFederate;
  TestFederateAmbassador duplicateNameFederate;
  auto creator = makeRti();
  auto joiner = makeRti();
  auto duplicateName = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(creator->connect(creatorFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(joiner->connect(joiningFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(duplicateName->connect(duplicateNameFederate, HLA_EVOKED));

  SECTION("Create rejects unresolved logical-time or standard-MIM designators") {
    REQUIRE_THROWS_AS(
        creator->createFederationExecution(federationName, fomModule),
        rti1516_2025::InconsistentFOM);
    REQUIRE_THROWS_AS(
        creator->createFederationExecution(federationName, fomModule, L"ExampleCustomTime"),
        rti1516_2025::CouldNotCreateLogicalTimeFactory);
    REQUIRE_THROWS_AS(
        creator->createFederationExecutionWithMIM(
            federationName,
            std::vector<std::wstring>{fomModule},
            standard_hla::mom::standard_mim,
            standard_hla::mom::integer64_time),
        rti1516_2025::DesignatorIsHLAstandardMIM);
  }

  REQUIRE_NOTHROW(
      creator->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_THROWS_AS(
      creator->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time),
      rti1516_2025::FederationExecutionAlreadyExists);

  FederateHandle creatorHandle;
  REQUIRE_NOTHROW(
      creatorHandle = creator->joinFederationExecution(L"creator", L"owner", federationName));
  REQUIRE(creatorHandle.isValid());
  REQUIRE_THROWS_AS(
      creator->joinFederationExecution(L"creator", federationName),
      rti1516_2025::FederateAlreadyExecutionMember);
  REQUIRE_THROWS_AS(creator->disconnect(), rti1516_2025::FederateIsExecutionMember);

  FederateHandle joinerHandle;
  REQUIRE_NOTHROW(
      joinerHandle = joiner->joinFederationExecution(
          L"joiner",
          L"observer",
          federationName,
          std::vector<std::wstring>{fomModule}));
  REQUIRE(joinerHandle.isValid());
  REQUIRE(creatorHandle != joinerHandle);
  REQUIRE_THROWS_AS(
      duplicateName->joinFederationExecution(L"creator", L"observer", federationName),
      rti1516_2025::FederateNameAlreadyInUse);
  REQUIRE_THROWS_AS(
      creator->destroyFederationExecution(federationName),
      rti1516_2025::FederatesCurrentlyJoined);

  REQUIRE_THROWS_AS(
      creator->resignFederationExecution(static_cast<ResignAction>(-1)),
      rti1516_2025::InvalidResignAction);
  REQUIRE_NOTHROW(creator->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(joiner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(creator->destroyFederationExecution(federationName));

  REQUIRE_NOTHROW(creator->disconnect());
  REQUIRE_NOTHROW(joiner->disconnect());
  REQUIRE_NOTHROW(duplicateName->disconnect());
}
}  // namespace
