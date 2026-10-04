#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded timed federation save admits mixed Flush Queue and ordinary constrained members",
    "[integration][development-profile][federation-management][save-restore][time-management]"
    "[timed-save][multi-federate-callback-ordering][timed-save-mixed-fqr-tar-members]"
    "[rti.service.request-federation-save][rti.service.time-advance-request]"
    "[rti.service.flush-queue-request][rti.service.enable-time-constrained]"
    "[rti.service.enable-time-regulation][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][federate.callback.initiate-federate-save]"
    "[federate.callback.time-advance-grant][federate.callback.flush-queue-grant]"
    "[federate.callback.federation-saved]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador firstReports;
  ReportingFederateAmbassador secondReports;
  ReportingFederateAmbassador thirdReports;
  auto owner = makeRti();
  auto first = makeRti();
  auto second = makeRti();
  auto third = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  auto const saveLabel = std::wstring{L"mixed-flush-queue-save"};

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(first->connect(firstReports, HLA_EVOKED));
  REQUIRE_NOTHROW(second->connect(secondReports, HLA_EVOKED));
  REQUIRE_NOTHROW(third->connect(thirdReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"mixed-flush-queue-owner", L"regulator", federationName));
  REQUIRE_NOTHROW(first->joinFederationExecution(
      L"mixed-flush-queue-first", L"time-constrained", federationName));
  REQUIRE_NOTHROW(second->joinFederationExecution(
      L"mixed-flush-queue-second", L"time-constrained", federationName));
  REQUIRE_NOTHROW(third->joinFederationExecution(
      L"mixed-flush-queue-third", L"time-constrained", federationName));

  REQUIRE_NOTHROW(first->enableTimeConstrained());
  REQUIRE_FALSE(first->evokeCallback(0.0));
  REQUIRE_NOTHROW(second->enableTimeConstrained());
  REQUIRE_FALSE(second->evokeCallback(0.0));
  REQUIRE_NOTHROW(third->enableTimeConstrained());
  REQUIRE_FALSE(third->evokeCallback(0.0));
  REQUIRE_NOTHROW(owner->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));

  REQUIRE_NOTHROW(owner->requestFederationSave(
      saveLabel,
      rti1516_2025::HLAinteger64Time(5)));
  // The regulator's pending request produces GALT 6. Both FQRs therefore
  // have actual grants strictly beyond the save time, while TAR reaches the
  // inclusive boundary at 5. All three requests must be known before the
  // first direct admission starts the save operation.
  REQUIRE_NOTHROW(first->flushQueueRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(second->flushQueueRequest(rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_NOTHROW(third->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));

  REQUIRE_FALSE(first->evokeCallback(0.0));
  REQUIRE(firstReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(firstReports.flushQueueGrantReports.size() == 1);
  REQUIRE(firstReports.flushQueueGrantReports.back().value == L"6");
  REQUIRE(firstReports.callbackOrder == std::vector<std::string>{"save-initiate", "flush-grant"});
  REQUIRE(secondReports.initiateFederateSaveReports.empty());
  REQUIRE(secondReports.callbackOrder.empty());
  REQUIRE(thirdReports.initiateFederateSaveReports.empty());
  REQUIRE(thirdReports.callbackOrder.empty());
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());
  REQUIRE(ownerReports.callbackOrder.empty());

  REQUIRE_FALSE(second->evokeCallback(0.0));
  REQUIRE(secondReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(secondReports.flushQueueGrantReports.size() == 1);
  REQUIRE(secondReports.flushQueueGrantReports.back().value == L"6");
  REQUIRE(secondReports.callbackOrder == std::vector<std::string>{"save-initiate", "flush-grant"});
  REQUIRE(thirdReports.initiateFederateSaveReports.empty());
  REQUIRE(thirdReports.callbackOrder.empty());
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());
  REQUIRE(ownerReports.callbackOrder.empty());

  REQUIRE_FALSE(third->evokeCallback(0.0));
  REQUIRE(thirdReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(thirdReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(thirdReports.timeAdvanceGrantReports.back().value == L"5");
  REQUIRE(thirdReports.callbackOrder == std::vector<std::string>{"save-initiate", "grant"});
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());
  REQUIRE(ownerReports.callbackOrder.empty());

  while (owner->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});

  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(first->federateSaveBegun());
  REQUIRE_NOTHROW(second->federateSaveBegun());
  REQUIRE_NOTHROW(third->federateSaveBegun());
  REQUIRE_NOTHROW(owner->federateSaveComplete());
  REQUIRE_NOTHROW(first->federateSaveComplete());
  REQUIRE_NOTHROW(second->federateSaveComplete());
  REQUIRE_NOTHROW(third->federateSaveComplete());
  while (owner->evokeCallback(0.0)) {
  }
  while (first->evokeCallback(0.0)) {
  }
  while (second->evokeCallback(0.0)) {
  }
  while (third->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.federationSavedReportCount == 1);
  REQUIRE(firstReports.federationSavedReportCount == 1);
  REQUIRE(secondReports.federationSavedReportCount == 1);
  REQUIRE(thirdReports.federationSavedReportCount == 1);

  REQUIRE_NOTHROW(third->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(second->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(first->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(third->disconnect());
  REQUIRE_NOTHROW(second->disconnect());
  REQUIRE_NOTHROW(first->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}
}
