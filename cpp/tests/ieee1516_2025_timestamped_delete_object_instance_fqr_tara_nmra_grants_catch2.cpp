#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

TEST_CASE(
    "Embedded timestamped Delete Object Instance delivers before FQR TARA and NMRA grants",
    "[integration][development-profile][object-management][time-management][tso]"
    "[timestamped-object-deletion]"
    "[timestamped-delete-fqr-tara-nmra]"
    "[flush-queue-request][time-advance-request-available][next-message-request-available]"
    "[rti.service.delete-object-instance][rti.service.retract]"
    "[rti.service.flush-queue-request]"
    "[rti.service.time-advance-request-available]"
    "[rti.service.next-message-request-available][rti.service.time-advance-request]"
    "[federate.callback.remove-object-instance][federate.callback.flush-queue-grant]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador fqrReports;
  ReportingFederateAmbassador taraReports;
  ReportingFederateAmbassador nmraReports;
  auto publisher = makeRti();
  auto fqr = makeRti();
  auto tara = makeRti();
  auto nmra = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0xD8, 0x61, 0x4E};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(fqr->connect(fqrReports, HLA_EVOKED));
  REQUIRE_NOTHROW(tara->connect(taraReports, HLA_EVOKED));
  REQUIRE_NOTHROW(nmra->connect(nmraReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timestamped-delete-alternate-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(fqr->joinFederationExecution(
      L"timestamped-delete-alternate-fqr", L"subscriber", federationName));
  REQUIRE_NOTHROW(tara->joinFederationExecution(
      L"timestamped-delete-alternate-tara", L"subscriber", federationName));
  REQUIRE_NOTHROW(nmra->joinFederationExecution(
      L"timestamped-delete-alternate-nmra", L"subscriber", federationName));

  auto const child = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliable = publisher->getAttributeHandle(child, L"ReliableBaseA");
  auto const bestEffort = publisher->getAttributeHandle(child, L"BestEffortBase");
  AttributeHandleSet const attributes{reliable, bestEffort};
  REQUIRE(child.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE(bestEffort.isValid());
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(fqr->subscribeObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(tara->subscribeObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(nmra->subscribeObjectClassAttributes(child, attributes));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(child));
  for (auto* receiver : {fqr.get(), tara.get(), nmra.get()}) {
    while (receiver->evokeCallback(0.0)) {
    }
  }
  REQUIRE(fqrReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(taraReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(nmraReports.objectDiscoveryReports.size() == 1U);

  REQUIRE_NOTHROW(fqr->enableTimeConstrained());
  while (fqr->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(tara->enableTimeConstrained());
  while (tara->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(nmra->enableTimeConstrained());
  while (nmra->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (publisher->evokeCallback(0.0)) {
  }

  // The deletion is accepted at timestamp 7. FQR, TARA, and NMRA use three
  // distinct grant frontiers over the same queued Remove Object Instance:
  // FQR requests 10, TARA reaches 7 inclusively, and NMRA selects the queued
  // timestamp 7 from its request-10 boundary.
  auto const retraction = publisher->deleteObjectInstance(
      objectInstance,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(retraction.isValid());
  REQUIRE(fqrReports.objectRemovalReports.empty());
  REQUIRE(taraReports.objectRemovalReports.empty());
  REQUIRE(nmraReports.objectRemovalReports.empty());

  REQUIRE_NOTHROW(fqr->flushQueueRequest(rti1516_2025::HLAinteger64Time(10)));
  REQUIRE_NOTHROW(tara->timeAdvanceRequestAvailable(
      rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_NOTHROW(nmra->nextMessageRequestAvailable(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  while (publisher->evokeCallback(0.0)) {
  }

  for (auto* receiver : {fqr.get(), tara.get(), nmra.get()}) {
    while (receiver->evokeCallback(0.0)) {
    }
  }
  REQUIRE(publisherReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(publisherReports.timeAdvanceGrantReports.front().value == L"2");
  REQUIRE(fqrReports.objectRemovalReports.size() == 1U);
  REQUIRE(taraReports.objectRemovalReports.size() == 1U);
  REQUIRE(nmraReports.objectRemovalReports.size() == 1U);
  REQUIRE(fqrReports.flushQueueGrantReports.size() == 1U);
  REQUIRE(fqrReports.flushQueueGrantReports.front().value == L"7");
  REQUIRE(fqrReports.flushQueueGrantReports.front().optimisticValue == L"7");
  REQUIRE(taraReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(taraReports.timeAdvanceGrantReports.front().value == L"7");
  REQUIRE(nmraReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(nmraReports.timeAdvanceGrantReports.front().value == L"7");
  REQUIRE(fqrReports.callbackOrder == std::vector<std::string>{"remove", "flush-grant"});
  REQUIRE(taraReports.callbackOrder == std::vector<std::string>{"remove", "grant"});
  REQUIRE(nmraReports.callbackOrder == std::vector<std::string>{"remove", "grant"});

  auto requireRemoval = [&](auto const& report) {
    REQUIRE(report.objectInstance == objectInstance);
    REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
            std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
    REQUIRE(report.producingFederate == publisherHandle);
    REQUIRE(report.timeImplementationName == L"HLAinteger64Time");
    REQUIRE(report.timeValue == L"7");
    REQUIRE(report.sentOrderType == rti1516_2025::TIMESTAMP);
    REQUIRE(report.receivedOrderType == rti1516_2025::TIMESTAMP);
    REQUIRE(report.retractionSupplied);
    REQUIRE(report.retractionValid);
  };
  requireRemoval(fqrReports.objectRemovalReports.front());
  requireRemoval(taraReports.objectRemovalReports.front());
  requireRemoval(nmraReports.objectRemovalReports.front());

  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(nmra->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(tara->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(fqr->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(nmra->disconnect());
  REQUIRE_NOTHROW(tara->disconnect());
  REQUIRE_NOTHROW(fqr->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

} // namespace
