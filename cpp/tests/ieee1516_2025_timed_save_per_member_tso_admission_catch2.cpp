#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded timed federation save defers each constrained initiation for its queued TSO",
    "[integration][development-profile][federation-management][save-restore][time-management]"
    "[timed-save][multi-federate-callback-ordering]"
    "[rti.service.request-federation-save][rti.service.time-advance-request]"
    "[rti.service.send-interaction][rti.service.enable-time-constrained]"
    "[rti.service.enable-time-regulation][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][federate.callback.receive-interaction]"
    "[federate.callback.initiate-federate-save][federate.callback.time-advance-grant]"
    "[federate.callback.federation-saved][timed-save-per-member-tso-admission]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador readyReports;
  ReportingFederateAmbassador delayedReports;
  auto owner = makeRti();
  auto ready = makeRti();
  auto delayed = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  auto const saveLabel = std::wstring{L"per-member-tso-save"};
  unsigned char const tagBytes[] = {0x54, 0x53, 0x4F};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(ready->connect(readyReports, HLA_EVOKED));
  REQUIRE_NOTHROW(delayed->connect(delayedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"per-member-tso-owner", L"regulator", federationName));
  REQUIRE_NOTHROW(ready->joinFederationExecution(
      L"per-member-tso-ready", L"time-constrained", federationName));
  REQUIRE_NOTHROW(delayed->joinFederationExecution(
      L"per-member-tso-delayed", L"time-constrained", federationName));

  auto const interactionClass = owner->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const identifier = owner->getParameterHandle(interactionClass, fixture_hla::fixture::identifier);
  ParameterHandleValueMap parameters;
  unsigned char const identifierBytes[] = {0x50, 0x45, 0x52};
  parameters.emplace(identifier, VariableLengthData(identifierBytes, sizeof(identifierBytes)));
  REQUIRE_NOTHROW(owner->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(owner->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(delayed->subscribeInteractionClass(interactionClass));

  REQUIRE_NOTHROW(ready->enableTimeConstrained());
  REQUIRE_FALSE(ready->evokeCallback(0.0));
  REQUIRE_NOTHROW(delayed->enableTimeConstrained());
  REQUIRE_FALSE(delayed->evokeCallback(0.0));
  REQUIRE_NOTHROW(owner->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));

  auto const message = owner->sendInteraction(
      interactionClass,
      parameters,
      tag,
      rti1516_2025::HLAinteger64Time(5));
  REQUIRE(message.isValid());
  REQUIRE_NOTHROW(owner->requestFederationSave(
      saveLabel,
      rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(ready->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(delayed->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));

  // The first member can enter the save operation at its clean inclusive
  // boundary. The delayed member's initiation remains local to its grant: it
  // must not happen until the TSO payload at the scheduled time is delivered.
  REQUIRE_FALSE(ready->evokeCallback(0.0));
  REQUIRE(readyReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(readyReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(readyReports.timeAdvanceGrantReports.back().value == L"5");
  REQUIRE(readyReports.callbackOrder == std::vector<std::string>{"save-initiate", "grant"});
  REQUIRE(delayedReports.timestampedInteractionReports.empty());
  REQUIRE(delayedReports.initiateFederateSaveReports.empty());
  REQUIRE(delayedReports.callbackOrder.empty());
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());
  REQUIRE(ownerReports.callbackOrder.empty());

  REQUIRE_FALSE(delayed->evokeCallback(0.0));
  REQUIRE(delayedReports.timestampedInteractionReports.size() == 1);
  REQUIRE(delayedReports.timestampedInteractionReports.back().timeValue == L"5");
  REQUIRE(delayedReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(delayedReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(delayedReports.timeAdvanceGrantReports.back().value == L"5");
  REQUIRE(delayedReports.callbackOrder ==
          std::vector<std::string>{"interaction", "save-initiate", "grant"});
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());
  REQUIRE(ownerReports.callbackOrder.empty());

  REQUIRE(owner->evokeCallback(0.0));
  REQUIRE_FALSE(owner->evokeCallback(0.0));
  REQUIRE(ownerReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(ownerReports.timeAdvanceGrantReports.back().value == L"5");
  REQUIRE(ownerReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(ownerReports.callbackOrder == std::vector<std::string>{"grant", "save-initiate"});

  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(ready->federateSaveBegun());
  REQUIRE_NOTHROW(delayed->federateSaveBegun());
  REQUIRE_NOTHROW(owner->federateSaveComplete());
  REQUIRE_NOTHROW(ready->federateSaveComplete());
  REQUIRE_NOTHROW(delayed->federateSaveComplete());
  while (owner->evokeCallback(0.0)) {
  }
  while (ready->evokeCallback(0.0)) {
  }
  while (delayed->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.federationSavedReportCount == 1);
  REQUIRE(readyReports.federationSavedReportCount == 1);
  REQUIRE(delayedReports.federationSavedReportCount == 1);

  REQUIRE_NOTHROW(delayed->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(ready->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(delayed->disconnect());
  REQUIRE_NOTHROW(ready->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}
}
