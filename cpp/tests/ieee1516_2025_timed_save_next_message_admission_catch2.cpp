#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded timed federation save covers inclusive and exclusive next-message boundaries",
    "[integration][development-profile][federation-management][save-restore][time-management]"
    "[timed-save][multi-federate-callback-ordering]"
    "[next-message-request][next-message-request-available]"
    "[rti.service.request-federation-save][rti.service.time-advance-request]"
    "[rti.service.next-message-request][rti.service.next-message-request-available]"
    "[rti.service.enable-time-constrained][rti.service.enable-time-regulation]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[federate.callback.initiate-federate-save][federate.callback.time-advance-grant]"
    "[federate.callback.federation-saved]"
    "[timed-save-next-message-admission]") {
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
  auto const inclusiveLabel = std::wstring{L"next-message-save-inclusive"};
  auto const exclusiveLabel = std::wstring{L"next-message-save-exclusive"};

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"next-message-save-owner", L"regulator", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"next-message-save-receiver", L"time-constrained", federationName));

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(owner->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));

  ownerReports.callbackOrder.clear();
  receiverReports.callbackOrder.clear();
  REQUIRE_NOTHROW(owner->requestFederationSave(
      inclusiveLabel,
      rti1516_2025::HLAinteger64Time(5)));

  // NMR is inclusive: its next grant at the scheduled save timestamp admits
  // the save and invokes the constrained recipient directly before that grant.
  REQUIRE_NOTHROW(receiver->nextMessageRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{inclusiveLabel});
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(receiverReports.timeAdvanceGrantReports.back().value == L"5");
  REQUIRE(receiverReports.callbackOrder == std::vector<std::string>{"save-initiate", "grant"});
  REQUIRE(ownerReports.callbackOrder == std::vector<std::string>{"grant"});
  static_cast<void>(owner->evokeCallback(0.0));
  REQUIRE(ownerReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{inclusiveLabel});
  REQUIRE(ownerReports.callbackOrder == std::vector<std::string>{"grant", "save-initiate"});

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

  ownerReports.callbackOrder.clear();
  receiverReports.callbackOrder.clear();
  REQUIRE_NOTHROW(owner->requestFederationSave(
      exclusiveLabel,
      rti1516_2025::HLAinteger64Time(7)));

  // NMRA is exclusive. An equal next grant leaves the replacement request
  // pending; a later next grant performs the direct pre-grant admission.
  REQUIRE_NOTHROW(receiver->nextMessageRequestAvailable(
      rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));
  REQUIRE(ownerReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{inclusiveLabel});
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 2);
  REQUIRE(receiverReports.timeAdvanceGrantReports.back().value == L"7");
  REQUIRE(receiverReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{inclusiveLabel});
  REQUIRE(receiverReports.callbackOrder == std::vector<std::string>{"grant"});
  REQUIRE(ownerReports.callbackOrder == std::vector<std::string>{"grant"});

  REQUIRE_NOTHROW(receiver->nextMessageRequestAvailable(
      rti1516_2025::HLAinteger64Time(8)));
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{inclusiveLabel, exclusiveLabel});
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 3);
  REQUIRE(receiverReports.timeAdvanceGrantReports.back().value == L"8");
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"grant", "save-initiate", "grant"});
  REQUIRE(ownerReports.callbackOrder == std::vector<std::string>{"grant", "grant"});
  static_cast<void>(owner->evokeCallback(0.0));
  REQUIRE(ownerReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{inclusiveLabel, exclusiveLabel});
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
  REQUIRE(ownerReports.federationSavedReportCount == 2);
  REQUIRE(receiverReports.federationSavedReportCount == 2);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}
} // namespace
