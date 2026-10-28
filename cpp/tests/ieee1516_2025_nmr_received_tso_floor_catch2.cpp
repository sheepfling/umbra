#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "hla_test_names.hpp"

#include <catch2/generators/catch_generators.hpp>

namespace {

void drainCallbacks(
    rti1516_2025::RTIambassador& rti,
    rti1516_2025::CallbackModel callbackModel) {
  if (callbackModel != rti1516_2025::HLA_EVOKED) {
    return;
  }
  for (int pass = 0; pass != 128; ++pass) {
    if (!rti.evokeCallback(0.0)) {
      break;
    }
  }
}

TEST_CASE(
    "Embedded NMR after pre-grant TSO enforces the grant-time lookahead floor",
    "[integration][development-profile][time-management]"
    "[nmr-received-tso-grant-floor]"
    "[rti.service.next-message-request]"
    "[rti.service.send-interaction]"
    "[federate.callback.receive-interaction]"
    "[federate.callback.time-advance-grant]") {
  auto const callbackModel = GENERATE(
      rti1516_2025::HLA_EVOKED,
      rti1516_2025::HLA_IMMEDIATE);
  ReportingFederateAmbassador requesterReports;
  ReportingFederateAmbassador peerReports;
  auto requester = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
       "data" / "parameter-handle-provider-fom.xml")
          .wstring();

  REQUIRE_NOTHROW(requester->connect(requesterReports, callbackModel));
  REQUIRE_NOTHROW(peer->connect(peerReports, callbackModel));
  REQUIRE_NOTHROW(requester->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"nmr-received-floor-requester", L"publisher", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(
      L"nmr-received-floor-peer", L"publisher", federationName));

  auto const interactionClass = requester->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const peerInteractionClass = peer->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  REQUIRE(interactionClass.isValid());
  REQUIRE(peerInteractionClass.isValid());
  REQUIRE_NOTHROW(requester->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(peer->publishInteractionClass(peerInteractionClass));
  REQUIRE_NOTHROW(requester->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(peer->changeInteractionOrderType(peerInteractionClass, TIMESTAMP));
  REQUIRE_NOTHROW(requester->subscribeInteractionClass(interactionClass));

  REQUIRE_NOTHROW(requester->enableTimeConstrained());
  drainCallbacks(*requester, callbackModel);
  REQUIRE_NOTHROW(requester->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*requester, callbackModel);
  REQUIRE_NOTHROW(peer->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*peer, callbackModel);

  auto const peerParameter = peer->getParameterHandle(
      peerInteractionClass, fixture_hla::fixture::identifier);
  REQUIRE(peerParameter.isValid());
  unsigned char const payload[] = {0x4EU, 0x4DU, 0x52U};
  unsigned char const tagBytes[] = {0x4EU, 0x4DU, 0x52U, 0x2DU, 0x46U};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  ParameterHandleValueMap incomingValues;
  incomingValues.emplace(peerParameter, VariableLengthData(payload, sizeof(payload)));
  REQUIRE(peer->sendInteraction(
      peerInteractionClass,
      incomingValues,
      tag,
      rti1516_2025::HLAinteger64Time(5)).isValid());

  // The queued TSO(5) selects NMR(10)'s grant time. The requester has
  // received TSO before that grant, so §8.10.2 uses grant time 5—not the
  // request time 10—as the base for its actual-lookahead floor.
  REQUIRE_NOTHROW(requester->nextMessageRequest(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE(requesterReports.timeAdvanceGrantReports.empty());
  // Peer TAR(2) would leave GALT below the queued TSO because the requester
  // is itself time-regulating. Advance the producer far enough to establish
  // a safe bound for delivery at timestamp 5.
  REQUIRE_NOTHROW(peer->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(4)));
  drainCallbacks(*peer, callbackModel);
  drainCallbacks(*requester, callbackModel);
  REQUIRE(requesterReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(requesterReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(requesterReports.timestampedInteractionReports.front().timeValue == L"5");
  REQUIRE(requesterReports.timeAdvanceGrantReports.front().value == L"5");

  ParameterHandleValueMap outgoingValues;
  outgoingValues.emplace(peerParameter, VariableLengthData(payload, sizeof(payload)));
  REQUIRE_THROWS_AS(
      requester->sendInteraction(
          interactionClass,
          outgoingValues,
          tag,
          rti1516_2025::HLAinteger64Time(5)),
      rti1516_2025::InvalidLogicalTime);
  REQUIRE_NOTHROW(requester->sendInteraction(
      interactionClass,
      outgoingValues,
      tag,
      rti1516_2025::HLAinteger64Time(6)));

  REQUIRE_NOTHROW(requester->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(peer->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(requester->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}

TEST_CASE(
    "Embedded NMR with no delivered TSO grants at the requested logical time",
    "[integration][development-profile][time-management]"
    "[nmr-empty-message-set-grant-time]"
    "[rti.service.next-message-request]"
    "[federate.callback.time-advance-grant]") {
  auto const callbackModel = GENERATE(
      rti1516_2025::HLA_EVOKED,
      rti1516_2025::HLA_IMMEDIATE);
  ReportingFederateAmbassador requesterReports;
  ReportingFederateAmbassador peerReports;
  auto requester = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
       "data" / "parameter-handle-provider-fom.xml")
          .wstring();

  REQUIRE_NOTHROW(requester->connect(requesterReports, callbackModel));
  REQUIRE_NOTHROW(peer->connect(peerReports, callbackModel));
  REQUIRE_NOTHROW(requester->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"nmr-empty-set-requester", L"publisher", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(
      L"nmr-empty-set-peer", L"publisher", federationName));

  REQUIRE_NOTHROW(requester->enableTimeConstrained());
  drainCallbacks(*requester, callbackModel);
  REQUIRE_NOTHROW(requester->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*requester, callbackModel);
  REQUIRE_NOTHROW(peer->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*peer, callbackModel);

  // With no TSO message eligible for delivery, NMR(10) completes at its
  // specified logical time once the peer's advance makes that grant safe.
  REQUIRE_NOTHROW(requester->nextMessageRequest(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE(requesterReports.timeAdvanceGrantReports.empty());
  REQUIRE_NOTHROW(peer->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(10)));
  drainCallbacks(*peer, callbackModel);
  drainCallbacks(*requester, callbackModel);

  REQUIRE(requesterReports.timestampedInteractionReports.empty());
  REQUIRE(requesterReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(requesterReports.timeAdvanceGrantReports.front().value == L"10");
  rti1516_2025::HLAinteger64Time logicalTime(99);
  requester->queryLogicalTime(logicalTime);
  REQUIRE(logicalTime.getTime() == 10);

  REQUIRE_NOTHROW(requester->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(peer->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(requester->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}

TEST_CASE(
    "Embedded NMR received TSO floor rejects timestamped attribute updates at grant floor",
    "[integration][development-profile][time-management][object-management]"
    "[nmr-received-tso-grant-floor][rti.service.next-message-request]"
    "[rti.service.update-attribute-values][federate.callback.time-advance-grant]") {
  auto const callbackModel = GENERATE(
      rti1516_2025::HLA_EVOKED,
      rti1516_2025::HLA_IMMEDIATE);
  ReportingFederateAmbassador requesterReports;
  ReportingFederateAmbassador peerReports;
  auto requester = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const dataDirectory =
      std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";
  auto const objectFom = (dataDirectory / "attribute-update-passel-fom.xml").wstring();
  auto const interactionFom =
      (dataDirectory / "parameter-handle-provider-fom.xml").wstring();

  REQUIRE_NOTHROW(requester->connect(requesterReports, callbackModel));
  REQUIRE_NOTHROW(peer->connect(peerReports, callbackModel));
  REQUIRE_NOTHROW(requester->createFederationExecution(
      federationName, std::vector<std::wstring>{objectFom, interactionFom},
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"nmr-attribute-floor-requester", L"publisher", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(
      L"nmr-attribute-floor-peer", L"publisher", federationName));

  auto const interactionClass = requester->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const peerInteractionClass = peer->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  REQUIRE(interactionClass.isValid());
  REQUIRE(peerInteractionClass.isValid());
  REQUIRE_NOTHROW(peer->publishInteractionClass(peerInteractionClass));
  REQUIRE_NOTHROW(peer->changeInteractionOrderType(peerInteractionClass, TIMESTAMP));
  REQUIRE_NOTHROW(requester->subscribeInteractionClass(interactionClass));

  auto const objectClass = requester->getObjectClassHandle(
      umbra::test::hla::wide::fom::attribute_fixture_base);
  auto const attribute = requester->getAttributeHandle(objectClass, umbra::test::hla::wide::fixture::reliable_base_a);
  REQUIRE(objectClass.isValid());
  REQUIRE(attribute.isValid());
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(objectClass, {attribute}));
  auto const object = requester->registerObjectInstance(objectClass);
  REQUIRE(object.isValid());
  REQUIRE_NOTHROW(requester->changeAttributeOrderType(
      object, {attribute}, TIMESTAMP));

  REQUIRE_NOTHROW(requester->enableTimeConstrained());
  drainCallbacks(*requester, callbackModel);
  REQUIRE_NOTHROW(requester->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*requester, callbackModel);
  REQUIRE_NOTHROW(peer->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*peer, callbackModel);

  auto const peerParameter = peer->getParameterHandle(
      peerInteractionClass, fixture_hla::fixture::identifier);
  REQUIRE(peerParameter.isValid());
  unsigned char const payload[] = {0x4EU, 0x4DU, 0x52U};
  unsigned char const tagBytes[] = {0x4EU, 0x4DU, 0x52U, 0x2DU, 0x41U};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  ParameterHandleValueMap incomingValues;
  incomingValues.emplace(peerParameter, VariableLengthData(payload, sizeof(payload)));
  REQUIRE(peer->sendInteraction(
      peerInteractionClass, incomingValues, tag,
      rti1516_2025::HLAinteger64Time(5)).isValid());

  REQUIRE_NOTHROW(requester->nextMessageRequest(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE(requesterReports.timeAdvanceGrantReports.empty());
  REQUIRE_NOTHROW(peer->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(4)));
  drainCallbacks(*peer, callbackModel);
  drainCallbacks(*requester, callbackModel);
  REQUIRE(requesterReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(requesterReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(requesterReports.timestampedInteractionReports.front().timeValue == L"5");
  REQUIRE(requesterReports.timeAdvanceGrantReports.front().value == L"5");

  unsigned char const attributeBytes[] = {0x4EU, 0x4DU, 0x52U, 0x2DU, 0x41U};
  AttributeHandleValueMap outgoingValues;
  outgoingValues.emplace(
      attribute, VariableLengthData(attributeBytes, sizeof(attributeBytes)));
  REQUIRE_THROWS_AS(
      requester->updateAttributeValues(
          object, outgoingValues, tag, rti1516_2025::HLAinteger64Time(5)),
      rti1516_2025::InvalidLogicalTime);
  REQUIRE_NOTHROW(requester->updateAttributeValues(
      object, outgoingValues, tag, rti1516_2025::HLAinteger64Time(6)));

  REQUIRE_NOTHROW(requester->resignFederationExecution(
      rti1516_2025::DELETE_OBJECTS));
  REQUIRE_NOTHROW(peer->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(requester->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}

TEST_CASE(
    "Embedded NMR received TSO floor rejects timestamped directed interactions at grant floor",
    "[integration][development-profile][time-management][object-management]"
    "[nmr-received-tso-grant-floor][rti.service.next-message-request]"
    "[rti.service.send-directed-interaction][federate.callback.time-advance-grant]") {
  auto const callbackModel = GENERATE(
      rti1516_2025::HLA_EVOKED,
      rti1516_2025::HLA_IMMEDIATE);
  ReportingFederateAmbassador requesterReports;
  ReportingFederateAmbassador peerReports;
  auto requester = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const dataDirectory =
      std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";
  auto const objectFom =
      (dataDirectory / "directed-interaction-object-consumer-fom.xml").wstring();
  auto const directedFom =
      (dataDirectory / "directed-interaction-interaction-provider-fom.xml").wstring();
  auto const incomingFom =
      (dataDirectory / "parameter-handle-provider-fom.xml").wstring();

  REQUIRE_NOTHROW(requester->connect(requesterReports, callbackModel));
  REQUIRE_NOTHROW(peer->connect(peerReports, callbackModel));
  REQUIRE_NOTHROW(requester->createFederationExecution(
      federationName,
      std::vector<std::wstring>{objectFom, directedFom, incomingFom},
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"nmr-directed-floor-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(
      L"nmr-directed-floor-peer", L"subscriber", federationName));

  auto const objectClass = requester->getObjectClassHandle(
      fixture_hla::fom::directed_fixture_object);
  auto const marker = requester->getAttributeHandle(
      objectClass, fixture_hla::fixture::directed_target_marker);
  auto const directedClass = requester->getInteractionClassHandle(
      fixture_hla::fom::directed_fixture_interaction);
  auto const incomingClass = requester->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const peerIncomingClass = peer->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  REQUIRE(objectClass.isValid());
  REQUIRE(marker.isValid());
  REQUIRE(directedClass.isValid());
  REQUIRE(incomingClass.isValid());
  REQUIRE(peerIncomingClass.isValid());
  rti1516_2025::InteractionClassHandleSet const directedClasses{directedClass};
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(peer->subscribeObjectClassAttributes(
      peer->getObjectClassHandle(fixture_hla::fom::directed_fixture_object),
      {marker}));
  REQUIRE_NOTHROW(requester->publishObjectClassDirectedInteractions(
      objectClass, directedClasses));
  REQUIRE_NOTHROW(peer->subscribeObjectClassDirectedInteractions(
      peer->getObjectClassHandle(fixture_hla::fom::directed_fixture_object),
      directedClasses, true));
  REQUIRE_NOTHROW(peer->publishInteractionClass(peerIncomingClass));
  REQUIRE_NOTHROW(peer->changeInteractionOrderType(peerIncomingClass, TIMESTAMP));
  REQUIRE_NOTHROW(requester->subscribeInteractionClass(incomingClass));
  auto const target = requester->registerObjectInstance(objectClass);
  REQUIRE(target.isValid());
  drainCallbacks(*peer, callbackModel);

  REQUIRE_NOTHROW(requester->enableTimeConstrained());
  drainCallbacks(*requester, callbackModel);
  REQUIRE_NOTHROW(requester->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*requester, callbackModel);
  REQUIRE_NOTHROW(peer->enableTimeConstrained());
  drainCallbacks(*peer, callbackModel);
  REQUIRE_NOTHROW(peer->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*peer, callbackModel);

  unsigned char const tagBytes[] = {0x4EU, 0x4DU, 0x52U, 0x2DU, 0x44U};
  rti1516_2025::VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  REQUIRE(peer->sendInteraction(
      peerIncomingClass, rti1516_2025::ParameterHandleValueMap{}, tag,
      rti1516_2025::HLAinteger64Time(5)).isValid());
  REQUIRE_NOTHROW(requester->nextMessageRequest(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE_NOTHROW(peer->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(4)));
  drainCallbacks(*peer, callbackModel);
  drainCallbacks(*requester, callbackModel);
  REQUIRE(requesterReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(requesterReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(requesterReports.timeAdvanceGrantReports.front().value == L"5");

  REQUIRE_THROWS_AS(
      requester->sendDirectedInteraction(
          directedClass, target, rti1516_2025::ParameterHandleValueMap{}, tag,
          rti1516_2025::HLAinteger64Time(5)),
      rti1516_2025::InvalidLogicalTime);
  REQUIRE_NOTHROW(requester->sendDirectedInteraction(
      directedClass, target, rti1516_2025::ParameterHandleValueMap{}, tag,
      rti1516_2025::HLAinteger64Time(6)));

  REQUIRE_NOTHROW(requester->resignFederationExecution(
      rti1516_2025::DELETE_OBJECTS));
  REQUIRE_NOTHROW(peer->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(requester->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}

}  // namespace
