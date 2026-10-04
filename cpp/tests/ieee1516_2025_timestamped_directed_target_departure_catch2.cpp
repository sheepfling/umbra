#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded timestamped target departure suppresses directed delivery at the grant boundary",
    "[integration][development-profile][object-management][interaction-management]"
    "[directed][directed-target-departure][time-management]"
    "[timestamped-directed-interaction][tso]"
    "[rti.service.publish-object-class-attributes][rti.service.subscribe-object-class-attributes]"
    "[rti.service.publish-object-class-directed-interactions]"
    "[rti.service.subscribe-object-class-directed-interactions]"
    "[rti.service.send-directed-interaction][rti.service.delete-object-instance]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.time-advance-request]"
    "[federate.callback.remove-object-instance][federate.callback.receive-directed-interaction]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const objectConsumer = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                               "cpp" / "tests" / "data" /
                               "directed-interaction-object-consumer-fom.xml")
                                  .wstring();
  auto const interactionProvider = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                                    "cpp" / "tests" / "data" /
                                    "directed-interaction-interaction-provider-fom.xml")
                                       .wstring();
  unsigned char const tagBytes[] = {0x54, 0x41, 0x52, 0x47, 0x2D, 0x44};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  InteractionClassHandleSet directedClasses;

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      std::vector<std::wstring>{objectConsumer, interactionProvider},
      standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timestamped-target-departure-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-target-departure-receiver", L"subscriber", federationName));

  auto const objectClass = publisher->getObjectClassHandle(
      fixture_hla::fom::directed_fixture_object);
  auto const marker = publisher->getAttributeHandle(
      objectClass,
      fixture_hla::fixture::directed_target_marker);
  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::directed_fixture_interaction);
  REQUIRE(objectClass.isValid());
  REQUIRE(marker.isValid());
  REQUIRE(interactionClass.isValid());
  directedClasses.insert(interactionClass);

  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(publisher->publishObjectClassDirectedInteractions(
      objectClass,
      directedClasses));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassDirectedInteractions(
      objectClass,
      directedClasses,
      true));

  ObjectInstanceHandle target;
  REQUIRE_NOTHROW(target = publisher->registerObjectInstance(objectClass));
  REQUIRE(target.isValid());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);
  auto const targetName = publisher->getObjectInstanceName(target);
  REQUIRE(receiver->getObjectInstanceHandle(targetName) == target);

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (publisher->evokeCallback(0.0)) {
  }

  // Accept the directed passel at time 7 while the receiver still knows the
  // target. The later timestamp-six removal must make this queued callback
  // stale without retracting or replaying it.
  auto const directedRetraction = publisher->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(directedRetraction.isValid());
  REQUIRE(receiverReports.directedInteractionReports.empty());

  auto const deletionRetraction = publisher->deleteObjectInstance(
      target,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(deletionRetraction.isValid());
  REQUIRE(receiverReports.objectRemovalReports.empty());
  REQUIRE(receiver->getObjectInstanceHandle(targetName) == target);

  // The first TAR stops at the earlier removal boundary. The removal is
  // delivered, but the time-seven directed callback remains queued.
  receiverReports.callbackOrder.clear();
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(2)));
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.objectRemovalReports.size() == 1U);
  REQUIRE(receiverReports.directedInteractionReports.empty());
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"remove", "grant"});
  auto const& removal = receiverReports.objectRemovalReports.front();
  REQUIRE(removal.objectInstance == target);
  REQUIRE(removal.producingFederate == publisherHandle);
  REQUIRE(variableLengthDataBytes(removal.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(removal.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(removal.timeValue == L"6");
  REQUIRE(removal.sentOrderType == TIMESTAMP);
  REQUIRE(removal.receivedOrderType == TIMESTAMP);
  REQUIRE(removal.retractionSupplied);
  REQUIRE(removal.retractionValid);
  REQUIRE_THROWS_AS(
      receiver->getObjectInstanceHandle(targetName),
      rti1516_2025::ObjectInstanceNotKnown);

  // Crossing the later grant boundary rechecks the target at callback entry.
  // The accepted time-seven directed passel is therefore suppressed, and the
  // receiver still advances without a stale directed callback.
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(7)));
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.directedInteractionReports.empty());
  REQUIRE(receiverReports.objectRemovalReports.size() == 1U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 2U);
  REQUIRE_THROWS_AS(
      publisher->getObjectInstanceName(target),
      rti1516_2025::ObjectInstanceNotKnown);

  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassDirectedInteractions(
      objectClass,
      directedClasses));
  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributes(
      objectClass,
      {marker}));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
} // namespace
