#include "ieee1516_2025_federation_management_fixture_support.hpp"

TEST_CASE(
    "Embedded Request Retraction notifies delivered directed-interaction recipients and suppresses queued fanout",
    "[integration][development-profile][interaction-management][directed][time-management]"
    "[timestamped-directed-interaction]"
    "[timestamped-interaction-request-retraction-fanout]"
    "[rti.service.send-directed-interaction][rti.service.retract]"
    "[rti.service.time-advance-request][federate.callback.receive-directed-interaction]"
    "[federate.callback.request-retraction]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador immediateReports;
  ReportingFederateAmbassador constrainedReports;
  auto publisher = makeRti();
  auto immediate = makeRti();
  auto constrained = makeRti();
  auto const federationName = nextFederationName();
  auto const objectConsumer = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                               "cpp" / "tests" / "data" /
                               "directed-interaction-object-consumer-fom.xml")
                                  .wstring();
  auto const interactionProvider = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                                    "cpp" / "tests" / "data" /
                                    "directed-interaction-interaction-provider-fom.xml")
                                       .wstring();
  unsigned char const tagBytes[] = {0x52, 0x44, 0x49};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  InteractionClassHandleSet directedClasses;

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(immediate->connect(immediateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(constrained->connect(constrainedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      std::vector<std::wstring>{objectConsumer, interactionProvider},
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"directed-retraction-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(immediate->joinFederationExecution(
      L"directed-retraction-immediate", L"subscriber", federationName));
  REQUIRE_NOTHROW(constrained->joinFederationExecution(
      L"directed-retraction-constrained", L"subscriber", federationName));

  auto const objectClass = publisher->getObjectClassHandle(
      fixture_hla::fom::directed_fixture_object);
  auto const marker = publisher->getAttributeHandle(objectClass, fixture_hla::fixture::directed_target_marker);
  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::directed_fixture_interaction);
  REQUIRE(objectClass.isValid());
  REQUIRE(marker.isValid());
  REQUIRE(interactionClass.isValid());
  directedClasses.insert(interactionClass);

  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(immediate->subscribeObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(constrained->subscribeObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(publisher->publishObjectClassDirectedInteractions(
      objectClass, directedClasses));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(immediate->subscribeObjectClassDirectedInteractions(
      objectClass, directedClasses, true));
  REQUIRE_NOTHROW(constrained->subscribeObjectClassDirectedInteractions(
      objectClass, directedClasses, true));

  ObjectInstanceHandle target;
  REQUIRE_NOTHROW(target = publisher->registerObjectInstance(objectClass));
  REQUIRE(target.isValid());
  REQUIRE_FALSE(immediate->evokeCallback(0.0));
  REQUIRE_FALSE(constrained->evokeCallback(0.0));
  REQUIRE(immediateReports.objectDiscoveryReports.size() == 1);
  REQUIRE(constrainedReports.objectDiscoveryReports.size() == 1);
  REQUIRE_NOTHROW(constrained->enableTimeConstrained());
  REQUIRE_FALSE(constrained->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  while (publisher->evokeCallback(0.0)) {
  }

  auto const retraction = publisher->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag,
      rti1516_2025::HLAinteger64Time(2));
  REQUIRE(retraction.isValid());

  // The nonconstrained recipient has received the timestamped directed
  // interaction before the constrained recipient can cross its grant
  // boundary. The returned designator consequently distinguishes a delivered
  // recipient from the still-pending temporal fanout.
  REQUIRE_FALSE(immediate->evokeCallback(0.0));
  REQUIRE(immediateReports.directedInteractionReports.size() == 1);
  auto const& first = immediateReports.directedInteractionReports.front();
  REQUIRE(first.interactionClass == interactionClass);
  REQUIRE(first.objectInstance == target);
  REQUIRE(first.parameterValues.empty());
  REQUIRE(variableLengthDataBytes(first.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(first.timeValue == L"2");
  REQUIRE(first.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(first.receivedOrderType == rti1516_2025::RECEIVE);
  REQUIRE(first.retractionSupplied);
  REQUIRE(first.retractionValid);
  REQUIRE(constrainedReports.directedInteractionReports.empty());

  REQUIRE_NOTHROW(publisher->retract(retraction));
  REQUIRE_FALSE(immediate->evokeCallback(0.0));
  REQUIRE(immediateReports.requestRetractionReports.size() == 1);
  REQUIRE(immediateReports.requestRetractionReports.front().retractionValid);
  REQUIRE(variableLengthDataBytes(
              immediateReports.requestRetractionReports.front().encodedRetraction) ==
          variableLengthDataBytes(retraction.encode()));
  REQUIRE(immediateReports.callbackOrder ==
          std::vector<std::string>{"directed", "request-retraction"});

  // The remaining recipient has not received the original callback, so the
  // same legal Retract removes its TSO queue entry rather than issuing a
  // Request Retraction. Advancing its TAR proves the stale delivery cannot
  // cross the grant boundary.
  REQUIRE_NOTHROW(constrained->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(constrained->evokeCallback(0.0));
  REQUIRE(constrainedReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(constrainedReports.directedInteractionReports.empty());
  REQUIRE(constrainedReports.requestRetractionReports.empty());

  // With the constrained member gone, this is an immediate-only timestamped
  // directed interaction: it has no temporal queue fanout but must retain a
  // valid recipient ledger for Request Retraction.
  REQUIRE_NOTHROW(constrained->resignFederationExecution(NO_ACTION));
  auto const immediateOnlyRetraction = publisher->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag,
      rti1516_2025::HLAinteger64Time(4));
  REQUIRE(immediateOnlyRetraction.isValid());
  REQUIRE_FALSE(immediate->evokeCallback(0.0));
  REQUIRE(immediateReports.directedInteractionReports.size() == 2);
  REQUIRE(immediateReports.directedInteractionReports.back().retractionValid);
  REQUIRE_NOTHROW(publisher->retract(immediateOnlyRetraction));
  REQUIRE_FALSE(immediate->evokeCallback(0.0));
  REQUIRE(immediateReports.requestRetractionReports.size() == 2);
  REQUIRE(immediateReports.requestRetractionReports.back().retractionValid);
  REQUIRE(variableLengthDataBytes(
              immediateReports.requestRetractionReports.back().encodedRetraction) ==
          variableLengthDataBytes(immediateOnlyRetraction.encode()));

  REQUIRE_NOTHROW(immediate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(constrained->disconnect());
  REQUIRE_NOTHROW(immediate->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
