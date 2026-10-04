#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded timed federation save requested during a TSO callback waits for in-transit delivery",
    "[integration][development-profile][federation-management][save-restore][time-management]"
    "[timed-save][multi-federate-callback-ordering]"
    "[rti.service.request-federation-save][rti.service.time-advance-request]"
    "[rti.service.send-interaction][rti.service.enable-time-constrained]"
    "[rti.service.enable-time-regulation][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][federate.callback.receive-interaction]"
    "[federate.callback.initiate-federate-save][federate.callback.time-advance-grant]"
    "[federate.callback.federation-saved][timed-save-in-transit-tso-admission]") {
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
  auto const saveLabel = std::wstring{L"in-transit-tso-save"};
  unsigned char const tagBytes[] = {0x49, 0x4E, 0x54};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"in-transit-tso-owner", L"regulator", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"in-transit-tso-receiver", L"time-constrained", federationName));

  auto const interactionClass = owner->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const identifier = owner->getParameterHandle(interactionClass, fixture_hla::fixture::identifier);
  ParameterHandleValueMap parameters;
  unsigned char const identifierBytes[] = {0x49, 0x4E, 0x54};
  parameters.emplace(identifier, VariableLengthData(identifierBytes, sizeof(identifierBytes)));
  REQUIRE_NOTHROW(owner->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(owner->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(owner->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));

  bool reentrantSaveRequestAccepted = false;
  bool saveInitiatedBeforeTsoCallbackReturned = false;
  receiverReports.onTimestampedInteraction = [&] {
    saveInitiatedBeforeTsoCallbackReturned =
        !receiverReports.initiateFederateSaveReports.empty();
    try {
      owner->requestFederationSave(saveLabel, rti1516_2025::HLAinteger64Time(5));
      reentrantSaveRequestAccepted = true;
    } catch (...) {
      reentrantSaveRequestAccepted = false;
    }
  };

  auto const message = owner->sendInteraction(
      interactionClass,
      parameters,
      tag,
      rti1516_2025::HLAinteger64Time(5));
  REQUIRE(message.isValid());
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(4)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));
  REQUIRE(ownerReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(ownerReports.timeAdvanceGrantReports.back().value == L"4");

  // The timestamped request is made while this interaction is in transit.
  // It becomes eligible only after the callback returns and the runtime
  // completes that delivery, so the direct save callback follows Receive
  // Interaction and precedes the matching grant.
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(reentrantSaveRequestAccepted);
  REQUIRE_FALSE(saveInitiatedBeforeTsoCallbackReturned);
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1);
  REQUIRE(receiverReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(receiverReports.timeAdvanceGrantReports.back().value == L"5");
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"interaction", "save-initiate", "grant"});
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());
  REQUIRE(ownerReports.callbackOrder == std::vector<std::string>{"grant"});

  static_cast<void>(owner->evokeCallback(0.0));
  REQUIRE(ownerReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(ownerReports.timeAdvanceGrantReports.back().value == L"4");
  REQUIRE(ownerReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
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

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}
}
