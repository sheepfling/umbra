#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded federation restore restores one queued timestamped attribute update to multiple recipients",
    "[integration][development-profile][federation-management][save-restore]"
    "[object-management][time-management][tso][timestamped-attribute-update]"
    "[mixed-fanout][multi-federate-callback-ordering]"
    "[rti.service.request-federation-save][rti.service.request-federation-restore]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[rti.service.federate-restore-complete][rti.service.update-attribute-values]"
    "[rti.service.register-object-instance][rti.service.subscribe-object-class-attributes]"
    "[rti.service.flush-queue-request][rti.service.retract]"
    "[federate.callback.reflect-attribute-values][federate.callback.request-retraction]"
    "[federate.callback.federation-saved][federate.callback.federation-restored]"
    "[federate.callback.flush-queue-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador firstReceiverReports;
  ReportingFederateAmbassador secondReceiverReports;
  auto publisher = makeRti();
  auto firstReceiver = makeRti();
  auto secondReceiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" / "tests" / "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const valueBytes[] = {0x4D, 0x55, 0x4C, 0x54, 0x49};
  unsigned char const tagBytes[] = {0x4D, 0x45, 0x4D, 0x2D, 0x54};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  std::wstring const saveLabel = L"multi-recipient-tso-attribute-baseline";
  auto const drain = [](RTIambassador& ambassador) {
    while (ambassador.evokeCallback(0.0)) {}
  };

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(firstReceiver->connect(firstReceiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(secondReceiver->connect(secondReceiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"multi-recipient-tso-attribute-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(firstReceiver->joinFederationExecution(
      L"multi-recipient-tso-attribute-first", L"subscriber", federationName));
  REQUIRE_NOTHROW(secondReceiver->joinFederationExecution(
      L"multi-recipient-tso-attribute-second", L"subscriber", federationName));
  auto const objectClass = publisher->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const attribute = publisher->getAttributeHandle(
      objectClass, fixture_hla::fixture::reliable_base_a);
  REQUIRE(objectClass.isValid());
  REQUIRE(attribute.isValid());
  AttributeHandleSet const attributes{attribute};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(firstReceiver->subscribeObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(secondReceiver->subscribeObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(objectClass, attributes, TIMESTAMP));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(objectClass));
  drain(*firstReceiver);
  drain(*secondReceiver);
  REQUIRE(firstReceiverReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(secondReceiverReports.objectDiscoveryReports.size() == 1U);
  REQUIRE_NOTHROW(firstReceiver->enableTimeConstrained());
  drain(*firstReceiver);
  REQUIRE_NOTHROW(secondReceiver->enableTimeConstrained());
  drain(*secondReceiver);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drain(*publisher);
  AttributeHandleValueMap attributeValues;
  attributeValues.emplace(attribute, VariableLengthData(valueBytes, sizeof(valueBytes)));
  auto const retraction = publisher->updateAttributeValues(
      objectInstance, attributeValues, tag, rti1516_2025::HLAinteger64Time(7));
  REQUIRE(retraction.isValid());
  REQUIRE(firstReceiverReports.attributeReflectionReports.empty());
  REQUIRE(secondReceiverReports.attributeReflectionReports.empty());
  REQUIRE_NOTHROW(firstReceiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_NOTHROW(secondReceiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_NOTHROW(publisher->requestFederationSave(saveLabel));
  drain(*publisher);
  drain(*firstReceiver);
  drain(*secondReceiver);
  REQUIRE_NOTHROW(publisher->federateSaveBegun());
  REQUIRE_NOTHROW(firstReceiver->federateSaveBegun());
  REQUIRE_NOTHROW(secondReceiver->federateSaveBegun());
  REQUIRE_NOTHROW(publisher->federateSaveComplete());
  REQUIRE_NOTHROW(firstReceiver->federateSaveComplete());
  REQUIRE_NOTHROW(secondReceiver->federateSaveComplete());
  drain(*publisher);
  drain(*firstReceiver);
  drain(*secondReceiver);
  REQUIRE(publisherReports.federationSavedReportCount == 1U);
  REQUIRE(firstReceiverReports.federationSavedReportCount == 1U);
  REQUIRE(secondReceiverReports.federationSavedReportCount == 1U);
  REQUIRE_NOTHROW(publisher->retract(retraction));
  drain(*firstReceiver);
  drain(*secondReceiver);
  REQUIRE(firstReceiverReports.attributeReflectionReports.empty());
  REQUIRE(secondReceiverReports.attributeReflectionReports.empty());
  REQUIRE_THROWS_AS(publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);
  REQUIRE_NOTHROW(publisher->requestFederationRestore(saveLabel));
  drain(*publisher);
  drain(*firstReceiver);
  drain(*secondReceiver);
  REQUIRE_NOTHROW(publisher->federateRestoreComplete());
  REQUIRE_NOTHROW(firstReceiver->federateRestoreComplete());
  REQUIRE_NOTHROW(secondReceiver->federateRestoreComplete());
  drain(*publisher);
  drain(*firstReceiver);
  drain(*secondReceiver);
  REQUIRE(publisherReports.federationRestoredReportCount == 1U);
  REQUIRE(firstReceiverReports.federationRestoredReportCount == 1U);
  REQUIRE(secondReceiverReports.federationRestoredReportCount == 1U);
  firstReceiverReports.callbackOrder.clear();
  secondReceiverReports.callbackOrder.clear();
  auto const verifyReflection = [&](ReportingFederateAmbassador const& reports) {
    REQUIRE(reports.attributeReflectionReports.size() == 1U);
    REQUIRE(reports.flushQueueGrantReports.size() == 1U);
    auto const& report = reports.attributeReflectionReports.front();
    REQUIRE(report.objectInstance == objectInstance);
    REQUIRE(report.attributeValues.size() == 1U);
    REQUIRE(report.attributeValues.contains(attribute));
    REQUIRE(variableLengthDataBytes(report.attributeValues.at(attribute)) ==
            std::vector<unsigned char>(valueBytes, valueBytes + sizeof(valueBytes)));
    REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
            std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
    REQUIRE(report.producingFederate == publisherHandle);
    REQUIRE_FALSE(report.sentRegionsSupplied);
    REQUIRE(report.timeImplementationName == standard_hla::mom::integer64_time);
    REQUIRE(report.timeValue == L"7");
    REQUIRE(report.sentOrderType == rti1516_2025::TIMESTAMP);
    REQUIRE(report.receivedOrderType == rti1516_2025::TIMESTAMP);
    REQUIRE(report.retractionSupplied);
    REQUIRE(report.retractionValid);
  };
  REQUIRE_NOTHROW(firstReceiver->flushQueueRequest(rti1516_2025::HLAinteger64Time(10)));
  REQUIRE_FALSE(firstReceiver->evokeCallback(0.0));
  REQUIRE(secondReceiverReports.attributeReflectionReports.empty());
  REQUIRE_NOTHROW(secondReceiver->flushQueueRequest(rti1516_2025::HLAinteger64Time(10)));
  REQUIRE_FALSE(secondReceiver->evokeCallback(0.0));
  verifyReflection(firstReceiverReports);
  verifyReflection(secondReceiverReports);
  REQUIRE(firstReceiverReports.callbackOrder ==
          std::vector<std::string>{"reflect", "flush-grant"});
  REQUIRE(secondReceiverReports.callbackOrder ==
          std::vector<std::string>{"reflect", "flush-grant"});
  REQUIRE_NOTHROW(publisher->retract(retraction));
  REQUIRE_FALSE(firstReceiver->evokeCallback(0.0));
  REQUIRE_FALSE(secondReceiver->evokeCallback(0.0));
  REQUIRE(firstReceiverReports.requestRetractionReports.size() == 1U);
  REQUIRE(secondReceiverReports.requestRetractionReports.size() == 1U);
  REQUIRE(firstReceiverReports.requestRetractionReports.front().retractionValid);
  REQUIRE(secondReceiverReports.requestRetractionReports.front().retractionValid);
  REQUIRE_THROWS_AS(publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);
  REQUIRE_NOTHROW(firstReceiver->unsubscribeObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(secondReceiver->unsubscribeObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(firstReceiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(secondReceiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(firstReceiver->disconnect());
  REQUIRE_NOTHROW(secondReceiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
}
