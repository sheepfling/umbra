#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded timed federation save uses an exclusive Flush Queue grant boundary",
    "[integration][development-profile][federation-management][save-restore][time-management]"
    "[timed-save][multi-federate-callback-ordering]"
    "[rti.service.request-federation-save][rti.service.time-advance-request]"
    "[rti.service.flush-queue-request][rti.service.send-interaction]"
    "[rti.service.enable-time-constrained][rti.service.enable-time-regulation]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[federate.callback.receive-interaction][federate.callback.initiate-federate-save]"
    "[federate.callback.flush-queue-grant][federate.callback.federation-saved]"
    "[timed-save-flush-queue-admission]") {
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
  auto const saveLabel = std::wstring{L"flush-queue-save"};
  unsigned char const tagBytes[] = {0x46, 0x51, 0x52};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"flush-queue-save-owner", L"regulator", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"flush-queue-save-receiver", L"time-constrained", federationName));

  auto const interactionClass = owner->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const identifier = owner->getParameterHandle(interactionClass, fixture_hla::fixture::identifier);
  ParameterHandleValueMap parameters;
  unsigned char const identifierBytes[] = {0xF0, 0x51};
  parameters.emplace(identifier, VariableLengthData(identifierBytes, sizeof(identifierBytes)));
  REQUIRE_NOTHROW(owner->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(owner->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(owner->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));

  // A TSO payload at the save time is flushed before the first actual grant.
  // That actual grant is equal to the scheduled save time, so FQR's strict
  // timestamped-save condition must leave the request pending.
  auto const message = owner->sendInteraction(
      interactionClass,
      parameters,
      tag,
      rti1516_2025::HLAinteger64Time(5));
  REQUIRE(message.isValid());
  REQUIRE_NOTHROW(owner->requestFederationSave(
      saveLabel,
      rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(4)));
  REQUIRE_NOTHROW(receiver->flushQueueRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1);
  REQUIRE(receiverReports.timestampedInteractionReports.back().timeValue == L"5");
  REQUIRE(receiverReports.initiateFederateSaveReports.empty());
  REQUIRE(receiverReports.flushQueueGrantReports.size() == 1);
  REQUIRE(receiverReports.flushQueueGrantReports.back().value == L"5");
  REQUIRE(receiverReports.callbackOrder == std::vector<std::string>{"interaction", "flush-grant"});
  REQUIRE(ownerReports.callbackOrder.empty());

  // The owner first finishes its advance to establish a later GALT, then its
  // next request permits an actual FQR grant of 6. That strictly-later grant
  // must instruct the constrained recipient before state changes to 6.
  REQUIRE_FALSE(owner->evokeCallback(0.0));
  REQUIRE(ownerReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(ownerReports.timeAdvanceGrantReports.back().value == L"4");
  std::size_t flushQueueGrantsAtSaveInitiate = 0;
  receiverReports.onInitiateFederateSave =
      [&receiverReports, &flushQueueGrantsAtSaveInitiate] {
        flushQueueGrantsAtSaveInitiate = receiverReports.flushQueueGrantReports.size();
      };
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(receiver->flushQueueRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(flushQueueGrantsAtSaveInitiate == 1);
  REQUIRE(receiverReports.flushQueueGrantReports.size() == 2);
  REQUIRE(receiverReports.flushQueueGrantReports.back().value == L"6");
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"interaction", "flush-grant", "save-initiate", "flush-grant"});
  REQUIRE(ownerReports.callbackOrder == std::vector<std::string>{"grant"});

  while (owner->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});

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
