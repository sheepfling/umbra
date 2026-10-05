#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded timestamped Update Attribute Values requests retraction for an immediate-only recipient",
    "[integration][development-profile][object-management][time-management][tso]"
    "[request-retraction-attribute-update][callbacks][passels]"
    "[rti.service.update-attribute-values][rti.service.retract]"
    "[rti.service.change-default-attribute-order-type]"
    "[rti.service.subscribe-object-class-attributes]"
    "[federate.callback.reflect-attribute-values][federate.callback.request-retraction]"
    "[timestamped-update-retraction-immediate-only-recipient]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "attribute-update-passel-fom.xml")
          .wstring();
  unsigned char const tagBytes[] = {0x52, 0x45, 0x54, 0x52};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  unsigned char const reliableBytes[] = {0x11, 0x22};
  unsigned char const bestEffortBytes[] = {0x33, 0x44, 0x55};

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timestamped-update-retraction-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-update-retraction-immediate", L"subscriber", federationName));

  auto const objectClass = publisher->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const reliable = publisher->getAttributeHandle(
      objectClass,
      fixture_hla::fixture::reliable_base_a);
  auto const bestEffort = publisher->getAttributeHandle(
      objectClass,
      fixture_hla::fixture::best_effort_base);
  auto const reliableTransportation = publisher->getTransportationTypeHandle(
      standard_hla::mom::reliable);
  auto const bestEffortTransportation = publisher->getTransportationTypeHandle(
      standard_hla::mom::best_effort);
  REQUIRE(objectClass.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE(bestEffort.isValid());
  REQUIRE(reliableTransportation.isValid());
  REQUIRE(bestEffortTransportation.isValid());

  AttributeHandleSet const attributes{reliable, bestEffort};
  AttributeHandleValueMap const values{
      {reliable, VariableLengthData(reliableBytes, sizeof(reliableBytes))},
      {bestEffort, VariableLengthData(bestEffortBytes, sizeof(bestEffortBytes))},
  };
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(
      objectClass,
      attributes,
      TIMESTAMP));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(objectClass));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(receiverReports.objectDiscoveryReports.front().objectInstance == objectInstance);
  // Auto Provide is a discovery-time callback on the provider's queue; drain
  // it before the time-regulation callback assertion.
  while (publisher->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  auto const retraction = publisher->updateAttributeValues(
      objectInstance,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  REQUIRE(receiverReports.attributeReflectionReports.empty());
  receiverReports.callbackOrder.clear();
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.attributeReflectionReports.size() == 2U);
  REQUIRE(receiverReports.requestRetractionReports.empty());
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"reflect", "reflect"});

  bool reliableReported = false;
  bool bestEffortReported = false;
  for (auto const& reflection : receiverReports.attributeReflectionReports) {
    REQUIRE(reflection.objectInstance == objectInstance);
    REQUIRE(reflection.attributeValues.size() == 1U);
    REQUIRE(reflection.producingFederate == publisherHandle);
    REQUIRE(variableLengthDataBytes(reflection.userSuppliedTag) ==
            std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
    REQUIRE_FALSE(reflection.sentRegionsSupplied);
    REQUIRE(reflection.sentRegions.empty());
    REQUIRE(reflection.timeImplementationName == standard_hla::mom::integer64_time);
    REQUIRE(reflection.timeValue == L"6");
    REQUIRE(reflection.sentOrderType == TIMESTAMP);
    REQUIRE(reflection.receivedOrderType == RECEIVE);
    REQUIRE(reflection.retractionSupplied);
    REQUIRE(reflection.retractionValid);
    REQUIRE(variableLengthDataBytes(reflection.encodedRetraction) ==
            variableLengthDataBytes(retraction.encode()));
    if (reflection.transportationType == reliableTransportation) {
      reliableReported = reflection.attributeValues.contains(reliable);
      REQUIRE(variableLengthDataBytes(reflection.attributeValues.at(reliable)) ==
              std::vector<unsigned char>(
                  reliableBytes,
                  reliableBytes + sizeof(reliableBytes)));
    } else if (reflection.transportationType == bestEffortTransportation) {
      bestEffortReported = reflection.attributeValues.contains(bestEffort);
      REQUIRE(variableLengthDataBytes(reflection.attributeValues.at(bestEffort)) ==
              std::vector<unsigned char>(
                  bestEffortBytes,
                  bestEffortBytes + sizeof(bestEffortBytes)));
    } else {
      FAIL("Unexpected timestamped attribute transportation type");
    }
  }
  REQUIRE(reliableReported);
  REQUIRE(bestEffortReported);

  // The non-time-constrained recipient has already received both passels, so
  // Retract must produce one recipient-local Request Retraction callback for
  // the shared designator rather than another reflection.
  REQUIRE_NOTHROW(publisher->retract(retraction));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.attributeReflectionReports.size() == 2U);
  REQUIRE(receiverReports.requestRetractionReports.size() == 1U);
  auto const& requestRetraction = receiverReports.requestRetractionReports.front();
  REQUIRE(requestRetraction.retractionValid);
  REQUIRE(variableLengthDataBytes(requestRetraction.encodedRetraction) ==
          variableLengthDataBytes(retraction.encode()));
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"reflect", "reflect", "request-retraction"});
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
}  // namespace
