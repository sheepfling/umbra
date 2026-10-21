#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

TEST_CASE(
    "Embedded TSO at timed-save timestamp is delivered before Initiate Federate Save",
    "[integration][development-profile][2025][federation-management][time-management]"
    "[timed-save][fqr-distinct-grant][timed-save-equal-boundary-tso]"
    "[rti.service.request-federation-save][rti.service.time-advance-request]"
    "[rti.service.flush-queue-request][rti.service.send-interaction]"
    "[federate.callback.receive-interaction][federate.callback.initiate-federate-save]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador requesterReports;
  ReportingFederateAmbassador peerReports;
  auto requester = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  unsigned char const valueBytes[] = {0x53U, 0x41U, 0x56U, 0x45U};
  unsigned char const tagBytes[] = {0x54U, 0x53U, 0x4FU};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  auto const drain = [](RTIambassador& ambassador) {
    while (ambassador.evokeCallback(0.0)) {
    }
  };

  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(peer->connect(peerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"fqr-save-boundary-requester", L"publisher", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(
      L"fqr-save-boundary-peer", L"publisher", federationName));

  auto const interactionClass = requester->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const identifier = requester->getParameterHandle(
      interactionClass, fixture_hla::fixture::identifier);
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  REQUIRE_NOTHROW(requester->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(peer->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(requester->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(peer->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(requester->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(requester->enableTimeConstrained());
  drain(*requester);
  REQUIRE_NOTHROW(requester->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drain(*requester);
  REQUIRE_NOTHROW(peer->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drain(*peer);

  REQUIRE_NOTHROW(requester->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(4)));
  REQUIRE_NOTHROW(peer->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(4)));
  drain(*requester);
  drain(*peer);
  requesterReports.callbackOrder.clear();
  peerReports.callbackOrder.clear();

  ParameterHandleValueMap values;
  values.emplace(identifier, VariableLengthData(valueBytes, sizeof(valueBytes)));
  REQUIRE(peer->sendInteraction(
              interactionClass,
              values,
              tag,
              rti1516_2025::HLAinteger64Time(6))
              .isValid());
  REQUIRE_NOTHROW(requester->flushQueueRequest(
      rti1516_2025::HLAinteger64Time(10)));
  drain(*requester);
  REQUIRE(requesterReports.flushQueueGrantReports.size() == 1U);
  REQUIRE(requesterReports.flushQueueGrantReports.front().value == L"5");
  REQUIRE(requesterReports.flushQueueGrantReports.front().optimisticValue == L"6");
  REQUIRE(requesterReports.callbackOrder ==
          std::vector<std::string>{"interaction", "flush-grant"});

  // The TSO at the exact save timestamp must already be received before save
  // initiation is admitted; this isolates the equal-time delivery predicate.
  REQUIRE_NOTHROW(requester->requestFederationSave(
      L"fqr-equal-boundary-save", rti1516_2025::HLAinteger64Time(6)));
  REQUIRE(requesterReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(requesterReports.timestampedInteractionReports.front().timeValue == L"6");
  REQUIRE(requesterReports.initiateFederateSaveReports.empty());

  REQUIRE_NOTHROW(requester->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_NOTHROW(peer->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(7)));
  drain(*peer);
  drain(*requester);

  REQUIRE(requesterReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(requesterReports.initiateFederateSaveReports.size() == 1U);
  REQUIRE(requesterReports.initiateFederateSaveReports.front() ==
          L"fqr-equal-boundary-save");
  auto const saveInitiation = std::find(
      requesterReports.callbackOrder.begin() + 2,
      requesterReports.callbackOrder.end(),
      "save-initiate");
  REQUIRE(saveInitiation != requesterReports.callbackOrder.end());

  REQUIRE_NOTHROW(requester->federateSaveBegun());
  REQUIRE_NOTHROW(peer->federateSaveBegun());
  REQUIRE_NOTHROW(requester->federateSaveComplete());
  REQUIRE_NOTHROW(peer->federateSaveComplete());
  drain(*requester);
  drain(*peer);
  REQUIRE(requesterReports.federationSavedReportCount == 1);
  REQUIRE(peerReports.federationSavedReportCount == 1);

  REQUIRE_NOTHROW(requester->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(peer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(requester->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}
