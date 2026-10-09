#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

TEST_CASE(
    "Embedded timestamped Delete Object Instance reconstitutes on retraction and removes before grant",
    "[integration][development-profile][object-management][time-management]"
    "[rti.service.delete-object-instance][rti.service.retract]"
    "[rti.service.time-advance-request][federate.callback.remove-object-instance]"
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
  unsigned char const tagBytes[] = {0xD4, 0x16, 0x2A};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"timestamped-delete-publisher", L"publisher", federationName));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->getFederateHandle(L"timestamped-delete-publisher"));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-delete-receiver", L"subscriber", federationName));

  auto const child = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliable = publisher->getAttributeHandle(child, L"ReliableBaseA");
  auto const bestEffort = publisher->getAttributeHandle(child, L"BestEffortBase");
  AttributeHandleSet const attributes{reliable, bestEffort};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(child, attributes));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(child));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1);
  // Discovery can enqueue the RTI-invoked Auto Provide request on the
  // publisher. Drain that setup callback before isolating time-regulation
  // enablement below; it is not part of the time-management assertion.
  while (publisher->evokeCallback(0.0)) {
  }
  auto const objectInstanceName = publisher->getObjectInstanceName(objectInstance);

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  REQUIRE_THROWS_AS(
      publisher->deleteObjectInstance(
          objectInstance,
          tag,
          rti1516_2025::HLAinteger64Time(4)),
      rti1516_2025::InvalidLogicalTime);

  auto const firstHandle = publisher->deleteObjectInstance(
      objectInstance,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(firstHandle.isValid());
  REQUIRE(receiverReports.objectRemovalReports.empty());
  REQUIRE(receiver->getObjectInstanceHandle(objectInstanceName) == objectInstance);

  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->retract(firstHandle));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.objectRemovalReports.empty());
  REQUIRE(receiver->getObjectInstanceHandle(objectInstanceName) == objectInstance);

  auto const secondHandle = publisher->deleteObjectInstance(
      objectInstance,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(secondHandle.isValid());
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 2);
  REQUIRE(receiverReports.objectRemovalReports.size() == 1);
  REQUIRE(
      receiverReports.callbackOrder ==
      std::vector<std::string>{"grant", "remove", "grant"});

  auto const& removal = receiverReports.objectRemovalReports.front();
  REQUIRE(removal.objectInstance == objectInstance);
  REQUIRE(removal.producingFederate == publisherHandle);
  REQUIRE(variableLengthDataBytes(removal.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(removal.timeImplementationName == L"HLAinteger64Time");
  REQUIRE(removal.timeValue == L"7");
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
      publisher->retract(secondHandle),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}


}  // namespace
