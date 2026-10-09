#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

TEST_CASE(
    "Embedded timestamped Delete Object Instance delivers before TAR and NMR grants",
    "[integration][development-profile][object-management][time-management][tso]"
    "[timestamped-object-deletion][timestamped-delete-object-instance-tar-nmr]"
    "[time-advance-request][next-message-request]"
    "[rti.service.delete-object-instance][rti.service.time-advance-request]"
    "[rti.service.next-message-request]"
    "[federate.callback.remove-object-instance][federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador tarReports;
  ReportingFederateAmbassador nmrReports;
  auto publisher = makeRti();
  auto tar = makeRti();
  auto nmr = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x54, 0x41, 0x52, 0x2D, 0x4E, 0x4D, 0x52};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(tar->connect(tarReports, HLA_EVOKED));
  REQUIRE_NOTHROW(nmr->connect(nmrReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timestamped-delete-tar-nmr-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(tar->joinFederationExecution(
      L"timestamped-delete-tar-receiver", L"subscriber", federationName));
  REQUIRE_NOTHROW(nmr->joinFederationExecution(
      L"timestamped-delete-nmr-receiver", L"subscriber", federationName));

  auto const child = publisher->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const reliable = publisher->getAttributeHandle(child, fixture_hla::fixture::reliable_base_a);
  auto const bestEffort = publisher->getAttributeHandle(child, fixture_hla::fixture::best_effort_base);
  AttributeHandleSet const attributes{reliable, bestEffort};
  REQUIRE(child.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE(bestEffort.isValid());
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(tar->subscribeObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(nmr->subscribeObjectClassAttributes(child, attributes));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(child));
  REQUIRE_FALSE(tar->evokeCallback(0.0));
  REQUIRE_FALSE(nmr->evokeCallback(0.0));
  REQUIRE(tarReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(nmrReports.objectDiscoveryReports.size() == 1U);
  // Each discovery may solicit Auto Provide from the publisher. Complete
  // those RTI setup callbacks before testing time-regulation callbacks.
  while (publisher->evokeCallback(0.0)) {
  }

  REQUIRE_NOTHROW(tar->enableTimeConstrained());
  REQUIRE_FALSE(tar->evokeCallback(0.0));
  REQUIRE_NOTHROW(nmr->enableTimeConstrained());
  REQUIRE_FALSE(nmr->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  // The deletion is accepted at timestamp 7. TAR reaches that timestamp
  // directly, while NMR requests 10 and selects the queued timestamp 7.
  // Both recipients must receive the removal before their own grant, and the
  // producer's time advance to 2 supplies the outgoing TSO lower bound.
  auto const retraction = publisher->deleteObjectInstance(
      objectInstance,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(retraction.isValid());
  REQUIRE(tarReports.objectRemovalReports.empty());
  REQUIRE(nmrReports.objectRemovalReports.empty());

  REQUIRE_NOTHROW(tar->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_NOTHROW(nmr->nextMessageRequest(rti1516_2025::HLAinteger64Time(10)));
  REQUIRE(tarReports.timeAdvanceGrantReports.empty());
  REQUIRE(nmrReports.timeAdvanceGrantReports.empty());
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(tar->evokeCallback(0.0));
  REQUIRE_FALSE(nmr->evokeCallback(0.0));

  REQUIRE(publisherReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(publisherReports.timeAdvanceGrantReports.front().value == L"2");
  REQUIRE(tarReports.objectRemovalReports.size() == 1U);
  REQUIRE(nmrReports.objectRemovalReports.size() == 1U);
  REQUIRE(tarReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(nmrReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(tarReports.timeAdvanceGrantReports.front().value == L"7");
  REQUIRE(nmrReports.timeAdvanceGrantReports.front().value == L"7");
  REQUIRE(tarReports.callbackOrder == std::vector<std::string>{"remove", "grant"});
  REQUIRE(nmrReports.callbackOrder == std::vector<std::string>{"remove", "grant"});

  auto requireRemoval = [&](auto const& report) {
    REQUIRE(report.objectInstance == objectInstance);
    REQUIRE(report.producingFederate == publisherHandle);
    REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
            std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
    REQUIRE(report.timeImplementationName == standard_hla::mom::integer64_time);
    REQUIRE(report.timeValue == L"7");
    REQUIRE(report.sentOrderType == rti1516_2025::TIMESTAMP);
    REQUIRE(report.receivedOrderType == rti1516_2025::TIMESTAMP);
    REQUIRE(report.retractionSupplied);
    REQUIRE(report.retractionValid);
  };
  requireRemoval(tarReports.objectRemovalReports.front());
  requireRemoval(nmrReports.objectRemovalReports.front());
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  rti1516_2025::HLAinteger64Time tarTime;
  rti1516_2025::HLAinteger64Time nmrTime;
  REQUIRE_NOTHROW(tar->queryLogicalTime(tarTime));
  REQUIRE_NOTHROW(nmr->queryLogicalTime(nmrTime));
  REQUIRE(tarTime.getTime() == 7);
  REQUIRE(nmrTime.getTime() == 7);

  REQUIRE_NOTHROW(nmr->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(tar->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(nmr->disconnect());
  REQUIRE_NOTHROW(tar->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

} // namespace
