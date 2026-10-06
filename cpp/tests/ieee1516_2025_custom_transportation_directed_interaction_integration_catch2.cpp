#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded directed delivery accepts a declared custom FOM transportation",
    "[integration][development-profile][federation-management][mom][mom-request-report]"
    "[fom][transportation-type-lookup][transportation][directed][custom-transportation]"
    "[interaction-management][directed-interaction-custom-transportation-report]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.publish-object-class-directed-interactions]"
    "[rti.service.subscribe-object-class-directed-interactions]"
    "[rti.service.register-object-instance]"
    "[rti.service.send-directed-interaction][rti.service.send-interaction]"
    "[rti.service.subscribe-interaction-class]"
    "[rti.service.evoke-callback]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.receive-directed-interaction]") {
  ReportingFederateAmbassador producerReports;
  ReportingFederateAmbassador consumerReports;
  auto producer = makeRti();
  auto consumer = makeRti();
  auto const federationName = nextFederationName();
  auto const testData = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";
  std::vector<std::wstring> const fomModules{
      (testData / "directed-interaction-object-consumer-fom.xml").wstring(),
      (testData / "transportation-directed-interaction-provider-fom.xml").wstring(),
      (testData / "transportation-reference-provider-fom.xml").wstring(),
  };

  REQUIRE_NOTHROW(producer->connect(producerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(consumer->connect(consumerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(producer->createFederationExecution(
      federationName, fomModules, standard_hla::mom::integer64_time));
  FederateHandle producerHandle;
  FederateHandle consumerHandle;
  REQUIRE_NOTHROW(producerHandle = producer->joinFederationExecution(
      L"custom-directed-producer", L"producer", federationName));
  REQUIRE_NOTHROW(consumerHandle = consumer->joinFederationExecution(
      L"custom-directed-consumer", L"consumer", federationName));

  auto const custom = producer->getTransportationTypeHandle(
      fixture_hla::fixture::umbra_transportation_fixture);
  auto const objectClass = producer->getObjectClassHandle(fixture_hla::fom::directed_fixture_object);
  auto const marker = producer->getAttributeHandle(
      objectClass, fixture_hla::fixture::directed_target_marker);
  auto const interactionClass = producer->getInteractionClassHandle(
      fixture_hla::fom::directed_fixture_interaction);
  REQUIRE(custom.isValid());
  REQUIRE(objectClass.isValid());
  REQUIRE(marker.isValid());
  REQUIRE(interactionClass.isValid());
  InteractionClassHandleSet const directedClasses{interactionClass};
  REQUIRE_NOTHROW(producer->publishObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(consumer->subscribeObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(producer->publishObjectClassDirectedInteractions(objectClass, directedClasses));
  REQUIRE_NOTHROW(consumer->subscribeObjectClassDirectedInteractions(
      objectClass, directedClasses, true));

  ObjectInstanceHandle target;
  REQUIRE_NOTHROW(target = producer->registerObjectInstance(objectClass));
  REQUIRE(target.isValid());
  while (consumer->evokeCallback(0.0)) {
  }
  REQUIRE(consumerReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(consumerReports.objectDiscoveryReports.front().objectInstance == target);

  unsigned char const tagBytes[] = {0x44, 0x49, 0x52, 0x2D, 0x43};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  REQUIRE_NOTHROW(producer->sendDirectedInteraction(
      interactionClass, target, ParameterHandleValueMap{}, tag));
  while (consumer->evokeCallback(0.0)) {
  }

  REQUIRE(consumerReports.directedInteractionReports.size() == 1U);
  auto const& directed = consumerReports.directedInteractionReports.front();
  REQUIRE(directed.interactionClass == interactionClass);
  REQUIRE(directed.objectInstance == target);
  REQUIRE(directed.parameterValues.empty());
  REQUIRE(directed.transportationType == custom);
  REQUIRE(directed.producingFederate == producerHandle);
  REQUIRE(directed.sentOrderType == RECEIVE);
  REQUIRE(directed.receivedOrderType == RECEIVE);
  REQUIRE(variableLengthDataBytes(directed.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));

  auto const reportClass = consumer->getInteractionClassHandle(
      standard_hla::mom::report_directed_interactions_received);
  auto const reportTransportation = consumer->getParameterHandle(
      reportClass, standard_hla::mom::transportation);
  auto const reportInteractionCounts = consumer->getParameterHandle(
      reportClass, L"HLAinteractionCounts");
  auto const requestClass = consumer->getInteractionClassHandle(
      standard_hla::mom::request_directed_interactions_received);
  auto const requestFederate = consumer->getParameterHandle(
      requestClass, standard_hla::mom::federate);
  REQUIRE(reportClass.isValid());
  REQUIRE(reportTransportation.isValid());
  REQUIRE(reportInteractionCounts.isValid());
  REQUIRE(requestClass.isValid());
  REQUIRE(requestFederate.isValid());
  REQUIRE_NOTHROW(consumer->subscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(consumer->sendInteraction(
      requestClass,
      ParameterHandleValueMap{{requestFederate, consumerHandle.encode()}},
      VariableLengthData{}));
  while (consumer->evokeCallback(0.0)) {
  }

  auto const customTransportationBytes = variableLengthDataBytes(custom.encode());
  auto const customReport = std::find_if(
      consumerReports.interactionReports.begin(),
      consumerReports.interactionReports.end(),
      [&](ReportingFederateAmbassador::InteractionReport const& report) {
        return report.interactionClass == reportClass &&
            report.parameterValues.contains(reportTransportation) &&
            variableLengthDataBytes(report.parameterValues.at(reportTransportation)) ==
                customTransportationBytes;
      });
  REQUIRE(customReport != consumerReports.interactionReports.end());
  REQUIRE(customReport->parameterValues.contains(reportInteractionCounts));

  rti1516_2025::HLAvariableArrayT<rti1516_2025::HLAoctet> interactionClassHandlePrototype;
  rti1516_2025::HLAfixedRecord interactionCountPrototype;
  interactionCountPrototype.appendElement(interactionClassHandlePrototype)
      .appendElement(rti1516_2025::HLAinteger32BE{});
  rti1516_2025::HLAvariableArray interactionCounts{interactionCountPrototype};
  REQUIRE_NOTHROW(interactionCounts.decode(
      customReport->parameterValues.at(reportInteractionCounts)));
  auto const encodedInteractionClass = variableLengthDataBytes(interactionClass.encode());
  bool foundDirectedClassCount = false;
  for (std::size_t index = 0; index != interactionCounts.size(); ++index) {
    auto const& record = dynamic_cast<rti1516_2025::HLAfixedRecord const&>(
        interactionCounts.get(index));
    auto const& encodedClass = dynamic_cast<rti1516_2025::HLAvariableArray const&>(record.get(0));
    if (variableLengthDataBytes(encodedClass.encode()) == encodedInteractionClass) {
      REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(record.get(1)).get() == 1);
      foundDirectedClassCount = true;
    }
  }
  REQUIRE(foundDirectedClassCount);

  REQUIRE_NOTHROW(consumer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(producer->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(producer->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(consumer->disconnect());
  REQUIRE_NOTHROW(producer->disconnect());
}

}  // namespace
