#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded timestamped Update Attribute Values splits immediate delivery and pending retraction",
    "[integration][development-profile][federation-management][object-management][time-management]"
    "[timestamped-attribute-update][multi-federate-callback-ordering][mixed-recipient-time-boundary][tso]"
    "[request-retraction-attribute-update-mixed-fanout]"
    "[rti.service.update-attribute-values][rti.service.retract][rti.service.time-advance-request]"
    "[rti.service.subscribe-object-class-attributes][rti.service.change-default-attribute-order-type]"
    "[federate.callback.reflect-attribute-values][federate.callback.request-retraction]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador constrainedReports;
  ReportingFederateAmbassador immediateReports;
  auto publisher = makeRti();
  auto constrained = makeRti();
  auto immediate = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "attribute-update-passel-fom.xml")
          .wstring();
  unsigned char const valueBytes[] = {0x44, 0x55};
  unsigned char const tagBytes[] = {0x4D, 0x49, 0x58};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  VariableLengthData const value(valueBytes, sizeof(valueBytes));
  rti1516_2025::HLAinteger64Time const timestamp(6);

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(constrained->connect(constrainedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(immediate->connect(immediateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timestamped-update-mixed-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(constrained->joinFederationExecution(
      L"timestamped-update-mixed-constrained", L"subscriber", federationName));
  REQUIRE_NOTHROW(immediate->joinFederationExecution(
      L"timestamped-update-mixed-immediate", L"subscriber", federationName));

  auto const objectClass = publisher->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const attribute = publisher->getAttributeHandle(
      objectClass,
      fixture_hla::fixture::reliable_base_a);
  auto const reliableTransportation = publisher->getTransportationTypeHandle(
      standard_hla::mom::reliable);
  REQUIRE(objectClass.isValid());
  REQUIRE(attribute.isValid());
  REQUIRE(reliableTransportation.isValid());
  AttributeHandleSet const attributes{attribute};
  AttributeHandleValueMap const values{{attribute, value}};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(constrained->subscribeObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(immediate->subscribeObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(
      objectClass,
      attributes,
      TIMESTAMP));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(objectClass));
  REQUIRE_FALSE(constrained->evokeCallback(0.0));
  REQUIRE_FALSE(immediate->evokeCallback(0.0));
  REQUIRE(constrainedReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(immediateReports.objectDiscoveryReports.size() == 1U);
  // Both recipients can enqueue provider-side Auto Provide callbacks during
  // discovery. Keep that setup work out of the time-management assertion.
  while (publisher->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(constrained->enableTimeConstrained());
  REQUIRE_FALSE(constrained->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  constrainedReports.callbackOrder.clear();
  immediateReports.callbackOrder.clear();
  auto const retraction = publisher->updateAttributeValues(
      objectInstance,
      values,
      tag,
      timestamp);
  REQUIRE(retraction.isValid());
  REQUIRE(constrainedReports.attributeReflectionReports.empty());
  REQUIRE(immediateReports.attributeReflectionReports.empty());

  // The non-time-constrained recipient is delivered immediately, while the
  // constrained recipient retains the same passel in its temporal queue.
  REQUIRE_FALSE(immediate->evokeCallback(0.0));
  REQUIRE(immediateReports.attributeReflectionReports.size() == 1U);
  REQUIRE(immediateReports.requestRetractionReports.empty());
  REQUIRE(immediateReports.callbackOrder == std::vector<std::string>{"reflect"});
  auto const& immediateReflection =
      immediateReports.attributeReflectionReports.front();
  REQUIRE(immediateReflection.objectInstance == objectInstance);
  REQUIRE(immediateReflection.attributeValues.size() == 1U);
  REQUIRE(immediateReflection.attributeValues.contains(attribute));
  REQUIRE(variableLengthDataBytes(immediateReflection.attributeValues.at(attribute)) ==
          std::vector<unsigned char>(valueBytes, valueBytes + sizeof(valueBytes)));
  REQUIRE(variableLengthDataBytes(immediateReflection.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(immediateReflection.transportationType == reliableTransportation);
  REQUIRE(immediateReflection.producingFederate == publisherHandle);
  REQUIRE_FALSE(immediateReflection.sentRegionsSupplied);
  REQUIRE(immediateReflection.sentRegions.empty());
  REQUIRE(immediateReflection.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(immediateReflection.timeValue == timestamp.toString());
  REQUIRE(immediateReflection.sentOrderType == TIMESTAMP);
  REQUIRE(immediateReflection.receivedOrderType == RECEIVE);
  REQUIRE(immediateReflection.retractionSupplied);
  REQUIRE(immediateReflection.retractionValid);
  REQUIRE(variableLengthDataBytes(immediateReflection.encodedRetraction) ==
          variableLengthDataBytes(retraction.encode()));
  REQUIRE(constrainedReports.callbackOrder.empty());

  // Retract after immediate delivery: only that delivered recipient receives
  // Request Retraction; the constrained queue is withdrawn before its grant.
  REQUIRE_NOTHROW(constrained->timeAdvanceRequest(timestamp));
  REQUIRE_NOTHROW(publisher->retract(retraction));
  REQUIRE(constrainedReports.callbackOrder.empty());
  REQUIRE_FALSE(immediate->evokeCallback(0.0));
  REQUIRE(immediateReports.attributeReflectionReports.size() == 1U);
  REQUIRE(immediateReports.requestRetractionReports.size() == 1U);
  auto const& requestRetraction = immediateReports.requestRetractionReports.front();
  REQUIRE(requestRetraction.retractionValid);
  REQUIRE(variableLengthDataBytes(requestRetraction.encodedRetraction) ==
          variableLengthDataBytes(retraction.encode()));
  REQUIRE(immediateReports.callbackOrder ==
          std::vector<std::string>{"reflect", "request-retraction"});

  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(constrained->evokeCallback(0.0));
  REQUIRE(constrainedReports.attributeReflectionReports.empty());
  REQUIRE(constrainedReports.requestRetractionReports.empty());
  REQUIRE(constrainedReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(constrainedReports.timeAdvanceGrantReports.front().implementationName ==
          standard_hla::mom::integer64_time);
  REQUIRE(constrainedReports.timeAdvanceGrantReports.front().value == timestamp.toString());
  REQUIRE(constrainedReports.callbackOrder == std::vector<std::string>{"grant"});
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(constrained->unsubscribeObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(immediate->unsubscribeObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(constrained->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(immediate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(constrained->disconnect());
  REQUIRE_NOTHROW(immediate->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
}
