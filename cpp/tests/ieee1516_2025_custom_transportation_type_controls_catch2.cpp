#include "ieee1516_2025_federation_management_fixture_support.hpp"

TEST_CASE(
    "Embedded transportation-type controls accept declared FOM transportation handles",
    "[integration][development-profile][federation-management][transportation]"
    "[fom][transportation-management][custom-transportation]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.change-default-attribute-transportation-type]"
    "[rti.service.request-attribute-transportation-type-change]"
    "[rti.service.query-attribute-transportation-type]"
    "[rti.service.request-interaction-transportation-type-change]"
    "[rti.service.query-interaction-transportation-type]"
    "[rti.service.publish-object-class-attributes][rti.service.subscribe-object-class-attributes]"
    "[rti.service.publish-interaction-class][rti.service.subscribe-interaction-class]"
    "[rti.service.register-object-instance][rti.service.send-interaction]"
    "[rti.service.evoke-callback]"
    "[federate.callback.confirm-attribute-transportation-type-change]"
    "[federate.callback.report-attribute-transportation-type]"
    "[federate.callback.confirm-interaction-transportation-type-change]"
    "[federate.callback.report-interaction-transportation-type]"
    "[federate.callback.receive-interaction]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador peerReports;
  auto owner = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const testData = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";
  std::vector<std::wstring> const fomModules{
      (testData / "transportation-reference-consumer-fom.xml").wstring(),
      (testData / "transportation-reference-provider-fom.xml").wstring(),
  };

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(peer->connect(peerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModules, standard_hla::mom::integer64_time));
  FederateHandle ownerHandle;
  REQUIRE_NOTHROW(ownerHandle = owner->joinFederationExecution(
      L"custom-transport-control-owner", L"owner", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(
      L"custom-transport-control-peer", L"peer", federationName));

  auto const ownerObjectClass = owner->getObjectClassHandle(
      fixture_hla::fom::transportation_fixture_object);
  auto const peerObjectClass = peer->getObjectClassHandle(
      fixture_hla::fom::transportation_fixture_object);
  auto const ownerAttribute = owner->getAttributeHandle(
      ownerObjectClass,
      fixture_hla::fixture::umbra_transportation_fixture_attribute);
  auto const peerAttribute = peer->getAttributeHandle(
      peerObjectClass,
      fixture_hla::fixture::umbra_transportation_fixture_attribute);
  auto const ownerInteraction = owner->getInteractionClassHandle(
      fixture_hla::fom::transportation_fixture_interaction);
  auto const peerInteraction = peer->getInteractionClassHandle(
      fixture_hla::fom::transportation_fixture_interaction);
  auto const reliable = owner->getTransportationTypeHandle(standard_hla::mom::reliable);
  auto const custom = owner->getTransportationTypeHandle(
      fixture_hla::fixture::umbra_transportation_fixture);
  REQUIRE(ownerObjectClass.isValid());
  REQUIRE(peerObjectClass.isValid());
  REQUIRE(ownerAttribute.isValid());
  REQUIRE(peerAttribute.isValid());
  REQUIRE(ownerInteraction.isValid());
  REQUIRE(peerInteraction.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE(custom.isValid());
  REQUIRE(custom != reliable);

  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
      ownerObjectClass,
      AttributeHandleSet{ownerAttribute}));
  REQUIRE_NOTHROW(peer->subscribeObjectClassAttributes(
      peerObjectClass,
      AttributeHandleSet{peerAttribute}));
  REQUIRE_NOTHROW(owner->publishInteractionClass(ownerInteraction));
  REQUIRE_NOTHROW(peer->subscribeInteractionClass(peerInteraction));

  // Establish a distinguishable old value first. The FOM's declared value is
  // custom; the first registered object therefore captures the standard
  // value, and the later custom change exercises the execution catalog lookup
  // in Change Default Attribute Transportation Type.
  REQUIRE_NOTHROW(owner->changeDefaultAttributeTransportationType(
      ownerObjectClass,
      AttributeHandleSet{ownerAttribute},
      reliable));
  ObjectInstanceHandle firstObject;
  REQUIRE_NOTHROW(firstObject = owner->registerObjectInstance(ownerObjectClass));
  while (peer->evokeCallback(0.0)) {
  }
  REQUIRE(peerReports.objectDiscoveryReports.size() == 1U);

  REQUIRE_NOTHROW(owner->changeDefaultAttributeTransportationType(
      ownerObjectClass,
      AttributeHandleSet{ownerAttribute},
      custom));
  ObjectInstanceHandle secondObject;
  REQUIRE_NOTHROW(secondObject = owner->registerObjectInstance(ownerObjectClass));
  while (peer->evokeCallback(0.0)) {
  }
  REQUIRE(peerReports.objectDiscoveryReports.size() == 2U);
  REQUIRE_NOTHROW(owner->queryAttributeTransportationType(secondObject, ownerAttribute));
  while (owner->evokeCallback(0.0)) {
  }
  REQUIRE_FALSE(ownerReports.attributeTransportationTypeReports.empty());
  REQUIRE(ownerReports.attributeTransportationTypeReports.back().objectInstance == secondObject);
  REQUIRE(ownerReports.attributeTransportationTypeReports.back().transportationType == custom);

  // The instance-level request uses the same execution-scoped custom handle
  // and commits it at the confirmation callback boundary.
  REQUIRE_NOTHROW(owner->requestAttributeTransportationTypeChange(
      firstObject,
      AttributeHandleSet{ownerAttribute},
      custom));
  REQUIRE(ownerReports.attributeTransportationTypeChangeReports.empty());
  while (owner->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.attributeTransportationTypeChangeReports.size() == 1U);
  REQUIRE(ownerReports.attributeTransportationTypeChangeReports.back().objectInstance == firstObject);
  REQUIRE(ownerReports.attributeTransportationTypeChangeReports.back().transportationType == custom);
  REQUIRE_NOTHROW(owner->queryAttributeTransportationType(firstObject, ownerAttribute));
  while (owner->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.attributeTransportationTypeReports.size() == 2U);
  REQUIRE(ownerReports.attributeTransportationTypeReports.back().transportationType == custom);

  // Interaction transportation changes use the same catalog-backed handle
  // resolution and remain callback-gated. Prove the custom value is effective
  // for a subsequent send and for the peer's public query callback.
  REQUIRE_NOTHROW(owner->requestInteractionTransportationTypeChange(
      ownerInteraction,
      reliable));
  while (owner->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.interactionTransportationTypeChangeReports.size() == 1U);
  REQUIRE(ownerReports.interactionTransportationTypeChangeReports.back().transportationType == reliable);
  REQUIRE_NOTHROW(owner->requestInteractionTransportationTypeChange(
      ownerInteraction,
      custom));
  while (owner->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.interactionTransportationTypeChangeReports.size() == 2U);
  REQUIRE(ownerReports.interactionTransportationTypeChangeReports.back().transportationType == custom);

  REQUIRE_NOTHROW(owner->sendInteraction(
      ownerInteraction,
      ParameterHandleValueMap{},
      VariableLengthData{}));
  while (peer->evokeCallback(0.0)) {
  }
  REQUIRE(peerReports.interactionReports.size() == 1U);
  REQUIRE(peerReports.interactionReports.front().transportationType == custom);

  REQUIRE_NOTHROW(peer->queryInteractionTransportationType(ownerHandle, peerInteraction));
  while (peer->evokeCallback(0.0)) {
  }
  REQUIRE(peerReports.interactionTransportationTypeReports.size() == 1U);
  REQUIRE(peerReports.interactionTransportationTypeReports.front().transportationType == custom);

  REQUIRE_NOTHROW(peer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(peer->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}
