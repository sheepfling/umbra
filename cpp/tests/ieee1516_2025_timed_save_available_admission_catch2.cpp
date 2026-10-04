#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded timed federation save uses an exclusive available-advance boundary",
    "[integration][development-profile][federation-management][save-restore][time-management]"
    "[timed-save][multi-federate-callback-ordering]"
    "[rti.service.request-federation-save][rti.service.time-advance-request]"
    "[rti.service.time-advance-request-available][rti.service.enable-time-constrained]"
    "[rti.service.enable-time-regulation][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][federate.callback.initiate-federate-save]"
    "[federate.callback.time-advance-grant][federate.callback.federation-saved]"
    "[timed-save-available-advance-boundary]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador receiverReports;
  auto owner = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  auto const saveLabel = std::wstring{L"available-time-advance-save"};

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"available-save-owner", L"regulator", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"available-save-receiver", L"time-constrained", federationName));

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(owner->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));

  ownerReports.callbackOrder.clear();
  receiverReports.callbackOrder.clear();
  REQUIRE_NOTHROW(owner->requestFederationSave(
      saveLabel,
      rti1516_2025::HLAinteger64Time(5)));

  // TARA's next grant at exactly the save timestamp is not a qualifying
  // boundary. The save request remains pending after the recipient is granted
  // time 5 without an Initiate Federate Save callback.
  REQUIRE_NOTHROW(receiver->timeAdvanceRequestAvailable(
      rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(4)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(receiverReports.timeAdvanceGrantReports.back().value == L"5");
  REQUIRE(receiverReports.initiateFederateSaveReports.empty());
  REQUIRE(receiverReports.callbackOrder == std::vector<std::string>{"grant"});
  REQUIRE(ownerReports.callbackOrder == std::vector<std::string>{"grant"});

  // Once the available request's next grant is strictly greater than the
  // scheduled save time, it is a qualifying boundary and direct initiation
  // precedes the grant.
  REQUIRE_NOTHROW(receiver->timeAdvanceRequestAvailable(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 2);
  REQUIRE(receiverReports.timeAdvanceGrantReports.back().value == L"6");
  REQUIRE(
      receiverReports.callbackOrder ==
      std::vector<std::string>{"grant", "save-initiate", "grant"});
  REQUIRE(ownerReports.callbackOrder == std::vector<std::string>{"grant", "grant"});

  static_cast<void>(owner->evokeCallback(0.0));
  REQUIRE(ownerReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(ownerReports.callbackOrder ==
          std::vector<std::string>{"grant", "grant", "save-initiate"});

  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(receiver->federateSaveBegun());
  REQUIRE_NOTHROW(owner->federateSaveComplete());
  REQUIRE_NOTHROW(receiver->federateSaveComplete());
  while (owner->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.federationSavedReportCount == 1);
  REQUIRE(receiverReports.federationSavedReportCount == 1);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}
} // namespace
