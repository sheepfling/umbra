#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

TEST_CASE(
    "Embedded queued timestamped Delete Object Instance survives time-regulation disable and re-enable with changed lookahead",
    "[integration][development-profile][object-management][time-management]"
    "[timestamped-object-deletion][tso][re-enable][regulation-disable][changed-lookahead]"
    "[timestamped-object-deletion-regulation-reenable-changed-lookahead]"
    "[rti.service.delete-object-instance][rti.service.retract]"
    "[rti.service.enable-time-regulation][rti.service.disable-time-regulation]"
    "[rti.service.query-lookahead][rti.service.enable-time-constrained]"
    "[rti.service.time-advance-request]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.register-object-instance]"
    "[federate.callback.remove-object-instance]"
    "[federate.callback.time-regulation-enabled]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x43, 0x48, 0x47, 0x2D, 0x4C, 0x41};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timestamped-delete-changed-lookahead-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-delete-changed-lookahead-receiver", L"subscriber", federationName));

  auto const child = publisher->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const reliable = publisher->getAttributeHandle(
      child,
      fixture_hla::fixture::reliable_base_a);
  auto const bestEffort = publisher->getAttributeHandle(
      child,
      fixture_hla::fixture::best_effort_base);
  AttributeHandleSet const attributes{reliable, bestEffort};
  REQUIRE(child.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE(bestEffort.isValid());
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(child, attributes));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(child));
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);
  auto const objectInstanceName = publisher->getObjectInstanceName(objectInstance);
  REQUIRE(receiver->getObjectInstanceHandle(objectInstanceName) == objectInstance);

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  while (publisher->evokeCallback(0.0)) {
  }
  REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 1U);

  // The accepted deletion is queued at timestamp five under lookahead one.
  // Changing the producer's role and lookahead must not replace its payload or
  // recipient ledger.
  auto const retraction = publisher->deleteObjectInstance(
      objectInstance,
      tag,
      rti1516_2025::HLAinteger64Time(5));
  REQUIRE(retraction.isValid());
  REQUIRE(receiverReports.objectRemovalReports.empty());
  REQUIRE(receiver->getObjectInstanceHandle(objectInstanceName) == objectInstance);

  REQUIRE_NOTHROW(publisher->disableTimeRegulation());
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(3)));
  while (publisher->evokeCallback(0.0)) {
  }
  REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 2U);
  rti1516_2025::HLAinteger64Interval changedLookahead;
  REQUIRE_NOTHROW(publisher->queryLookahead(changedLookahead));
  REQUIRE(changedLookahead.getInterval() == 3);

  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE(receiverReports.objectRemovalReports.empty());
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(publisherReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(publisherReports.timeAdvanceGrantReports.front().value == L"2");
  REQUIRE(receiverReports.objectRemovalReports.size() == 1U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(receiverReports.callbackOrder == std::vector<std::string>{"remove", "grant"});

  auto const& removal = receiverReports.objectRemovalReports.front();
  REQUIRE(removal.objectInstance == objectInstance);
  REQUIRE(removal.producingFederate == publisherHandle);
  REQUIRE(variableLengthDataBytes(removal.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(removal.timeImplementationName == L"HLAinteger64Time");
  REQUIRE(removal.timeValue == L"5");
  REQUIRE(removal.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(removal.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(removal.retractionSupplied);
  REQUIRE(removal.retractionValid);
  REQUIRE_THROWS_AS(
      receiver->getObjectInstanceHandle(objectInstanceName),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      publisher->getObjectInstanceName(objectInstance),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

} // namespace
