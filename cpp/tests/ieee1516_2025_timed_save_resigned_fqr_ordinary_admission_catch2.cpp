#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded timed federation save drops a resigned FQR before ordinary admission",
    "[integration][development-profile][federation-management][save-restore][time-management]"
    "[timed-save][multi-federate-callback-ordering][resignation]"
    "[timed-save-resigned-fqr-ordinary-admission]"
    "[rti.service.request-federation-save][rti.service.resign-federation-execution]"
    "[rti.service.time-advance-request][rti.service.enable-time-constrained]"
    "[rti.service.enable-time-regulation][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][federate.callback.initiate-federate-save]"
    "[federate.callback.time-advance-grant][federate.callback.federation-saved]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador ordinaryReports;
  ReportingFederateAmbassador departingReports;
  auto owner = makeRti();
  auto ordinary = makeRti();
  auto departing = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  auto const saveLabel = std::wstring{L"resigned-fqr-save"};

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(ordinary->connect(ordinaryReports, HLA_EVOKED));
  REQUIRE_NOTHROW(departing->connect(departingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"resigned-fqr-owner", L"regulator", federationName));
  REQUIRE_NOTHROW(ordinary->joinFederationExecution(
      L"resigned-fqr-ordinary", L"time-constrained", federationName));
  REQUIRE_NOTHROW(departing->joinFederationExecution(
      L"resigned-fqr-departing", L"time-constrained", federationName));

  REQUIRE_NOTHROW(ordinary->enableTimeConstrained());
  REQUIRE_FALSE(ordinary->evokeCallback(0.0));
  REQUIRE_NOTHROW(departing->enableTimeConstrained());
  REQUIRE_FALSE(departing->evokeCallback(0.0));
  REQUIRE_NOTHROW(owner->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));

  ownerReports.callbackOrder.clear();
  ordinaryReports.callbackOrder.clear();
  departingReports.callbackOrder.clear();
  REQUIRE_NOTHROW(owner->requestFederationSave(
      saveLabel,
      rti1516_2025::HLAinteger64Time(5)));

  // A resigned constrained member must no longer gate the pending save. The
  // remaining ordinary TAR member can therefore be admitted at time 5.
  REQUIRE_NOTHROW(departing->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(ordinary->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));

  REQUIRE_FALSE(ordinary->evokeCallback(0.0));
  REQUIRE(ordinaryReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(ordinaryReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(ordinaryReports.timeAdvanceGrantReports.back().value == L"5");
  REQUIRE(ordinaryReports.callbackOrder == std::vector<std::string>{"save-initiate", "grant"});
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());
  REQUIRE(departingReports.initiateFederateSaveReports.empty());
  REQUIRE(departingReports.callbackOrder.empty());

  while (owner->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});

  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(ordinary->federateSaveBegun());
  REQUIRE_NOTHROW(owner->federateSaveComplete());
  REQUIRE_NOTHROW(ordinary->federateSaveComplete());
  while (owner->evokeCallback(0.0)) {
  }
  while (ordinary->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.federationSavedReportCount == 1);
  REQUIRE(ordinaryReports.federationSavedReportCount == 1);
  REQUIRE(departingReports.federationSavedReportCount == 0);

  REQUIRE_NOTHROW(ordinary->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(departing->disconnect());
  REQUIRE_NOTHROW(ordinary->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}
} // namespace
