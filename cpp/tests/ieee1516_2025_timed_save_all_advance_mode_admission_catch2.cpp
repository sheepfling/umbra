#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded timed federation save admits every advance mode before non-constrained members",
    "[integration][development-profile][federation-management][save-restore][time-management]"
    "[timed-save][multi-federate-callback-ordering]"
    "[rti.service.request-federation-save][rti.service.time-advance-request]"
    "[rti.service.next-message-request][rti.service.time-advance-request-available]"
    "[rti.service.next-message-request-available][rti.service.flush-queue-request]"
    "[rti.service.enable-time-constrained][rti.service.enable-time-regulation]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[federate.callback.initiate-federate-save][federate.callback.time-advance-grant]"
    "[federate.callback.flush-queue-grant][federate.callback.federation-saved]"
    "[timed-save-all-advance-mode-admission]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador tarReports;
  ReportingFederateAmbassador nmrReports;
  ReportingFederateAmbassador taraReports;
  ReportingFederateAmbassador nmraReports;
  ReportingFederateAmbassador fqrReports;
  auto owner = makeRti();
  auto tar = makeRti();
  auto nmr = makeRti();
  auto tara = makeRti();
  auto nmra = makeRti();
  auto fqr = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  auto const saveLabel = std::wstring{L"all-advance-modes-save"};

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(tar->connect(tarReports, HLA_EVOKED));
  REQUIRE_NOTHROW(nmr->connect(nmrReports, HLA_EVOKED));
  REQUIRE_NOTHROW(tara->connect(taraReports, HLA_EVOKED));
  REQUIRE_NOTHROW(nmra->connect(nmraReports, HLA_EVOKED));
  REQUIRE_NOTHROW(fqr->connect(fqrReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"all-advance-modes-owner", L"regulator", federationName));
  REQUIRE_NOTHROW(tar->joinFederationExecution(
      L"all-advance-modes-tar", L"time-constrained", federationName));
  REQUIRE_NOTHROW(nmr->joinFederationExecution(
      L"all-advance-modes-nmr", L"time-constrained", federationName));
  REQUIRE_NOTHROW(tara->joinFederationExecution(
      L"all-advance-modes-tara", L"time-constrained", federationName));
  REQUIRE_NOTHROW(nmra->joinFederationExecution(
      L"all-advance-modes-nmra", L"time-constrained", federationName));
  REQUIRE_NOTHROW(fqr->joinFederationExecution(
      L"all-advance-modes-fqr", L"time-constrained", federationName));

  REQUIRE_NOTHROW(tar->enableTimeConstrained());
  REQUIRE_FALSE(tar->evokeCallback(0.0));
  REQUIRE_NOTHROW(nmr->enableTimeConstrained());
  REQUIRE_FALSE(nmr->evokeCallback(0.0));
  REQUIRE_NOTHROW(tara->enableTimeConstrained());
  REQUIRE_FALSE(tara->evokeCallback(0.0));
  REQUIRE_NOTHROW(nmra->enableTimeConstrained());
  REQUIRE_FALSE(nmra->evokeCallback(0.0));
  REQUIRE_NOTHROW(fqr->enableTimeConstrained());
  REQUIRE_FALSE(fqr->evokeCallback(0.0));
  REQUIRE_NOTHROW(owner->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));

  REQUIRE_NOTHROW(owner->requestFederationSave(
      saveLabel,
      rti1516_2025::HLAinteger64Time(5)));
  // TAR and NMR are inclusive at 5. TARA, NMRA, and FQR are strict, so each
  // uses the known value 6; FQR derives that same actual grant from the
  // regulator's pending request. The first TAR callback may begin the save
  // only because every other constrained member is already dispatchable.
  REQUIRE_NOTHROW(tar->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(nmr->nextMessageRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(tara->timeAdvanceRequestAvailable(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(nmra->nextMessageRequestAvailable(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(fqr->flushQueueRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));

  REQUIRE_FALSE(tar->evokeCallback(0.0));
  REQUIRE(tarReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(tarReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(tarReports.timeAdvanceGrantReports.back().value == L"5");
  REQUIRE(tarReports.callbackOrder == std::vector<std::string>{"save-initiate", "grant"});
  REQUIRE(nmrReports.initiateFederateSaveReports.empty());
  REQUIRE(nmrReports.callbackOrder.empty());
  REQUIRE(taraReports.initiateFederateSaveReports.empty());
  REQUIRE(taraReports.callbackOrder.empty());
  REQUIRE(nmraReports.initiateFederateSaveReports.empty());
  REQUIRE(nmraReports.callbackOrder.empty());
  REQUIRE(fqrReports.initiateFederateSaveReports.empty());
  REQUIRE(fqrReports.callbackOrder.empty());
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());
  REQUIRE(ownerReports.callbackOrder.empty());

  REQUIRE_FALSE(nmr->evokeCallback(0.0));
  REQUIRE(nmrReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(nmrReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(nmrReports.timeAdvanceGrantReports.back().value == L"5");
  REQUIRE(nmrReports.callbackOrder == std::vector<std::string>{"save-initiate", "grant"});
  REQUIRE(taraReports.callbackOrder.empty());
  REQUIRE(nmraReports.callbackOrder.empty());
  REQUIRE(fqrReports.callbackOrder.empty());
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());
  REQUIRE(ownerReports.callbackOrder.empty());

  REQUIRE_FALSE(tara->evokeCallback(0.0));
  REQUIRE(taraReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(taraReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(taraReports.timeAdvanceGrantReports.back().value == L"6");
  REQUIRE(taraReports.callbackOrder == std::vector<std::string>{"save-initiate", "grant"});
  REQUIRE(nmraReports.callbackOrder.empty());
  REQUIRE(fqrReports.callbackOrder.empty());
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());
  REQUIRE(ownerReports.callbackOrder.empty());

  REQUIRE_FALSE(nmra->evokeCallback(0.0));
  REQUIRE(nmraReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(nmraReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(nmraReports.timeAdvanceGrantReports.back().value == L"6");
  REQUIRE(nmraReports.callbackOrder == std::vector<std::string>{"save-initiate", "grant"});
  REQUIRE(fqrReports.callbackOrder.empty());
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());
  REQUIRE(ownerReports.callbackOrder.empty());

  REQUIRE_FALSE(fqr->evokeCallback(0.0));
  REQUIRE(fqrReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(fqrReports.flushQueueGrantReports.size() == 1);
  REQUIRE(fqrReports.flushQueueGrantReports.back().value == L"6");
  REQUIRE(fqrReports.callbackOrder == std::vector<std::string>{"save-initiate", "flush-grant"});
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());
  REQUIRE(ownerReports.callbackOrder.empty());

  REQUIRE(owner->evokeCallback(0.0));
  REQUIRE_FALSE(owner->evokeCallback(0.0));
  REQUIRE(ownerReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(ownerReports.timeAdvanceGrantReports.back().value == L"5");
  REQUIRE(ownerReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(ownerReports.callbackOrder == std::vector<std::string>{"grant", "save-initiate"});

  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(tar->federateSaveBegun());
  REQUIRE_NOTHROW(nmr->federateSaveBegun());
  REQUIRE_NOTHROW(tara->federateSaveBegun());
  REQUIRE_NOTHROW(nmra->federateSaveBegun());
  REQUIRE_NOTHROW(fqr->federateSaveBegun());
  REQUIRE_NOTHROW(owner->federateSaveComplete());
  REQUIRE_NOTHROW(tar->federateSaveComplete());
  REQUIRE_NOTHROW(nmr->federateSaveComplete());
  REQUIRE_NOTHROW(tara->federateSaveComplete());
  REQUIRE_NOTHROW(nmra->federateSaveComplete());
  REQUIRE_NOTHROW(fqr->federateSaveComplete());
  while (owner->evokeCallback(0.0)) {
  }
  while (tar->evokeCallback(0.0)) {
  }
  while (nmr->evokeCallback(0.0)) {
  }
  while (tara->evokeCallback(0.0)) {
  }
  while (nmra->evokeCallback(0.0)) {
  }
  while (fqr->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.federationSavedReportCount == 1);
  REQUIRE(tarReports.federationSavedReportCount == 1);
  REQUIRE(nmrReports.federationSavedReportCount == 1);
  REQUIRE(taraReports.federationSavedReportCount == 1);
  REQUIRE(nmraReports.federationSavedReportCount == 1);
  REQUIRE(fqrReports.federationSavedReportCount == 1);

  REQUIRE_NOTHROW(fqr->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(nmra->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(tara->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(nmr->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(tar->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(fqr->disconnect());
  REQUIRE_NOTHROW(nmra->disconnect());
  REQUIRE_NOTHROW(tara->disconnect());
  REQUIRE_NOTHROW(nmr->disconnect());
  REQUIRE_NOTHROW(tar->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}
} // namespace
