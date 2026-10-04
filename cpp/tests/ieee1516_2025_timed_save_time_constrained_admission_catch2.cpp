#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded timed federation save admits every constrained member before non-constrained members",
    "[integration][development-profile][federation-management][save-restore][time-management]"
    "[timed-save][multi-federate-callback-ordering]"
    "[rti.service.request-federation-save][rti.service.time-advance-request]"
    "[rti.service.enable-time-constrained][rti.service.enable-time-regulation]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[federate.callback.initiate-federate-save][federate.callback.time-advance-grant]"
    "[federate.callback.federation-saved]"
    "[timed-save-time-constrained-admission]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador firstReports;
  ReportingFederateAmbassador secondReports;
  auto owner = makeRti();
  auto first = makeRti();
  auto second = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  auto const saveLabel = std::wstring{L"timed-time-advance-save"};

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(first->connect(firstReports, HLA_EVOKED));
  REQUIRE_NOTHROW(second->connect(secondReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"timed-save-owner", L"non-constrained-regulator", federationName));
  REQUIRE_NOTHROW(first->joinFederationExecution(
      L"timed-save-first", L"time-constrained", federationName));
  REQUIRE_NOTHROW(second->joinFederationExecution(
      L"timed-save-second", L"time-constrained", federationName));

  REQUIRE_NOTHROW(first->enableTimeConstrained());
  REQUIRE_FALSE(first->evokeCallback(0.0));
  REQUIRE_NOTHROW(second->enableTimeConstrained());
  REQUIRE_FALSE(second->evokeCallback(0.0));
  REQUIRE_NOTHROW(owner->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));

  ownerReports.callbackOrder.clear();
  firstReports.callbackOrder.clear();
  secondReports.callbackOrder.clear();
  REQUIRE_NOTHROW(owner->requestFederationSave(
      saveLabel,
      rti1516_2025::HLAinteger64Time(5)));
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());
  REQUIRE(firstReports.initiateFederateSaveReports.empty());
  REQUIRE(secondReports.initiateFederateSaveReports.empty());

  // TAR reaches the timestamp inclusively. Both constrained federates must
  // already be Time Advancing before either direct pre-grant initiation can
  // start; the ordinary regulator remains queued until the second one is
  // admitted at its own boundary.
  REQUIRE_NOTHROW(first->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(second->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  static_cast<void>(owner->evokeCallback(0.0));
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());
  REQUIRE(ownerReports.callbackOrder == std::vector<std::string>{"grant"});

  static_cast<void>(first->evokeCallback(0.0));
  REQUIRE(firstReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(firstReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(firstReports.timeAdvanceGrantReports.back().value == L"5");
  REQUIRE(firstReports.callbackOrder == std::vector<std::string>{"save-initiate", "grant"});
  REQUIRE(secondReports.initiateFederateSaveReports.empty());
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());
  REQUIRE(ownerReports.callbackOrder == std::vector<std::string>{"grant"});

  static_cast<void>(second->evokeCallback(0.0));
  REQUIRE(secondReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(secondReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(secondReports.timeAdvanceGrantReports.back().value == L"5");
  REQUIRE(secondReports.callbackOrder == std::vector<std::string>{"save-initiate", "grant"});
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());
  REQUIRE(ownerReports.callbackOrder == std::vector<std::string>{"grant"});

  static_cast<void>(owner->evokeCallback(0.0));
  REQUIRE(ownerReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(ownerReports.callbackOrder == std::vector<std::string>{"grant", "save-initiate"});

  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(first->federateSaveBegun());
  REQUIRE_NOTHROW(second->federateSaveBegun());
  REQUIRE_NOTHROW(owner->federateSaveComplete());
  REQUIRE_NOTHROW(first->federateSaveComplete());
  REQUIRE_NOTHROW(second->federateSaveComplete());
  while (owner->evokeCallback(0.0)) {
  }
  while (first->evokeCallback(0.0)) {
  }
  while (second->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.federationSavedReportCount == 1);
  REQUIRE(firstReports.federationSavedReportCount == 1);
  REQUIRE(secondReports.federationSavedReportCount == 1);

  REQUIRE_NOTHROW(second->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(first->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(second->disconnect());
  REQUIRE_NOTHROW(first->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}
} // namespace
