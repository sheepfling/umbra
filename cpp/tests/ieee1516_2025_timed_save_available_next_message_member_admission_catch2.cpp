#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded timed federation save admits Available and next-message constrained members",
    "[integration][development-profile][federation-management][save-restore][time-management]"
    "[timed-save][multi-federate-callback-ordering]"
    "[rti.service.request-federation-save][rti.service.time-advance-request]"
    "[rti.service.time-advance-request-available][rti.service.next-message-request-available]"
    "[rti.service.enable-time-constrained][rti.service.enable-time-regulation]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[federate.callback.initiate-federate-save][federate.callback.time-advance-grant]"
    "[federate.callback.federation-saved]"
    "[timed-save-available-next-message-member-admission]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador availableReports;
  ReportingFederateAmbassador nextMessageReports;
  auto owner = makeRti();
  auto available = makeRti();
  auto nextMessage = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  auto const saveLabel = std::wstring{L"available-next-message-save"};

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(available->connect(availableReports, HLA_EVOKED));
  REQUIRE_NOTHROW(nextMessage->connect(nextMessageReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"available-next-message-save-owner", L"regulator", federationName));
  REQUIRE_NOTHROW(available->joinFederationExecution(
      L"available-next-message-save-available", L"time-constrained", federationName));
  REQUIRE_NOTHROW(nextMessage->joinFederationExecution(
      L"available-next-message-save-next-message", L"time-constrained", federationName));

  REQUIRE_NOTHROW(available->enableTimeConstrained());
  REQUIRE_FALSE(available->evokeCallback(0.0));
  REQUIRE_NOTHROW(nextMessage->enableTimeConstrained());
  REQUIRE_FALSE(nextMessage->evokeCallback(0.0));
  REQUIRE_NOTHROW(owner->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));

  ownerReports.callbackOrder.clear();
  availableReports.callbackOrder.clear();
  nextMessageReports.callbackOrder.clear();
  REQUIRE_NOTHROW(owner->requestFederationSave(
      saveLabel,
      rti1516_2025::HLAinteger64Time(5)));

  // Both Available forms cross the strict clause-4.19 boundary at 6. The
  // save cannot start until each constrained member already has its own
  // queued grant, and the regulator must remain uninstructed until both
  // direct pre-grant callbacks have occurred.
  REQUIRE_NOTHROW(available->timeAdvanceRequestAvailable(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(nextMessage->nextMessageRequestAvailable(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));

  REQUIRE_FALSE(owner->evokeCallback(0.0));
  REQUIRE(ownerReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(ownerReports.timeAdvanceGrantReports.back().value == L"5");
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());

  REQUIRE_FALSE(available->evokeCallback(0.0));
  REQUIRE(availableReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{saveLabel});
  REQUIRE(availableReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(availableReports.timeAdvanceGrantReports.back().value == L"6");
  REQUIRE(availableReports.callbackOrder ==
          std::vector<std::string>{"save-initiate", "grant"});
  REQUIRE(nextMessageReports.initiateFederateSaveReports.empty());
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());
  REQUIRE(nextMessageReports.callbackOrder.empty());

  REQUIRE_FALSE(nextMessage->evokeCallback(0.0));
  REQUIRE(nextMessageReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{saveLabel});
  REQUIRE(nextMessageReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(nextMessageReports.timeAdvanceGrantReports.back().value == L"6");
  REQUIRE(nextMessageReports.callbackOrder ==
          std::vector<std::string>{"save-initiate", "grant"});
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());

  REQUIRE_FALSE(owner->evokeCallback(0.0));
  REQUIRE(ownerReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{saveLabel});
  REQUIRE(ownerReports.callbackOrder == std::vector<std::string>{"grant", "save-initiate"});

  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(available->federateSaveBegun());
  REQUIRE_NOTHROW(nextMessage->federateSaveBegun());
  REQUIRE_NOTHROW(owner->federateSaveComplete());
  REQUIRE_NOTHROW(available->federateSaveComplete());
  REQUIRE_NOTHROW(nextMessage->federateSaveComplete());
  while (owner->evokeCallback(0.0)) {
  }
  while (available->evokeCallback(0.0)) {
  }
  while (nextMessage->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.federationSavedReportCount == 1);
  REQUIRE(availableReports.federationSavedReportCount == 1);
  REQUIRE(nextMessageReports.federationSavedReportCount == 1);

  REQUIRE_NOTHROW(nextMessage->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(available->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(nextMessage->disconnect());
  REQUIRE_NOTHROW(available->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}
} // namespace
