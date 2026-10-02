#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

TEST_CASE(
    "Embedded support switches are seeded per federate and retain static FDD policy",
    "[integration][development-profile][federation-management][fom][switches][support-switches]"
    "[rti.service.get-convey-region-designator-sets-switch]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.get-automatic-resign-directive]"
    "[rti.service.set-automatic-resign-directive]"
    "[rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-exception-reporting-switch]"
    "[rti.service.set-exception-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-delay-subscription-evaluation-switch]"
    "[rti.service.get-allow-relaxed-ddm-switch]"
    "[support-switch-state][2025]") {
  TestFederateAmbassador ownerReports;
  TestFederateAmbassador peerReports;
  auto owner = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const supportFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(peer->connect(peerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, supportFom, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"support-owner", L"owner", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(
      L"support-peer", L"peer", federationName));

  // The explicit FDD settings seed each new member independently.
  REQUIRE(owner->getConveyRegionDesignatorSetsSwitch());
  REQUIRE(peer->getConveyRegionDesignatorSetsSwitch());
  REQUIRE(owner->getAutomaticResignDirective() == rti1516_2025::DELETE_OBJECTS_THEN_DIVEST);
  REQUIRE(peer->getAutomaticResignDirective() == rti1516_2025::DELETE_OBJECTS_THEN_DIVEST);
  REQUIRE(owner->getServiceReportingSwitch());
  REQUIRE(owner->getExceptionReportingSwitch());
  REQUIRE(owner->getSendServiceReportsToFileSwitch());
  REQUIRE(peer->getServiceReportingSwitch());
  REQUIRE(peer->getExceptionReportingSwitch());
  REQUIRE(peer->getSendServiceReportsToFileSwitch());

  // Static switches are seeded at federation creation and are visible to all
  // members; this profile exposes no setter for either official API.
  REQUIRE(owner->getDelaySubscriptionEvaluationSwitch());
  REQUIRE(peer->getDelaySubscriptionEvaluationSwitch());
  REQUIRE(owner->getAllowRelaxedDDMSwitch());
  REQUIRE(peer->getAllowRelaxedDDMSwitch());

  // Per-federate setters do not leak into the peer's membership state.
  REQUIRE_NOTHROW(owner->setConveyRegionDesignatorSetsSwitch(false));
  REQUIRE_NOTHROW(owner->setAutomaticResignDirective(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setExceptionReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  REQUIRE_FALSE(owner->getConveyRegionDesignatorSetsSwitch());
  REQUIRE(owner->getAutomaticResignDirective() ==
          rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST);
  REQUIRE_FALSE(owner->getServiceReportingSwitch());
  REQUIRE_FALSE(owner->getExceptionReportingSwitch());
  REQUIRE_FALSE(owner->getSendServiceReportsToFileSwitch());
  REQUIRE(peer->getConveyRegionDesignatorSetsSwitch());
  REQUIRE(peer->getAutomaticResignDirective() == rti1516_2025::DELETE_OBJECTS_THEN_DIVEST);
  REQUIRE(peer->getServiceReportingSwitch());
  REQUIRE(peer->getExceptionReportingSwitch());
  REQUIRE(peer->getSendServiceReportsToFileSwitch());

  REQUIRE_THROWS_AS(
      owner->setAutomaticResignDirective(static_cast<rti1516_2025::ResignAction>(99)),
      rti1516_2025::InvalidResignAction);

  REQUIRE_NOTHROW(peer->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}
