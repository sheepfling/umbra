#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded timed federation save can begin at an ordinary boundary before Flush Queue",
    "[integration][development-profile][federation-management][save-restore][time-management]"
    "[timed-save][multi-federate-callback-ordering][timed-save-mixed-fqr-tar-members]"
    "[rti.service.request-federation-save][rti.service.time-advance-request]"
    "[rti.service.flush-queue-request][rti.service.enable-time-constrained]"
    "[rti.service.enable-time-regulation][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][federate.callback.initiate-federate-save]"
    "[federate.callback.time-advance-grant][federate.callback.flush-queue-grant]"
    "[federate.callback.federation-saved]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador ordinaryReports;
  ReportingFederateAmbassador flushQueueReports;
  auto owner = makeRti();
  auto ordinary = makeRti();
  auto flushQueue = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  auto const saveLabel = std::wstring{L"ordinary-before-flush-queue-save"};

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(ordinary->connect(ordinaryReports, HLA_EVOKED));
  REQUIRE_NOTHROW(flushQueue->connect(flushQueueReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"ordinary-before-flush-queue-owner", L"regulator", federationName));
  REQUIRE_NOTHROW(ordinary->joinFederationExecution(
      L"ordinary-before-flush-queue-tar", L"time-constrained", federationName));
  REQUIRE_NOTHROW(flushQueue->joinFederationExecution(
      L"ordinary-before-flush-queue-fqr", L"time-constrained", federationName));

  REQUIRE_NOTHROW(ordinary->enableTimeConstrained());
  REQUIRE_FALSE(ordinary->evokeCallback(0.0));
  REQUIRE_NOTHROW(flushQueue->enableTimeConstrained());
  REQUIRE_FALSE(flushQueue->evokeCallback(0.0));
  REQUIRE_NOTHROW(owner->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));

  REQUIRE_NOTHROW(owner->requestFederationSave(
      saveLabel,
      rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(flushQueue->flushQueueRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(ordinary->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));

  // The TAR callback starts the operation only because the registry has
  // precomputed that the pending FQR will also have actual grant 6 > 5.
  REQUIRE_FALSE(ordinary->evokeCallback(0.0));
  REQUIRE(ordinaryReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(ordinaryReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(ordinaryReports.timeAdvanceGrantReports.back().value == L"5");
  REQUIRE(ordinaryReports.callbackOrder == std::vector<std::string>{"save-initiate", "grant"});
  REQUIRE(flushQueueReports.initiateFederateSaveReports.empty());
  REQUIRE(flushQueueReports.callbackOrder.empty());
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());

  REQUIRE_FALSE(flushQueue->evokeCallback(0.0));
  REQUIRE(flushQueueReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(flushQueueReports.flushQueueGrantReports.size() == 1);
  REQUIRE(flushQueueReports.flushQueueGrantReports.back().value == L"6");
  REQUIRE(flushQueueReports.callbackOrder == std::vector<std::string>{"save-initiate", "flush-grant"});
  REQUIRE(ordinaryReports.callbackOrder == std::vector<std::string>{"save-initiate", "grant"});
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());

  while (owner->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});

  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(ordinary->federateSaveBegun());
  REQUIRE_NOTHROW(flushQueue->federateSaveBegun());
  REQUIRE_NOTHROW(owner->federateSaveComplete());
  REQUIRE_NOTHROW(ordinary->federateSaveComplete());
  REQUIRE_NOTHROW(flushQueue->federateSaveComplete());
  while (owner->evokeCallback(0.0)) {
  }
  while (ordinary->evokeCallback(0.0)) {
  }
  while (flushQueue->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.federationSavedReportCount == 1);
  REQUIRE(ordinaryReports.federationSavedReportCount == 1);
  REQUIRE(flushQueueReports.federationSavedReportCount == 1);

  REQUIRE_NOTHROW(flushQueue->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(ordinary->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(flushQueue->disconnect());
  REQUIRE_NOTHROW(ordinary->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}
}
