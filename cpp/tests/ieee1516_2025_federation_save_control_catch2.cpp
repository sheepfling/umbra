#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded federation save control coordinates initiation, status, completion, and abort",
    "[integration][development-profile][federation-management][save-restore]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.federate-save-not-complete]"
    "[rti.service.abort-federation-save][rti.service.query-federation-save-status]"
    "[federate.callback.initiate-federate-save][federate.callback.federation-saved]"
    "[federate.callback.federation-not-saved][federate.callback.federation-save-status-response]"
    "[federation-save-control]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador peerReports;
  auto owner = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(peer->connect(peerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle ownerHandle;
  FederateHandle peerHandle;
  REQUIRE_NOTHROW(
      ownerHandle = owner->joinFederationExecution(L"save-owner", L"owner", federationName));
  REQUIRE_NOTHROW(
      peerHandle = peer->joinFederationExecution(L"save-peer", L"observer", federationName));

  REQUIRE_NOTHROW(owner->requestFederationSave(L"control-save-1"));
  REQUIRE(ownerReports.initiateFederateSaveReports == std::vector<std::wstring>{L"control-save-1"});
  REQUIRE(peerReports.initiateFederateSaveReports == std::vector<std::wstring>{L"control-save-1"});
  REQUIRE_THROWS_AS(owner->requestFederationSave(L"overlapping"), rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(owner->federateSaveComplete(), rti1516_2025::FederateHasNotBegunSave);

  REQUIRE_NOTHROW(owner->queryFederationSaveStatus());
  REQUIRE(ownerReports.federationSaveStatusReports.size() == 1);
  REQUIRE(ownerReports.federationSaveStatusReports.back().statuses.size() == 2);
  REQUIRE(ownerReports.federationSaveStatusReports.back().statuses[0].first == ownerHandle);
  REQUIRE(ownerReports.federationSaveStatusReports.back().statuses[0].second ==
          rti1516_2025::FEDERATE_INSTRUCTED_TO_SAVE);
  REQUIRE(ownerReports.federationSaveStatusReports.back().statuses[1].first == peerHandle);
  REQUIRE(ownerReports.federationSaveStatusReports.back().statuses[1].second ==
          rti1516_2025::FEDERATE_INSTRUCTED_TO_SAVE);

  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(peer->federateSaveBegun());
  REQUIRE_NOTHROW(owner->queryFederationSaveStatus());
  REQUIRE(ownerReports.federationSaveStatusReports.back().statuses[0].second ==
          rti1516_2025::FEDERATE_SAVING);
  REQUIRE(ownerReports.federationSaveStatusReports.back().statuses[1].second ==
          rti1516_2025::FEDERATE_SAVING);

  REQUIRE_NOTHROW(owner->federateSaveComplete());
  REQUIRE(ownerReports.federationSavedReportCount == 0);
  REQUIRE(peerReports.federationSavedReportCount == 0);
  REQUIRE_NOTHROW(peer->federateSaveComplete());
  REQUIRE(ownerReports.federationSavedReportCount == 1);
  REQUIRE(peerReports.federationSavedReportCount == 1);

  REQUIRE_NOTHROW(owner->queryFederationSaveStatus());
  REQUIRE(ownerReports.federationSaveStatusReports.back().statuses.size() == 2);
  REQUIRE(ownerReports.federationSaveStatusReports.back().statuses[0].second ==
          rti1516_2025::NO_SAVE_IN_PROGRESS);
  REQUIRE(ownerReports.federationSaveStatusReports.back().statuses[1].second ==
          rti1516_2025::NO_SAVE_IN_PROGRESS);

  REQUIRE_NOTHROW(owner->requestFederationSave(L"control-save-2"));
  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(peer->federateSaveBegun());
  REQUIRE_NOTHROW(peer->federateSaveNotComplete());
  REQUIRE(ownerReports.federationNotSavedReasons.back() ==
          rti1516_2025::FEDERATE_REPORTED_FAILURE_DURING_SAVE);
  REQUIRE(peerReports.federationNotSavedReasons.back() ==
          rti1516_2025::FEDERATE_REPORTED_FAILURE_DURING_SAVE);

  REQUIRE_NOTHROW(owner->requestFederationSave(L"control-save-3"));
  REQUIRE_NOTHROW(owner->abortFederationSave());
  REQUIRE(ownerReports.federationNotSavedReasons.back() == rti1516_2025::SAVE_ABORTED);
  REQUIRE(peerReports.federationNotSavedReasons.back() == rti1516_2025::SAVE_ABORTED);
  REQUIRE_THROWS_AS(owner->abortFederationSave(), rti1516_2025::SaveNotInProgress);

  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(peer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}
} // namespace
